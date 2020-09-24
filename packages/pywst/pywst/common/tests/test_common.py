#  _________________________________________________________________________
#
#  Water Security Toolkit (WST)
#  Copyright (c) 2012 Sandia Corporation.
#  This software is distributed under the Revised BSD License.
#  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive 
#  license for use of this work by or on behalf of the U.S. government.
#  For more information, see the License Notice in the WST User Manual.
#  _________________________________________________________________________

#
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
import pywst.common.wst_util as wst_util

import pywst.flushing.problem as problem

testDir = dirname(abspath(__file__))
wstRoot = join(testDir, *('..',)*5)
dataDir = join(wstRoot,'examples','Net3')

try:
    import pyepanet
except ImportError:
    pass
    
wst = join(wstRoot, 'python', 'bin', 'wst')
    
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
        
        try:
            self.enData = pyepanet.ENepanet()
            self.enData.ENopen(join(dataDir,'Net3.inp'),'tmp.rpt')
        except:
            raise RuntimeError("EPANET inp file not loaded using pyepanet")
        
    # This method is called after each test is executed.  
    def tearDown(self):
        self.enData.ENclose()
        os.chdir(self.origDir)
    
    # Test functionality
    def test_writeTSG(self):
        location = ['105']   
        type = 'MASS' 	
        species = 'BIO' 	
        strength = 2.42E7
        start_time = 0
        stop_time = 86400/60
        tsg_filename = 'test.tsg'
        wst_util.write_tsg(location, type, species, strength, start_time, stop_time, tsg_filename)
        
        fid = open(tsg_filename,'r')
        output = fid.read()
        self.assertEqual('105 MASS BIO 24200000.0 0 86400\n', output)
        
        location = ['105 110']   
        type = 'FLOWPACED' 	
        species = '' 	
        strength = 100
        start_time = 3600/60
        stop_time = 86000/60
        tsg_filename = 'test.tsg'
        wst_util.write_tsg(location, type, species, strength, start_time, stop_time, tsg_filename)
        
        fid = open(tsg_filename,'r')
        output = fid.read()
        self.assertEqual('105 110 FLOWPACED 100 3600 85980\n', output)
        
    def test_feasibleNodes(self):
        Net3_nzd_nodes = ['15','35','101','103','105','107','109','111','113',
                          '115','117','119','121','123','125','127','131','139',
                          '141','143','145','147','149','151','153','157','159',
                          '161','163','166','167','171','177','185','189','191',
                          '193','197','199','201','203','205','207','209','211',
                          '213','215','217','219','225','229','231','237','239',
                          '243','247','251','253','255']
        feasible = 'NZD'
        infeasible = Net3_nzd_nodes[::2] # pull out every other entry
        max_nodes = -1
        [node_names, node_indices] = wst_util.feasible_nodes(feasible, infeasible, max_nodes, self.enData)

        self.assertEqual(node_names,Net3_nzd_nodes[1::2])
        
    def test_feasibleLinks(self):
        Net3_14to18inch_pipes = ['101','103','109','137','180','181','243',
                                 '245','247','249','251'];
        feasible = 'DIAM 14 18'
        infeasible = 'NONE'
        max_pipes = -1
        [link_names, link_indices] = wst_util.feasible_links(feasible, infeasible, max_pipes, self.enData)

        self.assertEqual(link_names,Net3_14to18inch_pipes)

        
if __name__ == "__main__":
    unittest.main()
