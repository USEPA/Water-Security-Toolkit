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
# Perform acceptance tests for pico
#
#  TODO  If PICO gets a different answer, but the answer is better,
#   print a warning to update the test, don't treat it as an error

import os
import sys

######################################################
# Path variables were set by test/runtests.py
#
sys.path.insert(0, ".."+os.sep+"..")
import path
pydirs=path.PYTHON_DIRECTORIES
spotbin=path.SPOT_BIN+os.sep

dirs = pydirs.split(";")
for dir in dirs:
  sys.path.insert(0, dir)

currdir = os.path.dirname(os.path.abspath(__file__)) + os.sep
#
#
######################################################

import fnmatch
import unittest
import pyutilib.th
import pyutilib.subprocess
from pywst.spot import atests

#
# Create unittest testing class
#

class Test_pico(pyutilib.th.TestCase):

    systemType = ""
    outdir = ""
    #args="--debug=1 --lpType=clp   --RRTrialsPerCall=8 --RRDepthThreshold=-1 --feasibilityPump=false --usingCuts=true"
    args="--debug=1 --lpType=clp   --feasibilityPump=false --usingCuts=true"

    def setUp(self):
        """ Set up to occur before start of each test """

        self.systemType = atests.system_type()
        self.outdir = currdir+"output"+os.sep


    def run_model(self,model):
      cmd = spotbin+"PICO "+self.args+" input/test_"+model+".mps"
      pyutilib.subprocess.run(cmd, currdir+"test_pico_"+model+".out", currdir)

      self.assertFileEqualsBaseline(
        currdir+"test1.sol.txt",
        self.outdir+"test_"+model+".solution") 
    
    def test_pico_mc(self):
      # objective mc
      # constraint ub=ns,5
      #
      self.run_model("mc")

    def test_pico_mc_costs(self):
      # objective mc
      # constraint ub=ns,20
      # costs found in test1/test1-id-costs
      #
      self.run_model("mc_costs")

    def test_pico_ec_threshold(self):
      # objective ec
      # constraint ub=ns,5
      # threshold=ec,20000.0
      #
      self.run_model("ec_threshold")

    def Xtest_pico_mc_worst(self):   ## This test takes too long to run
      # objective mc_worst
      # constraint ub=ns,5
      #
      self.run_model("mc_worst")

    def Xtest_pico_mc_cvar(self):   ## test takes too long
      # objective mc_cvar
      # constraint ub=ns,5
      #
      self.run_model("mc_cvar")

    def test_pico_mc_sensor_locations(self):  
      # objective mc
      # constraint ub=ns,5
      # sensor-locations = test1-sensor-locations
      #
      self.run_model("mc_sensor_locations")

    def test_pico_mc_fixed_locations(self):  
      # objective mc
      # constraint ub=ns,5
      # sensor-locations = test1-fixed-locations
      # seed=1
      #
      self.run_model("mc_fixed_locations")

    def test_pico_mc_valid_locations(self):  
      # objective mc
      # constraint ub=ns,5
      # sensor-locations = test1-valid-locations
      # seed=1
      #
      self.run_model("mc_valid_locations")

    def tearDown(self):
      for file in os.listdir('.'):
        if fnmatch.fnmatch(file, '*.solution.dat'):
          os.remove(file)

def main():
    suite = unittest.TestLoader().loadTestsFromTestCase(Test_pico)
    unittest.TextTestRunner(verbosity=2).run(suite)

if __name__ == "__main__":
    unittest.main()

