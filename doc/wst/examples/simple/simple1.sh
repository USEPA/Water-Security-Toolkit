#!/bin/sh

bin=`pwd`/../../../bin
pythonpath=`pwd`/../../../python/bin
export PATH=$bin:$pythonpath:$PATH

mkdir simple1
cd simple1
cp ../Net3* .

if [ ! -e Net3.erd ]; then
# @tevasim:
tevasim --tsg Net3.tsg Net3.inp Net3.out Net3 > tevasim.out
# @:tevasim
fi
cp data/Net3.erd .
cp data/Net3.index.erd data/Net3-1.hyd.erd data/Net3-1.qual.erd .

if [ ! -e Net3_mc.impact ]; then
# @tso2Impact:
tso2Impact --mc --vc --td --nfd --ec Net3 Net3.erd
# @:tso2Impact
fi

# @sp:
sp --path=$bin --print-log --network="Net3" --objective=mc --solver=snl_grasp \
            --ub=ns,5 --seed=1234567
# @:sp

# @evalsensor:
evalsensor --nodemap=Net3.nodemap Net3.sensors Net3_ec.impact Net3_mc.impact \
            Net3_nfd.impact
# @:evalsensor

cd ..
\rm -Rf simple1
