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

import itertools
import pprint
import imp

import pywst.common.problem
import pywst.common.wst_util as wst_util
import pywst.common.wst_config as wst_config
from pyutilib.misc.config import ConfigBlock

import pywst.visualization.inp2svg as inp2svg

from pyomo.environ import *

logger = logging.getLogger('wst.inversion')

try:
    import pyepanet
except ImportError:
    pyepanet = {}
    #raise RuntimeError("EPANET DLL is missing or corrupt. Please reinstall PyEPANET.")


class Problem(pywst.common.problem.Problem):

    results = { 'dateOfLastRun': '',
                'nodesToSource': [],
                'finalMetric': -999 }

    # Trying handle all possible ways we may encounter None coming from the yaml parser

    none_list = ['none','','None','NONE', None]
    defLocs = {}
    epanetOkay = False

    def __init__(self):
        pywst.common.problem.Problem.__init__(self, 'inversion', ("network", "measurements", "inversion", "solver", "configure")) 
        self.filename = 'inversion.yml'
        self.loadPreferencesFile()
        self.validateEPANET()
        return

    def validateEPANET(self):
        """
        try:
            enData = pyepanet.ENepanet()
        except:
            raise RuntimeError("EPANET DLL is missing or corrupt. Please reinstall PyEPANET.")
        self.epanetOkay = True
        """
        return

    def trunc(self,f, n):
      '''Truncates/pads a float f to n decimal places without rounding'''
      slen = len('%.*f' % (n, f))
      return str(f)[:slen]

    def loadPreferencesFile(self):
        if os.name in ['nt','win','win32','win64','dos']:
            rcPath = os.path.join(os.path.abspath(os.environ['APPDATA']),
                                  '.wstrc')
        else:
            rcPath = os.path.join(os.path.abspath(os.environ['HOME']),
                                  '.wstrc')
        if os.path.exists(rcPath) and os.path.isfile(rcPath):
            fid = open(rcPath,'r')
            defLocs = yaml.load(fid)
            fid.close()
            self.defLocs = defLocs
            for key in defLocs.keys():
                if key == 'ampl':
                    self.opts['configure']['ampl executable'] = defLocs[key]
                if key == 'pyomo':
                    self.opts['configure']['pyomo executable'] = defLocs[key]
        return
        
    def runInversionsim(self):
        logger = logging.getLogger('wst.inversion.inversionsim')
        
        # Prepend all output file names with this
        prefix = self.getConfigureOption('output prefix')
        if prefix is None:
            prefix = ''

        #self.createInversionSimDat()
        cmd = ['inversionsim']

        is_inp_file = (self.getNetworkOption('epanet file') not in self.none_list)
        is_wqm_file = (self.getInversionOption('wqm file') not in self.none_list)

        # Optional arguments are simulation duration and water quality timestep, which will
        # override what is in the EPANET input file.
        if is_inp_file and self.getNetworkOption('simulation duration') not in ['INP','Inp','inp']:
            cmd.append('--simulation-duration-minutes='+str(self.getNetworkOption('simulation duration')))
        if is_inp_file and self.getNetworkOption('water quality timestep') not in ['INP','Inp','inp']:
            cmd.append('--quality-timestep-minutes='+str(self.getNetworkOption('water quality timestep')))

        # Substitude integers for node names in output files
        cmd.append('--output-merlion-labels')
        # the above command will produce the following file
        label_map_file = self._get_prefixed_filename('MERLION_LABEL_MAP.txt',tempfile=True)


        # Prepend all output file names with this
        cmd.append('--output-prefix='+prefix)

        #if self.getInversionOption('model format') not in ['AMPL','PYOMO']:
        #    raise IOError("Invalid model format: "+self.getInversionOption('model format'))

        #check for the horizon
        if self.getInversionOption('horizon') not in self.none_list:
            cmd.append('--horizon-minutes=' + str(self.getInversionOption('horizon')))

        # Allowed node filename
        if self.getInversionOption('feasible nodes') not in self.none_list:
            try:
                enData = pyepanet.ENepanet()
                enData.ENopen(self.opts['network']['epanet file'],'tmp.rpt')
            except:
                msg = 'EPANET inp file not loaded using pyepanet'
                logger.error(msg)
                raise RuntimeError(msg)

            feasible_node_names, feasible_node_indices = wst_util.feasible_nodes(\
                                                                 self.getInversionOption('feasible nodes'),\
                                                                 [], \
                                                                 True, enData)
            enData.ENclose()
            tmpNodesFile = self._get_tempfile('nodes.txt')
            # write nodes file
            fid = open(tmpNodesFile,'w')
            for n in feasible_node_names:
                fid.write(n + '\n')
            fid.close()

            cmd.append('--allowed-impacts=' + tmpNodesFile)
            self.opts['inversion']['feasible nodes'] = tmpNodesFile

        #check for water quality model tolerance
        #if self.getInversionOption('wqm_zero_tol') not in self.none_list:
            #cmd.append('--wqm-zero=' + str(self.getInversionOption('wqm_zero_tol')))

        #check for algorithm type
        if self.getInversionOption('algorithm')=='optimization':
            cmd.append('--optimization')
        elif self.getInversionOption('algorithm')=='bayesian':
            # Check is merlion is selected 
            if self.getInversionOption('merlion water quality model'):
                cmd.append('--merlion')
            cmd.append('--probability')
            if self.getInversionOption('negative threshold') not in self.none_list:
                cmd.append('--meas-threshold='+ str(self.getInversionOption('negative threshold')))
            else:
              cmd.append('--meas-threshold=0.0')
        else: cmd.append('--optimization')

        #probability options
        #if self.getInversionOption('meas_threshold') not in self.none_list:
            #cmd.append('--meas-threshold=' + str(self.getInversionOption('meas_threshold')))

        if self.getInversionOption('measurement failure') not in self.none_list and self.getInversionOption('measurement failure')!=0.05:
           cmd.append('--meas-failure='+ str(self.getInversionOption('measurement failure')))

        if self.getInversionOption('confidence') not in self.none_list and self.getInversionOption('confidence')!=0.95:
           cmd.append('--prob-confidence='+ str(self.getInversionOption('confidence')))

        if self.getInversionOption('output impact nodes'):
            cmd.append('--output-impact-nodes')

        #check for the horizon
        #if self.getInversionOption('start inversion') not in self.none_list:
            #cmd.append('--start-inversion=' + str(self.getInversionOption('start inversion')))

        # Ignore Merlion Warnings
        if self.getInversionOption('ignore merlion warnings'):
            cmd.append('--ignore-merlion-warnings')

        cmd.append('--epanet-rpt-file='+prefix+'epanet')
        cmd.append('--merlion-save-file='+prefix+'merlion')


        #LOGIC VALIDATION
        assert(is_inp_file != is_wqm_file) # one and only one is true

        # The file defining the water quality model
        if is_inp_file:
            cmd.append('--inp='+ self.getNetworkOption('epanet file'))
        elif is_wqm_file:
            cmd.append('--wqm='+ self.getInversionOption('wqm file'))

        # Check for the measurement file
        assert(self.getMeasurementOption('grab samples') not in self.none_list)
        cmd.append(self.getMeasurementOption('grab samples'))
        
        logger.info("Launching inversionsim executable ...")
        logger.debug(cmd)
        out = self._get_prefixed_filename('inversionsim.out')
        sim_timelimit = None
        sub_logger = logging.getLogger('wst.inversion.inversionsim.exec')
        sub_logger.setLevel(logging.DEBUG)
        fh = logging.FileHandler(out, mode='w')
        sub_logger.addHandler(fh)
        p = pyutilib.subprocess.run(cmd,timelimit=sim_timelimit,stdout=pywst.common.problem.LoggingFile(sub_logger))
        if p[0]:
            msg = 'An error occured when running the inversionsim executable (return code %s)\nError Message: %s\nCommand: %s' % (p[0], p[1], cmd)
            logger.error(msg)
            raise RuntimeError(msg)

        nodemap = {}
        f = open(label_map_file,'r')
        for line in f:
            t = line.split()
            nodemap[t[1]] = t[0]
        f.close()

        return nodemap

    def runCSArun(self,num_sensors, meas_step_sec, sim_stop_time):
        logger = logging.getLogger('wst.inversion.csarun')

        # Prepend all output file names with this
        prefix = self.getConfigureOption('output prefix')
        if prefix is None:
            prefix = ''

        cmd = ['csarun', '--output-prefix=' + prefix]

        is_inp_file = (self.getNetworkOption('epanet file') not in self.none_list)
        is_wqm_file = (self.getInversionOption('wqm file') not in self.none_list)
        
        if is_inp_file:
            cmd.append( '--inp='+self.getNetworkOption('epanet file') )
        else:
            msg = 'No INP file specified.'
            logger.error(msg)
            raise RuntimeError(msg)
            
        # Prepend all output file names with this
        cmd.append( '--num-sensors='+str(num_sensors) )

        cmd.append( '--meas-step-sec='+str(meas_step_sec) )

        cmd.append( '--qual-step-sec='+str(meas_step_sec) )

        cmd.append( '--sim-duration='+str(sim_stop_time) )

        meas_file_name = prefix + "csa_measurements"
        cmd.append( '--meas='+meas_file_name )

        sensor_file_name = prefix + "csa_sensors"
        cmd.append( '--sensors='+sensor_file_name )

        #check for the horizon
        if self.getInversionOption('horizon') not in self.none_list:
            cmd.append( '--horizon=' + str(self.getInversionOption('horizon')/60.0) ) # Converting to hours 

        # Allowed node filename
        if self.getInversionOption('feasible nodes') not in self.none_list:
            msg = 'The CSA algorithm does not yet support feasible nodes option'
            logger.error(msg)
            raise RuntimeError(msg)

        # Measurement threshold
        
        if self.getInversionOption('negative threshold') not in self.none_list:
            if self.getInversionOption('negative threshold') > 0.0:
                logger.info('\nWARNING: The current CSA implementation only supports a negetive threshold of 0. Setting to 0. \n')
            cmd.append( '--meas-threshold='+ str(0.0) )

        logger.info("Launching csarun executable ...")
        logger.debug(cmd)
        out = self._get_prefixed_filename('csarun.out') 
        sim_timelimit = None
        sub_logger = logging.getLogger('wst.inversion.csarun.exec')
        sub_logger.setLevel(logging.DEBUG)
        fh = logging.FileHandler(out, mode='w')
        sub_logger.addHandler(fh)
        p = pyutilib.subprocess.run(cmd,timelimit=sim_timelimit,stdout=pywst.common.problem.LoggingFile(sub_logger))
        if (p[0]):
            msg = 'An error occured when running the csarun executable\nError Message: %s\nCommand: %s' % (p[1], cmd)
            logger.error(msg)
            raise RuntimeError(msg)
        

    def run(self, cmd_line_options=None):
        
        logger.info("WST inversion subcommand")
        logger.info("---------------------------")
        
        # set start time
        self.startTime = time.time()
        
        # validate input
        logger.info("Validating configuration file")
        self.validate()

        # Override commandline options 
        if (cmd_line_options is not None) and \
           (cmd_line_options.inp_file is not None):
            self.setNetworkOption('epanet file', cmd_line_options.inp_file)
        if (cmd_line_options is not None) and \
           (cmd_line_options.measurements is not None):
            self.setMeasurementOption('grab samples', cmd_line_options.measurements)

        # Run C/C++ executables : inversionsim or runcsa
        if self.getInversionOption('algorithm') == 'csa':
            [num_sensors, meas_step_sec, sim_stop_time] = self.writeCSAInputFiles()
            self.runCSArun(num_sensors, meas_step_sec, sim_stop_time)            
            Solution = self.readCSAOutputFiles(sim_stop_time, meas_step_sec)
            logger.debug(Solution)
        else:
            nodemap = self.runInversionsim()

        # Setup result vectors 
        inversion_nodes = []
        objective_val = []
        
        if self.getInversionOption('algorithm') == 'optimization':

          # Check is merlion is selected 
            if not self.getInversionOption('merlion water quality model'):
                msg = 'ERROR: The optimization based method requires using the Merlion water quality model. Please set to true.' 
                logger.error(msg)
                raise RuntimeError(msg)

            solve_timelimit = None
            p = (1,"There was a problem with the 'formulation' or 'model format' options")
            cmd = None
            Solution = []

            allowed_nodes_set=set()
            if self.getInversionOption('feasible nodes') not in self.none_list \
               and len(open(self.getInversionOption('feasible nodes'),'r').readlines())!=0:
                label_map_file = self._get_prefixed_filename("MERLION_LABEL_MAP.txt")
                name_to_id={}
                f = open(label_map_file,'r')
                for line in f:
                    t = line.split()
                    name_to_id[t[0]] = t[1]
                f.close()
                for line in open(self.getInversionOption('feasible nodes'),'r'):
                    l=line.split()
                    for n_ in l:
                        if name_to_id.has_key(n_)!=True:
                            msg = 'ERROR: Nodename %s specified in %s is not part of the network' % (n_, self.getInversionOption('feasible nodes'))
                            logger.error(msg)
                            raise RuntimeError(msg)
                        allowed_nodes_set.add(int(name_to_id[n_]))

            #run pyomo or ampl
            self.setInversionOption('model format',self.getInversionOption('model format').upper())
            self.setInversionOption('formulation',self.getInversionOption('formulation').upper())
            #print self.getInversionOption('formulation')

            if self.getInversionOption('model format') == 'AMPL':
                exe = self.getConfigureOption('ampl executable')
                inp = self._get_prefixed_filename('ampl.run')
                out = self._get_prefixed_filename('ampl.out')
                results_file = ''
                if self.getInversionOption('formulation') in self.none_list or self.getInversionOption('formulation')=='LP_DISCRETE':
                    if self.getInversionOption('num injections') not in self.none_list \
                    and self.getInversionOption('num injections')!=1:
                        logger.info('\nWARNING: This model cannot handle more than one contamination injection.\tIt is recommended to use the MIP model.\n')
                    logger.info('Solving the reduce LP model ...')
                    results_file += self.createAMPLRunReduceLP(inp,allowed_nodes_set)

                elif self.getInversionOption('formulation') == 'MIP_DISCRETE_ND':
                    logger.info('Solving the reduce discrete MIP model with no decrease ...')
                    results_file += self.createAMPLRunReduceMIPnd(inp,allowed_nodes_set)

                elif self.getInversionOption('formulation') == 'MIP_DISCRETE':
                    logger.info('Solving the reduce discrete MIP model ...')
                    results_file += self.createAMPLRunReduceMIP(inp,allowed_nodes_set)

                elif self.getInversionOption('formulation') == 'MIP_DISCRETE_STEP':
                    if self.getInversionOption('num injections') not in self.none_list\
                    and self.getInversionOption('num injections') != 1:
                        logger.info('\nWARNING: This model cannot handle more than one contamination injection.\tIt is recommended to use the MIP model.\n')
                    logger.info('Solving the reduce STEP model ...')
                    results_file += self.createAMPLRunStep(inp,allowed_nodes_set)

                else:
                    msg = """Bad specification of the formulation option.\n
                        \tThe posibilities are:\n
                        \t1. LP_discrete\n
                        \t2. MIP_discrete\n
                        \t3. MIP_discrete_step\n
                        \t4. MIP_discrete_nd"""
                    logger.error(msg)
                    raise RuntimeError(msg)
                cmd = [exe,inp]
                logger.info('Launching AMPL ...')

                p = pyutilib.subprocess.run(cmd,timelimit=solve_timelimit,outfile=out)
                if (p[0]):
                    msg = 'An error occured when running the optimization problem\nError Message: %s\nCommand: %s' % (p[1], cmd)
                    logger.error(msg)
                    raise RuntimeError(msg) 

                #try to load the results file
                logger.info('AMPL result file from inversion: ' + results_file)
                Solution=self.AMPLresultsReader(results_file)
                #stop_timing=time()
                #print 'AMPL Timing',stop_timing-start_timing
                
            elif self.getInversionOption('model format') == 'PYOMO':
                if self.getInversionOption('formulation') in self.none_list or self.getInversionOption('formulation')=='LP_DISCRETE':
                    if self.getInversionOption('num injections') not in self.none_list \
                    and self.getInversionOption('num injections')!=1:
                        msg = """\nWARNING: This model cannot handle more than one contamination injection.
                            \tIt is recommended to use the MIP model.\n"""
                        logger.info(msg)
                    logger.info('Solving the reduce LP model ...')
                    Solution=self.runPYOMOReduceLP(allowed_nodes_set)

                elif self.getInversionOption('formulation') == 'MIP_DISCRETE_ND':
                    logger.info('Solving the MIP model ...')
                    Solution=self.runPYOMOReduceMIPnd(allowed_nodes_set)

                elif self.getInversionOption('formulation') == 'MIP_DISCRETE':
                    logger.info('Solving the MIP model ...')
                    Solution=self.runPYOMOReduceMIP(allowed_nodes_set)

                elif self.getInversionOption('formulation') == 'MIP_DISCRETE_STEP':
                    if self.getInversionOption('num injections') not in self.none_list\
                    and self.getInversionOption('num injections') != 1:
                        msg = """\nWARNING: This model cannot handle more than one contamination injection.
                            \tIt is recommended to use the MIP model.\n"""
                        logger.info(msg)
                    logger.info('Solving the reduce STEP model ...')
                    Solution=self.runPYOMOStep(allowed_nodes_set)

                else:
                    msg = """Bad specification of the formulation option.\n
                        \tThe posibilities are:\n
                        \t1. LP_discrete\n
                        \t2. MIP_discrete\n
                        \t3. MIP_discrete_step\n
                        \t4. MIP_discrete_nd"""
                    logger.error(msg)
                    raise RuntimeError(msg) 
                        
            else:
                msg = """\nBad specification of the model format option.\n
                    \tThe posibilities are:\n
                    \t1. AMPL\n
                    \t2. PYOMO"""
                logger.info(msg)
                
            # Normalize Objective values
            obj_list = []
            Solution.sort()
            try:
                bigger=Solution[-1][0]
            except IndexError:
                msg = 'ERROR: The optimization solution does not contain any nodes'
                logger.error(msg)
                raise RuntimeError(msg)
            
            bigger=Solution[-1][0]

            if(bigger==0):
                bigger=1
            for i in xrange(0,len(Solution)):
                if len(Solution)>1:
                    Obj=float(1-Solution[i][0]/bigger)
                    #Obj=Solution[i][0]
                else:
                    Obj=1
                obj_list.append(Obj)
                #Solution[i][0]=Obj
                nodes_=[]
                for j in xrange(0,len(Solution[i][1])):
                    nodename=nodemap[Solution[i][1][j][0]]
                    Solution[i][1][j][0]=nodename
                    nodes_.append(nodename)
                #print nodes_,'\t',Obj
            top_obj = obj_list[0]
            assert top_obj != 0.0, "ERROR: Highest objective value is 0!"
            
            for i in xrange(0,len(Solution)):
                Solution[i][0]=obj_list[i]/top_obj
            
            if self.getInversionOption('candidate threshold') not in self.none_list:
                tao = float(self.getInversionOption('candidate threshold'))
            else:
                tao = 0.95
            #print Solution
            json_file = self._get_prefixed_filename('inversion.json')
            json_file_wDir = os.path.join(os.path.abspath(os.curdir),json_file)
            [num_events, objective_val, inversion_nodes] = self.printResults(Solution, tao, json_file_wDir)
            [tsg_file, likely_nodes_file] = self.writeProfiles(Solution, tao) # tsg or scn file
            logger.debug(tsg_file)
            #self.writeAllowedNodesFile('allowed.dat',Solution,tao)
            
        elif self.getInversionOption('algorithm') == 'bayesian':
            json_file = self._get_prefixed_filename('inversion.json')
            json_file_wDir = os.path.join(os.path.abspath(os.curdir),json_file)
            data_prob_results = open(json_file_wDir).read()
            Solution = json.loads(data_prob_results)
            for sol in Solution:
              objective_val.append(sol['Objective'])
              inversion_nodes.append(sol['Nodes'][0]['Name'])
            num_events = self.printResults(Solution, 0.0, json_file)
            [tsg_file, likely_nodes_file] = self.writeProfiles(Solution, 0.0) # tsg or scn file
            #self.writeAllowedNodesFile('allowed.dat',Solution,tao)
            tao = 1 # for visualization
            
        else:
            [json_file_wDir, tsg_file, num_events, likely_nodes_file, objective_val, inversion_nodes] = self.writeCSAresults(Solution)
            tao = 1 # for visualization
    
        # remove temporary files if debug = 0
        if self.opts['configure']['debug'] == 0:
            pyutilib.services.TempfileManager.clear_tempfiles()
        
        # write output file 
        logfilename = logger.parent.handlers[0].baseFilename
        outfilename = logger.parent.handlers[0].baseFilename.replace('.log','.yml')
        visfilename = logger.parent.handlers[0].baseFilename.replace('.log','.html')
        visymlfilename = logger.parent.handlers[0].baseFilename.replace('.log','_vis.yml')
        
        # Write visualization YML file
        self.writeVisualizationFile(Solution, tao, outfilename, visfilename, visymlfilename)
        

        #build output vectors
        config = wst_config.output_config()
        module_blocks = ("general", "inversion")
        template_options = {
            'general':{
                'cpu time': round(time.time() - self.startTime,3),
                'directory': os.path.dirname(logfilename),
                'log file': os.path.basename(logfilename)},
            'inversion': {
                'tsg file': tsg_file,
                'likely nodes file': likely_nodes_file,
                'candidate nodes': inversion_nodes,
                'node likeliness': objective_val}}
                
        if outfilename != None:
            self.saveOutput(outfilename, config, module_blocks, template_options)

        # Run visualization
        cmd = ['wst', 'visualization', visymlfilename]
        p = pyutilib.subprocess.run(cmd) # logging information should not be printed to the screen

        # print solution to screen
        logger.info("\nWST normal termination")
        logger.info("---------------------------")
        logger.info("Directory: "+os.path.dirname(logfilename))
        logger.info("Results file: "+os.path.basename(outfilename))
        logger.info("Log file: "+os.path.basename(logfilename))
        #logger.info("Visualization file: "+os.path.basename(visfilename)+', '+os.path.basename(visymlfilename)+'\n')
        logger.info("Visualization Configuration file: " +os.path.basename(visymlfilename)+'\n')
        
        return [Solution, json_file_wDir, tsg_file, num_events]

    def AMPLresultsReader(self,filename=None,lower_strenght_value=1e-3):

        if filename is None:
            filename = self._get_prefixed_filename('inversion_results.dat')

        reader=open(filename,'r')
        #In case we want to change from time to timesteps later...
        timestep=float(reader.readline().split()[1])
        unit_change=timestep*60
        #profile{start,stop,strength}
        #object{objective_val,list_of_Nodes{node_name,profile}}
        results=[]
        while True:
            try:
                columns=reader.next().split()
                if columns[0]=='Solution':
                    node_names=[]
                    profiles=[]
                    objective=float(columns[-1])
                    if len(columns)>3:
                        for i in xrange(1,len(columns)-1):
                            node_names.append(columns[i])
                    else:
                        node_names.append(columns[1])
                    for j in xrange(0,len(node_names)):
                        str_profiles=reader.next().split()
                        profile=[];injection=[]
                        for k in xrange(0,len(str_profiles)):
                            injection.append(float(str_profiles[k]))
                            if len(injection)==3:
                                injection[0]=injection[0]*unit_change
                                injection[1]=injection[1]*unit_change
                                if injection[2]<=0:
                                    injection[2]=lower_strenght_value
                                profile.append(injection)
                                injection=[]
                        profiles.append(profile)
                    nodes=[[node_names[n],profiles[n]] for n in xrange(0,len(profiles))]
                    results.append([objective,nodes])
                    #print objective
            except StopIteration:
                break

        #print results
        return results

    def printResults(self, result_list, tao, file_name):
        if self.getInversionOption('algorithm') == 'optimization':
            inversion_nodes = []
            objective_val = []
            likely_events = [result for result in result_list if result[0] >= tao]

            '''
            print '\n*********************************************************************\n'
            print '\t\t\tInversion Results\n'
            if self.getInversionOption('num injections') not in self.none_list\
               and self.getInversionOption('num injections') != 1:
                flag=1;strl=''
                for node in result_list[0][1]:
                    strl+=node[0]
                    if flag<len(result_list[0][1]): strl+=','
                    flag+=1
                #print '\tMore likely injection nodes:\t\t\t',strl
            #else:
                #print'\tMore likely injection node:\t\t\t',result_list[0][1][0][0]
            #print'\tStart time of contaminant injection (s):\t',result_list[0][1][0][1][0][0]
            print'\tNumber of candidate events:\t\t\t',len(likely_events)
            print'\tInversion algorithm:\t\t\t\toptimization'
            print'\tInversion model:\t\t\t\t',self.getInversionOption('formulation')
            print'\tAML:\t\t\t\t\t\t',self.getInversionOption('model format')
            print'\tAllowed nodes in file:\t\t\t\t',self.getInversionOption('feasible nodes')
            print '\tDetailed results in:\t' + file_name +'\n'
            print '*********************************************************************\n'
            #
            '''
            
            if self.getInversionOption('num injections') not in self.none_list:
              num_inj = self.getInversionOption('num injections')
            else:
              num_inj = 1
            results_object = []
            for result in result_list:
                if result[0] < tao:
                    continue
                tmp_results = dict()
                nodes_list = []
                for node_i in result[1]:
                    profile_list = []
                    tmp_node_dic = dict()
                    for injection in node_i[1]:
                        #print injection
                        profile_list.append(dict(Start=injection[0], Stop=injection[1], Strength=injection[2]))
                    tmp_node_dic['Name'] = node_i[0]
                    tmp_node_dic['Profile'] = profile_list
                    nodes_list.append(tmp_node_dic)
                tmp_results['Objective'] = result[0]
                objective_val.append(self.trunc(result[0],3))
                tmp_results['Nodes'] = nodes_list
                if num_inj > 1:
                  inversion_nodes.append([i['Name'] for i in nodes_list])
                else:
                  inversion_nodes.append(nodes_list[0]['Name'])
                tmp_results['CPU time'] = time.time() - self.startTime
                tmp_results['run date'] = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                results_object.append(tmp_results)
            f = open(file_name, 'w')
            json.dump(results_object, f,indent=2)
            f.close()
            
            num_events = len(results_object)
            return [num_events, objective_val, inversion_nodes]
        else:
            num_events = len(result_list)
            '''
            print'\n*********************************************************************\n'
            print'\t\t\tInversion Results\n'
            print'\tNumber of candidate events:\t\t\t',num_events
            print'\tInversion algorithm:\t\t\t\tbayesian'
            print'\tAllowed nodes in file:\t\t\t\t',self.getInversionOption('feasible nodes')
            print'\tDetailed results in:\t' + file_name +'\n'
            print'*********************************************************************\n'
            '''
            return num_events

    def validateExecutables(self):
        amplExe = self.getConfigureOption('ampl executable')
        pyomoExe = self.getConfigureOption('pyomo executable')
        if amplExe is not None and not os.path.exists(amplExe):
            if 'ampl' in self.defLocs.keys():
                amplExe = self.defLocs['ampl']
            elif amplExe is not None:
                amplExe = os.path.split(amplExe)[1]
                for p in os.sys.path:
                    f = os.path.join(p,amplExe)
                    if os.path.exists(f) and os.path.isfile(f):
                        amplExe = f
                        break
                    f = os.path.join(p,amplExe+'.exe')
                    if os.path.exists(f) and os.path.isfile(f):
                        amplExe = f
                        break
        if pyomoExe is not None and not os.path.exists(pyomoExe):
            if 'pyomo' in self.defLocs.keys():
                pyomoExe = self.defLocs['pyomo']
            elif pyomoExe is not None:
                pyomoExe = os.path.split(pyomoExe)[1]
                for p in os.sys.path:
                    f = os.path.join(p,pyomoExe)
                    if os.path.exists(f) and os.path.isfile(f):
                        pyomoExe = f
                        break
                    f = os.path.join(p,pyomoExe+'.exe')
                    if os.path.exists(f) and os.path.isfile(f):
                        pyomoExe = f
                        break
        self.setConfigureOption('ampl executable',amplExe)
        self.setConfigureOption('pyomo executable',pyomoExe)

    def validate(self):
        output_prefix = self.getConfigureOption('output prefix')
        self.validateExecutables()
        if output_prefix == '':
            output_prefix = 'invsn'
            self.setConfigureOption('output prefix',output_prefix)
        return

    def createAMPLRunReduceLP(self,filename=None,allowed_list=set([])):

        if filename is None:
            filename = self._get_prefixed_filename('ampl.run')

        ampl_model = os.path.join(os.path.dirname(os.path.abspath(__file__)),'models','ampl','inversion_LP.mod')

        fid = open(filename,'wt')
        fid.write('option presolve 0;\n')
        fid.write('option substout 0;\n')
        fid.write('\n')

        fid.write('# LP source inversion model\n')
        fid.write('model %s;\n'%ampl_model)
        fid.write('\n')

        fid.write('# Source inversion data\n')
        fid.write('data '+self._get_prefixed_filename('CONC.dat')+'\n')
        fid.write('data '+self._get_prefixed_filename('INV_ROWS_INDEX.dat')+'\n')
        fid.write('data '+self._get_prefixed_filename('INV_ROWS_VALS.dat')+'\n')
        results_file = self._get_prefixed_filename('inversion_results.dat')
        if self.getInversionOption('positive threshold') not in self.none_list:
            fid.write('let P_TH_POS := '+str(self.getInversionOption('positive threshold'))+';\n')

        if self.getInversionOption('negative threshold') not in self.none_list:
            fid.write('let P_TH_NEG := '+str(self.getInversionOption('negative threshold'))+';\n')

        fid.write('# Solve the problem\n')
        fid.write('option solver '+self.getSolverOption('type')+';\n')
        # HACK: Not sure what the correct label is for solvers other than
        # cplex and gurobi so I will throw an error if I encounter options.
        # The alternative is to ask the user for the solver executable and this
        # ampl specific label which would be weird. The solver configuration system
        # will likely be updated in the future so this should work for now.
        options_label = ''
        if self.getSolverOption('type') == 'cplexamp':
            options_label += 'cplex_options'
        elif self.getSolverOption('type') == 'gurobi_ampl':
            options_label += 'gurobi_options'
        
        if self.getSolverOption('options') not in self.none_list:
            if options_label != '':
                fid.write('option '+options_label+' \'')
                for (key,value) in self.getSolverOption('options').iteritems():
                    if value in self.none_list:
                        # this is the case where an option does not accept a value
                        fid.write(key+' ')
                    else:
                        fid.write(key+'='+str(value)+' ')
                fid.write('\';\n')
            else:
                print >> sys.stderr, ' '
                print >> sys.stderr, "WARNING: Solver options in AMPL are currently not handled for"
                print >> sys.stderr, "         the specified solver: ", self.getSolverOption('type')
                print >> sys.stderr, "         All solver options will be ignored."
                print >> sys.stderr, ' '


        #fid.write('option solver cplexamp;\n')
        #if self.getSolverOption('options') in self.none_list:
        #fid.write('option cplex_options \'mipdisplay=2\';\n')
        if len(allowed_list)>0:
            fid.write('\n')
            fid.write('set S_ALLOWED_NODES;\n')
            fid.write('let S_ALLOWED_NODES :={')
            i=0
            for allowed_node in allowed_list:
                fid.write(str(allowed_node))
                if(i<len(allowed_list)-1):
                    fid.write(',')
                i+=1
            fid.write('};\n')
            fid.write('\n')

            fid.write('param flag;\n')
            fid.write('printf "Minutes_per_timestep %q\\n",P_MINUTES_PER_TIMESTEP>'+results_file+';\n')
            fid.write('for{n in {S_IMPACT_NODES inter S_ALLOWED_NODES}} \n')
            fid.write('{\n')
            fid.write('\tfor{nn in {S_IMPACT_NODES inter S_ALLOWED_NODES}}\n')
            fid.write('\t{\n')
            fid.write('\t\tunfix{t in S_IMPACT_TIMES[nn]} mn_tox_gpmin[nn,t];\n')
            fid.write('\t}\n')
            fid.write('\n')
            fid.write('\tfor{nn in S_IMPACT_NODES:nn!=n}\n')
            fid.write('\t{\n')
            fid.write('\t\tfix{t in S_IMPACT_TIMES[nn]} mn_tox_gpmin[nn,t]:=0;\n')
            fid.write('\t}\n')
            fid.write('\n')
        else:
            fid.write('\n')
            fid.write('param flag;\n')
            fid.write('printf "Minutes_per_timestep %q\\n",P_MINUTES_PER_TIMESTEP>'+results_file+';\n')
            fid.write('for{n in S_IMPACT_NODES} \n')
            fid.write('{\n')
            fid.write('\tfor{nn in S_IMPACT_NODES}\n')
            fid.write('\t{\n')
            fid.write('\t\tunfix{t in S_IMPACT_TIMES[nn]} mn_tox_gpmin[nn,t];\n')
            fid.write('\t}\n')
            fid.write('\n')
            fid.write('\tfor{nn in S_IMPACT_NODES:nn!=n}\n')
            fid.write('\t{\n')
            fid.write('\t\tfix{t in S_IMPACT_TIMES[nn]} mn_tox_gpmin[nn,t]:=0;\n')
            fid.write('\t}\n')
            fid.write('\n')

        fid.write('\tsolve;\n')
        fid.write('\tprintf "Solution\\t%q\\t%q\\n",n,OBJ >>'+results_file+';\n')
        fid.write('\tlet flag :=0;\n')
        fid.write('\tfor{t in S_IMPACT_TIMES[n]:t != first(S_IMPACT_TIMES[n])}\n')
        fid.write('\t{\n')
        fid.write('\t\tif mn_tox_gpmin[n,prev(t,S_IMPACT_TIMES[n])]>0 then\n')
        fid.write('\t\t{\n')
        fid.write('\t\t\tlet flag := 1;\n')
        fid.write('\t\t\tprintf "%q\\t%q\\t%q\\t",prev(t,S_IMPACT_TIMES[n]),t,mn_tox_gpmin[n,prev(t,S_IMPACT_TIMES[n])]>>'+results_file+';\n')
        fid.write('\t\t}\n')
        fid.write('\t}\n')
        fid.write('\tif flag==0 then\n')
        fid.write('\t{\n')
        fid.write('\t\tprintf "%q\\t%q\\t%q",(P_TIME_STEPS-1),P_TIME_STEPS,0>>'+results_file+';\n')
        fid.write('\t}\n')
        fid.write('\tprintf "\\n">>'+results_file+';\n')
        fid.write('}')
        fid.close()


        return results_file

    def runPYOMOReduceLP(self,allowed_list=set([]),lower_strenght_value=1e-3):

        pyomo_module = os.path.join(os.path.dirname(os.path.abspath(__file__)),'models','pyomo','inversion_LP')

        pm = imp.load_source(os.path.basename(pyomo_module),pyomo_module+".py")
        model = pm.model

        dat1=self._get_prefixed_filename('CONC.dat',tempfile=True)
        dat2=self._get_prefixed_filename('INV_ROWS_INDEX.dat',tempfile=True)
        dat3=self._get_prefixed_filename('INV_ROWS_VALS.dat',tempfile=True)
        
        if self.getInversionOption('positive threshold') not in self.none_list \
           or self.getInversionOption('negative threshold') not in self.none_list:
            with open(dat2, "a") as myfile:
                if self.getInversionOption('positive threshold') not in self.none_list:
                    myfile.write("\n")
                    myfile.write("param P_TH_POS :="+str(self.getInversionOption('positive threshold'))+";\n")
                if self.getInversionOption('negative threshold') not in self.none_list:
                    myfile.write("\n")
                    myfile.write("param P_TH_NEG :="+str(self.getInversionOption('negative threshold'))+";\n")

        modeldata=DataPortal()
        modeldata.load(model=model, filename=dat1)
        modeldata.load(model=model, filename=dat2)
        modeldata.load(model=model, filename=dat3)
        LPmod=model.create_instance(modeldata)
        opt=SolverFactory(self.getSolverOption('type'))
        if self.getSolverOption('options') not in self.none_list:
            for (key,val) in self.getSolverOption('options').iteritems():
                if val in self.none_list:
                    # this is the case where an option does not accept a value
                    opt.options[key] = ''
                else:
                    opt.options[key] = val


        #print "Node_Name   objective value"
        Solution = []
        allowed_list=[n for n in allowed_list if n in LPmod.S_IMPACT_NODES]
        loop_through=LPmod.S_IMPACT_NODES if len(allowed_list)==0 else allowed_list
        unit_change=value(LPmod.P_MINUTES_PER_TIMESTEP)*60
        for n in loop_through:
            profile=[]
            for nn in loop_through:
                for t in LPmod.S_IMPACT_TIMES[nn]:
                    LPmod.mn_tox_gpmin[nn,t].fixed=False

            for (nn,t) in LPmod.S_IMPACT:
                if nn != n:
                    LPmod.mn_tox_gpmin[nn,t].fixed=True
                    LPmod.mn_tox_gpmin[nn,t].value=0

            LPmod.preprocess()
            results = opt.solve(LPmod)
            #LPmod.load(results)
            times_minus_first=[time for time in LPmod.S_IMPACT_TIMES[n]]
            for tt in xrange(1,len(times_minus_first)):
                start=times_minus_first[tt-1]*unit_change
                end=times_minus_first[tt]*unit_change
                strength=value(LPmod.mn_tox_gpmin[n,times_minus_first[tt-1]])
                if strength>0:
                    profile.append([start,end,strength])
            if len(profile)==0:
                start=(value(LPmod.P_TIME_STEPS)-1)*unit_change
                end=value(LPmod.P_TIME_STEPS)*unit_change
                profile.append([start,end,lower_strenght_value])
            Solution.append([value(LPmod.OBJ),[[str(n),profile]]])
        return Solution

    def createAMPLRunReduceMIPnd(self,filename=None,allowed_list=set([])):
        
        if filename is None:
            filename = self._get_prefixed_filename('ampl.run')

        ampl_model = os.path.join(os.path.dirname(os.path.abspath(__file__)),'models','ampl','inversion_MIP_nd.mod')

        fid = open(filename,'wt')
        fid.write('option presolve 0;\n')
        fid.write('option substout 0;\n')
        fid.write('\n')

        fid.write('# MIP source inversion model\n')
        fid.write('model %s;\n'%ampl_model)
        fid.write('\n')

        fid.write('# Source inversion data\n')
        fid.write('data '+self._get_prefixed_filename('CONC.dat')+'\n')
        fid.write('data '+self._get_prefixed_filename('INV_ROWS_INDEX.dat')+'\n')
        fid.write('data '+self._get_prefixed_filename('INV_ROWS_VALS.dat')+'\n')
        fid.write('\n')

        if self.getInversionOption('positive threshold') not in self.none_list:
            fid.write('let P_TH_POS := '+str(self.getInversionOption('positive threshold'))+';\n')

        if self.getInversionOption('negative threshold') not in self.none_list:
            fid.write('let P_TH_NEG := '+str(self.getInversionOption('negative threshold'))+';\n')

        if self.getInversionOption('num injections') not in self.none_list:
            fid.write('let N_INJECTIONS :=' + str(self.getInversionOption('num injections'))+';\n')

        fid.write('# Solve the problem\n')
        fid.write('option solver '+self.getSolverOption('type')+';\n')
        # HACK: Not sure what the correct label is for solvers other than
        # cplex and gurobi so I will throw an error if I encounter options.
        # The alternative is to ask the user for the solver executable and this
        # ampl specific label which would be weird. The solver configuration system
        # will likely be updated in the future so this should work for now.
        options_label = ''
        if self.getSolverOption('type') == 'cplexamp':
            options_label += 'cplex_options'
        elif self.getSolverOption('type') == 'gurobi_ampl':
            options_label += 'gurobi_options'
        
        if self.getSolverOption('options') not in self.none_list:
            if options_label != '':
                fid.write('option '+options_label+' \'')
                for (key,value) in self.getSolverOption('options').iteritems():
                    if value in self.none_list:
                        # this is the case where an option does not accept a value
                        fid.write(key+' ')
                    else:
                        fid.write(key+'='+str(value)+' ')
                fid.write('\';\n')
            else:
                print >> sys.stderr, ' '
                print >> sys.stderr, "WARNING: Solver options in AMPL are currently not handled for"
                print >> sys.stderr, "         the specified solver: ", self.getSolverOption('type')
                print >> sys.stderr, "         All solver options will be ignored."
                print >> sys.stderr, ' '


        #fid.write('option solver cplexamp;\n')
        #if self.getSolverOption('options') is None:
        #fid.write('option cplex_options \'mipdisplay=2\';\n')

        if len(allowed_list)>0:
            fid.write('\n')
            fid.write('set S_ALLOWED_NODES;\n')
            fid.write('let S_ALLOWED_NODES :={')
            i=0
            for allowed_node in allowed_list:
                fid.write(str(allowed_node))
                if(i<len(allowed_list)-1):
                    fid.write(',')
                i+=1
            fid.write('};\n')
            fid.write('fix{n in S_IMPACT_NODES diff S_ALLOWED_NODES} y[n]:=0;\n')
            fid.write('\n')

        fid.write('solve;\n')
        results_file = self._get_prefixed_filename('inversion_results.dat')
        fid.write('printf "Minutes_per_timestep %q\\n",P_MINUTES_PER_TIMESTEP>'+results_file+';\n')
        fid.write('printf "Solution" >>'+results_file+';\n')
        fid.write('param flag;\n')
        fid.write('for{n in S_IMPACT_NODES:y[n]!=0}\n')
        fid.write('{\n')
        fid.write('\tprintf "\\t%q",n>>'+results_file+';\n')
        fid.write('}\n')
        fid.write('printf "\\t%q\\n",OBJ>>'+results_file+';\n')
        fid.write('for{n in S_IMPACT_NODES:y[n]!=0}\n')
        fid.write('{\n')
        fid.write('\tlet flag := 0;\n')
        fid.write('\tfor{t in S_IMPACT_TIMES[n]:t != first(S_IMPACT_TIMES[n])}\n')
        fid.write('\t{\n')
        fid.write('\t\tif mn_tox_gpmin[n,prev(t,S_IMPACT_TIMES[n])]>0 then\n')
        fid.write('\t\t{\n')
        fid.write('\t\tlet flag := 1;\n')
        fid.write('\t\t\tprintf "%q\\t%q\\t%q\\t",prev(t,S_IMPACT_TIMES[n]),t,mn_tox_gpmin[n,prev(t,S_IMPACT_TIMES[n])]>>'+results_file+';\n')
        fid.write('\t\t}\n')
        fid.write('\t}\n')
        fid.write('\tif flag ==0 then')
        fid.write('\t{\n')
        fid.write('\t\tprintf "%q\\t%q\\t%q\\t",(P_TIME_STEPS-1),P_TIME_STEPS,0>>'+results_file+';\n')
        fid.write('\t}\n')
        fid.write('\tprintf "\\n">>'+results_file+';\n')
        fid.write('}\n')
        fid.close()



        return results_file

    def createAMPLRunReduceMIP(self,filename=None,allowed_list=set([])):

        if filename is None:
            filename = self._get_prefixed_filename('ampl.run')

        ampl_model = os.path.join(os.path.dirname(os.path.abspath(__file__)),'models','ampl','inversion_MIP.mod')

        fid = open(filename,'wt')
        fid.write('option presolve 0;\n')
        fid.write('option substout 0;\n')
        fid.write('\n')

        fid.write('# MIP source inversion model\n')
        fid.write('model %s;\n'%ampl_model)
        fid.write('\n')

        fid.write('# Source inversion data\n')
        fid.write('data '+self._get_prefixed_filename('CONC.dat')+'\n')
        fid.write('data '+self._get_prefixed_filename('INV_ROWS_INDEX.dat')+'\n')
        fid.write('data '+self._get_prefixed_filename('INV_ROWS_VALS.dat')+'\n')
        fid.write('\n')

        if self.getInversionOption('positive threshold') not in self.none_list:
            fid.write('let P_TH_POS := '+str(self.getInversionOption('positive threshold'))+';\n')

        if self.getInversionOption('negative threshold') not in self.none_list:
            fid.write('let P_TH_NEG := '+str(self.getInversionOption('negative threshold'))+';\n')

        if self.getInversionOption('num injections') not in self.none_list:
            fid.write('let N_INJECTIONS :=' + str(self.getInversionOption('num injections'))+';\n')

        fid.write('# Solve the problem\n')
        fid.write('option solver '+self.getSolverOption('type')+';\n')
        # HACK: Not sure what the correct label is for solvers other than
        # cplex and gurobi so I will throw an error if I encounter options.
        # The alternative is to ask the user for the solver executable and this
        # ampl specific label which would be weird. The solver configuration system
        # will likely be updated in the future so this should work for now.
        options_label = ''
        if self.getSolverOption('type') == 'cplexamp':
            options_label += 'cplex_options'
        elif self.getSolverOption('type') == 'gurobi_ampl':
            options_label += 'gurobi_options'
        
        if self.getSolverOption('options') not in self.none_list:
            if options_label != '':
                fid.write('option '+options_label+' \'')
                for (key,value) in self.getSolverOption('options').iteritems():
                    if value in self.none_list:
                        # this is the case where an option does not accept a value
                        fid.write(key+' ')
                    else:
                        fid.write(key+'='+str(value)+' ')
                fid.write('\';\n')
            else:
                print >> sys.stderr, ' '
                print >> sys.stderr, "WARNING: Solver options in AMPL are currently not handled for"
                print >> sys.stderr, "         the specified solver: ", self.getSolverOption('type')
                print >> sys.stderr, "         All solver options will be ignored."
                print >> sys.stderr, ' '


        #fid.write('option solver cplexamp;\n')
        #if self.getSolverOption('options') is None:
        #fid.write('option cplex_options \'mipdisplay=2\';\n')

        if len(allowed_list)>0:
            fid.write('\n')
            fid.write('set S_ALLOWED_NODES;\n')
            fid.write('let S_ALLOWED_NODES :={')
            i=0
            for allowed_node in allowed_list:
                fid.write(str(allowed_node))
                if(i<len(allowed_list)-1):
                    fid.write(',')
                i+=1
            fid.write('};\n')
            fid.write('fix{n in S_IMPACT_NODES diff S_ALLOWED_NODES} y[n]:=0;\n')
            fid.write('\n')

        fid.write('solve;\n')
        results_file = self._get_prefixed_filename('inversion_results.dat')
        fid.write('printf "Minutes_per_timestep %q\\n",P_MINUTES_PER_TIMESTEP>'+results_file+';\n')
        fid.write('printf "Solution" >>'+results_file+';\n')
        fid.write('param flag;\n')
        fid.write('for{n in S_IMPACT_NODES:y[n]!=0}\n')
        fid.write('{\n')
        fid.write('\tprintf "\\t%q",n>>'+results_file+';\n')
        fid.write('}\n')
        fid.write('printf "\\t%q\\n",OBJ>>'+results_file+';\n')
        fid.write('for{n in S_IMPACT_NODES:y[n]!=0}\n')
        fid.write('{\n')
        fid.write('\tlet flag := 0;\n')
        fid.write('\tfor{t in S_IMPACT_TIMES[n]:t != first(S_IMPACT_TIMES[n])}\n')
        fid.write('\t{\n')
        fid.write('\t\tif mn_tox_gpmin[n,prev(t,S_IMPACT_TIMES[n])]>0 then\n')
        fid.write('\t\t{\n')
        fid.write('\t\tlet flag := 1;\n')
        fid.write('\t\t\tprintf "%q\\t%q\\t%q\\t",prev(t,S_IMPACT_TIMES[n]),t,mn_tox_gpmin[n,prev(t,S_IMPACT_TIMES[n])]>>'+results_file+';\n')
        fid.write('\t\t}\n')
        fid.write('\t}\n')
        fid.write('\tif flag ==0 then')
        fid.write('\t{\n')
        fid.write('\t\tprintf "%q\\t%q\\t%q\\t",(P_TIME_STEPS-1),P_TIME_STEPS,0>>'+results_file+';\n')
        fid.write('\t}\n')
        fid.write('\tprintf "\\n">>'+results_file+';\n')
        fid.write('}\n')
        fid.close()

        return results_file

    def runPYOMOReduceMIPnd(self,allowed_list=set([]),lower_strenght_value=1e-3):

        pyomo_module = os.path.join(os.path.dirname(os.path.abspath(__file__)),'models','pyomo','inversion_MIP_nd')

        pm = imp.load_source(os.path.basename(pyomo_module),pyomo_module+".py")
        model = pm.model

        dat1=self._get_prefixed_filename('CONC.dat',tempfile=True)
        dat2=self._get_prefixed_filename('INV_ROWS_INDEX.dat',tempfile=True)
        dat3=self._get_prefixed_filename('INV_ROWS_VALS.dat',tempfile=True)
        
        if self.getInversionOption('num injections') not in self.none_list\
           and self.getInversionOption('num injections') != 1:
            with open(dat2, "a") as myfile:
                myfile.write("\n")
                myfile.write("param N_INJECTIONS :="+str(self.getInversionOption('num injections'))+";\n")

        if self.getInversionOption('positive threshold') not in self.none_list \
           or self.getInversionOption('negative threshold') not in self.none_list:
            with open(dat2, "a") as myfile:
                if self.getInversionOption('positive threshold') not in self.none_list:
                    myfile.write("\n")
                    myfile.write("param P_TH_POS :="+str(self.getInversionOption('positive threshold'))+";\n")
                if self.getInversionOption('negative threshold') not in self.none_list:
                    myfile.write("\n")
                    myfile.write("param P_TH_NEG :="+str(self.getInversionOption('negative threshold'))+";\n")


        modeldata=DataPortal()
        modeldata.load(model=model, filename=dat1)
        modeldata.load(model=model, filename=dat2)
        modeldata.load(model=model, filename=dat3)
        MIPmodel=model.create_instance(modeldata)
        MIPmodel._defer_construction=False
        #opt=SolverFactory("cplex")
        opt=SolverFactory(self.getSolverOption('type'))
        if self.getSolverOption('options') not in self.none_list:
            for (key,val) in self.getSolverOption('options').iteritems():
                if val in self.none_list:
                    # this is the case where an option does not accept a value
                    opt.options[key] = ''
                else:
                    opt.options[key] = val



        if len(allowed_list)!=0:
            to_be_fixed=[n for n in MIPmodel.S_IMPACT_NODES if n not in allowed_list]
            for n in to_be_fixed:
                MIPmodel.y[n].fixed=True
                MIPmodel.y[n].value=0
            MIPmodel.preprocess()

        Solution = []
        unit_change=value(MIPmodel.P_MINUTES_PER_TIMESTEP)*60
        if self.getInversionOption('num injections') not in self.none_list\
           and self.getInversionOption('num injections') != 1:
            MIPmodel.N_INJECTIONS=self.getInversionOption('num injections')

            MIPmodel.preprocess()
            results = opt.solve(MIPmodel)
            #MIPmodel.load(results)
            ObjVals = [value(MIPmodel.OBJ),value(MIPmodel.OBJ)]
            percentage=0.1
            i=0
            MAX=30
            while (ObjVals[-1]<=ObjVals[-2]*(1+percentage)) and i<MAX:
                cuts_on = []
                cuts_off = []

                results = opt.solve(MIPmodel)
                #MIPmodel.load(results)
                for n in MIPmodel.S_IMPACT_NODES:
                    if int(round(MIPmodel.y[n].value)) == 1:
                        cuts_on.append(n)
                    else:
                        cuts_off.append(n)
                nodes_=[]
                for n in cuts_on:
                    profile=[]
                    times_minus_first=[time for time in MIPmodel.S_IMPACT_TIMES[n]]
                    for tt in xrange(1,len(times_minus_first)):
                        start=times_minus_first[tt-1]*unit_change
                        end=times_minus_first[tt]*unit_change
                        strength=value(MIPmodel.mn_tox_gpmin[n,times_minus_first[tt-1]])
                        if strength>0:profile.append([start,end,strength])
                    if len(profile)==0:
                        start=(value(MIPmodel.P_TIME_STEPS)-1)*unit_change
                        end=value(MIPmodel.P_TIME_STEPS)*unit_change
                        profile.append([start,end,lower_strenght_value])
                    nodes_.append([str(n),profile])
                # Add objective and potential injection nodes to the list
                Solution.append([float(ObjVals[-1]),nodes_])
                # Define rule for integer cut
                def int_cut_rule(m):
                    return sum( (1-m.y[r]) for (r) in cuts_on) + \
                           sum(   m.y[r]   for (r) in cuts_off) \
                           >= 1
                # Add new cut to the model
                setattr(MIPmodel,'int_cut_'+str(i), Constraint(rule=int_cut_rule))
                MIPmodel.preprocess()
                # determine new objective
                ObjVals.append(value(MIPmodel.OBJ))
                i+=1

        else:

            results = opt.solve(MIPmodel)
            #MIPmodel.load(results)

            nodes_=[]
            for n in MIPmodel.S_IMPACT_NODES:
                if int(round(MIPmodel.y[n].value)) == 1:
                    profile=[]
                    times_minus_first=[time for time in MIPmodel.S_IMPACT_TIMES[n]]
                    for tt in xrange(1,len(times_minus_first)):
                        start=times_minus_first[tt-1]*unit_change
                        end=times_minus_first[tt]*unit_change
                        strength=value(MIPmodel.mn_tox_gpmin[n,times_minus_first[tt-1]])
                        if strength>0:
                            profile.append([start,end,strength])
                    if len(profile)==0:
                        start=(value(MIPmodel.P_TIME_STEPS)-1)*unit_change
                        end=value(MIPmodel.P_TIME_STEPS)*unit_change
                        profile.append([start,end,lower_strenght_value])
                    nodes_.append([str(n),profile])
            Solution.append([float(value(MIPmodel.OBJ)),nodes_])
        return Solution

    def runPYOMOReduceMIP(self,allowed_list=set([]),lower_strenght_value=1e-3):

        pyomo_module = os.path.join(os.path.dirname(os.path.abspath(__file__)),'models','pyomo','inversion_MIP')

        pm = imp.load_source(os.path.basename(pyomo_module),pyomo_module+".py")
        model = pm.model

        dat1=self._get_prefixed_filename('CONC.dat',tempfile=True)
        dat2=self._get_prefixed_filename('INV_ROWS_INDEX.dat',tempfile=True)
        dat3=self._get_prefixed_filename('INV_ROWS_VALS.dat',tempfile=True)

        if self.getInversionOption('num injections') not in self.none_list\
           and self.getInversionOption('num injections') != 1:
            with open(dat2, "a") as myfile:
                myfile.write("\n")
                myfile.write("param N_INJECTIONS :="+str(self.getInversionOption('num injections'))+";\n")

        if self.getInversionOption('positive threshold') not in self.none_list \
           or self.getInversionOption('negative threshold') not in self.none_list:
            with open(dat2, "a") as myfile:
                if self.getInversionOption('positive threshold') not in self.none_list:
                    myfile.write("\n")
                    myfile.write("param P_TH_POS :="+str(self.getInversionOption('positive threshold'))+";\n")
                if self.getInversionOption('negative threshold') not in self.none_list:
                    myfile.write("\n")
                    myfile.write("param P_TH_NEG :="+str(self.getInversionOption('negative threshold'))+";\n")

        modeldata=DataPortal()
        modeldata.load(model=model, filename=dat1)
        modeldata.load(model=model, filename=dat2)
        modeldata.load(model=model, filename=dat3)
        MIPmodel=model.create_instance(modeldata)
        MIPmodel._defer_construction=False
        #opt=SolverFactory("cplex")
        opt=SolverFactory(self.getSolverOption('type'))
        if self.getSolverOption('options') not in self.none_list:
            for (key,val) in self.getSolverOption('options').iteritems():
                if val in self.none_list:
                    # this is the case where an option does not accept a value
                    opt.options[key] = ''
                else:
                    opt.options[key] = val



        if len(allowed_list)!=0:
            to_be_fixed=[n for n in MIPmodel.S_IMPACT_NODES if n not in allowed_list]
            for n in to_be_fixed:
                MIPmodel.y[n].fixed=True
                MIPmodel.y[n].value=0
            MIPmodel.preprocess()

        Solution = []
        unit_change=value(MIPmodel.P_MINUTES_PER_TIMESTEP)*60
        if self.getInversionOption('num injections') not in self.none_list\
           and self.getInversionOption('num injections') != 1:
            MIPmodel.N_INJECTIONS=self.getInversionOption('num injections')

            MIPmodel.preprocess()
            results = opt.solve(MIPmodel)
            #MIPmodel.load(results)
            ObjVals = [value(MIPmodel.OBJ),value(MIPmodel.OBJ)]
            percentage=0.1
            i=0;MAX=30
            while (ObjVals[-1]<=ObjVals[-2]*(1+percentage)) and i<MAX:
                cuts_on = []
                cuts_off = []

                results = opt.solve(MIPmodel)
                #MIPmodel.load(results)
                for n in MIPmodel.S_IMPACT_NODES:
                    if int(round(MIPmodel.y[n].value)) == 1:
                        cuts_on.append(n)
                    else:
                        cuts_off.append(n)
                nodes_=[]
                for n in cuts_on:
                    profile=[]
                    times_minus_first=[time for time in MIPmodel.S_IMPACT_TIMES[n]]
                    for tt in xrange(1,len(times_minus_first)):
                        start=times_minus_first[tt-1]*unit_change
                        end=times_minus_first[tt]*unit_change
                        strength=value(MIPmodel.mn_tox_gpmin[n,times_minus_first[tt-1]])
                        if strength>0:profile.append([start,end,strength])
                    if len(profile)==0:
                        start=(value(MIPmodel.P_TIME_STEPS)-1)*unit_change
                        end=value(MIPmodel.P_TIME_STEPS)*unit_change
                        profile.append([start,end,lower_strenght_value])
                    nodes_.append([str(n),profile])
                # Add objective and potential injection nodes to the list
                Solution.append([float(ObjVals[-1]),nodes_])
                # Define rule for integer cut
                def int_cut_rule(m):
                    return sum( (1-m.y[r]) for (r) in cuts_on) + \
                           sum(   m.y[r]   for (r) in cuts_off) \
                           >= 1
                # Add new cut to the model
                setattr(MIPmodel,'int_cut_'+str(i), Constraint(rule=int_cut_rule))
                MIPmodel.preprocess()
                # determine new objective
                ObjVals.append(value(MIPmodel.OBJ))
                i+=1

        else:

            results = opt.solve(MIPmodel)
            #MIPmodel.load(results)

            nodes_=[]
            for n in MIPmodel.S_IMPACT_NODES:
                if int(round(MIPmodel.y[n].value)) == 1:
                    profile=[]
                    times_minus_first=[time for time in MIPmodel.S_IMPACT_TIMES[n]]
                    for tt in xrange(1,len(times_minus_first)):
                        start=times_minus_first[tt-1]*unit_change
                        end=times_minus_first[tt]*unit_change
                        strength=value(MIPmodel.mn_tox_gpmin[n,times_minus_first[tt-1]])
                        if strength>0:
                            profile.append([start,end,strength])
                    if len(profile)==0:
                        start=(value(MIPmodel.P_TIME_STEPS)-1)*unit_change
                        end=value(MIPmodel.P_TIME_STEPS)*unit_change
                        profile.append([start,end,lower_strenght_value])
                    nodes_.append([str(n),profile])
            Solution.append([float(value(MIPmodel.OBJ)),nodes_])
        return Solution

    def createAMPLRunStep(self,filename=None,allowed_list=set([])):

        if filename is None:
            filename = self._get_prefixed_filename('ampl.run')

        ampl_model = os.path.join(os.path.dirname(os.path.abspath(__file__)),'models','ampl','MIP_step.mod')

        runfile = open(filename,'wt')
        runfile.write('option presolve 0;\n')
        runfile.write('option substout 0;\n')
        runfile.write('\n')

        runfile.write('# MIP step_inversion model\n')
        runfile.write('model %s;\n'%ampl_model)
        runfile.write('\n')

        runfile.write('# Source inversion data\n')
        runfile.write('data '+self._get_prefixed_filename('CONC.dat')+'\n')
        runfile.write('data '+self._get_prefixed_filename('INV_ROWS_INDEX.dat')+'\n')
        runfile.write('data '+self._get_prefixed_filename('INV_ROWS_VALS.dat')+'\n')
        results_file = self._get_prefixed_filename('inversion_results.dat')
        if self.getInversionOption('positive threshold') not in self.none_list:
            runfile.write('let P_TH_POS := '+str(self.getInversionOption('positive threshold'))+';\n')
            
        if self.getInversionOption('negative threshold') not in self.none_list:
            runfile.write('let P_TH_NEG := '+str(self.getInversionOption('negative threshold'))+';\n')
        if len(allowed_list)>0:

            runfile.write('\n')
            runfile.write('set S_ALLOWED_NODES;\n')
            runfile.write('let S_ALLOWED_NODES :={')
            i=0
            for allowed_node in allowed_list:
                runfile.write(str(allowed_node))
                if(i<len(allowed_list)-1):
                    runfile.write(',')
                i+=1
            runfile.write('};\n')
            runfile.write('\n')

        runfile.write('# Solve the problem\n')
        runfile.write('option solver '+self.getSolverOption('type')+';\n')
        # HACK: Not sure what the correct label is for solvers other than
        # cplex and gurobi so I will throw an error if I encounter options.
        # The alternative is to ask the user for the solver executable and this
        # ampl specific label which would be weird. The solver configuration system
        # will likely be updated in the future so this should work for now.
        options_label = ''
        if self.getSolverOption('type') == 'cplexamp':
            options_label += 'cplex_options'
        elif self.getSolverOption('type') == 'gurobi_ampl':
            options_label += 'gurobi_options'
            
        if self.getSolverOption('options') not in self.none_list:
            if options_label != '':
                runfile.write('option '+options_label+' \'')
                for (key,value) in self.getSolverOption('options').iteritems():
                    if value in self.none_list:
                        # this is the case where an option does not accept a value
                        runfile.write(key+' ')
                    else:
                        runfile.write(key+'='+str(value)+' ')
                runfile.write('\';\n')
            else:
                print >> sys.stderr, ' '
                print >> sys.stderr, "WARNING: Solver options in AMPL are currently not handled for"
                print >> sys.stderr, "         the specified solver: ", self.getSolverOption('type')
                print >> sys.stderr, "         All solver options will be ignored."
                print >> sys.stderr, ' '


            #runfile.write('option solver cplexamp;\n')
            #runfile.write('option cplex_options \'timing=2 mipdisplay=2\';\n\n')
        runfile.write('\n')
        runfile.write('printf "Timestep_minutes=\\t%q\\n",P_MINUTES_PER_TIMESTEP>'+results_file+';\n')
        runfile.write('param start_time;\n')
        runfile.write('for{n in S_IMPACT_NODES}\n')
        runfile.write('{\n')
        runfile.write('\tunfix {nn in S_IMPACT_NODES,t in S_ALL_TIMES} y[nn,t];\n')
        runfile.write('\tfix {nn in S_IMPACT_NODES,t in S_ALL_TIMES:nn!=n} y[nn,t]:=0;\n')
        runfile.write('\tsolve;\n')
        runfile.write('\tprintf "Solution\\t%q\\t%q\\n",n,OBJ>>'+results_file+';\n')
        runfile.write('\tif card({t in S_ALL_TIMES:y[n,t]!=0})=0 then let start_time := P_TIME_STEPS-1;\n')
        runfile.write('\telse\n')
        runfile.write('\t{\n')
        runfile.write('\t\tif y[n,first(S_ALL_TIMES)]==1 then let start_time := first(S_ALL_TIMES);\n')
        runfile.write('\t\telse\n')
        runfile.write('\t\t{\n')
        runfile.write('\t\t\tfor{t in S_ALL_TIMES:t!=first(S_ALL_TIMES)}\n')
        runfile.write('\t\t\t{\n')
        runfile.write('\t\t\t\tif y[n,t]!=y[n,prev(t)] then let start_time := t\n')
        runfile.write('\t\t\t}\n')
        runfile.write('\t\t}\n')
        runfile.write('\t}\n')
        runfile.write('\tprintf "%q\\t%q\\t%q\\n",start_time,P_TIME_STEPS,strength>>'+results_file+';\n')
        runfile.write('}\n')

        runfile.close()
        return results_file

    def runPYOMOStep(self,allowed_list=set([]),lower_strenght_value=1e-3):

        pyomo_module = os.path.join(os.path.dirname(os.path.abspath(__file__)),'models','pyomo','step_MIP')

        pm = imp.load_source(os.path.basename(pyomo_module),pyomo_module+".py")
        model = pm.model

        dat1=self._get_prefixed_filename('CONC.dat',tempfile=True)
        dat2=self._get_prefixed_filename('INV_ROWS_INDEX.dat',tempfile=True)
        dat3=self._get_prefixed_filename('INV_ROWS_VALS.dat',tempfile=True)
        
        if self.getInversionOption('positive threshold') not in self.none_list \
           or self.getInversionOption('negative threshold') not in self.none_list:
            with open(dat2, "a") as myfile:
                if self.getInversionOption('positive threshold') not in self.none_list:
                    myfile.write("\n")
                    myfile.write("param P_TH_POS :="+str(self.getInversionOption('positive threshold'))+";\n")
                if self.getInversionOption('negative threshold') not in self.none_list:
                    myfile.write("\n")
                    myfile.write("param P_TH_NEG :="+str(self.getInversionOption('negative threshold'))+";\n")

        modeldata=DataPortal()
        modeldata.load(model=model, filename=dat1)
        modeldata.load(model=model, filename=dat2)
        modeldata.load(model=model, filename=dat3)
        Stepmod=model.create_instance(modeldata)
        #opt=SolverFactory("cplex")
        opt=SolverFactory(self.getSolverOption('type'))
        if self.getSolverOption('options') not in self.none_list:
            for (key,val) in self.getSolverOption('options').iteritems():
                if val in self.none_list:
                    # this is the case where an option does not accept a value
                    opt.options[key] = ''
                else:
                    opt.options[key] = val

                    
        #start_timing=time()
        Solution=[]
        allowed_list=[n for n in allowed_list if n in Stepmod.S_IMPACT_NODES]
        loop_through=Stepmod.S_IMPACT_NODES if len(allowed_list)==0 else allowed_list
        unit_change=value(Stepmod.P_MINUTES_PER_TIMESTEP)*60
        for n in Stepmod.S_IMPACT_NODES:
            for t in Stepmod.S_ALL_TIMES:
                Stepmod.y[n,t].value = 0
                Stepmod.y[n,t].fixed = True

        for n in loop_through:
            #unfix binary variables
            for t in Stepmod.S_ALL_TIMES:
                Stepmod.y[n,t].fixed = False

            Stepmod.preprocess()
            solved_instance=opt.solve(Stepmod)
            #Stepmod.load(solved_instance)
            start_time=0
            stop_time=value(Stepmod.P_TIME_STEPS)*unit_change
            injection_strength=value(Stepmod.strength)
            timesteps=[ts for ts in Stepmod.S_ALL_TIMES if Stepmod.y[n,ts].value!=0]
            if len(timesteps)>0:
                start_time=timesteps[0]*unit_change
            else:
                start_time=(value(Stepmod.P_TIME_STEPS)-1)*unit_change
            if injection_strength<=0:
                injection_strength=lower_strenght_value
            profile=[start_time,stop_time,injection_strength]
            Solution.append([value(Stepmod.OBJ),[[str(n),[profile]]]])

            for t in Stepmod.S_ALL_TIMES:
                Stepmod.y[n,t].value = 0
                Stepmod.y[n,t].fixed = True

        #stop_timing=time()
        #print 'PYOMO timing',stop_timing-start_timing


        return Solution

    # the profile output file is used by the grab sample code
    def writeProfiles(self, results_list, tao):
        
        output_prefix = self.getConfigureOption('output prefix')
        if output_prefix in self.none_list:
            output_prefix = ''
        if self.getInversionOption('algorithm') == 'bayesian':
            # the profile file is actually printed by the c++ code for the probablility algorithm
            if output_prefix == '':
                filename = 'profile.tsg'
                impact_nodes_file = 'Likely_Nodes.dat'
            else:
                filename = output_prefix + '_profile.tsg'
                impact_nodes_file = output_prefix + 'Likely_Nodes.dat'
        else:
            if output_prefix == '':
                filename = 'profile.tsg'
                impact_nodes_file = 'Likely_Nodes.dat'
            else:
                filename = output_prefix + 'profile.tsg'
                impact_nodes_file = output_prefix + 'Likely_Nodes.dat'
            #
            profile = open(filename, 'w')
            impact_nodes = open(impact_nodes_file, 'w')
            #print tao
            events = 0
            for result in results_list:
                if result[0] >= tao:
                    events += 1
                    for node in result[1]:
                            #print node
                        if self.getInversionOption('output impact nodes'):
                            impact_nodes.write(node[0] + '\n')
                        profile.write(node[0]+ '\tMASS\t' + str(node[1][0][2]) + '\t' + str(node[1][0][0]) + '\t' + str(node[1][0][1]) + '\n')
            profile.close()
            impact_nodes.close()

        return filename, impact_nodes_file

    def writeCSAInputFiles(self):
        # Prefix
        if self.getConfigureOption('output prefix') not in self.none_list:
            output_prefix = self.getConfigureOption("output prefix")
        else:
            output_prefix = ""
        # Read measurements
        csa_measures = {}
        if self.getMeasurementOption('grab samples') not in self.none_list:
            meas_file_name = self.getMeasurementOption('grab samples')
            meas_file = open(meas_file_name, "r")
            for line in meas_file:
                line = line.strip()
                if len(line) == 0: continue
                if line[0] == "#" : continue
                [sensor, time, meas] = line.split()
                if csa_measures.has_key(sensor):
                    csa_measures[sensor]['meas'].append(meas)
                    csa_measures[sensor]['time'].append(int(time))
                    csa_measures[sensor]['length'] += 1
                else:
                    csa_measures[sensor] = {'meas':[meas], 'time':[int(time)], 'length': 1}
            meas_file.close()
            all_meas_length = [s['length'] for s in csa_measures.itervalues()]
            meas_length = all_meas_length[0] # Used when writing meas file
            assert max(all_meas_length) == min(all_meas_length) != 1, "INPUT ERROR: CSA algorithm does not support grabsamples. Make " + \
                "sure the length of measurements from each sensor is the same. Also make sure there are more than 1 measurements."
        else:
            raise IOError("ERROR: Measurements file not specified.")
        # Write CSA sensor file
        csa_sensor_file = open(output_prefix+"csa_sensors", 'w')
        for key in csa_measures.iterkeys():
            csa_sensor_file.write(key+'\n')
        csa_sensor_file.close()
        # Write CSA Measurements file
        csa_meas_file = open(output_prefix+"csa_measurements",'w')
        for t in range(0,meas_length):
            for sensor_ in csa_measures.itervalues():
                csa_meas_file.write(sensor_['meas'][t]+'\t')
            csa_meas_file.write('\n')
        csa_meas_file.close()
        # Calculate return values
        num_sensors =  len(csa_measures)
        item = csa_measures.popitem()
        meas_time_step_sec = item[1]['time'][1] - item[1]['time'][0]
        sim_stop_time = (item[1]['time'][-1])/3600.0 #one step ahead
        #logger.debug("SIM STOP TIME: ",str(sim_stop_time))
        
        return [num_sensors, meas_time_step_sec, sim_stop_time] 
        
    def readCSAOutputFiles(self, sim_stop_time, meas_step_sec):
        if self.getConfigureOption('output prefix') not in self.none_list:
            output_prefix = self.getConfigureOption("output prefix")
        else:
            output_prefix = ""
        Smatrix = []
        matrix_file_name = output_prefix + "Smatrix.txt"
        matrix_file = open(matrix_file_name, 'r')
        sim_stop_time = round(sim_stop_time,2)
        time_found = False
        for line in matrix_file:
            if time_found:
                Smatrix.append(line.split())
            if str(sim_stop_time) in line.strip():
                time_found = True
        matrix_file.close()

        # Loop through each node and time and save solution
        horizon_sec = sim_stop_time*3600.0
        # Change value if specified
        if self.getInversionOption('horizon') not in self.none_list:
          horizon_sec = self.getInversionOption('horizon')*60.0
        result_window_start_time = sim_stop_time*3600.0 - horizon_sec
        inj_stop_time_sec = sim_stop_time*3600.0 #Assumption 
        inj_strength = 1000 #Assumption
        node_count = len(Smatrix)
        time_count = len(Smatrix[0])
        # Load epanet data to get node names
        try:
            enData = pyepanet.ENepanet()
            enData.ENopen(self.opts['network']['epanet file'],'tmp.rpt')
        except:
            msg = 'EPANET inp file not loaded using pyepanet'
            logger.error(msg)
            raise RuntimeError(msg)
        all_nodes_with_obj = []
        for i in range(node_count):
            try:
                node = enData.ENgetnodeid(i+1)
            except:
                msg = 'Pyepanet could not find node id'
                logger.error(msg)
                raise RuntimeError(msg)
            all_node_inj_profile = []
            for j in range(time_count):
                if float(Smatrix[i][j]) > 0.9:
                    inj_start_time_sec = result_window_start_time + j*meas_step_sec
                    inj_profile = [inj_start_time_sec,inj_stop_time_sec,inj_strength]
                    all_node_inj_profile.append(inj_profile)
            if len(all_node_inj_profile) > 0:
                all_nodes_with_obj.append([node,all_node_inj_profile])

        enData.ENclose()
        Solution = [[1, all_nodes_with_obj]] #All events are assumed to have the same objective = 1 
        
        return Solution

    def writeCSAresults(self,Solution):
        output_prefix = self.getConfigureOption('output prefix') 
        if output_prefix not in self.none_list:    
            json_file = self.getConfigureOption('output prefix') + '_inversion.json'
            tsg_filename = output_prefix + 'profile.tsg'
            impact_nodes_file = output_prefix + 'Likely_Nodes.dat'
        else:
            json_file = 'inversion.json'
            tsg_filename = 'profile.tsg'
            impact_nodes_file = 'Likely_Nodes.dat'
        json_file_wDir = os.path.join(os.path.abspath(os.curdir),json_file)
        wst_util.declare_tempfile(json_file_wDir)
        num_events = len(Solution[0][1])
        
        '''
        print '\n*********************************************************************\n'
        print '\t\t\tInversion Results\n'
        print'\tNumber of candidate events:\t\t\t',num_events
        print'\tInversion algorithm:\t\t\t\tCSA'
        print '\tDetailed results in:\t' + json_file_wDir +'\n'
        print '*********************************************************************\n'
        '''
            #
        inversion_nodes = []
        results_object = []
        for result in Solution:
            tmp_results = dict()
            nodes_list = []
            for node_i in result[1]:
                profile_list = []
                tmp_node_dic = dict()
                for injection in node_i[1]:
                        #print injection
                    profile_list.append(dict(Start=injection[0], Stop=injection[1], Strength=injection[2]))
                tmp_node_dic['Name'] = node_i[0]
                tmp_node_dic['Profile'] = profile_list
                nodes_list.append(tmp_node_dic)
            inversion_nodes.append([i['Name'] for i in nodes_list])
            tmp_results['Objective'] = result[0]
            tmp_results['Nodes'] = nodes_list
            tmp_results['run date'] = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            results_object.append(tmp_results)
        f = open(json_file, 'w')
        json.dump(results_object, f,indent=2)
        f.close()
        # Print TSG and Likely Nodes File
        profile = open(tsg_filename, 'w')
        if self.getInversionOption('output impact nodes'):
            impact_nodes = open(impact_nodes_file, 'w')
        for result in Solution:
            for node in result[1]:
                        #print node
                if self.getInversionOption('output impact nodes'):
                    impact_nodes.write(node[0] + '\n')
                profile.write(node[0]+ '\tMASS\t' + str(node[1][0][2]) + '\t' + str(node[1][0][0]) + '\t' + str(node[1][0][1]) + '\n')
        profile.close()
        if self.getInversionOption('output impact nodes'):
            impact_nodes.close()

        objective_val = [1 for i in inversion_nodes[0]]
        return [json_file_wDir, tsg_filename, num_events, impact_nodes_file, objective_val, inversion_nodes[0]]        

    def writeAllowedNodesFile(self,filename,results_list,tao):
        fnodes=open(filename,'w')
        for result in results_list:
            if result[0]>=tao:
                for node in result[1]:
                    fnodes.write(node[0]+ '\n')
        fnodes.close()
    
    def writeVisualizationFile(self, Solution, tao, yml_file, html_filename, yml_filename):
        inp_file = os.path.abspath(self.opts['network']['epanet file'])
        #
        if self.getConfigureOption('output prefix') not in self.none_list:
            #output_prefix = os.path.join('vis', os.path.basename(self.opts['configure']['output prefix'])) 
            output_prefix = os.path.join(os.path.abspath(self.opts['configure']['output prefix'])) 
        else:
            output_prefix = ""
        #
        if inp_file and len(inp_file) > 0:
            svg = inp2svg.inp2svg(inp_file)
            #
            sensor_list = []
            source_list = []
            #
            if self.getMeasurementOption('grab samples') not in self.none_list:
                sensor_file_name = self.getMeasurementOption('grab samples')
                sensor_file = open(sensor_file_name, "r")
                sensors = sensor_file.readlines()
                for line in sensors:
                    line = line.strip()
                    if len(line) == 0: continue
                    if line[0] == "#" : continue
                    sensor = line.split()[0]
                    svg.addShapeOn("square", sensor, sc="#000099", sw=2, fo=0, fs=15)
                    sensor_list.append(sensor)
            nlen1 = len(Solution)
            bOpt = self.getInversionOption("algorithm") == "optimization"
            bCSA = self.getInversionOption("algorithm") in ["csa","CSA"]
            max_objective = 0
            if bOpt or bCSA:
                for i in range(0, nlen1):
                    objective = Solution[i][0]
                    max_objective = max(max_objective, objective)
                for i in range(0, nlen1):
                    objective = Solution[i][0]
                    if objective < tao: continue
                    scale = objective / max_objective * 15
                    if len(Solution[i][1]) == 0:
                        pass
                    else:
                        node_name = Solution[i][1][0][0]
                        svg.addShapeOn("circle", node_name, fc="#aa0000", sc="#bb0000", sw=1, so=1, fs=scale)
                        source_list.append({"objective": objective / max_objective, "name": node_name})
            else:
                for i in range(0, nlen1):
                    objective = Solution[i]["Objective"]
                    max_objective = max(max_objective, objective)
                for i in range(0, nlen1):
                    objective = Solution[i]["Objective"]
                    scale = objective / max_objective * 15
                    node_name = Solution[i]["Nodes"][0]["Name"]
                    svg.addShapeOn("circle", node_name, fc="#aa0000", sc="#bb0000", sw=1, so=1, fs=scale)
                    source_list.append({"objective": objective / max_objective, "name": node_name})
            #
            scale = 15 / max_objective if max_objective > 0 else 15
            svg.setWidth(800)
            svg.setNodeSize(3)
            svg.setLinkSize(1)
            #
            svg.addLayer("Measurement locations")
            svg.getLayer(0)["type"        ] = svg.LAYER_TYPE_NODE
            svg.getLayer(0)["shape"       ] = "Square"
            svg.getLayer(0)["fill color"  ] = "#000099"
            svg.getLayer(0)["fill opacity"] = 0
            svg.getLayer(0)["line size"   ] = 2
            svg.getLayer(0)["line color"  ] = "#000099"
            svg.getLayer(0)["line opacity"] = 0.6
            #
            svg.addLayer("Possible source locations")
            svg.getLayer(1)["type"        ] = svg.LAYER_TYPE_NODE
            svg.getLayer(1)["shape"       ] = "Circle"
            svg.getLayer(1)["fill color"  ] = "#aa0000" 
            svg.getLayer(1)["fill opacity"] = 0.6
            svg.getLayer(1)["line color"  ] = "#aa0000" 
            svg.getLayer(1)["line size"   ] = 1
            svg.getLayer(1)["line opacity"] = 0.8
            #
            svg.setLegendColor("white")
            svg.setBackgroundColor("white")
            svg.setLegendXY(10,10)
            svg.showLegend()
            #svg.writeFile(html_filename)
        else:
            logger.info('EPANet file input requried for Visualization.')
            inp_file = "<REQUIRED INPUT>"
        #
        f = open(yml_filename, "w")
        f.write("# YML input file for custom Inversion visualization\n")
        f.write("\n")
        #
        vis = {}
        vis["network"] = {}
        vis["network"]["epanet file"] = inp_file
        #
        vis["visualization"] = {}
        vis["visualization"]["nodes"] = {}
        vis["visualization"]["nodes"]["size"] = 3
        vis["visualization"]["links"] = {}
        vis["visualization"]["links"]["size"] = 1
        #
        vis["visualization"]["layers"] = []
        #
        vis["visualization"]["layers"].append({})
        vis["visualization"]["layers"][0]["label"       ] = "Measurement locations"
        vis["visualization"]["layers"][0]["shape"       ] = "Square"
        vis["visualization"]["layers"][0]["fill"] = {}
        vis["visualization"]["layers"][0]["fill"]["color"  ] = "#000099"
        vis["visualization"]["layers"][0]["fill"]["size"   ] = 15
        vis["visualization"]["layers"][0]["fill"]["opacity"] = 0
        vis["visualization"]["layers"][0]["line"] = {}
        vis["visualization"]["layers"][0]["line"]["color"  ] = "#000099"
        vis["visualization"]["layers"][0]["line"]["size"   ] = 2
        vis["visualization"]["layers"][0]["line"]["opacity"] = 0.6
        vis["visualization"]["layers"][0]["locations"   ] = []
        sensor_list = list(set(sensor_list))
        for sensor in sensor_list:
            vis["visualization"]["layers"][0]["locations"].append(sensor)
        #
        vis["visualization"]["layers"].append({})
        vis["visualization"]["layers"][1]["label"       ] = "Possible source locations"
        vis["visualization"]["layers"][1]["file"   ] = yml_file
        vis["visualization"]["layers"][1]["shape"       ] = "Circle"
        vis["visualization"]["layers"][1]["fill"] = {}
        vis["visualization"]["layers"][1]["fill"]["size"   ] = "['inversion']['node likeliness'][i] * " + str(scale)
        vis["visualization"]["layers"][1]["fill"]["color"  ] = "#aa0000"
        vis["visualization"]["layers"][1]["fill"]["opacity"] = 0.6
        vis["visualization"]["layers"][1]["line"] = {}
        vis["visualization"]["layers"][1]["line"]["color"  ] = "#aa0000"
        vis["visualization"]["layers"][1]["line"]["size"   ] = 1
        vis["visualization"]["layers"][1]["line"]["opacity"] = 0.8
        vis["visualization"]["layers"][1]["locations"   ] = "['inversion']['candidate nodes'][i]"
        #
        vis["configure"] = {}
        vis["configure"]["output prefix"] = output_prefix
        #
        yaml.dump(vis, f, default_flow_style=False)
        return

    # General Option SET functions
    def setNetworkOption(self, name, value):
        self.opts['network'][name] = value
        return

    def setMeasurementOption(self, name, value):
        self.opts['measurements'][name] = value
        return

    def setInversionOption(self, name, value):
        self.opts['inversion'][name] = value
        return

    def setConfigureOption(self, name, value):
        self.opts['configure'][name] = value
        return

    # General Option GET functions
    def getConfigureOption(self, name):
        return self.opts['configure'][name]

    def getInversionOption(self, name):
        return self.opts['inversion'][name]

    def getMeasurementOption(self, name):
        return self.opts['measurements'][name]

    def getNetworkOption(self, name):
        return self.opts['network'][name]

    def getSolverOption(self, name):
        return self.opts['solver'][name]

    def getInternalOption(self, name):
        return self.opts['internal'][name]
