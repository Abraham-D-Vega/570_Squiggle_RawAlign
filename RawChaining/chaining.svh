


typedef struct packed {
    logic [31:0] target_position;
    logic [31:0] query_position;
} ri_anchor_t;

localparam RI_CHAIN_MAX_ANCHORS = 8;
typedef struct packed {
    typedef logic signed [31:0] score;
    logic [31:0]      reference_sequence_index;
    logic [31:0]      start_position;
    logic [31:0]      end_position;
    logic [31:0]      n_anchors;
    logic [7:0]       mapq
    logic signed [31:0] strand;
    ri_anchor_t       anchors [RI_CHAIN_MAX_ANCHORS];
} ri_chain_t;