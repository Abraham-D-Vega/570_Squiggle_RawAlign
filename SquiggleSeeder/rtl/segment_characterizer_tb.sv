`timescale 1ns/1ps
`include "utils.svh"
// Make sure segment_characterizer is compiled / included in your file list
// or uncomment the following if you want to include directly:
// `include "segment_characterizer.sv"

module segment_characterizer_tb;

    // DUT interface
    Anchor [`MAX_NUM_SEEDS-1:0] seeds;
    logic  [`NUM_SEGMENTS-1:0][31:0] seg_begin;
    logic  [`NUM_SEGMENTS-1:0][31:0] seg_end;

    // Instantiate DUT
    segment_characterizer dut (
        .seeds     (seeds),
        .seg_begin (seg_begin),
        .seg_end   (seg_end)
    );

    // Initialize all seeds as invalid
    task automatic init_seeds();
        for (int i = 0; i < `MAX_NUM_SEEDS; i++) begin
            seeds[i].q     = '0;
            seeds[i].r     = '0;
            seeds[i].valid = 1'b0;
        end
    endtask

    // Print segment ranges
    task automatic print_segments();
        $display("==============================================================");
        $display(" Segment Characterizer Results");
        $display(" (indices into seeds array; seg_end is exclusive)");
        $display("==============================================================");
        for (int j = 0; j < `NUM_SEGMENTS; j++) begin
            if (seg_begin[j] != 32'hFFFFFFFF) begin
                $display("Segment %0d: begin = %0d, end = %0d",
                         j, seg_begin[j], seg_end[j]);
            end
            else begin
                $display("Segment %0d: <no seeds in this segment>", j);
            end
        end
        $display("==============================================================");
    endtask

    initial begin
        int fd;
        int code;
        int q_read;
        int r_read;
        int idx;

        init_seeds();

        // Covid0.txt lines look like:
        //  482,69
        //  422,70
        //  423,71
        // So:   <q>,<r>  in decimal with a comma between
        fd = $fopen("covid_0.txt", "r");
        if (fd == 0) begin
            $fatal(1, "ERROR: Could not open Covid0.txt");
        end

        idx = 0;
        while (!$feof(fd) && idx < `MAX_NUM_SEEDS) begin
            // Note the comma in the format string
            code = $fscanf(fd, "%d,%d\n", q_read, r_read);
            if (code == 2) begin
                seeds[idx].q     = q_read[10:0];   // q is 11 bits in Anchor
                seeds[idx].r     = r_read[31:0];   // r is 32 bits
                seeds[idx].valid = 1'b1;
                idx++;
            end
            else begin
                // If line is blank or malformed, you’ll see this
                $display("WARNING: Malformed/extra line in Covid0.txt (fscanf=%0d)", code);
            end
        end

        $fclose(fd);

        $display("Read %0d seeds from Covid0.txt", idx);

        // Combinational DUT, give it a delta cycle
        #1;

        print_segments();

        $finish;
    end

endmodule
