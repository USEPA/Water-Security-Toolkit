#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

import sys
from sys import stdin, stdout, stderr
from optparse import OptionParser

# Function to flag when an intitial detection is reported.
def __init_run(option, opt_str, value, parser):
    parser.values.init = True
    parser.values.ename = parser.rargs[0].lower()

def get_sm_parser( ):
    # Create options
    parser = OptionParser()
    parser.add_option('--number-of-sample-teams', '-n',
                      dest = 'nodes',
                      default = 6)
    parser.add_option('--possible-scenarios-file','-P',
                      dest = 'psfileout',
                      default = None)
    parser.add_option('--output-file','-o',
                      dest = 'pnfileout',
                      default = None)
    parser.add_option('--sample-results','-R',
                      dest = 'resfileout',
                      default = None)
    parser.add_option('--eventdata-file','-E',
                      dest = 'timedat',
                      default = None)
    parser.add_option('--nodelist-file','-N',
                      dest = 'nlibfile',
                      default = None)
    parser.add_option('--scenariolist-file','-S',
                      dest = 'slibfile',
                      default = None)
    parser.add_option('--interactive','-i',
                      dest = 'interactive',
                      default = False,
                      action = "store_true")
    return parser

