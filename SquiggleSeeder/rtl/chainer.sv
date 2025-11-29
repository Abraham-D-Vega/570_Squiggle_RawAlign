//Chain Anchors and provide input to Aligner
module Chainer(
    input Anchor seeds [MAX_NUM_SEEDS-1: 0],
    output Anchor chains [MAX_NUM_CHAINS-1:0][MAX_NUM_SEEDS_IN_SEG-1: 0]
)

    // -----------------------------
    // 2. Build overlapping segments in reference space
    //    Example pattern: [0,110000), [100000,210000), ...
    // -----------------------------
    // 
    logic [31:0] seg_b [`NUM_SEGMENTS];// Beginning index of each reference segment in seeds
    logic [31:0] seg_e [`NUM_SEGMENTS];//End index of each reference segment in seeds

     segment_characterizer(
        seeds,
        seg_b,
        seg_e
     )
endmodule