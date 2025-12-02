`timescale 1ns/1ps
`include "utils.svh"

module segment_dp_tb;

    localparam MAX_NUM_SEEDS = `MAX_NUM_SEEDS;

    // DUT signals
    Chain  [`MAX_NUM_SEEDS-1:0]chains_in;
    logic [`MAX_NUM_SEEDS-1:0][31:0] max_r_in;
    logic [`MAX_NUM_SEEDS-1:0][31:0] min_r_in;
    Anchor [`MAX_NUM_SEEDS-1:0] seeds ;
    logic [31:0] s;
    logic [31:0] ip;

    Chain chain_out;
    logic [31:0] max_r_out;
    logic [31:0] min_r_out;

    // Simple bubble sort by r member (ascending)
    // task automatic sort_seeds_by_r(ref Anchor arr[MAX_NUM_SEEDS]);
    //     int i, j;
    //     Anchor temp;
    //     for (i = 0; i < MAX_NUM_SEEDS-1; i++) begin
    //         for (j = 0; j < MAX_NUM_SEEDS-1-i; j++) begin
    //             if (arr[j].r > arr[j+1].r) begin
    //                 temp     = arr[j];
    //                 arr[j]   = arr[j+1];
    //                 arr[j+1] = temp;
    //             end
    //         end
    //     end
    // endtask

    // Initialize and sort seeds by r
    // task automatic init_inputs_sorted();
    //     int i;
    //     for (i = 0; i < MAX_NUM_SEEDS; i++) begin
    //         seeds[i].q     = $urandom_range(1,1000);
    //         seeds[i].r     = $urandom_range(1,1000);
    //         seeds[i].valid = 1'b1;
    //     end
    //     sort_seeds_by_r(seeds);
    //     s  = 0;
    //     ip = 1; // Simple ip
    // endtask

    

    segment_dp dut(
        .chains_in(chains_in),
        .max_r_in(max_r_in),
        .min_r_in(min_r_in),
        .seeds(seeds),
        .s(s),
        .ip(ip),
        .chain_out(chain_out),
        .max_r_out(max_r_out),
        .min_r_out(min_r_out)
    );

    initial begin
        
        

        $display("==== segment_dp Testbench (Sorted Seeds) ====");
        //init_inputs_sorted();
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
        for (int i = 0; i < MAX_NUM_SEEDS; i++) begin
            $display("seeds[%0d]: q=%0d, r=%0d, valid=%b", i, seeds[i].q, seeds[i].r, seeds[i].valid);
        end
            chains_in = '0;
            max_r_in = '0;
            min_r_in = '0;
            s = 0;
            ip = 0;
        $display("Chain in:");
        for (int i = 0; i < MAX_NUM_SEEDS; i++) begin
            $display("chains_in[%0d]: score =%0d, chain=%0b", i, chains_in[i].score, chains_in[i].anchors);
            $display("max_r_in[%0d] = %0d min_r_in[%0d] = %0d\n", i, max_r_in[i], i, min_r_in[i]);
        end

           
        #1; // Wait for combinational logic

        $display("chain_out.score:   %0d", chain_out.score);
        $display("chain_out.anchors: %b", chain_out.anchors);
        $display("max_r_out:         %0d", max_r_out);
        $display("min_r_out:         %0d", min_r_out);



        #1;
            chains_in[0].score = 32'd100;
         chains_in[0].anchors = 5'b001;
            max_r_in[0] = 32'd10;
            min_r_in[0] = 32'd10;
            s = 0;
            ip = 1;
        $display("Chain in:");
        for (int i = 0; i < MAX_NUM_SEEDS; i++) begin
            $display("chains_in[%0d]: score =%0d, chain=%0d", i, chains_in[i].score, chains_in[i].anchors);
            $display("max_r_in[%0d] = %0d min_r_in[%0d] = %0d", i, max_r_in[i], i, min_r_in[i]);
        end

        #1;

        $display("chain_out.score:   %0d", chain_out.score);
        $display("chain_out.anchors: %b", chain_out.anchors);
        $display("max_r_out:         %0d", max_r_out);
        $display("min_r_out:         %0d", min_r_out);

        #1;
            chains_in[1].score = 32'd199;
         chains_in[1].anchors = 5'b011;
            max_r_in[1] = 32'd12;
            min_r_in[1] = 32'd10;
            s = 0;
            ip = 2;
        $display("Chain in:");
        for (int i = 0; i < MAX_NUM_SEEDS; i++) begin
            $display("chains_in[%0d]: score =%0d, chain=%0d", i, chains_in[i].score, chains_in[i].anchors);
            $display("max_r_in[%0d] = %0d min_r_in[%0d] = %0d", i, max_r_in[i], i, min_r_in[i]);
        end

        #1;

        $display("chain_out.score:   %0d", chain_out.score);
        $display("chain_out.anchors: %b", chain_out.anchors);
        $display("max_r_out:         %0d", max_r_out);
        $display("min_r_out:         %0d", min_r_out);

        #1;
            chains_in[2].score = 32'd298;
         chains_in[2].anchors = 5'b111;
            max_r_in[2] = 32'd14;
            min_r_in[2] = 32'd10;
            s = 0;
            ip = 3;
        $display("Chain in:");
        for (int i = 0; i < MAX_NUM_SEEDS; i++) begin
            $display("chains_in[%0d]: score =%0d, chain=%0d", i, chains_in[i].score, chains_in[i].anchors);
            $display("max_r_in[%0d] = %0d min_r_in[%0d] = %0d", i, max_r_in[i], i, min_r_in[i]);
        end

        #1;

        $display("chain_out.score:   %0d", chain_out.score);
        $display("chain_out.anchors: %b", chain_out.anchors);
        $display("max_r_out:         %0d", max_r_out);
        $display("min_r_out:         %0d", min_r_out);

         #1;
            chains_in[3].score = 32'd100;
         chains_in[3].anchors = 5'b1000;
            max_r_in[3] = 32'd10;
            min_r_in[3] = 32'd10;
            s = 0;
            ip = 4;
        $display("Chain in:");
        for (int i = 0; i < MAX_NUM_SEEDS; i++) begin
            $display("chains_in[%0d]: score =%0d, chain=%0d", i, chains_in[i].score, chains_in[i].anchors);
            $display("max_r_in[%0d] = %0d min_r_in[%0d] = %0d", i, max_r_in[i], i, min_r_in[i]);
        end

        #1;

        $display("chain_out.score:   %0d", chain_out.score);
        $display("chain_out.anchors: %b", chain_out.anchors);
        $display("max_r_out:         %0d", max_r_out);
        $display("min_r_out:         %0d", min_r_out);

        // Edge case: invalid seed
        // seeds[s+ip].valid = 1'b0;
        // #1;
        // $display("--- After seed invalidation ---");
        // $display("chain_out.score:   %0d", chain_out.score);

        // seeds[s+ip].valid = 1'b1;
        // #1;

        $finish;
    end

endmodule