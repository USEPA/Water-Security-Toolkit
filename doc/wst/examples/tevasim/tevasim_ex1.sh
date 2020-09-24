#!/bin/sh

tname=tevasim_ex1

cp ../../../../examples/$tname.yml .
mkdir Net3
cp ../../../../examples/Net3/Net3.inp Net3
cp ../../../../examples/Net3/Net3.tsg Net3

wst -t tevasim $tname.yml

cp $tname/*tevasim_output.yml $tname\_output.yml

version=`grep 'version' $tname\_output.yml`
version=${version:11:3}  
sed -i "s#directory: \(.*\)#directory: C:/WST-"$version"/examples/"$tname"#g" $tname\_output.yml

rm -r Net3
rm -r $tname
rm $tname.yml