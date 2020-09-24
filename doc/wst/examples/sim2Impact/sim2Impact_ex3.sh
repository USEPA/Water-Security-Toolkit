#!/bin/sh

tname=sim2Impact_ex3

cwd=$(pwd)

cp ../../../../examples/$tname.yml .
mkdir Net3
cp ../../../../examples/Net3/Net3*.erd Net3

wst -t sim2Impact $tname.yml

cp $tname/*sim2Impact_output.yml $tname\_output.yml

version=`grep 'version' $tname\_output.yml`
version=${version:11:3}  
sed -i "s#directory: \(.*\)#directory: C:/WST-"$version"/examples/"$tname"#g" $tname\_output.yml

rm -r Net3
rm -r $tname
rm $tname.yml