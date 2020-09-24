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
currdir = dirname(abspath(__file__))+os.sep
    
import inspect
import json
import re
import copy
import shutil
import pyutilib.th as unittest
from nose.tools import nottest
import pyutilib.th
import pyutilib
import pyutilib.misc
import pyutilib.common
import pywst.sp.problem as problem
import yaml
import pprint
from pywst.common import pyunit
from pywst.sp.parse_sp import process_evalsensors
from pywst.sp.driver import sp_main

from pyomo.environ import *

#if __name__ == "__main__":
    #os.environ['PYUTILIB_UNITTEST_CATEGORIES'] = 'smoke'

testDir = dirname(abspath(__file__))
wstRoot = join(testDir, *('..',)*5)
if pyunit.datadir is None:
    dataDir = None
else:
    dataDir = join(pyunit.datadir,'sp')
baselineDir = join(testDir,'baselines')

wst = join(wstRoot, 'python', 'bin', 'wst')
os.environ['PATH'] = join(wstRoot,'bin') + os.pathsep + join(wstRoot,'python','bin') + os.pathsep + os.environ['PATH']

pico_available = False
try:
    opt = SolverFactory('pico')
    pico_available = opt.available(False) and (not opt.executable() is None)
    del opt
except pyutilib.common.ApplicationError:
    pico_available=False
pico_available = False

cplex_available = False
try:
    opt = SolverFactory('cplex')
    cplex_available = opt.available(False) and (not opt.executable() is None)
    del opt
except pyutilib.common.ApplicationError:
    cplex_available=False

cplex_direct_available = False
try:
    opt = SolverFactory('cplex_direct')
    cplex_direct_available = opt.available(False) and (not opt.executable() is None)
    del opt
except pyutilib.common.ApplicationError:
    cplex_direct_available=False

gurobi_available = False
try:
    opt = SolverFactory('gurobi')
    gurobi_available = opt.available(False) and (not opt.executable() is None)
    del opt
except pyutilib.common.ApplicationError:
    gurobi_available=False

gurobi_direct_available = False
try:
    opt = SolverFactory('gurobi_direct')
    gurobi_direct_available = opt.available(False) and (not opt.executable() is None) 
    del opt
except pyutilib.common.ApplicationError:
    gurobi_direct_available=False

glpk_available = False
try:
    opt = SolverFactory('glpk')
    glpk_available = opt.available(False) and (not opt.executable() is None)
    del opt
except pyutilib.common.ApplicationError:
    glpk_available=False

cbc_available = False
try:
    opt = SolverFactory('cbc')
    cbc_available = opt.available(False) and (not opt.executable() is None)
    del opt
except pyutilib.common.ApplicationError:
    cbc_available=False

xpress_available = False
try:
    opt = SolverFactory('xpress')
    xpress_available = opt.available(False) and (not opt.executable() is None)
    del opt
except pyutilib.common.ApplicationError:
    xpress_available=False



dir_counter = 1
#
# Create unittest testing class
#
class TestBase(pyutilib.th.TestCase):

    solver = None
    problem_lang = 'pyomo'
    wst_command = 'sp'
    shellcmd = False
    statistic = None
    abstolerance = {}
    skip_list = set()
    expected_exceptions = set()
    _aux_tests = None

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
        self.problem.opts.set_value( 
            {
            'impact data': {
                    'name': 'mc',
                    'impact file': os.path.join(dataDir, 'test1_mc.impact'),
                    'nodemap file': os.path.join(dataDir, 'test1.nodemap'),
                    'weight file': None,
                    #'directory': dataDir,
                    #'response time': None,
                    },
            'cost': {
                    'name': None,
                    'cost file': None,},
                    #'directory': None,},
            'objective': {
                    'name': 'obj1',
                    'goal': 'mc',
                    'statistic': 'MEAN',
                    'gamma': None},
            'constraint': [{
                    'name': 'const1',
                    'goal': 'NS',
                    'statistic': 'TOTAL',
                    'gamma': None,
                    'bound': 5,
                    'scenario': [{
                        'name': None,
                        'probability': 1.0,
                        'bound': 0
                        }]
                    }],
            'aggregate': {
                    'name': None,
                    'goal': None,
                    'type': None,
                    'value': None,
                    'conserve memory': None,
                    'distinguish detection': None,
                    'disable aggregation': None},
            'imperfect': {
                    'name': None,
                    'sensor class file': None,
                    'junction class file': None},
                    #'directory': None},
            'solver': {
                    'name': 'solver1',
                    'type': self.solver,
                    'options': {},
                    # remove most of these options...
                    #'number of samples': None,
                    #'representation': None,
                    #'runtime': None,
                    #'notify': None,
                    #'compute bound': False,
                    #'seed': None,
                    #'timelimit': None,
                    },
            'sensor placement': [{
                    'name': 'problem1',
                    'type': 'P-MEDIAN',
                    'modeling language': self.problem_lang,
                    'objective': 'obj1',
                    'constraint': 'const1',
                    'aggregate': None,
                    'imperfect': None,
                    'location': [{
                        'feasible nodes': 'ALL',
                        'infeasible nodes': 'NONE',
                        'fixed nodes': 'NONE',
                        'unfixed nodes': 'NONE'
                        }],
                    'compute greedy ranking': False}],
            'configure': {
                    #'output prefix': 'wst_sp',
                    #'problem': 'problem1',
                    #'solver': 'solver1',
                    
                    #'evaluate all': None,
                    #'memmon': False,
                    #'memcheck': None,
                    #'format': None,
                    #'ampl executable': None,
                    #'ampl data': None,
                    #'ampl model': None,
                    #'gap': None,
                    #'evalsensor': True,
                    'keepfiles': True,
                    #'print log': False,
                    #'ampl cplex path': None,
                    #'pico path': None,
                    #'glpk path': None,
                    #'sp executable': 'sp',
                    #'temp directory': None,
                    #'path': [],
                    #'version': False,
                    'debug': 1}
            } )
                    
        self.problem.saveAll('temp.yml')            
        self.problem.load('temp.yml')
        os.remove('temp.yml')
        
    # This method is called after each test is executed.
    def tearDown(self):
        os.chdir(self.origDir)
    
    # Execute integration tests
    @nottest
    def execute_test(self, name=None, baseline=False, quiet=False, rmdir=False):
        #"""Execute a test given different information"""
        self.data = None
        if name is None:
            name = inspect.stack()[1][3]
        if name in self.skip_list:
            self.skipTest("Skipping test %s" % name)
        #
        newdir_ = currdir+name
        newdir = newdir_+os.sep
        if os.path.exists(newdir):
            shutil.rmtree(newdir)
        ans = None
        try:
            os.mkdir(newdir)
            currdir_ = os.getcwd()
            os.chdir(newdir)
            #
            self.problem.saveAll(name+'.yml')
            #
            if self.shellcmd:
                cmd = [wst, '-t', self.wst_command, name+'.yml']
                if not quiet:
                    print "Running",cmd
                pyutilib.subprocess.run(cmd, name+"_test_cmd.out",cwd=newdir)
            else:
                pyutilib.misc.setup_redirect(name+"_test_cmd.out")
                try:
                    if name in self.expected_exceptions:
                        with self.assertRaises(Exception):
                            sp_main([name+'.yml'])
                    else:
                        sp_main([name+'.yml'])
                finally:
                    pyutilib.misc.reset_redirect()
            if not name in self.expected_exceptions:
                self.verify_output(name+"_test_cmd.out")
                ans = process_evalsensors(name+"_test_cmd.out", name+"_parsed.out")
                os.chdir(self.origDir)
                if baseline is True:
                    baselinefile = join(baselineDir, name+'.qa')
                elif baseline:
                    baselinefile = join(baselineDir, baseline+'.qa')
                if baseline:
                    if self.statistic:
                        def filter_fn(line):
                            return self.statistic not in line
                        self.assertFileEqualsBaseline(newdir+name+"_parsed.out", baselinefile, delete=False, filter=filter_fn, tolerance=self.abstolerance.get(name,0.0))
                    else:
                        self.assertFileEqualsBaseline(newdir+name+"_parsed.out", baselinefile, delete=False)    
                if not self._aux_tests is None:
                    self._aux_tests(self)
                    self._aux_tests = None
        except Exception, err:
            print "ERROR: %s" % str(err)
            raise
        finally:
            os.chdir(currdir_)
            #pyutilib.misc.reset_redirect()
            if os.path.exists(newdir):
                if rmdir:
                    shutil.rmtree(newdir)
                else:
                    global dir_counter
                    shutil.move(newdir_+os.sep, newdir_+'.'+str(dir_counter)+os.sep)
                    dir_counter += 1
        return ans

    def verify_output(self, fname):
        pass

    def parse_output(self, fname):
        data = pyutilib.misc.Options()
        data.wst = False
        data.pyomo = False
        #
        INPUT = open(fname, 'r')
        for line in INPUT:
            #print "LINE", line
            line = line.strip()
            if 'WST...' in line:
                data.wst = True
            elif 'Modeling Language' in line and 'ampl' in line:
                data.pyomo = False
            elif 'Modeling Language' in line and 'none' in line:
                data.pyomo = False
            elif 'Modeling Language' in line and 'pyomo' in line:
                data.pyomo = True
            elif line.startswith('Objective:'):
                data.objective = eval(re.split('[ \t]+', line)[1])
            elif line.startswith('Lower Bound:'):
                val = re.split('[ \t]+', line.strip())[2]
                if val == '-inf':
                    data.lower_bound = float(val)
                else:
                    data.lower_bound = eval(val)
            elif line.startswith('Upper Bound:'):
                val = re.split('[ \t]+', line.strip())[2]
                data.upper_bound = float(val)
        INPUT.close()
        self.data = data
        return data

    def _assert_zero_sensors(self, name, solver):
        self.problem.setConstraintOption('bound', 0, 0)
        #self.problem.setSolverOption('type', 0, solver)
        try:
            data = self.execute_test(name, baseline=False, quiet=True, rmdir=True)
            self.fail("Expecting an exception")
        except:
            pass
        
    def _assert_too_many_sensors(self, name, solver):
        self.problem.setConstraintOption('bound', 0, 200)
        #self.problem.setSolverOption('type', 0, solver)
        try:
            data = self.execute_test(name, baseline=False, quiet=True, rmdir=True)
            self.fail("Expecting an exception")
        except:
            pass

    def _assert_no_feasible_locations(self, name, solver):
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('infeasible nodes', 0, 'ALL')
        self.problem.setConstraintOption('bound', 0, 200)
        #self.problem.setSolverOption('type', 0, solver)
        try:
            data = self.execute_test(name, baseline=False, quiet=True, rmdir=True)
            self.fail("Expecting an exception")
        except:
            pass

    def _assert_only_fixed_locations(self, name, solver):
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('fixed nodes', 0, 'ALL')
        self.problem.setConstraintOption('bound', 0, 200)
        #self.problem.setSolverOption('type', 0, solver)
        try:
            data = self.execute_test(name, baseline=False, quiet=True, rmdir=True)
            self.fail("Expecting an exception")
        except:
            pass


class TestHelp(TestBase):

    solver = None

    def test_help_solvers(self):
        pyutilib.misc.setup_redirect(currdir+'help_solvers.out')
        try:
            sp_main(['--help-solvers'])
        finally:
            pyutilib.misc.reset_redirect()
        os.remove(currdir+'help_solvers.out')

    def test_help_problems(self):
        pyutilib.misc.setup_redirect(currdir+'help_problems.out')
        try:
            sp_main(['--help-problems'])
        finally:
            pyutilib.misc.reset_redirect()
        os.remove(currdir+'help_problems.out')


class TestErrors(TestBase):

    expected_exceptions = set(['test_bad_goal_statistic'])

    def test_bad_goal_statistic(self):
        #"""p-median objective=mc_bad, ub=ns,5"""
        self.problem.setObjectiveOption('statistic', 0, 'bad')
        self.execute_test(baseline='test1a', quiet=True, rmdir=True)
    

class PyomoTester(object):

    def verify_output(self, fname):
        data = self.parse_output(fname)
        if not data.wst or not data.pyomo:
            self.fail("Expected a WST solver with Pyomo model: coopr=%s pyomo=%s" % (str(data.wst),str(data.pyomo)))

    #

    def test_pmedian_none_all_ns(self):
        #"""p-median objective=mc, ub=ns,5"""
        self.execute_test(baseline='test1a', quiet=True)
    
    def test_pmedian_thresh_all_ns(self):
        #"""p-median objective=mc ub=ns,5 aggr-threshold=mc,20000"""
        self.problem.setLocationOption('infeasible nodes', 0, [219])
        self.problem.setAggregateOption('name', 0, 'agg1')
        self.problem.setAggregateOption('goal', 0, 'mc')
        self.problem.setAggregateOption('type', 0, 'THRESHOLD')
        self.problem.setAggregateOption('value', 0, 20000)
        self.problem.setProblemOption('aggregate', 0, 'agg1')
        self.execute_test(baseline='test1h', quiet=True)
    
    def test_pmedian_percent_all_ns(self):
        #"""p-median objective=mc ub=ns,5 aggr:percent=mc,0.5"""
        self.problem.setLocationOption('infeasible nodes', 0, [219])
        self.problem.setAggregateOption('name', 0, 'agg1')
        self.problem.setAggregateOption('goal', 0, 'mc')
        self.problem.setAggregateOption('type', 0, 'PERCENT')
        self.problem.setAggregateOption('value', 0, 0.5)
        self.problem.setProblemOption('aggregate', 0, 'agg1')
        self.execute_test(baseline='test1tt', quiet=True)
    
    #

    def test_pmedian_none_fixed_ns(self):
        #"""p-median objective=mc ub=ns,5 locs:fixed"""
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('infeasible nodes', 0, [219])
        self.problem.setLocationOption('fixed nodes', 0, [143,167])
        self.execute_test(baseline='test1m', quiet=True)
    
    def test_pmedian_thresh_fixed_ns(self):
        #"""p-median objective=mc ub=ns,5 aggr-threshold=mc,20000 locs:fixed"""
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('infeasible nodes', 0, [219])
        self.problem.setLocationOption('fixed nodes', 0, [143,167])
        self.problem.setAggregateOption('name', 0, 'agg1')
        self.problem.setAggregateOption('goal', 0, 'mc')
        self.problem.setAggregateOption('type', 0, 'THRESHOLD')
        self.problem.setAggregateOption('value', 0, 20000)
        self.problem.setProblemOption('aggregate', 0, 'agg1')
        self.execute_test(baseline='test1m', quiet=True)
    
    def test_pmedian_percent_fixed_ns(self):
        #"""p-median objective=mc ub=ns,5 aggr:percent=mc,0.5 locs:fixed"""
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('infeasible nodes', 0, [219])
        self.problem.setLocationOption('fixed nodes', 0, [143,167])
        self.problem.setAggregateOption('name', 0, 'agg1')
        self.problem.setAggregateOption('goal', 0, 'mc')
        self.problem.setAggregateOption('type', 0, 'PERCENT')
        self.problem.setAggregateOption('value', 0, 0.5)
        self.problem.setProblemOption('aggregate', 0, 'agg1')
        self.execute_test(baseline='test1m', quiet=True)
    
    #

    def test_pmedian_none_feas_ns(self):
        #"""p-median objective=mc ub=ns,5 locs:feas"""
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('infeasible nodes', 0, 'ALL')
        self.problem.setLocationOption('feasible nodes', 1, [101,103,105,107,109,111,113])
        self.execute_test(baseline='test1o', quiet=True)
    
    def test_pmedian_thresh_feas_ns(self):
        #"""p-median objective=mc ub=ns,5 aggr-threshold=mc,20000 locs:feas"""
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('infeasible nodes', 0, 'ALL')
        self.problem.setLocationOption('feasible nodes', 1, [101,103,105,107,109,111,113])
        self.problem.setAggregateOption('name', 0, 'agg1')
        self.problem.setAggregateOption('goal', 0, 'mc')
        self.problem.setAggregateOption('type', 0, 'THRESHOLD')
        self.problem.setAggregateOption('value', 0, 20000)
        self.problem.setProblemOption('aggregate', 0, 'agg1')
        self.execute_test(baseline='test1oo', quiet=True)
    
    def test_pmedian_percent_feas_ns(self):
        #"""p-median objective=mc ub=ns,5 aggr:percent=mc,0.5 locs:feas"""
        self.problem.setConstraintOption('bound', 0, 3)
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('infeasible nodes', 0, 'ALL')
        self.problem.setLocationOption('feasible nodes', 1, [101,103,105,107,109,111,113])
        self.problem.setAggregateOption('name', 0, 'agg1')
        self.problem.setAggregateOption('goal', 0, 'mc')
        self.problem.setAggregateOption('type', 0, 'PERCENT')
        self.problem.setAggregateOption('value', 0, 0.5)
        self.problem.setProblemOption('aggregate', 0, 'agg1')
        self.execute_test(baseline='test1x', quiet=True)
    
    #
    #

    def test_pmedian_none_all_cost(self):
        #"""p-median objective=mc, ub=cost,20"""
        self.problem.setConstraintOption('goal', 0, 'cost1')
        self.problem.setConstraintOption('bound', 0, 20)
        self.problem.setCostOption('name', 0, 'cost1')
        self.problem.setCostOption('cost file', 0, os.path.join(dataDir, 'test1-id-costs'))
        #self.problem.setCostOption('directory', 0, dataDir)
        self.execute_test(baseline='test1f', quiet=True)
    
    def test_pmedian_thresh_all_cost(self):
        #"""p-median objective=mc ub=cost,20 aggr-threshold=mc,20000"""
        self.problem.setLocationOption('infeasible nodes', 0, [219])
        self.problem.setConstraintOption('goal', 0, 'cost1')
        self.problem.setConstraintOption('bound', 0, 20)
        self.problem.setCostOption('name', 0, 'cost1')
        self.problem.setCostOption('cost file', 0, os.path.join(dataDir, 'test1-id-costs'))
        #self.problem.setCostOption('directory', 0, dataDir)
        self.problem.setAggregateOption('name', 0, 'agg1')
        self.problem.setAggregateOption('goal', 0, 'mc')
        self.problem.setAggregateOption('type', 0, 'THRESHOLD')
        self.problem.setAggregateOption('value', 0, 20000)
        self.problem.setProblemOption('aggregate', 0, 'agg1')
        self.execute_test(baseline='cost2', quiet=True)
    
    def test_pmedian_percent_all_cost(self):
        #"""p-median objective=mc ub=cost,20 aggr:percent=mc,0.5"""
        self.problem.setLocationOption('infeasible nodes', 0, [219])
        self.problem.setConstraintOption('goal', 0, 'cost1')
        self.problem.setConstraintOption('bound', 0, 20)
        self.problem.setCostOption('name', 0, 'cost1')
        self.problem.setCostOption('cost file', 0, os.path.join(dataDir, 'test1-id-costs'))
        #self.problem.setCostOption('directory', 0, dataDir)
        self.problem.setAggregateOption('name', 0, 'agg1')
        self.problem.setAggregateOption('goal', 0, 'mc')
        self.problem.setAggregateOption('type', 0, 'PERCENT')
        self.problem.setAggregateOption('value', 0, 0.5)
        self.problem.setProblemOption('aggregate', 0, 'agg1')
        self.execute_test(baseline='cost2', quiet=True)
    
    #

    def test_pmedian_none_fixed_cost(self):
        #"""p-median objective=mc ub=cost,20 locs:fixed"""
        self.problem.setConstraintOption('goal', 0, 'cost1')
        self.problem.setConstraintOption('bound', 0, 20)
        self.problem.setCostOption('name', 0, 'cost1')
        self.problem.setCostOption('cost file', 0, os.path.join(dataDir, 'test1-id-costs'))
        #self.problem.setCostOption('directory', 0, dataDir)
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('infeasible nodes', 0, [219])
        self.problem.setLocationOption('fixed nodes', 0, [143,167])
        self.execute_test(baseline='cost1', quiet=True)
    
    def test_pmedian_thresh_fixed_cost(self):
        #"""p-median objective=mc ub=cost,20 aggr-threshold=mc,20000 locs:fixed"""
        self.problem.setConstraintOption('goal', 0, 'cost1')
        self.problem.setConstraintOption('bound', 0, 20)
        self.problem.setCostOption('name', 0, 'cost1')
        self.problem.setCostOption('cost file', 0, os.path.join(dataDir, 'test1-id-costs'))
        #self.problem.setCostOption('directory', 0, dataDir)
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('infeasible nodes', 0, [219])
        self.problem.setLocationOption('fixed nodes', 0, [143,167])
        self.problem.setAggregateOption('name', 0, 'agg1')
        self.problem.setAggregateOption('goal', 0, 'mc')
        self.problem.setAggregateOption('type', 0, 'THRESHOLD')
        self.problem.setAggregateOption('value', 0, 20000)
        self.problem.setProblemOption('aggregate', 0, 'agg1')
        self.execute_test(baseline='cost1', quiet=True)
    
    def test_pmedian_percent_fixed_cost(self):
        #"""p-median objective=mc ub=cost,20 aggr:percent=mc,0.5 locs:fixed"""
        self.problem.setConstraintOption('goal', 0, 'cost1')
        self.problem.setConstraintOption('bound', 0, 20)
        self.problem.setCostOption('name', 0, 'cost1')
        self.problem.setCostOption('cost file', 0, os.path.join(dataDir, 'test1-id-costs'))
        #self.problem.setCostOption('directory', 0, dataDir)
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('infeasible nodes', 0, [219])
        self.problem.setLocationOption('fixed nodes', 0, [143,167])
        self.problem.setAggregateOption('name', 0, 'agg1')
        self.problem.setAggregateOption('goal', 0, 'mc')
        self.problem.setAggregateOption('type', 0, 'PERCENT')
        self.problem.setAggregateOption('value', 0, 0.5)
        self.problem.setProblemOption('aggregate', 0, 'agg1')
        self.execute_test(baseline='cost3', quiet=True)
    
    #

    def test_pmedian_none_feas_cost(self):
        #"""p-median objective=mc ub=cost,15 locs:feas"""
        self.problem.setConstraintOption('goal', 0, 'cost1')
        self.problem.setConstraintOption('bound', 0, 15)
        self.problem.setCostOption('name', 0, 'cost1')
        self.problem.setCostOption('cost file', 0, os.path.join(dataDir, 'test1-id-costs'))
        #self.problem.setCostOption('directory', 0, dataDir)
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('infeasible nodes', 0, 'ALL')
        self.problem.setLocationOption('feasible nodes', 1, [101,103,105,107,109,111,113])
        self.execute_test(baseline='cost5', quiet=True)
    
    def test_pmedian_thresh_feas_cost(self):
        #"""p-median objective=mc ub=cost,15 aggr-threshold=mc,20000 locs:feas"""
        self.problem.setConstraintOption('goal', 0, 'cost1')
        self.problem.setConstraintOption('bound', 0, 15)
        self.problem.setCostOption('name', 0, 'cost1')
        self.problem.setCostOption('cost file', 0, os.path.join(dataDir, 'test1-id-costs'))
        #self.problem.setCostOption('directory', 0, dataDir)
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('infeasible nodes', 0, 'ALL')
        self.problem.setLocationOption('feasible nodes', 1, [101,103,105,107,109,111,113])
        self.problem.setAggregateOption('name', 0, 'agg1')
        self.problem.setAggregateOption('goal', 0, 'mc')
        self.problem.setAggregateOption('type', 0, 'THRESHOLD')
        self.problem.setAggregateOption('value', 0, 20000)
        self.problem.setProblemOption('aggregate', 0, 'agg1')
        self.execute_test(baseline='cost5', quiet=True)
    
    def test_pmedian_percent_feas_cost(self):
        #"""p-median objective=mc ub=cost,15 aggr:percent=mc,0.5 locs:feas"""
        self.problem.setConstraintOption('goal', 0, 'cost1')
        self.problem.setConstraintOption('bound', 0, 15)
        self.problem.setCostOption('name', 0, 'cost1')
        self.problem.setCostOption('cost file', 0, os.path.join(dataDir, 'test1-id-costs'))
        #self.problem.setCostOption('directory', 0, dataDir)
        self.problem.setLocationOption('feasible nodes', 0, None)
        self.problem.setLocationOption('infeasible nodes', 0, 'ALL')
        self.problem.setLocationOption('feasible nodes', 1, [101,103,105,107,109,111,113])
        self.problem.setAggregateOption('name', 0, 'agg1')
        self.problem.setAggregateOption('goal', 0, 'mc')
        self.problem.setAggregateOption('type', 0, 'PERCENT')
        self.problem.setAggregateOption('value', 0, 0.5)
        self.problem.setProblemOption('aggregate', 0, 'agg1')
        self.execute_test(baseline='cost5', quiet=True)
    
    ##

    def test_worst_none_all_ns(self):
        #"""worst objective=mc, ub=ns,5"""
        self.problem.setConstraintOption('bound', 0, 10)
        # This eliminates an alternate solution
        self.problem.setLocationOption('infeasible nodes', 0, [143])
        self.problem.setObjectiveOption('statistic', 0, 'worst')
        self.problem.setProblemOption('type', 0, 'worst-case perfect-sensor')
        self.execute_test(baseline='worst1', quiet=True)
    
    def test_worst_none_infeas_ns(self):
        #"""worst objective=mc, ub=ns,1 infeas"""
        self.problem.setConstraintOption('bound', 0, 1)
        self.problem.setLocationOption('infeasible nodes', 0, [15,109,131,166,167,219,225,231,243,253])
        self.problem.setObjectiveOption('statistic', 0, 'worst')
        self.problem.setProblemOption('type', 0, 'worst-case perfect-sensor')
        self.execute_test(baseline='worst2', quiet=True)
    
    def test_worst_thresh_all_ns(self):
        #"""worst objective=mc, ub=ns,10"""
        self.problem.setAggregateOption('name', 0, 'agg1')
        self.problem.setAggregateOption('goal', 0, 'mc')
        self.problem.setAggregateOption('type', 0, 'THRESHOLD')
        self.problem.setAggregateOption('value', 0, 20000)
        self.problem.setProblemOption('aggregate', 0, 'agg1')
        self.problem.setConstraintOption('bound', 0, 15)
        self.problem.setObjectiveOption('statistic', 0, 'worst')
        self.problem.setProblemOption('type', 0, 'worst-case perfect-sensor')
        self.execute_test(baseline=False, quiet=True)
        if not self.data is None:
            self.assertAlmostEqual(self.data.objective, 57145.9, places=5)
    
    def test_worst_none_all_cost(self):
        #"""worst objective=mc, ub=cost,50"""
        self.problem.setObjectiveOption('statistic', 0, 'worst')
        self.problem.setProblemOption('type', 0, 'worst-case perfect-sensor')
        self.problem.setConstraintOption('goal', 0, 'cost1')
        self.problem.setConstraintOption('bound', 0, 50)
        self.problem.setCostOption('name', 0, 'cost1')
        self.problem.setCostOption('cost file', 0, os.path.join(dataDir, 'test1-id-costs'))
        #self.problem.setCostOption('directory', 0, dataDir)
        self.execute_test(baseline=False, quiet=True)
        if not self.data is None:
            self.assertAlmostEqual(self.data.objective, 144271.0, places=4)
    
    ##

    def test_zero_sensors(self):
        #"""test zero sensors"""
        for solver in [self.solver]:
            self._assert_zero_sensors('test_zero_sensors', solver)

    def test_too_many_sensors(self):
        #"""test too many sensors"""
        for solver in [self.solver]:
            self._assert_too_many_sensors('test_too_many_sensors', solver)

    def test_no_feasible_locations(self):
        #"""test no feasible locations"""
        for solver in [self.solver]:
            self._assert_no_feasible_locations('test_no_feasible_locations', solver)

    def test_only_fixed_locations(self):
        #"""test only fixed locations"""
        for solver in [self.solver]:
            self._assert_only_fixed_locations('test_only_fixed_locations', solver)

    ##

    #
    # BUG: Pyomo relaxations continue to impact models, even
    # after the current model is analyzed
    #
    def Xtest_pmedian_none_all_ns_bound(self):
        #"""p-median objective=mc, ub=ns,5 compute-bound"""
        self.problem.setProblemOption('compute bound', 0, True)
        self.execute_test(baseline=False, quiet=True)
        if not self.data is None:
            self.assertAlmostEqual(self.data.objective, 21781.989878, places=1)
            self.assertAlmostEqual(self.data.lower_bound, 21781.989878, places=1)
            self.assertEqual(self.data.upper_bound, None)
    

#
# pico solver, pyomo model
#
@unittest.skipIf(not pico_available, "The 'pico' executable is not available")
class TestPicoPyomo(PyomoTester, TestBase):
    solver = 'pico'

#
# att_grasp solver, none model
#
class TestAttGrasp(PyomoTester, TestBase):

    solver = None
    problem_lang = 'none'
    statistic = 'Mean impact'
    abstolerance = {'test_pmedian_thresh_feas_ns':100.0,
                    'test_pmedian_thresh_all_ns':100.0,
                    'test_pmedian_percent_all_ns':200.0,
                    'test_pmedian_percent_feas_ns':500.0
                    }
    skip_list = set(['test_pmedian_none_all_ns_bound'])
    expected_exceptions = set([
        'test_pmedian_none_fixed_cost', 
        'test_pmedian_thresh_fixed_cost', 
        'test_pmedian_thresh_feas_cost', 
        'test_pmedian_thresh_all_cost', 
        'test_pmedian_percent_fixed_cost', 
        'test_pmedian_percent_feas_cost', 
        'test_pmedian_percent_all_cost', 
        'test_pmedian_none_feas_cost', 
        'test_pmedian_none_all_cost',
        'test_pmedian_thresh_fixed_ns', 
        'test_pmedian_thresh_feas_ns', 
        'test_pmedian_thresh_all_ns', 
        'test_pmedian_percent_fixed_ns', 
        'test_pmedian_percent_feas_ns', 
        'test_pmedian_percent_all_ns',
        'test_worst_thresh_all_ns',
        'test_worst_none_all_cost'])

    def setUp(self):
        super(TestAttGrasp, self).setUp()
        self.problem.setSolverOption('type', 0, 'att_grasp')

    #
    # Long-term these should be executed from the new solver interface.  But for now...
    #
    def verify_output(self, fname):
        data = self.parse_output(fname)
        if not data.wst:
            self.fail("Expected a WST solver interface: coopr=%s pyomo=%s" % (str(data.coopr),str(data.pyomo)))
        if data.pyomo:
            self.fail("Expected to use 'none' for the modeling language: coopr=%s pyomo=%s" % (str(data.wst),str(data.pyomo)))


#
# snl_grasp solver, none model
#
class TestSnlGrasp(TestAttGrasp):

    def setUp(self):
        super(TestSnlGrasp, self).setUp()
        self.problem.setSolverOption('type', 0, 'snl_grasp')



class TestMisc(TestBase):

    solver = None
    problem_lang = 'none'
    skip_list = set(['test1b', 'test1s'])

    #
    # Long-term these should be executed from the new solver interface.  But for now...
    #
    def verify_output(self, fname):
        data = self.parse_output(fname)
        if not data.wst:
            self.fail("Expected a WST solver interface: coopr=%s pyomo=%s" % (str(data.wst),str(data.pyomo)))
        if data.pyomo:
            self.fail("Expected to use 'none' for the modeling language: coopr=%s pyomo=%s" % (str(data.wst),str(data.pyomo)))

    def test1b(self):
        #"""sp test1b"""
        #self.execute_test(name="test1b", options="--objective=mc --ub=ns,5 --solver=lagrangian")
        name = "test1b"
        self.problem.setSolverOption('type', 0, 'lagrangian')
        self.execute_test(name, baseline=True, quiet=True)
    
    def test1s(self):
        #"""sp test1s"""
        #self.execute_test(name="test1s", options="--objective=mc --ub=ns,5 --imperfect-jcfile=test1.imperfectjc --imperfect-scfile=test1.imperfectsc --numsamples=5 --solver=heuristic")
        name = "test1s"
        self.problem.setImperfectOption('name', 0, 'imperf1')
        self.problem.setImperfectOption('junction class file', 0, 'test1.imperfectjc')
        self.problem.setImperfectOption('sensor class file', 0, os.path.join(dataDir, 'test1.imperfectsc'))
        #self.problem.setImperfectOption('directory', 0, dataDir)
        self.problem.setProblemOption('imperfect', 0, 'imperf1')
        self.problem.setSolverOption('number of samples', 0, 5)
        self.problem.setSolverOption('type', 0, 'heuristic')
        self.execute_test(name, baseline=True, quiet=True)
    

class TestMiscOld(TestMisc):

    solver = None
    wst_command = 'spold'
    shellcmd = True
    skip_list = set(['test1b', 'test1s'])

    def verify_output(self, fname):
        data = self.parse_output(fname)
        self.data = None
        if data.wst:
            self.fail("Expected a the sp script interface: coopr=%s pyomo=%s" % (str(data.wst),str(data.pyomo)))



#
# cplex solver, pyomo model
#
@unittest.category('nightly')
@unittest.skipIf(not cplex_available, "The 'cplex' executable is not available")
class TestCplexPyomo(PyomoTester, TestBase):

    solver = 'cplex'


#
# xpress solver, pyomo model
#
@unittest.category('nightly')
@unittest.skipIf(not xpress_available, "The 'xpress' executable is not available")
class TestXpressPyomo(PyomoTester, TestBase):

    solver = 'xpress'


#
# gurobi solver, pyomo model
#
@unittest.category('nightly')
@unittest.skipIf(not gurobi_available, "The 'gurobi' executable is not available")
class TestGurobiPyomo(PyomoTester, TestBase):

    solver = 'gurobi'


#
# glpk solver, pyomo model
#
@unittest.category('nightly')
@unittest.skipIf(not glpk_available, "The 'glpk' executable is not available")
class TestGlpkPyomo(PyomoTester, TestBase):

    solver = 'glpk'


#
# cbc solver, pyomo model
#
@unittest.category('nightly')
@unittest.skipIf(not cbc_available, "The 'cbc' executable is not available")
class TestCbcPyomo(PyomoTester, TestBase):

    solver = 'cbc'

    skip_list = set([
        'test_pmedian_none_all_ns_bound'])

#
# pico solver, ampl model
#
@unittest.skipIf(not pico_available, "The 'pico' executable is not available")
class TestPicoAmpl(PyomoTester, TestBase):

    solver = 'pico'
    problem_lang = 'ampl'

    def verify_output(self, fname):
        data = self.parse_output(fname)
        self.data = None
        if not data.wst or data.pyomo:
            self.fail("Expected a Coopr solver with an AMPL model: coopr=%s pyomo=%s" % (str(data.wst),str(data.pyomo)))


#
# pico solver, ampl model
# SPOLD
#
@unittest.skipIf(not pico_available, "The 'pico' executable is not available")
class TestMipSp(PyomoTester, TestBase):

    wst_command = 'spold'
    solver = 'pico'
    shellcmd = True

    def verify_output(self, fname):
        data = self.parse_output(fname)
        self.data = None
        if data.wst:
            self.fail("Expected a the sp script interface: coopr=%s pyomo=%s" % (str(data.wst),str(data.pyomo)))


#
# ef solver, pyomo model, pico subsolver
#
class TestEFSingleStage(PyomoTester, TestBase):

    solver='ef'

    skip_list = set(['test_pmedian_none_all_ns_bound',
        'test_worst_none_all_ns',
        'test_worst_none_infeas_ns',
        'test_worst_thresh_all_ns',
        'test_worst_none_all_cost'])
    expected_exceptions = set([
        'test_pmedian_none_fixed_cost', 
        'test_pmedian_thresh_fixed_cost', 
        'test_pmedian_thresh_feas_cost', 
        'test_pmedian_thresh_all_cost', 
        'test_pmedian_percent_fixed_cost', 
        'test_pmedian_percent_feas_cost', 
        'test_pmedian_percent_all_cost', 
        'test_pmedian_none_feas_cost', 
        'test_pmedian_none_all_cost',
        'test_pmedian_thresh_fixed_ns', 
        'test_pmedian_thresh_feas_ns', 
        'test_pmedian_thresh_all_ns', 
        'test_pmedian_percent_fixed_ns', 
        'test_pmedian_percent_feas_ns', 
        'test_pmedian_percent_all_ns'
        ])


    def setUp(self):
        TestBase.setUp(self)
        self.problem.setSolverOption('type', 0, self.solver)
        self.problem.setProblemOption('type', 0, 'multi-stage')
        self.problem.setConstraintOption('scenario', 'const1', [{'probability':1.0, 'bound':0}])
        #self.problem.setSolverOption('seed',0, 1)

    def verify_output(self, fname):
        data = self.parse_output(fname)
        if not data.wst or not data.pyomo:
            self.fail("Expected a WST solver with Pyomo model: coopr=%s pyomo=%s" % (str(data.wst),str(data.pyomo)))

    def test_scenarios1(self):
        #"""test3a"""
        name = "scenarios1"
        def _other(self):
            data = json.load(open(currdir+name+os.sep+'%s.json' % self.solver,'r'))
            self.assertEqual(data['SecondStage'].keys(), ['Node0'])
        self._aux_tests = _other
        self.execute_test(name, baseline=False, quiet=True)
        
    
    def test_scenarios2(self):
        #"""test3aa - Run test3a with different scenario names"""
        name = "scenarios2"
        def _other(self):
            data = json.load(open(currdir+name+os.sep+'%s.json' % self.solver,'r'))
            self.assertEqual(data['SecondStage'].keys(), ['FOO'])
        self._aux_tests = _other
        self.problem.setConstraintOption('scenario', 'const1', [{'name':'FOO', 'probability':1.0, 'bound':0}])
        self.execute_test(name, baseline=False, quiet=True)
    

#
# ph solver, pyomo model, pico subsolver
#
class TestPHSingleStage(TestEFSingleStage):

    solver='ph'


#@unittest.category('nightly')
#class TestStudy(TestBase):
class Study(object):

    # Replicate tests in test_objectives.py

    def test_detected(self):
        #""" sp test objectives with nfd"""
        name = "test_detected"
        solvers = ["pico"]
        nfds = [1.0, 0.9, 0.7, 0.5]
        for obj in ["dec", "dtd", "dmc", "dnfd", "dpe", "dpk", "dpd", "dvc"]:
            print ""
            print "Objective: "+obj
            print "---------------------"
            data = {}
            for solver in solvers:
                for nfd in nfds:
                    #data[(solver,nfd)] = self.execute_test(name="test1f", options="--objective="+obj+" --ub=ns,5 --solver="+solver+" --ub=nfd,"+str(nfd), baseline=False, quiet=False)
                    impactFile = 'test1_'+obj+'.impact'
                    #impactDir = self.problem.getImpactOption('directory', 0)
                    self.problem.setImpactOption('impact file', 0, impactFile)
                    self.problem.setConstraintOption('name', 1, 'const2')
                    self.problem.setConstraintOption('goal', 1, 'NFD')
                    self.problem.setConstraintOption('statistic', 1, 'TOTAL')
                    self.problem.setConstraintOption('bound', 1, nfd)
                    self.problem.setConfigureOption('constraint', ['const1', 'const2'])
                    self.problem.setSolverOption('type', 0, solver)
                    data[(solver,nfd)] = self.execute_test(name, baseline=False, quiet=True)
                    keys = data[(solver,nfd)].keys()
                    if "Mean impact" in data[(solver,nfd)][keys[len(keys)-1]]:
                        print solver,nfd,data[(solver,nfd)][keys[len(keys)-1]]["Mean impact"]
                    else:
                        print solver,nfd,"error"
                print ""

    # Replicate tests in test_sensors.py
    def sensor_study(self, statistic):
        #""" sp test sensors"""
        name = "test_"+statistic
        print "------------------------------------------------------------"
        print " Analyzing Solvers for "+statistic+" statistic"
        print "------------------------------------------------------------"
        if statistic == "WORST":
            solvers = ["pico", "att_grasp", "snl_grasp"]
        else:
            solvers = ["pico", "att_grasp", "snl_grasp", "lagrangian"]
        sensors = [1,2,3,4,5,10,20]
        for obj in ["ec", "td", "mc", "nfd", "pe", "pk", "pd", "vc"]:
            print ""
            print "Objective: ",obj
            print "---------------------"
            data = {}
            for solver in solvers:
                for ns in sensors:
                    #data[(solver,ns)] = self.execute_test(name="test1f", options="--objective="+obj+" --ub=ns,"+str(ns)+" --solver="+solver, baseline=False, quiet=True)
                    impactFile = 'test1_'+obj+'.impact'
                    self.problem.setImpactOption('impact file', 0, impactFile)
                    self.problem.setObjectiveOption('statistic', 0, statistic)
                    self.problem.setConstraintOption('bound', 0, ns)
                    self.problem.setSolverOption('type', 0, solver)
                    data[(solver,ns)] = self.execute_test(name, baseline=False, quiet=True)
                    keys = data[(solver,ns)].keys()
                    if "Mean impact" in data[(solver,ns)][keys[len(keys)-1]]:
                        print solver,ns,data[(solver,ns)][keys[len(keys)-1]]["Mean impact"]
                    else:
                        print solver,ns,"error"
                print ""
            #
            # Checks
            #
            tmp = copy.copy(solvers)
            tmp.remove("pico")
            for solver in tmp:
                for ns in sensors:
                    keys = data[(solver,ns)].keys()
                    if not "Mean impact" in data[(solver,ns)][keys[len(keys)-1]]:
                        print "ERROR in the following test:"
                        #self.execute_test(name="test1f", options="--objective="+obj+" --ub=ns,"+   str(ns)+" --solver="+solver, baseline=False, quiet=False)
                        impactFile = 'test1_'+obj+'.impact'
                        self.problem.setImpactOption('impact file', 0, impactFile)
                        self.problem.setObjectiveOption('statistic', 0, statistic)
                        self.problem.setConstraintOption('bound', 0, ns)
                        self.problem.setSolverOption('type', 0, solver)
                        self.execute_test(name, baseline=False, quiet=True)
                    elif data[(solver,ns)][keys[len(keys)-1]]["Mean impact"] < data[("pico",ns)][keys[len(keys)-1]]["Mean impact"]:
                        print "WARNING: solver "+solver+" appears to have generated better values than solver pico when using "+str(ns)+" sensors."
            #
            for i in range(1,len(sensors)):
                for solver in solvers:
                    if "Mean impact" in data[(solver,sensors[i])][keys[len(keys)-1]] and "Mean impact" in data[(solver,sensors[i-1])] and data[(solver,sensors[i])]["Mean impact"] > data[(solver,sensors[i-1])]["Mean impact"]:
                        print "WARNING: solver "+solver+" appears to have generated better values with "+str(sensors[i])+" sensors than with "+str(sensors[i-1])+" sensors"

    def test_mean(self):
        self.sensor_study("MEAN")
            
    def test_worst(self):
        self.sensor_study("WORST")
    
    

if __name__ == "__main__":
    unittest.main()
