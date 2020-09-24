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
# Perform functionality test for evalsensor
#

import os
import sys
from os.path import abspath, dirname, join, normpath
#sys.path.insert(0, dirname(dirname(abspath(__file__)))+os.sep+".."+os.sep+"..")
currdir = dirname(abspath(__file__))

import unittest
import pyutilib.subprocess
import pyutilib.th
from pywst.spot import pyunit, parse_sp
from nose.tools import nottest

basedir = normpath(join(dirname(abspath(__file__)),'..','..','..','..'))
bindir = join(basedir, 'bin')
netdir = join(pyunit.datadir, "wst", "data", "Net3", 'linux64')

#
# Create unittest testing class
#
# Methods test_* are executed (in an arbitrary order)
# The setUp() method is run before every test.
# The tearDown() method is run after every test.
#
class Test(pyutilib.th.TestCase):

    def create_command(self, options, sensor_file, network, impacts, nodemap):
        """ Create the command line """
        cmd = [ join(bindir,"evalsensor") ]
        for option in options:
            cmd.append(option)
        #if os.path.exists(network+"_"+impact+".prob"):
            #cmd += " --incident-weights " + network+"_"+impact+".impact"
        if nodemap is None:
            cmd.append("--nodemap="+network+".nodemap")
        else:
            cmd.append("--nodemap="+nodemap)
        cmd.append( sensor_file )
        for impact in impacts:
            cmd.append( network+"_"+impact+".impact" )
        return cmd

    @nottest
    def execute_test(self, options, sensor_file, network, impacts, name=None, nodemap=None):
        """ Execute a test given different information """
        cmd = self.create_command(options, sensor_file, network, impacts, nodemap)
        print "Running",cmd
        if name is None:
            name = network
        pyutilib.subprocess.run(cmd, join(currdir,name+".out"),cwd=currdir)
        #
        # Parse the output to get the impact lines that should always be
        # the same.
        #
        parse_sp.process_evalsensors( join(currdir,name+".out"), join(currdir,name+"_parsed.out") )
        self.assertFileEqualsBaseline( join(currdir,name+"_parsed.out"), join(currdir,name+".qa") )
        os.remove(join(currdir, name+".out"))

    @nottest
    def execute_other_test(self, options, sensor_file, network, impacts, name=None, nodemap=None):
        """ Execute a test given different information """
        cmd = self.create_command(options, sensor_file, network, impacts, nodemap)
        print "Running",cmd
        if name is None:
            name = network
        pyutilib.subprocess.run(cmd, join(currdir,name+".out"),cwd=currdir)
        self.assertFileEqualsBaseline(join(currdir,name+".out"), join(currdir,name+".qa") )
        #os.remove(join(currdir,name+".out"))

    def test1(self):
        """ evalsensor tested with no sensors"""
        self.execute_test([],"none",  join(netdir,"Net3_quarterly"), ["mc"], "test1")

    def test2(self):
        """ evalsensor tested with a sensors file"""
        self.execute_test([],"OneSensor.txt",  join(netdir,"Net3_quarterly"), ["mc"], "test2")

    def test3(self):
        """ evalsensor tested with an impact that has skewed weights"""
        self.execute_test([],"none",  join(netdir,"Net3_quarterly"), ["mcWeighted"], "test3")

    def Xtest4(self):
        """ evalsensor tested with XLS output"""
        self.execute_other_test(["--format", "xls"],"OneSensor.txt",  join(netdir,"Net3_quarterly"), 
                                ["mc"], "test4")

if __name__ == "__main__":
    unittest.main()
