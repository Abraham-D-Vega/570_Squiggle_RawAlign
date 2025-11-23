#!/bin/bash
# simulate_seeder.sh
# Simulates seeding pipeline: hash table generation -> event detection -> seeding -> matching

set -e

if [ $# -lt 1 ]; then
    echo "Usage: $0 <genome> [--align]"
    echo "  genome: genome name (e.g., lambda, ecoli, covid)"
    echo "  --align: run alignment and show statistics (optional)"
    exit 1
fi

GENOME=$1
ALIGN_FLAG=""
if [[ "$@" =~ --align ]]; then
    ALIGN_FLAG="--align"
fi

echo "================================================"
echo "Seeding Simulation for $GENOME"
echo "================================================"

# Step 1: Compile hash table builder
echo ""
echo "Step 1: Compiling hash table builder..."
g++ -std=c++17 -O3 -I SquiggleSeeder/src \
    SquiggleSeeder/src/hash_table.cpp \
    -o hash_table_builder.o
echo "Compiled hash_table_builder.o"

# Step 2: Generate hash table (always regenerate)
echo ""
echo "Step 2: Generating hash table..."
./hash_table_builder.o "data/$GENOME/reference.fasta" "data/dna_kmer_model.txt" "datasets/$GENOME/hash_table"
if [ ! -f "datasets/$GENOME/hash_table0.txt" ]; then
    echo "Error: Hash table generation failed"
    exit 1
fi
echo "Generated datasets/$GENOME/hash_table0.txt"

# Step 3: Preprocess reference signal
echo ""
echo "Step 3: Preprocessing reference signal..."
python3 SquiggleSeeder/scripts/preprocess_ref.py "$GENOME"
if [ ! -f "datasets/$GENOME/ref.txt" ]; then
    echo "Error: Reference preprocessing failed"
    exit 1
fi

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

# Step 4: Preprocess genome read signals
echo ""
echo "Step 4: Preprocessing $GENOME read signals..."
python3 SquiggleSeeder/scripts/preprocess_reads.py "$GENOME" --num-files "$NUM_FILES" --read-size "$READ_SIZE"
SAMPLE_GENOME_READ="datasets/$GENOME/${GENOME}_raw0.txt"
if [ ! -f "$SAMPLE_GENOME_READ" ]; then
        echo "Error: Genome read preprocessing failed"
        exit 1
fi

# Step 5: Preprocess human read signals
echo ""
echo "Step 5: Preprocessing human read signals..."
python3 SquiggleSeeder/scripts/preprocess_reads.py human --num-files "$NUM_FILES" --read-size "$READ_SIZE"
SAMPLE_HUMAN_READ="datasets/human/human_raw0.txt"
if [ ! -f "$SAMPLE_HUMAN_READ" ]; then
    echo "Error: Human read preprocessing failed"
    exit 1
fi

# Step 6: Compile C++ seeding program
echo ""
echo "Step 6: Compiling seeding program..."
g++ -std=c++17 -O3 -I SquiggleSeeder/src \
    SquiggleSeeder/src/simulate_seeder.cpp \
    -o SquiggleSeeder/src/simulate_seeder.o
echo "Compiled simulate_seeder.o"

# Step 7: Run seeding simulation
echo ""
echo "Step 7: Running seeding simulation..."
mkdir -p "results/seeder"
OUTPUT_FILE="results/seeder/${GENOME}"

if [ -n "$ALIGN_FLAG" ]; then
    ./SquiggleSeeder/src/simulate_seeder.o "$GENOME" "$OUTPUT_FILE" "$NUM_FILES" --align
else
    ./SquiggleSeeder/src/simulate_seeder.o "$GENOME" "$OUTPUT_FILE" "$NUM_FILES"
fi

echo ""
echo "================================================"
echo "Simulation complete!"
echo "Results saved to: $OUTPUT_FILE"
echo "================================================"

if [ -n "$ALIGN_FLAG" ]; then
    # Step 8: Analyze results
    echo ""
    echo "Step 8: Analyzing alignment costs..."
    echo ""

    # Extract genome and human costs (column 4)
    ALIGN_FILE="${OUTPUT_FILE}_align.txt"
    GENOME_COSTS=$(grep "^\s*$GENOME" "$ALIGN_FILE" | awk '{print $4}')
    HUMAN_COSTS=$(grep "^\s*human" "$ALIGN_FILE" | awk '{print $4}')

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
    echo "" >> "$ALIGN_FILE"
    echo "#" >> "$ALIGN_FILE"
    echo "# ================================================" >> "$ALIGN_FILE"
    echo "# Alignment Cost Statistics" >> "$ALIGN_FILE"
    echo "# ================================================" >> "$ALIGN_FILE"
    printf "# %-15s Mean: %8.0f  StdDev: %8.0f\n" "$GENOME reads:" "$GENOME_MEAN" "$GENOME_STDDEV" >> "$ALIGN_FILE"
    printf "# %-15s Mean: %8.0f  StdDev: %8.0f\n" "Human reads:" "$HUMAN_MEAN" "$HUMAN_STDDEV" >> "$ALIGN_FILE"
    echo "#" >> "$ALIGN_FILE"
    printf "# Separation: %.2f standard deviations\n" "$SEPARATION" >> "$ALIGN_FILE"
    echo "# $ASSESSMENT" >> "$ALIGN_FILE"
    echo "# ================================================" >> "$ALIGN_FILE"

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
fi
