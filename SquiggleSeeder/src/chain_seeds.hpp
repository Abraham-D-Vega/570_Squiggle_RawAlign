// // chain_seeds.hpp
// // This file takes seed hits (anchors) for a read and produces chains using the same chaining logic as rmap.cpp
// // To be used by simulate_seeder.cpp

// #ifndef CHAINSEEDS_HPP
// #define CHAINSEEDS_HPP

// #include <vector>
// #include <algorithm>
// #include <cstdio>
// #include <cstring>

// void chain_seeds(
//     const std::vector<std::vector<uint32_t> >& anchors,
//     uint32_t WINDOW_SIZE,
//     uint32_t k,
//     std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& chains) 
// {
//     chains.clear();

//     // Flatten: (q_pos, ref_pos) for each candidate
//     struct Anchor {
//         uint32_t q; // query position
//         uint32_t r; // reference position
//     };

//     std::vector<Anchor> seeds;
//     size_t total = 0;
//     for (const auto& v : anchors) total += v.size();
//     seeds.reserve(total);

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

//     // dp[i]      = best chain length ending at i
//     // parent[i]  = previous index in chain
//     // start_idx[i] = index of the first anchor in the chain ending at i
//     std::vector<int> dp(n, 1);
//     std::vector<int> parent(n, -1);
//     std::vector<int> start_idx(n);

//     for (int i = 0; i < n; ++i) {
//         start_idx[i] = i;
//     }

//     int best_len = 1;
//     int best_i   = 0;

//     // O(n^2) DP with window constraint on reference positions
//     for (int i = 0; i < n; ++i) {
//         for (int j = 0; j < i; ++j) {
//             // Enforce strictly increasing in both query and ref
//             if (seeds[j].q < seeds[i].q && seeds[j].r < seeds[i].r) {
//                 uint32_t r_start = seeds[start_idx[j]].r;
//                 // Because r is increasing along the chain, checking
//                 // (r_i - r_start <= WINDOW_SIZE) enforces that all
//                 // refs in the chain lie within a WINDOW_SIZE span.
//                 if (seeds[i].r - r_start <= WINDOW_SIZE) {
//                     if (dp[j] + 1 > dp[i]) {
//                         dp[i]       = dp[j] + 1;
//                         parent[i]   = j;
//                         start_idx[i] = start_idx[j];
//                     }
//                 }
//             }
//         }
//         if (dp[i] > best_len) {
//             best_len = dp[i];
//             best_i   = i;
//         }
//     }

//     // Reconstruct best chain
//     std::vector<std::pair<uint32_t, uint32_t> > tmp;
//     for (int cur = best_i; cur != -1; cur = parent[cur]) {
//         tmp.emplace_back(seeds[cur].q, seeds[cur].r);
//     }
//     std::reverse(tmp.begin(), tmp.end());

//     chains.push_back(std::move(tmp));
// }

// #endif // CHAINSEEDS_HPP

#ifndef CHAINSEEDS_HPP
#define CHAINSEEDS_HPP

#include <vector>
#include <algorithm>
#include <cstdint>
#include <utility>

void chain_seeds(
    const std::vector<std::vector<uint32_t> >& anchors,
    std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& chains,
    uint32_t WINDOW_SIZE = 2000,
    std::size_t max_chains = 10 // top-K chains to extract
) 
{
    chains.clear();
    if (max_chains == 0) return;

    // Flatten: (q_pos, ref_pos) for each candidate
    struct Anchor {
        uint32_t q; // query position
        uint32_t r; // reference position
    };

    std::vector<Anchor> seeds;
    seeds.reserve(
        [&]() {
            std::size_t total = 0;
            for (const auto& v : anchors) total += v.size();
            return total;
        }()
    );

    for (uint32_t q = 0; q < anchors.size(); ++q) {
        for (uint32_t r : anchors[q]) {
            seeds.push_back(Anchor{q, r});
        }
    }

    if (seeds.empty()) return;

    // Sort by (q, r) so we can run LIS over r with increasing q
    std::sort(seeds.begin(), seeds.end(),
              [](const Anchor& a, const Anchor& b) {
                  if (a.q != b.q) return a.q < b.q;
                  return a.r < b.r;
              });

    const int n = static_cast<int>(seeds.size());

    // Track which seeds are still available for chaining
    std::vector<bool> active(n, true);

    // Buffers reused across iterations
    std::vector<int> dp(n);
    std::vector<int> parent(n);
    std::vector<int> start_idx(n);

    for (std::size_t chain_idx = 0; chain_idx < max_chains; ++chain_idx) {
        // Initialize DP for this round
        int best_len = 0;
        int best_i   = -1;

        for (int i = 0; i < n; ++i) {
            if (!active[i]) {
                dp[i]       = 0;
                parent[i]   = -1;
                start_idx[i] = i;
                continue;
            }

            dp[i]       = 1;
            parent[i]   = -1;
            start_idx[i] = i;

            // Standard O(n^2) DP with window constraint, only over active seeds
            for (int j = 0; j < i; ++j) {
                if (!active[j] || dp[j] == 0) continue;

                // Enforce strictly increasing in both query and ref
                if (seeds[j].q < seeds[i].q && seeds[j].r < seeds[i].r) {
                    uint32_t r_start = seeds[start_idx[j]].r;
                    // r is increasing along the chain, so this enforces the window
                    if (seeds[i].r - r_start <= WINDOW_SIZE) {
                        if (dp[j] + 1 > dp[i]) {
                            dp[i]        = dp[j] + 1;
                            parent[i]    = j;
                            start_idx[i] = start_idx[j];
                        }
                    }
                }
            }

            if (dp[i] > best_len) {
                best_len = dp[i];
                best_i   = i;
            }
        }

        // No more chains can be formed
        if (best_len <= 0 || best_i == -1) break;

        // Optional: skip trivial chains of length 1 if you don't care about them.
        // if (best_len <= 1) break;

        // Reconstruct best chain for this round
        std::vector<std::pair<uint32_t, uint32_t>> cur_chain;
        std::vector<int> used_indices; // to mark these anchors as inactive later

        for (int cur = best_i; cur != -1; cur = parent[cur]) {
            cur_chain.emplace_back(seeds[cur].q, seeds[cur].r);
            used_indices.push_back(cur);
        }
        std::reverse(cur_chain.begin(), cur_chain.end());
        std::reverse(used_indices.begin(), used_indices.end());

        // Store this chain
        chains.push_back(std::move(cur_chain));

        // Mark used anchors as inactive so they won't appear in later chains
        for (int idx : used_indices) {
            active[idx] = false;
        }
    }
}

#endif // CHAINSEEDS_HPP
