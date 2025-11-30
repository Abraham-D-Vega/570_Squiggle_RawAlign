// One column of the chainig array. Finds chains for one segment of the reference

module segment_column(
    input Anchor seeds [`MAX_NUM_SEEDS-1: 0],
    input logic [31:0] s, //beginning of seeds in segment
    input logic [31:0] e, //end of seeds in segment
    output Chain segment_chains [`MAX_CHAINS]
)
 logic [31:0] len;
 assign len = e - s;
 logic [31:0] score [`MAX_NUM_SEEDS];
 logic [31:0] prev [`MAX_NUM_SEEDS];
 logic [31:0] max_r [`MAX_NUM_SEEDS];
 logic [31:0] min_r [`MAX_NUM_SEEDS];
 // Find all idividual chains in the segment
genvar i
generate
    for(i = 0, i < `MAX_NUM_SEEDS; i++) begin
        segment_dp(
            .score_in(score),
            .prev_in(prev),
            .max_r_in(max_r),
            .min_r_in(min_r),
            .seeds(seeds),
            .score_out(score[i]),
            .prev_out(score[i]),
            .max_r_out(max_r[i]),
            .min_r_out(min_r[i])
        )
    end
endgenerate


//Sort chains of the segment

//Output all segments that do not overlap


endmodule
