#!/bin/sh

bin=`pwd`/../../../bin
pythonpath=`pwd`/../../../python/bin
export PATH=$bin:$pythonpath:$PATH

# @cmd:
evalsensor --help
# @:cmd
