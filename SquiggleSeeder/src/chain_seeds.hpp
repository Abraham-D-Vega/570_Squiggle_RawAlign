#ifndef CHAINSEEDS_HPP
#define CHAINSEEDS_HPP

#include "utils.hpp"

#include <vector>
#include <algorithm>
#include <cstdint>
#include <utility>
#include <cmath>
#include <thread>
#include <atomic>

void chain_seeds(
    const std::vector<Anchor>& seeds,
    std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& chains,
    uint32_t WINDOW_SIZE         = 2000,
    float    LAMBDA              = 0.01f,   // penalty per unit deviation from slope 1
    std::size_t max_chains       = 5        // top-K chains to extract
)
{
    chains.clear();
    if (seeds.empty()) return;

    const std::size_t n = seeds.size();
    if (n == 0) return;

    // seeds are assumed sorted by (r, q) before calling this function

    // -----------------------------
    // 2. Build overlapping segments in reference space
    //    Example pattern: [0, 400000+WINDOW_SIZE), [400000, 800000+WINDOW_SIZE), ...
    // -----------------------------
    uint32_t r_min = seeds.front().r;
    uint32_t r_max = seeds.back().r;

    unsigned hw_threads = std::max(std::thread::hardware_concurrency(), 4u);

    const uint32_t SEG_STRIDE   = 400'000;                 // distance between segment starts
    const uint32_t SEG_OVERLAP  = WINDOW_SIZE;             // must be >= WINDOW_SIZE
    const uint32_t SEG_SIZE     = SEG_STRIDE + SEG_OVERLAP; // total width of each segment

    struct Segment {
        uint32_t    r_lo;
        uint32_t    r_hi;
        std::size_t begin; // index into seeds
        std::size_t end;   // index into seeds
    };

    std::vector<Segment> segments;

    // Start segments from the floor of r_min to the stride grid
    uint32_t start_r0 = (r_min / SEG_STRIDE) * SEG_STRIDE;
    for (uint32_t seg_start_r = start_r0;
         seg_start_r <= r_max;
         seg_start_r += SEG_STRIDE)
    {
        uint32_t seg_lo = seg_start_r;
        uint32_t seg_hi = seg_start_r + SEG_SIZE; // half-open [lo, hi)

        // Find [begin, end) of seeds with r in [seg_lo, seg_hi)
        auto cmp_lo = [](const Anchor &a, uint32_t val) { return a.r < val; };

        auto it_begin = std::lower_bound(seeds.begin(), seeds.end(), seg_lo, cmp_lo);
        auto it_end   = std::lower_bound(seeds.begin(), seeds.end(), seg_hi, cmp_lo);

        std::size_t b = static_cast<std::size_t>(it_begin - seeds.begin());
        std::size_t e = static_cast<std::size_t>(it_end   - seeds.begin());

        if (b < e) {
            segments.push_back(Segment{seg_lo, seg_hi, b, e});
        }
    }

    if (segments.empty()) return;

    // -----------------------------
    // 3. Per-segment DP + local chain extraction (ONE chain per segment)
    //    We only keep (start, end) anchors, no full backtrack.
    // -----------------------------
    struct SegmentChain {
        float    score;
        uint32_t q_start, r_start;
        uint32_t q_end,   r_end;
    };

    std::vector<std::vector<SegmentChain>> segment_chains(segments.size());

    // Thread pool over segments
    std::atomic<std::size_t> next_seg(0);

    const std::size_t num_threads =
        std::max<std::size_t>(1, std::min<std::size_t>(segments.size(), hw_threads));

    auto worker = [&]() {
        const uint32_t MAX_DEV = WINDOW_SIZE;

        while (true) {
            std::size_t si = next_seg.fetch_add(1, std::memory_order_relaxed);
            if (si >= segments.size()) break;

            const Segment &seg = segments[si];
            const std::size_t s   = seg.begin;
            const std::size_t e   = seg.end;
            const std::size_t len = e - s;
            if (len == 0) continue;

            // Local DP arrays for this segment
            std::vector<float>    score(len);
            std::vector<uint32_t> start_idx(len); // local start index of chain ending at ip

            std::size_t j_start = 0; // local index [0..len)

            // DP over local indices ip = 0..len-1, mapping to global i = s + ip
            for (std::size_t ip = 0; ip < len; ++ip) {
                std::size_t i = s + ip;
                uint32_t qi = seeds[i].q;
                uint32_t ri = seeds[i].r;

                // Maintain r-window within segment: seeds[j].r in [ri - WINDOW_SIZE, ri]
                while (j_start < ip) {
                    std::size_t jg = s + j_start;
                    if (ri - seeds[jg].r > WINDOW_SIZE)
                        ++j_start;
                    else
                        break;
                }

                // Baseline: start a new chain at i (start=end=i)
                float    best_score    = 1.0f;
                uint32_t best_start_ip = static_cast<uint32_t>(ip);

                // Inner loop: try extending from j -> i
                for (std::size_t jp = j_start; jp < ip; ++jp) {
                    std::size_t j = s + jp;
                    uint32_t qj = seeds[j].q;
                    uint32_t rj = seeds[j].r;

                    // Enforce monotone q and r (r monotone already from sorting, q check remains)
                    if (!(qj < qi && rj < ri)) continue;

                    // Compute span in reference from start of chain ending at jp to i
                    std::size_t global_start = s + start_idx[jp];
                    uint32_t r_start = seeds[global_start].r;
                    uint32_t span_r  = ri - r_start;
                    if (span_r > WINDOW_SIZE) continue;

                    uint32_t dq = qi - qj;
                    uint32_t dr = ri - rj;
                    if (dq == 0 || dr == 0) continue;

                    int dev = std::abs(static_cast<int>(dq) - static_cast<int>(dr));
                    if (static_cast<uint32_t>(dev) > MAX_DEV) continue;

                    float candidate = score[jp] + 1.0f - LAMBDA * static_cast<float>(dev);
                    if (candidate > best_score) {
                        best_score    = candidate;
                        best_start_ip = static_cast<uint32_t>(start_idx[jp]);
                    }
                }

                score[ip]    = best_score;
                start_idx[ip] = best_start_ip;
            }

            // ---- ONE chain per segment: pick the single best endpoint ----
            int   best_ip        = -1;
            float best_seg_score = 0.0f;

            for (std::size_t ip = 0; ip < len; ++ip) {
                if (ip == 0 || score[ip] > best_seg_score) {
                    best_seg_score = score[ip];
                    best_ip        = static_cast<int>(ip);
                }
            }

            if (best_ip == -1 || best_seg_score <= 0.0f) {
                continue; // No useful chain in this segment
            }

            // Extract just start and end anchors (no backtracking)
            std::size_t end_local   = static_cast<std::size_t>(best_ip);
            std::size_t start_local = static_cast<std::size_t>(start_idx[end_local]);

            std::size_t g_start = s + start_local;
            std::size_t g_end   = s + end_local;

            const Anchor &a_start = seeds[g_start];
            const Anchor &a_end   = seeds[g_end];

            SegmentChain sc;
            sc.score   = best_seg_score;
            sc.q_start = a_start.q;
            sc.r_start = a_start.r;
            sc.q_end   = a_end.q;
            sc.r_end   = a_end.r;

            segment_chains[si].push_back(sc); // exactly 0 or 1 per segment
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (std::size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back(worker);
    }
    for (auto &th : threads) th.join();

    // -----------------------------
    // 4. Merge all segment chains & pick global best max_chains
    //    Each chain is represented only by its (start, end) anchors.
    // -----------------------------
    std::vector<SegmentChain> all_chains;
    all_chains.reserve(segments.size());
    for (auto &vec : segment_chains) {
        if (!vec.empty()) {
            all_chains.push_back(vec[0]); // 0 or 1 per segment
        }
    }

    if (all_chains.empty()) return;

    // Sort by score descending
    std::sort(all_chains.begin(), all_chains.end(),
              [](const SegmentChain &a, const SegmentChain &b) {
                  return a.score > b.score;
              });

    std::size_t taken = 0;
    for (const auto &sc : all_chains) {
        if (taken >= max_chains) break;

        std::vector<std::pair<uint32_t, uint32_t>> chain;
        chain.reserve(2);
        chain.emplace_back(sc.q_start, sc.r_start);
        chain.emplace_back(sc.q_end,   sc.r_end);

        chains.push_back(std::move(chain));
        ++taken;
    }
}

#endif // CHAINSEEDS_HPP
