#!/bin/sh

tname=template

cp ../../../../examples/Net3/Net3.inp .

wst -t tevasim --template $tname.yml
wst -t tevasim $tname.yml > $tname\_screen.txt

version=`grep 'version' *tevasim_output.yml`
version=${version:11:3}  
sed -i "s#Directory: \(.*\)#Directory: C:/WST-"$version"/examples/#g" $tname\_screen.txt


rm $tname.yml
rm *tevasim_output.yml
rm *tevasim_output.log
rm Net3.inp
rm *.erd
rm *.rpt
