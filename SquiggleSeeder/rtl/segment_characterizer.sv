//Takes in array of seeds and outputs start and end reference index of each section
`timescale 1ns/1ps
`include "utils.svh"

module segment_characterizer(
    input Anchor [`MAX_NUM_SEEDS-1:0] seeds,
    input  logic rst,
    input  logic clk,
    output logic [`NUM_SEGMENTS-1:0][31:0] seg_begin,
    output logic [`NUM_SEGMENTS-1:0][31:0] seg_end,
    output logic start
);
    logic [`CNT_SIZE-1:0] cnt;
    
    always_comb begin : segmentAssiment
        for(int j = 0; j < `NUM_SEGMENTS; j++) begin
            logic [31:0] b = 32'hFFFFFFFF;
            logic [31:0] e = 32'h0;
            logic [31:0] seg_lo = j * `SEG_STRIDE;
            logic [31:0] seg_hi = j * `SEG_STRIDE + `SEGMENT_SIZE;

            seg_begin[j]  = 32'hFFFFFFFF;
            seg_end[j] = 32'h0;

            for (int i = 0; i < `MAX_NUM_SEEDS; i++) begin
                if (seeds[i].valid) begin
                    if (seeds[i].r > seg_lo && seeds[i].r < seg_hi) begin
                        if (b == 32'hFFFFFFFF) begin
                            b = i; //update first match 
                        end
                        e = i; //update to be latest match
                    end 
                end
            end

            if (b != 32'hFFFFFFFF) begin //Update if valid beginning
                seg_begin[j] = b;
                seg_end[j] = e+1;
            end
        end
        //where a return statement could go?
    end

    always_ff @(posedge clk) begin
        if (rst) begin  
            cnt <= '0;
        end else if (cnt != ..) begin
            cnt <= cnt + 1;
        end else if (cnt == ...) begin
            start <= 1'b1;
            cnt <= cnt + 1;
        end else begin
            start <= 1'b0;
            cnt <= cnt;
        end
    end
endmodule
