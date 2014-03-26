#!/bin/sh
#
# Run -bench threshold sweeps for each test image,
#   and chart image error versus samples per pixel.
#
make

rm bench.txt
# for image in `echo ../real/* ../synthetic/*`
for image in `echo ../irtc/*.jpg`
do
	echo "---- $image ----"
	imshort=`basename $image`
	./main -bench -img "$image"
	mv bench.txt bench_multi/"$imshort"
done

./bench_multi_plot.sh
xv bench_multi_plot.jpg &

