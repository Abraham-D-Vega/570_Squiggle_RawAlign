#!/usr/bin/env python3
import sys, os
import numpy as np

def get_fasta(fasta_fn):
    with open(fasta_fn, 'r') as fasta:
        return ''.join(fasta.read().split('\n')[1:])

def rev_comp(bases):
    return bases.replace('A','t').replace('T','a').replace('G','c').replace('C','g').upper()[::-1]

def load_model(kmer_model_fn):
    kmer_model = {}
    with open(kmer_model_fn, 'r') as model_file:
        for line in model_file:
            kmer, current = line.split()
            kmer_model[kmer] = float(current)
    return kmer_model

def discrete_normalize(seq, bits=8, minval=-4, maxval=4):
    mean = int(np.mean(seq))
    mean_avg_dev = int(np.mean(np.abs(seq - mean)))
    norm_seq = (seq - mean) / mean_avg_dev
    norm_seq[norm_seq < minval] = minval
    norm_seq[norm_seq > maxval] = maxval 
    norm_seq = ((norm_seq - minval) * (2**bits/(maxval-minval))).astype(int)
    return norm_seq

def ref_signal(fasta, kmer_model, k=6):
    signal = np.zeros(len(fasta))
    for kmer_start in range(len(fasta)-k):
        signal[kmer_start] = kmer_model[fasta[kmer_start:kmer_start+k]]
    return discrete_normalize(signal*100)

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 SquiggleSeeder/scripts/preprocess_ref.py <genome>")
        sys.exit(1)
    genome = sys.argv[1]
    data_dir = "data"
    kmer_model_fn = os.path.join(data_dir, "dna_kmer_model.txt")
    ref_fasta_fn = os.path.join(data_dir, genome, "reference.fasta")
    output_fn = os.path.join(data_dir, genome, "ref.txt")
    if not os.path.exists(ref_fasta_fn):
        print(f"Reference FASTA not found: {ref_fasta_fn}")
        sys.exit(1)
    kmer_model = load_model(kmer_model_fn)
    fasta = get_fasta(ref_fasta_fn)
    fwd_ref_sig = ref_signal(fasta, kmer_model)
    rev_ref_sig = ref_signal(rev_comp(fasta), kmer_model)
    ref = np.concatenate((fwd_ref_sig, rev_ref_sig))
    with open(output_fn, "w") as f:
        f.write("\n".join(str(int(x)) for x in ref))
    print(f"ref.txt generated at: {output_fn}")

if __name__ == "__main__":
    main()
