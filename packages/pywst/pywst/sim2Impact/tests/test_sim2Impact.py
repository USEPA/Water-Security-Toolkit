#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

#
# Get the directory where this script is defined, and where the baseline
# files are located.
#
"""
import os
import sys
from os.path import abspath, dirname, join

import copy
import shutil
import unittest

from nose.tools import nottest

import pyutilib.th
import pyutilib
import pywst.sim2Impact.problem as problem

testDir = dirname(abspath(__file__))
wstRoot = join(testDir, *('..',)*5)
dataDir = join(wstRoot,'..','wst_data','sim2Impact')


wst = join(wstRoot, 'python', 'bin', 'wst')
"""

import os
import sys
from os.path import abspath, dirname, join, basename

import copy
import shutil
import unittest

from nose.tools import nottest
import pyutilib.th as unittest
import pyutilib.services

import pywst.sim2Impact.problem as problem
from pywst.common import pyunit

testDir = dirname(abspath(__file__))
if pyunit.dataroot is not None:
    dataDir = join(pyunit.dataroot, 'sim2Impact')
else:
    dataDir = join(dirname(abspath(__file__)), '..')

pyutilib.services.register_executable('wst')

sim2Impact = pyutilib.services.registered_executable('sim2Impact')
if not sim2Impact is None:
    sim2Impact = sim2Impact.get_path()
wst = pyutilib.services.registered_executable('wst')
if wst is not None:
    wst = wst.get_path()
    

#
# Create unittest testing class
#
class Test(pyutilib.th.TestCase):

    # This method is called once, before all tests are executed.
    @classmethod
    def setUpClass(self):
        pass

    # This method is called once, after all tests are executed.
    @classmethod
    def tearDownClass(self):
        pass

    # This method is called before each test is executed.
    def setUp(self):
        self.origDir = os.getcwd()
        os.chdir(testDir)
        self.problem = problem.Problem()
        
    # This method is called after each test is executed.
    def tearDown(self):
        os.chdir(self.origDir)
    
    @nottest
    def execute_test(self, name=None, metric=None, solution=None):
        os.chdir(dataDir)
        ans = pyutilib.subprocess.run([wst, 'tevasim', name+'.yml'])
        os.chdir(testDir)
        self.problem.opts['impact']['erd file'] = join(dataDir,name+'.erd')
        self.problem.opts['impact']['metric'] = metric
        self.problem.opts['configure']['output prefix'] = name
        #self.problem.saveAll(name+'.yml')
        fid = open(name+'.yml', 'w')
        fid.write(self.problem.opts.generate_yaml_template())
        fid.close()
        ans = pyutilib.subprocess.run([wst, 'sim2Impact', name+'.yml'])
        self.assertEqual(ans[0], 0)
        
        fid = open(name+'_'+metric.lower()+'.impact','r')
        impact = fid.readlines()
        MC = float(impact[len(impact)-1].split(' ')[3].rstrip('\n'))
        self.assertLessEqual(MC/solution, 1) 
        self.assertGreaterEqual(MC/solution, 0.9999) 
        
    # Test integration 
    def test_MassConsumed(self):
        # tevasim
        self.execute_test(name='2Node_tevasim', metric='MC', solution=240000)
        self.execute_test(name='2Node_tevasim_UP', metric='MC', solution=240000)
        self.execute_test(name='2Node_merlion', metric='MC', solution=240000)
        self.execute_test(name='2Node_merlion_UP', metric='MC', solution=240000)


if __name__ == "__main__":
    unittest.main()
