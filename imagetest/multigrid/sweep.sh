#!/bin/sh
#
# Vary the multigrid starting level count
#

for levels in 2 3 4 5 6
do
	echo $levels
	echo $levels > sweep_levels.h
	./bench_target.sh "levels$levels"
done

tail runs/results.txt

