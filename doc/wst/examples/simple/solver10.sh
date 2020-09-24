#!/bin/sh

bin=`pwd`/../../../bin
mod=`pwd`/../../../etc/mod
pico=`pwd`/../../../../acro-pico/bin
pythonpath=`pwd`/../../../python/bin
export PATH=$bin:$pythonpath:$pico:$PATH

mkdir solver10
cd solver10
cp ../Net3* .

if [ ! -e Net3.erd ]; then
# @prelim:
tevasim --tsg Net3.tsg Net3.inp Net3.out Net3 > tevasim.out
tso2Impact --mc --vc --td --nfd --ec Net3 Net3.erd > tso.out
# @:prelim
else
cp data/Net3.erd .
cp data/Net3*.erd .
if [ ! -e Net3_mc.impact ]; then
tso2Impact --mc --vc --td --nfd --ec Net3 Net3.erd > tso.out
fi
fi

# @sp:
sp --path=$bin --network=Net3 --objective=ec_tce --ub=mc_mean,30000 --ub=ns,5 \
    --solver=snl_grasp
# @:sp

cd ..
\rm -Rf solver10
