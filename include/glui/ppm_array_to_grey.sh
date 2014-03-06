#!/bin/sh
for f in `echo *.c`
do
	echo "--------- fixing $f ------------"
	cat $f \
	| sed -e "s/int /unsigned char /" \
	| awk -F, '{
		if (substr($1,0,3)=="   ") { 
			for (i=1;i<NF;i+=3) printf("%s,",$i); 
			printf("\n"); 
		} else print $0
	}' \
	> $f.new || exit 1
	mv $f $f.bak_grey || exit 1
	mv $f.new $f || exit 1
done
