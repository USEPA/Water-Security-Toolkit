import os
import shutil
import sys
from os.path import abspath, dirname, join, basename
import pyutilib.th as unittest
from nose.tools import nottest
import pywst.booster_mip.problem as problem
from pywst.common import pyunit
import yaml

testDir = dirname(abspath(__file__))
if not pyunit.dataroot is None:
    dataDir = join(pyunit.dataroot,'booster_mip')
else:
    dataDir = join(testDir,'data') 

def parse_impact_file(filename):
    max_impacts = []
    with open(filename) as f:
        lines = f.readlines()
        for line in lines[2:]:
            line = line.strip().split()
            if line[1] == '-1':
                max_impacts.append(float(line[3]))
    return max_impacts

class Test(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        self.problem = problem.Problem()
        self.default_opts = self.problem.opts.value()

    @classmethod
    def tearDownClass(self):
        pass

    def setUp(self):
        self.origDir = os.getcwd()
        os.chdir(testDir)
        self.problem.opts.set_value(self.default_opts)
        self.problem.opts['configure']['output prefix'] = self.gen_test_dir()
        # This must be done after the output prefix has been set
        self.problem.setLogger('booster_mip')
        
    def tearDown(self):
        os.chdir(self.origDir)

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

    @nottest
    def _validate_booster_mip_results(self, results):
        
        detected = results['detected scenarios']
        non_detected = results['non-detected scenarios']
        non_discarded = results['all non-discarded scenarios']
        for val1, val2 in zip(detected['mass consumed grams'],detected['mass injected grams']):
            self.assertTrue(abs(val1-val2) < 0.0001)
        for val1, val2 in zip(detected['mass consumed grams'],detected['response window mass injected grams']):
            self.assertTrue(abs(val1-val2) < 0.0001)
        for val in detected['tank mass grams']:
            self.assertEqual(val,0.0)
        for val in detected['pre-booster mass consumed grams']:
            self.assertEqual(val, 0.0)
        self.assertEqual(results['booster nodes'],[])
        self.assertEqual(detected['count'], non_discarded['count'])
        self.assertEqual(non_detected['count'], 0)
        for val in non_detected['mass injected grams']:
            self.assertEqual(val,0.0)
        self.assertEqual(non_detected['expected mass consumed grams'], 0.0)

    def _generate_impacts(self, tsg_file, inp_file, prefix, impact, tai_file="", merlion=False):

        cmd = 'tevasim --tsg '+tsg_file+' '
        if merlion:
            cmd += '--merlion --merlion-ignore-warnings '
        cmd += inp_file+' '+prefix+'TEVA'+' '+prefix+'TEVA'
        os.system(cmd)

        os.system('tso2Impact --'+impact+' '+prefix+'TEVA'+' '+prefix+'TEVA'+' '+tai_file)

        impact_file = prefix+'TEVA_'+impact+'.impact'
        max_impacts = parse_impact_file(impact_file)
        return max_impacts
        
    @unittest.nottest
    def _test(self, pref_file, model_format, model_type, impact, tai_file=''):
        
        prefix = self.problem.opts['configure']['output prefix']
        self.assertTrue(self.problem.load(pref_file))
        self.problem.opts['booster mip']['model format'] = model_format
        self.problem.opts['booster mip']['model type'] = model_type
        tsg_file = self.problem.opts['scenario']['tsg file']
        inp_file = self.problem.opts['network']['epanet file']
        self.problem.opts['configure']['output prefix'] = prefix
        sol_fnames = self.problem.run()
        self.assertTrue(len(sol_fnames) == 1)
        with open(sol_fnames[0]) as f:
            booster_sol = yaml.load(f)
        
        #self._validate_booster_mip_results(booster_sol)

        # if 'mc' units are mg
        max_impacts_epanet = \
            self._generate_impacts(tsg_file, inp_file, prefix, impact, tai_file=tai_file, merlion=False)
        # if 'mc' units are mg
        max_impacts_merlion = \
            self._generate_impacts(tsg_file, inp_file, prefix, impact, tai_file=tai_file, merlion=True)
        
        booster_mass_consumed_mg = [val_g*1000.0 for val_g in \
                                    booster_sol['detected scenarios']['mass consumed grams']]
        if impact == 'mc':
            booster_impact = booster_mass_consumed_mg
        elif impact == 'pd':
            booster_impact = booster_sol['detected scenarios']['population dosed']
        else:
            raise ValueError
        print 'Booster', booster_impact
        print 'TevaEPA', max_impacts_epanet
        print 'TevaMER', max_impacts_merlion
        for booster, epanet, merlion in zip(booster_impact, max_impacts_epanet, max_impacts_merlion):
            self.assertAlmostEqual(booster, epanet, places=2)
            self.assertAlmostEqual(booster, merlion, places=2)
        self.cleanup(self.problem.opts['configure']['output prefix'])

    def test1_PYOMO_NEUTRAL(self):
        pref_file = join(testDir, 'test1.yml')
        self._test(pref_file,'PYOMO','NEUTRAL','mc')

    def test1_AMPL_NEUTRAL(self):
        pref_file = join(testDir, 'test1.yml')
        self._test(pref_file,'AMPL','NEUTRAL','mc')

    def test2_PYOMO_LIMIT(self):
        pref_file = join(testDir, 'test2.yml')
        self._test(pref_file,'PYOMO','LIMIT','mc')

    def test2_AMPL_LIMIT(self):
        pref_file = join(testDir, 'test2.yml')
        self._test(pref_file,'AMPL','LIMIT','mc')

    def test3_PYOMO_LIMIT(self):
        pref_file = join(testDir, 'test3.yml')
        self._test(pref_file,'PYOMO','LIMIT','pd', tai_file='bio.tai')

    def test3_AMPL_LIMIT(self):
        pref_file = join(testDir, 'test3.yml')
        self._test(pref_file,'AMPL','LIMIT','pd', tai_file='bio.tai')

    def test4_PYOMO_LIMIT(self):
        pref_file = join(testDir, 'test4.yml')
        self._test(pref_file,'PYOMO','LIMIT','pd', tai_file='bio.tai')

    def test4_AMPL_LIMIT(self):
        pref_file = join(testDir, 'test4.yml')
        self._test(pref_file,'AMPL','LIMIT','pd', tai_file='bio.tai')

if __name__ == "__main__":
    unittest.main()

        
