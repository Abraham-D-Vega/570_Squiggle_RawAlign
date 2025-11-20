

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