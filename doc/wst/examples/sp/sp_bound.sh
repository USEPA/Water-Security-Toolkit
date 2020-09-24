#!/bin/sh

tname=sp_bound

#cp ../../../../examples/$tname.yml .
mkdir Net3
cp ../../../../examples/Net3/* Net3

wst -t sp $tname.yml

cp $tname/Net3sp_output.yml sp_bound_output.yml
cp $tname/Net3_evalsensor.out sp_bound_evalsensor.out

version=`grep 'version' sp_bound_output.yml`
version=${version:11:3}  
sed -i "s#directory: \(.*\)#directory: C:/wst-"$version"/examples/"$tname"#g" $tname\_output.yml

rm -r Net3
rm -r $tname
