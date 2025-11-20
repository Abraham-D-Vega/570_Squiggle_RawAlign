#include "utils.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

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

// Generate reverse complement of a DNA sequence
std::string rev_comp(const std::string& seq) {
    std::string rev_seq;
    rev_seq.reserve(seq.size());
    for (auto it = seq.rbegin(); it != seq.rend(); ++it) {
        char rc;
        switch (*it) {
            case 'A': rc = 'T'; break;
            case 'C': rc = 'G'; break;
            case 'G': rc = 'C'; break;
            case 'T': rc = 'A'; break;
            default:
                throw std::runtime_error("Invalid base in sequence for reverse complement.");
        }
        rev_seq.push_back(rc);
    }
    return rev_seq;
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

void dump_hash_table(const std::string& out_path, const std::unordered_map<uint32_t, std::vector<uint32_t>>& hash_table, int start_idx, int end_idx){
    std::ofstream out(out_path);
    if (!out) {
        throw std::runtime_error("Failed to open output file: " + out_path);
    }
    out << "Hash Table for reference seeds[" << start_idx << ", " << end_idx << ")\n" << "<hash>,loc1,loc2,loc3...\n";
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
        std::string fwd_ref = read_reference(ref_path);
        std::string rev_ref = rev_comp(fwd_ref);
        std::string ref_seq = fwd_ref + rev_ref;
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
        std::vector<float> norm_events;
        normalize_events(events, norm_events);

        // 4. Quantize each event into (Q - p)-bit code
        std::vector<uint8_t> event_codes;
        quantize_events(norm_events, event_codes);

        // 5. Decide N: # of events grouped together into a seed
        const int N = compute_N_from_genome_size(ref_seq.size());
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
                // Generate hash from N consecutive event codes
                uint32_t hash_val = generate_seed_hash(event_codes, e, N);
                
                // Location: use event start index as reference position
                hash_table[hash_val].push_back(e);
            }

            // 7. Write out hash table
            std::string hash_table_path = out_path + std::to_string(tile) + ".txt";
            dump_hash_table(hash_table_path, hash_table, start_idx, end_idx);

            // Finished all hash tables
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
