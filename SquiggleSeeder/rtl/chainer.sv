//Chain Anchors and provide input to Aligner
`timescale 1ns/1ps
`include "utils.svh"

module chainer (
    input Anchor [`MAX_NUM_SEEDS-1:0] seeds,
    output Anchor [`MAX_NUM_CHAINS-1:0][`MAX_NUM_SEEDS-1:0] chains 
);
    // -----------------------------
    // 2. Build overlapping segments in reference space
    //    Example pattern: [0,110000), [100000,210000), ...
    // -----------------------------

    logic [31:0][`NUM_SEGMENTS-1:0] seg_begin; // Beginning index of each reference segment in seeds
    logic [31:0][`NUM_SEGMENTS-1:0] seg_end; // End index of each reference segment in seeds
    Chain [`NUM_SEGMENTS-1:0][`MAX_NUM_CHAINS-1:0] chains_per_segment;
    
    segment_characterizer seg_char (
        .seeds(seeds),
        .seg_begin(seg_begin),
        .seg_end(seg_end)
    );
    
    // -----------------------------
    // 3. Per-segment DP + local chain extraction
    // -----------------------------

    chain_extraction chain_extract(
        .seg_begin(seg_begin),
        .seg_end(seg_end),
        .seeds(seeds),
        .chains_per_segment(chains_per_segment)
    );

    // ---------------------------
    // 4. Find top five non overlapping chains
    // ---------------------------

    Chain [`NUM_SEGMENTS*`MAX_NUM_CHAINS-1:0] chain_in;
    Chain [`MAX_NUM_CHAINS-1:0] final_chains;
    logic [`MAX_NUM_CHAINS-1:0] valid_out;
    logic [`NUM_SEGMENTS*`MAX_NUM_CHAINS-1:0] valid_mask;
    
    assign chain_in = chains_per_segment;

    always_comb begin
        for (int j = 0; j < (`NUM_SEGMENTS*`MAX_NUM_CHAINS); j++) begin
            valid_mask[j] = (chain_in[j].score > 32'h0) ? 1'b1 : 1'b0;
        end
    end
    
    chain_sorter  #(
        .NUM_INPUT_CHAINS(`NUM_SEGMENTS*`MAX_NUM_CHAINS),
        .NUM_OUTPUT_CHAINS(`MAX_NUM_CHAINS)
    )
    top_level_sorter
    (
        .chain_in(chain_in),
        .valid(valid_mask),
        .chain_out(final_chains),
        .valid_out(valid_out)
    );

    always_comb begin
        chains = {(`MAX_NUM_SEEDS*(12 +32)*`MAX_NUM_CHAINS){1'b0}};
        for(int i = 0; i < `MAX_NUM_CHAINS; i++) begin //each of the five chains
            int j = 0;
                for(int k = 0; k < `MAX_NUM_SEEDS; k++) begin
                    if(valid_out[i]) begin
                        if(final_chains[i].anchors[k]) begin
                            chains[i][j] = seeds[k];
                            j++;
                        end
                    end
                end
            
        end
    end

endmodule
