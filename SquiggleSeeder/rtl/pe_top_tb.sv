`timescale 1ns/1ps
`include "utils.svh"

module pe_top_tb;

    parameter int MAX_NUM_SEEDS = `MAX_NUM_SEEDS;
    parameter int WINDOW_SIZE   = `WINDOW_SIZE;

    // Clock and reset
    logic clk;
    logic rst;

    // Inputs for DUT
    logic [31:0] seg_start;
    logic [31:0] seg_end;
    Anchor [MAX_NUM_SEEDS-1:0] seeds;
    logic start;

    // Outputs from DUT
    logic [31:0] best_score;
    Anchor best_start;
    Anchor best_end;
    logic done;

    // DUT instance
    pe_top dut (
        .clk(clk),
        .seg_start(seg_start),
        .seg_end(seg_end),
        .seeds(seeds),
        .start(start),
        .best_score(best_score),
        .best_start(best_start),
        .best_end(best_end),
        .done(done)
    );

    // Simple clock generation
    //always #5 clk = ~clk;

    always @(posedge clk ) begin
        $display("c_start_in: %0d valid_in %0b", dut.c_start_in, dut.valid_in);
    end
    

    // Drive inputs and monitor outputs
    initial begin
        $display("---- pe_top Simple Unit Test ----");
        // Initialize inputs
        clk = '0;
        seg_start = '0;
        seg_end = 32'd4; // process 5 seeds (index 0..4)
        start = '1;
        $display("check1");
    
        // Fill seeds array with sample values
        // for (int i = 0; i < 5; i++) begin
        //     seeds[i].q = i[10:0];
        //     seeds[i].r  = i[31:0];
        //     seeds[i].valid = 1'b1;
        // end
         
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
        $display("check");
        @(negedge clk);
        $display("check");

        // Pulse start to begin
        start = 1'b1;
        #5;
        clk = ~clk;
        #5;
        clk = ~clk;
        $display("check");
        start = '0;

        // Wait for done to be asserted
       // wait (done == 1);
       #1000;
        $display("check");
        @(negedge clk);
        $display("check");
        $display("Simulation finished.");
        $display("Best Score  = %d", best_score);
        $display("Best Start  = {reff: %d, q: %d}", best_start.r, best_start.q);
        $display("Best End    = {reffpos: %d, q: %d}", best_end.r, best_end.q);

        $finish;
    end

endmodule
