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
# Set Python path so Coopr can be imported
#
import os
import sys
from inspect import getfile, currentframe
from os.path import abspath, dirname, exists, normpath

def find_topdir():
    curr = abspath(dirname( getfile( currentframe() ) ))
    while os.sep in curr:
        if os.path.exists(curr+os.sep+"etc"+os.sep+"mod"):
            os.environ["PATH"] = os.path.join(curr,"python","bin")+os.pathsep+os.environ["PATH"]
            os.environ["PATH"] = os.path.join(curr,"bin")+os.pathsep+os.environ["PATH"]
            return curr
        if os.path.basename(curr) == "":
            return None
        curr = os.path.dirname(curr)


topdir=os.environ.get('WST_ROOT', find_topdir())
if topdir is None:
    raise ValueError, "Cannot find the top-level WST directory!"
moddir=os.sep.join([topdir,"etc","mod"""])
if os.path.exists( os.sep.join([topdir,"packages"]) ):
    testdir=os.sep.join([topdir,"packages","test",""])
else:
    testdir=os.sep.join([topdir,"test",""])

if 'WST_DATA' in os.environ:
    dataroot = normpath(os.environ['WST_DATA'])
else:
    dataroot = normpath(os.path.join(os.path.dirname(topdir),'wst_data'))
if not os.path.exists(dataroot):
    dataroot = None

if dataroot is None:
    datadir = None
else:                            
    datadir=os.path.join(os.path.dirname(topdir),'wst_data')


import pyutilib.misc
import platform

def nbits():
    systemType = platform.system() # Linux, Microsoft, CYGWIN_NT-5.2-WOW64
    if systemType.find("Linux") >= 0:
        wordSize = platform.architecture('/bin/ls')[0]   # 64bit, 32bit
        if wordSize.find('64') >= 0:
            return 64
    else:
        import ctypes, sys
        i = ctypes.c_int()
        kernel32 = ctypes.windll.kernel32
        process = kernel32.GetCurrentProcess()
        kernel32.IsWow64Process(process, ctypes.byref(i))
        if i.value == 0:
            return 64
    return 32

def system_type():
  wordSize = platform.architecture()[0]   # 64bit, 32bit
  systemType = platform.system()          # Linux, Microsoft, CYGWIN_NT-5.2-WOW64

  if 'WST_PLATFORM' in os.environ:
    return os.environ['WST_PLATFORM']

  if systemType.find("Linux") >= 0:
    s = "linux"
  else:
    s = "win"

  nb = nbits()

  return s+str(nb)

