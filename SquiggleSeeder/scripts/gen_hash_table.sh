#!/bin/bash
set -e

# Usage: ./gen_hash_table.sh <genome>

if [ $# -ne 1 ]; then
    echo "Usage: $0 <genome>"
    exit 1
fi

GENOME="$1"

REF="data/${GENOME}/reference.fasta"
KMER="data/dna_kmer_model.txt"
OUT_DIR="SquiggleSeeder/hash_tables/${GENOME}"
OUT="${OUT_DIR}/hash_table"

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

# Compile hash_table.cpp
echo "Compiling hash_table.cpp..."
g++ -O3 -std=c++17 SquiggleSeeder/src/hash_table.cpp -o hash_table_builder
if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit 1
fi

echo "Running hash table builder..."
./hash_table_builder "$REF" "$KMER" "$OUT"

if [ $? -eq 0 ]; then
    echo "Hash table(s) successfully generated at: $OUT_DIR/"
else
    echo "Error: Hash table generation failed."
    exit 1
fi