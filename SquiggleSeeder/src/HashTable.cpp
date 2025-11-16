#include "params.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <cassert>

static uint8_t quantize_event(float x) {
    uint32_t bits;
    std::memcpy(&bits, &x, sizeof(float));

    // Take the top Q bits of the 32-bit float representation
    uint32_t topQ = bits >> (32 - Q);

    // Now apply RawHash-style pruning: keep bits [1,2] and [3+p .. Q]
    uint32_t top2 = topQ >> (Q - 2); // top 2 bits of the Q-bit window

    uint32_t low_mask = (1u << LOW_BITS) - 1u;
    uint32_t low = topQ & low_mask;

    uint8_t code = static_cast<uint8_t>((top2 << LOW_BITS) | low);
    assert(code >> (LOW_BITS + 2) == 0);
    return code;
}

// Simple usage helper
static void usage(const char* prog) {
    std::cerr << "Usage: " << prog
              << " <reference_fasta> <kmer_lookup_table> <output_hash_table>\n";
}

// Read the reference genome
static std::string read_reference(const std::string& ref_path) {
    std::ifstream in(ref_path);
    if (!in) {
        throw std::runtime_error("Failed to open reference file: " + ref_path);
    }

    std::string line;
    std::string seq;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line[0] == '>') continue; // header
        for (char c : line) {
            assert(c == 'A' || c == 'C' || c == 'G' || c == 'T');
            seq.push_back(c);
        }
    }
    return seq;
}

// Read kmer → current lookup table: "<KMER>\t<value>"
static std::unordered_map<std::string, double>
read_kmer_lookup(const std::string& table_path) {
    std::ifstream in(table_path);
    if (!in) {
        throw std::runtime_error("Failed to open k-mer lookup table: " + table_path);
    }

    std::unordered_map<std::string, double> table;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string kmer;
        double value;
        if (!(iss >> kmer >> value)) continue;
        table[kmer] = value;
    }
    return table;
}

void dump_hashtable(const std::string& out_path, const std::unordered_map<uint32_t, std::vector<uint32_t>>& hash_table, int start_idx, int end_idx){
    std::ofstream out(out_path);
    if (!out) {
        throw std::runtime_error("Failed to open output file: " + out_path);
    }
    out << "Hashtable for reference seeds[" << start_idx << ", " << end_idx << ")\n" << "<hash>,loc1,loc2,loc3...\n";
    for (const auto &[h, locs]: hash_table) {
        out << h;
        for (uint32_t loc : locs) {
            out << "," << loc;
        }
        out << "\n";
    }

    out.flush();
    if (!out) {
        throw std::runtime_error("Error while writing output file: " + out_path);
    }
}

int main(int argc, char** argv) {
    if (argc != 4) {
        usage(argv[0]);
        return 1;
    }

    const std::string ref_path   = argv[1];
    const std::string table_path = argv[2];
    const std::string out_path   = argv[3];

    try {
        // 1. Load reference and k-mer model
        std::string ref_seq = read_reference(ref_path);
        if (ref_seq.size() < static_cast<size_t>(KMER_LEN)) {
            throw std::runtime_error("Reference sequence is shorter than KMER_LEN.");
        }

        auto kmer_table = read_kmer_lookup(table_path);
        if (kmer_table.empty()) {
            throw std::runtime_error("K-mer lookup table is empty or invalid.");
        }

        // 2. Generate event values from reference using the k-mer lookup table
        std::vector<double> events;
        const uint32_t num_events = ref_seq.size() - KMER_LEN + 1;
        events.reserve(num_events);

        for (size_t i = 0; i < num_events; ++i) {
            std::string kmer = ref_seq.substr(i, KMER_LEN);
            auto it = kmer_table.find(kmer);
            events.push_back(it->second);
        }

        // 3. Global normalization (z-score) over reference events (RawHash-style)
        double sum = 0.0, sum_sq = 0.0;
        for (double v : events) {
            sum += v;
            sum_sq += (v*v);
        }
        double mean = sum / num_events;
        double var = sum_sq / num_events - mean * mean;
        double stddev = std::sqrt(var);

        std::vector<float> norm_events(num_events);
        for (size_t i = 0; i < num_events; ++i) {
            norm_events[i] = static_cast<float>((events[i] - mean) / stddev);
        }

        // 4. Quantize each event into (Q - p)-bit code
        std::vector<uint8_t> event_codes(num_events);
        for (size_t i = 0; i < num_events; ++i) {
            event_codes[i] = quantize_event(norm_events[i]);
        }

        // 5. Decide N: # of events grouped together into a seed
        const int N = (ref_seq.size() < VIRAL_BASE_THRESHOLD) ? 5 : (ref_seq.size() < SMALL_BASE_THRESHOLD) ? 6 : 7;
        std::cout << "This genome has " << ref_seq.size() << " bases => N = " << N << "\n";

        // 6. Build hash table: hash -> list of locations
        const uint32_t num_seeds = num_events - N + 1;
        uint32_t tile = 0;
        uint32_t start_idx = 0;
        uint32_t end_idx = num_seeds;
        if (IS_TILED) {
            end_idx = std::min(num_seeds, TILE_SIZE + TILE_OVERLAP);
        }
        while (true) {
            std::unordered_map<uint32_t, std::vector<uint32_t>> hash_table;
            for (uint32_t e = start_idx; e < end_idx; ++e) {
                // Build seed from N consecutive event codes
                uint64_t seed_code = 0;
                for (int j = 0; j < N; ++j) {
                    seed_code <<= BITS_PER_EVENT;
                    seed_code |= static_cast<uint64_t>(event_codes[e + j]);
                }
                uint32_t hash32_val = hash64to32(seed_code);
                uint16_t hash16_val = fold32to16(hash32_val);

                // Location: use event start index as reference position
                uint32_t loc = static_cast<uint32_t>(e);
                if (HASH_BITS == 32) {
                    hash_table[hash32_val].push_back(loc);
                } else if (HASH_BITS == 16) {
                    hash_table[hash16_val].push_back(loc);
                }
            }

            // 7. Write out hashtable
            std::string hashtable_path = out_path + std::to_string(tile) + ".txt";
            dump_hashtable(hashtable_path, hash_table, start_idx, end_idx);

            // Finished all hashtables
            if (end_idx == num_seeds) {
                break;
            }

            tile += 1;
            end_idx = std::min(num_seeds, end_idx + TILE_SIZE);
            start_idx += TILE_SIZE;
        }

        if (tile > 0) {
            std::cout << "Hash tables are tiled... look for several files\n";
        }

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
