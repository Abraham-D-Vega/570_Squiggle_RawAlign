// -----------------------------
// 3. Per-segment DP + local chain extraction
// -----------------------------
//TODO: Implement submodules and then test
`include "utils.svh"

module chain_extraction(
    input logic [31:0] seg_begin [`NUM_SEGMENTS],
    input logic [31:0] seg_end [`NUM_SEGMENTS],
    input Anchor seeds [`MAX_NUM_SEEDS-1: 0],
    output Chain chains_per_segment [`NUM_SEGMENTS][`MAX_NUM_CHAINS]
);


// Each column reperesents a segment of the reference and is independant of all other columns
genvar i;
generate
    for (i = 0; i < `NUM_SEGMENTS; i++) begin
        segment_column col (
            .seeds(seeds),
            .s(seg_begin[i]),
            .e(seg_end[i]),
            .segment_chains(chains_per_segment[i])
        );
    end
endgenerate


endmodule