#! /usr/bin/env python

import os 
from os.path import normpath, join, dirname, abspath

import sys
import glob
import pyutilib.subprocess

def runWSTTests():
    baseDir = normpath(join(dirname(abspath(__file__)), '..', '..', '..'))
    localBin = normpath(join(dirname(abspath(__file__)), '..', '..', '..', 'python', 'bin'))

    if os.environ.get('PATH',None) is None:
        os.environ['PATH'] = localBin
    else:
        os.environ['PATH'] = os.pathsep.join([localBin,os.environ['PATH']])

    print ""
    print "*****************************"
    print " Executing test.wst"
    print "*****************************"

    # execute the packages/test/pyunit/runtests script
    os.chdir( join(baseDir,'packages','test','pyunit') )
    if os.path.isfile('runtests'):
        print "\n\n*****************************"
        print "* Running 'runtests' in "+os.getcwd()
        print "*****************************\n"
        sys.stdout.flush()
        pyutilib.subprocess.run([sys.executable, 'runtests'], tee=True)
        sys.stdout.flush()
    else:
        print "\n\n*****************************"
        print "* WARNING: 'runtests' not found in "+os.getcwd()
        print "*****************************\n"


    # execute the cxxtest runner executables
    runner_locs = [
        join(baseDir,'packages','test','unit'),
        join(baseDir,'packages','sim','merlion','test','unit'),
        join(baseDir,'packages','sim','merlion','merlionUtils','test','unit'),
        ]

    for loc in runner_locs:
        os.chdir( loc )
        if os.path.isfile('runner'):
            print "\n\n*****************************"
            print "* Running 'runner' in "+os.getcwd()
            print "*****************************\n"
            sys.stdout.flush()
            pyutilib.subprocess.run(['./runner','-v'], tee=True)
            sys.stdout.flush()
        else:
            print "\n\n*****************************"
            print "* WARNING: 'runner' not found in "+os.getcwd()
            print "*****************************\n"

if __name__ == '__main__':
    runWSTTests()
