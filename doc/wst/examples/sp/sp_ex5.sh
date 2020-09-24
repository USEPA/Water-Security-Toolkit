#!/bin/sh

tname=sp_ex5

cwd=$(pwd)

cp ../../../../examples/$tname.yml .
mkdir Net3
cp ../../../../examples/Net3/* Net3

wst -t sp $tname.yml

cp $tname/Net3_evalsensor.out sp_ex5_evalsensor.out

version=`grep 'version' $tname/Net3sp_output.yml`
version=${version:11:3}  
sed "s#directory: \(.*\)#directory: C:/WST-"$version"/examples/"$tname"#g" < $tname/Net3sp_output.yml > sp_ex5_output.yml

cat $tname/Net3_evalsensor.out
rm -r Net3
rm -r $tname
