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

import copy
import shutil
import unittest
import yaml
import pprint

from nose.tools import nottest

import pyutilib.services
import pyutilib.th as unittest
import pywst.tevasim.problem as problem
from pywst.common import pyunit

testDir = dirname(abspath(__file__))
if pyunit.dataroot is not None:
    dataDir     = join(pyunit.dataroot, 'tevasim', 'data')
    baselineDir = join(pyunit.dataroot, 'tevasim', 'baselines')
    

pyutilib.services.register_executable('wst')

erddiff = pyutilib.services.registered_executable('erddiff')
if not erddiff is None:
    erddiff = erddiff.get_path()
tevasim = pyutilib.services.registered_executable('tevasim')
if not tevasim is None:
    tevasim = tevasim.get_path()
wst = pyutilib.services.registered_executable('wst')
if not wst is None:
    wst = wst.get_path()



#
# Create unittest testing class
#
@unittest.skipIf(pyunit.dataroot is None, "The imput data for these tests was not found (please checkout wst_data")
class Test(unittest.TestCase):

    # This method is called once, before all tests are executed.
    @classmethod
    def setUpClass(self):
        problem.Problem().setLogger('tevasim')

    # This method is called once, after all tests are executed.
    @classmethod
    def tearDownClass(self):
        pass

    # This method is called before each test is executed.
    def setUp(self):
        global erddiff
        if erddiff is None:
            self.skipTest("The 'erddiff' command is not available")
        global tevasim
        if tevasim is None:
            self.skipTest("The 'tevasim' command is not available")
        global wst
        if wst is None:
            self.skipTest("The 'wst' command is not available")
        #
        self.origDir = os.getcwd()
        os.chdir(testDir)
        # 
        self.problem = problem.Problem()
       
        self.problem.opts['network']['epanet file'] = join(dataDir,'Net3.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'Net3.tsg')
        self.problem.opts['scenario']['erd compression'] = 'RLE'

    # This method is called after each test is executed.
    def tearDown(self):
        os.chdir(self.origDir)
    
    # Test integration 
    @nottest
    def clearIntegrationTestTiles(self,basename):
        os.remove(join(testDir,basename+'.erd'))
        os.remove(join(testDir,basename+'-1.qual.erd'))
        os.remove(join(testDir,basename+'-1.hyd.erd'))
        os.remove(join(testDir,basename+'.index.erd'))
        os.remove(join(testDir,basename+'.rpt'))
        try:
            os.remove(join(testDir,'hydraulics.hyd'))
        except: OSError
        try:
            os.remove(join(testDir,'stats.yml'))
        except: OSError
        try:
            os.remove(join(testDir,'test.yml'))
        except: OSError
        
    def testCompareMerlionEpanetNet1(self):
        #AbsErrTol = 1e-8 # Not used in this test
        RelRMSErrTol = 0.5      
        
        network_name = 'Net1'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline.tsg')
        
        # run with merlion
        erdMer = 'comp_'+network_name+'_5min_merlion'
        self.problem.opts['configure']['output prefix'] = erdMer
        self.problem.opts['scenario']['merlion'] = True
        self.problem.run()
        
        # run with epanet
        erdEpa = 'comp_'+network_name+'_5min_epanet'
        self.problem.opts['configure']['output prefix'] = erdEpa
        self.problem.opts['scenario']['merlion'] = False
        self.problem.run()
            
        #compare merlion and epanet
        ans = pyutilib.subprocess.run([ erddiff, 
                                        join(testDir,erdMer+'.erd'), 
                                        join(testDir,erdEpa+'.erd'), 
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdMer)
        self.clearIntegrationTestTiles(erdEpa)
        
    def testCompareMerlionEpanetNet2(self):
        #AbsErrTol = 1e-8 # Not used in this test
        RelRMSErrTol = 5.0
        
        network_name = 'Net2'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline.tsg')
        
        # run with merlion
        erdMer = 'comp_'+network_name+'_5min_merlion'
        self.problem.opts['configure']['output prefix'] = erdMer
        self.problem.opts['scenario']['merlion'] = True
        self.problem.run()
        
        # run with epanet
        erdEpa = 'comp_'+network_name+'_5min_epanet'
        self.problem.opts['configure']['output prefix'] = erdEpa
        self.problem.opts['scenario']['merlion'] = False
        self.problem.run()
            
        #compare merlion and epanet
        ans = pyutilib.subprocess.run([ erddiff, 
                                        join(testDir,erdMer+'.erd'),
                                        join(testDir,erdEpa+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdMer)
        self.clearIntegrationTestTiles(erdEpa)
        
    def testCompareMerlionEpanetNet3(self):
        #AbsErrTol = 1e-8 # Not used in this test
        RelRMSErrTol = 50.0    
        
        network_name = 'Net3'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline.tsg')
        
        # run with merlion
        erdMer = 'comp_'+network_name+'_5min_merlion'
        self.problem.opts['configure']['output prefix'] = erdMer
        self.problem.opts['scenario']['merlion'] = True
        self.problem.run()
        
        # run with epanet
        erdEpa = 'comp_'+network_name+'_5min_epanet'
        self.problem.opts['configure']['output prefix'] = erdEpa
        self.problem.opts['scenario']['merlion'] = False
        self.problem.run()
            
        #compare merlion and epanet
        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdMer+'.erd'),
                                        join(testDir,erdEpa+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdMer)
        self.clearIntegrationTestTiles(erdEpa)

    def test_TevasimMerlionNet1_single_rhs(self):
        AbsErrTol = 0.1
        RelRMSErrTol = 1e-3
        
        network_name = 'Net1'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline.tsg')
        erdName = network_name+'_5min_merlion'
        self.problem.opts['configure']['output prefix'] = erdName
        self.problem.opts['scenario']['merlion'] = True
        self.problem.opts['scenario']['merlion nsims'] = 1
        self.problem.run()
        
        # check against merlion baseline
        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdName+'.erd'),
                                        join(baselineDir,'baseline_'+erdName+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['max abse']['Value'], AbsErrTol)
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdName)

    def test_TevasimMerlionNet1(self):
        AbsErrTol = 0.1
        RelRMSErrTol = 1e-3
        
        network_name = 'Net1'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline.tsg')
        erdName = network_name+'_5min_merlion'
        self.problem.opts['configure']['output prefix'] = erdName
        self.problem.opts['scenario']['merlion'] = True
        self.problem.run()
        
        # check against merlion baseline
        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdName+'.erd'),
                                        join(baselineDir,'baseline_'+erdName+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['max abse']['Value'], AbsErrTol)
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdName)
        
    def test_TevasimEpanetNet1(self):
        AbsErrTol = 0.1
        RelRMSErrTol = 1e-3
        
        network_name = 'Net1'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline.tsg')
        erdName = network_name+'_5min_epanet'
        self.problem.opts['configure']['output prefix'] = erdName
        self.problem.opts['scenario']['merlion'] = False
        self.problem.run()
        
        # check against merlion baseline
        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdName+'.erd'),
                                        join(baselineDir,'baseline_'+erdName+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['max abse']['Value'], AbsErrTol)
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdName)
        
    def test_TevasimMerlionNet2(self):
        AbsErrTol = 0.8
        RelRMSErrTol = 1e-2
        
        network_name = 'Net2'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline.tsg')
        erdName = network_name+'_5min_merlion'
        self.problem.opts['configure']['output prefix'] = erdName
        self.problem.opts['scenario']['merlion'] = True
        self.problem.opts['scenario']['merlion nsims'] = 20
        self.problem.run()
        
        # check against merlion baseline
        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdName+'.erd'),
                                        join(baselineDir,'baseline_'+erdName+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['max abse']['Value'], AbsErrTol)
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdName)
        
    def test_TevasimEpanetNet2(self):
        AbsErrTol = 0.1
        RelRMSErrTol = 1e-3
        
        network_name = 'Net2'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline.tsg')
        erdName = network_name+'_5min_epanet'
        self.problem.opts['configure']['output prefix'] = erdName
        self.problem.opts['scenario']['merlion'] = False
        self.problem.run()
        
        # check against merlion baseline
        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdName+'.erd'),
                                        join(baselineDir,'baseline_'+erdName+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['max abse']['Value'], AbsErrTol)
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdName)
        
    def test_TevasimMerlionNet3(self):
        AbsErrTol = 1
        RelRMSErrTol = 1
        
        network_name = 'Net3'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline.tsg')
        erdName = network_name+'_5min_merlion'
        self.problem.opts['configure']['output prefix'] = erdName
        self.problem.opts['scenario']['merlion'] = True
        self.problem.run()
        
        # check against merlion baseline
        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdName+'.erd'),
                                        join(baselineDir,'baseline_'+erdName+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['max abse']['Value'], AbsErrTol)
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdName)
        
    def test_TevasimEpanetNet3(self):
        AbsErrTol = 0.1
        RelRMSErrTol = 1e-3
        
        network_name = 'Net3'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline.tsg')
        erdName = network_name+'_5min_epanet'
        self.problem.opts['configure']['output prefix'] = erdName
        self.problem.opts['scenario']['merlion'] = False
        self.problem.run()
        
        # check against merlion baseline
        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdName+'.erd'),
                                        join(baselineDir,'baseline_'+erdName+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['max abse']['Value'], AbsErrTol)
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdName)
        
    @unittest.category('64bit')
    def test_TevasimMerlion13000(self):
        AbsErrTol = 0.1
        RelRMSErrTol = 1e-3
        
        network_name = '13000'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline_13000.tsg')
        erdName = network_name+'_5min_merlion'
        self.problem.opts['configure']['output prefix'] = erdName
        self.problem.opts['scenario']['merlion'] = True
        self.problem.run()
        
        # check against merlion baseline
        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdName+'.erd'),
                                        join(baselineDir,'baseline_'+erdName+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['max abse']['Value'], AbsErrTol)
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdName)
        
    def test_TevasimEpanet13000(self):
        AbsErrTol = 1.0
        RelRMSErrTol = 1e-2
        
        network_name = '13000'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline_13000.tsg')
        erdName = network_name+'_5min_epanet'
        self.problem.opts['configure']['output prefix'] = erdName
        self.problem.opts['scenario']['merlion'] = False
        self.problem.run()
        
        # check against merlion baseline
        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdName+'.erd'),
                                        join(baselineDir,'baseline_'+erdName+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['max abse']['Value'], AbsErrTol)
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdName)
    
    def test_dvfCase(self):
        AbsErrTol = 0.1
        RelRMSErrTol = 0.1
        
        erdName = 'wst_dvfCase'
        self.problem.opts['configure']['output prefix'] = erdName
        self.problem.opts['scenario']['dvf file'] = join(dataDir,'Net3.dvf')
        self.problem.opts['scenario']['erd compression'] = 'RLE'
        #self.problem.saveAll(join(testDir,'test.yml'))
        #fid = open(join(testDir,'test.yml'), 'w')
        #fid.write(self.problem.opts.generate_yaml_template())
        #fid.close()
        #pyutilib.subprocess.run(wst+' tevasim '+join(testDir,'test.yml'))
        self.problem.run()

        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdName+'.erd'),
                                        join(baselineDir,'qa_dvfCase.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
        #    self.assertLessEqual(stat['species species']['max abse']['Value'], AbsErrTol)
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdName)
        
    def testCompareMerlionEpanetNet1withDecay(self):
        #AbsErrTol = 1e-8 # Not used in this test
        RelRMSErrTol = 0.5      
        
        network_name = 'Net1_with_decay'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline.tsg')
        
        # run with merlion
        erdMer = 'comp_'+network_name+'_5min_merlion'
        self.problem.opts['configure']['output prefix'] = erdMer
        self.problem.opts['scenario']['merlion'] = True
        self.problem.run()
        
        # run with epanet
        erdEpa = 'comp_'+network_name+'_5min_epanet'
        self.problem.opts['configure']['output prefix'] = erdEpa
        self.problem.opts['scenario']['merlion'] = False
        self.problem.run()
            
        #compare merlion and epanet
        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdMer+'.erd'),
                                        join(testDir,erdEpa+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdMer)
        self.clearIntegrationTestTiles(erdEpa)
        
    def testCompareMerlionEpanetNet3withDecay(self):
        #AbsErrTol = 1e-8 # Not used in this test
        RelRMSErrTol = 50.0    
        
        network_name = 'Net3_with_decay'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline.tsg')
        
        # run with merlion
        erdMer = 'comp_'+network_name+'_5min_merlion'
        self.problem.opts['configure']['output prefix'] = erdMer
        self.problem.opts['scenario']['merlion'] = True
        self.problem.run()
        
        # run with epanet
        erdEpa = 'comp_'+network_name+'_5min_epanet'
        self.problem.opts['configure']['output prefix'] = erdEpa
        self.problem.opts['scenario']['merlion'] = False
        self.problem.run()
            
        #compare merlion and epanet
        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdMer+'.erd'),
                                        join(testDir,erdEpa+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdMer)
        self.clearIntegrationTestTiles(erdEpa)

    def test_TevasimMerlionNet3withDecay(self):
        AbsErrTol = 0.1
        RelRMSErrTol = 1e-3
        
        network_name = 'Net3_with_decay'
        self.problem.opts['network']['epanet file'] = join(dataDir,'baseline_'+network_name+'.inp')
        self.problem.opts['scenario']['tsg file'] = join(dataDir,'baseline.tsg')
        erdName = network_name+'_5min_merlion'
        self.problem.opts['configure']['output prefix'] = erdName
        self.problem.opts['scenario']['merlion'] = True
        self.problem.run()
        
        # check against merlion baseline
        ans = pyutilib.subprocess.run([ erddiff,
                                        join(testDir,erdName+'.erd'),
                                        join(baselineDir,'baseline_'+erdName+'.erd'),
                                        '--yaml', '--strict' ])
        self.assertEqual(ans[0], 0)
        
        f = open(join(testDir,'stats.yml'),'r')
        stats = yaml.load(f)
        f.close()
        
        self.assertFalse(len(stats)==0)
        
        for stat in stats:
            self.assertLessEqual(stat['species species']['max abse']['Value'], AbsErrTol)
            self.assertLessEqual(stat['species species']['rmse'], RelRMSErrTol)
        
        self.clearIntegrationTestTiles(erdName)

    """
    def test_msxCase(self):
        self.problem.opts['scenario']['msx file'] = 'data/bio.msx'
        self.problem.opts['scenario']['msx species'] = 'BIO'
        self.problem.saveAll('test.yml')
        pyutilib.subprocess.run([wst, 'tevasim', 'test.yml'])
        # erddiff does not work with msx output
        self.assertFileEqualsBaseline('wst.erd', 'qa_msxCase.erd')
    """

if __name__ == "__main__":
    unittest.main()
