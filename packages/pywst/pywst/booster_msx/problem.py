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
import pyutilib.subprocess
import yaml, json
import time
import logging

import re
import xml.dom.minidom

import pywst.common.problem
import pywst.common.wst_config as wst_config
from pyutilib.misc.config import ConfigBlock
import pywst.common.wst_util as wst_util
import pywst.visualization.inp2svg as inp2svg

try:
    import pyepanet
except ImportError:
    pass

class Problem(pywst.common.problem.Problem):

    results = { 'dateOfLastRun': '',
                'nodesToBoost': [],
                'finalMetric': -999,
                'runTime': None }
                                
    none_list = ['none','','None','NONE', None]  
    time_detect = []
    node_names = []
    node_indices = []

    defLocs = {}
    epanetOkay = False
    
    def __init__(self):
        pywst.common.problem.Problem.__init__(self, 'booster_msx', ("network", "scenario", "impact", "booster msx", "solver", "configure"))
        self.epanetOkay = False
        try:
            enData = pyepanet.ENepanet()
        except Exception as E:
            print type(E), E
            raise RuntimeError("EPANET DLL is missing or corrupt. Please reinstall PyEPANET.")
        self.epanetOkay = True
    
    def _loadPreferences(self):
        for key in defLocs.keys():
            if key == 'coliny':
                self.opts['configure']['coliny executable'] = defLocs[key]
            if key == 'dakota':
                self.opts['configure']['dakota executable'] = defLocs[key]
            if key == 'sim2Impact':
                self.opts['configure']['sim2Impact executable'] = defLocs[key]
            if key == 'tevasim':
                self.opts['configure']['tevasim executable'] = defLocs[key]

    def validate(self):
        # executables
        self._validateExecutable('tevasim')
        self._validateExecutable('sim2Impact')
        
        output_prefix = self.opts['configure']['output prefix']

        if self.opts['scenario']['merlion']:
            raise RuntimeError("Error: merlion must be False\n")
            
        if self.opts['scenario']['msx file'] in pywst.common.problem.none_list:
            raise RuntimeError("Error: msx file required\n")
        
        if not self.opts['booster msx']['type'] == 'FLOWPACED':
            raise RuntimeError("Error: booster type must be FLOWPACED for booster_msx\n")
            
        if len(self.opts['impact']['metric']) > 1:
            raise RuntimeError("Error: Only one metric used in booster msx\n")    
            
        if output_prefix == '':
            output_prefix = 'booster_msx'
            self.opts['configure']['output prefix'] = output_prefix

        return

    def run(self):
        # setup logger
        logger = logging.getLogger('wst.booster_msx')
        logger.info("WST booster_msx subcommand")
        logger.info("---------------------------")
        
        # set start time
        self.startTime = time.time()
        
        # validate input
        logger.info("Validating configuration file")
        self.validate()
        
        # open inp file, set feasible nodes        
        try:
            enData = pyepanet.ENepanet()
            enData.ENopen(self.opts['network']['epanet file'],'tmp.rpt')
        except:
            rmsg = "Error: EPANET inp file not loaded using pyepanet"
            logger.error(msg)
            raise RuntimeError(msg) 

        nlinks = enData.ENgetcount(pyepanet.EN_LINKCOUNT)    
        self.all_link_ids = [enData.ENgetlinkid(i+1) for i in range(nlinks)]
        self.all_link_endpoints = dict((i+1, enData.ENgetlinknodes(i+1)) for i in range(nlinks))

        self.node_names, self.node_indices = wst_util.feasible_nodes(\
            self.opts['booster msx']['feasible nodes'],\
            self.opts['booster msx']['infeasible nodes'], \
            self.opts['booster msx']['max boosters'], enData)
        #if len(self.node_names) == 0:
        #    logger.warn('List of feasible node locations is empty. Booster msx will default to using all nzd junctions as feasible node locations.')
        if len(self.node_names) < self.opts['booster msx']['max boosters']:
            logger.warn('Max nodes reduced to match number of feasible locations.')
            self.opts['booster msx']['max boosters'] = len(self.node_names)
        
        enData.ENclose()
        
        # write tmp TSG file if ['scenario']['tsg file'] == none
        if self.opts['scenario']['tsi file'] in pywst.common.problem.none_list:
            if self.opts['scenario']['tsg file'] in pywst.common.problem.none_list:
                tmpdir = os.path.dirname(self.opts['configure']['output prefix'])
                tmpTSGFile = pyutilib.services.TempfileManager.create_tempfile(dir=tmpdir, prefix='tmp_', suffix='.tsg')
            
                wst_util.write_tsg(self.opts['scenario']['location'],\
                                   self.opts['scenario']['type'],\
                                   self.opts['scenario']['species'],\
                                   self.opts['scenario']['strength'],\
                                   self.opts['scenario']['start time'],\
                                   self.opts['scenario']['end time'],\
                                   tmpTSGFile)
                self.opts['scenario']['tsg file'] = tmpTSGFile
            # expand tsg file
            extTSGfile = wst_util.expand_tsg(self.opts)
            self.opts['scenario']['tsg file'] = extTSGfile
            
        # get detection times
        self.time_detect = wst_util.eventDetection_tevasim(self.opts, self.opts['booster msx']['detection'])
                
        self.results = { 'dateOfLastRun': '',
                         'nodesToBoost': [],
                         'finalMetric': -999,
                         'runTime': None }
        self.results['dateOfLastRun'] = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        # define tmp filenames for booster_msx
        tmpdir = os.path.dirname(self.opts['configure']['output prefix'])
        #ftevasim = pyutilib.services.TempfileManager.create_tempfile(dir=tmpdir, prefix='tmp_', suffix='_tevasim.yml')
        #fsim2Impact = pyutilib.services.TempfileManager.create_tempfile(dir=tmpdir, prefix='tmp_', suffix='_sim2Impact.yml')
        finp = pyutilib.services.TempfileManager.create_tempfile(dir=tmpdir, prefix='tmp_', suffix='.in')
        fout = pyutilib.services.TempfileManager.create_tempfile(dir=tmpdir, prefix='tmp_', suffix='.out')
        ffwd = pyutilib.services.TempfileManager.create_tempfile(dir=tmpdir, prefix='tmp_', suffix='_fwd.py')

        _solver_type = self.opts['solver']['type']
        
        # coliny_ea solver (through DAKOTA)
        if _solver_type == 'dakota:coliny_ea' or _solver_type == 'coliny_ea':
            self.createDakotaInput(filename=finp, fwdRunFilename=ffwd)
            self.createDriverScripts(filename=ffwd, driver='dakota')
            
            logger.info('Launching Dakota ...')
            dexe = self.opts['configure']['dakota executable']
            if dexe is None:
                msg = "Error: Cannot find the dakota executable on the system PATH"
                logger.error(msg)
                raise RuntimeError(msg) 
            cmd = ' '.join([dexe,'-input',finp,'-output',fout])
            logger.debug(cmd)
            sub_logger = logging.getLogger('wst.booster_msx.dakota')
            sub_logger.setLevel(logging.DEBUG)
            fh = logging.FileHandler(fout, mode='w')
            sub_logger.addHandler(fh)
            p = pyutilib.subprocess.run(cmd,stdout=pywst.common.problem.LoggingFile(sub_logger))
            sub_logger.removeHandler(fh)
            fh.close()
            self.parseDakotaOutput(fout)

        # coliny_ea solver (through COLINY)
        elif _solver_type == 'coliny:ea':
            self.createColinyInput(
                'sco:ea',
                ['max_iterations','max_function_evaluations','population_size',
                 'initialization_type','fitness_type','crossover_rate',
                 'crossover_type','mutation_rate','mutation_type','seed'],
                 filename=finp, fwdRunFilename=ffwd)
            self.createDriverScripts(filename=ffwd, driver='coliny')
            
            logger.info('Launching Coliny ...')
            cexe = self.opts['configure']['coliny executable']
            if cexe is None:
                msg = "Error: Cannot find the coliny executable on the system PATH"
                logger.error(msg)
                raise RuntimeError(msg) 
            cmd = ' '.join([cexe,finp])
            logger.debug(cmd)
            sub_logger = logging.getLogger('wst.booster_msx.coliny')
            sub_logger.setLevel(logging.DEBUG)
            fh = logging.FileHandler(fout, mode='w')
            sub_logger.addHandler(fh)
            p = pyutilib.subprocess.run(cmd,stdout=pywst.common.problem.LoggingFile(sub_logger))
            sub_logger.removeHandler(fh)
            fh.close()
            self.parseColinyOutput(fout)
        
        # StateMachine LS (through COLINY)
        elif _solver_type == 'coliny:StateMachineLS':
            try:
                enData = pyepanet.ENepanet()
                enData.ENopen(self.opts['network']['epanet file'],'tmp.rpt')
            except:
                msg = "Error: EPANET inp file not loaded using pyepanet"
                logger.error(msg)
                raise RuntimeError(msg) 
            nJunctions = enData.ENgetcount(pyepanet.EN_NODECOUNT) \
                         - enData.ENgetcount(pyepanet.EN_TANKCOUNT)
            enData.ENclose()

            init_pts = self.opts['solver']['initial points']
            self.createStateMachineInput()
            self.createColinyInput(
                'sco:StateMachineLS',
                ['verbosity', 'max_iterations', 'max_fcn_evaluations'],
                filename=finp, fwdRunFilename=ffwd, init_pts=init_pts,repn='bin')
            self.createDriverScripts(filename=ffwd, driver='coliny', repn='bin')
            
            logger.info('Launching Coliny ...')
            cexe = self.opts['configure']['coliny executable']
            cmd = ' '.join([cexe,finp])
            logger.debug(cmd)
            sub_logger = logging.getLogger('wst.booster_msx.coliny')
            sub_logger.setLevel(logging.DEBUG)
            fh = logging.FileHandler(fout, mode='w')
            sub_logger.addHandler(fh)
            p = pyutilib.subprocess.run(cmd,stdout=pywst.common.problem.LoggingFile(sub_logger))
            sub_logger.removeHandler(fh)
            fh.close()
            self.parseColinyOutput(fout,repn='bin')
        
        elif _solver_type == 'EVALUATE':
            self.createDakotaInput(filename=finp, fwdRunFilename=ffwd)
            self.createDriverScripts(filename=ffwd, driver='dakota')
            
            logger.info('Evaluate Placement ...')
            # create dummy params.in
	    fid = open('params.in','w')
            fid.write('%d variables\n'%len(self.node_names))
            for i in range(len(self.node_names)):
                fid.write('%s x%d\n'%(i+1, i+1))
            #fid.write('1 functions\n')
            #fid.write('1 ASV_1\n')
            #fid.write('2 derivative_variables\n')
            #fid.write('1 DVV_1\n')
            #fid.write('2 DVV_2\n')
            #fid.write('0 analysis_components\n')
            fid.close()
            eval_out_file = pyutilib.services.TempfileManager.create_tempfile(dir=tmpdir, prefix='tmp_', suffix='_eval.out')
            cmd = ' '.join(['python',ffwd,'params.in',eval_out_file])
            logger.debug(cmd)
            sub_logger = logging.getLogger('wst.booster_msx.exec')
            sub_logger.setLevel(logging.DEBUG)
            fh = logging.FileHandler(fout, mode='w')
            sub_logger.addHandler(fh)
            p = pyutilib.subprocess.run(cmd,stdout=pywst.common.problem.LoggingFile(sub_logger))
            sub_logger.removeHandler(fh)
            fh.close()
            fid = open(eval_out_file,'r')
            self.results['finalMetric'] = float(fid.read())
            self.results['nodesToBoost'] = self.opts['booster msx']['feasible nodes']
            fid.close()

        else:
            raise Exception("ERROR: Unknown or unsupported solver, '%s'" % _solver_type)
        
        # remove temporary files if debug = 0
        if self.opts['configure']['debug'] == 0:
            pyutilib.services.TempfileManager.clear_tempfiles()
            if os.path.exists('./hydraulics.hyd'):
                os.remove('./hydraulics.hyd')
            if os.path.exists('./tmp.rpt'):
                os.remove('./tmp.rpt')
            
        # write output file 
        prefix = os.path.basename(self.opts['configure']['output prefix'])      
        logfilename = logger.parent.handlers[0].baseFilename
        outfilename = logger.parent.handlers[0].baseFilename.replace('.log','.yml')
        visymlfilename = logger.parent.handlers[0].baseFilename.replace('.log','_vis.yml')
        
        # Write output yml file
        config = wst_config.output_config()
        module_blocks = ("general", "booster")
        template_options = {
            'general':{
                'cpu time': round(time.time() - self.startTime,3),
                'directory': os.path.dirname(logfilename),
                'log file': os.path.basename(logfilename)},
            'booster': {
                'nodes': self.results['nodesToBoost'],
                'objective': self.results['finalMetric']} }
        self.saveOutput(outfilename, config, module_blocks, template_options)
        
        # Write output visualization yml file
        config = wst_config.master_config()
        module_blocks = ("network", "visualization", "configure")
        template_options = {
            'network':{
                'epanet file': os.path.abspath(self.opts['network']['epanet file'])},
            'visualization': {
                'layers': []},
            'configure': {
                'output prefix': os.path.abspath(self.opts['configure']['output prefix'])}}
        if len(self.opts['booster msx']['detection']) > 0:
            template_options['visualization']['layers'].append({
                    'label': 'Sensor locations', 
                    'locations': self.opts['booster msx']['detection'],
                    'location type': 'node',  
                    'shape': 'square',             
                    'fill': {
                        'color': '#000099"',
                        'size': 15,
                        'opacity': 0},
                    'line': {
                        'color': '#000099',
                        'size': 2,
                        'opacity': 0.6}})
        if len(self.results['nodesToBoost']) > 0:
            template_options['visualization']['layers'].append({
                    'label': 'Booster locations', 
                    'locations': "['booster']['nodes'][i]",
                    'file': outfilename,
                    'location type': 'node',  
                    'shape': 'circle',             
                    'fill': {
                        'color': '#aa0000',
                        'size': 15,
                        'opacity': 0.6},
                    'line': {
                        'color': '#000099',
                        'size': 1,
                        'opacity': 0}})
        if visymlfilename != None:
            self.saveVisOutput(visymlfilename, config, module_blocks, template_options)        
        
        # Run visualization
        cmd = ['wst', 'visualization', visymlfilename]
        p = pyutilib.subprocess.run(cmd) # logging information should not be printed to the screen
        
        # print solution to screen
        logger.info("\nWST Normal Termination")
        logger.info("---------------------------")
        logger.info("Directory: "+os.path.dirname(logfilename))
        logger.info("Results file: "+os.path.basename(outfilename))
        logger.info("Log file: "+os.path.basename(logfilename))
        logger.info("Visualization configuration file: "+os.path.basename(visymlfilename)+'\n')
        return
        
    def createDakotaInput(self, filename=None, fwdRunFilename=None):
        if filename is None: 
            filename = self.opts['configure']['output prefix']+'_dakota.in'
        fid = open(filename,'wt')
        fid.write(dakota_template_a)
                
        dakota_method = '''method
         max_iterations = %s
         max_function_evaluations = %s
         coliny_ea
          population_size = %s
          initialization_type
           %s
          fitness_type
           %s
          replacement_type
           elitist = 1
          crossover_rate = %s
          crossover_type
           %s
          mutation_rate = %s
          mutation_type
           %s
          seed = %s
        '''

        fid.write('method\n')
        for x in ('max_iterations','max_function_evaluations'):
            if x in self.opts['solver']['options']:
                fid.write(" %s = %s\n" % ( x, self.opts['solver']['options'][x] ))
        fid.write(' coliny_ea\n  replacement_type\n   elitist = 1\n')
        for x in ('population_size','crossover_rate','mutation_rate','seed'):
            if x in self.opts['solver']['options']:
                fid.write("  %s = %s\n" % (x, self.opts['solver']['options'][x] ))
        for x in ('initialization_type','fitness_type','crossover_type','mutation_type'):
            if x in self.opts['solver']['options']:
                fid.write("  %s %s\n" % (x, self.opts['solver']['options'][x] ))

        fid.write(dakota_template_b)
        nNodes = self.opts['booster msx']['max boosters']
        nVars = nNodes
        fid.write(' discrete_design_range = %d\n'%nVars)
        fid.write('  lower_bounds =')
        for i in range(nVars):
            fid.write(' 1')
        fid.write('\n')
        fid.write('  upper_bounds =')
        for i in range(nNodes):
            fid.write(' '+str(len(self.node_indices)))
        fid.write('\n')
        fid.write('  descriptors =')
        for i in range(nNodes):
            fid.write(" 'bs"+str(i)+"'")
        fid.write('\n')
        if fwdRunFilename is None:
            fwdRunFilename=self.opts['configure']['output prefix']+'_fwd.py'
        if os.name in ['nt','dos']:
            fwdRunFilename = fwdRunFilename.replace('\\','/')
            fwdRunFilename = fwdRunFilename.replace('.py','.bat')
            fid.write(dakota_template_c%(fwdRunFilename))
        else:
            fid.write(dakota_template_c%(sys.executable+' '+fwdRunFilename))
        fid.close()
        
    def createColinyInput(self, solver, validOptions, filename=None, fwdRunFilename=None, init_pts=None, repn='idx'):
        def pushNode(name):
            elt.append(doc.createElement(name))

        def popNode():
            node = elt.pop()
            elt[-1].appendChild(node)

        def setAttr(name, value):
            elt[-1].setAttribute(str(name), str(value))

        def addText(data):
            elt[-1].appendChild(doc.createTextNode(str(data)))
            
        nNodes = self.opts['booster msx']['max boosters']

        nodeUB = len(self.node_indices)

        doc = xml.dom.minidom.Document()
        root = doc.createElement("ColinInput")
        doc.appendChild(root)
        elt = [root]

        pushNode("CacheFactory")
        setAttr("default_cache_type","Local")
        pushNode("UnifiedGlobalCache")
        popNode()
        popNode()

        pushNode("CacheView")
        setAttr("type", "Pareto")
        setAttr("id","BestPoint")
        popNode()

        pushNode("Problem")
        setAttr("type","UINLP")
        pushNode("Objective")
        setAttr("sense","min")
        popNode()
        pushNode("Domain")
        pushNode("IntVars")
        if repn == 'bin':
            # The extra 2 variables are to count "placed" nodes 
            nVars = nodeUB + 1
            setAttr('num', nVars)
            pushNode("Lower")
            setAttr("value", 0)
            popNode()
            pushNode("Upper")
            setAttr("value", 1)
            popNode()
            pushNode("Upper")
            setAttr("index", nVars)
            setAttr("value", nNodes)
            popNode()
        elif repn == 'idx':
            nVars = nNodes 
            setAttr('num', nVars)
            # All lower bounds are 0
            pushNode("Lower")
            setAttr("value", 0)
            popNode()
            for i in range(nNodes):
                pushNode("Upper")
                setAttr("index", i+1)
                setAttr("value", nodeUB)
                popNode()
        else:
            raise Exception("Unknown problem representation")
        popNode()
        popNode()
        pushNode("Driver")
        pushNode("Command")
        if fwdRunFilename is None:
            fwdRunFilename=self.opts['configure']['output prefix']+'_fwd.py'
        addText(sys.executable+' '+fwdRunFilename)
        popNode()
        popNode()
        popNode()

        pushNode("Solver")
        setAttr("type", solver)
        pushNode("Options")
        availableOptions = self.opts['solver']['options']
        for opt in validOptions:
            if availableOptions and opt in availableOptions:
                pushNode("Option")
                setAttr("name", opt)
                addText(availableOptions[opt])
                popNode()
        popNode()
        popNode()

        pushNode("Execute")
        pushNode("solve:default")
        if init_pts:
            pushNode("InitialPoint")
            for pt in init_pts:
                pushNode("Point")
                if repn == 'bin':
                    data = [0]*nVars
                    _count = 0
                    for n in pt.get('nodes',[]):
                        if n in self.node_names:
                            _n = n
                        elif str(n) in self.node_names:
                            _n = str(n)
                        else:
                            if n:
                                print "ERROR: unknown initial node '%s'; skipping" % n
                            continue
                        
                        if data[self.node_names.index(_n)]:
                            print "ERROR: duplicate initial node '%s'; skipping" % n
                        else:
                            data[self.node_names.index(_n)] = 1
                            _count += 1
                    data[nVars-1] = _count

                    addText( ('i(%d: '%len(data))
                             + ','.join(str(x) for x in data)
                             +')' )
                else:
                    raise Exception("Implement me")
                popNode()
            popNode()
        popNode()
        pushNode("PrintCache")
        setAttr("cache", "BestPoint")
        popNode()
        popNode()

        if len(elt) != 1:
            raise Exception("Stack imbalance when generating COLIN input")

        if filename is None: 
            filename = self.opts['configure']['output prefix']+'_coliny.in'
        fid = open(filename,'wt')
        doc.writexml(fid," "," ","\n","UTF-8")
        fid.close()

    def createStateMachineInput(self):
        #
        # NB: Node, Pipe indices are 1-based, but colin is 0-based
        #
        
        nNodes = self.opts['booster msx']['max boosters']
        maxNIDs = len(self.node_indices)
        freeNode = maxNIDs
        fid = open('StateMachineLS.states', 'wt')
        #fid.write("# Nodes: add/remove\n")
        # If not all the nodes are on, then we can turn them on
        for i in xrange(nNodes):
            for var in xrange(maxNIDs):
                # An "off" node can be turned on to any node
                fid.write("s%d,%d:%d,0,=|%d,%d;%d,1\n" %
                            (freeNode, i, var, freeNode, i+1, var))
            #if self.opts['booster msx']['allow location removal']:
            #    for var in xrange(maxNIDs):
            #        # An "on" node can be turned off
            #        fid.write("s%d,1:%d,%d,=|%d,%d;%d,0\n" %
            #                  (var, freeNode, i+1, freeNode, i, var))
        #fid.write("# Nodes: adjacent moves\n")
        # Build the lists of adjacent nodes to each booster node
        validNodes = dict((id, i) for i, id in enumerate(self.node_indices))
        adjacentNodes = {}
        for l, n in self.all_link_endpoints.iteritems():
            # each link is bi-directional
            adjacentNodes.setdefault(n[0], set()).add(n[1])
            adjacentNodes.setdefault(n[1], set()).add(n[0])
        # Prune out the unavailable nodes
        _deletionQueue = []
        for src, adj in adjacentNodes.iteritems():
            if src in validNodes:
                continue
            _deletionQueue.append(src)
            for dest in adj:
                adjacentNodes[dest].update(adj)
                adjacentNodes[dest].discard(src)
                adjacentNodes[dest].discard(dest)
        for _n in _deletionQueue:
            del adjacentNodes[_n]
        for src, adj in adjacentNodes.iteritems():
            for dest in adj:
                s = validNodes[src]
                d = validNodes[dest]
                fid.write("s%d,1:%d,0,=|%d,0;%d,1\n" % (s,d,s,d))
        fid.close()
            
    def createDriverScripts(self, filename=None, driver=None, repn='idx'):
        try:
            enData = pyepanet.ENepanet()
            enData.ENopen(self.opts['network']['epanet file'],'tmp.rpt')
        except:
            raise RuntimeError("EPANET inp file not loaded using pyepanet")
            
        # convert time unit to pattern units for msx file
        import math
        patternTimestep = enData.ENgettimeparam(pyepanet.EN_PATTERNSTEP) # returns timestep (in seconds)
        simulationDuration = math.floor(int(enData.ENgettimeparam(pyepanet.EN_DURATION)/patternTimestep))
        enData.ENclose()

        responseTime = math.floor(self.opts['booster msx']['response time']/(patternTimestep/60))
        duration = math.floor(self.opts['booster msx']['duration']/(patternTimestep/60))
        timeDetect = [int(math.floor(x/(patternTimestep/60))) for x in self.time_detect]

        if filename is None: 
            filename = self.opts['configure']['output prefix']+'_fwd.py'
        fid = open(filename,'w')
        fid.write('#!'+sys.executable+'\n')
        fid.write(python_template_a)
        fid.write('verbose = %d\n' % (self.opts['solver']['verbose'],))
        # booster options
        fid.write('timeDetect = %r\n' % (timeDetect,))
        fid.write('responseDelay = %d\n'%responseTime)
        fid.write('boostConc = %r\n' % (self.opts['booster msx']['strength'],))
        fid.write('boostLength = %d\n' % (duration,))
        fid.write('simDuration = %d\n' % (simulationDuration,))
        fid.write('toxinSpecies = %r\n' % (self.opts['booster msx']['toxin species'],))
        fid.write('deconSpecies = %r\n' % (self.opts['booster msx']['decon species'],))
        fid.write('maxNodes = %d\n' % (self.opts['booster msx']['max boosters'],))
        fid.write('nodeMap = %r\n' % (self.node_names,))
        fid.write('prefix = %r\n' %'tmp') # (self.opts['configure']['output prefix'],))
        fid.write('prefixDir = r"%s"\n'%os.path.dirname(self.opts['configure']['output prefix']))
        # network options
        fid.write('epanetInputFile = r"%s"\n'%os.path.abspath(self.opts['network']['epanet file']))
        # scenario options
        if self.opts['scenario']['tsi file'] not in pywst.common.problem.none_list:
            fid.write('scenarioFile = %r\n' % (self.opts['scenario']['tsi file'],))
        else:
            fid.write('scenarioFile = %r\n' % (self.opts['scenario']['tsg file'],))
        fid.write('msxFile = r"%s"\n'%os.path.abspath(self.opts['scenario']['msx file']))
        fid.write('merlion = %r\n' % (self.opts['scenario']['merlion'],))
        fid.write('erdCompression = %r\n' % (self.opts['scenario']['erd compression'],))
        fid.write('merlionNsims = %r\n' % (self.opts['scenario']['merlion nsims'],))
        fid.write('ignoreMerlionWarnings = %r\n' % (self.opts['scenario']['ignore merlion warnings'],))
        # impact options
        fid.write('metric = %r\n' % (self.opts['impact']['metric'][0].lower(),))
        if self.opts['impact']['tai file'] not in pywst.common.problem.none_list:
            fid.write('healthImpactsFile = r"%s"\n'%os.path.abspath(self.opts['impact']['tai file']))
        else:
            fid.write('healthImpactsFile = r"%s"\n'%self.opts['impact']['tai file'])
        fid.write('responseTime = %r\n' % (self.opts['impact']['response time'],))
        fid.write('detectionLimit = %r\n' % (self.opts['impact']['detection limit'],))
        fid.write('detectionConfidence = %r\n' % (self.opts['impact']['detection confidence'],))
        # executables
        fid.write('tevasimexe = %r\n' % (self.opts['configure']['tevasim executable'],))
        fid.write('tso2Impactexe = %r\n' % (self.opts['configure']['sim2Impact executable'],))
        fid.write('driver = %s_Problem("%s")\n' % (driver, repn))
        fid.write('\n\n')
        fid.write(python_template_b)
        fid.close()
        #cmdfilename = self.opts['configure']['output prefix']+'_dakota.cmd'
        #fid = open(cmdfilename,'wt')
        #fid.write('"%s" -input "%s" -output "%s"'%(self.opts['configure']['dakota executable'],
        #                self.opts['configure']['output prefix']+'_dakota.in',
        #                self.opts['configure']['output prefix']+'_dakota.out'))
        #fid.close()
        if os.name in['nt','dos']:
            batfilename = filename.replace('.py','.bat')
            fid = open(batfilename,'wt')
            fid.write('@echo off\n')
            fid.write('"%s" "%s" %%*%%'%(sys.executable,filename))
            fid.close()
        else:
            os.chmod(filename,0755)
            #os.chmod(cmdfilename,0755)
        return

    def parseDakotaOutput(self, file, filename=None, resultsfile=None):        
        #file = self.opts['configure']['output prefix']+'_dakota.out'
        fid = open(file,'r')
        lines = fid.readlines()
        fid.close()
        for lidx in range(len(lines)):
            tokens = lines[lidx].split()
            if len(tokens) < 1: continue
            if tokens[0] != r'<<<<<': continue
            if tokens[2].strip() == 'parameters':
                maxNodes = self.opts['booster msx']['max boosters']
                nodeMap = self.node_names
                for i in range(maxNodes):
                    line = lines[lidx+1+i]
                    (v,n) = line.split()
                    nodeIdx = nodeMap[int(v)-1]
                    self.results['nodesToBoost'].append(nodeIdx)
                nodeSet = list(set(self.results['nodesToBoost']))
                self.results['nodesToBoost'] = nodeSet
            if tokens[2].strip() == 'objective':
                self.results['finalMetric'] = float(lines[lidx+1])
        resultText = yaml.dump(self.results, default_flow_style=False)
        if resultsfile is None:
            filename = self.opts['configure']['output prefix']+'.out'
        else: filename = resultsfile
        if filename is None:
            print resultText
            
        else:
            fid = open(filename,'w')
            fid.write(resultText)
            fid.close()
        return

    def parseColinyOutput(self, file, repn='idx'):        
        #file = self.opts['configure']['output prefix']+'_coliny.out'
        fid = open(file,'r')
        lines = fid.readlines()
        fid.close()
        done = None
        foundCache = False
        foundBestCache = False
        for line in lines:
            line_ = line.strip()
            if not foundCache:
                if line_.startswith('Cache:'):
                    foundCache = True
                    foundBestCache = False
                else:
                    continue
            if not foundBestCache:
                if line_.startswith('name:'):
                    if line_.split(":",1)[1].strip() == 'BestPoint':
                        if done:
                            raise Exception("Multiple BestPoint caches found in Coliny output!")
                        foundBestCache = True
                    else:
                        foundCache = False
                continue
            if line_.startswith('size:'):
                if line_.split(":",1)[1].strip() != '1':
                    sys.stderr.write("WARNING: Multiple values found in the BestPoint cache!"
                                     "\n\tArbitrarily returning the first solution\n")
                    #raise Exception("BestPoint cache had != 1 value!")
            if line_.startswith('domain:') and not done:
                done = re.search("i\(\d+\s*:\s*(.*)\)", line.split(":",1)[1].strip())
                if not done:
                    raise Exception("Error parsing coliny BestPoint cache")
                vars = [int(x) for x in done.group(1).split() if x]
                if repn == 'bin':
                    nNodes = len(self.node_indices)
                    self.results['nodesToBoost'] = sorted(
                        [self.node_names[i] for i in range(len(self.node_indices)) if vars[i]])
                elif repn == 'idx':
                    maxNodes = self.opts['booster msx']['max boosters']
                    if len(vars) != maxNodes:
                        raise Exception("Bad number of results returned from Coliny (%s != %s)"
                                        % (len(vars), maxNodes))
                    tmp = set(vars[:maxNodes])
                    if 0 in tmp:
                        tmp.remove(0)
                    self.results['nodesToBoost'] = sorted([self.node_names[i-1] for i in tmp])
                else:
                    raise Exception("Unknown problem representation")
            if line_.strip().startswith('objective function:'):
                self.results['finalMetric'] = float(line.split(":",1)[1].strip())
            if line_.strip().startswith('Annotations:'):
                foundCache = False
                
        return


python_template_a = '''import os, sys, xml.dom.minidom

def log(msg):
    sys.stdout.write(msg)
    sys.stdout.flush()

class Problem():
    function_values = []
    var_names = []
    var_values = []
    analysis_components = []
    
    def __init__(self):
        pass
        
    def function_value(self):
        scenarioFmt = scenarioFile.split('.')[-1]
        fid = open(scenarioFile,'r')
        tsg = fid.readlines()
        fid.close()

        sum = 0
        count = 0
        
        cwd = os.getcwd()
        if prefixDir:
            os.chdir(prefixDir)
                
        for i in range(len(tsg)-1):
            # create single scenario tsg file
            singleScenarioFile = '%s.tsg'%prefix
            fid_tsg = open(singleScenarioFile,'w')
            fid_tsg.write(tsg[i+1])
            fid_tsg.close()
            
            fid = open(msxFile, 'r')
            MSX = fid.read()
            fid.close()
            MSX = MSX.splitlines()
            pattern = [0] * simDuration
            pattern[timeDetect[i]+responseDelay:timeDetect[i]+responseDelay+boostLength] = [1]*boostLength
            pattern = pattern[0:simDuration]
            pattern = [99]+pattern # set pattern number to 99
            nodes = list(set(self.var_values[:maxNodes]))
            tmp_msxFile = os.path.join(os.path.dirname(msxFile),'tmp_'+os.path.basename(msxFile))
            fid = open(tmp_msxFile, 'w')
            foundSource = 0
            foundPattern = 0
            for i in range(len(MSX)):
                print >>fid, '%s'%MSX[i]
                if MSX[i] == '[SOURCES]':
                    foundSource = 1
                    for node in nodes:
                        print >>fid, 'FLOWPACED %s %s %d 99' %(nodeMap[(int(node))-1],deconSpecies,boostConc)
                        if verbose: print '%s ' %nodeMap[(int(node))-1]
                if MSX[i] == '[PATTERNS]':
                    foundPattern = 1
                    for i in range(len(pattern)):
                        print >>fid, '%i ' %pattern[i],
                    print >> fid, ''
            if foundSource == 0:
                print >> fid, ''
                print >>fid, '[SOURCES]'
                for node in nodes:
                        print >>fid, 'FLOWPACED %s %s %d 99' %(nodeMap[(int(node))-1],deconSpecies,boostConc)
                        if verbose: print '%s ' %nodeMap[(int(node))-1]
            if foundPattern == 0:
                print >> fid, ''
                print >>fid, '[PATTERNS]'
                for i in range(len(pattern)):
                    print >>fid, '%i ' %pattern[i],
                print >> fid, ''
            fid.close()
            if verbose: print '] ... tevasim ... ',
            scmd = '%s '%(tevasimexe)
            scmd = scmd + ' --%s=%s --msx %s --mss %s'%(scenarioFmt,singleScenarioFile,tmp_msxFile,toxinSpecies)
            
            # ADDED for full tevasim functionality
            if merlion is True:
                scmd = scmd + " --merlion"
                scmd = scmd + " --merlion-nsims="+str(merlionNsims)
                if ignoreMerlionWarnings is True:
                    scmd += " --merlion-ignore-warnings"
            if erdCompression in ['rle','RLE']:
                scmd += " --rle"
            elif erdCompression in ['none','','None','NONE', None]:
                if merlion:
                    scmd += " --rle"
                    
            scmd = scmd + ' %s %s.rpt %s'%(epanetInputFile,epanetInputFile,prefix)
            sys.stdout.flush()
            os.system(scmd+'>'+prefix+'_tevasim.out')
            sys.stdout.flush()
            if verbose: print 'tso2Impact ... ',
            icmd = '%s --%s --species %s'%(tso2Impactexe,metric,toxinSpecies)
            if metric in ['ec','tec']:
                icmd = icmd
            elif metric in ['pd','pe','pk']:
                icmd = icmd + ' --tai=%s'%(healthImpactsFile)
            else:
                icmd = icmd
            
            # ADDED for full sim2Impact functionality
            if detectionConfidence not in ['none','','None','NONE', None]:
                icmd = icmd + " --detectionConfidence=" + str(detectionConfidence)
            for s in detectionLimit:
                icmd = icmd + " --detectionLimit=" + str(s)
            if responseTime not in ['none','','None','NONE', None]:
                icmd = icmd + " --responseTime=" + str(responseTime)
                
            icmd = icmd + ' %s %s.erd '%(prefix,prefix)
            sys.stdout.flush()
            os.system(icmd+'>'+prefix+'_sim2Impact.out')
            sys.stdout.flush()
            try:
                fid = open('%s_%s.impact'%(prefix,metric),'r')
                hdr = fid.readline()
                hdr = fid.readline()
                lines = fid.readlines()
                fid.close()
            except:
                sum = float('inf')
                count = 1
                lines = []
            for line in lines:
                vars = line.split()
                if len(vars) < 4: continue
                if vars[1] == '-1':
                    count = count + 1
                    sum = sum + float(vars[3])
                    if verbose: print float(vars[3])
        
        if prefixDir:
            os.chdir(cwd)
            
        meanObj = float(sum)/float(count)
        if verbose: print meanObj
        self.function_values.append(meanObj)
        sys.stdout.flush()
        return

class dakota_Problem(Problem):
    def __init__(self, repn):
        if repn != 'idx':
            raise Exception("ERROR: Dakota driver only handles index-based problem representations")
    
    def reporter(self, filename):
        fid = open(filename,'w')
        for v in self.function_values:
            print >>fid, v
        fid.close()
        return
    
    def parser(self, filename):
        fid = open(filename,'r')
        lines = fid.readlines()
        fid.close()
        case = ''
        nInCase = 0
        nDone = 0
        for line in lines:
            (v, n) = line.strip().split()
            if n == 'variables':
                case = n
                nInCase = int(v)
            elif n == 'functions':
                nInCase = 0
            elif n == 'ASV_1':
                nInCase = 0
            elif n == 'derivative_variables':
                nInCase = int(v)
                case = n
            elif n == 'analysis_components':
                nInCase = int(v)
                case = n
            elif nInCase:
                if case == 'variables':
                    self.var_names.append(n)
                    self.var_values.append(float(v))
                    nDone = nDone + 1
                    if nDone >= nInCase:
                        nInCase = 0
                        nDone = 0
                        case = ''
                elif case == 'derivative_variables':
                    self.derivative_variables[n] = v
                    nDone = nDone + 1
                    if nDone >= nInCase:
                        nInCase = 0
                        nDone = 0
                        case = ''
                elif case == 'analysis_components':
                    self.analysis_components[n] = v
                    nDone = nDone + 1
                    if nDone >= nInCase:
                        nInCase = 0
                        nDone = 0
                        case = ''
        return
        
class coliny_Problem(Problem):
    def __init__(self, repn):
        self.repn = repn
    
    def reporter(self, filename):
        fid = open(filename,'w')
        fid.write('<ColinResponse><FunctionValue>'
                  + ' '.join(["%0.12g" % x for x in self.function_values])
                  + '</FunctionValue></ColinResponse>\\n')
        fid.close()
        return
    
    def parser(self, filename):
        input_doc = xml.dom.minidom.parse(filename)
        domain = input_doc.getElementsByTagName("Domain")[0]
        self.var_values = []
        for field in 'Reals','Integer','Binary':
            for node in domain.getElementsByTagName(field):
                txt = '';
                for n in node.childNodes:
                    if n.nodeType in (n.TEXT_NODE, n.CDATA_SECTION_NODE):
                        txt += ' '+n.data
                for val in txt.strip().split():
                    if val:
                        self.var_values.append(float(val))
        # spot-check the request
        domain = input_doc.getElementsByTagName("Requests")[0]
        for node in domain.childNodes:
            if node.tagName == 'FunctionValue':
                continue
            sys.stderr.write("WARNING: unexpected (and uncomputable) request: %%s\\n"
                             % (node.tagName,) )
        if self.repn == 'bin':
            nnodes = len(nodeMap)
            nvals = [i+1 for i in range(nnodes) if self.var_values[i]]
            while len(nvals) < maxNodes:
                nvals.append(0)
            self.var_values = nvals 
        return
        
'''
# verbose
# timeDetect
# responseDelay
# valveDelay
# boostConc
# boostLength
# simDuration
# maxNodes
# maxPipes
# nodeMap
# pipeMap
# prefix
# prefixDir
# scenarioFmt
# scenarioFile
# msxFile
# toxinSpecies
# deconSpecies
# merlion
# merlionNsims
# ignoreMerlionWarnings
# erdCompression
# detectionConfidence 
# detectionLimit
# responseTime 
# epanetInputFile
# metfir
# healthImpactsFile
python_template_b = '''

if __name__ == '__main__':
  try:
    problem = driver
    problem.parser(sys.argv[1])
    problem.function_value()
    problem.reporter(sys.argv[2])
  except:
    print "ERROR running %s" % (sys.argv,)
    os.system('cat %s' % (sys.argv[1],))
    raise

'''

dakota_template_a = '''# DAKOTA INPUT FILE - based on dakota_rosenbrock_ea_opt.in
strategy
 single_method
'''
dakota_method_default = '''method
 max_iterations = 100
 max_function_evaluations = 2000
 coliny_ea
  population_size = 50
  initialization_type
   unique_random
  fitness_type
   linear_rank
  replacement_type
   elitist = 1
  crossover_rate = 0.8
  crossover_type
   uniform
  mutation_rate = 1.0
  mutation_type
   offset_uniform
  seed = 11011011
'''
dakota_template_b = '''model
 single
variables
'''
# discrete_design_range = 10
#  lower_bounds = 1 1 1 1 1 1 1 1 1 1
#  upper_bounds = nboost nboost nboost nboost nboost nboost nboost nboost nboost nboost
#  descriptors = '1' '2' '3' '4' '5' '6' '7' '8' '9' '10'
dakota_template_c = '''interface
 analysis_drivers = '%s'
  system
   parameters_file = 'tevaopt.in'
   results_file = 'results.out'
responses
 num_objective_functions = 1
 no_gradients
 no_hessians
'''
