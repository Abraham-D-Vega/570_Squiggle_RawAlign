#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iomanip>
using namespace std;

static void print_usage(const char *prog) {
    cerr << "Usage: " << prog << " -i input.txt -o output.txt [-n window_size] [-k threshold_factor] [--adaptive]\n";
    cerr << "  -i, --input      Input file (one integer per line)\n";
    cerr << "  -o, --output     Output events file\n";
    cerr << "  -n, --window     Window size (default 3)\n";
    cerr << "  -k, --factor     Threshold factor multiplier for adaptive mode (default 1.0)\n";
    cerr << "  --adaptive       Enable adaptive threshold based on signal variance (recommended)\n";
    cerr << "  -t, --threshold  Fixed absolute difference threshold (overrides adaptive if set)\n";
}

int main(int argc, char **argv) {
    string input_path;
    string output_path;
    int window = 3;
    double threshold = -1.0;  // -1 means use adaptive
    double threshold_factor = 1.0;  // multiplier for adaptive threshold
    bool use_adaptive = true;  // default to adaptive

    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "-i" || a == "--input") {
            if (i + 1 < argc) input_path = argv[++i];
        } else if (a == "-o" || a == "--output") {
            if (i + 1 < argc) output_path = argv[++i];
        } else if (a == "-n" || a == "--window") {
            if (i + 1 < argc) window = stoi(argv[++i]);
        } else if (a == "-k" || a == "--factor") {
            if (i + 1 < argc) threshold_factor = stod(argv[++i]);
        } else if (a == "--adaptive") {
            use_adaptive = true;
            threshold = -1.0;
        } else if (a == "-t" || a == "--threshold") {
            if (i + 1 < argc) {
                threshold = stod(argv[++i]);
                use_adaptive = false;
            }
        } else if (a == "-h" || a == "--help") {
            print_usage(argv[0]);
            return 0;
        } else {
            cerr << "Unknown arg: " << a << '\n';
            print_usage(argv[0]);
            return 1;
        }
    }

    if (input_path.empty() || output_path.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    if (window <= 0) {
        cerr << "window must be > 0\n";
        return 1;
    }

    ifstream in(input_path);
    if (!in) {
        cerr << "Failed to open input: " << input_path << '\n';
        return 1;
    }

    vector<long long> samples;
    string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        // allow comments after #
        auto pos = line.find('#');
        if (pos != string::npos) line = line.substr(0, pos);
        // trim
        auto start = line.find_first_not_of(" \t\r\n");
        if (start == string::npos) continue;
        auto end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);
        try {
            int v = stoi(line);
            samples.push_back(v);
        } catch (...) {
            // skip invalid lines
        }
    }
    in.close();

    size_t N = samples.size();
    if (N == 0) {
        cerr << "No samples read from input\n";
        return 1;
    }

    // prefix sums for O(1) window sums
    vector<long long> pref(N + 1, 0);
    for (size_t i = 0; i < N; ++i) pref[i+1] = pref[i] + samples[i];

    auto window_avg = [&](size_t start)->double {
        // start is zero-based, window assumed to fit
        long long s = pref[start + window] - pref[start];
        return double(s) / double(window);
    };

    // Compute adaptive threshold if needed
    if (use_adaptive && threshold < 0) {
        // Compute all window-to-window differences
        vector<double> diffs;
        if (N >= 2 * window) {
            for (size_t i = 0; i + 2 * window <= N; ++i) {
                double avg1 = window_avg(i);
                double avg2 = window_avg(i + 1);
                diffs.push_back(fabs(avg2 - avg1));
            }
        }
        
        if (!diffs.empty()) {
            // Compute standard deviation of differences
            double sum = 0.0;
            for (double d : diffs) sum += d;
            double mean = sum / diffs.size();
            
            double var_sum = 0.0;
            for (double d : diffs) {
                double delta = d - mean;
                var_sum += delta * delta;
            }
            double stddev = sqrt(var_sum / diffs.size());
            
            // Set threshold as factor * stddev
            // Higher factor = fewer, longer events
            // Lower factor = more, shorter events
            threshold = threshold_factor * stddev;
            
            // Ensure minimum threshold to avoid over-segmentation
            if (threshold < 0.5) threshold = 0.5;
            
            cerr << "[ADAPTIVE] mean_diff=" << mean << " stddev=" << stddev 
                 << " threshold=" << threshold << " (factor=" << threshold_factor << ")\n";
        } else {
            threshold = 1.0; // fallback
            cerr << "[ADAPTIVE] Insufficient data, using threshold=" << threshold << "\n";
        }
    }

    struct Event { size_t start; size_t end; size_t count; double avg; };
    vector<Event> events;

    size_t sidx = 0; // current run start, zero-based
    while (sidx + window <= N) {
        // if we don't have at least two windows ahead, produce final event
        size_t i = sidx + 1;
        double w_prev = window_avg(sidx);
        bool made_event = false;
        while (i + window <= N) {
            double w_curr = window_avg(i);
            if (fabs(w_curr - w_prev) <= threshold) {
                // extend run
                w_prev = w_curr;
                ++i;
                continue;
            } else {
                // divergence detected comparing window starting at i-1 and i
                size_t event_end = i + window - 2; // inclusive
                if (event_end >= N) event_end = N - 1;
                size_t count = event_end - sidx + 1;
                long long sum = pref[event_end + 1] - pref[sidx];
                double avg = double(sum) / double(count);
                events.push_back({sidx, event_end, count, avg});
                sidx = event_end + 1; // start after the event
                made_event = true;
                break; // restart outer while with new sidx
            }
        }
        if (!made_event) {
            // reached end without divergence: average remaining samples
            size_t event_end = N - 1;
            size_t count = event_end - sidx + 1;
            long long sum = pref[event_end + 1] - pref[sidx];
            double avg = double(sum) / double(count);
            events.push_back({sidx, event_end, count, avg});
            sidx = N; // done
            break;
        }
    }

    // if there remain samples fewer than window at the very start or between events
    if (sidx < N) {
        long long sum = pref[N] - pref[sidx];
        size_t count = N - sidx;
        double avg = double(sum) / double(count);
        events.push_back({sidx, N - 1, count, avg});
    }

    ofstream out(output_path);
    if (!out) {
        cerr << "Failed to open output: " << output_path << '\n';
        return 1;
    }
    out << "# start_index(1-based) end_index(1-based) count average(10-bit)\n";
    for (auto &e : events) {
        int avg_10bit = static_cast<int>(round(e.avg));
        if (avg_10bit < 0) avg_10bit = 0;
        if (avg_10bit > 1023) avg_10bit = 1023;
        out << (e.start + 1) << ' ' << (e.end + 1) << ' ' << e.count << ' ' << avg_10bit << '\n';
    }
    out.close();

    // Also print a short summary to stdout
    cout << "Wrote " << events.size() << " events to " << output_path << "\n";

    return 0;
}
