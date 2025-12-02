//Chain Anchors and provide input to Aligner
module Chainer(
    input Anchor seeds [`MAX_NUM_SEEDS-1: 0],
    output Anchor chains [`MAX_NUM_CHAINS-1:0][`MAX_NUM_SEEDS_IN_SEG-1: 0]
)

    // -----------------------------
    // 2. Build overlapping segments in reference space
    //    Example pattern: [0,110000), [100000,210000), ...
    // -----------------------------
    // 
    logic [31:0] seg_begin [`NUM_SEGMENTS];//Beginning index of each reference segment in seeds
    logic [31:0] seg_end [`NUM_SEGMENTS];//End index of each reference segment in seeds
    logic [31:0] r_min; 
    logic [31:0] r_max;
    Chain chains_per_segment [`NUM_SEGMENTS][`MAX_CHAINS]
    r_min = seeds[0].r;
    r_max = seeds[`MAX_NUM_SEEDS-1].r;


    segment_characterizer seg_char (
        seeds,
        seg_begin,
        seg_end
    );
    
    // -----------------------------
    // 3. Per-segment DP + local chain extraction
    // -----------------------------

    chain_extraction chain_extract(
        seg_begin,
        seg_end,
        seeds,
        chains_per_segment
    );

    //TODO: Add interface for step 4
    Chain [`NUM_SEGMENTS*`MAX_CHAINS-1:0]  chain_in;
    assign chain_in = chains_per_segment;

    logic [31:0] valid_mask [`MAX_NUM_SEEDS];
    always_comb begin
        for (int j = 0; j < (`NUM_SEGMENTS*`MAX_CHAINS); j++) begin
            valid_mask[j] = (chain_in[j].score > 32'h0) ? 32'h1 : 32'h0;
        end
    end
    Chain [`MAX_CHAINS-1:0] final_chains;
    logic [31:0] valid_out [`MAX_NUM_SEEDS];

    chain_sorter(
        .chain_in(chain_in),
        .valid(valid_mask),
        .chain_out(final_chains),
        .valid_out(valid_out)
    );

    always_comb {
        chains = '0;
        for(int i = 0; i < `MAX_CHAINS; i++) begin
            for(int j = 0; j < `NUM_SEEDS; j++) begin
                for(int k = 0; k < `NUM_SEEDS; k++) begin
                    if(valid_out[i]) begin
                        if(final_chains.anchors[k]) begin
                            chains[i][j] = seeds[k];
                            j++;
                        end
                    end
                end
            end
        end
    }
endmodule