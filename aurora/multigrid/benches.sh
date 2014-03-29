#!/bin/sh
f="../simple/bench.txt"
o="benches.txt"
	echo -n "Time " > $o
	awk '{printf("%f ",$1);}' < "$f" >> $o
	echo >> $o
	echo -n "Naive " >> $o
	awk '{printf("%f ",$3);}' < "$f" >> $o
	echo >> $o

for thresh in 0.0 0.03 0.05 0.1 0.2 0.5 1.0
do
	./main -bench -threshold $thresh  || exit 1
	f="bench_thresh_$thresh.txt"
	mv bench.txt "$f"
	
	echo -n "$thresh " >> $o
	awk '{printf("%f ",$3);}' < "$f" >> $o
	echo >> $o
done

