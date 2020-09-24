# Imports
import pyutilib.th as unittest
import glob
import os
from os.path import dirname, abspath, abspath, basename
import sys

currdir = dirname(abspath(__file__))+os.sep
datadir = currdir
dirs = ('simple', 'sp')
dirs = [datadir+dir+os.sep for dir in dirs]

def filter(line):
    #print 'LINE', line
    #print 'Sensor placement id:' in line
    return 'seconds' in line or\
        'Sensor placement id:' in line or\
        'Time=' in line

# Declare an empty TestCase class
class Test(unittest.TestCase): pass

# Find all example*.py files, and use them to define baseline tests
for dir in dirs:
    for file in glob.glob(dir+'example*.py'):
        bname = basename(file)
        name=bname.split('.')[0]
        if dir == datadir:
            #tname=name
            pass
        else:
            name=basename(os.path.split(file)[0])+'_'+name
        Test.add_import_test(name=name, dir=dir, baseline=dir+name+'.txt')

# Find all *.sh files, and use them to define baseline tests
for dir in dirs:
    for file in glob.glob(dir+'*.sh'):
        bname = basename(file)
        name=bname.split('.')[0]
        if dir == datadir:
            tname=name
        else:
            tname = basename(os.path.split(file)[0]) + '_'+name
        Test.add_baseline_test(cmd='cd %s; %s/runsh %s' % (dir, currdir, file), baseline=dir+name+'.txt', name=tname, filter=filter)


# Execute the tests
if __name__ == '__main__':
    unittest.main()

