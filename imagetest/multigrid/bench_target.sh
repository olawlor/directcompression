#!/bin/bash
#
# Run -target for each image, and analyze the results into an overall image error rate.
#
name=$1

rm main.o
make || exit 1

target=0.33

rm bench_target.txt bench_target_run.txt bench_target_log.txt
# for image in `echo ../real/* ../synthetic/*`
for image in `echo ../irtc/*.jpg`
do
	
	echo "---- $image ----"
	imshort=`basename $image`
	./main -target $target -img "$image" -metric `cat metric_value` | tee bench_target.txt || exit 1
	cat bench_target.txt >> bench_target_run.txt
	grep "^Target" bench_target.txt | sed -e 's/Target/'$imshort'/' >> bench_target_log.txt
	convert -flip image00000.ppm bench_target_img/$imshort
done

result=`awk '{
	avg_tar+=$3;
	avg_err+=$4;
	avg_n++;
} END {
	printf("Err: %.2f%% for target %f (%d images)",
		avg_err/avg_n*100.0,
		avg_tar/avg_n,
		avg_n);
}' < bench_target_log.txt`


# Save the run 
d="runs/$name"
rm -fr "$d"
mkdir "$d"
cp -r main.cpp interpolate*.txt main bench_target* "$d"
echo "$result	$name" >> runs/results.txt
echo "$result	$name"

