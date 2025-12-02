// chain_sorter.sv
// Sorts chains by score and selects top MAX_CHAINS chains
// Uses iterative selection to find top-K without full sort
`timescale 1ns/1ps
`include "utils.svh"

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
                    if (valid_mask[k] && ((chain_in[k].anchors ^ chain_in[best_idx].anchors) != (chain_in[k].anchors | chain_in[best_idx].anchors)) && (k != best_idx)) begin
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