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

    // Task to generate test chains
    task generate_test_chains();
        for (int i = 0; i < NUM_INPUT_CHAINS; i++) begin
            chain_in[i].score = 0;
            chain_in[i].anchors = '0;
        end

        // Set up 6 test chains with known scores
        chain_in[0].score = 100;
        valid[0] = 1'b1;
        
        chain_in[1].score = 500;
        valid[1] = 1'b1;
        
        chain_in[2].score = 300;
        valid[2] = 1'b1;
        
        chain_in[3].score = 400;
        valid[3] = 1'b1;
        
        chain_in[4].score = 200;
        valid[4] = 1'b1;
        
        chain_in[5].score = 999;
        valid[5] = 1'b1;
    endtask

    // Test sequence
    initial begin
        $display("=== chain_sorter Testbench ===\n");

        // Initialize all chains
        valid = '0;
        generate_test_chains();

        // Wait for combinational logic to settle
        #10;

        // Print input chains
        $display("Input Chains (valid entries):");
        for (int k = 0; k < NUM_INPUT_CHAINS; k++) begin
            if (valid[k]) begin
                $display("  Chain[%0d]: score=%0d", k, chain_in[k].score);
            end
        end

        // Print output chains
        $display("\nOutput Chains (Top %0d selected):", NUM_OUTPUT_CHAINS);
        for (int i = 0; i < NUM_OUTPUT_CHAINS; i++) begin
            if (valid_out[i]) begin
                $display("  Output[%0d]: score=%0d", i, chain_out[i].score);
            end else begin
                $display("  Output[%0d]: INVALID", i);
            end
        end

        // Finish simulation
        $finish;
    end

endmodule
