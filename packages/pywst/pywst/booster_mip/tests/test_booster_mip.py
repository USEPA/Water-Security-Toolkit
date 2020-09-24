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

import glob
import yaml
import copy
import shutil
import unittest
from nose.tools import nottest
import pyutilib.th as unittest
import pyutilib
import pywst.booster_mip.problem as problem
from pywst.common import pyunit
import pywst.common.problem

testDir = dirname(abspath(__file__))
if not pyunit.dataroot is None:
    dataDir = join(pyunit.dataroot,'booster_mip')
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
        # Prepend all tempory files created during these tests with this tag
        self.model_sol = {}
        self.problem = problem.Problem()
        self.default_opts = self.problem.opts.value()

    # This method is called once, after all tests are executed.
    @classmethod
    def tearDownClass(self):
        pass

    # This method is called before each test is executed.
    def setUp(self):
        self.origDir = os.getcwd()
        os.chdir(testDir)
        self.problem.opts.set_value(self.default_opts)
        self.problem.opts['configure']['output prefix'] = self.gen_test_dir()
        # This must be done after the output prefix has been set
        self.problem.setLogger('booster_mip')

    # This method is called after each test is executed.
    def tearDown(self):
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
    def diffSolutions(self,sol1,sol2,tol=0.01):
        self.assertAlmostEquals(sol1['detected scenarios']['expected mass consumed grams'],sol2['detected scenarios']['expected mass consumed grams'], delta=tol)
        self.assertAlmostEquals(sol1['non-detected scenarios']['expected mass consumed grams'],sol2['non-detected scenarios']['expected mass consumed grams'], delta=tol)
        self.assertAlmostEquals(sol1['all non-discarded scenarios']['expected mass consumed grams'],sol2['all non-discarded scenarios']['expected mass consumed grams'], delta=tol)
        self.assertEquals(sol1['detected scenarios']['count'],sol2['detected scenarios']['count'])
        self.assertEquals(sol1['non-detected scenarios']['count'],sol2['non-detected scenarios']['count'])
        self.assertEquals(sol1['all non-discarded scenarios']['count'],sol2['all non-discarded scenarios']['count'])
        self.assertEquals(sol1['total scenarios provided'],sol2['total scenarios provided'])
        self.assertEquals(sorted(sol1['booster nodes']),sorted(sol2['booster nodes']))
        self.assertEquals(len(sol1['detected scenarios']['mass consumed grams']),len(sol2['detected scenarios']['mass consumed grams']))
        self.assertEquals(len(sol1['detected scenarios']['mass injected grams']),len(sol2['detected scenarios']['mass injected grams']))
        self.assertEquals(len(sol1['detected scenarios']['response window mass injected grams']),len(sol2['detected scenarios']['response window mass injected grams']))
        self.assertEquals(len(sol1['non-detected scenarios']['mass injected grams']),len(sol2['non-detected scenarios']['mass injected grams']))
        for i in xrange(len(sol1['non-detected scenarios']['mass injected grams'])):
            self.assertAlmostEquals(sol1['non-detected scenarios']['mass injected grams'][i],sol2['non-detected scenarios']['mass injected grams'][i],delta=tol)
        for i in xrange(len(sol1['detected scenarios']['mass consumed grams'])):
            self.assertAlmostEquals(sol1['detected scenarios']['mass consumed grams'][i],sol2['detected scenarios']['mass consumed grams'][i],delta=tol)
        for i in xrange(len(sol1['detected scenarios']['pre-booster mass consumed grams'])):
            self.assertAlmostEquals(sol1['detected scenarios']['pre-booster mass consumed grams'][i],sol2['detected scenarios']['pre-booster mass consumed grams'][i],delta=tol)
        for i in xrange(len(sol1['detected scenarios']['mass injected grams'])):
            self.assertAlmostEquals(sol1['detected scenarios']['mass injected grams'][i],sol2['detected scenarios']['mass injected grams'][i],delta=tol)
        for i in xrange(len(sol1['detected scenarios']['response window mass injected grams'])):
            self.assertAlmostEquals(sol1['detected scenarios']['response window mass injected grams'][i],sol2['detected scenarios']['response window mass injected grams'][i],delta=tol)
        for i in xrange(len(sol1['detected scenarios']['tank mass grams'])):
            self.assertAlmostEquals(sol1['detected scenarios']['tank mass grams'][i],sol2['detected scenarios']['tank mass grams'][i],delta=tol)
    @nottest
    def setDefaultModelTestOptions(self):
        self.problem.opts['network']['epanet file'] = join(dataDir,'Net1.inp')
        self.problem.opts['network']['water quality timestep'] = 15
        self.problem.opts['network']['simulation duration'] = 1440
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'Net1.tsg')
        with open(join(dataDir,'Net1.sensors')) as f:
            self.problem.opts['booster mip']['detection'] = list(x.strip() for x in f.readlines())
        self.problem.opts['booster mip']['toxin decay coefficient'] = 0
        self.problem.opts['booster mip']['decon decay coefficient'] = 0
        self.problem.opts['booster mip']['feasible nodes'] = 'NZD'
        self.problem.opts['booster mip']['stoichiometric ratio'] = 0.01
        self.problem.opts['booster mip']['evaluate'] = False
        self.problem.opts['booster mip']['max boosters'] = [5,5]
        self.problem.opts['booster mip']['type'] = 'FLOWPACED'
        self.problem.opts['booster mip']['strength'] = '10000'
        self.problem.opts['booster mip']['response time'] = 60
        self.problem.opts['booster mip']['duration'] = 480
        if cplexamp is not None:
            self.problem.opts['solver']['type'] = basename(cplexamp)
        self.problem.opts['solver']['options'] = {'threads':1, 'predual':1, 'primalopt':None}
        self.problem.opts['solver']['problem writer'] = 'nl'
        if ampl is not None:
            self.problem.opts['configure']['ampl executable'] = basename(ampl)
        if pyomo is not None:
            self.problem.opts['configure']['pyomo executable'] = pyomo
        self.problem.opts['eventDetection'] = {}
        self.problem.opts['eventDetection']['options string'] = '--enable-logging'+' '
        self.problem.opts['boostersim'] = {}
        self.problem.opts['boostersim']['options string'] = '--max-rhs=400 --enable-logging'+' '
        self.problem.opts['boosterimpact'] = {}        
        self.problem.opts['boosterimpact']['options string'] = '--max-rhs=400 --enable-logging'+' '
        f = open(join(dataDir,'Net1.results'),'r')
        baseline = yaml.load(f)
        f.close()
        return baseline

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
 
    @unittest.skipIf(True, "This test is not ready.")
    def test_solve_pysp_limit(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'LIMIT'
        self.problem.opts['booster mip']['model format'] = 'PYSP'
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [pyomo,cplexamp], "pyomo or cplexamp is not available")
    def test_solve_pyomo_limit(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'LIMIT'
        self.problem.opts['booster mip']['model format'] = 'PYOMO'
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_ampl_limit(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'LIMIT'
        self.problem.opts['booster mip']['model format'] = 'AMPL'
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(True, "This test is not ready.")
    def test_solve_pysp_neutral(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'PYSP'
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)

    @unittest.skipIf(None in [pyomo,cplexamp], "pyomo or cplexamp is not available")
    def test_solve_pyomo_neutral(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'PYOMO'
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_ampl_neutral(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'AMPL'
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_ampl_neutralNet1(self):
        self.setDefaultModelTestOptions()
        self.problem.opts['network']['epanet file'] = join(dataDir,'Net1.inp')
        self.problem.opts['network']['water quality timestep'] = 5
        self.problem.opts['network']['simulation duration'] = 5760
        self.problem.opts['booster mip']['response time'] = 0
        self.problem.opts['booster mip']['duration'] = 1440
        self.problem.opts['booster mip']['max boosters'] = [0,1,5]
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'Test.tsg')
        with open(join(dataDir,'Net1.sensors')) as f:
            self.problem.opts['booster mip']['detection'] = list(x.strip() for x in f.readlines())
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'AMPL'
        baselines = [join(dataDir,'Net1-'+str(i)+'.results') for i in self.problem.opts['booster mip']['max boosters']]
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for i in xrange(len(sol_fnames)):
            f = open(sol_fnames[i],'r')
            sol = yaml.load(f)
            f.close()
            f = open(baselines[i],'r')
            bsol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, bsol)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_ampl_neutralNet1_slow(self):
        self.setDefaultModelTestOptions()
        self.problem.opts['network']['epanet file'] = join(dataDir,'Net1.inp')
        self.problem.opts['network']['water quality timestep'] = 5
        self.problem.opts['network']['simulation duration'] = 5760
        self.problem.opts['booster mip']['response time'] = 0
        self.problem.opts['booster mip']['duration'] = 1440
        self.problem.opts['booster mip']['max boosters'] = [0,1,5]
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'Test.tsg')
        with open(join(dataDir,'Net1.sensors')) as f:
            self.problem.opts['booster mip']['detection'] = list(x.strip() for x in f.readlines())
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'AMPL'
        self.problem.opts['boostersim']['options string'] = self.problem.opts['boostersim']['options string'] + ' --disable-reduced-problem --without-reduction --disable-scenario-aggregation --postprocess-passes=0 --max-rhs=1 '
        baselines = [join(dataDir,'Net1-'+str(i)+'.results') for i in self.problem.opts['booster mip']['max boosters']]
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for i in xrange(len(sol_fnames)):
            f = open(sol_fnames[i],'r')
            sol = yaml.load(f)
            f.close()
            f = open(baselines[i],'r')
            bsol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, bsol)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_ampl_neutralNet2(self):
        self.setDefaultModelTestOptions()
        self.problem.opts['network']['epanet file'] = join(dataDir,'Net2.inp')
        self.problem.opts['network']['water quality timestep'] = 5
        self.problem.opts['network']['simulation duration'] = 5760
        self.problem.opts['booster mip']['response time'] = 0
        self.problem.opts['booster mip']['duration'] = 1440
        self.problem.opts['booster mip']['max boosters'] = [0,1,5,10]
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'Test.tsg')
        with open(join(dataDir,'Net2.sensors')) as f:
            self.problem.opts['booster mip']['detection'] = list(x.strip() for x in f.readlines())
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'AMPL'
        baselines = [join(dataDir,'Net2-'+str(i)+'.results') for i in self.problem.opts['booster mip']['max boosters']]
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for i in xrange(len(sol_fnames)):
            f = open(sol_fnames[i],'r')
            sol = yaml.load(f)
            f.close()
            f = open(baselines[i],'r')
            bsol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, bsol, tol=0.1)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_ampl_neutralNet3(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['network']['epanet file'] = join(dataDir,'Net3.inp')
        self.problem.opts['network']['water quality timestep'] = 5
        self.problem.opts['network']['simulation duration'] = 5760
        self.problem.opts['network']['ignore merlion warnings'] = True
        self.problem.opts['booster mip']['response time'] = 0
        self.problem.opts['booster mip']['duration'] = 1440
        self.problem.opts['booster mip']['max boosters'] = [0,1,5,10]
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'Test.tsg')
        with open(join(dataDir,'Net3.sensors')) as f:
            self.problem.opts['booster mip']['detection'] = list(x.strip() for x in f.readlines())
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'AMPL'
        baselines = [join(dataDir,'Net3-'+str(i)+'.results') for i in self.problem.opts['booster mip']['max boosters']]
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for i in xrange(len(sol_fnames)):
            f = open(sol_fnames[i],'r')
            sol = yaml.load(f)
            f.close()
            f = open(baselines[i],'r')
            bsol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, bsol, tol=1.0)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_ampl_neutralNet1_nopostprocess(self):
        self.setDefaultModelTestOptions()
        self.problem.opts['network']['epanet file'] = join(dataDir,'Net1.inp')
        self.problem.opts['network']['water quality timestep'] = 5
        self.problem.opts['network']['simulation duration'] = 5760
        self.problem.opts['booster mip']['response time'] = 0
        self.problem.opts['booster mip']['duration'] = 1440
        self.problem.opts['booster mip']['max boosters'] = [0,1,5]
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'Test.tsg')
        with open(join(dataDir,'Net1.sensors')) as f:
            self.problem.opts['booster mip']['detection'] = list(x.strip() for x in f.readlines())
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'AMPL'
        self.problem.opts['boostersim']['options string'] = self.problem.opts['boostersim']['options string']+' --postprocess-passes=0 --max-rhs=-1 '
        baselines = [join(dataDir,'Net1-'+str(i)+'.results') for i in self.problem.opts['booster mip']['max boosters']]
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for i in xrange(len(sol_fnames)):
            f = open(sol_fnames[i],'r')
            sol = yaml.load(f)
            f.close()
            f = open(baselines[i],'r')
            bsol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, bsol)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_ampl_neutralNet2_nopostprocess(self):
        self.setDefaultModelTestOptions()
        self.problem.opts['network']['epanet file'] = join(dataDir,'Net2.inp')
        self.problem.opts['network']['water quality timestep'] = 5
        self.problem.opts['network']['simulation duration'] = 5760
        self.problem.opts['booster mip']['response time'] = 0
        self.problem.opts['booster mip']['duration'] = 1440
        self.problem.opts['booster mip']['max boosters'] = [0,1,5,10]
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'Test.tsg')
        with open(join(dataDir,'Net2.sensors')) as f:
            self.problem.opts['booster mip']['detection'] = list(x.strip() for x in f.readlines())
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'AMPL'
        self.problem.opts['boostersim']['options string'] = self.problem.opts['boostersim']['options string']+' --postprocess-passes=0 --max-rhs=1 '
        baselines = [join(dataDir,'Net2-'+str(i)+'.results') for i in self.problem.opts['booster mip']['max boosters']]
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for i in xrange(len(sol_fnames)):
            f = open(sol_fnames[i],'r')
            sol = yaml.load(f)
            f.close()
            f = open(baselines[i],'r')
            bsol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, bsol, tol=0.1)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_ampl_neutralNet3_nopostprocess(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['network']['epanet file'] = join(dataDir,'Net3.inp')
        self.problem.opts['network']['water quality timestep'] = 5
        self.problem.opts['network']['simulation duration'] = 5760
        self.problem.opts['network']['ignore merlion warnings'] = True
        self.problem.opts['booster mip']['response time'] = 0
        self.problem.opts['booster mip']['duration'] = 1440
        self.problem.opts['booster mip']['max boosters'] = [0,1,5,10]
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'Test.tsg')
        with open(join(dataDir,'Net3.sensors')) as f:
            self.problem.opts['booster mip']['detection'] = list(x.strip() for x in f.readlines())
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'AMPL'
        self.problem.opts['boostersim']['options string'] = self.problem.opts['boostersim']['options string'] + ' --postprocess-passes=0 --max-rhs=1 '
        baselines = [join(dataDir,'Net3-'+str(i)+'.results') for i in self.problem.opts['booster mip']['max boosters']]
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for i in xrange(len(sol_fnames)):
            f = open(sol_fnames[i],'r')
            sol = yaml.load(f)
            f.close()
            f = open(baselines[i],'r')
            bsol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, bsol, tol=1)
        self.cleanup(self.problem.opts['configure']['output prefix'])
            
    @unittest.skipIf(None in [pyomo,cplexamp], "pyomo or cplexamp is not available")
    def test_solve_pyomo_limit_Evaluate(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'LIMIT'
        self.problem.opts['booster mip']['model format'] = 'PYOMO'
        self.problem.opts['booster mip']['evaluate'] = True
        self.problem.opts['booster mip']['feasible nodes'] = copy.deepcopy(baseline['booster nodes'])
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_ampl_limit_Evaluate(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'LIMIT'
        self.problem.opts['booster mip']['model format'] = 'AMPL'
        self.problem.opts['booster mip']['evaluate'] = True
        self.problem.opts['booster mip']['feasible nodes'] = copy.deepcopy(baseline['booster nodes'])
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)
        self.cleanup(self.problem.opts['configure']['output prefix'])
            
    @unittest.skipIf(None in [pyomo,cplexamp], "pyomo or cplexamp is not available")
    def test_solve_pyomo_neutral_Evaluate(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'PYOMO'
        self.problem.opts['booster mip']['evaluate'] = True
        self.problem.opts['booster mip']['feasible nodes'] = copy.deepcopy(baseline['booster nodes'])
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_ampl_neutral_Evaluate(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'AMPL'
        self.problem.opts['booster mip']['evaluate'] = True
        self.problem.opts['booster mip']['feasible nodes'] = copy.deepcopy(baseline['booster nodes'])
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [pyomo,cplexamp], "pyomo or cplexamp is not available")
    def test_solve_pyomo_limit_Fixed(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'LIMIT'
        self.problem.opts['booster mip']['model format'] = 'PYOMO'
        self.problem.opts['booster mip']['fixed nodes'] = copy.deepcopy(baseline['booster nodes'])
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_ampl_limit_Fixed(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'LIMIT'
        self.problem.opts['booster mip']['model format'] = 'AMPL'
        self.problem.opts['booster mip']['fixed nodes'] = copy.deepcopy(baseline['booster nodes'])
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)
        self.cleanup(self.problem.opts['configure']['output prefix'])
            
    @unittest.skipIf(None in [pyomo,cplexamp], "pyomo or cplexamp is not available")
    def test_solve_pyomo_neutral_Fixed(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'PYOMO'
        self.problem.opts['booster mip']['fixed nodes'] = copy.deepcopy(baseline['booster nodes'])
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_ampl_neutral_Fixed(self):
        baseline = self.setDefaultModelTestOptions()
        self.problem.opts['booster mip']['model type'] = 'NEUTRAL'
        self.problem.opts['booster mip']['model format'] = 'AMPL'
        self.problem.opts['booster mip']['fixed nodes'] = copy.deepcopy(baseline['booster nodes'])
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) > 0)
        for fname in sol_fnames:
            f = open(fname,'r')
            sol = yaml.load(f)
            f.close()
            self.diffSolutions(sol, baseline)
        self.cleanup(self.problem.opts['configure']['output prefix'])

if __name__ == "__main__":
    unittest.main()
