#!/usr/bin/env python3
"""
preprocess_reads.py

Extracts raw ADC values from FAST5 files without any normalization.
Creates N files with entire raw signal from each read.
Output files are stored in datasets/lambda/ directory.
"""

import os
import sys
import random
from glob import glob

# Import required packages
import numpy as np

# Use ont-fast5-api for better VBZ compression support
try:
    from ont_fast5_api.fast5_interface import get_fast5_file
    USE_ONT_API = True
except ImportError:
    # Fall back to h5py with hdf5plugin for VBZ support
    import h5py
    import hdf5plugin
    USE_ONT_API = False


def extract_signal_from_fast5_ont_api(fast5_path, max_length=5000, read_index=0):
    """
    Extract raw ADC signal using ONT Fast5 API (better VBZ compression support).
    Returns raw signal truncated to max_length samples.
    
    Args:
        fast5_path: Path to FAST5 file
        max_length: Maximum number of samples to extract
        read_index: Index of read to extract from multi-FAST5 files
    """
    try:
        with get_fast5_file(fast5_path, mode="r") as f5:
            read_ids = f5.get_read_ids()
            if len(read_ids) == 0 or read_index >= len(read_ids):
                return None
            
            # Get read at specified index
            read_id = read_ids[read_index]
            read = f5.get_read(read_id)
            
            # Get raw signal (ADC values)
            signal = read.get_raw_data()
            
            # Filter out values outside 0-1023 range
            signal = signal[(signal >= 0) & (signal <= 1023)]
            
            # Truncate to max_length
            if len(signal) > max_length:
                signal = signal[:max_length]
            
            if len(signal) > 0:
                return signal
                        
    except Exception as e:
        print(f"Error reading {fast5_path} (read {read_index}): {e}", file=sys.stderr)
        return None
    
    return None


def extract_signal_from_fast5_h5py(fast5_path, max_length=5000, read_index=0):
    """
    Extract raw ADC signal using h5py (for VBZ-compressed FAST5 files).
    Returns raw signal truncated to max_length samples.
    
    Args:
        fast5_path: Path to FAST5 file
        max_length: Maximum number of samples to extract
        read_index: Index of read to extract from multi-FAST5 files
    """
    try:
        import h5py
        # hdf5plugin is imported at module level, which automatically registers filters
        with h5py.File(fast5_path, 'r') as f:
            # Get list of reads in this file
            read_ids = list(f.keys())
            if len(read_ids) == 0 or read_index >= len(read_ids):
                return None
            
            # Get read at specified index
            read_id = read_ids[read_index]
            
            # Extract raw signal (ADC values as int16)
            signal = np.array(f[read_id]['Raw']['Signal'][:], dtype=np.float64)
            
            # Filter out values outside 0-1023 range
            signal = signal[(signal >= 0) & (signal <= 1023)]
            
            # Truncate to max_length
            if len(signal) > max_length:
                signal = signal[:max_length]
            
            if len(signal) > 0:
                return signal
                    
    except Exception as e:
        print(f"Error reading {fast5_path} (read {read_index}): {e}", file=sys.stderr)
        return None
    
    return None


def extract_signal_from_fast5(fast5_path, max_length=5000, read_index=0):
    """
    Extract raw ADC signal from a FAST5 file, truncated to max_length samples.
    
    Args:
        fast5_path: Path to FAST5 file
        max_length: Maximum number of samples to extract
        read_index: Index of read to extract from multi-FAST5 files
    
    Returns:
        numpy array of raw signal values (up to max_length), or None if error
    """
    if USE_ONT_API:
        return extract_signal_from_fast5_ont_api(fast5_path, max_length, read_index)
    else:
        return extract_signal_from_fast5_h5py(fast5_path, max_length, read_index)


def generate_read_file(output_path, signal):
    """
    Generate a read file with raw ADC values (one value per line).
    
    Args:
        output_path: Path to output file
        signal: Numpy array of raw signal values (no normalization)
    """
    # Write raw ADC values one per line
    with open(output_path, 'w') as f:
        for value in signal:
            f.write(f"{value}\n")
    
    print(f"Generated {output_path}: {len(signal)} raw ADC values")


def main():
    import argparse
    
    # Parse command-line arguments
    parser = argparse.ArgumentParser(
        description='Extract raw ADC values from FAST5 files (no normalization)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 preprocess_reads.py lambda
  python3 preprocess_reads.py human --num-files 50
  python3 preprocess_reads.py covid
        """
    )
    parser.add_argument('genome', type=str,
                        help='Genome name (e.g., lambda, human, covid)')
    parser.add_argument('--num-files', type=int, default=100,
                        help='Number of read files to generate (default: 100)')
    
    args = parser.parse_args()
    
    # Configuration
    genome = args.genome
    fast5_dir = f"data/{genome}/fast5"
    output_dir = f"datasets/{genome}"
    num_files = args.num_files
    
    # Validate fast5 directory exists
    if not os.path.isdir(fast5_dir):
        print(f"Error: FAST5 directory not found: {fast5_dir}", file=sys.stderr)
        print(f"Please ensure the directory exists with FAST5 files", file=sys.stderr)
        sys.exit(1)
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    # Find all FAST5 files
    fast5_files = sorted(glob(os.path.join(fast5_dir, "*.fast5")))
    
    if len(fast5_files) == 0:
        print(f"Error: No FAST5 files found in {fast5_dir}", file=sys.stderr)
        sys.exit(1)
    
    print(f"Found {len(fast5_files)} FAST5 files in {fast5_dir}")
    print(f"Generating {num_files} read files with raw ADC values (entire reads)...")
    print()
    
    # Generate read files - extract multiple reads from multi-FAST5 files if needed
    # Only keep reads with at least 5000 samples
    MIN_READ_LENGTH = 5000
    generated_count = 0
    file_index = 0
    read_index = 0
    skipped_count = 0
    
    while generated_count < num_files and file_index < len(fast5_files):
        fast5_path = fast5_files[file_index]
        
        # Extract raw signal from specific read index
        signal = extract_signal_from_fast5(fast5_path, read_index=read_index)
        
        if signal is not None:
            # Check if read is long enough
            if len(signal) >= MIN_READ_LENGTH:
                # Generate output file with genome-specific naming
                output_path = os.path.join(output_dir, f"{genome}_raw{generated_count}.txt")
                generate_read_file(output_path, signal)
                generated_count += 1
            else:
                skipped_count += 1
            
            # Try next read from same file
            read_index += 1
        else:
            # Move to next file
            file_index += 1
            read_index = 0
    
    if generated_count < num_files:
        print(f"\nWarning: Only generated {generated_count}/{num_files} files", file=sys.stderr)
        print(f"Not enough valid reads found in FAST5 files (skipped {skipped_count} reads < 5000 samples)", file=sys.stderr)
        sys.exit(1)
    
    print(f"\n✓ Successfully generated {num_files} read files in {output_dir}/")
    print(f"  Each file contains the entire raw ADC signal from one read (≥5000 samples)")


if __name__ == "__main__":
    main()
