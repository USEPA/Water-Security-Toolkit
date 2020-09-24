#!/bin/sh

bin=`pwd`/../../../bin
mod=`pwd`/../../../etc/mod
pico=`pwd`/../../../../acro-pico/bin
pythonpath=`pwd`/../../../python/bin
export PATH=$bin:$pythonpath:$pico:$PATH

mkdir solver16
cd solver16
cp ../Net3* .

cp data/Net3.erd .
cp data/Net3*.erd .
if [ ! -e Net3_dec.impact ]; then
tso2Impact --mc --vc --td --nfd --ec --dec Net3 Net3.erd > tso.out
fi

# @sp:
sp-2tier --path=$bin --network=Net3 --objective=mc --skeleton=8 --stat=min\
         --ub1=ns,5 --ub2=ns,5 --solver1=snl_grasp --solver2=snl_grasp
# @:sp

cd ..
\rm -Rf solver16
