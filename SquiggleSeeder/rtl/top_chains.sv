`timescale 1ns/1ps
`include "utils.svh"

module top_chains(
    input logic [`NUM_SEGMENTS-1:0][31:0] scores_in,
    input Anchor [`NUM_SEGMENTS-1:0] starts_in,
    input Anchor [`NUM_SEGMENTS-1:0] ends_in,

    output Anchor [`MAX_NUM_CHAINS-1:0] chain_starts,
    output Anchor [`MAX_NUM_CHAINS-1:0] chain_ends
); 
    logic [`MAX_NUM_CHAINS-1:0][31:0] top_scores;
    logic[`NUM_SEGMENTS-1:0] visited;
    logic [31:0] idx;
    always_comb begin
        top_scores = '0;
        chain_ends = '0;
        chain_starts = '0;
        visited = '0;
        
        for(int i = 0; i < `MAX_NUM_CHAINS; i++) begin
            idx = '0;
            for(int j = 0; j < `NUM_SEGMENTS; j++) begin
                if(~visited[j] && (scores_in[j] > top_scores[i])) begin
                    top_scores[i] = scores_in[j];
                    chain_starts[i] = starts_in[j];
                    chain_ends[i] = ends_in[j];
                    idx = j;
                end
            end
            visited[idx] = 1'b1;
        end
    end
endmodule