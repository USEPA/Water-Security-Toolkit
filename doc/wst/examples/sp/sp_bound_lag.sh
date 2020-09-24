#!/bin/sh

tname=sp_bound_lag

#cp ../../../../examples/$tname.yml .
mkdir Net3
cp ../../../../examples/Net3/* Net3

wst -t sp $tname.yml

cp $tname/Net3sp_output.yml $tname\_output.yml
cp $tname/Net3_evalsensor.out $tname\_evalsensor.out

version=`grep 'version' sp_output_bound_lag.yml`
version=${version:11:3}  
sed -i "s#directory: \(.*\)#directory: C:/wst-"$version"/examples/"$tname"#g" $tname\_output.yml

rm -r Net3
#rm -r $tname
