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
from os.path import abspath, dirname
sys.path.insert(0, dirname(dirname(abspath(__file__)))+os.sep+".."+os.sep+"..")
currdir = dirname(abspath(__file__))+os.sep

import unittest
import pyutilib.subprocess
import pyutilib.th
from pywst.spot import pyunit

test1dir = os.path.join(pyunit.datadir, "test1", '')
netdir = os.path.join(pyunit.datadir, "Net3", '')
modeldir=pyunit.moddir


#
# Create unittest testing class
#

#class Test_tevasim(pyutilib.th.TestCase):
class ignore(object):

    systemType = ""
    outdir = ""
    outsysdir = ""

    def setUp(self):
        """ Set up to occur before start of tests """

        self.systemType = pyunit.system_type()
        self.outdir = currdir+"output"+os.sep
        self.outsysdir = self.outdir+self.systemType+os.sep

    def tearDown(self):
      for file in ["demand.txt", "hyddata.tmp","test1.out", "test1b.out", "test1c.out"]:
        if os.path.exists(file):
          os.remove(file);

    #
    # What should we do with the *.out files (tevasim's stdout/err)
    # Should we save in case of error?
    #
    
    def test1(self):
      cmd = "tevasim --tsg "+test1dir+"test1.tsg --tsi test1.tsi --tso test1.tso "+netdir+"Net3.inp Net3.out"

      #print "Running...",cmd
      pyutilib.subprocess.run(cmd, currdir+"test1.out", currdir)
      #differ = pyunit.compare_Net3_dot_out(currdir+"Net3.out", self.outdir+"Net3.out")
      #self.assert_(differ == False)

      self.assertFileEqualsBinaryFile(
        currdir+"test1.tso",
        self.outsysdir+"test1.tso") 

      self.assertFileEqualsBaseline(
        currdir+"test1.tsi",
        self.outsysdir+"test1.tsi") 

      self.assertFileEqualsBinaryFile(
        currdir+"test1.sdx",
        self.outsysdir+"test1.sdx") 

    def test1b(self):
      cmd = "tevasim --tsg "+test1dir+"test1b.tsg --tsi test1b.tsi --tso test1b.tso "+netdir+"Net3.inp Net3.out"

      print cmd
      pyutilib.subprocess.run(cmd, currdir+"test1b.out", currdir)
      #differ = pyunit.compare_Net3_dot_out(currdir+"Net3.out", self.outdir+"Net3.out")
      #self.assert_(differ == False)

      self.assertFileEqualsBinaryFile(
        currdir+"test1b.tso",
        self.outsysdir+"test1b.tso") 

      self.assertFileEqualsBaseline(
        currdir+"test1b.tsi",
        self.outsysdir+"test1b.tsi") 

      self.assertFileEqualsBinaryFile(
        currdir+"test1b.sdx",
        self.outsysdir+"test1b.sdx") 

    def test1c(self):
      cmd = "tevasim --tsg "+test1dir+"test1c.tsg --tsi test1c.tsi --tso test1c.tso "+netdir+"Net3.inp Net3.out"

      print cmd
      pyutilib.subprocess.run(cmd, currdir+"test1c.out", currdir)
      #differ = pyunit.compare_Net3_dot_out(currdir+"Net3.out", self.outdir+"Net3.out")
      #self.assert_(differ == False)

      self.assertFileEqualsBinaryFile(
        currdir+"test1c.tso",
        self.outsysdir+"test1c.tso") 

      self.assertFileEqualsBaseline(
        currdir+"test1c.tsi",
        self.outsysdir+"test1c.tsi") 

      self.assertFileEqualsBinaryFile(
        currdir+"test1c.sdx",
        self.outsysdir+"test1c.sdx") 
 
if __name__ == "__main__":
    unittest.main()

