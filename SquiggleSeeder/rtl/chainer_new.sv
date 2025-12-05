//Chain Anchors and provide input to Aligner
`timescale 1ns/1ps
`include "utils.svh"

module chainer (
    input Anchor  [`MAX_NUM_SEEDS-1:0]  seeds,
    output Anchor [`MAX_NUM_CHAINS-1:0] chain_starts,
    output Anchor [`MAX_NUM_CHAINS-1:0] chain_ends
);
    logic [`NUM_SEGMENTS-1:0][31:0] seg_begin; // Beginning index of each reference segment in seeds
    logic [`NUM_SEGMENTS-1:0][31:0] seg_end; // End index of each reference segment in seeds
    logic [`MAX_NUM_SEGMENTS-1:0][31:0] best_scores;
    Anchor [`MAX_NUM_SEGMENTS-1:0] best_starts;
    Anchor [`MAX_NUM_SEGMENTS-1:0] best_ends;
    logic [`MAX_NUM_SEGMENTS-1:0] done;
    logic start; // signal from seg_characterizer saying when to start the PE looping
    
    segment_characterizer seg_char (
        .seeds(seeds),
        .seg_begin(seg_begin),
        .seg_end(seg_end),

        // TODO new
        .start(start)
    );

    // Generate MAX_NUM_SEGMENTS PE_Top's
    genvar i;
    generate
        for(i = 0; i < `MAX_NUM_SEGMENTS-1; i++) begin : pe_top_instances
            pe_top pe_top_inst(
                .clk(clk),
                .seg_start(seg_begin[i]),
                .seg_end(seg_end[i]),
                .seeds(seeds),
                .start(start),
                .best_score(best_scores[i]),
                .best_start(best_starts[i]),
                .best_end(best_ends[i]),
                .done(done[i])
            );
        end
    endgenerate

    // Check when all done bits are set and pick top 5 best chains and set chains output
    always_comb begin
        best_scores_in = '0;
        best_starts_in = '0;
        best_ends_in   = '0;

        if(done == '1) begin
            best_scores_in = best_scores;
            best_starts_in = best_starts;
            best_ends_in   = best_ends;
        end
    end

    // Pick top `MAX_NUM_CHAINS chains with hioghest scores
    top_chains picker (
        .socres_in(best_scores_in),
        .starts_in(best_starts_in),
        .ends_in(best_ends_in),
        .chain_starts(chain_starts),
        .chain_ends(chain_ends)
    );
endmodule