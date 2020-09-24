#!/bin/sh

tname=sp3
bin=`pwd`/../../../../bin
mod=`pwd`/../../../../etc/mod
pico=`pwd`/../../../../../acro-pico/bin
pythonpath=`pwd`/../../../../python/bin
export PATH=$bin:$pythonpath:$pico:$PATH

cd ../Net3
make -q sp
cd ../sp

if [ -d $tname ]; then
   rm -Rf $tname
fi
mkdir $tname
cp $tname.yml $tname
cd $tname
cp ../../Net3/* .

# @sp:
wst -t sp $tname.yml
# @:sp

cd ..
\rm -Rf $tname
