#!/bin/sh

bin=`pwd`/../../../bin
pythonpath=`pwd`/../../../python/bin
export PATH=$bin:$pythonpath:$PATH

mkdir simple4
cd simple4
cp ../Net3* .

if [ ! -e Net3.erd ]; then
# @tevasim:
tevasim --tsg Net3.tsg --tso Net3.inp Net3.out Net3 > tevasim.out
# @:tevasim
fi
cp data/Net3.erd .
cp data/Net3.index.erd data/Net3-1.hyd.erd data/Net3-1.qual.erd .

cp Net3.erd Net3_1a.erd
cp Net3.erd Net3_1b.erd
cp Net3.index.erd Net3_1a.index.erd
cp Net3.index.erd Net3_1b.index.erd
cp Net3-1.hyd.erd Net3-1_1a.hyd.erd
cp Net3-1.hyd.erd Net3-1_1b.hyd.erd
cp Net3-1.qual.erd Net3-1_1a.qual.erd
cp Net3-1.qual.erd Net3-1_1b.qual.erd 
# @tso2Impact:
tso2Impact --detectionLimit 30000000 --detectionLimit 0.0001 --mc Net3 Net3_1a.erd       Net3_1b.erd
# @:tso2Impact

cd ..
\rm -Rf simple4
