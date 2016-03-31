#!/bin/sh

# Run swifi.tcl against different interval length (T), and get the list of
# throughput values.

for T in `seq 4 2 16`; do
	ns swifi.tcl pcf baseline $T
	octave mean_throughput.m $T swifi_pcf_baseline.dat
done
