#!/bin/sh

bin=`pwd`/../../../bin
mod=`pwd`/../../../etc/mod
pico=`pwd`/../../../../acro-pico/bin
pythonpath=`pwd`/../../../python/bin
export PATH=$bin:$pythonpath:$pico:$PATH

mkdir solver11
cd solver11
cp ../Net3* .

cp data/Net3.erd .
cp data/Net3*.erd .
if [ ! -e Net3_dec.impact ]; then
tso2Impact --mc --vc --td --nfd --ec --dec Net3 Net3.erd > tso.out
fi

# @sp:
sp --path=$bin --path=$pico --path=$mod --network=Net3 --objective=ec --ub=ns,5 --solver=pico 2>&1 > solver11.sp
# @:sp

mv Net3.sensors Net3_orig.sensors

# @evalsensor:
evalsensor --nodemap=Net3.nodemap Net3_orig.sensors Net3_ec.impact Net3_dec.impact \
    Net3_nfd.impact
# @:evalsensor

cd ..
\rm -Rf solver11
