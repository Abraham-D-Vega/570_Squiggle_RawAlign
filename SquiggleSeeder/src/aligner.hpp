// aligner.hpp
// sDTW (subsequence Dynamic Time Warping) alignment for raw signals

#ifndef ALIGNER_HPP
#define ALIGNER_HPP

#include "chainer.hpp"
#include <vector>
#include <limits>
#include <cmath>
#include <algorithm>

// Discrete normalization as in SquiggleFilter
// Converts signal to 8-bit integers with mean=0, scale=mean_avg_dev, clipped to [-4,4]
inline void discrete_normalize(const std::vector<uint16_t> &seq, std::vector<uint8_t> &out, 
                                int bits=8, int minval=-4, int maxval=4) {
    out.clear();
    out.resize(seq.size());
    
    // Compute mean
    double mean = 0.0f;
    for (uint16_t val : seq) mean += val;
    mean /= seq.size();

    uint16_t mean_uint16 = static_cast<uint16_t>(mean);
    
    // Compute mean absolute deviation
    double mad = 0.0f;
    for (uint16_t val : seq) { 
        mad += std::abs(val - mean_uint16);
    }

    mad /= seq.size();

    uint16_t mad_uint16 = static_cast<uint16_t>(mad);
    
    // Normalize, clip, and discretize
    uint16_t scale = (1 << bits) / (maxval - minval);
    for (size_t i = 0; i < seq.size(); ++i) {
        double norm_val = static_cast<double>(seq[i] - mean_uint16) / mad;
        if (norm_val < minval) norm_val = minval;
        if (norm_val > maxval) norm_val = maxval;
        out[i] = static_cast<uint8_t>((norm_val - minval) * scale);
    }
}

// Result structure for sDTW alignment
struct SDTWResult {
    uint32_t cost;       // Alignment cost
    uint32_t ref_start_pos;   // Reference position where query alignment starts
    uint32_t ref_end_pos;     // Reference position where query alignment ends
};

// Subsequence DTW (sDTW) as implemented in SquiggleFilter
// Returns alignment cost and reference positions where query aligned best
// Operates on discrete normalized signals (8-bit integers)
inline SDTWResult sDTW(const std::vector<uint8_t> &query, const std::vector<uint8_t> &ref) {
    int N = query.size();
    int M = ref.size();
    std::vector<uint32_t> prev_consec(N, 0);
    std::vector<uint32_t> curr_consec(N, 0);
    std::vector<uint32_t> prev_cost(N, 0);
    std::vector<uint32_t> curr_cost(N, 0);
    std::vector<uint32_t> min_cost(N, 0);
    
    // Track start position for each query position i at each reference position j
    // We only need to track for the final query position (N-1)
    std::vector<uint32_t> prev_start(N, 0);
    std::vector<uint32_t> curr_start(N, 0);
    
    // Track where the minimum occurs
    uint32_t best_ref_start = 0;
    uint32_t best_ref_end = 0;

    prev_cost[0] = std::abs(query[0] - ref[0]);
    min_cost[0] = std::abs(query[0] - ref[0]);
    prev_start[0] = 0;

    for (size_t i = 1; i < N; i++) {
        prev_cost[i] = prev_cost[i-1] + std::abs(query[i] - ref[0]);
        min_cost[i] = min_cost[i-1] + std::abs(query[i] - ref[0]);
        prev_start[i] = 0;  // All paths starting from j=0
    }

    for (size_t j = 1; j < M; j++) {
        uint32_t bonus = 10;
        // Initialize first position of current column (i=0 is implicit, stays 0)
        curr_start[0] = j;  // Starting at this reference position
        
        for (size_t i = 1; i < N; i++) {
            bool move = (prev_cost[i-1] - prev_consec[i-1] * bonus) < curr_cost[i-1];
            if (move) {
                // Diagonal move from (i-1, j-1)
                curr_consec[i] = 0;
                curr_cost[i] = prev_cost[i-1] - prev_consec[i-1] * bonus + std::abs(query[i] - ref[j]);
                curr_start[i] = prev_start[i-1];  // Inherit start from diagonal
            } else {
                // Vertical move from (i-1, j)
                curr_consec[i] = std::min(10u, prev_consec[i] + 1);
                curr_cost[i] = curr_cost[i-1] + std::abs(query[i] - ref[j]);
                curr_start[i] = curr_start[i-1];  // Inherit start from above
            }
            
            // Update minimum and track positions
            if (curr_cost[i] < min_cost[i]) {
                min_cost[i] = curr_cost[i];
                if (i == N-1) {
                    best_ref_start = curr_start[i];
                    best_ref_end = j;
                }
            }
        }
        for (size_t i = 0; i < N; i++) {
            prev_consec[i] = curr_consec[i];
            curr_consec[i] = 0;
            prev_cost[i] = curr_cost[i];
            curr_cost[i] = 0;
            prev_start[i] = curr_start[i];
            curr_start[i] = 0;
        }
    }
    return {min_cost[N-1], best_ref_start, best_ref_end};
}

inline SDTWResult single_sdtw(const std::vector<uint16_t> &query, const std::vector<uint8_t> &ref) {
    std::vector<uint8_t> query_norm;
    discrete_normalize(query, query_norm);
    return sDTW(query_norm, ref);
}

#endif // ALIGNER_HPP
