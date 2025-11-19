//Compare Chains by score,.....other variables in chain struct
module sort #(
    parameter SIZE = 16
) ( input wire clk,
    input ri_chain_t [SIZE-1:0] input,
    output ri_chain_t [SIZE-1:0] output
);
    reg [SIZE-1:0] sorted_bus;
    always @(posedge clk) begin
        output <= sorted_bus;
    end

    integer i, j;
    ri_chain_t [SIZE-1:0] temp;
    ri_chain_t [SIZE-1:0] array [1:];

    always @* begin
        for (i = 0; i < 


        for (i = SIZE; i > 0; i = i - 1) begin
            for (j = 1; j < i; j = j + 1) begin
                if (array[j] < array[j + 1]) begin
                    temp         = array[j];
                    array[j]     = array[j + 1];
                    array[j + 1] = temp;
                end
            end
        end