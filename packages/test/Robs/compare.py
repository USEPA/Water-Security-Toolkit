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
# Run comparisons of different solvers with different parameters.
# Output a csv (comma separated values) file that can be read
# into a spreadsheet program.
#
# The spreadsheet title will be "Comparisons", or if there
# is an argument to this script it will be that argument.
#
#    python compare.py "Comparsions for Revisions 2.0"
# 
# This is written so each command has either 0 or 1 constraint.
# It is assumed if there's a constraint it's of the type
# --ub=constraint.  Parts of this code need to change otherwise.
#
# TODO figure out if command failed, note it in the
# sensorIDs field, and go on with the next command
#
# TODO - flush csv file so we get results if we terminate script
##################################################################

solvers=["pico", "heuristic", "snl_heuristic", "lagrangian"]
numberOfSensors = range(3, 6) 
objectives=["pe", "mc"]
constraints=["none", "ub=vc,35000","ub=pe,800","ub=ec,15000"]

#responseDelay=["0"]    would need to rerun tso2Impact
#detectionLimit=["0"]   would need to rerun tso2Impact

hiStrings=["pe","pk","pd"]    #health impact parameters
hiObjectives=[]
hiConstraints=[]

for objective in objectives:
  for hi in hiStrings:
    if hi in objective:
      hiObjectives.append(objective)
for constraint in constraints:
  for hi in hiStrings:
    if hi in constraint:
      hiConstraints.append(constraint)
  
##################################################################
#
import os
import sys

######################################################
# Path variables were set by test/runtests.py
#
sys.path.insert(0, "../")
import path
pydirs=path.PYTHON_DIRECTORIES
spotbin=path.SPOT_BIN+os.sep
acrobin=path.ACRO_BIN+os.sep
modeldir=path.MODEL_DIR+os.sep

dirs = pydirs.split(":")
for dir in dirs:
  sys.path.insert(0, dir)

currdir = os.path.dirname(os.path.abspath(__file__)) + os.sep
#
#
######################################################

from os.path import abspath, dirname, basename
import shutil
import re
import time

timestr=time.strftime("%Y%m%d%H%M")
datadir = currdir+"testfiles"+os.sep

import pyutilib_th
import pyutilib
from pyspot.atests import system_type
from pyspot.parse_sp import process_evalsensors

##################################################################
#

if not os.path.exists(datadir):
  print "Test data directory "+datadir+" is missing"
  sys.exit(1)

systemType = system_type()

if "linux" in systemType:
  systemType = "linux"
else:
  systemType = "windows"

title = "Comparisons"

if len(sys.argv) > 1:
  title = sys.argv[1]

##################################################################
# Create commands to run

commands = []
cstart = acrobin + "sp --path=" + spotbin + " --path=" + modeldir + " --network=TestNet3 "

for solver in solvers:
  for ns in numberOfSensors:
    for objective in objectives:
      for constraint in constraints:
        cmd = cstart + "--objective=" + objective + " "
        cmd = cmd + "--ub=ns," + `ns` + " "
        if constraint != "none":
          cmd = cmd + "--" + constraint + " "
        cmd = cmd + "--solver=" + solver
        commands.append(cmd)

  commands.append("space")

solverPattern = re.compile("--solver=\S+")
objectivePattern = re.compile("--objective=\S+")
constraintPattern = re.compile("--ub=\S+")

##################################################################
# Run commands, write csv results

global solverName, numSensors, objectiveName
global constraintName, impact, sensorIDs, runTime
global solutionNumSensors
global prevTime, endTime
emptyLine=",,,,,,\n"
csvHead="SOLVER, #SENSORS, OBJECTIVE, CONSTRAINT, IMPACT, SENSOR IDS, RUN TIME\n"
csvText = title + "\n" + "\n" + csvHead

cmdOutput=currdir+"compare.txt"
parsedOutput=currdir+"compare_parsed.txt"
outFileName = "compare.csv"

# get input files

for fname in os.listdir(datadir):
  if ".tai" in fname or ".impact" in fname or ".nodemap" in fname:
    shutil.copy(datadir+os.sep+fname, fname)

# make a copy of the last csv file created

if os.path.exists(outFileName):
  tokens = outFileName.split(".")
  newFileName = tokens[0] + "_"+timestr+"."+tokens[1]
  shutil.copy(outFileName, newFileName)

outFile = open(outFileName, "w")
outFile.write(csvText)

prevTime = -1

for cmd in commands:
  csvText = ""
  solutionNumSensors=0

  if cmd == "space":
    csvText = csvText + emptyLine   # for readability
    csvText = csvText + csvHead
    outFile.write(csvText)
    continue

  print cmd

  if systemType == "linux":
    prevTime = time.clock()  # on linux, elapsed processor time
  elif systemType == "windows" and prevTime == -1:
    prevTime = time.clock()  # on windows, the wall time elapsed since first call
  else:
    prevTime = endTime

  # TODO how do we know if it failed to complete
  # if it fails, set fields to "failed" and keep going
  pyutilib.run_command(cmd, cmdOutput, cwd=currdir)

  endTime = time.clock()

  runTime = endTime - prevTime

  # get the solution
  process_evalsensors(cmdOutput, parsedOutput)

  resultFile = open(parsedOutput, "r")

  impact = None
  for line in resultFile:
    tokens = line.strip().split(":")
    if len(tokens) >= 2:
      label = tokens[0].strip()
      value = tokens[1].strip()
      if label == "Number of sensors":
        solutionNumSensors = value
      elif label == "Mean impact":
        impact = value
      elif label  == "Sensor node IDs":
        sensorIDs = value

  resultFile.close()

  solverInfo = solverPattern.search(cmd)
  solverName= solverInfo.group().strip()
  tokens = solverName.split("=")
  solverName = tokens[1]

  objectiveInfo = objectivePattern.search(cmd)
  objectiveName= objectiveInfo.group().strip()
  tokens = objectiveName.split("=")
  objectiveName = tokens[1]

  constraintInfo = constraintPattern.findall(cmd)
  if len(constraintInfo) == 2:
    constraintName = constraintInfo[1].strip()
    tokens = constraintName.split("=")
    constraintName = "ub="+tokens[1]
  else:
    constraintName = "none"

  numSensors = constraintInfo[0].strip()   #--ub=ns,5
  tokens = numSensors.split("=")
  tokens = tokens[1].split(",")
  numSensors = tokens[1]

  if int(numSensors) > 0 and int(solutionNumSensors) == 0:
    sensorIDs = "infeasible"
  elif int(numSensors) > int(solutionNumSensors):
    sensorIDs = sensorIDs + " ("+solutionNumSensors+" sensors)"

  if impact == None:
    impactStr = "error"
  else:
    impactF = float(impact)
    impactStr = "%d"%impactF
    if impactF > 9999:
      impactStr = "%e"%impactF

  runTimeStr = "%e"%runTime

  # remove comma, if any, from constraint name, since
  # comma delimits our fields

  tokens = constraintName.split(",")
  if len(tokens) > 1:
    constraintName = " ".join(tokens)

  csvText = csvText + solverName + ", "
  csvText = csvText + numSensors + ", "
  csvText = csvText + objectiveName + ", "
  csvText = csvText + constraintName + ", "
  csvText = csvText + impactStr + ", "
  csvText = csvText + sensorIDs + ", "
  csvText = csvText + runTimeStr + "\n"

  outFile.write(csvText)

##################################################################
# Clean up

outFile.close()

for fname in os.listdir(currdir):
  tmpFile = False
  if fname == cmdOutput or fname == parsedOutput:
    tmpFile = True
  elif "TestNet3" in fname or "bio" in fname or ".txt" in fname:
    tmpFile = True

  if tmpFile == True:
    os.remove(fname)

print "Results are in compare.csv"
