`timescale 1ns/1ps
`include "utils.svh"
module segment_column_tb;

    //DUT signals
    Anchor [`MAX_NUM_SEEDS-1: 0] seeds;
    logic [31:0] s; //beginning of seeds in segment
    logic [31:0] e; //end of seeds in segment
    Chain [`MAX_NUM_CHAINS-1:0] chain_out;

    segment_column dut(
        .seeds(seeds),
        .s(s),
        .e(e),
        .chain_out(chain_out)
    );


    
    initial begin
        seeds = '0;
        seeds[0].q = 11'd12;
        seeds[0].r = 32'd10;
        seeds[0].valid = 1'b1;
        seeds[1].q = 11'd13;
        seeds[1].r = 32'd12;
        seeds[1].valid = 1'b1;
        seeds[2].q = 11'd14;
        seeds[2].r = 32'd14;
        seeds[2].valid = 1'b1;
        seeds[3].q = 11'd16;
        seeds[3].r = 32'd10;
        seeds[3].valid = 1'b1;
        seeds[4].q = 11'd23;
        seeds[4].r = 32'd35;
        seeds[4].valid = 1'b1;

         $display("Sorted seeds:");
        for (int i = 0; i < `MAX_NUM_SEEDS; i++) begin
            $display("seeds[%0d]: q=%0d, r=%0d, valid=%b", i, seeds[i].q, seeds[i].r, seeds[i].valid);
        end


        s = '0;
        e = 32'd4;
        
        #1

        $display("Segment Chains");
        for(int i = 0; i < `MAX_NUM_SEEDS; i++) begin
            $display("Segment_chain[%0d].score = %0d Segment_chain[%0d].anchors = %0b", i, dut.Segment_chains[i].score, i, dut.Segment_chains[i].anchors);
        end
            $display("Valid mask = %0b", dut.valid_mask);
            
        for(int i = 0; i < `MAX_NUM_CHAINS; i++) begin
            $display("Chain out [%0d].score = %0d Chain_out[%0d].anchors = %0b\n", i, chain_out[i].score, i, chain_out[i].anchors);
        end
        $finish;
    end

endmodule