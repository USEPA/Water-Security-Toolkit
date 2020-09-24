#!/bin/sh

tname=fixed0
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

sed -e "s#$cwd/$tname/##g" wst_sp.json > ../$tname.json
#sed -e "s#$cwd/$tname/##g" solver.out > ../$tname-solver.out
cd ..
\rm -Rf $tname
