// simulate_seeder.cpp
// Simulates seeding pipeline: event detection -> normalization -> quantization -> seeding -> hash matching

#include "oracle_segmenter.hpp"
#include "chain_seeds.hpp"
#include "aligner.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <unordered_map>

// Read raw signal from file
bool read_raw_signal(const std::string &filepath, std::vector<uint32_t> &signal) {
    std::ifstream in(filepath);
    if (!in) {
        std::cerr << "Error: Could not open " << filepath << "\n";
        return false;
    }
    
    signal.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        uint32_t val;
        if (iss >> val) {
            signal.push_back(val);
        }
    }
    
    return !signal.empty();
}

// Load hash table from file
bool load_hash_table(const std::string &path, std::unordered_map<uint32_t, std::vector<uint32_t>> &hash_table, uint32_t &genome_size) {
    std::ifstream in(path);
    if (!in) return false;
    
    std::string header;
    if (!std::getline(in, header)) return false;
    
    // Parse header to get genome size
    size_t start_pos = header.find('[');
    size_t comma_pos = header.find(',', start_pos);
    size_t end_pos = header.find(')', comma_pos);
    if (start_pos != std::string::npos && comma_pos != std::string::npos && end_pos != std::string::npos) {
        std::string end_str = header.substr(comma_pos + 1, end_pos - comma_pos - 1);
        size_t first = end_str.find_first_not_of(" \t");
        if (first != std::string::npos) {
            end_str = end_str.substr(first);
            uint32_t num_seeds = std::stoul(end_str);
            genome_size = num_seeds;  // fwd+rev
        }
    }
    
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string token;
        if (!std::getline(iss, token, ',')) continue;
        
        auto lpos = token.find_first_not_of(" \t\r\n");
        auto rpos = token.find_last_not_of(" \t\r\n");
        if (lpos == std::string::npos) continue;
        token = token.substr(lpos, rpos - lpos + 1);
        
        if (token.empty() || token.find_first_not_of("0123456789") != std::string::npos) continue;
        
        uint32_t h = 0;
        try {
            h = static_cast<uint32_t>(std::stoul(token));
        } catch (...) {
            continue;
        }
        
        std::vector<uint32_t> locs;
        while (std::getline(iss, token, ',')) {
            auto l = token.find_first_not_of(" \t\r\n");
            if (l == std::string::npos) continue;
            auto r = token.find_last_not_of(" \t\r\n");
            std::string tok = token.substr(l, r - l + 1);
            if (tok.empty() || tok.find_first_not_of("0123456789") != std::string::npos) continue;
            try {
                locs.push_back(static_cast<uint32_t>(std::stoul(tok)));
            } catch (...) {
                continue;
            }
        }
        hash_table[h] = std::move(locs);
    }
    return true;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <genome> <output_file> <num_reads> [--align]\n";
        std::cerr << "  genome: genome name (e.g., lambda, ecoli, covid)\n";
        std::cerr << "  output_file: path to output results file\n";
        std::cerr << "  --align: (optional) output sDTW alignment results\n";
        return 1;
    }

    std::string genome = argv[1];
    std::string output_file = argv[2];
    int num_reads = std::stoi(argv[3]);
    output_file = output_file + ".txt";
    bool do_align = (argc > 4 && std::string(argv[4]) == "--align");
    std::string align_file = argv[2];
    align_file = align_file + "_align.txt";

    // Load hash table
    std::string hash_table_path = "datasets/" + genome + "/hash_table0.txt";
    std::unordered_map<uint32_t, std::vector<uint32_t>> hash_table;
    uint32_t genome_size = 0;
    
    if (!load_hash_table(hash_table_path, hash_table, genome_size)) {
        std::cerr << "Failed to load hash table from " << hash_table_path << "\n";
        return 1;
    }

    // Determine N based on genome size
    const int N = compute_N_from_genome_size(genome_size);

    std::cout << "Loaded hash table: " << hash_table.size() << " unique hashes\n";
    std::cout << "Genome size: ~" << genome_size << " bases\n";
    std::cout << "Using N = " << N << " events per seed\n";

    // Open output file for seeds/chains
    std::ofstream out(output_file);
    if (!out) {
        std::cerr << "Failed to open output file: " << output_file << "\n";
        return 1;
    }
    std::string ref_path_genome = "datasets/" + genome + "/ref.txt";

    // Open output file for alignments if requested
    std::ofstream out_align;
    if (do_align) {
        out_align.open(align_file);
        if (!out_align) {
            std::cerr << "Failed to open alignment file: " << align_file << "\n";
            return 1;
        }
        out_align << "# Seeding + Chaining based sDTW Alignment Results\n";
        out_align << "# Genome: " << genome << "\n";
        out_align << "# Reference: " << ref_path_genome << "\n";
        out_align << "#\n";
        out_align << "# Format: read_type read_id read_size cost ref_start ref_end alignment_length\n";
        out_align << "#\n";
    }

    // Process genome and human reads (<num_reads> each)
    for (int pass = 0; pass < 2; ++pass) {
        std::string read_type = (pass == 0) ? genome : "human";
        std::string base_path = (pass == 0) ? ("datasets/" + genome + "/") : "datasets/human/";
        std::string id_prefix = (pass == 0) ? (genome + "_raw") : "human_raw";
        int ref_size = (pass == 0) ? genome_size : genome_size; // Use genome_size for both for now
        // Load reference signal
        std::vector<uint8_t> ref_signal;
        std::string ref_path = (pass == 0) ? ref_path_genome : "data/covid/ref.txt";
        std::ifstream ref_in(ref_path);
        if (!ref_in) {
            std::cerr << "Failed to open reference file: " << ref_path << "\n";
            continue;
        }
        std::string line;
        while (std::getline(ref_in, line)) {
            if (line.empty() || line[0] == '#') continue;
            uint32_t val;
            if (std::istringstream(line) >> val) {
                ref_signal.push_back(static_cast<uint8_t>(val));
            }
        }
        for (int i = 0; i < num_reads; i++) {
            std::string read_id = id_prefix + std::to_string(i);
            std::string query_path = base_path + read_id + ".txt";

            // Read raw signal
            std::vector<uint32_t> raw_signal;
            if (!read_raw_signal(query_path, raw_signal)) {
                std::cerr << "Warning: Could not read " << query_path << ", skipping\n";
                continue;
            }

            // Detect events using oracle segmenter
            Options opt;
            std::vector<Event> events = detect_events_from_raw(raw_signal, opt);
            if (events.empty()) {
                std::cerr << "Warning: No events detected for " << read_id << ", skipping\n";
                continue;
            }

            // Extract event averages
            std::vector<float> event_avgs;
            for (const auto &e : events) event_avgs.push_back(e.avg);

            // Normalize events
            std::vector<float> norm_events;
            normalize_events(event_avgs, norm_events);

            // Quantize events
            std::vector<uint8_t> codes;
            quantize_events(norm_events, codes);

            // Output header
            out << "# " << read_type << " " << read_id << "\n";

            // Output seeds
            int num_seeds = codes.size() - N + 1;
            int num_hits = 0;

            std::vector<std::vector<uint32_t>> anchors(num_seeds);
            for (size_t j = 0; j < num_seeds; ++j) {
                uint32_t hash = generate_seed_hash(codes, j, N);
                auto it = hash_table.find(hash);
                if (it != hash_table.end()) {
                    num_hits++;
                    out << std::setw(10) << read_type << " "
                        << std::setw(15) << read_id << " "
                        << std::setw(10) << j << " ";
                    for (size_t k = 0; k < it->second.size(); ++k) {
                        if (k > 0) out << ",";
                        out << it->second[k];
                        // For chaining
                        anchors[j].push_back(it->second[k]);
                    }
                    out << "\n";
                }
            }
            out << "\n";

            // Output chains
            std::vector<std::vector<std::pair<uint32_t, uint32_t>>> chains;
            chain_seeds(anchors, chains);
            for (const auto& chain : chains) {
                out << "CHAIN " << read_type << " " << read_id << " ";
                for (const auto& anchor : chain) {
                    out << "  ANCHOR ref_pos=" << anchor.second << " query_pos=" << anchor.first << "\n";
                }
                out << "\n";
            }
            out << "\n";

            // Output best sDTW alignment only if requested
            if (do_align) {
                if (!chains.empty()) {
                    std::vector<uint16_t> raw_read_uint16(raw_signal.begin(), raw_signal.end());
                    ChainSDTWResult best_align = run_best_chain_sdtw(raw_read_uint16, ref_signal, chains, 5000);
                    uint32_t read_size = raw_read_uint16.size();
                    uint32_t start = best_align.ref_start + best_align.sdtw_result.ref_start_pos;
                    uint32_t end = best_align.ref_start + best_align.sdtw_result.ref_end_pos;
                    uint32_t alignment_length = best_align.sdtw_result.ref_end_pos - best_align.sdtw_result.ref_start_pos;
                    out_align << std::setw(10) << read_type << " "
                              << std::setw(15) << read_id << " "
                              << std::setw(10) << read_size << " "
                              << std::setw(10) << best_align.sdtw_result.cost << " "
                              << std::setw(10) << start << " "
                              << std::setw(10) << end << " "
                              << std::setw(10) << alignment_length << "\n";
                } else {
                    out_align << std::setw(10) << read_type << " "
                              << std::setw(15) << read_id << " "
                              << std::setw(10) << raw_signal.size() << " "
                              << std::setw(10) << -1 << " "
                              << std::setw(10) << -1 << " "
                              << std::setw(10) << -1 << " "
                              << std::setw(10) << -1 << "\n";
                }
            }
            std::cout << "  " << read_id << ": " << num_seeds << " seeds, " << num_hits << " hits, " << chains.size() << " chains\n";
        }
    }
    out.close();
    if (do_align) {
        out_align.close();
        std::cout << "\nResults written to: " << output_file << "\n";
        std::cout << "Alignment results written to: " << align_file << "\n";
    } else {
        std::cout << "\nResults written to: " << output_file << "\n";
    }
    return 0;
}
