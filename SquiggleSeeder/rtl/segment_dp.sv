//Take in previous scores, prev, max_r, and min_r, and output score, prev, max_r, and min_r 
//TODO: Implement logic from lines 115 - 172 of chain_seeds.hpp
module segment_dp(
    input logic [31:0] score_in [`MAX_NUM_SEEDS],
    input logic [31:0] prev_in [`MAX_NUM_SEEDS],
    input logic [31:0] max_r_in [`MAX_NUM_SEEDS],
    input logic [31:0] min_r_in [`MAX_NUM_SEEDS],
    input Anchor seeds  [`MAX_NUM_SEEDS],
    output logic [31:0] score_out,
    output logic [31:0] prev_out,
    output logic [31:0] max_r_out,
    output logic [31:0] min_r_out
)


endmodule