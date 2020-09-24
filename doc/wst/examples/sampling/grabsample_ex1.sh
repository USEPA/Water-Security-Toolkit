#!/bin/sh

tname=grabsample_ex1

cp ../../../../examples/$tname.yml .
mkdir Net3
cp ../../../../examples/Net3/Net3.inp Net3
cp ../../../../examples/Net3/Net3_gs_profile.tsg Net3
cp ../../../../examples/Net3/Net3_fixed_sensors Net3

wst -t grabsample $tname.yml

cp $tname/*grabsample_output.yml $tname\_output.yml

version=`grep 'version' $tname\_output.yml`
version=${version:11:3}  
sed -i "s#directory: \(.*\)#directory: C:/wst-"$version"/examples/"$tname"#g" $tname\_output.yml

rm -r Net3
rm -r $tname
rm $tname.yml
rm *.rpt
