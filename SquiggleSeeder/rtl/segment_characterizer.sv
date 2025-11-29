//Takes in array of seeds and outputs start and end reference index of each section


module segment_characterizer(
    input Anchor seeds [`MAX_NUM_SEEDS-1: 0],
    ouput logic [31:0]  seg_b  [`NUM_SEGMENTS],
    output logic [31:0] seg_e  [`NUM_SEGMENTS]
)
    
    always_comb begin : segmentAssiment

        for(int j = 0; j < `NUM_SEGMENTS; j++)begin
            seg_b[j]  = 32'hFFFFFFFF;
            seg_e[j] = 32'h0;
            for(int i = 0; i < `MAX_NUM_SEEDS; i++)begin
                if(seeds[i] > j*`SEG_STRIDE && seeds[i] < (j*`SEG_STRIDE + `SEGMENT_SIZE)) begin
                    if(seeds[j].r < b) begin
                        seg_b[j] = i;
                    end
                    if(seeds[j].r > e)
                        seg_e[j] = i;
                    end
                end 
        end
    end




endmodule
