`ifndef __UTILS_SVH__
`define __UTILS_SVH__

// `timescale 1ns/1ns
`timescale 1ns/1ps
`define FALSE 1'h0
`define TRUE  1'h1

// Constants derived from RawHash parameters (DO NOT CHANGE)

// # consecutive bases grouped into an event
`define KMER_LEN 6

// Quantization (EXACT RawHash parameters)
// RawHash extracts top Q=9 bits from IEEE-754, then keeps:
//   - Top 2 bits (sign + MSB of exponent): bits [31:30]
//   - Low lq=3 bits from mantissa: bits [25:23]
// Total quantization: lq+2 = 5 bits per event
`define Q 9
`define p 4
`define lq (`Q - `p - 2)
`define BITS_PER_EVENT = (`lq + 2) //assert 5
`define LOW_BITS `lq //assert 3


/* Amount of events to group into a seed (computed at runtime) 
    VIRAL:                  N = 5
    SMALL (< 50M bases):    N = 6
    LARGE (> 50M bases):    N = 7
*/
`define VIRAL_BASE_THRESHOLD 1000000
`define SMALL_BASE_THRESHOLD 50000000

`define MAX_ALIGN_COST_FOR_POSITIVE 75000

// # bits in hash value (CHANGEABLE: 32 or 16)
`define HASH_BITS 16


/*
    Hash table configuration (CHANGEABLE)
*/
`define IS_TILED `FALSE
//Tile overlap should always be less than tile size
`define TILE_SIZE 100000
`define TILE_OVERLAP 10000

// Hashing functions
`define HASH32_MASK 64'hFFFFFFFF
`define HASH16_MASK 32'hFFFF

//Chaining Definitions
// `define CHAIN_SIZE 44*(`MAX_NUM_CHAINS*`MAX_NUM_SEEDS)
`define CHARACTERIZATION_DELAY 50
`define MAX_NUM_SEEDS 10
`define MAX_NUM_CHAINS 5
`define SEGMENT_SIZE `SEG_STRIDE + `SEG_OVERLAP // total width of each segment
`define SEG_STRIDE  400000               // distance between segment starts
`define WINDOW_SIZE 2000
`define SEG_OVERLAP `WINDOW_SIZE       // must be >= WINDOW_SIZE
`define MAX_DEV `WINDOW_SIZE
`define NUM_SEGMENTS 1 //TODO actually set this to a reasonable number
`define CNT_SIZE 10 // number of cnt bits for segment_characterizer
typedef struct packed {
    logic [10:0] q;
    logic [31:0] r;
    logic valid;
} Anchor; // 'my_packed_data_t' is the new type name

typedef struct packed{
    logic [31:0] score; 
    logic [`MAX_NUM_SEEDS-1:0] anchors ; // TODO: anchor_r and anchor_q form the archor pairs, determine max number of slots needed to store these in
} Chain;






// Depricated


// function logic [31:0] hash64to32(input logic [63:0] key_input);
//     logic [63:0] key;
//     key = key_input;

//     key = (~key + (key << 21)) & `HASH32_MASK;
//     key = key ^ (key >> 24);
//     key = ((key + (key << 3)) + (key << 8)) & `HASH32_MASK;
//     key = key ^ (key >> 14);
//     key = ((key + (key << 2)) + (key << 4)) & `HASH32_MASK;
//     key = key ^ (key >> 28);
//     //key = (key + (key << 31)) & `HASH32_MASK;
//     key = (key + (key << 31)) & ((1 << `HASH_BITS) -1);

//     return key[31:0];
// endfunction

// function logic [15:0] fold32to16(input logic [31:0] hash);
//     logic [31:0] hash_mask;
//     hash_mask = hash;
//     hash = (hash & `HASH16_MASK);

//     return hash[15:0];
// endfunction

// function logic [7:0] quantize_event(input shortreal float_x);

//     logic [31:0] shift_r;
//     logic [31:0] mask_l_quant;

//     shift_r = 32 - `Q;
//     mask_l_quant = (1 << `lq) - 1;

//     logic [31:0] top2 = (float_x >> 30);
//     logic [31:0] low = (float_x >> shift_r) & mask_l_quant;

//     logic [7:0] code;
//     code = ((top2 << `lq) | low);

//     return code[7:0];
// endfunction

// function quantize_events(input shortreal norm_events[], output logic[7:0] codes[]);
//     logic [31:0] n;
//     n = norm_events.size();
//     codes = new[n];
//     for(logic [31:0] i = 0; i < n; i++) begin
//         codes[i] = quantize_event(norm_events[i]);
//     end

// endfunction

// function normalize_events(input shortreal events[], output shortreal norm_events[]);

//     int n = events.size();
//     if(n == 0){
//         return;
//     }
//     real sum = 0.0;
//     real sum_sq = 0.0;
//     for(int i = 0; i < n; i++)begin
//         sum += events[i];
//         sum_sq += (events[i] * events[i]);
//     end
//     real mean = (sum / n);
//     real var = (sum_sq / n) - (mean * mean);   
//     real std_dev = sqrt(var);
    
//     norm_events = new[n];
//     for (int i = 0; i < n; i++)begin
//         norm_events[i] = ((events[i] - mean) / std_dev);
//     end

// endfunction

// function logic[31:0] generate_seed_hash(input logic[7:0] codes[], input logic[31:0] start_idx, input int num_events_per_seed);
//     logic[63:0] seed_code;
//     seed_code = 0;
//     for(int j = 0; j < num_events_per_seed; j++)begin
//         seed_code = (seed_code << `BITS_PER_EVENT);
//         seed_code = (seed_code | codes[start_idx + j]);
//     end
//     logic[31:0] hash32;
//     hash32 = hash64to32(seed_code);
//     if(`HASH_BITS == 32)begin
//         return hash32;
//     end else begin
//         return fold32to16(hash32);
//     end
// endfunction

// function int compute_N_from_genome_size(logic[31:0] genome_size);
//     if(genome_size < `VIRAL_BASE_THRESHOLD) begin
//         return 5;
//     end else if(genome_size < `SMALL_BASE_THRESHOLD) begin
//         return 6;
//     end else begin
//         return 7;
//     end
// endfunction

`endif 
