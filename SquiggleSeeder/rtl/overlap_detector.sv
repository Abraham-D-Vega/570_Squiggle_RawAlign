// overlap_detector.sv
// Filters chains to ensure no anchor positions are reused across multiple chains
// Processes chains in score order (highest to lowest) and marks overlapping ones as invalid

module overlap_detector #(
    parameter NUM_CHAINS = 5,
    parameter MAX_SEEDS = 1000
)(
    input  logic [31:0] sorted_indices [NUM_CHAINS],     // Indices pointing to chain endpoints
    input  logic [31:0] prev_array [MAX_SEEDS],          //do we need this?
    input  logic [31:0] segment_start,                   // Start index of this segment in seeds array
    input  logic [NUM_CHAINS-1:0] valid_in,              // Which chains are valid from sorting
    output logic [NUM_CHAINS-1:0] valid_out              // Which chains remain valid after overlap check
);


endmodule
