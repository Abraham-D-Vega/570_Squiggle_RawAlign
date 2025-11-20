//Compare Chains by score,.....other variables in chain struct
module sort #(
    parameter NUM_CHAINS = 16
) ( input ri_chain_t input_chains [NUM_CHAINS],
    output ri_chain_t output_chains [NUM_CHAINS]
);
    //reg [NUM_CHAINS-1:0] sorted_bus;
    //always @(posedge clk) begin
    //    output_chains <= sorted_bus;
    //end

    integer i, j;
    ri_chain_t temp;
    ri_chain_t array [NUM_CHAINS];

    always_comb begin
        for (i = 0; i < NUM_CHAINS; i++) begin
            array[i] = input_chains[i];
        end

        for (i = NUM_CHAINS-1; i > 0; i--) begin
            for (j = 0; j < i; j++) begin
                if (chain_greater(array[j+1], array[j])) begin
                    temp         = array[j];
                    array[j]     = array[j+1];
                    array[j+1] = temp;
                end
            end
        end

        for (i = 0; i < NUM_CHAINS; i++) begin
            output_chains[i] = array[i];
        end
    end
endmodule

function automatic bit chain_greater(const ri_chain_t a, const ri_anchor_t b);
    if (a.alignment_score != b.alignment_score) begin
        return a.alignment_score > b.alignment_score;
    end

    if (a.chaining_score != b.chaining_score) begin
        return a.chaining_score > b.chaining_score;
    end

    if (a.n_anchors != b.n_anchors) begin
        return a.n_anchors > b.n_anchors;
    end

    if (a.strand != b.strand) begin
        return a.strand > b.strand;
    end

    if (a.reference_sequence_index != b.reference_sequence_index) begin
        return a.reference_sequence_index > b.reference_sequence_index;
    end

    if (a.start_position != b.start_position) begin
        return a.start_position > b.start_position;
    end

    if(a.end_position != b.end_position) begin
        return a.end_position > b.end_position;
    end

    return 0; //tie
endfunction