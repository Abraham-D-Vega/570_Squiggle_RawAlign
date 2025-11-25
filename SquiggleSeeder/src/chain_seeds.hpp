#ifndef CHAINSEEDS_HPP
#define CHAINSEEDS_HPP

#include "utils.hpp"

#include <vector>
#include <algorithm>
#include <cstdint>
#include <utility>
#include <limits>
#include <cmath>
#include <iostream>
#include <thread>
#include <atomic>
#include <unordered_set>

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

    // -----------------------------
    // 2. Build overlapping segments in reference space
    //    Example pattern: [0,110000), [100000,210000), ...
    // -----------------------------
    if (n == 0) return;

    uint32_t r_min = seeds.front().r;
    uint32_t r_max = seeds.back().r;

    unsigned hw_threads = std::max(std::thread::hardware_concurrency(), 4u);

    // You can tune these:
    const uint32_t SEG_STRIDE   = 400'000;               // distance between segment starts
    const uint32_t SEG_OVERLAP  = WINDOW_SIZE;          // must be >= WINDOW_SIZE
    const uint32_t SEG_SIZE     = SEG_STRIDE + SEG_OVERLAP; // total width of each segment

    struct Segment {
        uint32_t r_lo;
        uint32_t r_hi;
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
        auto cmp_hi = [](uint32_t val, const Anchor &a) { return val < a.r; };

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
    // 3. Per-segment DP + local chain extraction
    // -----------------------------
    struct SegmentChain {
        float score;  // chain score (score at endpoint)
        std::vector<std::pair<uint32_t,uint32_t>> anchors; // (q,r) pairs
    };

    std::vector<std::vector<SegmentChain>> segment_chains(segments.size());

    // Thread pool over segments
    std::atomic<std::size_t> next_seg(0);

    const std::size_t num_threads =
        std::max<std::size_t>(1, std::min<std::size_t>(segments.size(),
                                                       hw_threads));

    auto worker = [&]() {
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
            std::vector<int32_t>  prev(len);
            std::vector<uint32_t> min_r(len), max_r(len);

            const uint32_t MAX_DEV = WINDOW_SIZE;
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

                // Baseline: start a new chain at i
                float    best_score = 1.0f;
                int32_t  best_prev  = -1;
                uint32_t best_min_r = ri;
                uint32_t best_max_r = ri;

                // Inner loop: try extending from j -> i
                for (std::size_t jp = j_start; jp < ip; ++jp) {
                    std::size_t j = s + jp;
                    uint32_t qj = seeds[j].q;
                    uint32_t rj = seeds[j].r;

                    // Enforce monotone q and r
                    if (!(qj < qi && rj < ri)) continue;

                    // New span in r if we extend chain ending at j with i
                    uint32_t new_min_r = std::min(min_r[jp], ri);
                    uint32_t new_max_r = std::max(max_r[jp], ri);
                    if (new_max_r - new_min_r > WINDOW_SIZE) continue;

                    uint32_t dq = qi - qj;
                    uint32_t dr = ri - rj;
                    if (dq == 0 || dr == 0) continue;

                    int dev = std::abs(static_cast<int>(dq) - static_cast<int>(dr));
                    if (static_cast<uint32_t>(dev) > MAX_DEV) continue;

                    float candidate = score[jp] + 1.0f - LAMBDA * static_cast<float>(dev);
                    if (candidate > best_score) {
                        best_score = candidate;
                        best_prev  = static_cast<int32_t>(jp); // local index
                        best_min_r = new_min_r;
                        best_max_r = new_max_r;
                    }
                }

                score[ip] = best_score;
                prev[ip]  = best_prev;
                min_r[ip] = best_min_r;
                max_r[ip] = best_max_r;
            }

            // Local chain selection within this segment (similar to your global one)
            std::vector<int> order(len);
            for (std::size_t ip = 0; ip < len; ++ip) order[ip] = static_cast<int>(ip);

            const std::size_t rawK = max_chains * 20; // oversample for overlaps
            const std::size_t L    = std::min<std::size_t>(rawK, len);

            std::partial_sort(order.begin(),
                              order.begin() + L,
                              order.end(),
                              [&](int a, int b) { return score[a] > score[b]; });

            std::vector<uint8_t> used(len, 0);
            std::size_t chains_found = 0;
            auto &out_vec = segment_chains[si];

            for (std::size_t idx = 0; idx < L && chains_found < max_chains; ++idx) {
                int ip = order[idx];
                if (score[ip] <= 0.0f) break; // no more good chains in this segment

                // Check local overlap in this segment
                bool overlaps = false;
                int cur = ip;
                while (cur != -1) {
                    if (used[cur]) {
                        overlaps = true;
                        break;
                    }
                    cur = prev[cur];
                }
                if (overlaps) continue;

                // Backtrack to build chain in global coordinates
                std::vector<std::pair<uint32_t,uint32_t>> chain;
                cur = ip;
                while (cur != -1) {
                    std::size_t gi = s + static_cast<std::size_t>(cur);
                    chain.emplace_back(seeds[gi].q, seeds[gi].r);
                    used[cur] = 1;
                    cur = prev[cur];
                }
                std::reverse(chain.begin(), chain.end());

                if (!chain.empty()) {
                    SegmentChain sc;
                    sc.score   = score[ip];
                    sc.anchors = std::move(chain);
                    out_vec.push_back(std::move(sc));
                    ++chains_found;
                }
            }
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
    //    Ensure we don't reuse the same (q,r) anchor across chains.
    // -----------------------------
    std::vector<SegmentChain> all_chains;
    for (auto &vec : segment_chains) {
        for (auto &c : vec) {
            all_chains.push_back(std::move(c));
        }
    }

    if (all_chains.empty()) return;

    std::sort(all_chains.begin(), all_chains.end(),
              [](const SegmentChain &a, const SegmentChain &b) {
                  return a.score > b.score;
              });

    std::unordered_set<uint64_t> used_anchors;
    used_anchors.reserve(all_chains.size() * 16);

    auto encode_anchor = [](uint32_t q, uint32_t r) -> uint64_t {
        return (static_cast<uint64_t>(q) << 32) | static_cast<uint64_t>(r);
    };

    std::size_t taken = 0;
    for (const auto &sc : all_chains) {
        if (taken >= max_chains) break;

        bool overlaps = false;
        for (const auto &p : sc.anchors) {
            uint64_t key = encode_anchor(p.first, p.second);
            if (used_anchors.find(key) != used_anchors.end()) {
                overlaps = true;
                break;
            }
        }
        if (overlaps) continue;

        // Accept this chain
        chains.push_back(sc.anchors);
        ++taken;

        for (const auto &p : sc.anchors) {
            uint64_t key = encode_anchor(p.first, p.second);
            used_anchors.insert(key);
        }
    }
}

#endif // CHAINSEEDS_HPP

