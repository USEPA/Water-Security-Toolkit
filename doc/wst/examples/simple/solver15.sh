#!/bin/sh

bin=`pwd`/../../../bin
mod=`pwd`/../../../etc/mod
pico=`pwd`/../../../../acro-pico/bin
pythonpath=`pwd`/../../../python/bin
export PATH=$bin:$pythonpath:$pico:$PATH

mkdir solver15
cd solver15
cp ../Net3* .

cp data/Net3.erd .
cp data/Net3*.erd .
if [ ! -e Net3_dec.impact ]; then
tso2Impact --mc --vc --td --nfd --ec --dec Net3 Net3.erd > tso.out
fi

# @sp:
sp --path=$bin --network=Net3 --objective=ec --ub=ns,5 --imperfect-scfile=Net3.imperfectsc \
    --imperfect-jcfile=Net3.imperfectjc --solver=snl_grasp
# @:sp

cd ..
\rm -Rf solver15
