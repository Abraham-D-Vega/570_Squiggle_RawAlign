void comp_mapq(std::vector<ri_chain_t> &chains, const ri_mapopt_t *opt) {
  if (chains.size() == 1) {
    chains[0].mapq = 60;
    return;
  } else {
    int mapq;
	if(opt->flag & RI_M_DTW_EVALUATE_CHAINS){
		//TODO: Possibly need to adapt this mapq calculation, since the alignment score might behave differently than the chaining score
		//currently, this is the same as the chaining score calculation from rawhash
		mapq = 40 * (1 - chains[1].alignment_score / chains[0].alignment_score);
	}
	else{
		mapq = 40 * (1 - chains[1].chaining_score / chains[0].chaining_score);
	}

	if (mapq > 60) {
      mapq = 60;
    }
    if (mapq < 0) {
      mapq = 0;
    }
    chains[0].mapq = (uint8_t)mapq;
  }
}

`define RI_M_DTW_EVALUATE_CHAINS	0x2
//#define RI_M_DTW_EVALUATE_CHAINS	0x2


// COMPUTE MAPQ (variable of ri_chain_t)
module comp_mapq #(
    parameter NUM_CHAINS = 16
) (
    //INPUTS
    input ri_chain_t input_chains [NUM_CHAINS],
    input ri_mapopt_t opt,
    output ri_chain_t output_chain_MAPQ [NUM_CHAINS]
);
    
    ri_chain_t temp [NUM_CHAINS];
    temp = input_chains;

    if(NUM_CHAINS == 1){
        temp[0].mapq = 60;
    }
    else{
        reg [31:0] mapq;
        reg [7:0] mapq8;
        if(opt.flag & RI_M_DTW_EVALUATE_CHAINS){
            //need multiplier for this part of the chaining
        }
        else{
            //need multiplier for this part of the chaining
        }

        if (mapq > 60) {
            mapq = 60;
        }
        if (mapq < 0) {
            mapq = 0;
        }
        mapq8 = mapq[7:0];
        temp[0].mapq = mapq8;
    }

    always_comb begin 
        output_chain_MAPQ =  temp;
        //OR
        for (int i = 0; i < NUM_CHAINS; i++) begin
            output_chain_MAPQ[i] = temp[i];
        end
    end
