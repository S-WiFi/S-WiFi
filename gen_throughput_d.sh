#!/bin/sh

# Run swifi.tcl against different distances (d), and get the list of
# (timely-)throughput values.

# Usage: ./gen_throughput_d.sh policy symmetry
# where policy can be one of baseline (0), smart (7), or any number in between,
# and symmetry can be one of sym or asym.

policy=${1:-smart}
symmetry=${2:-sym}

for d in 1 `seq 200 200 2000`; do
	ns swifi.tcl pcf $policy $d $symmetry
	cat swifi_pcf_${symmetry}_${policy}.dat >> throughput_d_${symmetry}_${policy}.dat
done
