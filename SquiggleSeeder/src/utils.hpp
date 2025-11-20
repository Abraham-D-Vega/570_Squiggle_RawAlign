#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
#include <cmath>
#include <cassert>

// Constants derived from RawHash parameters (DO NOT CHANGE)

// # consecutive bases grouped into an event
constexpr int KMER_LEN = 6;
static_assert(KMER_LEN == 6);

// Quantization (EXACT RawHash parameters)
// RawHash extracts top Q=9 bits from IEEE-754, then keeps:
//   - Top 2 bits (sign + MSB of exponent): bits [31:30]
//   - Low lq=3 bits from mantissa: bits [25:23]
// Total quantization: lq+2 = 5 bits per event
constexpr int Q = 9;           // Top bits to extract from float
constexpr int p = 4;           // Pruned middle bits (RawHash's p parameter)
constexpr int lq = Q - p - 2;  // Low bits to keep: 9-4-2 = 3
constexpr int BITS_PER_EVENT = lq + 2;  // Total quantization bits = 5
constexpr int LOW_BITS = lq;   // Bottom lq bits of the Q-bit window = 3
static_assert(BITS_PER_EVENT == 5);
static_assert(LOW_BITS == 3);

/* Amount of events to group into a seed (computed at runtime) 
    VIRAL:                  N = 5
    SMALL (< 50M bases):    N = 6
    LARGE (> 50M bases):    N = 7
*/
constexpr int VIRAL_BASE_THRESHOLD = 1'000'000;
constexpr int SMALL_BASE_THRESHOLD = 50'000'000;

// # bits in hash value (CHANGEABLE: 32 or 16)
constexpr int HASH_BITS = 32;
static_assert(HASH_BITS == 32 || HASH_BITS == 16);

/*
    Hash table configuration (CHANGEABLE)
*/
constexpr bool IS_TILED = false;
constexpr uint32_t TILE_SIZE = 100'000; // # seeds in reference genome to compute 1 hash table
constexpr uint32_t TILE_OVERLAP = 10'000; // # seeds to overlap with previous tile
static_assert(TILE_OVERLAP < TILE_SIZE);

// Hashing functions
constexpr uint64_t HASH32_MASK = (1ULL<<32)-1;
constexpr uint32_t HASH16_MASK = (1UL<<16)-1;

inline uint32_t hash64to32(uint64_t key){
    key = (~key + (key << 21)) & HASH32_MASK; // key = (key << 21) - key - 1;
    key = key ^ key >> 24;
    key = ((key + (key << 3)) + (key << 8)) & HASH32_MASK; // key * 265
    key = key ^ key >> 14;
    key = ((key + (key << 2)) + (key << 4)) & HASH32_MASK; // key * 21
    key = key ^ key >> 28;
    key = (key + (key << 31)) & HASH32_MASK;
    return static_cast<uint32_t>(key);
}

// Trivially converts 32 bit hash to 16 bit hash
inline uint16_t fold32to16(uint32_t h) {
    return static_cast<uint16_t>(h & HASH16_MASK);
}

// ============================================================================
// SHARED UTILITY FUNCTIONS FOR EVENT PROCESSING
// ============================================================================

/**
 * Quantize a normalized event value into 5-bit code using EXACT RawHash method.
 * 
 * RawHash quantization (from rsketch.c lines 180-181):
 *   signal = *((uint32_t*)&s_values[i]);
 *   tmpQuantSignal = signal>>30<<lq | ((signal>>shift_r)&mask_l_quant);
 * 
 * Where:
 *   - shift_r = 32 - Q = 32 - 9 = 23
 *   - mask_l_quant = (1 << lq) - 1 = (1 << 3) - 1 = 7 = 0b111
 * 
 * This extracts:
 *   - Top 2 bits: bits[31:30] << 3
 *   - Low 3 bits: bits[25:23]
 *   - Result: 5-bit code combining both
 */
inline uint8_t quantize_event(float x) {
    uint32_t bits;
    std::memcpy(&bits, &x, sizeof(float));

    // EXACT RawHash quantization
    constexpr uint32_t shift_r = 32 - Q;         // = 23
    constexpr uint32_t mask_l_quant = (1u << lq) - 1u;  // = 7 = 0b111
    
    // Extract top 2 bits (sign + MSB of exponent) and shift left by lq
    uint32_t top2 = bits >> 30;
    
    // Extract low lq bits from position shift_r
    uint32_t low = (bits >> shift_r) & mask_l_quant;
    
    // Combine: (top2 << lq) | low
    uint8_t code = static_cast<uint8_t>((top2 << lq) | low);
    assert(code < (1u << BITS_PER_EVENT));  // Should fit in 5 bits
    return code;
}

/**
 * Normalize a vector of event values using z-score normalization.
 * Computes mean and standard deviation, then normalizes: (x - mean) / stddev
 * 
 * @param events Input event values (double precision)
 * @param norm_events Output normalized events (float precision)
 */
inline void normalize_events(const std::vector<double>& events, std::vector<float>& norm_events) {
    const size_t n = events.size();
    if (n == 0) return;

    // Compute mean and variance
    double sum = 0.0, sum_sq = 0.0;
    for (double v : events) {
        sum += v;
        sum_sq += (v * v);
    }
    double mean = sum / n;
    double var = sum_sq / n - mean * mean;
    double stddev = std::sqrt(var);

    // Normalize
    norm_events.resize(n);
    for (size_t i = 0; i < n; ++i) {
        norm_events[i] = static_cast<float>((events[i] - mean) / stddev);
    }
}

/**
 * RawHash-style streaming normalization for reads.
 * Normalizes events in windows (default 1 second at 4kHz sampling = ~500 events).
 * This accounts for signal drift over time during sequencing.
 * 
 * @param events Input event values
 * @param norm_events Output normalized events
 * @param window_size Number of events per normalization window (default: 500 for 1 sec at 4kHz)
 */
inline void normalize_events_streaming(const std::vector<float>& events, std::vector<float>& norm_events, size_t window_size = 500) {
    const size_t n = events.size();
    if (n == 0) return;

    norm_events.resize(n);

    // Process events in sliding windows
    for (size_t i = 0; i < n; ++i) {
        // Define window boundaries centered on current event
        size_t window_start = (i >= window_size / 2) ? (i - window_size / 2) : 0;
        size_t window_end = std::min(n, window_start + window_size);
        
        // Compute mean and stddev for this window
        double sum = 0.0, sum_sq = 0.0;
        size_t window_count = window_end - window_start;
        for (size_t j = window_start; j < window_end; ++j) {
            sum += events[j];
            sum_sq += events[j] * events[j];
        }
        
        double mean = sum / window_count;
        double var = sum_sq / window_count - mean * mean;
        double stddev = std::sqrt(var);
        
        if (stddev < 1e-6) stddev = 1.0;  // Avoid division by zero
        
        // Normalize this event using window statistics
        norm_events[i] = static_cast<float>((events[i] - mean) / stddev);
    }
}

/**
 * Overload for float input events (global normalization - used for reference)
 */
inline void normalize_events(const std::vector<float>& events, std::vector<float>& norm_events) {
    const size_t n = events.size();
    if (n == 0) return;

    // Compute mean and variance
    double sum = 0.0, sum_sq = 0.0;
    for (float v : events) {
        sum += v;
        sum_sq += (v * v);
    }
    double mean = sum / n;
    double var = sum_sq / n - mean * mean;
    double stddev = std::sqrt(var);

    // Normalize
    norm_events.resize(n);
    for (size_t i = 0; i < n; ++i) {
        norm_events[i] = static_cast<float>((events[i] - mean) / stddev);
    }
}

/**
 * Quantize a vector of normalized events into codes.
 * 
 * @param norm_events Input normalized event values
 * @param codes Output quantized codes (5-bit values matching RawHash)
 */
inline void quantize_events(const std::vector<float>& norm_events, std::vector<uint8_t>& codes) {
    const size_t n = norm_events.size();
    codes.resize(n);
    for (size_t i = 0; i < n; ++i) {
        codes[i] = quantize_event(norm_events[i]);
    }
}

/**
 * Generate a hash from N consecutive event codes.
 * 
 * @param codes Quantized event codes
 * @param start_idx Starting index in codes vector
 * @param N Number of events per seed
 * @return Hash value (16-bit or 32-bit depending on HASH_BITS)
 */
inline uint32_t generate_seed_hash(const std::vector<uint8_t>& codes, size_t start_idx, int N) {
    uint64_t seed_code = 0;
    for (int j = 0; j < N; ++j) {
        seed_code <<= BITS_PER_EVENT;
        seed_code |= static_cast<uint64_t>(codes[start_idx + j]);
    }
    uint32_t h32 = hash64to32(seed_code);
    if (HASH_BITS == 32) {
        return h32;
    } else {
        return static_cast<uint32_t>(fold32to16(h32));
    }
}

/**
 * Determine N (events per seed) based on genome size.
 * 
 * @param genome_size Size of genome in bases
 * @return N value (5 for viral, 6 for small, 7 for large)
 */
inline int compute_N_from_genome_size(uint32_t genome_size) {
    if (genome_size < VIRAL_BASE_THRESHOLD) {
        return 5;
    } else if (genome_size < SMALL_BASE_THRESHOLD) {
        return 6;
    } else {
        return 7;
    }
}
