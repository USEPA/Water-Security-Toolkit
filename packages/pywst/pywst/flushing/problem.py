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
import yaml
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

def pointToIndexList(pt, node_names, link_names):
    node_idx = []
    _nodeList = pt.get('nodes',None)
    if _nodeList is None:
        _nodeList = []
    else:
        _nodeList = _nodeList.value()
    for n in _nodeList:
        if n in node_names:
            _n = n
        elif str(n) in node_names:
            _n = str(n)
        else:
            if n:
                print "ERROR: unknown initial node '%s'; skipping.\nValid nodes:\n%s" % (n, '\n'.join(node_names))
            continue

        _nid = node_names.index(_n)
        if _nid in node_idx:
            print "ERROR: duplicate initial node '%s'; skipping" % n
        else:
            node_idx.append(_nid)

    pipe_idx = []
    _pipeList = pt.get('pipes',None)
    if _pipeList is None:
        _pipeList = []
    else:
        _pipeList = _pipeList.value()
    for p in _pipeList:
        if p in link_names:
            _p = p
        elif str(p) in link_names:
            _p = str(p)
        else:
            if p:
                print "ERROR: unknown initial closed pipe '%s'; skipping" % p
            continue

        _pid = data[link_names.index(_p)]
        if _pid in pipe_idx:
            print "ERROR: duplicate initial closed pipe '%s'; skipping" % p
        else:
            pipe_idx.append(_pid)

    return node_idx, pipe_idx


def pointToBinaryList(pt, nVars, node_names, link_names):
    node_idx, pipe_idx = pointToIndexList(pt, node_names, link_names)

    data = [0]*nVars

    for n in node_idx:
        data[n] = 1
    data[-2] = len(node_idx)

    for p in pipe_idx:
        data[p+nodeUB] = 1
    data[-1] = len(pipe_idx)

    return data


class Problem(pywst.common.problem.Problem):

    results = { 'dateOfLastRun': None,
                'nodesToFlush': [],
                'pipesToClose': [],
                'finalMetric': -999,
                'runTime': None }
                
    none_list = ['none','','None','NONE', None]  
    time_detect = []
    node_names = []
    link_names = []
    node_indices = []
    link_indices = []
    
    defLocs = {}
    epanetOkay = False
    
    def __init__(self):
        pywst.common.problem.Problem.__init__(self, 'flushing', ("network", "scenario", "impact", "flushing", "solver", "configure"))
        self.epanetOkay = False
        try:
            enData = pyepanet.ENepanet()
        except:
            _t, _e = sys.exc_info[:2]
            print _t, _e
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
        
        if self.opts['scenario']['tsi file'] not in pywst.common.problem.none_list:
            msg = "Error: flushing currently not compatible with TSI scenario format"
            logger.error(msg)
            raise RuntimeError(msg) 
            
        if output_prefix == '':
            output_prefix = 'flush'
            self.opts['configure']['output prefix'] = output_prefix

        return
        
    def run(self):
        # setup logger
        logger = logging.getLogger('wst.flushing')
        logger.info("WST flushing subcommand")
        logger.info("---------------------------")
        
        # set start time
        self.startTime = time.time()
    
        # validate input
        logger.info("Validating configuration file")
        self.validate()

        tmpDir = os.path.dirname(self.opts['configure']['output prefix'])
        
        # open inp file, set feasible nodes and links
        try:
            cwd = os.getcwd()
            os.chdir(tmpDir)
            enData = pyepanet.ENepanet()
            enData.ENopen(
                self.opts['network']['epanet file'],
                self.opts['configure']['output prefix']+'epanet_tmp.rpt' )
        except:
            msg = "Error: EPANET inp file not loaded using pyepanet"
            logger.error(msg)
            raise RuntimeError(msg)
        finally:
            os.chdir(cwd)

        nlinks = enData.ENgetcount(pyepanet.EN_LINKCOUNT)    
        self.all_link_ids = [enData.ENgetlinkid(i+1) for i in range(nlinks)]
        self.all_link_endpoints = dict((i+1, enData.ENgetlinknodes(i+1)) for i in range(nlinks))
            
        self.node_names, self.node_indices = wst_util.feasible_nodes(\
            self.opts['flushing']['flush nodes']['feasible nodes'],\
            self.opts['flushing']['flush nodes']['infeasible nodes'], \
            self.opts['flushing']['flush nodes']['max nodes'], enData)
        #if len(self.node_names) == 0:
        #    logger.warn('List of feasible node locations is empty. Flushing will default to using all nzd junctions as feasible node locations.')
        if len(self.node_names) < self.opts['flushing']['flush nodes']['max nodes']:
            logger.warn('Max nodes reduced to match number of feasible locations.')
            self.opts['flushing']['flush nodes']['max nodes'] = len(self.node_names)
        
        self.link_names, self.link_indices = wst_util.feasible_links(\
            self.opts['flushing']['close valves']['feasible pipes'],\
            self.opts['flushing']['close valves']['infeasible pipes'], \
            self.opts['flushing']['close valves']['max pipes'], enData)
        #if len(self.link_names) == 0:
        #    logger.warn('List of feasible pipe locations is empty. Flushing will default to using all pipes as feasible pipe locations.')
        if len(self.link_names) < self.opts['flushing']['close valves']['max pipes']:
            logger.warn('Max pipes reduced to match number of feasible locations.')
            self.opts['flushing']['close valves']['max pipes'] = len(self.link_names)
        
        enData.ENclose()

        # write tmp TSG file if ['scenario']['tsg file'] == none
        if self.opts['scenario']['tsi file'] in pywst.common.problem.none_list:
            if self.opts['scenario']['tsg file'] in pywst.common.problem.none_list:
                tmpTSGFile = pyutilib.services.TempfileManager.create_tempfile(dir=tmpDir, prefix='tmp_', suffix='.tsg')
            
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
        if self.opts['scenario']['merlion']:
            self.time_detect = wst_util.eventDetection_merlion(self.opts, self.opts['flushing']['detection'])
        else:
            self.time_detect = wst_util.eventDetection_tevasim(self.opts, self.opts['flushing']['detection'])

        if False:
            _tmp = {}
            for _t in self.time_detect:
                _tmp[_t] = _tmp.get(_t,0) + 1
            self.time_detect = [ sorted(_tmp, key=_tmp.get)[-1] ]*len(self.time_detect)

        self.results = { 'dateOfLastRun': '',
                         'nodesToFlush': [],
                         'pipesToClose': [],
                         'finalMetric': -999,
                         'runTime': None }
        self.results['dateOfLastRun'] = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        # define tmp filenames for flushing
        #ftevasim = pyutilib.services.TempfileManager.create_tempfile(dir=tmpdir, prefix='tmp_', suffix='_tevasim.yml')
        #fsim2Impact = pyutilib.services.TempfileManager.create_tempfile(dir=tmpdir, prefix='tmp_', suffix='_sim2Impact.yml')
        finp = pyutilib.services.TempfileManager.create_tempfile(dir=tmpDir, prefix='tmp_', suffix='.in')
        fout = pyutilib.services.TempfileManager.create_tempfile(dir=tmpDir, prefix='tmp_', suffix='.out')
        ffwd = pyutilib.services.TempfileManager.create_tempfile(dir=tmpDir, prefix='tmp_', suffix='_fwd.py')

        _solver_type = self.opts['solver']['type']

        # Default to DAKOTA as our solver interface
        if _solver_type == 'coliny_ea':
            _solver_type = 'dakota:coliny_ea'
        if _solver_type == 'StateMachineLS':
            _solver_type = 'dakota:StateMachineLS'

        if _solver_type.startswith('dakota:'):
            driver = 'dakota'
            _solver_type = _solver_type[7:]
        elif _solver_type.startswith('coliny:'):
            driver = 'coliny'
            _solver_type = _solver_type[7:]
        elif _solver_type == 'EVALUATE':
            driver = 'eval'
            _solver_repn = 'idx'
        else:
            raise RuntimeError("Unrecognized solver, '%s'" % _solver_type)

        init_pts = self.opts['solver']['initial points']

        if _solver_type == 'StateMachineLS':
            if driver == 'dakota':
                _solver_type = '|sco:StateMachineLS'
            else:
                _solver_type = 'sco:StateMachineLS'
            _solver_repn = 'bin'
            #_solver_options = ['verbosity', 'max_iterations', 'max_fcn_evaluations']
            try:
                enData = pyepanet.ENepanet()
                enData.ENopen(
                    self.opts['network']['epanet file'],
                    self.opts['configure']['output prefix']+'epanet_tmp.rpt' )
            except:
                msg = "Error: EPANET inp file not loaded using pyepanet"
                logger.error(msg)
                raise RuntimeError(msg) 
            nJunctions = enData.ENgetcount(pyepanet.EN_NODECOUNT) \
                         - enData.ENgetcount(pyepanet.EN_TANKCOUNT)
            enData.ENclose()
            #len(self.node_indices) != nJunctions \
            if len(self.link_indices) != len(self.all_link_ids) and len(self.link_indices) > 0:
                raise Exception("""
                The StateMachineLS can only operate over the entire network.
                Restricting the set of closeable links is currently not supported.""")

            self.createStateMachineInput()

        elif _solver_type in ('ea', 'coliny_ea'):
            if driver == 'dakota':
                _solver_type = 'coliny_ea'
            else:
                _solver_type = 'sco:ea'
            _solver_repn = 'idx'
            #_solver_options = [
            #    'max_iterations','max_function_evaluations','population_size',
            #    'initialization_type','fitness_type','crossover_rate',
            #    'crossover_type','mutation_rate','mutation_type','seed'
            #]
        
        if driver == 'dakota':
            #self.createDakotaInput(filename=finp, fwdRunFilename=ffwd)
            self.createDakotaInput(
                _solver_type, init_pts=init_pts, repn=_solver_repn,
                filename=finp, fwdRunFilename=ffwd)
            self.createDriverScripts(filename=ffwd, driver=driver, repn=_solver_repn)
            
            logger.info('Launching Dakota ...')
            dexe = self.opts['configure']['dakota executable']
            if dexe is None:
                msg = "Error: Cannot find the dakota executable on the system PATH"
                logger.error(msg)
                raise RuntimeError(msg) 
            cmd = ' '.join([dexe,'-input',finp,'-output',fout])
            logger.debug(cmd)
            sub_logger = logging.getLogger('wst.flushing.dakota')
            sub_logger.setLevel(logging.DEBUG)
            fh = logging.FileHandler(fout, mode='w')
            sub_logger.addHandler(fh)
            p = pyutilib.subprocess.run(
                cmd, 
                stdout=pywst.common.problem.LoggingFile(sub_logger),
                cwd=tmpDir,
            ) 
            sub_logger.removeHandler(fh)
            fh.close()
            self.parseDakotaOutput(fout, repn=_solver_repn)

        elif driver == 'coliny':
            self.createColinyInput(
                _solver_type, init_pts=init_pts, repn=_solver_repn,
                filename=finp, fwdRunFilename=ffwd )
            self.createDriverScripts(filename=ffwd, driver=driver, repn=_solver_repn)
            
            logger.info('Launching Coliny ...')
            cexe = self.opts['configure']['coliny executable']
            if cexe is None:
                msg = "Error: Cannot find the coliny executable on the system PATH"
                logger.error(msg)
                raise RuntimeError(msg)
            cmd = ' '.join([cexe,finp])
            logger.debug(cmd)
            sub_logger = logging.getLogger('wst.flushing.coliny')
            sub_logger.setLevel(logging.DEBUG)
            fh = logging.FileHandler(fout, mode='w')
            sub_logger.addHandler(fh)
            p = pyutilib.subprocess.run(
                cmd,
                stdout=pywst.common.problem.LoggingFile(sub_logger),
                cwd=tmpDir,
            )
            sub_logger.removeHandler(fh)
            fh.close()
            self.parseColinyOutput(fout,repn=_solver_repn)

        elif _solver_type == 'EVALUATE':
            #self.createDakotaInput(
            #    _solver_type, init_pts=None, repn=_solver_repn,
            #    filename=finp, fwdRunFilename=ffwd)
            self.createDriverScripts(filename=ffwd, driver='dakota', repn=_solver_repn)
            
            logger.info('Evaluate Placement ...')
            # create dummy params.in
            fid = open(os.path.join(tmpDir,'params.in'),'w')
            fid.write('%d variables\n'%len(self.node_names+self.link_names))
            for i in range(len(self.node_names)):
                fid.write('%s x%d\n'%(i+1, i+1))
            for i in range(len(self.link_names)):
                fid.write('%s x%d\n'%(i+1, i+1))
            #fid.write('1 functions\n')
            #fid.write('1 ASV_1\n')
            #fid.write('2 derivative_variables\n')
            #fid.write('1 DVV_1\n')
            #fid.write('2 DVV_2\n')
            #fid.write('0 analysis_components\n')
            fid.close()
            eval_out_file = pyutilib.services.TempfileManager.create_tempfile(
                dir=tmpDir, prefix='tmp_', suffix='_eval.out' )
            cmd = [sys.executable,ffwd,'params.in',eval_out_file]
            logger.debug(cmd)
            sub_logger = logging.getLogger('wst.flushing.exec')
            sub_logger.setLevel(logging.DEBUG)
            fh = logging.FileHandler(fout, mode='w')
            sub_logger.addHandler(fh)
            p = pyutilib.subprocess.run(
                cmd,
                stdout=pywst.common.problem.LoggingFile(sub_logger),
                cwd=tmpDir,
            )
            sub_logger.removeHandler(fh)
            fh.close()
            fid = open(eval_out_file,'r')
            self.results['finalMetric'] = float(fid.read())
            if self.opts['flushing']['flush nodes']['feasible nodes'] not in pywst.common.problem.none_list:
                self.results['nodesToFlush'] = self.opts['flushing']['flush nodes']['feasible nodes']
            else:
                self.results['nodesToFlush'] = []
            if self.opts['flushing']['close valves']['feasible pipes'] not in pywst.common.problem.none_list:
                self.results['pipesToClose'] = self.opts['flushing']['close valves']['feasible pipes']
            else:
                self.results['pipesToClose'] = []
            fid.close()
        else:
            raise Exception("ERROR: Unknown or unsupported solver, '%s'" % _solver_type)

        # remove temporary files if debug = 0
        if self.opts['configure']['debug'] == 0:
            pyutilib.services.TempfileManager.clear_tempfiles()
            for tmpFile in (
                    os.path.join('.','hydraulics.hyd'),
                    self.opts['configure']['output prefix']+'epanet_tmp.rpt' ):
                if os.path.exists(tmpFile):
                    os.remove(tmpFile)
                
        # write output file 
        prefix = os.path.basename(self.opts['configure']['output prefix'])      
        logfilename = logger.parent.handlers[0].baseFilename
        outfilename = logger.parent.handlers[0].baseFilename.replace('.log','.yml')
        visymlfilename = logger.parent.handlers[0].baseFilename.replace('.log','_vis.yml')
        
        # Write output yml file
        config = wst_config.output_config()
        module_blocks = ("general", "flushing")
        template_options = {
            'general':{
                'cpu time': round(time.time() - self.startTime,3),
                'directory': os.path.dirname(logfilename),
                'log file': os.path.basename(logfilename)},
            'flushing': {
                'nodes': self.results['nodesToFlush'],
                'pipes': self.results['pipesToClose'],
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
        if len(self.opts['flushing']['detection']) > 0:
            template_options['visualization']['layers'].append({
                    'label': 'Sensor locations', 
                    'locations': self.opts['flushing']['detection'],
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
        if len(self.results['nodesToFlush']) > 0:
            template_options['visualization']['layers'].append({
                    'label': 'Flushing locations', 
                    'locations': "['flushing']['nodes'][i]",
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
        if len(self.results['pipesToClose']) > 0:
            template_options['visualization']['layers'].append({
                    'label': 'Valve closure', 
                    'locations': "['flushing']['pipes'][i]",
                    'file': outfilename,
                    'location type': 'link',  
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
        p = pyutilib.subprocess.run(cmd, cwd=tmpDir) # logging information should not be printed to the screen
        
        # print solution to screen
        logger.info("\nWST normal termination")
        logger.info("---------------------------")
        logger.info("Directory: "+os.path.dirname(logfilename))
        logger.info("Results file: "+os.path.basename(outfilename))
        logger.info("Log file: "+os.path.basename(logfilename))
        logger.info("Visualization configuration file: "+os.path.basename(visymlfilename)+'\n')
        
        return
        
    def createDakotaInput( self,  solver, init_pts, repn,
                           filename, fwdRunFilename ):
        if filename is None: 
            filename = self.opts['configure']['output prefix']+'_dakota.in'
        fid = open(filename,'wt')
        fid.write("# DAKOTA INPUT FILE - based on dakota_rosenbrock_ea_opt.in\n")
        # Strategy was removed prior to DAKOTA 6
        #fid.write("strategy\n")
        #fid.write(" single_method\n")
            
        _highlevel_options = ('max_iterations','max_function_evaluations')
        availableOptions = self.opts['solver']['options']

        fid.write('method\n')
        for opt in _highlevel_options:
            if availableOptions and opt in availableOptions:
                fid.write(" %s = %s\n" % ( opt, availableOptions[opt] ))
        if solver[0] == '|':
            fid.write(" coliny_beta\n  beta_solver_name = '%s'\n" % solver[1:])
        else:
            fid.write(' %s\n' % solver)
        #if solver == 'coliny_ea':
        #    fid.write('  replacement_type\n   elitist = 1\n')
        for opt in availableOptions:
            if opt not in _highlevel_options:
                #pushNode("Option")
                #setAttr("name", opt)
                #addText(availableOptions[opt])
                fid.write("  %s = %s\n" % (opt, availableOptions[opt] ))

        fid.write("model\n")
        fid.write(" single\n")

        fid.write("variables\n")
        nNodes = self.opts['flushing']['flush nodes']['max nodes']
        nPipes = self.opts['flushing']['close valves']['max pipes']
        if repn == 'idx':
            nVars = nNodes + nPipes
            fid.write(' discrete_design_range = %d\n'%nVars)
            fid.write('  lower_bounds =')
            for i in range(nVars):
                fid.write(' 1')
            fid.write('\n')
            fid.write('  upper_bounds =')
            for i in range(nNodes):
                fid.write(' '+str(len(self.node_indices)))
            for i in range(nPipes):
                fid.write(' '+str(len(self.link_indices)))
            fid.write('\n')
            fid.write("  descriptors = %s %s\n" % (
                ''.join(" 'nid%s'" % i for i in range(nNodes)),
                ''.join(" 'pid%s'" % i for i in range(nPipes)) ))
            if init_pts:
                if len(init_pts) != 1:
                    raise RuntimeError("DAKOTA interface only accepts a single initial point")
                n_idx, p_idx = pointToIndexList(init_pts[0], nVars, self.node_names, self.link_names)
                # Arbitrarily pad up to nNodes with the first entry
                while nNodes > len(n_idx):
                    n_idx.append(1)
                if nNodes < len(n_idx):
                    raise RuntimeError("Initial point specified too many nodes to flush")
                while nPipes > len(p_idx):
                    p_idx.append(1)
                if nPipes < len(p_idx):
                    raise RuntimeError("Initial point specified too many pipes to close")
                fid.write('  initial_point = %s %s \n' % (
                    ' '.join(str(i) for i in n_idx),
                    ' '.join(str(i) for i in p_idx),
                ))
                
        elif repn == 'bin':
            # The extra 2 variables are to count "placed" nodes & links
            nodeUB = len(self.node_indices)
            linkUB = len(self.link_indices)
            nVars = nodeUB + linkUB + 2

            fid.write(' discrete_design_range = %d\n'%nVars)
            fid.write('  lower_bounds =')
            for i in range(nVars):
                fid.write(' 0')
            fid.write('\n')
            fid.write('  upper_bounds = ' + '1 '*(nVars-2))
            fid.write(' %s %s\n' % ( nNodes, nPipes ))
            fid.write("  descriptors = %s %s 'nNodes' 'nLinks'\n" % (
                ''.join(" 'n%s'" % i for i in range(nodeUB)),
                ''.join(" 'p%s'" % i for i in range(linkUB)) ))
            if init_pts:
                if len(init_pts) != 1:
                    raise RuntimeError("DAKOTA interface only accepts a single initial point")
                fid.write('  initial_point = %s \n' % (
                    ' '.join(str(i) for i in pointToBinaryList(
                        init_pts[0], nVars, self.node_names, self.link_names)) ))

        if fwdRunFilename is None:
            fwdRunFilename=self.opts['configure']['output prefix']+'_fwd.py'
        if os.name in ['nt','dos']:
            _cmd = fwdRunFilename.replace('.py','.bat')
        else:
            _cmd = sys.executable+' '+fwdRunFilename
        tmpDir = os.path.join(
            os.path.dirname(self.opts['configure']['output prefix']),
            'dakota_work' )

        if os.name in ['nt','dos']:
            _cmd = _cmd.replace('\\','/')
            tmpDir = tmpDir.replace('\\','/')

        fid.write("interface\n")
        fid.write(" analysis_drivers = '%s'\n" % _cmd)
        fid.write("  fork asynchronous evaluation_concurrency = %d\n" % self.opts['solver']['threads'],)
        fid.write("   parameters_file = 'tevaopt.in'\n")
        fid.write("   results_file = 'results.out'\n")
        fid.write("   work_directory named '%s' directory_tag\n" % tmpDir)

        fid.write("responses\n")
        fid.write(" num_objective_functions = 1\n")
        fid.write(" no_gradients\n")
        fid.write(" no_hessians\n")
        fid.close()


    def createColinyInput( self, solver, init_pts, repn,
                           filename, fwdRunFilename ):
        def pushNode(name):
            elt.append(doc.createElement(name))

        def popNode():
            node = elt.pop()
            elt[-1].appendChild(node)

        def setAttr(name, value):
            elt[-1].setAttribute(str(name), str(value))

        def addText(data):
            elt[-1].appendChild(doc.createTextNode(str(data)))
            
        nNodes = self.opts['flushing']['flush nodes']['max nodes']
        nPipes = self.opts['flushing']['close valves']['max pipes']

        nodeUB = len(self.node_indices)
        linkUB = len(self.link_indices)

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
            # The extra 2 variables are to count "placed" nodes & links
            nVars = nodeUB + linkUB + 2
            setAttr('num', nVars)
            pushNode("Lower")
            setAttr("value", 0)
            popNode()
            pushNode("Upper")
            setAttr("value", 1)
            popNode()
            pushNode("Upper")
            setAttr("index", nVars-1)
            setAttr("value", nNodes)
            popNode()
            pushNode("Upper")
            setAttr("index", nVars)
            setAttr("value", nPipes)
            popNode()
        elif repn == 'idx':
            nVars = nNodes + nPipes
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
            for i in range(nPipes):
                pushNode("Upper")
                setAttr("index", i+1+nNodes)
                setAttr("value", linkUB)
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
        for opt in availableOptions:
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
                    data = pointToBinaryList(pt, nVars, self.node_names, self.link_names)
                    addText( 'i(%d: %s)' % ( 
                        len(data),
                        ','.join(str(x) for x in data) ) )
                else:
                    n_idx, p_idx = pointToIndexList(init_pts[0], nVars, self.node_names, self.link_names)
                    # Arbitrarily pad up to nNodes with the first entry
                    while nNodes > len(n_idx):
                        n_idx.append(1)
                    if nNodes < len(n_idx):
                        raise RuntimeError("Initial point specified too many nodes to flush")
                    while nPipes > len(p_idx):
                        p_idx.append(1)
                    if nPipes < len(p_idx):
                        raise RuntimeError("Initial point specified too many pipes to close")
                    data = n_idx + p_idx
                    addText( 'i(%d: %s)' % ( 
                        len(data),
                        ','.join(str(x) for x in data) ) )
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
        tmpDir = os.path.dirname(self.opts['configure']['output prefix'])
        nNodes = self.opts['flushing']['flush nodes']['max nodes']
        nPipes = self.opts['flushing']['close valves']['max pipes']
        maxNIDs = len(self.node_indices)
        maxLIDs = len(self.link_indices)
        freeNode = maxNIDs + maxLIDs
        freeLink = maxNIDs + maxLIDs + 1
        fid = open(os.path.join(tmpDir, 'StateMachineLS.states'), 'wt')
        #fid.write("# Nodes: add/remove\n")
        # If not all the nodes are on, then we can turn them on
        for i in xrange(nNodes):
            for var in xrange(maxNIDs):
                # An "off" node can be turned on to any node
                fid.write("s%d,%d:%d,0,=|%d,%d;%d,1\n" %
                            (freeNode, i, var, freeNode, i+1, var))
            if self.opts['flushing']['allow location removal']:
                for var in xrange(maxNIDs):
                    # An "on" node can be turned off
                    fid.write("s%d,1:%d,%d,=|%d,%d;%d,0\n" %
                                (var, freeNode, i+1, freeNode, i, var))
        #fid.write("# Nodes: adjacent moves\n")
        # Build the lists of adjacent nodes to each flushable node
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
        
        #fid.write("# Links: add/remove\n")
        # If not all the pipes are closed, then we can close them
        for var in xrange(maxLIDs):
            for i in xrange(nPipes):
                # An "off" link can be turned on to any link
                fid.write("s%d,%d:%d,0,=|%d,%d;%d,1\n" %
                            (freeLink, i, maxNIDs+var, freeLink, i+1, maxNIDs+var))
                # An "on" link can be turned off
                if self.opts['flushing']['allow location removal']:
                    fid.write("s%d,1:%d,%d,=|%d,%d;%d,0\n" %
                                (maxNIDs+var, freeLink, i+1, freeLink, i, maxNIDs+var))
        # build the list of links to each node
        #fid.write("# Links: adjacent moves\n")
        if maxLIDs:
            nodeLinks = {}
            for l, n in self.all_link_endpoints.iteritems():
                nodeLinks.setdefault(n[0], set()).add(l)
                nodeLinks.setdefault(n[1], set()).add(l)
            # each link can move to a link connected to either endpoint
            for l, n  in self.all_link_endpoints.iteritems():
                for n_ in n:
                    for l1 in nodeLinks[n_]:
                        if l1 == l:
                            continue
                        fid.write("s%d,1:%d,0,=|%d,0;%d,1\n" %
                                    (maxNIDs+l-1, maxNIDs+l1-1, maxNIDs+l-1, maxNIDs+l1-1))
        fid.close()
        
    def createDriverScripts(self, filename, driver, repn):
        try:
            enData = pyepanet.ENepanet()
            enData.ENopen( 
                self.opts['network']['epanet file'],
                self.opts['configure']['output prefix']+'epanet_tmp.rpt' )
        except:
            msg = "Error: EPANET inp file not loaded using pyepanet"
            logger.error(msg)
            raise RuntimeError(msg) 
            
        # convert time units for dvf (integer hours)
        simulationDuration = int(enData.ENgettimeparam(pyepanet.EN_DURATION)/3600)
        
        # convert flow unit
        flowunit = enData.ENgetflowunits()
        enData.ENclose()

        rate = self.opts['flushing']['flush nodes']['rate'] # GPM
        if flowunit == 0: # EN_CFS = 0 cubic feet per second
            rate = rate/(60*7.48);
        elif flowunit == 1: # EN_GPM = 1 gallons per minute
            pass
        elif flowunit == 2: # EN_MGD = 2 million gallons per day
            rate = (rate*24*60)/1e6
        elif flowunit == 3: # EN_IMGD = 3 Imperial mgd
            rate = (rate*24*60*1.2)/1e6
        elif flowunit == 4: # EN_AFD = 4 acre-feet per day
            rate = (rate*24*60)/325851
        elif flowunit == 5: # EN_LPS = 5 liters per second
            rate = (rate*3.785)/60
        elif flowunit == 6: # EN_LPM = 6 liters per minute
            rate = rate*3.785
        elif flowunit == 7: # EN_MLD = 7 million liters per day
            rate = (rate*24*60*3.785)/1e6
        elif flowunit == 8: # EN_CMH = 8 cubic meters per hour
            rate = (rate*60)/264
        elif flowunit == 9: # EN_CMD = 9 cubic meters per day
            rate = (rate*24*60)/264

        if filename is None: 
            filename = self.opts['configure']['output prefix']+'_fwd.py'
        if os.path.exists(filename):
            if os.path.isfile(filename):
                os.unlink(filename)
            else:
                raise IOError("Cannot create the input file: it already exists and is not a normal file")
        fid = open(filename,'w')
        fid.write('#!'+sys.executable+'\n')
        fid.write(python_template_a)
        fid.write('verbose = %s\n'%self.opts['solver']['verbose'])
        fid.write('timeDetect = %s\n'%self.time_detect)
        fid.write('responseDelay = %d\n'%int(self.opts['flushing']['flush nodes']['response time']/60))
        fid.write('valveDelay = %d\n'%int(self.opts['flushing']['close valves']['response time']/60))
        fid.write('flushRate = %f\n'%self.opts['flushing']['flush nodes']['rate'])
        fid.write('flushLength = %d\n'%int(self.opts['flushing']['flush nodes']['duration']/60))
        fid.write('simDuration = %d\n'%simulationDuration)
        fid.write('maxNodes = %d\n'%self.opts['flushing']['flush nodes']['max nodes'])
        fid.write('maxPipes = %d\n'%self.opts['flushing']['close valves']['max pipes'])
        fid.write('nodeMap = %s\n'%self.node_indices)
        fid.write('pipeMap = %s\n'%self.link_indices)
        fid.write('prefix = r"%s"\n'%'tmp') #os.path.basename(self.opts['configure']['output prefix']))
        fid.write('prefixDir = r"%s"\n' % os.path.dirname(self.opts['configure']['output prefix']))
        if self.opts['scenario']['tsi file'] not in pywst.common.problem.none_list:
            fid.write('scenarioFile = r"%s"\n'%self.opts['scenario']['tsi file'])
        else:
            fid.write('scenarioFile = r"%s"\n'%self.opts['scenario']['tsg file'])
        fid.write('metric = r"%s"\n'%self.opts['impact']['metric'][0].lower())
        fid.write('tevasimexe = r"%s"\n'%self.opts['configure']['tevasim executable'])
        fid.write('healthImpactsFile = r"%s"\n'%os.path.abspath(self.opts['impact']['tai file']))
        fid.write('epanetInputFile = r"%s"\n'%os.path.abspath(self.opts['network']['epanet file']))
        fid.write('sim2Impactexe = r"%s"\n'%self.opts['configure']['sim2Impact executable'])
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
            #fwdRunFilename=self.opts['configure']['output prefix']+'_fwd.py'
            #batfilename = fwdRunFilename.replace('.py','.bat')
            batfilename = filename.replace('.py','.bat')
            fid = open(batfilename,'wt')
            fid.write('@echo off\n')
            fid.write('"%s" "%s" %%*%%'%(sys.executable,filename))
            fid.close()
        else:
            os.chmod(filename,0755)
            #os.chmod(cmdfilename,0755)
        return

    def parseDakotaOutput(self, file, repn):
        #file = self.opts['configure']['output prefix']+'_dakota.out'
        fid = open(file,'r')
        lines = fid.readlines()
        fid.close()
        for lidx in range(len(lines)):
            tokens = lines[lidx].split()
            if len(tokens) < 1: continue
            if tokens[0] != r'<<<<<': continue
            if tokens[2].strip() == 'parameters':
                maxNodes = self.opts['flushing']['flush nodes']['max nodes']
                maxLinks = self.opts['flushing']['close valves']['max pipes']
                nodeMap = self.node_names
                linkMap = self.link_names 

                if repn == 'bin':
                    nNodes = len(self.node_indices)
                    self.results['nodesToFlush'] = sorted(
                        [self.node_names[i] for i in range(len(self.node_indices)) if int(lines[lidx+1+i].split()[0]) ])
                    self.results['pipesToClose'] = sorted(
                        [self.link_names[i] for i in range(len(self.link_indices)) if int(lines[lidx+1+i+nNodes].split()[0]) ]) 
                elif repn == 'idx':
                    for i in range(maxNodes):
                        line = lines[lidx+1+i]
                        (v,n) = line.split()
                        nodeIdx = nodeMap[int(v)-1]
                        self.results['nodesToFlush'].append(nodeIdx)
                    nodeSet = list(set(self.results['nodesToFlush']))
                    self.results['nodesToFlush'] = nodeSet
                    for i in range(maxLinks):
                        line = lines[lidx+1+maxNodes+i]
                        (v,n) = line.split()
                        pipeIdx = linkMap[int(v)-1]
                        self.results['pipesToClose'].append(pipeIdx)
                    linkSet = list(set(self.results['pipesToClose']))
                    self.results['pipesToClose'] = linkSet
                else:
                    raise Exception("Unknown problem representation")
            if tokens[2].strip() == 'objective':
                self.results['finalMetric'] = float(lines[lidx+1])
        return


    def parseColinyOutput(self, file, repn):
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
                    self.results['nodesToFlush'] = sorted(
                        [self.node_names[i] for i in range(len(self.node_indices)) if vars[i]])
                    self.results['pipesToClose'] = sorted(
                        [self.link_names[i] for i in range(len(self.link_indices)) if vars[i+nNodes]])
                elif repn == 'idx':
                    maxNodes = self.opts['flushing']['flush nodes']['max nodes']
                    maxLinks = self.opts['flushing']['close valves']['max pipes']
                    if len(vars) != maxNodes + maxLinks:
                        raise Exception("Bad number of results returned from Coliny (%s != %s + %s)"
                                        % (len(vars), maxNodes, maxLinks))
                    tmp = set(vars[:maxNodes])
                    if 0 in tmp:
                        tmp.remove(0)
                    self.results['nodesToFlush'] = sorted([self.node_names[i-1] for i in tmp])
                    tmp = set(vars[maxNodes:])
                    if 0 in tmp:
                        tmp.remove(0)
                    self.results['pipesToClose'] = sorted([self.link_names[i-1] for i in tmp])
                else:
                    raise Exception("Unknown problem representation")
            if line_.strip().startswith('objective function:'):
                self.results['finalMetric'] = float(line.split(":",1)[1].strip())
            if line_.strip().startswith('Annotations:'):
                foundCache = False
                
        return


    # General Option SET functions
    def setConfigureOption(self, name, value):
        if name == 'output prefix' and value != '':
            output_prefix = os.path.splitext(os.path.split(value)[1])[0]
            value = output_prefix
        self.opts['configure'][name] = value
        return
        
    def setSolverOption(self, name, value):
        self.opts['solver'][name] = value
        return   
        
    def setFlushingOption(self, name, value):
        self.opts['flushing'][name] = value
        return
        
    def setFlushingNodeOption(self, name, value):
        self.opts['flushing']['flush nodes'][name] = value
        return
    
    def setFlushingPipeOption(self, name, value):
        self.opts['flushing']['close valves'][name] = value
        return   
    
    def setScenarioOption(self, name, value):
        self.opts['scenario'][name] = value
        return

    def setNetworkOption(self, name, value):
        self.opts['network'][name] = value
        return
        
    def setImpactOption(self, name, value):
        self.opts['impact'][name] = value
        return
        
    # General Option GET functions
    def getSolverOption(self, name):
        return self.opts['solver'][name]
        
    def getConfigureOption(self, name):
        return self.opts['configure'][name]
        
    def getFlushingNodeOption(self, name):
        return self.opts['flushing']['flush nodes'][name]
        
    def getFlushingPipeOption(self, name):
        return self.opts['flushing']['close valves'][name]
        
    def getScenarioOption(self, name):
        return self.opts['scenario'][name]
    
    def getNetworkOption(self, name):
        return self.opts['network'][name]

    def getImpactOption(self, name):
        return self.opts['impact'][name]

python_template_a = '''import os, sys, xml.dom.minidom

def log(msg):
    sys.stdout.write(msg)
    sys.stdout.flush()

class Problem(object):
    function_values = []
    var_names = []
    var_values = []
    
    def __init__(self):
        pass
    
    def function_value(self):
        scenarioFmt = scenarioFile.split('.')[-1]
        fid = open(scenarioFile,'r')
        tsg = fid.readlines()
        fid.close()

        sum = 0
        count = 0

        nvars = len(self.var_values)
        nodes = set(self.var_values[:maxNodes])
        if 0 in nodes:
            nodes.remove(0)
        nodes = sorted(nodes)
        pipes = set(self.var_values[maxNodes:])
        if 0 in pipes:
            pipes.remove(0)
        pipes = sorted(pipes)
        if verbose:
            log('nodes: %s, pipes: %s ... \\n' %
                ( str([nodeMap[int(n)-1] for n in nodes]),
                  str([pipeMap[int(p)-1] for p in pipes]), ))

        scenarioSet = {}
        for i in range(len(tsg)-1):
            scenarioSet.setdefault(timeDetect[i], set()).add(tsg[i+1])
            
        for _timeDetect, _scenarios in scenarioSet.iteritems():
            if verbose:
                log('Running tevasim for detection time %s (%s scenarios):\\n\\t%s\\n' % 
                    ( _timeDetect, len(_scenarios), "\\t".join(_scenarios) ))
                    
            cwd = os.getcwd()
            #if prefixDir:
            #    os.chdir(prefixDir)

            # create tsg file for this subset of scenarios
            singleScenarioFile = 'tmp_%s.tsg'%(prefix,)
            fid_tsg = open(singleScenarioFile,'w')
            fid_tsg.write(''.join(_scenarios))
            fid_tsg.close()

            # create dvf file
            dvfFile = 'tmp_%s.dvf'%(prefix,)
            fid = open(dvfFile,'w')
            print >>fid, '%d'%True
            print >>fid, '%d %d %d %f %d %d'%(_timeDetect/60, responseDelay, valveDelay,
                                                flushRate, flushLength, simDuration)
            print >>fid, '%d '%len(nodes),
            for node in nodes:
                print >>fid, '%d '%nodeMap[(int(node))-1],
            print >>fid, ''
            print >>fid, '%d '%len(pipes),
            for pipe in pipes:
                print >>fid, '%d '%pipeMap[(int(pipe))-1],
            print >>fid, ''
            fid.close()
            
            # run tevasim
            if verbose:
                log('s')
            scmd = 'tevasim --dvf=%s --tsg=%s %s %s.rpt %s'%(dvfFile, singleScenarioFile,epanetInputFile,epanetInputFile,prefix) 
            log(scmd+'\\n')
            os.system(scmd+'>'+prefix+'_tevasim.out') 
            #os.system('wst tevasim '+prefix+'_tevasim.yml >'+prefix+'_tevasim.out')
            
            # run sim2Impact
            if verbose:
                log('i')
            icmd = 'tso2Impact --%s '%(metric) 
            if metric in ['ec','tec']: 
                icmd = icmd 
            else: 
                icmd = icmd + ' --dvf=%s '%dvfFile 
            icmd = icmd + ' %s %s.erd '%(prefix,prefix) 
            if metric in ['pd','pe','pk']: 
                icmd = icmd + ' %s'%(healthImpactsFile) 
            log(icmd+'\\n')
            os.system(icmd+'>'+prefix+'_sim2Impact.out') 
            #os.system('wst sim2Impact '+prefix+'_sim2Impact.yml >'+prefix+'_sim2Impact.out')
            
            # extract objective
            try:
                fid = open('%s_%s.impact'%(prefix,metric),'r')
                hdr = fid.readline()
                hdr = fid.readline()
                lines = fid.readlines()
                fid.close()
            except:
                sum += float('inf')
                count += len(_scenarios)
                lines = []
            for line in lines:
                vars = line.split()
                if len(vars) < 4: continue
                if vars[1] == '-1':
                    count += 1
                    sum += float(vars[3])
        
        #if prefixDir:
        #    os.chdir(cwd)
                
        meanObj = float(sum)/float(count)
        if verbose:
            log(" ... %g\\n" % (meanObj,))
        self.function_values.append(meanObj)
        
        return

class dakota_Problem(Problem):
    def __init__(self, repn):
        self.repn = repn
    
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

        if self.repn == 'bin':
            nnodes = len(nodeMap)
            npipes = len(pipeMap)
            nvals = [i+1 for i in range(nnodes) if self.var_values[i]]
            pvals = [i+1 for i in range(npipes) if self.var_values[i+nnodes]]
            while len(nvals) < maxNodes:
                nvals.append(0)
            self.var_values = nvals + pvals

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
            npipes = len(pipeMap)
            nvals = [i+1 for i in range(nnodes) if self.var_values[i]]
            pvals = [i+1 for i in range(npipes) if self.var_values[i+nnodes]]
            while len(nvals) < maxNodes:
                nvals.append(0)
            self.var_values = nvals + pvals
        return
        
'''
# verbose
# timeDetect
# responseDelay
# valveDelay
# flushRate
# flushLength
# simDuration
# maxNodes
# maxPipes
# nodeMap
# pipeMap
# prefix
# prefixDir
# scenarioFmt
# scenarioFile
# metric
# healthImpactsFile
# epanetInputFile

python_template_b = '''

if __name__ == '__main__':
    problem = driver
    problem.parser(sys.argv[1])
    problem.function_value()
    problem.reporter(sys.argv[2])

'''

