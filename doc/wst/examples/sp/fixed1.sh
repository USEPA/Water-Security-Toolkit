#!/bin/sh

tname=fixed1

#cp ../../../../examples/$tname.yml .
mkdir Net3
cp ../../../../examples/Net3/* Net3

wst -t sp $tname.yml

cp $tname/Net3_sp.json $tname.json

rm -r Net3
rm -r $tname
