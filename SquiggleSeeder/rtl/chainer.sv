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
endmodule