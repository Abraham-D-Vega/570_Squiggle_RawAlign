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

module comp_mapq