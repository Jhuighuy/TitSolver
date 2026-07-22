#!/bin/sh

set -eu

mpi_launcher=$1
num_processes_flag=$2
solver=$3

"$mpi_launcher" "$num_processes_flag" 1 "$solver" \
  --max-steps 1 \
  --particles-per-height 8 \
  --output initial.tit-run

"$mpi_launcher" "$num_processes_flag" 2 "$solver" \
  --restart initial.tit-run \
  --max-steps 1 \
  --particles-per-height 8 \
  --output restarted.tit-run
