#!/bin/sh

tname=sp_ex1

cwd=$(pwd)

cp ../../../../examples/$tname.yml .
mkdir Net3
cp ../../../../examples/Net3/* Net3

wst -t sp $tname.yml

cp $tname/Net3_evalsensor.out sp_ex1_evalsensor.out

version=`grep 'version' $tname/Net3sp_output.yml`
version=${version:11:3}  
sed "s#directory: \(.*\)#directory: C:/wst-"$version"/examples/"$tname"#g" < $tname/Net3sp_output.yml > sp_ex1_output.yml
sed -i "s#Impact File: \(.*\)#Impact File: Net3_ec.impact#g" sp_ex1_evalsensor.out
sed -i "s#Greedy ordering of sensors: \(.*\)#Greedy ordering of sensors: Net3_ec.impact#g" sp_ex1_evalsensor.out

cat $tname/Net3_evalsensor.out
rm -r Net3
rm -r $tname
