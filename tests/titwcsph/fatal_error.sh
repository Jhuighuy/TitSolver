#!/bin/sh

set -eu

mpi_launcher=$1
num_processes_flag=$2
solver=$3

mkdir occupied.tit-run
touch occupied.tit-run/existing-file

if "$mpi_launcher" "$num_processes_flag" 2 "$solver" \
  --max-steps 1 \
  --particles-per-height 8 \
  --output occupied.tit-run; then
  echo "Solver unexpectedly accepted a non-empty output run." >&2
  exit 1
fi
