// 1999 subsequent PEs
`include "utils.svh"
`timescale 1ns/1ps

module pe (
    input logic clk,
    input logic valid_in, 
    input logic [31:0] score,
    input Anchor c_start,
    input Anchor c_end,
    input Anchor curr_anchor,
    input logic start,
    output logic valid_out,
    output Anchor c_start_out,
    output Anchor c_end_out,
    output logic [31:0] score_back,
    output logic [31:0] score_out // will be 0 if it is out of range of the canidate seed (ie PE0)
);
    reg [31:0] score_r;
    Anchor start_r, end_r;
    reg valid_r;

    always_comb begin
        valid_out = valid_r;
        c_start_out = start_r;
        c_end_out = end_r;
        score_out = '0;
        score_back = '0;
        if(valid_r)begin
            score_out = score_r;
        if(start_r.r < curr_anchor.r && start_r.q < curr_anchor.q) begin //monotonically increasing
            if(curr_anchor.r - start_r.r < `WINDOW_SIZE) begin
                score_back = score_r;
            end 
        end
        end
    end

    always @(posedge clk ) begin
        if(start) begin
            valid_r <= 1'b0;
            score_r <= '0;
            start_r <= '0;
            end_r <= '0;
        end    
        else begin
        if(valid_in) begin
            valid_r <= 1'b1;
            score_r <= score;
            start_r <= c_start;
            end_r <= c_end;
        end
        else begin
            valid_r <= 1'b0;
            score_r <= '0;
            start_r <= '0;
            end_r <= '0;
        end
        end
    end
endmodule