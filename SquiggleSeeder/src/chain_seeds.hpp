// // chain_seeds.hpp
// // This file takes seed hits (anchors) for a read and produces chains using the same chaining logic as rmap.cpp
// // To be used by simulate_seeder.cpp

#ifndef CHAINSEEDS_HPP
#define CHAINSEEDS_HPP

#include <vector>
#include <algorithm>
#include <cstdint>
#include <utility>
#include <iostream>

// void chain_seeds(
//     const std::vector<std::vector<uint32_t> >& anchors,
//     std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& chains,
//     uint32_t WINDOW_SIZE = 2000,
//     std::size_t max_chains = 5 // top-K chains to extract
// ) 
// {
//     chains.clear();
//     if (max_chains == 0) return;

//     // Flatten: (q_pos, ref_pos) for each candidate
//     struct Anchor {
//         uint32_t q; // query position
//         uint32_t r; // reference position
//     };

//     std::vector<Anchor> seeds;
//     seeds.reserve(
//         [&]() {
//             std::size_t total = 0;
//             for (const auto& v : anchors) total += v.size();
//             return total;
//         }()
//     );

//     for (uint32_t q = 0; q < anchors.size(); ++q) {
//         for (uint32_t r : anchors[q]) {
//             seeds.push_back(Anchor{q, r});
//         }
//     }

//     if (seeds.empty()) return;

//     // Sort by (q, r) so we can run LIS over r with increasing q
//     std::sort(seeds.begin(), seeds.end(),
//               [](const Anchor& a, const Anchor& b) {
//                   if (a.q != b.q) return a.q < b.q;
//                   return a.r < b.r;
//               });

//     const int n = static_cast<int>(seeds.size());

//     // Track which seeds are still available for chaining
//     std::vector<bool> active(n, true);

//     // Buffers reused across iterations
//     std::vector<int> dp(n);
//     std::vector<int> parent(n);
//     std::vector<int> start_idx(n);

//     for (std::size_t chain_idx = 0; chain_idx < max_chains; ++chain_idx) {
//         // Initialize DP for this round
//         int best_len = 0;
//         int best_i   = -1;

//         for (int i = 0; i < n; ++i) {
//             if (!active[i]) {
//                 dp[i]       = 0;
//                 parent[i]   = -1;
//                 start_idx[i] = i;
//                 continue;
//             }

//             dp[i]       = 1;
//             parent[i]   = -1;
//             start_idx[i] = i;

//             // Standard O(n^2) DP with window constraint, only over active seeds
//             for (int j = 0; j < i; ++j) {
//                 if (!active[j] || dp[j] == 0) continue;

//                 // Enforce strictly increasing in both query and ref
//                 if (seeds[j].q < seeds[i].q && seeds[j].r < seeds[i].r) {
//                     uint32_t r_start = seeds[start_idx[j]].r;
//                     // r is increasing along the chain, so this enforces the window
//                     if (seeds[i].r - r_start <= WINDOW_SIZE) {
//                         if (dp[j] + 1 > dp[i]) {
//                             dp[i]        = dp[j] + 1;
//                             parent[i]    = j;
//                             start_idx[i] = start_idx[j];
//                         }
//                     }
//                 }
//             }

//             if (dp[i] > best_len) {
//                 best_len = dp[i];
//                 best_i   = i;
//             }
//         }

//         // No more chains can be formed
//         if (best_len <= 0 || best_i == -1) break;

//         // Optional: skip trivial chains of length 1 if you don't care about them.
//         // if (best_len <= 1) break;

//         // Reconstruct best chain for this round
//         std::vector<std::pair<uint32_t, uint32_t>> cur_chain;
//         std::vector<int> used_indices; // to mark these anchors as inactive later

//         for (int cur = best_i; cur != -1; cur = parent[cur]) {
//             cur_chain.emplace_back(seeds[cur].q, seeds[cur].r);
//             used_indices.push_back(cur);
//         }
//         std::reverse(cur_chain.begin(), cur_chain.end());
//         std::reverse(used_indices.begin(), used_indices.end());

//         // Store this chain
//         chains.push_back(std::move(cur_chain));

//         // Mark used anchors as inactive so they won't appear in later chains
//         for (int idx : used_indices) {
//             active[idx] = false;
//         }
//     }
// }


void chain_seeds(
    const std::vector<std::vector<uint32_t> >& anchors,
    std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& chains,
    uint32_t DEBUG_START_REF_POS = 0,
    uint32_t DEBUG_END_REF_POS = 0xffffffff,
    uint32_t WINDOW_SIZE = 2000,
    double LAMBDA = 0.01,           // penalty per unit deviation
    std::size_t max_chains = 5 // top-K chains to extract
)
{
    chains.clear();

    // Flatten anchors into (q, r) pairs
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
            if (r < DEBUG_START_REF_POS || r > DEBUG_END_REF_POS) continue;
            seeds.push_back({q, r});
        }
    }
    // std::cout << "Finding chains from " << seeds.size() << " anchors\n";
    if (DEBUG_START_REF_POS != 0) {
        for (const auto &s : seeds) {
            std::cout << "  Seed: q=" << s.q << " r=" << s.r << "\n";
        }
    }

    if (seeds.empty()) return;

    // Sort by q, then by r to enforce increasing q in chains
    std::sort(seeds.begin(), seeds.end(),
              [](const Anchor &a, const Anchor &b) {
                  if (a.q != b.q) return a.q < b.q;
                  return a.r < b.r;
              });

    const std::size_t n = seeds.size();

    // Track which anchors have been "peeled off" into chains already
    std::vector<bool> used(n, false);

    // Parameters for slope penalty
    const uint32_t MAX_DEV = WINDOW_SIZE; // hard cutoff on deviation

    // Temporary DP buffers reused across peel-off iterations
    std::vector<double> score(n);
    std::vector<int> prev(n);
    std::vector<uint32_t> min_r(n);
    std::vector<uint32_t> max_r(n);

    std::size_t chains_found = 0;

    while (chains_found < max_chains) {
        // std::cout << "On iteration " << (chains_found + 1) << "\n";
        // Recompute DP from scratch each iteration, skipping used anchors
        const double NEG_INF = -std::numeric_limits<double>::infinity();

        for (std::size_t i = 0; i < n; ++i) {
            if (used[i]) {
                score[i] = NEG_INF;
                prev[i]  = -1;
                min_r[i] = max_r[i] = seeds[i].r;
                continue;
            }

            // Start a new chain at i
            score[i] = 1.0;          // base score for a single anchor
            prev[i]  = -1;
            min_r[i] = seeds[i].r;
            max_r[i] = seeds[i].r;

            const auto &ai = seeds[i];

            // Try to extend chains ending at j < i
            for (std::size_t j = 0; j < i; ++j) {
                if (used[j]) continue;
                if (score[j] == NEG_INF) continue;

                const auto &aj = seeds[j];

                // Enforce monotone increasing q and r
                if (!(aj.q < ai.q && aj.r < ai.r)) continue;

                // Compute new min/max r if we extend chain j with i
                uint32_t new_min_r = std::min(min_r[j], ai.r);
                uint32_t new_max_r = std::max(max_r[j], ai.r);

                // Check window constraint on reference positions
                if (new_max_r - new_min_r > WINDOW_SIZE) continue;

                // Deviation from slope 1:1
                uint32_t dq = ai.q - aj.q;
                uint32_t dr = ai.r - aj.r;

                if (dq == 0 || dr == 0) {
                    // Shouldn't happen with aj.q < ai.q && aj.r < ai.r, but be safe
                    continue;
                }

                int dev = std::abs(static_cast<int>(dq) - static_cast<int>(dr));
                if (static_cast<uint32_t>(dev) > MAX_DEV) {
                    // Too far from slope 1:1
                    continue;
                }

                double candidate = score[j] + 1.0 - LAMBDA * static_cast<double>(dev);
                if (candidate > score[i]) {
                    score[i] = candidate;
                    prev[i]  = static_cast<int>(j);
                    min_r[i] = new_min_r;
                    max_r[i] = new_max_r;
                }
            }
        }

        // Find best-scoring chain endpoint among unused anchors
        int best_idx = -1;
        double best_score = 0.0; // require positive score to keep a chain

        for (std::size_t i = 0; i < n; ++i) {
            if (used[i]) continue;
            if (score[i] > best_score) {
                best_score = score[i];
                best_idx = static_cast<int>(i);
            }
        }

        if (best_idx == -1 || best_score <= 0.0) {
            // No more meaningful chains
            break;
        }

        // Backtrack to recover this chain
        std::vector<std::pair<uint32_t, uint32_t>> chain;
        int cur = best_idx;
        while (cur != -1 && !used[cur]) {
            chain.emplace_back(seeds[cur].q, seeds[cur].r);
            used[cur] = true; // peel off this anchor
            cur = prev[cur];
        }
        std::reverse(chain.begin(), chain.end());

        if (!chain.empty()) {
            chains.push_back(std::move(chain));
            ++chains_found;
        } else {
            // Shouldn't really happen, but guard against infinite loops
            break;
        }
    }
}

#endif // CHAINSEEDS_HPP
