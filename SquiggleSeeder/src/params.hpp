#pragma once
#include <cstdint>

// Constants derived from RawHash parameters (DO NOT CHANGE)

// # consecutive bases grouped into an event
constexpr int KMER_LEN = 6;

// Quantization
constexpr int Q = 9;
constexpr int p = 3;
constexpr int LOW_BITS = Q - p - 2;
constexpr int BITS_PER_EVENT = Q - p;
static_assert(BITS_PER_EVENT > 0);

/* Amount of events to group into a seed (computed at runtime) 
    VIRAL:                  N = 5
    SMALL (< 50M bases):    N = 6
    LARGE (> 50M bases):    N = 7
*/
constexpr int VIRAL_BASE_THRESHOLD = 1'000'000; // TODO: Confirm this
constexpr int SMALL_BASE_THRESHOLD = 50'000'000;

// # bits in hash value (CHANGEABLE: 32 or 16)
constexpr int HASH_BITS = 32;
static_assert(HASH_BITS == 32 || HASH_BITS == 16);

/*
    Hashtable configuration (CHANGEABLE)
*/
constexpr bool IS_TILED = true;
constexpr uint32_t TILE_SIZE = 100'000; // # seeds in reference genome to compute 1 hashtable
constexpr uint32_t TILE_OVERLAP = 10'000; // # seeds to overlap with previous tile
static_assert(TILE_OVERLAP < TILE_SIZE);

// TODO: use a better hash function
uint32_t hash64to32(uint64_t x) {
    x ^= x >> 32;
    x *= 0xdaba0b6eb09322e3ULL;  // 1 multiplier
    x ^= x >> 32;
    return (uint32_t)x;
}

// Trivially converts 32 bit hash to 16 bit hash
uint16_t fold32to16(uint32_t h) {
    return (uint16_t)((h >> 16) ^ (h & 0xFFFF));
}