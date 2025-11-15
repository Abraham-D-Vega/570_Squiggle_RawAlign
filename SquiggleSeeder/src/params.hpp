#pragma once
#include <cstdint>

// Constants derived from RawHash parameters (DO NOT CHANGE)

// # consecutive bases grouped into an event
constexpr int KMER_LEN = 6;

// Quantization
constexpr int Q = 9;
constexpr int p = 3;
constexpr int LOW_BITS = Q - p - 2;
constexpr int BITS_PER_EVENT = 8; // 2 ^ ceil(log2(Q-p))

/* Amount of events to group into a seed 
    VIRAL:                  N = 5
    SMALL (< 50M bases):    N = 6
    LARGE (> 50M bases):    N = 7
*/
constexpr int N = 5;

// # bits in hash value (CHANGEABLE)
constexpr int HASH_BITS = 32;

/*
    Hashtable configuration (CHANGEABLE)
    TODO: implement tiling logic in HashTable.cpp
*/
constexpr bool IS_TILED = false;
constexpr int TILE_SIZE = 100000; // # bases in reference genome to compute 1 hashtable
constexpr int TILE_OVERLAP = 10000; // # bases to overlap with previous reference genome segment

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