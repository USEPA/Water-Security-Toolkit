#!/bin/sh

tname=booster_mip_ex1

cp ../../../../examples/$tname.yml .
mkdir Net3
cp ../../../../examples/Net3/Net3.inp Net3

wst -t booster_mip $tname.yml

runs=(1 2 3 4 5)
for run in ${runs[@]}; do 
	cp $tname/*booster_mip_output_$run.yml $tname\_output_$run.yml

	version=`grep 'version' $tname\_output_$run.yml`
	version=${version:11:3}  
	sed -i "s#directory: \(.*\)#directory: C:/wst-"$version"/examples/"$tname"#g" $tname\_output_$run.yml
done

rm -r Net3
rm -r $tname
rm $tname.yml