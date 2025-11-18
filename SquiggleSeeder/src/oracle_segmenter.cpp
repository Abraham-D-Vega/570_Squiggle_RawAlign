#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <algorithm>

// Standalone C++ re-implementation of src/revent.c that
// - accepts a vector of 10-bit integers (0..1023)
// - performs the same t-stat, dual-detector peak picking, and event segmentation
// - outputs per-segment means (floating-point) to a file you specify
// - builds without any project headers or kalloc; all memory is std::vector
// - can run as a CLI tool reading integers (one per line) from a file path or stdin
//
// Usage examples:
//   # from file (positional path or --in file)
//   ./revent_standalone signal.txt out.txt --win1 8 --win2 32 --th1 5.0 --th2 3.0 --peak 1.0
//   ./revent_standalone --in signal.txt --out out.txt --win1 8 --win2 32 --th1 5.0 --th2 3.0 --peak 1.0
//   # from stdin
//   cat signal.txt | ./revent_standalone --win1 8 --win2 32 --th1 5.0 --th2 3.0 --peak 1.0
//
// Output: writes one event value (double) per line to the output file (or stdout if omitted).

struct Options {
    uint32_t window_length1 = 8;   // short window
    uint32_t window_length2 = 32;  // long window
    double   threshold1     = 5.0; // t-stat threshold (short)
    double   threshold2     = 3.0; // t-stat threshold (long)
    double   peak_height    = 1.0; // hysteresis (drop from peak)
};

struct Detector {
    int      DEF_PEAK_POS   = -1;
    double   DEF_PEAK_VAL   = std::numeric_limits<double>::infinity();
    const std::vector<double>* sig = nullptr; // points to tstat array
    uint32_t s_len           = 0;
    double   threshold       = 0.0;
    uint32_t window_length   = 0;
    uint32_t masked_to       = 0;
    int      peak_pos        = -1;
    double   peak_value      = std::numeric_limits<double>::infinity();
    int      valid_peak      = 0;
};

static inline void comp_prefix_prefixsq(const std::vector<double>& sig,
                                        std::vector<double>& prefix_sum,
                                        std::vector<double>& prefix_sum_sq) {
    const uint32_t s_len = static_cast<uint32_t>(sig.size());
    assert(s_len > 0);
    prefix_sum.assign(s_len + 1, 0.0);
    prefix_sum_sq.assign(s_len + 1, 0.0);
    for (uint32_t i = 0; i < s_len; ++i) {
        prefix_sum[i + 1]   = prefix_sum[i]   + sig[i];
        prefix_sum_sq[i + 1]= prefix_sum_sq[i]+ sig[i] * sig[i];
    }
}

static inline std::vector<double> comp_tstat(const std::vector<double>& prefix_sum,
                                             const std::vector<double>& prefix_sum_sq,
                                             uint32_t s_len, uint32_t w_len) {
    const double eta = std::numeric_limits<float>::min();
    std::vector<double> tstat(s_len + 1, 0.0);
    if (s_len < 2 * w_len || w_len < 2) return tstat;

    // boundaries zeroed
    for (uint32_t i = w_len; i <= s_len - w_len; ++i) {
        double sum1   = prefix_sum[i] - prefix_sum[i - w_len];
        double sumsq1 = prefix_sum_sq[i] - prefix_sum_sq[i - w_len];
        double sum2   = prefix_sum[i + w_len] - prefix_sum[i];
        double sumsq2 = prefix_sum_sq[i + w_len] - prefix_sum_sq[i];
        double mean1  = sum1 / w_len;
        double mean2  = sum2 / w_len;
        double combined_var = sumsq1 / w_len - mean1 * mean1
                             + sumsq2 / w_len - mean2 * mean2;
        combined_var = std::max(combined_var, static_cast<double>(eta));
        double delta_mean = mean2 - mean1;
        tstat[i] = std::fabs(delta_mean) / std::sqrt(combined_var / w_len);
    }
    return tstat;
}

static inline uint32_t gen_peaks(Detector& short_det, Detector& long_det,
                                 double peak_height, std::vector<uint32_t>& peaks) {
    assert(short_det.s_len == long_det.s_len);
    const uint32_t s_len = short_det.s_len;
    peaks.clear();
    peaks.reserve(s_len / 8 + 2);

    Detector* dets[2] = { &short_det, &long_det };

    for (uint32_t i = 0; i < s_len; ++i) {
        for (int k = 0; k < 2; ++k) {
            Detector* d = dets[k];
            if (d->masked_to >= i) continue;
            double current_value = (*(d->sig))[i];
            if (d->peak_pos == d->DEF_PEAK_POS) {
                // not yet recorded a maximum
                if (current_value < d->peak_value) {
                    d->peak_value = current_value; // track deeper minimum
                } else if (current_value - d->peak_value > peak_height) {
                    d->peak_value = current_value;
                    d->peak_pos   = static_cast<int>(i);
                }
            } else {
                // in a peak, see if better
                if (current_value > d->peak_value) {
                    d->peak_value = current_value;
                    d->peak_pos   = static_cast<int>(i);
                }
                // short dominates long if going to fire
                if (d == &short_det) {
                    if (d->peak_value > d->threshold) {
                        long_det.masked_to   = d->peak_pos + d->window_length;
                        long_det.peak_pos    = long_det.DEF_PEAK_POS;
                        long_det.peak_value  = long_det.DEF_PEAK_VAL;
                        long_det.valid_peak  = 0;
                    }
                }
                // validate
                if (d->peak_value - current_value > peak_height && d->peak_value > d->threshold) {
                    d->valid_peak = 1;
                }
                // emit when far enough from the peak position
                if (d->valid_peak && (static_cast<int>(i) - d->peak_pos) > static_cast<int>(d->window_length / 2)) {
                    peaks.push_back(static_cast<uint32_t>(d->peak_pos));
                    d->peak_pos    = d->DEF_PEAK_POS;
                    d->peak_value  = current_value;
                    d->valid_peak  = 0;
                }
            }
        }
    }
    return static_cast<uint32_t>(peaks.size());
}

struct Event {
    uint32_t start; // 0-based
    uint32_t end;   // 0-based inclusive
    uint32_t count;
    double   avg;
};

static inline std::vector<Event> gen_events(const std::vector<uint32_t>& peaks,
                                            const std::vector<double>& prefix_sum,
                                            const std::vector<double>& prefix_sum_sq,
                                            uint32_t s_len) {
    (void)prefix_sum_sq; // kept for parity with original signature (unused here)

    // Count events = number of valid peaks + 1
    uint32_t n_ev = 1;
    for (size_t i = 1; i < peaks.size(); ++i) {
        if (peaks[i] > 0 && peaks[i] < s_len) n_ev++;
    }
    if (n_ev == 0) return {};

    std::vector<Event> events(n_ev);

    uint32_t l_idx = 0; // start of current segment
    double l_prefixsum = 0.0;

    // events between peaks (per-segment mean in original signal domain)
    for (uint32_t pi = 0; pi < n_ev - 1; ++pi) {
        uint32_t peak_idx = peaks[pi];
        uint32_t seg_start = l_idx;
        uint32_t seg_end = peak_idx > 0 ? peak_idx - 1 : 0;
        uint32_t seg_count = peak_idx - l_idx;
        double seg_sum = prefix_sum[peak_idx] - l_prefixsum;
        double seg_avg = seg_sum / std::max(1.0, static_cast<double>(seg_count));
        
        events[pi] = {seg_start, seg_end, seg_count, seg_avg};
        
        l_idx = peak_idx;
        l_prefixsum = prefix_sum[peak_idx];
    }
    // last event [last_peak, s_len)
    uint32_t seg_start_last = l_idx;
    uint32_t seg_end_last = s_len > 0 ? s_len - 1 : 0;
    uint32_t seg_count_last = s_len - l_idx;
    double seg_sum_last = prefix_sum[s_len] - l_prefixsum;
    double seg_avg_last = seg_sum_last / std::max(1.0, static_cast<double>(seg_count_last));
    
    events[n_ev - 1] = {seg_start_last, seg_end_last, seg_count_last, seg_avg_last};

    return events;
}

// High-level API: input is 10-bit integers, output is per-segment events with start/end/count/avg
static inline std::vector<Event> detect_events_from_u10(const std::vector<uint16_t>& sig_u10,
                                                        const Options& opt) {
    // clamp and convert to double
    const uint32_t s_len = static_cast<uint32_t>(sig_u10.size());
    if (s_len == 0) return {};

    std::vector<double> sig(s_len);
    for (uint32_t i = 0; i < s_len; ++i) sig[i] = static_cast<double>(std::min<uint16_t>(sig_u10[i], 1023));

    std::vector<double> prefix_sum, prefix_sum_sq;
    comp_prefix_prefixsq(sig, prefix_sum, prefix_sum_sq);
    std::vector<double> t1 = comp_tstat(prefix_sum, prefix_sum_sq, s_len, opt.window_length1);
    std::vector<double> t2 = comp_tstat(prefix_sum, prefix_sum_sq, s_len, opt.window_length2);

    Detector short_det;
    short_det.sig           = &t1;
    short_det.s_len         = s_len;
    short_det.threshold     = opt.threshold1;
    short_det.window_length = opt.window_length1;

    Detector long_det;
    long_det.sig            = &t2;
    long_det.s_len          = s_len;
    long_det.threshold      = opt.threshold2;
    long_det.window_length  = opt.window_length2;

    std::vector<uint32_t> peaks;
    gen_peaks(short_det, long_det, opt.peak_height, peaks);

    return gen_events(peaks, prefix_sum, prefix_sum_sq, s_len);
}

// Simple CLI helpers
static void usage(const char* prog) {
    std::cerr << "Usage: " << prog << " <in_path|--in file|-> [out_path|--out file] [--win1 N] [--win2 N] [--th1 T] [--th2 T] [--peak H]\n"
              << "       If in_path/--in is '-' or omitted, read integers (0..1023) from stdin.\n"
              << "       If out_path/--out is '-' or omitted, write events to stdout.\n";
}

static bool parse_args(int argc, char** argv, std::string& in_path, std::string& out_path, Options& opt) {
    in_path = "-";  // default stdin
    out_path = "-"; // default stdout
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto need = [&](int more){ if (i + more >= argc) { usage(argv[0]); exit(1);} };
        if (a == "--in") { need(1); in_path = argv[++i]; }
        else if (a == "--out") { need(1); out_path = argv[++i]; }
        else if (a == "--win1") { need(1); opt.window_length1 = static_cast<uint32_t>(std::stoul(argv[++i])); }
        else if (a == "--win2") { need(1); opt.window_length2 = static_cast<uint32_t>(std::stoul(argv[++i])); }
        else if (a == "--th1") { need(1); opt.threshold1 = std::stod(argv[++i]); }
        else if (a == "--th2") { need(1); opt.threshold2 = std::stod(argv[++i]); }
        else if (a == "--peak") { need(1); opt.peak_height = std::stod(argv[++i]); }
        else if (a == "-h" || a == "--help") { usage(argv[0]); exit(0);} 
        else if (!a.empty() && a[0] != '-') {
            // positional paths: first is input, second is output
            if (in_path == "-") in_path = a;
            else if (out_path == "-") out_path = a;
            else { std::cerr << "Unexpected extra positional arg: " << a << "\n"; usage(argv[0]); return false; }
        }
        else { std::cerr << "Unknown arg: " << a << "\n"; usage(argv[0]); return false; }
    }
    return true;
}

static bool read_u10_stream(std::istream& is, std::vector<uint16_t>& out) {
    out.clear();
    uint32_t v;
    while (is >> v) {
        if (v > 1023) v = 1023; // clamp
        out.push_back(static_cast<uint16_t>(v));
    }
    return !out.empty();
}

// Convert floating events (0..1023) to 10-bit integers with rounding and clamping
static std::vector<uint16_t> quantize_events_u10(const std::vector<double>& ev) {
    std::vector<uint16_t> out;
    out.reserve(ev.size());
    for (double v : ev) {
        long r = std::lround(v);
        if (r < 0) r = 0;
        if (r > 1023) r = 1023;
        out.push_back(static_cast<uint16_t>(r));
    }
    return out;
}

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string in_path;
    std::string out_path;
    Options opt;
    if (!parse_args(argc, argv, in_path, out_path, opt)) return 1;

    std::vector<uint16_t> sig_u10;
    if (in_path == "-" || in_path.empty()) {
        if (!read_u10_stream(std::cin, sig_u10)) {
            std::cerr << "No input read from stdin. Provide 10-bit ints separated by whitespace.\n";
            return 1;
        }
    } else {
        std::ifstream ifs(in_path);
        if (!ifs) { std::cerr << "Failed to open: " << in_path << "\n"; return 1; }
        if (!read_u10_stream(ifs, sig_u10)) {
            std::cerr << "No integers found in: " << in_path << "\n"; return 1; }
    }

    auto events = detect_events_from_u10(sig_u10, opt);

    // Open output (file or stdout) and write in format: start(1-based) end(1-based) count average(10-bit)
    if (out_path == "-" || out_path.empty()) {
        std::cout << "# start_index(1-based) end_index(1-based) count average(10-bit)\n";
        for (const auto& e : events) {
            int avg_10bit = static_cast<int>(std::round(e.avg));
            if (avg_10bit < 0) avg_10bit = 0;
            if (avg_10bit > 1023) avg_10bit = 1023;
            std::cout << (e.start + 1) << ' ' << (e.end + 1) << ' ' << e.count << ' ' << avg_10bit << "\n";
        }
    } else {
        std::ofstream ofs(out_path);
        if (!ofs) { std::cerr << "Failed to open for write: " << out_path << "\n"; return 1; }
        ofs << "# start_index(1-based) end_index(1-based) count average(10-bit)\n";
        for (const auto& e : events) {
            int avg_10bit = static_cast<int>(std::round(e.avg));
            if (avg_10bit < 0) avg_10bit = 0;
            if (avg_10bit > 1023) avg_10bit = 1023;
            ofs << (e.start + 1) << ' ' << (e.end + 1) << ' ' << e.count << ' ' << avg_10bit << "\n";
        }
        ofs.close();
        std::cout << "Wrote " << events.size() << " events to " << out_path << "\n";
    }
    return 0;
}
