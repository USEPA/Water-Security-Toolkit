#!/bin/bash
##
## Test1 test script
##

function do_tests
{
# pretty fast "smoke" tests
echo "========= test1a ============="
./test1a
echo "========= test1f ============="
./test1f
echo "========= test1g ============="
./test1g
echo "========= test1aa ============="
./test1aa
echo "========= test1m ============="
./test1m
echo "========= test1n ============="
./test1n
echo "========= test1o ============="
./test1o
echo "========= test1r ============="
./test1r
echo "========= test1s ============="
./test1s
echo "========= test1a_m ============="
./test1a_m
echo "========= test1p ============="
./test1p
#
# slower tests - uncomment if you are patient
#echo "========= test1h ============="
#./test1h
#echo "========= test1k ============="
#./test1k
#echo "========= test1l ============="
#./test1l
#echo "========= test1t ============="
#./test1t
#echo "========= test1q ============="
#./test1q
#echo "========= test1i ============="
#./test1i
#echo "========= test1j ============="
#./test1j
#
# Uncomment these if you have AMPL.
#
#./test1a
#./test1ab
}

uname -a
time do_tests
