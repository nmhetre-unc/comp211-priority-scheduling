#!/bin/bash
# Run two identical CPU-bound programs on the SAME core, one at nice 0 and one at
# nice N, then compare how much work each got done.
#
# Usage: ./run_nice.sh [nice_value] [seconds]
#   ./run_nice.sh 19 10    (default)
#   ./run_nice.sh 5 10

NICE=${1:-19}
SECS=${2:-10}

gcc -O2 -Wall -o busy busy.c || exit 1

echo "Running nice 0 vs nice $NICE, both pinned to CPU 0, for $SECS seconds..."
taskset -c 0 ./busy "$SECS" > out_nice0.txt &
taskset -c 0 nice -n "$NICE" ./busy "$SECS" > out_niceN.txt &
wait

A=$(awk '{print $3}' out_nice0.txt)
B=$(awk '{print $3}' out_niceN.txt)
TOTAL=$((A + B))

echo "nice 0:     $A million iterations ($(awk "BEGIN {printf \"%.1f\", 100*$A/$TOTAL}")%)"
echo "nice $NICE: $B million iterations ($(awk "BEGIN {printf \"%.1f\", 100*$B/$TOTAL}")%)"
rm -f out_nice0.txt out_niceN.txt
