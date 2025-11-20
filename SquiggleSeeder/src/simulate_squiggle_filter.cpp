// simulate_squiggle_filter.cpp
// Simulates SquiggleFilter alignment: reads preprocessed 8-bit signals and runs sDTW

#include "aligner.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>

// Read 8-bit normalized signal from preprocessed file
bool read_8bit_signal(const std::string &filepath, std::vector<uint8_t> &signal) {
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
        int val;
        if (iss >> val) {
            signal.push_back(static_cast<uint8_t>(val));
        }
    }
    
    return !signal.empty();
}

// Read 16-bit raw signal for normalization
bool read_16bit_signal(const std::string &filepath, std::vector<uint16_t> &signal) {
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
        int val;
        if (iss >> val) {
            signal.push_back(static_cast<uint16_t>(val));
        }
    }
    
    return !signal.empty();
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <genome> <output_file>\n";
        std::cerr << "  genome: genome name (e.g., lambda, ecoli, covid)\n";
        std::cerr << "  output_file: path to output results file\n";
        return 1;
    }

    std::string genome = argv[1];
    std::string output_file = argv[2];

    // Read reference 8-bit signal from datasets/<genome>/ref.txt
    std::string ref_path = "datasets/" + genome + "/ref.txt";
    std::vector<uint8_t> ref_signal;
    if (!read_8bit_signal(ref_path, ref_signal)) {
        std::cerr << "Failed to read reference signal from " << ref_path << "\n";
        return 1;
    }
    
    std::cout << "Loaded reference signal: " << ref_signal.size() << " samples\n";

    // Open output file
    std::ofstream out(output_file);
    if (!out) {
        std::cerr << "Failed to open output file: " << output_file << "\n";
        return 1;
    }

    // Write header
    out << "# sDTW Alignment Results\n";
    out << "# Genome: " << genome << "\n";
    out << "# Reference: " << ref_path << "\n";
    out << "#\n";
    out << "# Format: read_type read_id cost ref_start ref_end alignment_length\n";
    out << "#\n";

    // Process genome reads (20 reads from datasets/<genome>/)
    std::cout << "\nProcessing " << genome << " reads...\n";
    for (int i = 0; i < 20; i++) {
        std::string read_type = genome;
        std::string read_id = genome + "_raw" + std::to_string(i);
        std::string query_path = "datasets/" + genome + "/" + read_id + ".txt";

        // Read 16-bit query signal and normalize internally
        std::vector<uint16_t> query_signal;
        if (!read_16bit_signal(query_path, query_signal)) {
            std::cerr << "Warning: Could not read " << query_path << ", skipping\n";
            continue;
        }
        
        SDTWResult result = single_sdtw(query_signal, ref_signal);

        uint32_t alignment_length = result.ref_end_pos - result.ref_start_pos;
        
        out << std::setw(10) << read_type << " "
            << std::setw(15) << read_id << " "
            << std::setw(10) << result.cost << " "
            << std::setw(10) << result.ref_start_pos << " "
            << std::setw(10) << result.ref_end_pos << " "
            << std::setw(10) << alignment_length << "\n";
        
        std::cout << "  " << read_id << ": cost=" << result.cost 
                  << ", ref[" << result.ref_start_pos << ":" << result.ref_end_pos << "]"
                  << " (len=" << alignment_length << ")\n";
    }

    // Process human reads (20 reads from datasets/human/)
    std::cout << "\nProcessing human reads...\n";
    for (int i = 0; i < 20; i++) {
        std::string read_type = "human";
        std::string read_id = "human_raw" + std::to_string(i);
        std::string query_path = "datasets/human/" + read_id + ".txt";

        // Read 16-bit query signal and normalize internally
        std::vector<uint16_t> query_signal;
        if (!read_16bit_signal(query_path, query_signal)) {
            std::cerr << "Warning: Could not read " << query_path << ", skipping\n";
            continue;
        }
        
        SDTWResult result = single_sdtw(query_signal, ref_signal);

        uint32_t alignment_length = result.ref_end_pos - result.ref_start_pos;
        
        out << std::setw(10) << read_type << " "
            << std::setw(15) << read_id << " "
            << std::setw(10) << result.cost << " "
            << std::setw(10) << result.ref_start_pos << " "
            << std::setw(10) << result.ref_end_pos << " "
            << std::setw(10) << alignment_length << "\n";
        
        std::cout << "  " << read_id << ": cost=" << result.cost 
                  << ", ref[" << result.ref_start_pos << ":" << result.ref_end_pos << "]"
                  << " (len=" << alignment_length << ")\n";
    }

    out.close();
    std::cout << "\nResults written to: " << output_file << "\n";
    
    return 0;
}
