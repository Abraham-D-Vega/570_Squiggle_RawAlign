// oracle_segmenter.hpp
// API for oracle event detection (extracted from oracle_segmenter.cpp)

#ifndef ORACLE_SEGMENTER_HPP
#define ORACLE_SEGMENTER_HPP

#include <vector>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <cmath>
#include <cassert>

struct Options {
    uint32_t window_length1 = 3;
    uint32_t window_length2 = 6;
    float    threshold1     = 4.30265f;
    float    threshold2     = 2.57058f;
    float    peak_height    = 1.0f;
};

struct Event {
    uint32_t start;
    uint32_t end;
    uint32_t count;
    float    avg;
};

struct Detector {
    int      DEF_PEAK_POS   = -1;
    float    DEF_PEAK_VAL   = std::numeric_limits<float>::infinity();
    const std::vector<float>* sig = nullptr;
    uint32_t s_len           = 0;
    float    threshold       = 0.0f;
    uint32_t window_length   = 0;
    uint32_t masked_to       = 0;
    int      peak_pos        = -1;
    float    peak_value      = std::numeric_limits<float>::infinity();
    int      valid_peak      = 0;
};

static inline void comp_prefix_prefixsq(const std::vector<float>& sig,
                                        std::vector<float>& prefix_sum,
                                        std::vector<float>& prefix_sum_sq) {
    const uint32_t s_len = static_cast<uint32_t>(sig.size());
    assert(s_len > 0);
    prefix_sum.assign(s_len + 1, 0.0f);
    prefix_sum_sq.assign(s_len + 1, 0.0f);
    for (uint32_t i = 0; i < s_len; ++i) {
        prefix_sum[i + 1]   = prefix_sum[i]   + sig[i];
        prefix_sum_sq[i + 1]= prefix_sum_sq[i]+ sig[i] * sig[i];
    }
}

static inline std::vector<float> comp_tstat(const std::vector<float>& sig,
                                             const std::vector<float>& prefix_sum,
                                             const std::vector<float>& prefix_sum_sq,
                                             uint32_t s_len, uint32_t w_len) {
    const float eta = std::numeric_limits<float>::min();
    std::vector<float> tstat(s_len + 1, 0.0f);
    if (s_len < 2 * w_len || w_len < 2) return tstat;

    for (uint32_t i = w_len; i <= s_len - w_len; ++i) {
        float sum1 = prefix_sum[i];
        if (i > w_len) {
            sum1 -= prefix_sum[i - w_len];
        }
        float sum2   = prefix_sum[i + w_len] - prefix_sum[i];
        float mean1  = sum1 / w_len;
        float mean2  = sum2 / w_len;
        
        float var1 = 0.0f;
        uint32_t start1 = (i > w_len) ? (i - w_len) : 0;
        for (uint32_t j = start1; j < i; ++j) {
            float deviation = sig[j] - mean1;
            var1 += deviation * deviation;
        }
        var1 /= w_len;
        
        float var2 = 0.0f;
        for (uint32_t j = i; j < i + w_len; ++j) {
            float deviation = sig[j] - mean2;
            var2 += deviation * deviation;
        }
        var2 /= w_len;
        
        float combined_var = var1 + var2;
        combined_var = std::max(combined_var, eta);
        float delta_mean = mean2 - mean1;
        tstat[i] = std::fabs(delta_mean) / std::sqrt(combined_var / w_len);
    }
    return tstat;
}

static inline uint32_t gen_peaks(Detector& short_det, Detector& long_det,
                                 float peak_height, std::vector<uint32_t>& peaks) {
    assert(short_det.s_len == long_det.s_len);
    const uint32_t s_len = short_det.s_len;
    peaks.clear();
    peaks.reserve(s_len / 8 + 2);

    Detector* dets[2] = { &short_det, &long_det };

    for (uint32_t i = 0; i < s_len; ++i) {
        for (int k = 0; k < 2; ++k) {
            Detector* d = dets[k];
            if (d->masked_to >= i) continue;
            float current_value = (*(d->sig))[i];
            if (d->peak_pos == d->DEF_PEAK_POS) {
                if (current_value < d->peak_value) {
                    d->peak_value = current_value;
                } else if (current_value - d->peak_value > peak_height) {
                    d->peak_value = current_value;
                    d->peak_pos   = static_cast<int>(i);
                }
            } else {
                if (current_value > d->peak_value) {
                    d->peak_value = current_value;
                    d->peak_pos   = static_cast<int>(i);
                }
                if (d == &short_det) {
                    if (d->peak_value > d->threshold) {
                        long_det.masked_to   = d->peak_pos + d->window_length;
                        long_det.peak_pos    = long_det.DEF_PEAK_POS;
                        long_det.peak_value  = long_det.DEF_PEAK_VAL;
                        long_det.valid_peak  = 0;
                    }
                }
                if (d->peak_value - current_value > peak_height && d->peak_value > d->threshold) {
                    d->valid_peak = 1;
                }
                if (d->valid_peak && (i - d->peak_pos) > d->window_length / 2) {
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

static inline std::vector<Event> gen_events(const std::vector<uint32_t>& peaks,
                                            const std::vector<float>& prefix_sum,
                                            const std::vector<float>& prefix_sum_sq,
                                            uint32_t s_len) {
    (void)prefix_sum_sq;

    uint32_t n_ev = 1;
    for (size_t i = 1; i < peaks.size(); ++i) {
        if (peaks[i] > 0 && peaks[i] < s_len) n_ev++;
    }
    if (n_ev == 0) return {};

    std::vector<Event> events(n_ev);

    uint32_t l_idx = 0;
    float l_prefixsum = 0.0f;

    uint32_t peaks_used = 0;
    for (uint32_t pi = 0; pi < peaks.size() && peaks_used < n_ev - 1; ++pi) {
        uint32_t peak_idx = peaks[pi];
        if (pi > 0 && (peak_idx == 0 || peak_idx >= s_len)) continue;
        
        uint32_t seg_start = l_idx;
        uint32_t seg_end = peak_idx > 0 ? peak_idx - 1 : 0;
        uint32_t seg_count = peak_idx - l_idx;
        float seg_sum = prefix_sum[peak_idx] - l_prefixsum;
        float seg_avg = seg_sum / std::max(1.0f, static_cast<float>(seg_count));
        
        events[peaks_used] = {seg_start, seg_end, seg_count, seg_avg};
        
        l_idx = peak_idx;
        l_prefixsum = prefix_sum[peak_idx];
        peaks_used++;
    }
    
    uint32_t seg_start_last = l_idx;
    uint32_t seg_end_last = s_len > 0 ? s_len - 1 : 0;
    uint32_t seg_count_last = s_len - l_idx;
    float seg_sum_last = prefix_sum[s_len] - l_prefixsum;
    float seg_avg_last = seg_sum_last / std::max(1.0f, static_cast<float>(seg_count_last));
    
    events[n_ev - 1] = {seg_start_last, seg_end_last, seg_count_last, seg_avg_last};

    return events;
}

// Main API: detect events from raw ADC signal
inline std::vector<Event> detect_events_from_raw(const std::vector<uint32_t>& sig_uint32,
                                                  const Options& opt = Options()) {
    std::vector<float> sig(sig_uint32.size());
    for (size_t i = 0; i < sig_uint32.size(); ++i) {
        sig[i] = static_cast<float>(sig_uint32[i]);
    }

    const uint32_t s_len = static_cast<uint32_t>(sig.size());
    if (s_len == 0) return {};

    std::vector<float> prefix_sum, prefix_sum_sq;
    comp_prefix_prefixsq(sig, prefix_sum, prefix_sum_sq);
    std::vector<float> t1 = comp_tstat(sig, prefix_sum, prefix_sum_sq, s_len, opt.window_length1);
    std::vector<float> t2 = comp_tstat(sig, prefix_sum, prefix_sum_sq, s_len, opt.window_length2);

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

#endif // ORACLE_SEGMENTER_HPP
