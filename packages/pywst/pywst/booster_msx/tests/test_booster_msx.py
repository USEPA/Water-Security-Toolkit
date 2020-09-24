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

import copy
import shutil
import unittest
from nose.tools import nottest
import pyutilib.th as unittest
import pyutilib

import pywst.booster_msx.problem as problem
import pywst.common.wst_util as wst_util
from pywst.common import pyunit

testDir = dirname(abspath(__file__))
wstRoot = join(testDir, *('..',)*5)
dataDir = join(wstRoot,'examples','Net3')

try:
    import pyepanet
except ImportError:
    pass
    
wst = join(wstRoot, 'python', 'bin', 'wst')

dakota = pyutilib.services.registered_executable('dakota')
if not dakota is None:
    dakota = dakota.get_path()
    
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
            'scenario': {'end time': 1440,
                        'location': [101],
                        'msx file': 'Net3_bio.msx',
                        'msx species': 'BIO',
                        'species': 'BIO',
                        'start time': 0,
                        'strength': 100,
                        'type': 'MASS'},
            'impact': {'metric': ['MC'], 
                        'msx species': 'BIO'},
            'booster msx': {'decon species': 'CLF',
                        'detection': [111, 127, 179],
                        'duration': 600,
                        'max boosters': 2,
                        'response time': 0,
                        'strength': 4,
                        'toxin species': 'BIO',
                        'type': 'FLOWPACED'},
             'solver': {'type': 'coliny_ea'},
             'configure': {'output prefix': 'Net3'}}
        self.problem.opts.set_value(template_options)
        
        self.problem.opts['network']['epanet file'] = join(dataDir,'Net3.inp')
        self.problem.opts['scenario']['msx file'] = join(dataDir,'Net3_bio.msx')
        
    # This method is called after each test is executed.
    def tearDown(self):
        os.chdir(self.origDir)

    # Test integration
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
        
        ans = pyutilib.subprocess.run([wst, 'booster_msx', 'colinyEA.yml'])
        self.assertEquals(ans[0], 0)
        
        f = open('colinyEA_booster_msx.json', 'r')
        output = json.load(f)
        
        self.assertEquals(output['booster nodes'], ['10'])
        self.assertAlmostEquals(output['final metric'],37268.3, delta=0.01)
        
    
if __name__ == "__main__":
    unittest.main()
