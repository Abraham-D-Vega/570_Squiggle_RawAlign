// One column of the chainig array. Finds chains for one segment of the reference
//TODO: Try and shrink down the sizes of all arrays to minimum values
`include "utils.svh"

module segment_column(
    input Anchor [`MAX_NUM_SEEDS-1: 0] seeds,
    input logic [31:0] s, //beginning of seeds in segment
    input logic [31:0] e, //end of seeds in segment
    output Chain [`MAX_NUM_CHAINS-1:0] chain_out 
);
    logic [31:0] len;
    logic [`MAX_NUM_SEEDS-1:0][31:0]  score ;
    logic [`MAX_NUM_SEEDS-1:0] [31:0] prev;
    logic [`MAX_NUM_SEEDS-1:0] [31:0]  max_r;
    logic [`MAX_NUM_SEEDS-1:0] [31:0] min_r;
    logic [`MAX_NUM_CHAINS-1:0] valid_out;
    Chain [`MAX_NUM_SEEDS-1:0] Segment_chains;
    // Find all individual chains in the segment
    assign len = e - s;
    genvar i;
    generate
        for(i = 0; i < `MAX_NUM_SEEDS; i++) 
            begin : segment_instance
                segment_dp segment_dp_inst(
                    .chains_in(Segment_chains),
                    .max_r_in(max_r),
                    .min_r_in(min_r),
                    .seeds(seeds),
                    .s(s),
                    .ip(i),
                    .chain_out(Segment_chains[i]),
                    .max_r_out(max_r[i]),
                    .min_r_out(min_r[i])
                );
            end
    endgenerate


    // Sort chains of the segment by score
    
    // Create valid mask for chains (score > 0 means valid chain)
    logic  [`MAX_NUM_SEEDS-1:0] valid_mask;
    always_comb begin
        for (int j = 0; j < `MAX_NUM_SEEDS; j++) begin
            valid_mask[j] = (score[j] > 32'h0) ? 1'b1 : 1'b0;
        end
    end




    // Instantiate chain sorter
    chain_sorter #(
        .NUM_INPUT_CHAINS(`MAX_NUM_SEEDS),
        .NUM_OUTPUT_CHAINS(`MAX_NUM_CHAINS)
    ) sorter (
        .chain_in(Segment_chains),
        .valid(valid_mask),
        .chain_out(chain_out),
        .valid_out(valid_out)
    );
    
endmodule
