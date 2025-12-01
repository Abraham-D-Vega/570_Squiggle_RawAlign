//Take in previous scores, prev, max_r, and min_r, and output score, prev, max_r, and min_r 
//TODO: Test / make this code less cursed
//Each of these is equivalent to one iteration of the ip for loop in software model

module segment_dp(
    input logic [31:0] score_in [`MAX_NUM_SEEDS],//TODO: make sure that these indexes make sense
    input logic [31:0] prev_in [`MAX_NUM_SEEDS],
    input logic [31:0] max_r_in [`MAX_NUM_SEEDS],
    input logic [31:0] min_r_in [`MAX_NUM_SEEDS],
    input Anchor seeds  [`MAX_NUM_SEEDS],
    input logic [31:0] s,
    input logic [31:0] ip,
    output logic [31:0] score_out,
    output logic [31:0] prev_out,
    output logic [31:0] max_r_out,
    output logic [31:0] min_r_out
)

    logic [31:0] qi, ri, dq, dr, dev, candidate, new_min_r, new_max_r;
    logic [31:0] i;
    logic [`MAX_NUM_SEEDS-1:0] seeds_mask; //binary mask of what locations to try chaining from

    
    
    
    assign i = s + ip;
    assign qi = seeds[i].q;
    assign ri = seeds[i].r;

    always_comb begin
        seeds_mask = '0;
        score_out = 1;
        prev_out = '1;
        min_r_out = ri;
        max_r_out = ri;
        for(int j = s; j < i; j++)begin
            if(ri - seeds[j].r <= `WINDOW_SIZE) begin
                seeds_mask[j] = 1'b1;
                if(seeds[j].q < qi && seeds[j].r < ri)begin
                    new_min_r = (min_r_in[j-s] < ri) ? min_r_in[j-s] : ri;
                    new_max_r = (min_r_in[j-s] > ri) ? min_r_in[j-s] : ri;
                    if(new_max_r - new_min_r <= `WINDOW_SIZE)begin
                        dq = qi - seeds[j].q;
                        dr = ri -seeds[j].r;
                        if(dq != 0 && dr != 0) begin
                            dev = (dq > dr) ? (dq - dr) : (dr-dq);
                            if(dev <= `MAX_DEV) begin
                                candidate = score[j-s] + 1 - `LAMBDA * dev;
                                if(candidate > best_score) begin
                                    score_out = candidate;
                                    prev_out = (j-s);
                                    min_r_out = new_min_r;
                                    max_r_out = new_max_r;
                                end
                            end
                        end
                    end
                end

            end
        end
        
    end




endmodule