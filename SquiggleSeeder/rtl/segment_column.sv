// One column of the chainig array. Finds chains for one segment of the reference
//TODO: Try and shrink down the sizes of all arrays to minimum values
`include "utils.svh"

module segment_column(
    input Anchor seeds [`MAX_NUM_SEEDS-1: 0],
    input logic [31:0] s, //beginning of seeds in segment
    input logic [31:0] e, //end of seeds in segment
    output Chain segment_chains [`MAX_NUM_CHAINS]
);
    logic [31:0] len;
    assign len = e - s;
    logic [31:0] score [`MAX_NUM_SEEDS];
    logic [31:0] prev [`MAX_NUM_SEEDS];
    logic [31:0] max_r [`MAX_NUM_SEEDS];
    logic [31:0] min_r [`MAX_NUM_SEEDS];
    
    // Find all individual chains in the segment
    genvar i;
    generate
        for(i = 0; i < `MAX_NUM_SEEDS; i++) 
            begin : segment_instance
                segment_dp segment_dp_inst(
                    .score_in(score),
                    .prev_in(prev),
                    .max_r_in(max_r),
                    .min_r_in(min_r),
                    .seeds(seeds),
                    .s(s),
                    .ip(i),
                    .score_out(score[i]),
                    .prev_out(prev[i]),
                    .max_r_out(max_r[i]),
                    .min_r_out(min_r[i])
                );
            end
    endgenerate

    Chain Segment_chains [`MAX_NUM_SEEDS];
    assign Segment_chains.anchors = prev;
    assign Segment_chains.score = score;


    // Sort chains of the segment by score
    
    // Create valid mask for chains (score > 0 means valid chain)
    logic [31:0] valid_mask [`MAX_NUM_SEEDS];
    always_comb begin
        for (int j = 0; j < `MAX_NUM_SEEDS; j++) begin
            valid_mask[j] = (score[j] > 32'h0) ? 32'h1 : 32'h0;
        end
    end
    
    // Sorted chain indices and scores
    logic [31:0] sorted_indices [`MAX_NUM_CHAINS];
    logic [31:0] sorted_scores [`MAX_NUM_CHAINS];
    logic [`MAX_NUM_CHAINS-1:0] valid_sorted;
    
    // Instantiate chain sorter
    chain_sorter #(
        .NUM_INPUT_CHAINS(`MAX_NUM_SEEDS),
        .NUM_OUTPUT_CHAINS(`MAX_NUM_CHAINS)
    ) sorter (
        .scores_in(score),
        .valid_mask(valid_mask),
        .sorted_indices(sorted_indices),
        .sorted_scores(sorted_scores),
        .valid_out(valid_sorted)
    );
    
    // Filter out overlapping chains
    logic [`MAX_NUM_CHAINS-1:0] valid_final;
    
    overlap_detector #(
        .NUM_CHAINS(`MAX_NUM_CHAINS),
        .MAX_SEEDS(`MAX_NUM_SEEDS)
    ) overlap_det (
        .sorted_indices(sorted_indices),
        .prev_array(prev),
        .segment_start(s),
        .valid_in(valid_sorted),
        .valid_out(valid_final)
    );

endmodule
