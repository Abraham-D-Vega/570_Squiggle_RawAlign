//First processing element in a WINDOW SIZE string of elements
// Stores Start, End, Valid, and score of a chain
//End is the curret_seed and start is taken from the best scoring element to append to
`include "utils.svh"
`timescale 1ns/1ps

module pe_start(
    input Anchor c_start_in, //from level up
    input logic valid_in, //From level up
    input logic [`NUM_PEs-2:0] [31:0] prev_scores, //from other pes
    input Anchor[`NUM_PEs-2:0] prev_starts, //from other pes
    input Anchor[`NUM_PEs-2:0] prev_ends, //from other pes
    
    output Anchor curr_anchor, //To other pes
    output Anchor start_out,//to next pe
    output Anchor end_out, //to next pe
    output logic [31:0] score_out, //to next pe
    output logic valid_out //to next pe
);

    logic [10:0] dq;
    logic [31:0] dr, dev, candidate;
    always_comb begin
        score_out = 32'd100;
        start_out = c_start_in;
        end_out = c_start_in;
        valid_out = valid_in;
        curr_anchor = c_start_in;
        dev = '0;
        candidate = '0;
        for(int i = 0; i < `WINDOW_SIZE-1; i++) begin
            dq = (c_start_in.q > prev_ends[i].q) ?  c_start_in.q - prev_ends[i].q : '0;
            dr = (c_start_in.r > prev_ends[i].r) ? c_start_in.r - prev_ends[i].r : '0;
            if(dq != 0 && dr != 0) begin
                dev = ({21'b0, dq} > dr) ? ({21'b0, dq} - dr) : (dr - {21'b0, dq});
                 if(dev <= `MAX_DEV ) begin
                    candidate = ((prev_scores[i] + 100) > dev) ? prev_scores[i] + 100 - dev : 0;
                    if(candidate > score_out) begin
                        if(curr_anchor.q ==11'd117) begin
                            $display( "candidate = %0d prev_anchor.r = %0d", candidate, prev_ends[i].r);
                        end
                        score_out = candidate;
                        start_out = prev_starts[i];
                    end
                end
            end
        end
    end
    
endmodule
