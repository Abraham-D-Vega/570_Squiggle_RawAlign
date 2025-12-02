#!/bin/bash
# run_gen_preds.sh
# Compile and run gen_preds.cpp

set -e

if [[ "$1" == "-h" || "$1" == "--help" || $# -lt 3 ]]; then
    echo "Usage: $0 <genome> <read_type> <num_reads>"
    echo "  genome: genome name (e.g., lambda, ecoli, covid)"
    echo "  read_type: <genome> or human"
    echo "  num_reads: number of reads to process"
    echo "Example: $0 ecoli ecoli 100"
    exit 0
fi

SRC="SquiggleSeeder/src/gen_preds.cpp"
EXE="SquiggleSeeder/src/gen_preds.o"

# Compile
echo "Compiling $SRC..."
g++ -std=c++17 -O3 -I SquiggleSeeder/src "$SRC" -o "$EXE"
echo "Compiled to $EXE"

echo "Running $EXE..."
"$EXE" "$@"
