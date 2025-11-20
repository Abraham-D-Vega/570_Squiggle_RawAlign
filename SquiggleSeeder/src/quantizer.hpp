// quantizer.hpp
// Event normalization, quantization, seed generation, and hash collision detection

#ifndef QUANTIZER_HPP
#define QUANTIZER_HPP

#include "utils.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <iomanip>

// Structure for anchors (seed hits)
struct Anchor {
    uint32_t target_pos;  // position in reference
    uint32_t query_pos;   // position in read (event index)
    bool operator<(const Anchor& other) const {
        return target_pos < other.target_pos;
    }
};

// Generate seeds from quantized events and find collisions in hash table
// Returns a vector of all anchors (seed matches)
inline std::vector<Anchor> generate_seeds_and_find_collisions(
    const std::vector<uint8_t> &codes,
    const std::unordered_map<uint32_t, std::vector<uint32_t>> &hash_table,
    int N,
    bool verbose = true)
{
    std::vector<Anchor> anchors;
    
    if (verbose) {
        std::cout << "\n=== Generated Seeds ===\n";
        std::cout << "Query_pos | Seed_codes | Hash | Matches\n";
        std::cout << "----------|------------|------|--------\n";
    }
    
    for (size_t i = 0; i + N <= codes.size(); ++i) {
        // Generate hash from N consecutive event codes using shared utility
        uint32_t hash_val = generate_seed_hash(codes, i, N);
        
        if (verbose) {
            // Print seed information
            std::cout << std::setw(9) << i << " | ";
            for (int j = 0; j < N; ++j) {
                std::cout << std::setw(2) << (int)codes[i + j];
                if (j < N - 1) std::cout << ",";
            }
            std::cout << " | " << std::setw(10) << hash_val << " | ";
        }
        
        auto it = hash_table.find(hash_val);
        if (it != hash_table.end()) {
            const auto &locs = it->second;
            if (verbose) {
                std::cout << locs.size() << " hits: ";
                for (size_t k = 0; k < std::min(locs.size(), (size_t)5); ++k) {
                    std::cout << locs[k];
                    if (k < std::min(locs.size(), (size_t)5) - 1) std::cout << ",";
                }
                if (locs.size() > 5) std::cout << "...";
                std::cout << "\n";
            }
            
            for (size_t k = 0; k < locs.size(); ++k) {
                anchors.push_back(Anchor{locs[k], static_cast<uint32_t>(i)});
            }
        } else {
            if (verbose) {
                std::cout << "no match\n";
            }
        }
    }
    
    if (verbose) {
        std::cout << "\n";
    }
    
    return anchors;
}

#endif // QUANTIZER_HPP
