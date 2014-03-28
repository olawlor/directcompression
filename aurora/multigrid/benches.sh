#!/bin/sh
cp benches_single.txt benches.txt
for thresh in 0.0 0.02 0.05 0.1 0.2 0.5
do
	./main -bench -threshold $thresh  || exit 1
	f="bench_thresh_$thresh.txt"
	mv bench.txt "$f"
	echo -n "$thresh " >> benches.txt
	awk '{printf("%f ",$3);}' < "$f" >> benches.txt
	echo >> benches.txt
done

