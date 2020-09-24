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
import os
import sys
from os.path import abspath, dirname, join, basename

import json
import glob
import yaml
import copy
import shutil
import unittest
from nose.tools import nottest
import pyutilib.th as unittest
import pyutilib
import pywst.grabsample.problem as problem
from pywst.common import pyunit

testDir = dirname(abspath(__file__))
if not pyunit.dataroot is None:
    dataDir = join(pyunit.dataroot,'grabsample')
else:
    dataDir = join(testDir,'data') 
wstRoot = join(testDir, *('..',)*5)

pyomo = join(wstRoot,'python','bin','pyomo')
if not os.path.exists(pyomo):
    pyomo = None

ampl = pyutilib.services.registered_executable('ampl')
if ampl is None:
    ampl = pyutilib.services.registered_executable('ampl64')
if not ampl is None:
    ampl = ampl.get_path()

pyutilib.services.register_executable('cplexamp')
cplexamp = pyutilib.services.registered_executable('cplexamp')
if not cplexamp is None:
    cplexamp = cplexamp.get_path()

#
# Create unittest testing class
#
class Test(unittest.TestCase):

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
        self.problem.opts['configure']['output prefix'] = self.gen_test_dir()
        # This must be done after the output prefix has been set
        self.problem.setLogger('grabsample')

    # This method is called after each test is executed.
    def tearDown(self):
        self.problem.resetLogger('grabsample')
        os.chdir(self.origDir)
                
    # Test yaml options

    # Test functionality
    
    # Test integration

    # Test models
    @nottest
    def has_cplex_lp():
        from pyomo.environ import SolverFactory
        try:
            cplex = SolverFactory('cplex',keepFiles=True)
            available = (not cplex.executable() is None) and cplex.available(False)
            return available
        except pyutilib.common.ApplicationError:
            return False
    @nottest
    def diffSolutions(self,sol1,sol2,algorithm,tol=0.01):
        if algorithm == 'bayesian':
            self.assertEquals(len(sol1),len(sol2))
            for i in range(len(sol1)):
                self.assertAlmostEquals(sol1[i]["Objective"], sol2[i]["Objective"], delta=tol)
                self.assertEquals(len(sol1[i]["Nodes"]),len(sol2[i]["Nodes"]))
                for n in range(len(sol1[i]["Nodes"])):
                    self.assertEquals(sol1[i]["Nodes"][n]["Name"], sol2[i]["Nodes"][n]["Name"])
        elif algorithm == 'optimization':
            self.assertEquals(len(sol1),len(sol2))
            for i in range(len(sol1)):
                self.assertAlmostEquals(sol1[i]["Objective"], sol2[i]["Objective"], delta=tol)
                self.assertEquals(len(sol1[i]["Nodes"]),len(sol2[i]["Nodes"]))
                for n in range(len(sol1[i]["Nodes"])):
                    self.assertEquals(sol1[i]["Nodes"][n]["Name"], sol2[i]["Nodes"][n]["Name"])
                    self.assertEquals(len(sol1[i]["Nodes"][n]["Profile"]),len(sol2[i]["Nodes"][n]["Profile"]))
                    for p in range(len(sol1[i]["Nodes"][n]["Profile"])):
                        self.assertAlmostEquals(sol1[i]["Nodes"][n]["Profile"][p]["Start"],sol2[i]["Nodes"][n]["Profile"][p]["Start"], delta=tol)
                        self.assertAlmostEquals(sol1[i]["Nodes"][n]["Profile"][p]["Stop"],sol2[i]["Nodes"][n]["Profile"][p]["Stop"], delta=tol)
                        self.assertAlmostEquals(sol1[i]["Nodes"][n]["Profile"][p]["Strength"],sol2[i]["Nodes"][n]["Profile"][p]["Strength"], delta=1e3*tol)

    @nottest
    def setDefaultModelTestOptions(self):
        self.problem.opts['network']['epanet file'] = join(dataDir,'Net1.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'Net1profile.tsg')
        self.problem.opts['scenario']['merlion'] = True
        self.problem.opts['grabsample']['model format'] = 'PYOMO'
        self.problem.opts['grabsample']['sample time'] = 300
        self.problem.opts['grabsample']['threshold'] = None
        self.problem.opts['grabsample']['fixed sensors'] = None
        self.problem.opts['grabsample']['feasible nodes'] = None
        self.problem.opts['grabsample']['num samples'] = 1
        self.problem.opts['grabsample']['greedy selection'] = False

        if cplexamp is not None:
            self.problem.opts['solver']['type'] = basename(cplexamp)
        self.problem.opts['solver']['options'] = {'threads':1, 'predual':1, 'primalopt':None}
        self.problem.opts['solver']['problem writer'] = 'nl'
        if ampl is not None:
            self.problem.opts['configure']['ampl executable'] = basename(ampl)
        if pyomo is not None:
            self.problem.opts['configure']['pyomo executable'] = pyomo

    @nottest
    def cleanup(self, test_dir):
        assert os.path.abspath(test_dir) != os.path.abspath(os.getcwd())
        try:
            shutil.rmtree(test_dir)
        except:
            pass

    @nottest
    def gen_test_dir(self):
        class_name, test_name = self.id().split('.')[-2:]
        dirname = class_name+'-'+test_name+os.sep
        self.cleanup(dirname)
        os.mkdir(dirname)
        return dirname

    # Testing default options for base case 
    def test_solve_base(self):
        self.setDefaultModelTestOptions()
        Solution = self.problem.run()
        self.assertTrue(len(Solution) > 0)
        self.assertEquals(Solution[1][0], '10')
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing default options fro greedy algorithm
    def test_solve_base_greedy(self):
        self.setDefaultModelTestOptions()
        self.problem.opts['grabsample']['greedy selection'] = True
        Solution = self.problem.run()
        self.assertTrue(len(Solution) > 0)
        self.assertEquals(Solution[1][0], '10')
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing default options for base case with epanet water quality
    # simulations
    def test_solve_base_epanet(self):
        self.setDefaultModelTestOptions()
        self.problem.opts['scenario']['merlion'] = False
        Solution = self.problem.run()
        self.assertTrue(len(Solution) > 0)
        self.assertEquals(Solution[1][0], '10')
        self.cleanup(self.problem.opts['configure']['output prefix'])

if __name__ == "__main__":
    unittest.main()
