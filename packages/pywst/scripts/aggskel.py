#!/usr/bin/python
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
# Aggregation and Skeletonization tools
# 
    
def getNodemap(nodemapFile):
    # Open nodemap file, return epanetID and nodeID pairs
    fid = open(nodemapFile, 'r')
    nodemap = fid.read()
    fid.close()
    nodemap = nodemap.splitlines()
    nodeID = list()
    epanetID = list()
    for i in range(len(nodemap)):
        temp = nodemap[i].split(' ')
        nodeID.append(int(temp[0])) # nodeID is an integer
        epanetID.append(temp[1]) # epanetID is a string

    return epanetID, nodeID
    
def getSkeleton(skeletonFile, nodemapDict, nodemapOutFile):
    # Open map file, convert epanetID to nodeID, return node/supernode pairs.
    # Write condensed nodemap file
    fid = open(skeletonFile, 'r')
    skeleton = fid.read()
    fid.close()
    skeleton = skeleton.splitlines()
    fidOut = open(nodemapOutFile, 'w')
    nodeList = list()
    supernodeList = list() 
    for i in range(len(skeleton)): # loop through each line in skeleton file
        temp = skeleton[i].split(' ')
        SN = nodemapDict[temp[0]]  # convert ID
        for j in range(len(temp[2:])):
            if temp[2+j] == '':
                break
            nodeList.append(nodemapDict[temp[2+j]]) # convert ID
            supernodeList.append(i+1) #condense set (changed SN to i)
        fidOut.write("%i %s\n" % (i+1,temp[0]))
    fidOut.close()
    
    return nodeList, supernodeList

def getSkeletonSensor(skeletonFile, nodemapDetailDict, nodemapOutFile, sensor):
    # Open map file, return list of nodes from the detailed nodemap to include 
    # in the final sensor placement (based on solution from the first sp, 
    # sensor file).  Write condensed nodemap file
    fid = open(skeletonFile, 'r')
    skeleton = fid.read()
    fid.close()
    skeleton = skeleton.splitlines()
    fidOut = open(nodemapOutFile, 'w')
    sensorList = list()
    condense = 1
    sn_membership = 0
    for i in range(len(skeleton)):
        temp = skeleton[i].split(' ')
        SN = nodemapDetailDict[temp[0]] # convert ID
        if sensor.count(SN) == 1:
            sn_membership = sn_membership+1
            for j in range(len(temp[2:])):
                if temp[2+j] == '':
                    break
                node = int(nodemapDetailDict[temp[2+j]]) # convert ID
                sensorList.append(node)
                fidOut.write("%i %i %i %s\n" % (condense,sn_membership,node,temp[2+j]))# old nodemap file had j+1 for sn_membership 
                condense = condense+1
    fidOut.close()
    
    return sensorList
    
def getSkeletonWeight(skeletonFile, nodemapDetailDict, eventList):
    # Open map file, return supernode and weight pairs.  The weight is the 
    # number of times an event occurs in the supernode
    fid = open(skeletonFile, 'r')
    skeleton = fid.read()
    fid.close()
    skeleton = skeleton.splitlines()
    weightList = list()
    supernodeList = list()
    numList = list()
    for i in range(len(skeleton)):
        temp = skeleton[i].split(' ')
        SN = nodemapDetailDict[temp[0]] # convert ID
        supernodeList.append(SN)
        numList.append(temp[1])
        nodeList = list()
        for j in range(len(temp[2:])):
            if temp[2+j] == '':
                break
            nodeList.append(nodemapDetailDict[temp[2+j]]) # convert ID
        weightList.append(len(set(nodeList) & set(eventList))) # set intersect
    
    return supernodeList, weightList
    
def getSensor(sensorFile):
    # Open sensor file (output from sp), return sensor locations
    fid = open(sensorFile, 'r')
    for line in fid:
        if line[0] != '#':
            sensor = line
            break
    fid.close()
    sensor = sensor.replace('\n','')
    temp = list()
    sensor = sensor.split(' ')
    for i in range(len(sensor)):
        if sensor[i].isdigit(): # removes whitespace
            temp.append(int(sensor[i]))
    temp.pop(0) # remove first two entries
    temp.pop(0)
    sensor = temp
    
    return sensor
    
def getEvent(impactFile):
    # Open impact file, return event locations (where time = 0)
    fidIn = open(impactFile, 'r')
    impactLine1 = fidIn.readline()
    impactLine2 = fidIn.readline()
    eventList = list()
    while True:
        lines = fidIn.readlines(100000)
        if not lines:
            break
        lines = map(str.rstrip,lines,'\n')
        lines = map(str.split,lines,' ')
        for line in lines:
            if int(line[2]) == 0: # if time = 0 WARNING: ADD CHECK for no zero time
                eventList.append(int(line[1])) # keep node
    fidIn.close()
    
    return eventList
    
def geographic_aggregation(impactFile, skeletonFile, nodemapFile, method, \
        impactOutFile, nodemapOutFile):
    # Skeletonize the impact file using geographic aggregation, line by line, 
    # using min, max, mean, first or last method.  Write new impact file and 
    # nodemap file
    from operator import itemgetter

    # Read nodemap file, create a dictonary [epanetID: nodeID]
    epanetID, nodeID = getNodemap(nodemapFile)
    nodemapDict = dict(zip(epanetID, nodeID))
    
    # Read skeleton file, create a dictonary [node: supernode], write condensed 
    # nodemap file
    nodeList, supernodeList = getSkeleton(skeletonFile, nodemapDict, nodemapOutFile)
    skelDict = dict(zip(nodeList, supernodeList)) # supernodeList is a condensed set
    
    # Skeletonize the impact file, read line by line, write once per sceanrio
    fidIn = open(impactFile, 'r')
    impactLine1 = fidIn.readline()
    impactLine1 = str(max(skelDict.values()))+'\n' #impact header, condensed set
    impactLine2 = fidIn.readline()
    
    fidOut = open(impactOutFile, 'w')
    fidOut.write(impactLine1)
    fidOut.write(impactLine2)
    
    timeDict = {}   # SN: time
    impactDict = {} # SN: impact
    countDict = {}  # SN: count
    
    while True:
        lines = fidIn.readlines(100000)
        if not lines:
            break
        lines = map(str.rstrip,lines,'\n')
        lines = map(str.split,lines,' ')
        for line in lines:
            line = [int(line[0]), int(line[1]), int(line[2]), float(line[3])]
            
            if line[1] == -1: 
                # sort by supernode
                timeTuple = sorted(timeDict.items(),key=itemgetter(0))
                
                # calculate mean before sorting by time
                if method == "mean": 
                    for i in range(len(timeTuple)):
                        timeList = list(timeTuple[i])
                        SN = timeList[0]
                        timeList[1] = timeList[1]/countDict[SN]
                        impactDict[SN] = impactDict[SN]/countDict[SN]
                        timeTuple[i] = tuple(timeList)
                
                # sort by time
                timeTuple = sorted(timeTuple,key=itemgetter(1))
                
                # write to file
                for i in range(len(timeTuple)):
                    SN = timeTuple[i][0]
                    time = timeTuple[i][1]
                    impact = impactDict[SN]
                    fidOut.write("%i %i %i %-7g\n" % (line[0],SN,time,impact))
                
                # add dummy
                fidOut.write("%i %i %i %-7g\n" % (line[0],line[1],line[2],line[3]))
                
                # reset time, impact, and count
                timeDict = {}   
                impactDict = {} 
                countDict = {} 
            
            # convert node to supernode and update time, impact, and count
            else:
                line[1] = skelDict[line[1]] # covert node to supernode
                
                try:
                    countDict[line[1]] += 1
                    if method == "min":
                        timeDict[line[1]] = min(timeDict[line[1]],line[2])
                        impactDict[line[1]] = min(impactDict[line[1]],line[3])
                    elif method == "max":
                        timeDict[line[1]] = max(timeDict[line[1]],line[2])
                        impactDict[line[1]] = max(impactDict[line[1]],line[3])
                    elif method == "mean": # keep sum
                        timeDict[line[1]] = timeDict[line[1]] + line[2]
                        impactDict[line[1]] = impactDict[line[1]] + line[3]
                    elif method == "last":
                        timeDict[line[1]] = line[2]
                        impactDict[line[1]] = line[3]
                except KeyError: # first occurance, method = "first"
                    timeDict[line[1]] = line[2]
                    impactDict[line[1]] = line[3]
                    countDict[line[1]] = 1

    fidIn.close()
    fidOut.close()
    
def geographic_aggregation_median(impactFile, skeletonFile, nodemapFile,\
        impactOutFile, nodemapOutFile):
    # Skeletonize the impact file using geographic aggregation, scenario by 
    # scenario.  Use for median method when the file is not sorted. Write new 
    # impact file and nodemap file
    from operator import itemgetter
    #import numpy as np
    import pdb
    
    # Read nodemap file, create a dictonary [epanetID: nodeID]
    epanetID, nodeID = getNodemap(nodemapFile)
    nodemapDict = dict(zip(epanetID, nodeID))
    
    # Read skeleton file, create a dictonary [node: supernode], write condensed 
    # nodemap file
    nodeList, supernodeList = getSkeleton(skeletonFile, nodemapDict, nodemapOutFile)
    skelDict = dict(zip(nodeList, supernodeList)) # supernodeList is a condensed set
    
    # Skeletonize the impact file, read line by line, write once per sceanrio
    fidIn = open(impactFile, 'r')
    impactLine1 = fidIn.readline()
    impactLine1 = str(max(skelDict.values()))+'\n' #impact header, condensed set
    impactLine2 = fidIn.readline()
    
    fidOut = open(impactOutFile, 'w')
    fidOut.write(impactLine1)
    fidOut.write(impactLine2)
    
    timeDict = {}   # SN: time
    impactDict = {} # SN: impact
    
    while True:
        lines = fidIn.readlines(100000)
        if not lines:
            break
        lines = map(str.rstrip,lines,'\n')
        lines = map(str.split,lines,' ')
        for line in lines:
            line = [int(line[0]), int(line[1]), int(line[2]), float(line[3])]
            
            if line[1] == -1:
                # calculate median
                for i in range(len(timeDict)):
                    #timeDict[timeDict.keys()[i]] = np.median(timeDict.items()[i][1])
                    #impactDict[impactDict.keys()[i]] = np.median(impactDict.items()[i][1])
                    timeDict[timeDict.keys()[i]] = median(timeDict.items()[i][1])
                    impactDict[impactDict.keys()[i]] = median(impactDict.items()[i][1])
                
                # sort by supernode then time
                timeTuple = sorted(timeDict.items(),key=itemgetter(0))
                timeTuple = sorted(timeTuple,key=itemgetter(1))
                
                # write to file
                for i in range(len(timeTuple)):
                    SN = timeTuple[i][0]
                    time = timeTuple[i][1]
                    impact = impactDict[SN]
                    fidOut.write("%i %i %i %-7g\n" % (line[0],SN,time,impact))
                
                # add dummy
                fidOut.write("%i %i %i %-7g\n" % (line[0],line[1],line[2],line[3]))
                
                # reset time and impact
                timeDict = {}  
                impactDict = {} 
            
            # convert node to supernode and keep time and impact for each supernode
            else:
                pdb.set_trace()
                line[1] = skelDict[line[1]] # covert node to supernode
                try:
                    timeDict[line[1]].append(line[2])
                    impactDict[line[1]].append(line[3])
                except KeyError: # first occurance
                    timeDict[line[1]] = [line[2]]
                    impactDict[line[1]] = [line[3]]

    fidOut.close()
    
def geographic_aggregation_sorted_median(impactFile, skeletonFile, nodemapFile, \
        impactOutFile, nodemapOutFile):
    # Skeletonize the impact file using geographic aggregation.  Use for the 
    # option when the file is sorted (monotonically increasing) according to 
    # impact value for each scenario.  Runtime is significantly improved as 
    # compared to geographic_aggregation_scenario (which should be used if the impact 
    # file is not sorted). Write new impact file and nodemap file
    from operator import itemgetter
    import math

    # Read nodemap file, create a dictonary [epanetID: nodeID]
    epanetID, nodeID = getNodemap(nodemapFile)
    nodemapDict = dict(zip(epanetID, nodeID))
    
    # Read skeleton file, create a dictonary [node: supernode], write condensed 
    # nodemap file
    nodeList, supernodeList = getSkeleton(skeletonFile, nodemapDict, nodemapOutFile)
    skelDict = dict(zip(nodeList, supernodeList)) # supernodeList is a condensed set
    
    # Skeletonize the impact file, read line by line, write once per sceanrio
    fidIn = open(impactFile, 'r')
    impactLine1 = fidIn.readline()
    impactLine1 = str(max(skelDict.values()))+'\n' #impact header, condensed set
    impactLine2 = fidIn.readline()
    
    fidOut = open(impactOutFile, 'w')
    fidOut.write(impactLine1)
    fidOut.write(impactLine2)
    
    fidOutTemp = open('temp.impact', 'w')
    timeDict = {}   # SN: time
    impactDict = {} # SN: impact
    countDict = {}  # SN: count  
    recountDict = {} # SN: recount
    
    while True:
        lines = fidIn.readlines(100000)
        if not lines:
            break
        lines = map(str.rstrip,lines,'\n')
        lines = map(str.split,lines,' ')
        for line in lines:
            line = [int(line[0]), int(line[1]), int(line[2]), float(line[3])]
            
            if line[1] == -1:
                dummyLine = line
                fidOutTemp.close()
                fidInTemp = open('temp.impact', 'r')
                while True:
                    lines = fidInTemp.readlines(100000)
                    if not lines:
                        break
                    lines = map(str.rstrip,lines,'\n')
                    lines = map(str.split,lines,' ')
                    for line in lines:
                        line = [int(line[0]), int(line[1]), int(line[2]), float(line[3])]
                        try:
                            recountDict[line[1]] += 1
                        except KeyError: # first occurance
                            recountDict[line[1]] = 1
                        # ceil and floor will be the same for list with odd number of 
                        # members.  For even number of members, the median is the mean 
                        # the two middle values.
                        if (recountDict[line[1]] == math.floor((countDict[line[1]]+1.)/2)):
                            timeDict[line[1]] = line[2] # first occurance
                            impactDict[line[1]] = line[3]
                        if (recountDict[line[1]] == math.ceil((countDict[line[1]]+1.)/2)):
                            timeDict[line[1]] = (timeDict[line[1]] + line[2])/2 # take the mean
                            impactDict[line[1]] = (impactDict[line[1]] + line[3])/2
                
                # sort by supernode then time
                timeTuple = sorted(timeDict.items(),key=itemgetter(0))
                timeTuple = sorted(timeTuple,key=itemgetter(1))
                
                # write to file
                for i in range(len(timeTuple)):
                    SN = timeTuple[i][0]
                    time = timeTuple[i][1]
                    impact = impactDict[SN]
                    fidOut.write("%i %i %i %-7g\n" % (line[0],SN,time,impact))
                
                # add dummy
                fidOut.write("%i %i %i %-7g\n" % (dummyLine[0],dummyLine[1],dummyLine[2],dummyLine[3]))
                
                # reset time, impact, count, and recount
                fidOutTemp = open('temp.impact', 'w')
                timeDict = {}  
                impactDict = {} 
                countDict = {}  
                recountDict = {} 
            
            # convert node to supernode and count occurances for each scenario
            else:
                line[1] = skelDict[line[1]] # covert node to supernode
                try:
                    countDict[line[1]] += 1
                except KeyError: # first occurance
                    countDict[line[1]] = 1
                fidOutTemp.write("%i %i %i %-7g\n" % (line[0],line[1],\
                    line[2],line[3]))

    fidOut.close()
    import os
    os.remove('temp.impact')
    
def refine_impact(impactFile, skeletonFile, sensorFile, nodemapFile, \
        nodemapSkelFile, impactOutFile, nodemapOutFile):
    # Refine the impact file.  Keep only locations selected by the skeletonized 
    # sp solution.  Write new impact file and nodemap file.
    
    # Read detailed nodemap file, create a dictonary [epanetID: nodeID]
    epanetID, nodeID = getNodemap(nodemapFile)
    nodemapDetailDict = dict(zip(epanetID, nodeID))
    
    # Read skeletonized nodemap file and create a dictonary [nodeID: epanetID]
    # nodemapSkelDict is intentionally set up in reverse so you can easily 
    # convert between the detailed nodeID and skeletonized nodeID
    # detailedNode = nodemapDetailDict[nodemapSkelDict[skeletonNode]] 
    epanetID, nodeID = getNodemap(nodemapSkelFile)
    nodemapSkelDict = dict(zip(nodeID, epanetID)) 
    
    # Read sensor file and create sensor list of original nodes that belong to 
    # sensor supernodes
    sensor = getSensor(sensorFile)
    for i in range(len(sensor)):
        sensor[i] = nodemapDetailDict[nodemapSkelDict[sensor[i]]] # convert
    
    # Read skeleton file and create a sensor list
    sensorList = getSkeletonSensor(skeletonFile, nodemapDetailDict, \
        nodemapOutFile, sensor)
    
    # Refine the impact file.
    # Keep only locations included in the selected supernodes
    fidIn = open(impactFile, 'r')
    impactLine1 = fidIn.readline()
    impactLine1 = str(len(sensorList))+'\n' #change impactLine1 for condensed set
    impactLine2 = fidIn.readline()
    
    fidOut = open(impactOutFile, 'w')
    fidOut.write(impactLine1)
    fidOut.write(impactLine2)
    
    while True:
        lines = fidIn.readlines(100000)
        if not lines:
            break
        lines = map(str.rstrip,lines,'\n')
        lines = map(str.split,lines,' ')
        for line in lines:
            line = [int(line[0]), int(line[1]), int(line[2]), float(line[3])]
            
            if line[1] is -1: # if node is dummy
                fidOut.write("%i %i %i %-7g\n" % (line[0],line[1],line[2],line[3]))
            elif line[1] in sensorList: # if node is in the sensor list
                line[1] = sensorList.index(line[1])+1 # condensed set
                fidOut.write("%i %i %i %-7g\n" % (line[0],line[1],line[2],line[3]))
    
    fidIn.close()
    fidOut.close()

def refine_impact_Lag(impactFile, sensorFile, nodemapFile, impactOutFile, nodemapOutFile):
    # Refine the impact file.  Keep only locations selected by the first 
    # sp solution.  Write new impact file and nodemap file.
    
    # Read detailed nodemap file, create a dictonary [epanetID: nodeID]
    epanetID, nodeID = getNodemap(nodemapFile)
    nodemapDetailDict = dict(zip(epanetID, nodeID))
    
    # Read sensor file and create sensor list of original nodes that belong to 
    # sensor supernodes
    sensorList = getSensor(sensorFile)
    
    # Create final nodemap
    fidOut = open(nodemapOutFile, 'w')
    fidOut.write("FOO\n") 
    fidOut.close()
    
    # Refine the impact file.
    # Keep only locations included in the selected supernodes
    fidIn = open(impactFile, 'r')
    impactLine1 = fidIn.readline()
    impactLine1 = str(len(sensorList))+'\n' #change impactLine1 for condensed set
    impactLine2 = fidIn.readline()
    
    fidOut = open(impactOutFile, 'w')
    fidOut.write(impactLine1)
    fidOut.write(impactLine2)
    
    while True:
        lines = fidIn.readlines(100000)
        if not lines:
            break
        lines = map(str.rstrip,lines,'\n')
        lines = map(str.split,lines,' ')
        for line in lines:
            line = [int(line[0]), int(line[1]), int(line[2]), float(line[3])]
            
            if line[1] is -1: # if node is dummy
                fidOut.write("%i %i %i %-7g\n" % (line[0],line[1],line[2],line[3]))
            elif line[1] in sensorList: # if node is in the sensor list
                line[1] = sensorList.index(line[1])+1 # condensed set
                fidOut.write("%i %i %i %-7g\n" % (line[0],line[1],line[2],line[3]))
    
    fidIn.close()
    fidOut.close()
    
def weight_impact(impactFile, impactSkelFile, skeletonFile, nodemapFile, 
                  nodemapSkelFile, impactOutFile):
    # Weight the impact file according to the number of events that occur in 
    # each supernode. Write new impact file.
    
    # Read detailed nodemap file, create a dictonary [epanetID: nodeID]
    epanetID, nodeID = getNodemap(nodemapFile)
    nodemapDetailDict = dict(zip(epanetID, nodeID))

    # Read skeletonized nodemap file and create a dictonary [nodeID: epanetID]
    # nodemapSkelDict is intentionally set up in reverse so you can easily 
    # convert between the detailed nodeID and skeletonized nodeID
    # detailedNode = nodemapDetailDict[nodemapSkelDict[skeletonNode]] 
    epanetID, nodeID = getNodemap(nodemapSkelFile)
    nodemapSkelDict = dict(zip(nodeID, epanetID)) 

    # Create a list of event locations. Events originate where the time = 0
    eventList = getEvent(impactFile) # list in detailed nodeID
    eventSkelList = getEvent(impactSkelFile) # list in skel nodeID
    
    # Read skeleton file and create a weight dictonary [supernode: weight]
    supernodeList, weightList = getSkeletonWeight(skeletonFile, \
        nodemapDetailDict, eventList)
    weightDict = dict(zip(supernodeList, weightList)) 
    
    """
    # write a weight file (use --incident-weights option for sp)
    fidOut = open('weights.txt', 'w')
    for i in range(len(eventSkelList)):
        eventNode = nodemapDetailDict[nodemapSkelDict[eventSkelList[i]]]
        fidOut.write("%i %i %i\n" % (eventSkelList[i], eventNode, weightDict[eventNode]))
    fidOut.close()
    """
    
    # Weight impact file according to the number of events in each start location
    fidIn = open(impactSkelFile, 'r')
    impactLine1 = fidIn.readline()
    impactLine2 = fidIn.readline()
    
    fidOut = open(impactOutFile, 'w')
    fidOut.write(impactLine1)
    fidOut.write(impactLine2)
    
    while True:
        lines = fidIn.readlines(100000)
        if not lines:
            break
        lines = map(str.rstrip,lines,'\n')
        lines = map(str.split,lines,' ')
        for line in lines:
            line = [int(line[0]), int(line[1]), int(line[2]), float(line[3])]
            
            if line[2] == 0:
                # convert node name between the detailed and skeletonized world
                eventNode = nodemapDetailDict[nodemapSkelDict[line[1]]] 
            
            line[3] = line[3]*weightDict[eventNode] # weight impact
            
            fidOut.write("%i %i %i %-7g\n" % (line[0], line[1], line[2], line[3]))
    
    fidIn.close()
    fidOut.close()
"""
THIS FUNCTION RELIES ON NUMPY, REWRITE
def feasible_impact(impactFile, locationFile, nodemapFile, optSense, impactOutFile):
    # Create a new impact file based on feasible, infeasible, fixed and unfixed 
    # locations in the location file.  
    import numpy as np
    
    # Read nodemap file, create a dictonary [epanetID: nodeID]
    epanetID, nodeID = getNodemap(nodemapFile)
    nodemapDict = dict(zip(epanetID, nodeID))
    
    # Read location file
    fid = open(locationFile, 'r')
    location = fid.read()
    fid.close()
    location = location.splitlines() 
    
    # Create fixed and infeasible datasets.  Locations are processed sequantially
    nodeFixed = list()
    nodeInfeasible = list()
    for i in range(len(location)):
        temp = location[i].split(' ')
        if temp[0] == 'feasible':
            for j in range(len(temp[1:])):
                if temp[1+j] == 'ALL' or temp[1+j] == '*': # all nodes feasbile
                    nodeInfeasible = list()
                    break
                else:
                    node = nodemapDict[temp[1+j]] # convert ID
                    if node in nodeInfeasible: nodeInfeasible.remove(node)
        elif temp[0] == 'infeasible':
            for j in range(len(temp[1:])):
                if temp[1+j] == 'ALL' or temp[1+j] == '*': # all nodes infeasible
                    nodeInfeasible = nodeID
                    break
                else:
                    node = nodemapDict[temp[1+j]] # convert ID
                    nodeInfeasible.append(node)
        elif temp[0] == 'unfixed':
            for j in range(len(temp[1:])):
                if temp[1+j] == 'ALL' or temp[1+j] == '*': # all nodes unfixed
                    nodeFixed = list()
                    break
                else:
                    node = nodemapDict[temp[1+j]] # convert ID
                    if node in nodeFixed: nodeFixed.remove(node)
        elif temp[0] == 'fixed':
            for j in range(len(temp[1:])):
                if temp[1+j] == 'ALL' or temp[1+j] == '*': # all nodes fixed
                    nodeFixed = nodeID
                    break
                else:
                    node = nodemapDict[temp[1+j]] # convert ID
                    nodeFixed.append(node)
    
    # Read in impact file by sceanrio, write once per sceanrio
    fidIn = open(impactFile, 'r')
    impactLine1 = fidIn.readline()
    impactLine2 = fidIn.readline()
        
    line = fidIn.readline()
    line = line.rstrip('\n')
    line = line.split(' ')
    line = [int(line[0]), int(line[1]), int(line[2]), float(line[3])]
    
    fidOut = open(impactOutFile, 'w')
    fidOut.write(impactLine1)
    fidOut.write(impactLine2)
    
    scenario = 0;
    continueLoop = True
    
    while continueLoop:
        scenario = line[0]
        impactSection = list()
        
        while (line[0] == scenario):
            if line[1] not in nodeInfeasible: # if node is feasible
                impactSection.append(line)
            
            # read next line from the impact file
            line = fidIn.readline()
            if line is None: 
                continueLoop = False
                break
            line = line.rstrip('\n')
            line = line.split(' ')
            if len(line) < 4: 
                continueLoop = False
                break
            line = [int(line[0]), int(line[1]), int(line[2]), float(line[3])]
        
        impactSection = np.array(impactSection)
        
        # if node is fixed, keep nodes if impact < (min sense) or > (max sense) 
        # the fixed impact value.  Set the dummy to the fixed impact value
        for i in range(len(nodeFixed)): 
            if nodeFixed[i] in impactSection[:,1]:
                bool = impactSection[:,1] == nodeFixed[i] 
                impactVal = impactSection[bool,3]
                if optSense == 'min': 
                    bool = (impactSection[:,3] < impactVal) +\
                           (impactSection[:,1] == -1)
                elif optSense == 'max':
                    bool = (impactSection[:,3] > impactVal) +\
                           (impactSection[:,1] == -1)
                impactSection = impactSection[bool,:]
                # Set dummy
                bool = impactSection[:,1] == -1
                impactSection[bool,3] = impactVal
        
        for i in range(len(impactSection)):
            fidOut.write("%i %i %i %-7g\n" % (impactSection[i,0], impactSection[i,1],\
                impactSection[i,2], impactSection[i,3]))
        
    fidIn.close()
    fidOut.close()
"""
def condense_impact(impactFile, outImpactFile, outNodemapFile):
    # Read impact file line by line.  Rename nodes for a condensed 
    # representation.  Write new impact file and nodemap file
    import os
    
    fidIn = open(impactFile, 'r')
    impactLine1 = fidIn.readline()
    impactLine2 = fidIn.readline()
        
    fidOut = open(outImpactFile, 'w')
    fidOut.write(impactLine1)
    fidOut.write(impactLine2)  
    
    origList = list()
    condList = list()
    
    while True:
        lines = fidIn.readlines(100000)
        if not lines:
            break
        lines = map(str.rstrip,lines,'\n')
        lines = map(str.split,lines,' ')
        for line in lines:
            line = [int(line[0]), int(line[1]), int(line[2]), float(line[3])]
            
            if line[1] is not -1:
                if line[1] not in origList: 
                    origList.append(line[1])
                line[1] = origList.index(line[1])+1
            
            fidOut.write("%i %i %i %-7g\n" % (line[0],line[1],line[2],line[3]))
            
    fidIn.close()
    fidOut.close()
    
    print origList 
    
    # remove the first line in the impact file and replace it with the 
    # actual number of nodes
    impactLine1 = str(len(origList))
    #sed -e '1d' -e '1i\16' -i temp.impact
    os.system('sed -e \'1d\' -e \'1i\\'+impactLine1+'\' -i '+outImpactFile) 
    os.system('unix2dos '+outImpactFile) # remove ^M
    
    # write nodemap to file
    fidOut = open(outNodemapFile, 'w')
    for i in range(len(origList)):
        fidOut.write("%i\n" % (origList[i]))
    
    fidOut.close()
    
def dummy_impact(impactFile, impactOutFile):
    # Read impact file line by line.  Remove dummy if no other nodes exist for 
    # that scenario.  Write new impact file.
    fidIn = open(impactFile, 'r')
    impactLine1 = fidIn.readline()
    impactLine2 = fidIn.readline()
        
    fidOut = open(impactOutFile, 'w')
    fidOut.write(impactLine1)
    fidOut.write(impactLine2)  
    
    scenario = 0
    while True:
        lines = fidIn.readlines(100000)
        if not lines:
            break
        lines = map(str.rstrip,lines,'\n')
        lines = map(str.split,lines,' ')
        for line in lines:
            line = [int(line[0]), int(line[1]), int(line[2]), float(line[3])]
                
            if line[1] is not -1 or scenario > 0: 
                scenario = line[0]
                fidOut.write("%i %i %i %-7g\n" % (line[0],line[1],line[2],line[3]))
            
            if line[1] is -1: 
                scenario = 0
        
    fidIn.close()
    fidOut.close()

def convert_fullSensorFile(sensorFile, nodemapFile, sensorOutFile):
    # Convert sensor file from condensed representation to actual names
    
    # Read detailed nodemap file, create a dictonary [epanetID: nodeID]
    epanetID, nodeID = getNodemap(nodemapFile)
    nodemapDetailDict = dict(zip(nodeID, epanetID))
    
    # Read sensor file and create sensor list of original nodes that belong to 
    # sensor supernodes
    sensor = getSensor(sensorFile)
    for i in range(len(sensor)):
        sensor[i] = nodemapDetailDict[sensor[i]] # convert
    
    # Write the converted sensor file
    fidOut = open(sensorOutFile, 'w')
    fidOut.write("1 %i " %(len(sensor)))  
    
    for i in range(len(sensor)):
        fidOut.write("%s " % (sensor[i]))
    fidOut.write("\n")
    
    fidOut.close()

def convert_skelSensorFile(sensorFile, nodemapFile, nodemapSkelFile, sensorOutFile):
    # Convert sensor file from condensed representation to actual names
    
    # Read detailed nodemap file, create a dictonary [epanetID: nodeID]
    epanetID, nodeID = getNodemap(nodemapFile)
    nodemapDetailDict = dict(zip(epanetID, nodeID))
    
    # Read skeletonized nodemap file and create a dictonary [nodeID: epanetID]
    # nodemapSkelDict is intentionally set up in reverse so you can easily 
    # convert between the detailed nodeID and skeletonized nodeID
    # detailedNode = nodemapDetailDict[nodemapSkelDict[skeletonNode]] 
    epanetID, nodeID = getNodemap(nodemapSkelFile)
    nodemapSkelDict = dict(zip(nodeID, epanetID)) 
    
    # Read sensor file and create sensor list of original nodes that belong to 
    # sensor supernodes
    sensor = getSensor(sensorFile)
    for i in range(len(sensor)):
        sensor[i] = nodemapDetailDict[nodemapSkelDict[sensor[i]]] # convert
    
    # Write the converted sensor file
    fidOut = open(sensorOutFile, 'w')
    fidOut.write("1 %i " %(len(sensor)))  
    
    for i in range(len(sensor)):
        fidOut.write("%i " % (sensor[i]))
    fidOut.write("\n")
    
    fidOut.close()
    
def convert_finalSensorFile(sensorFile, nodemapFinalFile, statOutFile, sensorOutFile):
    # Convert the final sensor placment (from SP2).  Write stats and convert file.
    
    # Read in nodemap final file
    fidIn = open(nodemapFinalFile, 'r')
    lines = fidIn.readlines()
    fidIn.close()
    lines = map(str.rstrip,lines,'\n')
    lines = map(str.split,lines,' ')
    
    # Get sensor locations
    sensor = getSensor(sensorFile)
    
    # Find stats, number of sensors, duplicates
    duplicate = 0
    for i in range(len(sensor)):
        if (lines[sensor[i]-1][1] == '1'):
            duplicate = duplicate+1
            
    
    # Write sensor statistics
    fidOut = open(statOutFile, 'w')
    fidOut.write("%i %i" %(len(lines), duplicate))  
    fidOut.close()

    # Write the converted sensor file
    fidOut = open(sensorOutFile, 'w')
    fidOut.write("1 %i " %(len(sensor)))

    for i in range(len(sensor)):
        fidOut.write("%s " % (lines[sensor[i]-1][3]))

    fidOut.write("\n")
    fidOut.close()

def median(numbers):
    sortList = sorted([float(x) for x in numbers])
    if len(sortList)%2==0: # if even number in list, take mean of middle 2
        median_value = (sortList[len(sortList)/2-1] + sortList[len(sortList)/2])/2
    else: # odd number
        median_value = sortList[len(sortList)/2]
    
    return median_value

