#!/bin/sh

# Run swifi.tcl against different distances (d), and get the list of
# (timely-)throughput values.

policy=${1:-smart}
symmetry=${2:-sym}

for d in 1 `seq 200 200 2000`; do
	ns swifi.tcl pcf $policy $d $symmetry
	octave mean_throughput.m "swifi_pcf_${symmetry}_${policy}.dat" "d" "$d" "$policy" "$symmetry"
done
