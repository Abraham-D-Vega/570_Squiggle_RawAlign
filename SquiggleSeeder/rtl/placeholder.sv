// -----------------------------
// 4. Merge all segment chains & pick global best max_chains
//    Ensure we don't reuse the same (q,r) anchor across chains.
// -----------------------------


// TODO: write this module when you know what the format of in_chains will be
module mergeChains (
    input  Segment [`SIZE] in_chains,
    output SegmentChain allChains
)

endmodule

// Plan to get the MAX_NUM_CHAINS highest scores
// allocate first 5 elements of canidate array to an array of size MAX_NUM_CHAINS, sort them
// Go to the next element and check it against all elements in the currently selected ones
// 

module topChainSelection (
    input  SegmentChain allChains,
    output TopChains    topChains
);
    // local top chains storage
    logic [31:0] top_scores [`MAX_NUM_CHAINS];
    Anchor top_anchors      [`MAX_NUM_CHAINS];

    int i, j, k;

    always_comb begin 
        // Initalize with minimum values for scores and zero out the anchors
        top_scores  = '0;
        top_anchors = '0;

        for (i = 0; i < `MAX_NUM_ANCHORS ; i++) begin
            if (allChains.empty[i]) begin // if this entry is not empty 
                for (j = 0; j < `MAX_NUM_CHAINS ; j++) begin
                    if (allChains.score[i] > top_scores[j]) begin
                        // shift everything below [j] down by one
                        for (k = (`MAX_NUM_CHAINS - 1); k > j; k--) begin
                            top_scores[k]  = top_scores[k-1];
                            top_anchors[k] = top_anchors[k-1];
                        end

                        top_scores[j]  = allChains.score[i];
                        top_anchors[j] = allChains.anchors[i];

                        break;
                    end
                end
            end
        end
        
        // drive output
        for (j = 0; j < `MAX_NUM_CHAINS ; j++) begin
            topChains[j].score   = top_scores[j];
            topChains[j].anchors = top_anchors[j];
        end
    end

endmodule