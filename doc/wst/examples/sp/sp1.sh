#!/bin/sh

tname=sp1
cwd=`pwd -P`
bin=$cwd/../../../../bin
mod=$cwd/../../../../etc/mod
pico=$cwd/../../../../../acro-pico/bin
pythonpath=$cwd/../../../../python/bin
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

mv wst_sp_output.yml ../${tname}_output.yml
cd ..
\rm -Rf $tname
