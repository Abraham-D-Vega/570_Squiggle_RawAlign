#include "oracle_segmenter.hpp"
#include "chain_seeds.hpp"
#include "aligner.hpp"
#include "utils.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cstddef>

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
bool load_hash_table(const std::string &path,
                     std::unordered_map<uint32_t, std::vector<uint32_t>> &hash_table,
                     uint32_t &genome_size) {
    std::ifstream in(path);
    if (!in) return false;

    std::string header;
    if (!std::getline(in, header)) return false;

    // Parse header to get genome size
    size_t start_pos = header.find('[');
    size_t comma_pos = header.find(',', start_pos);
    size_t end_pos   = header.find(')', comma_pos);
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

// Per-read result, filled by worker threads
struct ReadResult {
    bool processed = false;  // false if file missing / no events, etc.
    std::string read_type;
    std::string read_id;
    int num_seeds = 0;
    int num_hits  = 0;

    std::vector<std::vector<std::pair<uint32_t, uint32_t>>> chains;

    // Alignment info
    bool   align_valid       = false;  // true if sDTW run and cost != -1
    int    cost              = -1;
    uint32_t read_size       = 0;
    uint32_t align_start     = 0;
    uint32_t align_end       = 0;
    uint32_t alignment_length= 0;
#ifdef PROFILE
    double time_event_detect = 0.0;
    double time_normalize    = 0.0;
    double time_quantize     = 0.0;
    double time_hash         = 0.0;
    double time_anchors      = 0.0;
    double time_sort         = 0.0;
    double time_chain        = 0.0;
    double time_align        = 0.0;
#endif
};

// Fast radix sort specialized for Anchor (sort by r, then q).
// Assumes: r fits in 24 bits, q fits in 11 bits.
static void radix_sort_anchors(std::vector<Anchor> &anchors) {
    const std::size_t n = anchors.size();
    if (n <= 1) return;

    std::vector<uint64_t> keys(n);
    constexpr uint64_t Q_MASK = (1u << 11) - 1u;

    for (std::size_t i = 0; i < n; ++i) {
        uint64_t r = anchors[i].r;
        uint64_t q = anchors[i].q & Q_MASK;
        keys[i] = (r << 11) | q;
    }

    std::vector<uint64_t> tmp(n);

    auto pass = [&](unsigned shift, unsigned bits) {
        const std::size_t RADIX = 1u << bits;
        static thread_local std::vector<std::size_t> count;
        count.assign(RADIX, 0);

        // count
        for (std::size_t i = 0; i < n; ++i) {
            std::size_t bucket = (keys[i] >> shift) & (RADIX - 1u);
            ++count[bucket];
        }
        // prefix sums
        std::size_t sum = 0;
        for (std::size_t i = 0; i < RADIX; ++i) {
            auto c = count[i];
            count[i] = sum;
            sum += c;
        }
        // scatter
        for (std::size_t i = 0; i < n; ++i) {
            std::size_t bucket = (keys[i] >> shift) & (RADIX - 1u);
            tmp[count[bucket]++] = keys[i];
        }
        keys.swap(tmp);
    };

    // LSD: q (11 bits), then r low 12 bits, then r high 12 bits
    pass(0, 11);    // bits 0-10
    pass(11, 12);   // bits 11-22
    pass(23, 12);   // bits 23-34

    for (std::size_t i = 0; i < n; ++i) {
        uint64_t k = keys[i];
        uint32_t q = static_cast<uint32_t>(k & Q_MASK);
        uint32_t r = static_cast<uint32_t>(k >> 11);
        anchors[i].q = q;
        anchors[i].r = r;
    }
}

void process_read(int pass,
                  int read_index,
                  const std::string &genome,
                  const std::unordered_map<uint32_t, std::vector<uint32_t>> &hash_table,
                  int N,
                  const std::vector<uint8_t> &ref_signal,
                  bool do_align,
                  ReadResult &result)
{
    std::string read_type = (pass == 0) ? genome : "human";
    std::string base_path = (pass == 0) ? ("datasets/" + genome + "/") : "datasets/human/";
    std::string id_prefix = (pass == 0) ? (genome + "_raw") : "human_raw";
    std::string read_id   = id_prefix + std::to_string(read_index);
    std::string query_path = base_path + read_id + ".txt";

    result.read_type = read_type;
    result.read_id   = read_id;

    // Read raw signal
    std::vector<uint32_t> raw_signal;
    if (!read_raw_signal(query_path, raw_signal)) {
        std::cerr << "Warning: Could not read " << query_path << ", skipping\n";
        result.processed = false;
        return;
    }

    // Detect events using oracle segmenter
    #ifdef PROFILE
    auto t_start = std::chrono::high_resolution_clock::now();
    #endif

    Options opt;
    std::vector<Event> events = detect_events_from_raw(raw_signal, opt);
    if (events.empty()) {
        std::cerr << "Warning: No events detected for " << read_id << ", skipping\n";
        result.processed = false;
        return;
    }

    // Extract event averages
    std::vector<float> event_avgs;
    event_avgs.reserve(events.size());
    for (const auto &e : events) event_avgs.push_back(e.avg);
    #ifdef PROFILE
    double t_event_detect = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - t_start).count();
    result.time_event_detect = t_event_detect;
    #endif

    // Normalize events
    std::vector<float> norm_events;
    normalize_events(event_avgs, norm_events);
    #ifdef PROFILE
    double t_normalize = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - t_start).count();
    result.time_normalize = t_normalize - t_event_detect;
    #endif

    // Quantize events
    std::vector<uint8_t> codes;
    quantize_events(norm_events, codes);
    #ifdef PROFILE
    double t_quantize = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - t_start).count();
    result.time_quantize = t_quantize - t_normalize;
    #endif

    // Seeds
    int num_seeds = static_cast<int>(codes.size()) - N + 1;
    if (num_seeds < 0) num_seeds = 0;
    int num_hits = 0;

    std::vector<uint32_t> hashes;
    generate_seed_hashes(codes, N, hashes);
    #ifdef PROFILE
    double t_hash = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - t_start).count();
    result.time_hash = t_hash - t_quantize;
    #endif

    std::vector<Anchor> anchors;
    anchors.reserve(num_seeds);
    for (int j = 0; j < num_seeds; ++j) {
        auto it = hash_table.find(hashes[j]);
        if (it != hash_table.end()) {
            num_hits++;
            const auto &locs = it->second;
            for (auto r_pos : locs) {
                anchors.push_back(Anchor{static_cast<uint32_t>(j), r_pos});
            }
        }
    }
    #ifdef PROFILE
    double t_anchors = std::chrono::duration<double, std::milli>(           
        std::chrono::high_resolution_clock::now() - t_start).count();   
    result.time_anchors = t_anchors - t_hash;
    #endif

    radix_sort_anchors(anchors);
    
    #ifdef PROFILE
    double t_sort = std::chrono::duration<double, std::milli>(           
        std::chrono::high_resolution_clock::now() - t_start).count();   
    result.time_sort = t_sort - t_anchors;
    #endif

    // Chaining
    std::vector<std::vector<std::pair<uint32_t, uint32_t>>> chains;
    chain_seeds(anchors, chains);
    // Fill result
    result.processed = true;
    result.num_seeds = num_seeds;
    result.num_hits  = num_hits;
    result.chains    = std::move(chains);

    #ifdef PROFILE
    double t_chain = std::chrono::duration<double, std::milli>(           
        std::chrono::high_resolution_clock::now() - t_start).count();   
    result.time_chain = t_chain - t_sort;
    #endif

    // Alignment (if requested)
    if (do_align) {
        if (!result.chains.empty()) {
            std::vector<uint16_t> raw_read_uint16(raw_signal.begin(), raw_signal.end());
            ChainSDTWResult best_align = run_best_chain_sdtw(raw_read_uint16, ref_signal, result.chains);

            result.cost        = best_align.sdtw_result.cost;
            result.read_size   = static_cast<uint32_t>(raw_read_uint16.size());
            result.align_start = best_align.ref_start + best_align.sdtw_result.ref_start_pos;
            result.align_end   = best_align.ref_start + best_align.sdtw_result.ref_end_pos;
            result.alignment_length =
                best_align.sdtw_result.ref_end_pos - best_align.sdtw_result.ref_start_pos;
            result.align_valid = (result.cost != -1);
        } else {
            // No chains → cost stays -1, align_valid = false
            result.cost        = -1;
            result.read_size   = static_cast<uint32_t>(raw_signal.size());
            result.align_valid = false;
        }
    }  
    #ifdef PROFILE
    double t_align = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - t_start).count();
    result.time_align = t_align - t_chain;
    #endif
}

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <genome> <output_file> <num_reads> [--align]\n";
        std::cerr << "  genome: genome name (e.g., lambda, ecoli, covid)\n";
        std::cerr << "  output_file: path to output results file\n";
        std::cerr << "  --align: (optional) output sDTW alignment results\n";
        return 1;
    }

    std::string genome      = argv[1];
    std::string output_file = argv[2];
    int num_reads           = std::stoi(argv[3]);
    output_file = output_file + ".txt";
    bool do_align = (argc > 4 && std::string(argv[4]) == "--align");
    std::string align_file = std::string(argv[2]) + "_align.txt";

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

    // Load reference signal once (used for both passes)
    std::string ref_path_genome = "datasets/" + genome + "/ref.txt";
    std::vector<uint8_t> ref_signal;
    {
        std::ifstream ref_in(ref_path_genome);
        if (!ref_in) {
            std::cerr << "Failed to open reference file: " << ref_path_genome << "\n";
            return 1;
        }
        std::string line;
        while (std::getline(ref_in, line)) {
            if (line.empty() || line[0] == '#') continue;
            uint32_t val;
            if (std::istringstream(line) >> val) {
                ref_signal.push_back(static_cast<uint8_t>(val));
            }
        }
    }

    // Open output file for seeds/chains
    std::ofstream out(output_file);
    if (!out) {
        std::cerr << "Failed to open output file: " << output_file << "\n";
        return 1;
    }

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

    int genome_total   = 0, genome_correct   = 0;
    int human_total    = 0, human_correct    = 0;
    int overall_total  = 0, overall_correct  = 0;

    // Process genome and human reads (<num_reads> each)
    for (int pass = 0; pass < 2; ++pass) {
        std::vector<ReadResult> results(num_reads);
        std::vector<std::thread> threads;
        threads.reserve(num_reads);

        // Launch one thread per read for this pass
        for (int i = 0; i < num_reads; ++i) {
            threads.emplace_back(process_read,
                                 pass,
                                 i,
                                 std::cref(genome),
                                 std::cref(hash_table),
                                 N,
                                 std::cref(ref_signal),
                                 do_align,
                                 std::ref(results[i]));
            #ifdef PROFILE
                threads[i].join();
            #endif
            #ifndef PROFILE
                if (threads.size() >= 20) {
                    for (auto &t : threads) {
                        t.join();
                    }
                    threads.clear();
                }
            #endif
        }

        std::cout << "\nCompleted processing "
                  << ((pass == 0) ? genome : "human")
                  << " reads.\n";

        // Now write outputs in the original deterministic order
        for (int i = 0; i < num_reads; ++i) {
            auto &res = results[i];
            if (!res.processed) continue; // exactly like the sequential "continue" cases

            // Chains file: identical format/order as sequential version
            for (const auto &chain : res.chains) {
                out << "CHAIN " << res.read_type << " " << res.read_id << "\n";
                for (const auto &anchor : chain) {
                    out << "  ANCHOR ref_pos=" << anchor.second
                        << " query_pos=" << anchor.first << "\n";
                }
                out << "\n";
            }
            out << "\n";  // per-read blank line, same as original

            // Alignment file + classification stats (if enabled)
            if (do_align) {
                if (!res.chains.empty()) {
                    if (res.align_valid) {
                        out_align << std::setw(10) << res.read_type << " "
                                  << std::setw(15) << res.read_id << " "
                                  << std::setw(10) << res.read_size << " "
                                  << std::setw(10) << res.cost << " "
                                  << std::setw(10) << res.align_start << " "
                                  << std::setw(10) << res.align_end << " "
                                  << std::setw(10) << res.alignment_length << "\n";
                    } else {
                        // sDTW not run or failed; mimic original -1 line
                        out_align << std::setw(10) << res.read_type << " "
                                  << std::setw(15) << res.read_id << " "
                                  << std::setw(10) << res.read_size << " "
                                  << std::setw(10) << -1 << " "
                                  << std::setw(10) << -1 << " "
                                  << std::setw(10) << -1 << " "
                                  << std::setw(10) << -1 << "\n";
                    }
                } else {
                    // No chains: original writes -1 line with raw_signal.size()
                    out_align << std::setw(10) << res.read_type << " "
                              << std::setw(15) << res.read_id << " "
                              << std::setw(10) << res.read_size << " "
                              << std::setw(10) << -1 << " "
                              << std::setw(10) << -1 << " "
                              << std::setw(10) << -1 << " "
                              << std::setw(10) << -1 << "\n";
                }

                // Classification counting (same logic as original)
                if (res.align_valid && res.cost != -1) {
                    if (pass == 0) {
                        genome_total++;
                        if (res.cost <= MAX_ALIGN_COST_FOR_POSITIVE) genome_correct++;
                    } else {
                        human_total++;
                        if (res.cost > MAX_ALIGN_COST_FOR_POSITIVE) human_correct++;
                    }
                    overall_total++;
                    if ((pass == 0 && res.cost <= MAX_ALIGN_COST_FOR_POSITIVE) ||
                        (pass == 1 && res.cost > MAX_ALIGN_COST_FOR_POSITIVE)) {
                        overall_correct++;
                    }
                }
            }
        }
        #ifdef PROFILE
        std::cout << "\nProfiling results for "
                  << ((pass == 0) ? genome : "human") << " reads\n";
        ReadResult avg_res;
        int profiled_reads = 0;
        for (const auto &res : results) {
            if (!res.processed) continue;
            profiled_reads++;
            avg_res.time_event_detect += res.time_event_detect;
            avg_res.time_normalize    += res.time_normalize;
            avg_res.time_quantize     += res.time_quantize;
            avg_res.time_hash         += res.time_hash;
            avg_res.time_anchors      += res.time_anchors;
            avg_res.time_sort         += res.time_sort;
            avg_res.time_chain        += res.time_chain;
            avg_res.time_align        += res.time_align;
        }
        avg_res.time_event_detect /= profiled_reads;
        avg_res.time_normalize    /= profiled_reads;        
        avg_res.time_quantize     /= profiled_reads;
        avg_res.time_hash         /= profiled_reads;
        avg_res.time_anchors      /= profiled_reads;
        avg_res.time_sort         /= profiled_reads;
        avg_res.time_chain        /= profiled_reads;
        avg_res.time_align        /= profiled_reads;

        std::cout << "  Average event detection time: " << avg_res.time_event_detect << " ms\n";
        std::cout << "  Average normalization time:   " << avg_res.time_normalize    << " ms\n";
        std::cout << "  Average quantization time:    " << avg_res.time_quantize     << " ms\n";
        std::cout << "  Average hashing time:          " << avg_res.time_hash         << " ms\n";
        std::cout << "  Average anchor finding time:   " << avg_res.time_anchors      << " ms\n";
        std::cout << "  Average sorting time:          " << avg_res.time_sort         << " ms\n";
        std::cout << "  Average chaining time:         " << avg_res.time_chain        << " ms\n";
        std::cout << "  Average TOTAL seeding time:    "
                  << (avg_res.time_event_detect + avg_res.time_normalize +
                      avg_res.time_quantize + avg_res.time_hash +
                      avg_res.time_anchors + avg_res.time_sort +
                      avg_res.time_chain)
                  << " ms\n\n";
        if (do_align) {
            std::cout << "  Average alignment time:       " << avg_res.time_align        << " ms\n";
        }
        std::cout << "\n";
        #endif
    }

    out.close();
    if (do_align) {
        out_align.close();
        std::cout << "\nResults written to: " << output_file << "\n";
        std::cout << "Alignment results written to: " << align_file << "\n";
    } else {
        std::cout << "\nResults written to: " << output_file << "\n";
    }

    // Print accuracy summary
    if (do_align) {
        std::cout << "\nClassification accuracy summary (threshold: " << MAX_ALIGN_COST_FOR_POSITIVE << "):\n";
        std::cout << "  Genome reads: " << genome_correct << "/" << genome_total
                  << " = " << (genome_total ? (100.0 * genome_correct / genome_total) : 0) << "%\n";
        std::cout << "  Human reads:  " << human_correct << "/" << human_total
                  << " = " << (human_total ? (100.0 * human_correct / human_total) : 0) << "%\n";
        std::cout << "  Overall:      " << overall_correct << "/" << overall_total
                  << " = " << (overall_total ? (100.0 * overall_correct / overall_total) : 0) << "%\n";
    }

    return 0;
}
