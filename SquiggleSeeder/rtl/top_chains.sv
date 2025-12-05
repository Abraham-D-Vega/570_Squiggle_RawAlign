module top_chains(
    input logic [`MAX_NUM_SEGMENTS-1:0][31:0] scores_in,
    input Anchor [`MAX_NUM_SEGMENTS-1:0] starts_in,
    input Anchor [`MAX_NUM_SEGMENTS-1:0] ends_in,

    output Anchor [`MAX_NUM_CHAINS-1:0] chain_starts,
    output Anchor [`MAX_NUM_CHAINS-1:0] chain_ends
); 
    logic [`MAX_NUM_CHAINS-1:0] top_scores;

    always_comb begin
        top_scores = '0;
        for(int i = 0; i < `MAX_NUM_SEGMENTS; i++) begin
            if(top_scores[0] < scores_in[i]) begin
                top_scores[0] = scores_in[i];
            end
            for(int j = 1; j < `MAX_NUM_CHAINS; j++) begin
                if(top_scores[j] < scores_in[i] && scores_in[i] < top_scores[j-1]) begin
                    top_scores[j] = scores_in[i];
                end
            end
        end
    end
endmodule