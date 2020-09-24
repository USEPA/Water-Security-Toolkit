#!/bin/sh

bin=`pwd`/../../../bin
pythonpath=`pwd`/../../../python/bin
export PATH=$bin:$pythonpath:$PATH

mkdir simple7
cd simple7
cp ../Net3* .

if [ ! -e Net3_bio.erd ]; then
# @tevasim:
tevasim --tsg Net3_bio.tsg --msx bio.msx --mss BIO Net3.inp Net3.out Net3_bio > tevasim.out
# @:tevasim
fi
cp data/Net3_bio.erd .
cp data/Net3_bio.index.erd data/Net3_bio-1.hyd.erd data/Net3_bio-1.qual.erd .

# @tso2Impact:
tso2Impact --mc --vc --td --nfd --ec --species BIO Net3_bio Net3_bio.erd
# @:tso2Impact

cd ..
\rm -Rf simple7
