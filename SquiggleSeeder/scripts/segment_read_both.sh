#!/bin/bash
set -euo pipefail

# Run both oracle and simple segmenters on a given input file
# Usage:
#   scripts/segment_read_both.sh <input_file>
#   scripts/segment_read_both.sh data/testbench/human_raw0.txt
#
# Outputs:
#   - SquiggleSeeder/events/oracle/<basename>.events.txt
#   - SquiggleSeeder/events/simple/<basename>.events.txt

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <input_file>"
    echo "Example: $0 data/testbench/human_raw0.txt"
    exit 1
fi

INPUT_FILE="$1"

if [[ ! -f "${INPUT_FILE}" ]]; then
    echo "[ERROR] Input file not found: ${INPUT_FILE}"
    exit 1
fi

# Extract basename without extension for output naming
BASENAME=$(basename "${INPUT_FILE}" .txt)

# Define output paths
ORACLE_OUT="SquiggleSeeder/events/oracle/${BASENAME}.events.txt"
SIMPLE_OUT="SquiggleSeeder/events/simple/${BASENAME}.events.txt"

echo "=========================================="
echo "Running segmentation on: ${INPUT_FILE}"
echo "=========================================="

# Run oracle segmenter
echo ""
echo "[1/2] Running ORACLE segmenter..."
sh SquiggleSeeder/scripts/segment_read_oracle.sh "${INPUT_FILE}" "${ORACLE_OUT}"

# Run simple segmenter
echo ""
echo "[2/2] Running SIMPLE segmenter..."
sh SquiggleSeeder/scripts/segment_read_simple.sh "${INPUT_FILE}" "${SIMPLE_OUT}"

echo ""
echo "=========================================="
echo "COMPLETED"
echo "=========================================="
echo "Oracle output: ${ORACLE_OUT}"
echo "Simple output: ${SIMPLE_OUT}"
echo ""
echo "Event counts:"
echo "  Oracle: $(tail -n +2 "${ORACLE_OUT}" | wc -l | xargs) events"
echo "  Simple: $(tail -n +2 "${SIMPLE_OUT}" | wc -l | xargs) events"
echo ""
