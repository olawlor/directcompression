#!/bin/sh

gp='
# set logscale y
set nologscale
#set key right top
set nokey
set xlabel "Samples Per Finished Pixel"
set ylabel "Average Absolute Color Error (%)"
set yrange [0.0:8.0]
set xrange [0:1]

set term postscript eps color 22
#set term postscript eps monochrome 22
set output "bench_multi_plot.eps"

#set term jpeg giant
#set output "bench_multi_plot.jpg"

plot '
sep=""
for f in `echo bench_multi/*`
do
	gp="$gp $sep  \"$f\" using 3:(100*\$4) title \""`basename $f`"\" with lines"
	sep=","
done
echo "$gp" | gnuplot


