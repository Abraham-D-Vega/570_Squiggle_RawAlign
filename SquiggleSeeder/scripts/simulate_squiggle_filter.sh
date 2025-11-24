#!/bin/bash
# simulate_squiggle_filter.sh
# Simulates SquiggleFilter workflow: preprocess signals and run sDTW alignment

set -e

if [ $# -lt 1 ]; then
    echo "Usage: $0 <genome>"
    echo "  genome: genome name (e.g., lambda, ecoli, covid)"
    exit 1
fi

GENOME=$1

echo "================================================"
echo "sDTW Alignment Simulation for $GENOME"
echo "================================================"

# Step 1: Preprocess reference signal
echo ""
echo "Step 1: Preprocessing reference signal..."
python3 SquiggleSeeder/scripts/preprocess_ref.py "$GENOME"

if [ ! -f "datasets/$GENOME/ref.txt" ]; then
    echo "Error: Reference preprocessing failed - datasets/$GENOME/ref.txt not found"
    exit 1
fi

# Parse --num_files argument
NUM_FILES=100
while [[ $# -gt 0 ]]; do
    case $1 in
        --num_files)
            NUM_FILES="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

if [ "$GENOME" = "covid" ]; then
    READ_SIZE=5000
else
    READ_SIZE=10000
fi

# Step 2: Preprocess genome read signals
echo ""
echo "Step 2: Preprocessing $GENOME read signals..."z
python3 SquiggleSeeder/scripts/preprocess_reads.py "$GENOME" --num-files "$NUM_FILES" --read-size "$READ_SIZE"

# Check if genome read files were created
SAMPLE_GENOME_READ="datasets/$GENOME/${GENOME}_raw0.txt"
if [ ! -f "$SAMPLE_GENOME_READ" ]; then
        echo "Error: Read preprocessing failed - $SAMPLE_GENOME_READ not found"
        exit 1
fi

# Step 3: Preprocess human read signals
echo ""
echo "Step 3: Preprocessing human read signals..."
python3 SquiggleSeeder/scripts/preprocess_reads.py human --num-files "$NUM_FILES" --read-size "$READ_SIZE"

# Check if human read files were created
SAMPLE_HUMAN_READ="datasets/human/human_raw0.txt"
if [ ! -f "$SAMPLE_HUMAN_READ" ]; then
        echo "Error: Read preprocessing failed - $SAMPLE_HUMAN_READ not found"
        exit 1
fi

# Step 4: Compile C++ alignment program
echo ""
echo "Step 4: Compiling alignment program..."
g++ -std=c++17 -O3 -I SquiggleSeeder/src \
    SquiggleSeeder/src/simulate_squiggle_filter.cpp \
    -o SquiggleSeeder/src/simulate_squiggle_filter.o
echo "Compiled simulate_squiggle_filter.o"

# Step 5: Run sDTW alignments
echo ""
echo "Step 5: Running sDTW alignments..."
mkdir -p "results/squiggle_filter"
OUTPUT_FILE="results/squiggle_filter/${GENOME}_align.txt"

./SquiggleSeeder/src/simulate_squiggle_filter.o "$GENOME" "$OUTPUT_FILE" "$NUM_FILES"

# Step 6: Analyze results
echo ""
echo "Step 6: Analyzing alignment costs..."
echo ""

# Extract genome and human costs (column 4 now that read_size is in column 3)
GENOME_COSTS=$(grep "^\s*$GENOME" "$OUTPUT_FILE" | awk '{print $4}')
HUMAN_COSTS=$(grep "^\s*human" "$OUTPUT_FILE" | awk '{print $4}')

# Calculate mean and standard deviation for genome reads
GENOME_MEAN=$(echo "$GENOME_COSTS" | awk '{sum+=$1; sumsq+=$1*$1} END {print sum/NR}')
GENOME_STDDEV=$(echo "$GENOME_COSTS" | awk -v mean="$GENOME_MEAN" '{sum+=($1-mean)^2} END {print sqrt(sum/NR)}')

# Calculate mean and standard deviation for human reads
HUMAN_MEAN=$(echo "$HUMAN_COSTS" | awk '{sum+=$1; sumsq+=$1*$1} END {print sum/NR}')
HUMAN_STDDEV=$(echo "$HUMAN_COSTS" | awk -v mean="$HUMAN_MEAN" '{sum+=($1-mean)^2} END {print sqrt(sum/NR)}')

# Calculate separation (difference in means relative to pooled standard deviation)
SEPARATION=$(echo "$GENOME_MEAN $HUMAN_MEAN $GENOME_STDDEV $HUMAN_STDDEV" | \
    awk '{diff=$2-$1; pooled_std=sqrt(($3^2+$4^2)/2); print diff/pooled_std}')

# Qualitative assessment
if (( $(echo "$SEPARATION > 3.0" | bc -l) )); then
    ASSESSMENT="✓ Strong separation - highly discriminative"
elif (( $(echo "$SEPARATION > 2.0" | bc -l) )); then
    ASSESSMENT="✓ Good separation - discriminative"
elif (( $(echo "$SEPARATION > 1.0" | bc -l) )); then
    ASSESSMENT="⚠ Moderate separation - somewhat discriminative"
else
    ASSESSMENT="✗ Weak separation - poorly discriminative"
fi

# Append statistics to results file
echo "" >> "$OUTPUT_FILE"
echo "#" >> "$OUTPUT_FILE"
echo "# ================================================" >> "$OUTPUT_FILE"
echo "# Alignment Cost Statistics" >> "$OUTPUT_FILE"
echo "# ================================================" >> "$OUTPUT_FILE"
printf "# %-15s Mean: %8.0f  StdDev: %8.0f\n" "$GENOME reads:" "$GENOME_MEAN" "$GENOME_STDDEV" >> "$OUTPUT_FILE"
printf "# %-15s Mean: %8.0f  StdDev: %8.0f\n" "Human reads:" "$HUMAN_MEAN" "$HUMAN_STDDEV" >> "$OUTPUT_FILE"
echo "#" >> "$OUTPUT_FILE"
printf "# Separation: %.2f standard deviations\n" "$SEPARATION" >> "$OUTPUT_FILE"
echo "# $ASSESSMENT" >> "$OUTPUT_FILE"
echo "# ================================================" >> "$OUTPUT_FILE"

# Display statistics to console
echo "================================================"
echo "Alignment Cost Statistics"
echo "================================================"
printf "%-15s Mean: %8.0f  StdDev: %8.0f\n" "$GENOME reads:" "$GENOME_MEAN" "$GENOME_STDDEV"
printf "%-15s Mean: %8.0f  StdDev: %8.0f\n" "Human reads:" "$HUMAN_MEAN" "$HUMAN_STDDEV"
echo ""
printf "Separation: %.2f standard deviations\n" "$SEPARATION"
echo "$ASSESSMENT"

echo ""
echo "================================================"
echo "Simulation complete!"
echo "Results saved to: $OUTPUT_FILE"
echo "================================================"
