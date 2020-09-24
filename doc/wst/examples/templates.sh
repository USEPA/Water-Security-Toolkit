#!/bin/sh

bin=`pwd`/../../../bin
pythonpath=`pwd`/../../../python/bin
export PATH=$bin:$pythonpath:$PATH

# @cmd:
lbin wst sp --template sp_template.yml
# @:cmd

