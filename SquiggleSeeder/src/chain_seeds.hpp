#ifndef CHAINSEEDS_HPP
#define CHAINSEEDS_HPP

#include <vector>
#include <algorithm>
#include <cstdint>
#include <utility>
#include <limits>
#include <cmath>

void chain_seeds(
    const std::vector<std::vector<uint32_t> >& anchors,
    std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& chains,
    uint32_t WINDOW_SIZE         = 2000,
    double   LAMBDA              = 0.01,   // penalty per unit deviation from slope 1
    std::size_t max_chains       = 5       // top-K chains to peel off
)
{
    chains.clear();

    // -----------------------------
    // 1. Flatten anchors -> seeds
    // -----------------------------
    struct Anchor {
        uint32_t q;
        uint32_t r;
    };

    std::vector<Anchor> seeds;
    std::size_t total = 0;
    for (const auto &v : anchors) total += v.size();
    seeds.reserve(total);

    for (uint32_t q = 0; q < anchors.size(); ++q) {
        for (uint32_t r : anchors[q]) {
            seeds.push_back({q, r});
        }
    }

    if (seeds.empty()) return;

    // Sort by *reference* position, then q.
    // This lets us maintain a sliding window in r.
    std::sort(seeds.begin(), seeds.end(),
              [](const Anchor &a, const Anchor &b) {
                  if (a.r != b.r) return a.r < b.r;
                  return a.q < b.q;
              });

    const std::size_t n = seeds.size();

    // Split into separate arrays for hardware friendliness.
    std::vector<uint32_t> qs(n), rs(n);
    for (std::size_t i = 0; i < n; ++i) {
        qs[i] = seeds[i].q;
        rs[i] = seeds[i].r;
    }

    // Used flags (uint8_t instead of vector<bool>).
    std::vector<uint8_t> used(n, 0);

    // DP buffers reused across peel-off iterations.
    std::vector<double>   score(n);
    std::vector<int32_t>  prev(n);
    std::vector<uint32_t> min_r(n), max_r(n);

    const double   NEG_INF  = -std::numeric_limits<double>::infinity();
    const uint32_t MAX_DEV  = WINDOW_SIZE;  // hard cutoff on |dq - dr|

    const uint32_t *qs_ptr = qs.data();
    const uint32_t *rs_ptr = rs.data();

    auto run_dp_iteration = [&]() -> int {
        // j_start: smallest index that might still be within WINDOW_SIZE in r.
        std::size_t j_start = 0;

        for (std::size_t i = 0; i < n; ++i) {
            if (used[i]) {
                score[i] = NEG_INF;
                prev[i]  = -1;
                min_r[i] = max_r[i] = rs_ptr[i];
                continue;
            }

            const uint32_t qi = qs_ptr[i];
            const uint32_t ri = rs_ptr[i];

            // Slide j_start so that for all j < j_start: ri - rj > WINDOW_SIZE.
            // Those can never pass the pairwise span filter (ri - rj > WINDOW_SIZE).
            while (j_start < i && (ri - rs_ptr[j_start] > WINDOW_SIZE)) {
                ++j_start;
            }

            // Start a new chain at i.
            score[i] = 1.0;
            prev[i]  = -1;
            min_r[i] = ri;
            max_r[i] = ri;

            // ----------------------------------------------
            // INNER LOOP over j in the current r-window:
            //   rs_ptr[j] in [ri - WINDOW_SIZE, ri]
            // This is where the big speedup happens.
            // ----------------------------------------------
            for (std::size_t j = j_start; j < i; ++j) {
                if (used[j])             continue;
                if (score[j] == NEG_INF) continue;

                const uint32_t qj = qs_ptr[j];
                const uint32_t rj = rs_ptr[j];

                // Enforce monotone q and r
                if (!(qj < qi && rj < ri)) continue;

                // Pairwise reference distance filter
                if (ri - rj > WINDOW_SIZE) continue; // guaranteed false for j >= j_start, but keep as safety

                // New min/max r for this potential chain
                uint32_t new_min_r = std::min(min_r[j], ri);
                uint32_t new_max_r = std::max(max_r[j], ri);
                if (new_max_r - new_min_r > WINDOW_SIZE) continue;

                // Deviation from slope 1:1 between j and i
                uint32_t dq = qi - qj;
                uint32_t dr = ri - rj;
                if (dq == 0 || dr == 0) continue;

                int dev = std::abs(static_cast<int>(dq) - static_cast<int>(dr));
                if (static_cast<uint32_t>(dev) > MAX_DEV) continue;

                double candidate = score[j] + 1.0 - LAMBDA * static_cast<double>(dev);
                if (candidate > score[i]) {
                    score[i] = candidate;
                    prev[i]  = static_cast<int32_t>(j);
                    min_r[i] = new_min_r;
                    max_r[i] = new_max_r;
                }
            }
        }

        // Pick best endpoint among unused anchors.
        int best_idx = -1;
        double best_score = 0.0;
        for (std::size_t i = 0; i < n; ++i) {
            if (used[i]) continue;
            if (score[i] > best_score) {
                best_score = score[i];
                best_idx   = static_cast<int>(i);
            }
        }

        if (best_idx == -1 || best_score <= 0.0) {
            return -1; // no more good chains
        }
        return best_idx;
    };

    std::size_t chains_found = 0;
    while (chains_found < max_chains) {
        int best_idx = run_dp_iteration();
        if (best_idx == -1) break;

        // Backtrack best chain, marking anchors as used (peel-off).
        std::vector<std::pair<uint32_t, uint32_t>> chain;
        int cur = best_idx;
        while (cur != -1 && !used[cur]) {
            chain.emplace_back(qs_ptr[cur], rs_ptr[cur]);
            used[cur] = 1;
            cur = prev[cur];
        }
        std::reverse(chain.begin(), chain.end());

        if (!chain.empty()) {
            chains.push_back(std::move(chain));
            ++chains_found;
        } else {
            break; // safety
        }
    }
}

#endif // CHAINSEEDS_HPP
