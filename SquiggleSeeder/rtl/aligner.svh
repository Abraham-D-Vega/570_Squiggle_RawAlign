`ifndef __ALIGNER_SVH__
`define __ALIGNER_SVH__

typedef struct packed {
    logic[31:0] cost;
    logic[31:0] ref_start_pos;
    logic[31:0] ref_end_pos;
} SDTWResult;

typedef struct packed {
    SDTWResult sdtw_result;
    logic[31:0] chain_index;
    logic[31:0] read_start;
    logic[31:0] read_end;
    logic[31:0] ref_start;
    logic[31:0] ref_end;
} ChainSDTWResult;


function void discrete_normalize(input logic[15:0] seq[], output logic[7:0] out[], input int bits = 8,
                            input int minval = -4, input int maxval = 4);
    logic[31:0] n = seq.size();
    out = new[n];

    real mean = 0.0;
    for(logic[31:0] i = 0; i < n; i++) begin
        mean += seq[i];
    end
    mean = mean / n;

    //type'(var) is casting in SV for static_cast<type>(var)
    logic[15:0] mean_16b = logic'(mean);
    
    real mad = 0.0;
    for(logic[31:0] i = 0; i < n; i++) begin
        mad += abs(seq[i] - mean_16b);
    end
    
    mad = mad / seq.size();

    logic[15:0] mad_16b = logic'(mad);
    logic[15:0] scale = (1 << bits) / (maxval - minval);

    for(logic[31:0] i = 0; i < n; i++){  
        real norm_val;
        norm_val = real'(seq[i] - mean_16b) / mad;
        if (norm_val < minval) begin
            norm_val = minval;
        end 
        if (norm_val > maxval) begin
            norm_val = maxval;
        end
        out[i] = logic[7:0]'((norm_val - minval) * scale);
    }
endfunction

function SDTWResult sDTW(input logic[7:0] query[], input logic[7:0] ref[]);

    logic[31:0] n = query.size();
    logic[31:0] m = ref.size();

    logic[31:0] prev_consec[]; prev_consec = new[n];
    logic[31:0] curr_consec[]; curr_consec = new[n];
    logic[31:0] prev_cost[];   prev_cost   = new[n];
    logic[31:0] curr_cost[];   curr_cost   = new[n];
    logic[31:0] min_cost[];    min_cost    = new[n];
    logic[31:0] prev_start[];  prev_start  = new[n];
    logic[31:0] curr_start[];  curr_start  = new[n];

    logic[31:0] best_ref_start = 0;
    logic[31:0] best_ref_end = 0;
    logic[31:0] bonus = 10;

    prev_cost[0] = abs(query[0] - ref[0]);
    min_cost[0] = prev_cost[0];
    prev_start[0] = 0;

    for (logic[31:0] i = 1; i < n; i++) begin
        prev_cost[i] = prev_cost[i-1] + abs(query[i] - ref[0]);
        min_cost[i] = min_cost[i-1] + abs(query[i] - ref[0]);
        prev_start[i] = 0;
    end

    for (logic[31:0] j = 1; j < m; j++) begin
        curr_start[0] = j;

        for (logic[31:0] i = 1; i < n; i++) begin
            logic move;
            move = (prev_cost[i-1] - prev_consec[i-1] * bonus) < curr_cost[i-1];

            if(move) begin
                // Diagonal move from (i-1, j-1)
                curr_consec[i] = 0;
                curr_cost[i] = prev_cost[i-1] - prev_consec[i-1] * bonus + abs(query[i] - ref[j]);
                curr_start[i] = prev_start[i-1];
            end else begin
                // Vertical move from (i-1, j)
                curr_consec[i] = (prev_consec[i] + 1 > 10) ? 10 : prev_consec[i] + 1;
                curr_cost[i] = curr_cost[i-1] + abs(query[i] - ref[j]);
                curr_start[i] = curr_start[i-1];
            end

            // Update minimum and best positions
            if(curr_cost[i] < min_cost[i]) begin
                min_cost[i] = curr_cost[i];
                if (i == n-1) begin
                    best_ref_start = curr_start[i];
                    best_ref_end = j;
                end
            end
        end

        for(logic [31:0] i = 0; i < n; i++) begin
            prev_consec[i] = curr_consec[i];
            curr_consec[i] = 0;
            prev_cost[i]   = curr_cost[i];
            curr_cost[i]   = 0;
            prev_start[i]  = curr_start[i];
            curr_start[i]  = 0;
        end
    end

    SDTWResult result;
    result.cost = min_cost[N-1];
    result.ref_start_pos = best_ref_start;
    result.ref_end_pos = best_ref_end;
    return result;

endfunction


function SDTWResult single_sdtw(input logic[15:0] query[], input logic[7:0] ref[]);
    logic[7:0] query_norm[];
    discrete_normalize(query, query_norm);
    return sDTW(query_norm, ref);
endfunction

typedef struct packed {
    logic[31:0] first;
    logic[31:0] second;
} ChainPair;

function ChainSDTWResult run_best_chain_sdtw(
    input logic [15:0] raw_read[], input logic [7:0]  ref_signal[], 
    input ChainPair chains[][], input int window_size = 3000);

    logic [7:0] norm_read[]; 
    discrete_normalize(raw_read, norm_read);
    ChainSDTWResult best_result;
    best_result.sdtw_result.cost = 32'hFFFFFFFF;

    for (logic[31:0] i = 0; i < chains.size(); i++) begin
        ChainPair chain = chains[i][0];
        logic[31:0] ref_start;
        ref_start = (chain.second - 2 * chain.first);

        if (ref_start < 0) begin
            ref_start = 0;
        end

        logic[31:0] ref_end;
        ref_end = ref_start + window_size;

        if (ref_end > ref_signal.size()) begin
            ref_end = ref_signal.size();
            ref_start = ref_end - window_size;
        end

        logic [7:0] ref_seg[];
        ref_seg = new[window_size];
        //trying to create an array of ref_signal from (ref_start -> ref_end)
        //ref_end == ref_start + window_size. So we have window_size elements starting at ref_start
        for (logic[31:0] k = 0; k < window_size; k++) begin
            if (ref_start + k < ref_signal.size())
                ref_seg[k] = ref_signal[ref_start + k];
            else
                ref_seg[k] = 0;
        end

        SDTWResult sdtw_res = sDTW(norm_read, ref_seg);

        if(sdtw_res.cost < best_result.sdtw_result.cost) begin
            best_result.sdtw_result = sdtw_res;
            best_result.chain_index = i;
            best_result.read_start = 0;
            best_result.read_end = (norm_read.size());
            best_result.ref_start = ref_start;
            best_result.ref_end = ref_end;
        end
    end

    return best_result;

endfunction


`endif
