#!/bin/bash
set -e

# Usage: ./preprocess.sh <genome>

if [ $# -ne 1 ]; then
    echo "Usage: $0 <genome>"
    exit 1
fi

GENOME="$1"

sh SquiggleSeeder/scripts/gen_hash_table.sh "$GENOME"

echo "\nGenerating 8-bit events for reference genome to pass into SquiggleFilter"
python3 SquiggleSeeder/scripts/preprocess_ref.py "$GENOME"

# TODO: Add preprocessing script for reads from FAST5 files