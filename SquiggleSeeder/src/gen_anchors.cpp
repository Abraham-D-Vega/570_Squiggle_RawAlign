#include "oracle_segmenter.hpp"
#include "chain_seeds.hpp"
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

// Internal helper
template <typename RandomIt, typename Compare>
void parallel_sort_impl(RandomIt first, RandomIt last,
                        Compare comp,
                        std::size_t max_threads)
{
    auto len = last - first;
    const std::size_t SEQ_CUTOFF = 20000; // tune this threshold

    // Small range or no threads left: just do normal sort
    if (len <= 1 || max_threads <= 1 || len < SEQ_CUTOFF) {
        std::sort(first, last, comp);
        return;
    }

    RandomIt mid = first + len / 2;

    // Split threads roughly in half for each side
    std::size_t left_threads  = max_threads / 2;
    std::size_t right_threads = max_threads - left_threads;

    // Sort left half in a new thread
    std::thread left_thread([first, mid, comp, left_threads]() {
        parallel_sort_impl(first, mid, comp, left_threads);
    });

    // Sort right half in current thread
    parallel_sort_impl(mid, last, comp, right_threads);

    // Wait for left side
    left_thread.join();

    // Merge the two sorted halves
    std::inplace_merge(first, mid, last, comp);
}

template <typename RandomIt, typename Compare>
void parallel_sort(RandomIt first, RandomIt last,
                   Compare comp,
                   std::size_t max_threads = std::thread::hardware_concurrency())
{
    if (max_threads == 0) max_threads = 2; // fallback
    parallel_sort_impl(first, last, comp, max_threads);
}

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

void dump_chains(const std::vector<Anchor> &anchors, const std::string &output_file) {
    std::ofstream out(output_file);
    for (const auto &anchor : anchors) {
        out << anchor.q << "," << anchor.r << "\n";
    }
    out.close();
}

void process_read(int read_index,
                  const std::string &read_type,
                  const std::unordered_map<uint32_t, std::vector<uint32_t>> &hash_table,
                  int N,
                  const std::vector<uint8_t> &ref_signal,
                  std::vector<Anchor>& anchors)
{
    std::string base_path = ("datasets/" + read_type + "/");
    std::string id_prefix = read_type + "_raw";
    std::string read_id   = id_prefix + std::to_string(read_index);
    std::string query_path = base_path + read_id + ".txt";

    // Read raw signal
    std::vector<uint32_t> raw_signal;
    if (!read_raw_signal(query_path, raw_signal)) {
        std::cerr << "Warning: Could not read " << query_path << ", skipping\n";
        return;
    }

    // Detect events using oracle segmenter

    Options opt;
    std::vector<Event> events = detect_events_from_raw(raw_signal, opt);
    if (events.empty()) {
        std::cerr << "Warning: No events detected for " << read_id << ", skipping\n";
        return;
    }

    // Extract event averages
    std::vector<float> event_avgs;
    event_avgs.reserve(events.size());
    for (const auto &e : events) event_avgs.push_back(e.avg);

    // Normalize events
    std::vector<float> norm_events;
    normalize_events(event_avgs, norm_events);

    // Quantize events
    std::vector<uint8_t> codes;
    quantize_events(norm_events, codes);

    // Seeds
    int num_seeds = static_cast<int>(codes.size()) - N + 1;
    if (num_seeds < 0) num_seeds = 0;
    int num_hits = 0;

    std::vector<uint32_t> hashes;
    generate_seed_hashes(codes, N, hashes);

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

    parallel_sort(
        anchors.begin(), anchors.end(),
        [](const Anchor &a, const Anchor &b) {
            if (a.r != b.r) return a.r < b.r;
            return a.q < b.q;
        },
        10 // max threads you want to allow
    );
}

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <genome> <read_type> <output_file>\n";
        std::cerr << "  genome: genome name (e.g., lambda, ecoli, covid)\n";
        std::cerr << "  read_type: <genome> or human\n";
        std::cerr << "  num_reads: number of reads to process\n";
        return 1;
    }

    std::string genome      = argv[1];
    std::string read_type   = argv[2];
    int num_reads           = std::stoi(argv[3]);
    std::string output_file_prefix = "SquiggleSeeder/anchors_tmp/" + read_type;

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
    for(int i = 0; i < num_reads; i++) {
        std::vector<Anchor> anchors;
        process_read(i, read_type, hash_table, N, ref_signal, anchors);
        dump_chains(anchors, output_file_prefix + "_" + std::to_string(i) + ".txt");
    }
}
