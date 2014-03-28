#!/bin/sh
#
# Vary the starting error metric
#

echo >> runs/results.txt
echo "Metric sweep, with levels="`cat sweep_levels.h` >> runs/results.txt
for m in \
	 0  1   2  3   4  5   6  7   8  9  10 11 \
	20 21  22 23  24 25  26 27  28 29  30 31 \
	       42 43  44 45  46 47  48 49  50 51 \
	              64 65  66 67  68 69  70 71
do
	echo $m
	echo $m > metric_value
	./bench_target.sh "metric$m"
done

tail -50 runs/results.txt

