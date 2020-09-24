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
# Python functions used by unittest tests of the software components 
# used in the sensor placement problem.
#

#
# Notes on comparing output files to baseline files:
#
#  If the file is a binary file:
#
#     coopr.swtest.failUnlessFileEqualsBinaryFile(self, testfile, baseline)
#
#  You may need 3 binary baseline files, for Linux32, Linux64, and WindowsMingw.
#
#  If the file is a text file, and not very large, the following test will 
#  print the line number of the difference if the files differ:
#
#     coopr.swtest.failUnlessFileEqualsBaseline(self, testfile, baseline)
#
#  If the file is a very large text file (megabytes), reading line by line 
#  to compare can take several minutes or longer on a typical machine.  
#  Instead this alternative reads the files into large buffers and compares, 
#  but is unable to return a line number:
#
#     coopr.swtest.failUnlessFileEqualsBaselineAlt(self, testfile, baseline)
# 
#  For text files, differences in whitespace are ignored.
#  These "failUnless" tests delete the testfile after determining it is
#  equal to the baseline.

#
# Set Python path so Coopr can be imported
#
import os
import sys
from inspect import getfile, currentframe
from os.path import abspath, dirname, exists

def find_topdir():
    curr = abspath(dirname( getfile( currentframe() ) ))
    while os.sep in curr:
        if os.path.exists(curr+os.sep+"etc"+os.sep+"mod"):
            os.environ["PATH"] = curr+os.sep+"bin"+os.pathsep+os.environ["PATH"]
            return curr
        if os.path.basename(curr) == "":
            return None
        curr = os.path.dirname(curr)


topdir=os.environ.get('SPOT_ROOT', find_topdir())
if topdir is None:
    raise ValueError, "Cannot find the top-level SPOT directory!"
moddir=os.sep.join([topdir,"etc","mod"""])
if os.path.exists( os.sep.join([topdir,"packages"]) ):
    testdir=os.sep.join([topdir,"packages","test",""])
else:
    testdir=os.sep.join([topdir,"test",""])
if os.path.exists(os.path.join(os.path.dirname(topdir),'wst_data')):
    datadir=os.path.join(os.path.dirname(topdir),'wst_data')
else:
    datadir = None


import pyutilib.misc
import platform

def nbits():
    #systemType = platform.system() # Linux, Microsoft, CYGWIN_NT-5.2-WOW64
    if '64' in platform.machine():
        # Note: if this returns *64, then the machine IS a 64-bit
        # machine; however in some versions of Python, returning != 64
        # does not guarantee that the machine is 32-bit.
        return 64
    if '64 bit' in sys.version:
        # Obviously, if this is 64-bit Python, then the machine is 64-bit.
        return 64

    try:
        # Works on Windows
        import ctypes
        i = ctypes.c_int()
        kernel32 = ctypes.windll.kernel32
        process = kernel32.GetCurrentProcess()
        kernel32.IsWow64Process(process, ctypes.byref(i))
        if i.value != 0:
            return 64
    except:
        # Works elsewhere
        wordSize = platform.architecture('/bin/ls')[0]   # 64bit, 32bit
        if '64' in wordSize:
            return 64
    return 32

def system_type():
  wordSize = platform.architecture()[0]   # 64bit, 32bit
  systemType = platform.system()          # Linux, Microsoft, CYGWIN_NT-5.2-WOW64

  if 'SPOT_PLATFORM' in os.environ:
    return os.environ['SPOT_PLATFORM']

  if systemType.find("Linux") >= 0:
    s = "linux"
  else:
    s = "win"

  nb = nbits()

  return s+str(nb)

def file_to_string(fname):
  """
  Given a Net3.out file name return the contents in a string excluding the
  beginning and ending lines (which contain time/date info) and excluding
  white space.

  Return string if successful, None otherwise
  """

  try:
    netf = open(fname)
  except:
    print "Can not open "+fname
    return None

  text = netf.readlines()
  netf.close()

  fstart = fend = -1 

  for i in range(0, len(text)):
    if "Analysis ended" in text[i]:
      fend = i
      break
    elif "Analysis begun" in text[i]:
      fstart =i

  if fstart < 0 or fend < 0:
    print "Did not find Analysis begin and end in "+fname
    return None 

  textStr = "".join(text[fstart+1:fend])
  text = []

  ignore = ["\t"," ","\n","\r"]

  newText = pyutilib.misc.remove_chars_in_list(textStr, ignore)
  textStr = ""

  return newText

def compare_Net3_dot_out(testFile, baselineFile):
  """
  Verify that Net3.out is correct

  Omitting the preamble and information at the end (containing date/time steps)
  compare Net3.out to a baseline file.  Omit all whitespace in comparison (so
  we don't have problems with windows vs unix line terminators).

  Return True if the files differ, False if the match.
  """

  newBase = file_to_string(baselineFile) 
  newTest = file_to_string(testFile) 

  if newBase == None or newTest == None:
    print "Error trying to process "+testFile+" and "+baselineFile
    return True

  if len(newBase) != len(newTest) or newBase != newTest:
    print "Net3.out differs from baseline: %s" % baselineFile
    return True

  os.remove(testFile)

  return False
