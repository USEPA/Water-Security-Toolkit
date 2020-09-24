#!/bin/sh

tname=neutral
cwd=`pwd -P`

if [ -d $tname ]; then
   rm -Rf $tname
fi
mkdir $tname
cd $tname
cp ../$tname.yml .
cp ../../../../../examples/simple/Net3.inp .

wst -t booster_mip $tname.yml

cp $tname*.json ..

cd ..
\rm -Rf $tname
