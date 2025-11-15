#!/bin/bash
set -e

# Usage: ./run_hashtable.sh <genome>

if [ $# -ne 1 ]; then
    echo "Usage: $0 <genome>"
    exit 1
fi

GENOME="$1"

REF="data/${GENOME}/reference.fasta"
KMER="data/dna_kmer_model.txt"
OUT_DIR="SquiggleSeeder/hashtables/${GENOME}"
OUT="${OUT_DIR}/hashtable"

# Check input files
if [ ! -f "$REF" ]; then
    echo "Error: Reference genome not found at $REF"
    exit 1
fi

if [ ! -f "$KMER" ]; then
    echo "Error: K-mer lookup table not found at $KMER"
    exit 1
fi

# Ensure output directory exists
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

# Compile HashTable.cpp
echo "Compiling HashTable.cpp..."
g++ -O3 -std=c++17 SquiggleSeeder/src/HashTable.cpp -o hashtable_builder
if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit 1
fi

echo "Running hash table builder..."
./hashtable_builder "$REF" "$KMER" "$OUT"

if [ $? -eq 0 ]; then
    echo "Hash table(s) successfully generated at: $OUT_DIR/"
else
    echo "Error: Hash table generation failed."
    exit 1
fi