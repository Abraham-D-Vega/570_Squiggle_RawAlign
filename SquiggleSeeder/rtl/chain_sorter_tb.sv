`timescale 1ns/1ps
`include "utils.svh"

module chain_sorter_tb;

    parameter NUM_INPUT_CHAINS = 1000;
    parameter NUM_OUTPUT_CHAINS = 5;
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

    // Test sequence
    initial begin
        $display("chain sorter");

        // Initialize: all invalid
        valid = '0;
        for (int i = 0; i < NUM_INPUT_CHAINS; i++) begin
            chain_in[i].score = 0;
            chain_in[i].anchors = '0;
        end

        // 6 test chains
        chain_in[0].score = 100;
        chain_in[0].anchors = 100'h00000000000000000000000000000000000001;
        valid[0] = 1'b1;
        
        chain_in[1].score = 500;
        chain_in[1].anchors = 100'h00000000000000000000000000000000000002;
        valid[1] = 1'b1;
        
        chain_in[2].score = 300;
        chain_in[2].anchors = 100'h00000000000000000000000000000000000003;
        valid[2] = 1'b1;
        
        chain_in[3].score = 400;
        chain_in[3].anchors = 100'h00000000000000000000000000000000000004;
        valid[3] = 1'b1;
        
        chain_in[4].score = 200;
        chain_in[4].anchors = 100'h00000000000000000000000000000000000005;
        valid[4] = 1'b1;
        
        chain_in[5].score = 999;
        chain_in[5].anchors = 100'h00000000000000000000000000000000000006;
        valid[5] = 1'b1;

        #10;

        // Print input chains
        $display("\ninput chains = 6");
        $display("Index | Score");
        $display("------|------");
        for (int i = 0; i < NUM_INPUT_CHAINS; i++) begin
            if (valid[i]) begin
                $display("%5d | %5d", i, chain_in[i].score);
            end
        end

        // Print output
        $display("Rank | Output Score | Valid");
        $display("-----|--------------|------");
        for (int i = 0; i < NUM_OUTPUT_CHAINS; i++) begin
            if (valid_out[i]) begin
                $display("  %0d  | %12d | YES", i+1, chain_out[i].score);
            end else begin
                $display("  %0d  | %12s | NO", i+1, "---");
            end
        end

        $display("\nDone");
        $finish;
    end

endmodule
