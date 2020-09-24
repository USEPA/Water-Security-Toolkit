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
import pywst.inversion.problem as problem
from pywst.common import pyunit

testDir = dirname(abspath(__file__))
if not pyunit.dataroot is None:
    dataDir = join(pyunit.dataroot,'inversion')
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
        self.problem.setLogger('inversion')

    # This method is called after each test is executed.
    def tearDown(self):
        self.problem.resetLogger('inversion')
        os.chdir(self.origDir)
                
    # Test yaml options

    # Test functionality
    
    # Test integration

    # Test models
    @nottest
    def has_cplex_lp():
        from pymomo.environ import SolverFactory
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
    def setDefaultModelTestOptions(self,algorithm):
        self.problem.opts['network']['epanet file'] = join(dataDir,'Net1.inp')
        self.problem.opts['measurements']['grab samples'] = join(dataDir,'Net1_measures.dat')
        self.problem.opts['inversion']['algorithm'] = algorithm
        self.problem.opts['inversion']['formulation'] = 'MIP_discrete_nd'
        self.problem.opts['inversion']['model format'] = 'AMPL'
        self.problem.opts['inversion']['merlion water quality model'] = True
        self.problem.opts['inversion']['horizon'] = 1440
        self.problem.opts['inversion']['num injections'] = 1.0
        self.problem.opts['inversion']['measurement failure'] = 0.05
        self.problem.opts['inversion']['positive threshold'] = 100.0
        self.problem.opts['inversion']['negative threshold'] = 0.1
        self.problem.opts['inversion']['feasible nodes'] = None
        self.problem.opts['inversion']['candidate threshold'] = None
        self.problem.opts['inversion']['confidence'] = None
        self.problem.opts['inversion']['output impact nodes'] = False
        self.problem.opts['inversion']['wqm file'] = None
        if cplexamp is not None:
            self.problem.opts['solver']['type'] = basename(cplexamp)
        self.problem.opts['solver']['options'] = {'threads':1, 'predual':1, 'primalopt':None}
        self.problem.opts['solver']['problem writer'] = 'nl'
        if ampl is not None:
            self.problem.opts['configure']['ampl executable'] = basename(ampl)
        if pyomo is not None:
            self.problem.opts['configure']['pyomo executable'] = pyomo
#        if algorithm == 'BAYESIAN':
#            f = open(join(dataDir,'Net1.bayesian.results'),'r')
#        else:
#            f = open(join(dataDir,'Net1.optimization.results'),'r')
#        baseline = json.loads(f)
#        f.close()
#        return baseline

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

    # Testing default options with bayesian algorithm 
    def test_solve_bayesian(self):
        algorithm = 'bayesian'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['negative threshold'] = None
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.bayesian.results.defaults')).read()
        baseline = json.loads(f)
        self.assertTrue(len(Solution) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(Solution, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing default options with optimization (MIP_discrete_nd) algorithm
    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_mip_discrete_nd_AMPL(self):
        algorithm = 'optimization'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['formulation'] = 'MIP_discrete_nd'
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.MIP_discrete_nd.results.defaults')).read()
        baseline = json.loads(f)
        f2 = open(json_file).read()
        sol = json.loads(f2)
        self.assertTrue(len(sol) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(sol, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing default options with optimization (MIP_discrete) algorithm
    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_mip_discrete_AMPL(self):
        algorithm = 'optimization'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['formulation'] = 'MIP_discrete'
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.MIP_discrete.results.defaults')).read()
        baseline = json.loads(f)
        f2 = open(json_file).read()
        sol = json.loads(f2)
        self.assertTrue(len(sol) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(sol, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing default options with optimization (MIP_discrete_step) algorithm
    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_mip_discrete_step_AMPL(self):
        algorithm = 'optimization'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['formulation'] = 'MIP_discrete_step'
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.MIP_discrete_step.results.defaults')).read()
        baseline = json.loads(f)
        f2 = open(json_file).read()
        sol = json.loads(f2)
        self.assertTrue(len(sol) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(sol, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing default options with optimization (LP_discrete) algorithm
    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_lp_discrete_AMPL(self):
        algorithm = 'optimization'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['formulation'] = 'LP_discrete'
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.LP_discrete.results.defaults')).read()
        baseline = json.loads(f)
        f2 = open(json_file).read()
        sol = json.loads(f2)
        self.assertTrue(len(sol) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(sol, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing default options with optimization (MIP_discrete_nd) algorithm
    @unittest.skipIf(None in [pyomo,cplexamp], "pyomo or cplexamp is not available")
    def test_solve_mip_discrete_nd_PYOMO(self):
        algorithm = 'optimization'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['model format'] = 'PYOMO'
        self.problem.opts['inversion']['formulation'] = 'MIP_discrete_nd'
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.MIP_discrete_nd.results.defaults.pyomo')).read()
        baseline = json.loads(f)
        f2 = open(json_file).read()
        sol = json.loads(f2)
        self.assertTrue(len(sol) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(sol, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing default options with optimization (MIP_discrete) algorithm
    @unittest.skipIf(None in [pyomo,cplexamp], "pyomo or cplexamp is not available")
    def test_solve_mip_discrete_PYOMO(self):
        algorithm = 'optimization'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['model format'] = 'PYOMO'
        self.problem.opts['inversion']['formulation'] = 'MIP_discrete'
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.MIP_discrete.results.defaults.pyomo')).read()
        baseline = json.loads(f)
        f2 = open(json_file).read()
        sol = json.loads(f2)
        self.assertTrue(len(sol) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(sol, baseline, algorithm, 10)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing default options with optimization (MIP_discrete_step) algorithm
    @unittest.skipIf(None in [pyomo,cplexamp], "pyomo or cplexamp is not available")
    def test_solve_mip_discrete_step_PYOMO(self):
        algorithm = 'optimization'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['model format'] = 'PYOMO'
        self.problem.opts['inversion']['formulation'] = 'MIP_discrete_step'
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.MIP_discrete_step.results.defaults.pyomo')).read()
        baseline = json.loads(f)
        f2 = open(json_file).read()
        sol = json.loads(f2)
        self.assertTrue(len(sol) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(sol, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing default options with optimization (LP_discrete) algorithm
    @unittest.skipIf(None in [pyomo,cplexamp], "pyomo or cplexamp is not available")
    def test_solve_lp_discrete_PYOMO(self):
        algorithm = 'optimization'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['model format'] = 'PYOMO'
        self.problem.opts['inversion']['formulation'] = 'LP_discrete'
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.LP_discrete.results.defaults.pyomo')).read()
        baseline = json.loads(f)
        f2 = open(json_file).read()
        sol = json.loads(f2)
        self.assertTrue(len(sol) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(sol, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing feasible modes option with bayesian algorithm 
    def test_solve_bayesian_feasible(self):
        algorithm = 'bayesian'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['feasible nodes'] = join(dataDir,'allowed_nodes_bayesian')
        self.problem.opts['inversion']['negative threshold'] = None
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.bayesian.results.allowed')).read()
        baseline = json.loads(f)
        self.assertTrue(len(Solution) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(Solution, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing feasible nodes option with optimization (MIP_discrete) algorithm
    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_mip_discrete_AMPL_feasible(self):
        algorithm = 'optimization'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['feasible nodes'] = join(dataDir,'allowed_nodes_MIP_discrete')
        self.problem.opts['inversion']['formulation'] = 'MIP_discrete'
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.MIP_discrete.results.allowed')).read()
        baseline = json.loads(f)
        f2 = open(json_file).read()
        sol = json.loads(f2)
        self.assertTrue(len(sol) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(sol, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing measurement failure option with bayesian algorithm 
    def test_solve_bayesian_meas_failure(self):
        algorithm = 'bayesian'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['measurement failure'] = 0.50
        self.problem.opts['inversion']['negative threshold'] = None
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.bayesian.results.meas_failure')).read()
        baseline = json.loads(f)
        self.assertTrue(len(Solution) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(Solution, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing confidence option with bayesian algorithm 
    def test_solve_bayesian_confidence(self):
        algorithm = 'bayesian'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['confidence'] = 0.50
        self.problem.opts['inversion']['negative threshold'] = None
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.bayesian.results.confidence')).read()
        baseline = json.loads(f)
        self.assertTrue(len(Solution) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(Solution, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])


    # Testing horizon option with bayesian algorithm 
    def test_solve_bayesian_horizon(self):
        algorithm = 'bayesian'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['horizon'] = 720
        self.problem.opts['inversion']['negative threshold'] = None
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.bayesian.results.horizon')).read()
        baseline = json.loads(f)
        self.assertTrue(len(Solution) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(Solution, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing horizon option with optimization (MIP_discrete) algorithm
    @unittest.skipIf(None in [ampl,cplexamp], "ampl or cplexamp is not available")
    def test_solve_mip_discrete_AMPL_horizon(self):
        algorithm = 'optimization'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['horizon'] = 720
        self.problem.opts['inversion']['formulation'] = 'MIP_discrete'
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.MIP_discrete.results.horizon')).read()
        baseline = json.loads(f)
        f2 = open(json_file).read()
        sol = json.loads(f2)
        self.assertTrue(len(sol) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(sol, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    # Testing default options with bayesian algorithm 
    def test_solve_bayesian_with_epanet(self):
        algorithm = 'bayesian'
        self.setDefaultModelTestOptions(algorithm)
        self.problem.opts['inversion']['negative threshold'] = None
        self.problem.opts['inversion']['merlion water quality model'] = False
        [Solution, json_file, tsg_file, num_events] = self.problem.run()
        f = open(join(dataDir,'Net1.bayesian.results.defaults')).read()
        baseline = json.loads(f)
        self.assertTrue(len(Solution) > 0)
        self.assertTrue(len(json_file) > 0)
        self.diffSolutions(Solution, baseline, algorithm)
        self.cleanup(self.problem.opts['configure']['output prefix'])

if __name__ == "__main__":
    unittest.main()
