`timescale 1ns/1ps
`include "utils.svh"   // Make sure Anchor is defined in here!

module chainer_tb;

    // Parameters (Set according to your actual max values!)
    //parameter MAX_NUM_SEEDS  = 32; // Example, update as per definition
    // parameter MAX_NUM_CHAINS = 8;  // Example, update as per definition

    // DUT signals
    Anchor[`MAX_NUM_SEEDS-1:0] seeds;
    logic clk, rst;
    Anchor [`MAX_NUM_CHAINS-1:0] chain_starts;
    Anchor [`MAX_NUM_CHAINS-1:0] chain_ends;
    // Instantiate the DUT
    chainer_new dut(
        .clk(clk),
        .rst(rst),
        .seeds(seeds),
        .chain_starts(chain_starts),
        .chain_ends(chain_ends)
    );

    // Task to read seeds from file
    task read_seeds_from_file(string filename);
        integer fd, code, i;
        string line;
        int reff, query;

        fd = $fopen(filename, "r");
        if (fd == 0) begin
            $fatal("Could not open seeds file: %s", filename);
        end

        i = 0;
        // Read lines one by one
        while (!$feof(fd) && i < `MAX_NUM_SEEDS) begin
            line = "";
            code = $fgets(line, fd);
            if (code != 0) begin // Only if read succeeded
                // Parse ref and query
                if ($sscanf(line, "%d,%d", query, reff) == 2) begin
                    seeds[i] = Anchor'{query[10:0], reff[31:0], 1'b1};
                    i++;
                end
            end
        end

        // Fill unused seeds with default value (optional)
        for (; i < `MAX_NUM_SEEDS; i++)
            seeds[i] = Anchor'{default: '0};

        $fclose(fd);
    endtask

    // Test sequence
    initial begin
        // Read input seeds
        read_seeds_from_file("../anchors/covid_0.txt");

        // Wait for a short setup
        #10;

        // Optionally, print what you fed
        $display("Seeds input:");
        for (int k = 0; k < `MAX_NUM_SEEDS; k++) begin
            if(seeds[k] != '0) begin
            $display("Seed[%0d]: ref=%0d, query=%0d", k, seeds[k].r, seeds[k].q);
            end
        end

        // Wait and observe DUT output
        #100;

        $display("\nSegment Characterization");
        for(int i = 0; i < `NUM_SEGMENTS; i++) begin
            $display("seg_begin[%0d]=%0d  seg_end[%0d]=%0d", i, dut.seg_begin[i], i, dut.seg_end[i]);
        end

        $display("\n Chains Per Segment:");
        for(int i = 0; i < `NUM_SEGMENTS; i++) begin
            $display("\nbest_start[%0d]:= %0d  best_end[%0d]:= %0d best_scores[%0d]:= %0d ", i, dut.best_starts[i], i, dut.best_starts[i], i, dut.best_scores[i]);
        end

        // (Optional) Print output chains
        for (int i = 0; i < `MAX_NUM_CHAINS; i++) begin
            $display("\nChain[%0d]:", i);
                if(chain_starts[i].r != '0 && chain_starts[i].q != '0) begin
                $display("  Chain_start[%0d]: ref=%0d, query=%0d",
                    i, chain_starts[i].r, chain_starts[i].q);
                $display("  Chain_ends[%0d]: ref=%0d, query=%0d",
                    i, chain_ends[i].r, chain_ends[i].q);
                end
        end
        // Finish simulation
        $finish;
    end
endmodule