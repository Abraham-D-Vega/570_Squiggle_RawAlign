`timescale 1ns/1ps
`include "utils.svh"

module pe_start_tb;

    parameter int WINDOW_SIZE = `WINDOW_SIZE;
    parameter int MAX_DEV = `MAX_DEV;

    typedef struct packed {
        logic [10:0] q;
        logic [31:0] r;
        logic        valid;
    } Anchor;

    // DUT ports
    Anchor c_start_in;
    logic valid_in;
    logic [WINDOW_SIZE-2:0][31:0] prev_scores;
    Anchor [WINDOW_SIZE-2:0]      prev_starts;
    Anchor [WINDOW_SIZE-2:0]      prev_ends;

    Anchor curr_anchor;
    Anchor start_out;
    Anchor end_out;
    logic [31:0] score_out;
    logic valid_out;

    // Instantiate DUT
    pe_start dut (
        .c_start_in(c_start_in),
        .valid_in(valid_in),
        .prev_scores(prev_scores),
        .prev_starts(prev_starts),
        .prev_ends(prev_ends),
        .curr_anchor(curr_anchor),
        .start_out(start_out),
        .end_out(end_out),
        .score_out(score_out),
        .valid_out(valid_out)
    );

    initial begin
        $display("-- pe_start_tb (Anchor q[10:0], r[31:0], valid) --");
        
        valid_in = 1'b0;
        c_start_in.r = 32'b1;
        c_start_in.q = 11'd2;
        c_start_in.valid = 1'b0;
        prev_scores = '0;
        prev_starts = '0;
        prev_ends = '0;    
        $display("curr anchor.r = %0d curr anchor.q %0d start_out.q = %0d start_out.r = %0d end_out.r = %0d end_out.q = %0d score_out = %0d valid_out = %0b", curr_anchor.r, curr_anchor.q, start_out.r, start_out.q, end_out.r, end_out.q, score_out, valid_out); 

        #10

        valid_in = 1'b1;
        c_start_in.r = 32'b1;
        c_start_in.q = 11'd2;
        c_start_in.valid = 1'b1;
        prev_scores = '0;
        prev_starts = '0;
        prev_ends = '0;    
        $display("curr anchor.r = %0d curr anchor.q %0d start_out.q = %0d start_out.r = %0d end_out.r = %0d end_out.q = %0d score_out = %0d valid_out = %0b", curr_anchor.r, curr_anchor.q, start_out.r, start_out.q, end_out.r, end_out.q, score_out, valid_out); 

        #10
        $display("curr anchor.r = %0d curr anchor.q %0d start_out.q = %0d start_out.r = %0d end_out.r = %0d end_out.q = %0d score_out = %0d valid_out = %0b", curr_anchor.r, curr_anchor.q, start_out.r, start_out.q, end_out.r, end_out.q, score_out, valid_out); 
        c_start_in.r = 32'd2;
        c_start_in.q = 11'd6;
        prev_scores[0] = score_out;
        prev_starts[0] = start_out;
        prev_ends[0] = end_out;

        #10
        $display("curr anchor.r = %0d curr anchor.q %0d start_out.q = %0d start_out.r = %0d end_out.r = %0d end_out.q = %0d score_out = %0d valid_out = %0b", curr_anchor.r, curr_anchor.q, start_out.r, start_out.q, end_out.r, end_out.q, score_out, valid_out); 
        for(int i = 1; i < `WINDOW_SIZE-2; i++) begin
            prev_scores[i] = prev_scores[i-1];
            prev_starts[i] = prev_starts[i-1];
            prev_ends[i] = prev_ends[i-1];
        end
        prev_scores[0] = score_out;
        prev_starts[0] = start_out;
        prev_ends[0] = end_out;
        c_start_in.r = 32'd4;
        c_start_in.q = 11'd10;
        
        #10
        $display("curr anchor.r = %0d curr anchor.q %0d start_out.q = %0d start_out.r = %0d end_out.r = %0d end_out.q = %0d score_out = %0d valid_out = %0b", curr_anchor.r, curr_anchor.q, start_out.r, start_out.q, end_out.r, end_out.q, score_out, valid_out); 
        for(int i = 1; i < `WINDOW_SIZE-2; i++) begin
            prev_scores[i] = prev_scores[i-1];
            prev_starts[i] = prev_starts[i-1];
            prev_ends[i] = prev_ends[i-1];
        end
        prev_scores[0] = score_out;
        prev_starts[0] = start_out;
        prev_ends[0] = end_out;
        c_start_in.r = 32'd4;
        c_start_in.q = 11'd9;
        
        #10
        $display("curr anchor.r = %0d curr anchor.q %0d start_out.q = %0d start_out.r = %0d end_out.r = %0d end_out.q = %0d score_out = %0d valid_out = %0b", curr_anchor.r, curr_anchor.q, start_out.r, start_out.q, end_out.r, end_out.q, score_out, valid_out); 
        

        for(int i = 1; i < `WINDOW_SIZE-2; i++) begin
            prev_scores[i] = prev_scores[i-1];
            prev_starts[i] = prev_starts[i-1];
            prev_ends[i] = prev_ends[i-1];
        end
        prev_scores[0] = score_out;
        prev_starts[0] = start_out;
        prev_ends[0] = end_out;
        c_start_in.r = 32'd10000;
        c_start_in.q = 11'd957;
        
        #10
        $display("curr anchor.r = %0d curr anchor.q %0d start_out.q = %0d start_out.r = %0d end_out.r = %0d end_out.q = %0d score_out = %0d valid_out = %0b", curr_anchor.r, curr_anchor.q, start_out.r, start_out.q, end_out.r, end_out.q, score_out, valid_out); 
        

        
        $finish;
    end

endmodule
