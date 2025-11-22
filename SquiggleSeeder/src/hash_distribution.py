#!/usr/bin/env python3
import sys
import os
import matplotlib.pyplot as plt
import numpy as np

if len(sys.argv) != 2:
    print("Usage: python3 hash_distribution.py <genome>")
    sys.exit(1)

genome = sys.argv[1]
hash_table_path = os.path.join("datasets", genome, "hash_table0.txt")
if not os.path.exists(hash_table_path):
    print(f"Hash table not found: {hash_table_path}")
    sys.exit(1)

hash_freq = {}
with open(hash_table_path, 'r') as f:
    header = f.readline()  # skip header
    for line in f:
        line = line.strip()
        if not line: continue
        parts = line.split(',')
        if not parts[0].isdigit():
            continue  # skip non-integer hash values and headers
        hash_val = int(parts[0])
        freq = len(parts) - 1  # number of locations
        hash_freq[hash_val] = freq

hashes = list(hash_freq.keys())
frequencies = list(hash_freq.values())

# Find cutoff for high frequency outliers using Tukey's method (1.5*IQR)
q1 = np.percentile(frequencies, 25)
q3 = np.percentile(frequencies, 75)
iqr = q3 - q1
cutoff = q3 + 1.5 * iqr
num_pruned = sum(f > cutoff for f in frequencies)
print(f"Frequency cutoff for outliers: {cutoff:.2f}")
print(f"Number of hashes that would be pruned: {num_pruned} out of {len(frequencies)}")

plt.figure(figsize=(12,6))
plt.scatter(hashes, frequencies, s=2)
plt.axhline(cutoff, color='red', linestyle='--', label=f'Outlier cutoff ({cutoff:.2f})')
plt.xlabel('Hash')
plt.ylabel('Frequency (number of locations)')
plt.title(f'Hash Frequency Distribution for {genome}')
plt.legend()
plt.tight_layout()
plt.show()
