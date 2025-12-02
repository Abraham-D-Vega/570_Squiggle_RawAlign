`timescale 1ns/1ps
`include "utils.svh"

module chain_sorter_tb;

    // DUT parameters (should match chain_sorter)
    parameter NUM_INPUT_CHAINS = 1000;
    parameter NUM_OUTPUT_CHAINS = 5;

    // DUT signals
    Chain [NUM_INPUT_CHAINS-1:0] chain_in;
    logic [NUM_INPUT_CHAINS-1:0] valid;
    Chain [NUM_OUTPUT_CHAINS-1:0] chain_out;
    logic [NUM_OUTPUT_CHAINS-1:0] valid_out;

    // Instantiate the DUT
    chain_sorter #(
        .NUM_INPUT_CHAINS(NUM_INPUT_CHAINS),
        .NUM_OUTPUT_CHAINS(NUM_OUTPUT_CHAINS)
    ) dut(
        .chain_in(chain_in),
        .valid(valid),
        .chain_out(chain_out),
        .valid_out(valid_out)
    );

    // Task to generate test chains with OVERLAPPING anchors
    /* verilator lint_off WIDTHTRUNC */
    task generate_test_chains_with_overlap();
        for (int i = 0; i < NUM_INPUT_CHAINS; i++) begin
            chain_in[i].score = 0;
            chain_in[i].anchors = '0;
        end

        // Test Case 1: Non-overlapping chains (should all be selected)
        chain_in[0].score = 100;
        chain_in[0].anchors = 100'd1;  // Unique anchor set 1
        valid[0] = 1'b1;
        
        chain_in[1].score = 500;
        chain_in[1].anchors = 100'd2;  // Unique anchor set 2
        valid[1] = 1'b1;
        
        chain_in[2].score = 300;
        chain_in[2].anchors = 100'd4;  // Unique anchor set 3
        valid[2] = 1'b1;
        
        chain_in[3].score = 400;
        chain_in[3].anchors = 100'd8;  // Unique anchor set 4
        valid[3] = 1'b1;
        
        chain_in[4].score = 200;
        chain_in[4].anchors = 100'd16;  // Unique anchor set 5
        valid[4] = 1'b1;
        
        // Test Case 2: Overlapping chains
        chain_in[5].score = 999;   // Highest score and shares anchors with chain 1
        chain_in[5].anchors = 100'd2;
        valid[5] = 1'b1;
        
        chain_in[6].score = 450;   // High score and shares anchors with chain 3
        chain_in[6].anchors = 100'd8;
        valid[6] = 1'b1;
    endtask
    /* verilator lint_on WIDTHTRUNC */

    // Test sequence
    initial begin
        $display(" chain_sorter Testbench (OVERLAP TEST) \n");

        // Initialize all chains
        valid = '0;
        generate_test_chains_with_overlap();

        // Wait for combinational logic to settle
        #10;

        // Print input chains
        $display("Input Chains (test overlap detection):");
        for (int k = 0; k < NUM_INPUT_CHAINS; k++) begin
            if (valid[k]) begin
                $display("  Chain[%0d]: score=%0d, anchors=%0d", k, chain_in[k].score, chain_in[k].anchors);
            end
        end

        // Print output chains
        $display("\nOutput Chains (should exclude overlapping ones):");
        for (int i = 0; i < NUM_OUTPUT_CHAINS; i++) begin
            if (valid_out[i]) begin
                $display("  Output[%0d]: score=%0d, anchors=%0d", i, chain_out[i].score, chain_out[i].anchors);
            end else begin
                $display("  Output[%0d]: INVALID", i);
            end
        end

        $finish;
    end

endmodule
