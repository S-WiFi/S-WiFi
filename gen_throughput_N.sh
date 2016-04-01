#!/bin/sh

# Run swifi.tcl against different client number (N), and get the list of
# throughput values.

for N in `seq 1 5`; do
	ns swifi.tcl pcf baseline 10 $N
	octave mean_throughput.m $N swifi_pcf_baseline.dat
done
