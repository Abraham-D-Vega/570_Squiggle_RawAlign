//Take in previous scores, prev, max_r, and min_r, and output score, prev, max_r, and min_r 
//TODO: Test / make this code less cursed
//Each of these is equivalent to one iteration of the ip for loop in software model
`include "utils.svh"

module segment_dp(
    input Chain [`MAX_NUM_SEEDS-1:0] chains_in,
    input logic [`MAX_NUM_SEEDS-1:0] [31:0] max_r_in,
    input logic [`MAX_NUM_SEEDS-1:0] [31:0] min_r_in,
    input Anchor [`MAX_NUM_SEEDS-1:0] seeds,
    input logic [31:0] s,
    input logic [31:0] ip,
    output Chain chain_out,
    output logic [31:0] max_r_out,
    output logic [31:0] min_r_out
);

    logic [10:0] qi, dq;
    logic [31:0]  ri, dr, dev, candidate, new_min_r, new_max_r;
    logic [31:0] i;
    
    logic[`MAX_NUM_SEEDS-1:0] anchor_mask;
    
    assign i = s + ip;
    assign qi = seeds[i].q;
    assign ri = seeds[i].r;

    always_comb begin
        chain_out.score = 32'd100;
        anchor_mask = '0;
        chain_out = '0;
        min_r_out = ri;
        max_r_out = ri;
        new_max_r = ri;
        new_min_r = ri;
        dq = '0;
        dr = '0;
        dev = '0;
        candidate = '0;
        if(~seeds[i].valid) begin
            chain_out.score = '0;
        end
        else begin
        for(int j = s; j < i; j++)begin
            if(ri - seeds[j].r <= `WINDOW_SIZE ) begin
                if(seeds[j].q < qi && seeds[j].r < ri) begin
                    new_min_r = (min_r_in[j-s] < ri) ? min_r_in[j-s] : ri;
                    new_max_r = (min_r_in[j-s] > ri) ? max_r_in[j-s] : ri;
                    if(new_max_r - new_min_r <= `WINDOW_SIZE)begin
                        dq = qi - seeds[j].q;
                        dr = ri -seeds[j].r;
                        if(dq != 0 && dr != 0) begin
                            dev = (dq > dr) ? (dq - dr) : (dr-dq);
                            if(dev <= `MAX_DEV ) begin
                                candidate = chains_in[j-s].score + 100 - dev;
                                if(candidate > chain_out.score) begin
                                    chain_out.score = candidate;
                                    anchor_mask[i] = 1'b1;
                                    chain_out.anchors = chains_in[j-s].anchors & anchor_mask;
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
    end




endmodule
