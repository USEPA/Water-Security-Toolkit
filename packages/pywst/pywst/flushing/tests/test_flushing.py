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
from os.path import abspath, dirname, join
import json

import copy
import shutil
import unittest
from nose.tools import nottest
import pyutilib.th as unittest
import pyutilib

import pywst.flushing.problem as problem

testDir = dirname(abspath(__file__))
wstRoot = join(testDir, *('..',)*5)
dataDir = join(wstRoot,'examples','Net3')


wst = join(wstRoot, 'python', 'bin', 'wst')

dakota = pyutilib.services.registered_executable('dakota')
if not dakota is None:
    dakota = dakota.get_path()
coliny = pyutilib.services.registered_executable('coliny')
if not coliny is None:
    coliny = coliny.get_path()
    
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
        
        template_options = {
            'network': {'epanet file': 'Net3.inp'},
            'scenario': {'end time': 240,
                        'location': ['101'],
                        'start time': 180,
                        'strength': 1.450000e+010,
                        'type': 'MASS'},
            'impact': {'metric': ['PE'], 
                        'tai file': 'Net3_bio.tai'},
            'flushing': {'detection': [111, 127, 179],
                        'close valves': {'max pipes': 0, 
                                'response time': 0.0},
                        'flush nodes': {'duration': 480.0,
                                'feasible nodes': 'NZD',
                                'max nodes': 3,
                                'rate': 800.0,
                                'response time': 0.0,
                                'duration': 480.0}},
            'solver': {'type': 'coliny:StateMachineLS'},
            'configure': {'output prefix': 'Net3'}}
        self.problem.opts.set_value(template_options)
        
        self.problem.opts['network']['epanet file'] = join(dataDir,'Net3.inp')
        self.problem.opts['impact']['tai file'] = join(dataDir,'Net3_bio.tai')

    # This method is called after each test is executed.  
    def tearDown(self):
        os.chdir(self.origDir)
    
    # Test integration
    @unittest.skipIf(coliny is None, "Coliny is not available")
    def test_networkSolver_noInitialPoints(self):
        self.problem.opts['configure']['output prefix'] = 'networkSolver'
        fid = open('networkSolver.yml', 'w')
        fid.write(self.problem.opts.generate_yaml_template())
        fid.close()
        
        ans = pyutilib.subprocess.run([wst, 'flushing', 'networkSolver.yml'])
        self.assertEquals(ans[0], 0)
        
        f = open('networkSolver_flushing.json', 'r')
        output = json.load(f)
        
        self.assertEquals(output['nodes to flush'], ['101', '103', '109'])
        self.assertAlmostEquals(output['final metric'],4918.76, delta=0.01)
    
    @unittest.skipIf(dakota is None, "Dakota is not available")
    def test_colinyEA_smallCase(self):
        self.problem.opts['solver']['type'] = 'coliny_ea'
        self.problem.opts['solver']['options'] = {'max_iterations':10,
                                                  'max_function_evaluations':2,
                                                  'population_size':2,
                                                  'initialization_type':'unique_random',
                                                  'fitness_type':'linear_rank',
                                                  'crossover_rate':0.8,
                                                  'crossover_type':'uniform',
                                                  'mutation_rate':1,
                                                  'mutation_type':'offset_uniform',
                                                  'seed':11011011}
        self.problem.opts['configure']['output prefix'] = 'colinyEA'
        fid = open('colinyEA.yml', 'w')
        fid.write(self.problem.opts.generate_yaml_template())
        fid.close()
        
        ans = pyutilib.subprocess.run([wst, 'flushing', 'colinyEA.yml'])
        self.assertEquals(ans[0], 0)
        
        f = open('colinyEA_flushing.json', 'r')
        output = json.load(f)
        
        self.assertEquals(output['nodes to flush'], ['111', '107', '185'])
        self.assertAlmostEquals(output['final metric'],5758.34, delta=0.01)

        
if __name__ == "__main__":
    unittest.main()
