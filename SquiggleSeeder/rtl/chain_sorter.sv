// chain_sorter.sv
// Sorts chains by score and selects top MAX_CHAINS chains
// Uses iterative selection to find top-K without full sort

module chain_sorter #(
    parameter NUM_INPUT_CHAINS = 1000,  // Maximum chains to sort from
    parameter NUM_OUTPUT_CHAINS = 5     // Top K chains to select
)(
    input  logic [31:0] scores_in [NUM_INPUT_CHAINS],  // Chain scores
    input  logic [31:0] valid_mask [NUM_INPUT_CHAINS], // 1 if chain exists, 0 otherwise
    output logic [31:0] sorted_indices [NUM_OUTPUT_CHAINS], // Indices of top chains
    output logic [31:0] sorted_scores [NUM_OUTPUT_CHAINS],  // Scores of top chains
    output logic [NUM_OUTPUT_CHAINS-1:0] valid_out          // Which output positions are valid
);

    // Internal tracking for iterative selection
    logic [NUM_INPUT_CHAINS-1:0] used;  // Tracks which chains have been selected
    
    always_comb begin
        // Initialize outputs
        sorted_indices = '{default: 32'hFFFFFFFF};
        sorted_scores = '{default: 32'h0};
        valid_out = '0;
        used = '0;
        
        // Iteratively find top NUM_OUTPUT_CHAINS scores
        for (int k = 0; k < NUM_OUTPUT_CHAINS; k++) begin
            logic [31:0] best_score;
            logic [31:0] best_idx;
            logic found; //dummy boolean variable
            
            best_score = 32'h0;
            best_idx = 32'hFFFFFFFF;
            found = 1'b0;
            
            // Find highest unused score
            for (int i = 0; i < NUM_INPUT_CHAINS; i++) begin
                //check only which one is valid and 
                if (valid_mask[i] && !used[i]) begin
                    // Compare as unsigned integers (scores are positive floats? represented as fixed-point)
                    if (scores_in[i] > best_score) begin
                        best_score = scores_in[i];
                        best_idx = i;
                        found = 1'b1;
                    end
                end
            end
            
            // Store result if found
            if (found) begin
                sorted_indices[k] = best_idx;
                sorted_scores[k] = best_score;
                valid_out[k] = 1'b1;
                used[best_idx] = 1'b1;
            end else begin
                // No more valid chains
                break;
            end
        end
    end

endmodule
