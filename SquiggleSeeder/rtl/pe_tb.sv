module pe_tb;

    logic clk;
    logic valid_in;
    logic [31:0] score;
    Anchor c_start;
    Anchor c_end;
    Anchor curr_anchor;
    logic start;
    logic valid_out;
    Anchor c_start_out;
    Anchor c_end_out;
    logic [31:0] score_out;

    pe dut (
        .clk(clk),
        .valid_in(valid_in),
        .score(score),
        .c_start(c_start),
        .c_end(c_end),
        .curr_anchor(curr_anchor),
        .start(start),
        .valid_out(valid_out),
        .c_start_out(c_start_out),
        .c_end_out(c_end_out),
        .score_out(score_out)
    );

    always begin
        #5;
        clk = ~clk;
    end
    initial begin
        $display("beginning pe test \n");
        clk = '0;
        valid_in = '0;
        score = '0;
        c_start = '0;
        c_end = '0;
        curr_anchor = '0;
        start = '0;

        @(negedge clk);
        $display("valid out= %0d c_start_out= %0d c_end_out= %0d score_out= %0d", valid_out, c_start_out, c_end_out, score_out);
        
        start = 1'b1;
        @(negedge clk);
        $display("valid out= %0d c_start_out= %0d c_end_out= %0d score_out= %0d", valid_out, c_start_out, c_end_out, score_out);
        
        start = 1'b0;
        valid_in = 1'b1;
        score = 32'd100;
        c_start.q = 11'd1;
        c_start.r = 32'd1;
        c_end.q = 11'd1;
        c_end.r = 32'd1;
        c_start.r = 32'd1;
        curr_anchor.q = 11'd1;
        curr_anchor.r = 32'd1;

        @(negedge clk);
        $display("valid out= %0d c_start_out.r= %0d c_start_out.q= %0d c_end_out.r= %0d c_end_out.q=%0d score_out= %0d", valid_out, c_start_out.r, c_start_out.q, c_end_out.r, c_end_out.q, score_out);
        
        start = 1'b0;
        valid_in = 1'b1;
        score = 32'd100;
        c_start.q = 11'd0;
        c_start.r = 32'd1;
        c_end.q = 11'd0;
        c_end.r = 32'd1;
        c_start.r = 32'd1;
        curr_anchor.q = 11'd0;
        curr_anchor.r = 32'd1;

         @(posedge clk);
        $display("valid out= %0d c_start_out.r= %0d c_start_out.q= %0d c_end_out.r= %0d c_end_out.q=%0d score_out= %0d", valid_out, c_start_out.r, c_start_out.q, c_end_out.r, c_end_out.q, score_out);
        
        @(negedge clk);
        $display("valid out= %0d c_start_out.r= %0d c_start_out.q= %0d c_end_out.r= %0d c_end_out.q=%0d score_out= %0d", valid_out, c_start_out.r, c_start_out.q, c_end_out.r, c_end_out.q, score_out);
        
        start = 1'b0;
        valid_in = 1'b1;
        score = 32'd100;
        c_start.q = 11'd0;
        c_start.r = 32'd1;
        c_end.q = 11'd0;
        c_end.r = 32'd1;
        c_start.r = 32'd1;
        curr_anchor.q = 11'd0;
        curr_anchor.r = 32'd2;

        @(negedge clk);
        $display("valid out= %0d c_start_out.r= %0d c_start_out.q= %0d c_end_out.r= %0d c_end_out.q=%0d score_out= %0d", valid_out, c_start_out.r, c_start_out.q, c_end_out.r, c_end_out.q, score_out);
        
        $finish;
    end

endmodule;