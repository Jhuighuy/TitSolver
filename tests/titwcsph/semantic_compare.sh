#!/bin/sh

set -eu

mpi_launcher=$1
num_processes_flag=$2
solver=$3
comparator=$4

"$mpi_launcher" "$num_processes_flag" 1 "$solver" \
  --max-steps 2 \
  --snapshot-every 1 \
  --particles-per-height 8 \
  --output one-rank.tit-run

"$mpi_launcher" "$num_processes_flag" 2 "$solver" \
  --max-steps 2 \
  --snapshot-every 1 \
  --particles-per-height 8 \
  --output two-rank.tit-run

"$mpi_launcher" "$num_processes_flag" 4 "$solver" \
  --max-steps 2 \
  --snapshot-every 1 \
  --particles-per-height 8 \
  --output four-rank.tit-run

"$comparator" \
  one-rank.tit-run \
  two-rank.tit-run \
  four-rank.tit-run
