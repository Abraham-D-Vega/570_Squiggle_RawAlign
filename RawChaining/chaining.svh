

`define DEFAULT_ALIGNMENTS 100

typedef struct packed {
    logic [31:0] target_position;
    logic [31:0] query_position;
} ri_anchor_t;

localparam RI_CHAIN_MAX_ANCHORS = 8;
typedef struct packed {
    typedef logic signed [31:0] chaining_score;
    typedef logic signed [31:0] alignment_score;
    logic [31:0]                reference_sequence_index;
    logic [31:0]                start_position;
    logic [31:0]                end_position;
    logic [31:0]                n_anchors;
    logic [7:0]                 mapq;    
    logic signed [31:0]         strand;
    ri_anchor_t                 anchors [RI_CHAIN_MAX_ANCHORS];
    dtw_result                  dtw_result;
} ri_chain_t;


typedef struct packed {
    logic [31:0] i; //position in the reference
    logic [31:0] j; //position in the read
} position_pair;

typedef struct packed {
    position_pair position;
    typedef logic signed [31:0] difference;
} alignment_element;

typedef struct packed {
    typedef logic signed [31:0] cost;
    alignment_element alignment [DEFAULT_ALIGNMENTS];
    logic [$clog2(DEFAULT_ALIGNMENTS):0] alignment_size;
} dtw_result;


typedef struct packed {
    // ONT Device specific parameters
    logic [31:0] bp_per_sec;
    logic [31:0] sample_rate;
    logic [31:0] chunk_size;

    // Chaining parameters
    logic [31:0] min_events;
    logic [31:0] max_gap_length;
    logic [31:0] max_target_gap_length;
    logic [31:0] chaining_band_length;
    logic [31:0] max_num_skips;
    logic [31:0] min_num_anchors;
    logic [31:0] num_best_chains;
    logic [31:0]    min_chaining_score;

    // Mapping parameters
    logic [31:0] step_size;
    logic [31:0] max_num_chunk;
    logic [31:0] min_chain_anchor;
    logic [31:0] min_chain_anchor_out;
    logic [31:0] dtw_border_constraint;
    logic [31:0] dtw_fill_method;
    logic [31:0] dtw_band_radius_frac;
    logic [31:0] dtw_match_bonus;
    logic [31:0] dtw_min_score;

    logic [31:0] min_bestmap_ratio;
    logic [31:0] min_bestmap_ratio_out;

    logic [31:0] min_meanmap_ratio;
    logic [31:0] min_meanmap_ratio_out;

    logic [31:0] t_threshold;
    logic [31:0] tn_samples;
    logic [31:0] ttest_freq;
    logic [31:0] tmin_reads;

    logic signed [63:0] flag;             // int64_t
    logic signed [63:0] mini_batch_size;  // int64_t

    // Event detector options
    logic [31:0] window_length1;
    logic [31:0] window_length2;
    logic [31:0] threshold1;
    logic [31:0] threshold2;
    logic [31:0] peak_height;
} ri_mapopt_t;
