#  _________________________________________________________________________
#
#  Water Security Toolkit (WST)
#  Copyright (c) 2012 Sandia Corporation.
#  This software is distributed under the Revised BSD License.
#  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive 
#  license for use of this work by or on behalf of the U.S. government.
#  For more information, see the License Notice in the WST User Manual.
#  _________________________________________________________________________
#
import os, sys, datetime 
import yaml
from string import lower
import shutil
import pyutilib.subprocess
import logging
import pyutilib.services
from os.path import abspath, dirname, join, basename

try:
    import pyepanet
except ImportError:
    pass

none_list = ['none','','None','NONE', None]

#
# Sometimes the prefix indicates a directory name. This function will
# detect that situation and adjust the keywords sent to
# TempfileManager accordingly
#
def get_tempfile(prefix, suffix):
 
    tempfile_kwds = {}
    if prefix is None:
        pass
    elif os.path.isdir(prefix):
        tempfile_kwds['dir'] = prefix
    else:
        _f = basename(prefix)
        if _f:
            tempfile_kwds['prefix'] = _f
        _d = dirname(prefix)
        tempfile_kwds['dir'] = _d if _d else "."
    tempfile_kwds['suffix'] = suffix
        
    return pyutilib.services.TempfileManager.create_tempfile(**tempfile_kwds)

def declare_tempfile(filename):
    abs_fname = join(abspath(os.curdir), filename)
    pyutilib.services.TempfileManager.add_tempfile(filename, exists=False)
    return abs_fname

def write_tsg(location, type, species, strength, start_time, stop_time, tsg_filename):
    # <Src1><SrcN> <SrcType> <SrcSpecie> <SrcStrngth> <Start> <Stop>
    fid = open(tsg_filename, 'w')
    for i in range(len(location)):
        fid.write("%s " % location[i])
    fid.write("%s " % type)
    if not species in none_list:
        fid.write("%s " % species)
    fid.write("%s " % strength)
    fid.write("%i " % int(start_time*60))
    fid.write("%i\n" % int(stop_time*60))
    fid.close()
            
def expand_tsg(opts):
    import itertools
    # read in tsg file, output extended tsg file
    # tsg format = <Src1><SrcN> <SrcType> <SrcSpecie> <SrcStrngth> <Start> <Stop>
    # Srci = ALL, NZD, or EPANET node id [str]
    # SrcType = MASS, CONCEN, FLOWPACED, or SETPOINT [str]
    # SrcSpecies = The character ID of the water quality specie added by the source [str]
    # SrcStrngth = contaminant strength, units depend on SrcType
    # Start, Stop = time in seconds [integer]
    
    fidIn = open(opts['scenario']['tsg file'],'r')
    
    prefix = opts['configure']['output prefix']
    extTSGfile = get_tempfile(prefix,'.extended.tsg')

    fidOut = open(extTSGfile,'w')
    fidOut.write('; Extended tsg file\n')
    try:
        enData = pyepanet.ENepanet()
        enData.ENopen(opts['network']['epanet file'],prefix+'epanet_tmp.rpt')
    except:
        raise RuntimeError("EPANET inp file not loaded using pyepanet")
    nnodes = enData.ENgetcount(pyepanet.EN_NODECOUNT) - enData.ENgetcount(pyepanet.EN_TANKCOUNT)
    all_nodes = [];
    nzd_nodes = [];
    for i in xrange(enData.ENgetcount(pyepanet.EN_NODECOUNT)):
        if enData.ENgetnodetype(i+1) == pyepanet.EN_JUNCTION:
            all_nodes.append(enData.ENgetnodeid(i+1))
            dem = enData.ENgetnodevalue(i+1,pyepanet.EN_BASEDEMAND)
            if dem > 0:
                nzd_nodes.append(enData.ENgetnodeid(i+1))  
    enData.ENclose()

    nScenarios = 0
    while True:
        line = fidIn.readline()
        if not line:
            break
        line = line.split()
        if len(line) == 0 or line[0] == ';':
            continue
        source_nodes = []
        for i in range(len(line)):
            if line[i] not in ['MASS', 'CONCEN', 'FLOWPACED', 'SETPOINT','mass','concen','flowpaced','setpoint']:
                if line[i] == 'ALL':
                    source_nodes.append(all_nodes)
                elif line[i] == 'NZD':
                    source_nodes.append(nzd_nodes)
                else:
                    source_nodes.append([line[i]])
            else:
                scenarios = list(itertools.product(*source_nodes))
                for j in range(len(scenarios)):
                    scenarios[j] = list(scenarios[j])
                    for k in scenarios[j]:
                        fidOut.write('%s '%k)
                    for k in range(i,len(line)):
                        fidOut.write('%s '%line[k]) 
                    fidOut.write('\n')
                break
            #nScenarios = nScenarios + len(scenarios)
    
    fidIn.close()
    fidOut.close()
    
    return extTSGfile
    
def eventDetection_merlion(opts, detection, raw_options_string=''):
    import time
    
    start = time.time()
    
    logger = logging.getLogger('wst.eventdetection')
    logger.info("Calculating detection time for each scenario using MERLION water quality model...")
    
    cmd = pyutilib.services.registered_executable('eventDetection')
    if cmd is None:
        raise RuntimeError("Cannot find the eventDetection executable on the system PATH")
    else:
        cmd = cmd.get_path() + ' '
        
    if raw_options_string != '':
        cmd += raw_options_string+' '
    
    # water quality model file
    cmd += '--inp '+opts['network']['epanet file']+' '
    
    if opts['scenario']['ignore merlion warnings'] is True:
        cmd += '--ignore-merlion-warnings '
        
    if opts['scenario']['tsi file'] not in none_list:
        cmd += '--tsi '+opts['scenario']['tsi file']+' '
    elif opts['scenario']['tsg file'] not in none_list:
        cmd += '--tsg '+opts['scenario']['tsg file']+' '
    
    # output prefix
    prefix = opts['configure']['output prefix']
    if prefix is None:
        prefix = ''
    cmd += '--output-prefix='+prefix+' '

    tmpDir = os.path.dirname(prefix)
    
    # sensor file
    if detection.__class__ is list:
        # handle the case where we are given a list of sensor names
        tmp_sensor_file = get_tempfile(prefix,'.boostersim.sensors')
        f = open(tmp_sensor_file,'w')
        for s in detection:
            print >> f, s
        f.close()
        cmd += '--sensors '+tmp_sensor_file+' '
    else:
        if detection.strip() != '':
            # handle the case where we are given a sensor file
            cmd += '--sensors '+detection+' '
    
    out = prefix+'eventDetection.out' 
    sim_timelimit = None 

    p = pyutilib.subprocess.run(cmd,timelimit=sim_timelimit,outfile=out,cwd=tmpDir)
    if (p[0]):
        raise RuntimeError("An error occured when running the eventDetection executable\n" +\
            "Error Message: "+ p[1]+"\nCommand: "+ cmd)
        
    # open EventAnalysis.txt and load array into time_detect
    try:
        enData = pyepanet.ENepanet()
        enData.ENopen(opts['network']['epanet file'],prefix+'epanet_tmp.rpt')
    except:
        raise RuntimeError("EPANET inp file not loaded using pyepanet")
            
    # convert time units for flushing app (integer minutes)
    simulationDuration = int(enData.ENgettimeparam(pyepanet.EN_DURATION)/60)
    enData.ENclose()
        
    fidIn = open(prefix+'EventAnalysis.txt','r')
    lines = fidIn.readlines()
    fidIn.close()
    time_detect = list()
    for i in range(len(lines)):
        if lines[i].strip('\n') in none_list:
            time_detect.append(simulationDuration)
        else:
            time_detect.append(int(lines[i].strip('\n'))/60)
    
    return time_detect

def eventDetection_tevasim(opts, detection):
    import pywst.tevasim.problem as tevasim
    import pywst.sim2Impact.problem as sim2Impact
    import time
    import copy
    
    start = time.time()
    
    logger = logging.getLogger('wst.eventdetection')
    logger.info("Calculating detection time for each scenario using EPANET water quality model...")

    # Run tevasim
    # ***** TODO *****: WE NEED TO AVOID A DEEPCOPY OF THE ENTIRE OPTIONS DICTIONARY
    #                   SO WE CAN REPORT TO THE USER WHAT OPTIONS WERE NOT USED
    tevasim_opts = copy.deepcopy(opts.value())
    
    tmp_prefix = opts['configure']['output prefix'] + '_tmp'
    #prefix = dirname(opts['configure']['output prefix'])
    #tmp_prefix = prefix+os.path.sep+'tmp_'
    tevasim_opts['configure']['output prefix'] = tmp_prefix
    tmpDir = os.path.dirname(tmp_prefix)

    filename = get_tempfile(tmp_prefix, '.eventDetection_tevasim.yml')
    fid = open(filename,'wt')
    fid.write(yaml.dump(tevasim_opts,default_flow_style=False))
    fid.close()
    
    out = get_tempfile(tmp_prefix, '.eventDetection_tevasim.out')
    cmd = [ pyutilib.services.registered_executable('wst'), 'tevasim', filename ]
    if cmd[0] is None:
        raise RuntimeError("Cannot find the wst executable on the system PATH")
    else:
        cmd[0] = cmd[0].get_path()
    p = pyutilib.subprocess.run(cmd,outfile=out,cwd=tmpDir)
    if p[0]:
        raise RuntimeError("Error running 'wst tevasim':\n%s" % (p[1],))

    # Run sim2Impact
    # ***** TODO *****: WE NEED TO AVOID A DEEPCOPY OF THE ENTIRE OPTIONS DICTIONARY
    #                   SO WE CAN REPORT TO THE USER WHAT OPTIONS WERE NOT USED
    sim2Impact_opts = copy.deepcopy(opts.value())
    sim2Impact_opts['configure']['output prefix'] = tmp_prefix
    sim2Impact_opts['impact']['erd file'] = [tevasim_opts['configure']['output prefix']+'.erd']
    
    filename = get_tempfile(tmp_prefix, '.eventDetection_sim2Impact.yml')
    fid = open(filename,'wt')
    fid.write(yaml.dump(sim2Impact_opts,default_flow_style=False))
    fid.close()
    
    out = get_tempfile(tmp_prefix, '.eventDetection_sim2Impact.out') 
    cmd = [ pyutilib.services.registered_executable('wst'), 'sim2Impact', filename ]
    if cmd[0] is None:
        raise RuntimeError("Cannot find the wst executable on the system PATH")
    else:
        cmd[0] = cmd[0].get_path()
    p = pyutilib.subprocess.run(cmd,outfile=out,cwd=tmpDir)
    if p[0]:
        raise RuntimeError("Error running 'wst sim2Impact':\n%s" % (p[1],))
    
    # Create a nodemap dictonary
    nodemapFile = tmp_prefix+'.nodemap'
    epanetID, nodeID = get_nodemap(nodemapFile)
    nodemapDict = dict(zip(nodeID, epanetID))
    epanetmapDict = dict(zip(epanetID, nodeID))
    
    # Create a scenario dictonary
    scenariomapFile = tmp_prefix+'.scenariomap'
    scen_epanetID, scen_nodeID = get_scenariomap(scenariomapFile)
    scenarioDict = dict(zip(scen_nodeID, scen_epanetID))
    
    # Convert sensor list to node ID
    if detection.__class__ is list:
        # handle the case where we are given a list of sensor names
        sensor_epanetID = detection        
    else:
        # handle the case where we are given a sensor file
        fidIn = open(detection, 'r')
        sensor_epanetID = fidIn.read()
        sensor_epanetID = sensor_epanetID.split('\n')
        sensor_epanetID = [x for x in sensor_epanetID if x.strip()]# remove blank entries (last)
        
    #sensor_epanetID = detection
    sensor_nodeID = list()
    for i in sensor_epanetID:
        sensor_nodeID.append(epanetmapDict[str(i)])
        
    # Read in the impact file, find detection time, store scenario:time in a dictonary
    timeDict = {} 
    impactFile = tmp_prefix+'_'+sim2Impact_opts['impact']['metric'][0].lower()+'.impact'
    fidIn = open(impactFile, 'r')
    impactLine1 = fidIn.readline()
    impactLine2 = fidIn.readline()
    while True:
        lines = fidIn.readlines(100000)
        if not lines:
            break
        lines = map(str.rstrip,lines,'\n')
        lines = map(str.split,lines,' ')
        for line in lines:
            # line = scenario, node, time, impact
            line = [int(line[0]), int(line[1]), int(line[2]), float(line[3])]
            
            if line[1] in sensor_nodeID or line[1] == -1:
                try:
                    timeDict[line[0]][1] = min(timeDict[line[0]][1],line[2])
                    if timeDict[line[0]][1] == line[2]:
                        timeDict[line[0]][0] = line[1]
                except KeyError: # first occurance
                    timeDict[line[0]] = [line[1],line[2]]
    fidIn.close()
    
    # List detection times, in minutes
    time_detect = list() 
    for i in timeDict.values():
        time_detect.append(i[1])
    
    return time_detect
    
def get_nodemap(nodemapFile):
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
    
def get_sensor(sensorFile):
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
    
def get_scenariomap(scenarioFile):
    # Open sensor file (output from sp), return sensor locations
    fid = open(scenarioFile, 'r')
    scenario = fid.read()
    fid.close()
    scenario = scenario.splitlines()
    nodeID = list()
    epanetID = list()
    for i in range(len(scenario)):
        temp = scenario[i].split(' ')
        nodeID.append(int(temp[0])) # nodeID is an integer
        epanetID.append(temp[1]) # epanetID is a string

    return epanetID, nodeID


def feasible_nodes(feasible, infeasible, max_nodes, enData):
    node_names = []
    node_indices = []

    # get some basic network information
    all_node_ids = [
        enData.ENgetnodeid(i+1)
        for i in xrange(enData.ENgetcount(pyepanet.EN_NODECOUNT)) ]
     
    # set feasible locations  
    list_feas = []
    if not max_nodes:
        # list_feas = []
        pass
    elif feasible == 'ALL':
        list_feas = [
            i for idx,i in enumerate(all_node_ids)
            if enData.ENgetnodetype(idx+1) == pyepanet.EN_JUNCTION ]
    elif feasible == 'NZD':
        list_feas = [
            i for idx,i in enumerate(all_node_ids) \
            if enData.ENgetnodetype(idx+1) == pyepanet.EN_JUNCTION \
            and enData.ENgetnodevalue(idx+1, pyepanet.EN_BASEDEMAND) > 0 ]
    elif feasible.__class__ is list:
        for i in feasible:
            if i not in all_node_ids:
                print "WARNING: feasible node '%s' not found in INP file" % i
            else:
                list_feas.append(str(i))
    elif feasible in none_list:
        # prevents entering next 'elif' block
        pass
    elif feasible.__class__ is str:
        try:
            fid = open(feasible,'r')
        except:
            raise RuntimeError("feasible nodes file did not load")
        list_feas = fid.read()
        fid.close()
        list_feas = list_feas.splitlines()
    else:
        print "Unsupported feasible nodes, setting option to None"
        
    # set infeasible locations 
    list_infeas = []
    if infeasible == 'ALL':
        list_infeas = list(all_node_ids)
    elif infeasible == 'NZD':
        list_infeas = [
            i for idx,i in enumerate(all_node_ids) \
            if enData.ENgetnodetype(idx+1) == pyepanet.EN_JUNCTION \
            and enData.ENgetnodevalue(idx+1, pyepanet.EN_BASEDEMAND) > 0 ]
    elif infeasible.__class__ is list:
        for i in infeasible:
            if i not in all_node_ids:
                print "WARNING: infeasible node '%s' not found in INP file" % i
            else:
                list_infeas.append(str(i))
    elif infeasible in none_list:
        # prevents entering next 'elif' block
        pass
    elif infeasible.__class__ is str:
        try:
            fid = open(infeasible,'r')
        except:
            raise RuntimeError("infeasible nodes file did not load")
        list_infeas = fid.read()
        fid.close()
        list_infeas = list_infeas.splitlines()
    else:
        print "Unsupported infeasible nodes, setting option to None"
        
    # remove infeasible from feasible
    final_list = []
    for i in list_feas:
        if i not in list_infeas:
            final_list.append(i)
    # set index
    all_indices = dict((id, idx+1) for idx, id in enumerate(all_node_ids))
    # assign to node parameters
    node_names = final_list
    node_indices = [all_indices[id] for id in final_list]
    
    return node_names, node_indices
    
def feasible_links(feasible, infeasible, max_pipes, enData):
    link_names = []
    link_indices = []
    
    nlinks = enData.ENgetcount(pyepanet.EN_LINKCOUNT)

    # get some basic network information
    all_link_ids = [enData.ENgetlinkid(i+1) for i in range(nlinks)]
    all_link_endpoints = dict((i+1, enData.ENgetlinknodes(i+1)) for i in range(nlinks))
      
    # set feasible locations  
    list_feas = []
    if feasible.__class__ is str:
        f = feasible.split(' ',1)
        feasible = f[0]
        if len(f) > 1:
            f = f[1].strip(); f = f.split(' ',1); minDiam = float(f[0])
            f = f[1].strip(); f = f.split(' ',1); maxDiam = float(f[0])
    if not max_pipes:
        # list_feas = []
        pass
    elif feasible == 'ALL':
        list_feas = all_link_ids
    elif feasible == 'DIAM':
        for i in range(nlinks):
            diam = enData.ENgetlinkvalue(i+1,pyepanet.EN_DIAMETER)
            if diam <= maxDiam and diam >= minDiam:
                list_feas.append(enData.ENgetlinkid(i+1))
    elif feasible.__class__ is list:
        for i in feasible:
            list_feas.append(str(i))
    elif feasible in none_list:
        # prevents entering next 'elif' block
        pass
    elif feasible.__class__ is str:
        try:
            fid = open(feasible,'r')
        except:
            raise RuntimeError("feasible nodes file did not load")
        list_feas = fid.read()
        fid.close()
        list_feas = list_feas.splitlines()
    else:
        print "Unsupported feasible pipes, setting option to None"
            
    # set infeasible locations  
    list_infeas = []
    if infeasible.__class__ is str:
        f = infeasible.split(' ',1)
        infeasible = f[0]
        if len(f) > 1:
            f = f[1].strip(); f = f.split(' ',1); minDiam = float(f[0])
            f = f[1].strip(); f = f.split(' ',1); maxDiam = float(f[0])
    if infeasible == 'ALL':
        for i in range(nlinks):
            list_infeas.append(enData.ENgetlinkid(i+1))
    elif infeasible == 'DIAM':
        for i in range(nlinks):
            diam = enData.ENgetlinkvalue(i+1,pyepanet.EN_DIAMETER)
            if diam <= maxDiam and diam >= minDiam:
                list_infeas.append(enData.ENgetlinkid(i+1))
    elif infeasible.__class__ is list:
        for i in infeasible:
            list_infeas.append(str(i))
    elif infeasible in none_list:
        # prevents entering next 'elif' block
        pass
    elif infeasible.__class__ is str:
        try:
            fid = open(infeasible,'r')
        except:
            raise RuntimeError("infeasible nodes file did not load")
        list_infeas = fid.read()
        fid.close()
        list_infeas = list_infeas.splitlines()
    else:
        print "Unsupported infeasible pipes, setting option to None"
        
    # remove infeasible from feasible
    final_list = []
    idxs = []
    for i in list_feas:
        if i not in list_infeas:
            final_list.append(i)
    # set index
    for i in range(len(final_list)):
        idxs.append(i+1)  
    # assign to link parameters
    link_names = list(final_list) #copy
    link_indices = list(idxs) #copy

    return link_names, link_indices
