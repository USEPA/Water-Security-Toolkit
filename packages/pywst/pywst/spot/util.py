#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

def print_memmon_log(cmd, logfile):
    try:
        f = open(logfile, "r")
    except IOError:
        print cmd+" failure"
        sys.exit(1)

    for line in f:
        if 'memmon:' in line:
            print cmd+" "+line
    f.close()

