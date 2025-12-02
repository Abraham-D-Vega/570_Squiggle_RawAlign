// -----------------------------
// 3. Per-segment DP + local chain extraction
// -----------------------------

`timescale 1ns/1ps
`include "utils.svh"

module chain_extraction(
    input logic [`NUM_SEGMENTS-1:0][31:0] seg_begin,
    input logic [`NUM_SEGMENTS-1:0][31:0] seg_end,
    input Anchor [`MAX_NUM_SEEDS-1: 0] seeds,
    output Chain [`NUM_SEGMENTS-1:0][`MAX_NUM_CHAINS-1:0] chains_per_segment 
);

    // Each column reperesents a segment of the reference and is independant of all other columns
    genvar i;
    generate
        for (i = 0; i < `NUM_SEGMENTS; i++) begin
            segment_column col (
                .seeds(seeds),
                .s(seg_begin[i]),
                .e(seg_end[i]),
                .chain_out(chains_per_segment[i])
            );
        end
    endgenerate
endmodule