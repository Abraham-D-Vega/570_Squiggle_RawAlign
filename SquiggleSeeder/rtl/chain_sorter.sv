// chain_sorter.sv
// Sorts chains by score and selects top MAX_CHAINS chains
// Uses iterative selection to find top-K without full sort
`include "utils.svh"
// module chain_sorter #(
//     parameter NUM_INPUT_CHAINS = 1000,  // Maximum chains to sort from
//     parameter NUM_OUTPUT_CHAINS = 5     // Top K chains to select
// )(
//     input  logic [31:0] scores_in [NUM_INPUT_CHAINS],  // Chain scores
//     input  logic [31:0] valid_mask [NUM_INPUT_CHAINS], // 1 if chain exists, 0 otherwise
//     output logic [31:0] sorted_indices [NUM_OUTPUT_CHAINS], // Indices of top chains
//     output logic [31:0] sorted_scores [NUM_OUTPUT_CHAINS],  // Scores of top chains
//     output logic [NUM_OUTPUT_CHAINS-1:0] valid_out          // Which output positions are valid
// );

//     // Internal tracking for iterative selection
//     logic [NUM_INPUT_CHAINS-1:0] used;  // Tracks which chains have been selected
    
//     always_comb begin
//         // Initialize outputs
//         sorted_indices = '{default: 32'hFFFFFFFF};
//         sorted_scores = '{default: 32'h0};
//         valid_out = '0;
//         used = '0;
        
//         // Iteratively find top NUM_OUTPUT_CHAINS scores
//         for (int k = 0; k < NUM_OUTPUT_CHAINS; k++) begin
//             logic [31:0] best_score;
//             logic [31:0] best_idx;
//             logic found; //dummy boolean variable
            
//             best_score = 32'h0;
//             best_idx = 32'hFFFFFFFF;
//             found = 1'b0;
            
//             // Find highest unused score
//             for (int i = 0; i < NUM_INPUT_CHAINS; i++) begin
//                 //check only which one is valid and 
//                 if (valid_mask[i] && !used[i]) begin
//                     // Compare as unsigned integers (scores are positive floats? represented as fixed-point)
//                     if (scores_in[i] > best_score) begin
//                         best_score = scores_in[i];
//                         best_idx = i;
//                         found = 1'b1;
//                     end
//                 end
//             end
            
//             // Store result if found
//             if (found) begin
//                 sorted_indices[k] = best_idx;
//                 sorted_scores[k] = best_score;
//                 valid_out[k] = 1'b1;
//                 used[best_idx] = 1'b1;
//             end else begin
//                 // No more valid chains
//                 break;
//             end
//         end
//     end

// endmodule

module chain_sorter #(
    parameter NUM_INPUT_CHAINS  = 1000,  // Maximum chains to sort from
    parameter NUM_OUTPUT_CHAINS = 5     // Top K chains to select
)
(
    input  Chain [NUM_INPUT_CHAINS-1:0]  chain_in,
    input  logic [NUM_INPUT_CHAINS-1:0]  valid,    // 1 if chain exists, 0 otherwise
    output Chain [NUM_OUTPUT_CHAINS-1:0] chain_out,
    output logic [NUM_OUTPUT_CHAINS-1:0] valid_out // Which output positions are valid
);
    logic [31:0] best_score;
    logic [31:0] best_idx;
    logic        found;
    logic [NUM_INPUT_CHAINS-1:0] valid_mask;

    always_comb begin
        valid_mask = valid;
        chain_out = '0;
        valid_out = '0;

        for (int i = 0; i < NUM_OUTPUT_CHAINS ; i++) begin 
            found = 1'b0;
            best_score = '0;
            best_idx = '0;

            for (int j = 0; j < NUM_INPUT_CHAINS; j++) begin
                if (valid_mask[j]) begin
                    if (chain_in[j].score > best_score) begin
                        best_score = chain_in[j].score;
                        best_idx = j;
                        found = 1'b1;
                    end
                end
            end

            if (found) begin
                // loop over all chains and invalidate the ones that share anchors using an XOR
                for (int k = 0; k < NUM_INPUT_CHAINS; k++) begin
                    if (valid_mask[k] && ((chain_in[k] ^ chain_in[best_idx]) != (chain_in[k] | chain_in[best_idx])) && (k != best_idx)) begin
                        valid_mask[k] = 1'b0;
                    end
                end

                chain_out[i].score = best_score;
                chain_out[i].anchors = chain_in[best_idx].anchors;
                valid_out[i] = 1'b1;
                valid_mask[best_idx] = 1'b0; // Invalidate this chain
            end else begin
                break;
            end
        end
    end
endmodule