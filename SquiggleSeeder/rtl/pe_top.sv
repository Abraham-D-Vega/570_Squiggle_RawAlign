// Top level for MAX_WINDOW_SIZE PEs per segment
`include "utils.svh"
`timescale 1ns/1ps
module pe_top (
    input logic clk,
    input logic [31:0] seg_start,
    input logic [31:0] seg_end,
    input Anchor [`MAX_NUM_SEEDS-1:0] seeds,
    input logic start, // tells when to start the computations

    output logic [31:0] best_score,
    output Anchor best_start,
    output Anchor best_end,
    output logic done // bit to say when this Segment has been fully processed and the best values are finalized
);
    Anchor c_start_in, curr_anchor, best_start_r, best_end_r, n_best_end, n_best_start;
    logic  valid_in, valid_r;
    logic [31:0] best_score_r, seg_end_r, index, n_best_score;

    // Previous structures for PE back to start_pe
    logic  [`WINDOW_SIZE-1:0][31:0] prev_scores;
    Anchor [`WINDOW_SIZE-1:0]       prev_starts;
    Anchor [`WINDOW_SIZE-1:0]       prev_ends;

    // PE passing variables
    logic [`WINDOW_SIZE-1:0] valid_out;    

    // Instantiate PE_Start
    pe_start pe_start_inst (
        .c_start_in(c_start_in),  
        .valid_in(valid_in), 
        .prev_scores(prev_scores[`WINDOW_SIZE-1:1]), 
        .prev_starts(prev_starts[`WINDOW_SIZE-1:1]), 
        .prev_ends(prev_ends[`WINDOW_SIZE-1:1]), 
        .curr_anchor(curr_anchor),
        .start_out(prev_starts[0]),
        .end_out(prev_ends[0]),
        .score_out(prev_scores[0]), 
        .valid_out(valid_out[0]) 
    );

    // Generate MAX_WINDOW_SIZE-1 PEs here
    genvar i;
    generate
        for(i = 0; i < `WINDOW_SIZE-1; i++) begin : pe_instances
            pe pe_inst(
                .clk(clk),
                .valid_in(valid_out[i]), 
                .score(prev_scores[i]),
                .c_start(prev_starts[i]),
                .c_end(prev_ends[i]),
                .curr_anchor(curr_anchor),
                .start(start),
                .valid_out(valid_out[i+1]),
                .c_start_out(prev_starts[i+1]),
                .c_end_out(prev_ends[i+1]),
                .score_out(prev_scores[i+1])
            );
        end
    endgenerate

    always_comb begin
        c_start_in = '0;
        valid_in   = '0;

        if (valid_r) begin
            c_start_in = seeds[index];

            if (index < seg_end_r) begin
                valid_in = 1'b1;
            end else begin
                valid_in = 1'b0;
            end
        end

        best_score = best_score_r;
        best_start = best_start_r;
        best_end = best_end_r;
        n_best_end = best_end_r;
        n_best_start = best_start_r;
        n_best_score = best_score_r;
       // $display("what the sigma valid_out = ", valid_out[`WINDOW_SIZE-1]);
        if(valid_out[`WINDOW_SIZE-1] == 1'b1) begin
            n_best_end = prev_ends[`WINDOW_SIZE-1];
            n_best_start = prev_starts[`WINDOW_SIZE-1];
            n_best_score = prev_scores[`WINDOW_SIZE-1];
        end
        done = ~valid_out[`WINDOW_SIZE-1] & (best_score_r != 0);
    end

    // Reset index on start and store end of segement index
    always @(posedge clk) begin
        if (start) begin
            index     <= seg_start;
            seg_end_r <= seg_end;
            valid_r   <= 1'b1;
            best_score_r <= '0;
            best_start_r <= '0;
            best_end_r   <= '0;
        end else if (index == seg_end_r) begin
            index     <= '0;
            seg_end_r <= '0;
            valid_r   <= 1'b0;
            best_score_r <= n_best_score;
            best_start_r <= n_best_start;
            best_end_r   <= n_best_end;
        end else begin
            index     <= index + 1;
            seg_end_r <= seg_end_r;
            valid_r   <= valid_r;
            best_score_r <= n_best_score;
            best_start_r <= n_best_start;
            best_end_r   <= n_best_end;
        end
        
        
    end
    // 
    // always @(posedge clk) begin
    //     best_start_r <= '0;
    //     best_end_r  <= '0;
    //     best_score_r <= '0;
    //     seg_end_r <= '0;
    //     index <= '0;
    //     valid_r <= '0;

    // end 
endmodule
