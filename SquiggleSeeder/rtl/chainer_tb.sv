`timescale 1ns/1ps
`include "utils.svh"   // Make sure Anchor is defined in here!

module chainer_tb;

    // Parameters (Set according to your actual max values!)
    //parameter MAX_NUM_SEEDS  = 32; // Example, update as per definition
    // parameter MAX_NUM_CHAINS = 8;  // Example, update as per definition

    // DUT signals
    Anchor[`MAX_NUM_SEEDS-1:0] seeds;
    Anchor[`MAX_NUM_CHAINS-1:0][`MAX_NUM_SEEDS-1:0] chains;

    // Instantiate the DUT
    chainer dut(
        .seeds(seeds),
        .chains(chains)
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
        read_seeds_from_file("covid_0.txt");

        // Wait for a short setup
        #10;

        // Optionally, print what you fed
        $display("Seeds input:");
        for (int k = 0; k < `MAX_NUM_SEEDS; k++) begin
            $display("Seed[%0d]: ref=%0d, query=%0d", k, seeds[k].r, seeds[k].q);
        end

        // Wait and observe DUT output
        #100;

        // (Optional) Print output chains
        for (int i = 0; i < `MAX_NUM_CHAINS; i++) begin
            $display("Chain[%0d]:", i);
            for (int j = 0; j < `MAX_NUM_SEEDS; j++) begin
                $display("  Chain[%0d][%0d]: ref=%0d, query=%0d",
                    i, j, chains[i][j].r, chains[i][j].q);
            end
        end

        // Finish simulation
        $finish;
    end
endmodule