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
import zipfile
from os.path import join
import os

if len(sys.argv) < 3:
   print "unzip.py <zip-file> <dir>"
   sys.exit(0)

def unzip(filename, dir=None):
    fname = os.path.abspath(filename)
    zf = zipfile.ZipFile(fname, 'r')
    if not dir is None:
        cwd = os.getcwd()
        os.chdir(dir)
    #zf.extractall()
    for name in zf.namelist():
        data = zf.read(name)
        OUTPUT = open(name,'wb')
        OUTPUT.write(data)
        OUTPUT.close()
    if not dir is None:
        os.chdir(cwd)
    zf.close()

unzip(sys.argv[1],sys.argv[2])
