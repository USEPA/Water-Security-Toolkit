#!/bin/sh

tname=inversion_ex2

cp ../../../../examples/$tname.yml .
mkdir Net3
cp ../../../../examples/Net3/Net3.inp Net3
cp ../../../../examples/Net3/Net3_MEASURES.dat Net3

wst -t inversion $tname.yml

cp $tname/*inversion_output.yml $tname\_output.yml

version=`grep ' version:' $tname\_output.yml`
version=${version:11:3} 
sed -i "s#directory: \(.*\)#directory: C:/wst-"$version"/examples/"$tname"#g" $tname\_output.yml
sed -i "s#tsg file: \(.*\)#tsg file: Net3profile.tsg#g" $tname\_output.yml
sed -i "s#likely nodes file: \(.*\)#likely nodes file: Net3Likely_Nodes.dat#g" $tname\_output.yml

rm -r Net3
rm -r $tname
rm $tname.yml