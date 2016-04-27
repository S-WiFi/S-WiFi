#!/bin/sh

# Run swifi.tcl against different distances (d), and get the list of
# (timely-)throughput values.

# Usage: ./gen_throughput_d.sh policy symmetry
# where policy can be one of baseline (0), smart (7), or any number in between,
# and symmetry can be one of sym or asym.

policy=${1:-smart}
dist=${2:-1000}
symmetry=${3:-sym}

for nn in `seq 3 1 11`; do
	ns swifi.tcl pcf $policy $dist $symmetry $nn
	cat swifi_pcf_${symmetry}_${policy}.dat >> throughput_nn_${symmetry}_${policy}.dat
done
