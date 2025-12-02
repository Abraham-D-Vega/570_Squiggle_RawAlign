#include "aligner.hpp"
#include "utils.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <unordered_map>
#include <cstddef>

void read_raw_signal(int read_index,
                 const std::string &read_type,
                 std::vector<uint16_t> &raw_signal)
{
    std::string base_path = ("datasets/" + read_type + "/");
    std::string id_prefix = read_type + "_raw";
    std::string read_id   = id_prefix + std::to_string(read_index);
    std::string query_path = base_path + read_id + ".txt";

    raw_signal.clear();
    std::ifstream in(query_path);
    if (!in) {
        std::cerr << "Error: Could not open " << query_path << "\n";
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        uint16_t val;
        if (iss >> val) {
            raw_signal.push_back(val);
        }
    }
}

void read_chains(int read_index,
                 const std::string &read_type,
                 std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& chains)
{
    std::string base_path = "SquiggleSeeder/chains/" + read_type + "_";
    std::string chain_path = base_path + std::to_string(read_index) + ".txt";

    std::ifstream in(chain_path);
    if (!in) {
        std::cerr << "Failed to open chain file: " << chain_path << "\n";
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        std::string qs_s, rs_s;
        std::istringstream iss(line);
        if (std::getline(iss, qs_s, ',') &&
            std::getline(iss, rs_s)) {

            uint32_t qs = std::stoul(qs_s);
            uint32_t rs = std::stoul(rs_s);
            
            std::vector<std::pair<uint32_t, uint32_t>> tmp;
            tmp.emplace_back(std::make_pair(qs, rs));
            chains.emplace_back(tmp);
        }
    }
}

ChainSDTWResult align(std::vector<uint16_t>& raw_signal,
           const std::vector<uint8_t>& ref_signal,
           const std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& chains)
{
    return run_best_chain_sdtw(
        raw_signal,
        ref_signal,
        chains
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
        std::vector<std::vector<std::pair<uint32_t, uint32_t>>> chains;
        std::vector<uint16_t> raw_signal;
        read_raw_signal(i, read_type, raw_signal);
        read_chains(i, read_type, chains);
        ChainSDTWResult result = align(raw_signal, ref_signal, chains);
        std::cout << "Read " << i << ": prediction: " << 
            (result.sdtw_result.cost <= MAX_ALIGN_COST_FOR_POSITIVE ? genome : "human") 
            << ", cost: " << result.sdtw_result.cost << "\n";
    }
}