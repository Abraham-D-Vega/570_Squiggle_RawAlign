#!/bin/bash
set -euo pipefail

# Build and run the standalone event detector on a 10-bit-per-line input file.
# Defaults:
#   input:  data/testbench/human_raw0.txt
#   output: data/testbench/human_raw0.events.txt
# Usage:
#   tools/segment_read.sh                    # uses defaults above
#   tools/segment_read.sh <in>               # write output next to <in> as <in>.events.txt
#   tools/segment_read.sh <in> <out>         # explicit output path
#   CXXFLAGS="-O3 -march=native" tools/segment_read.sh <in> <out>

# Resolve repo root (script dir is tools/)

CXX=${CXX:-clang++}
CXXFLAGS=${CXXFLAGS:-"-O2 -std=c++17"}
SRC="SquiggleSeeder/src/oracle_segmenter.cpp"
BIN="oracle_segmenter"

# Defaults
DEFAULT_IN="data/testbench/human_raw0.txt"
DEFAULT_OUT="SquiggleSeeder/events/oracle/human_raw0.events.txt"
IN_PATH="${DEFAULT_IN}"
OUT_PATH=""

# Parse args
if [[ $# -ge 1 ]]; then
  IN_PATH=$1
fi
if [[ $# -ge 2 ]]; then
  OUT_PATH=$2
fi

# Derive default output if not provided
if [[ -z "${OUT_PATH}" ]]; then
  if [[ "${IN_PATH}" == "-" ]]; then
    OUT_PATH="-"  # stdout
  else
    # If using the default input, use the requested fixed output path; otherwise place next to input
    if [[ "${IN_PATH}" == "${DEFAULT_IN}" ]]; then
      OUT_PATH="${DEFAULT_OUT}"
    else
      OUT_PATH="${IN_PATH}.events.txt"
    fi
  fi
fi

# Ensure input exists unless stdin
if [[ "${IN_PATH}" != "-" && ! -f "${IN_PATH}" ]]; then
  echo "[ERROR] Input file not found: ${IN_PATH}" >&2
  exit 1
fi

# Compile
echo "[BUILD] ${CXX} ${CXXFLAGS} ${SRC} -o ${BIN}"
${CXX} ${CXXFLAGS} "${SRC}" -o "${BIN}"

# Run (write to file or stdout)
echo "[RUN ] ${BIN} ${IN_PATH} ${OUT_PATH} --win1 8 --win2 32 --th1 5.0 --th2 3.0 --peak 1.0"
if [[ "${OUT_PATH}" != "-" ]]; then
  mkdir -p "$(dirname "${OUT_PATH}")"
fi
"./${BIN}" "${IN_PATH}" "${OUT_PATH}" --win1 8 --win2 32 --th1 5.0 --th2 3.0 --peak 1.0

# If writing to a file, summarize
if [[ "${OUT_PATH}" != "-" ]]; then
  if [[ -f "${OUT_PATH}" ]]; then
    echo "[OK  ] Wrote $(wc -l < "${OUT_PATH}") events to ${OUT_PATH}"
    echo "[HEAD] First 5 lines:"
    head -n 5 "${OUT_PATH}" || true
  fi
fi
