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
from pyomo.environ import *
import imp

import pywst.common.problem
import pywst.common.wst_config as wst_config
from pyutilib.misc.config import ConfigBlock
import pywst.common.wst_util as wst_util
from pywst.common.signals import Simulator 
from pywst.common.signals import DataManager
from signals_sampler import SamplingLocation

import pywst.visualization.inp2svg as inp2svg

from pywst.uq.auxiliary.uq import (bayesian_update,
                                   compute_node_probabilities,
                                   get_earliest_meas_time,
                                   build_nodes_contamination_scenarios_sets)

logger = logging.getLogger('wst.grabsample')

try:
    import pyepanet
except ImportError:
    pass


class Problem(pywst.common.problem.Problem):
    filename = 'grabsample.yml'

    # Trying handle all possible ways we may encounter None coming from the yaml parser

    none_list = ['none','','None','NONE', None] 
    defLocs = {}
    epanetOkay = False

    def __init__(self):
        pywst.common.problem.Problem.__init__(self, 'grabsample', ("network", "scenario", "grabsample","measurements", "solver", "configure")) 
        self.loadPreferencesFile()
        self.validateEPANET()
        return

    def validateEPANET(self):
        """try:
            enData = pyepanet.ENepanet()
            enData.ENopen(self.opts['network']['epanet file'],'tmp.rpt')
            enData.ENclose()
        except:
            raise RuntimeError("EPANET inp file not loaded using pyepanet")

        return
        """

    def loadPreferencesFile(self):
        for key in self.defLocs.keys():
            if key == 'ampl':
                self.opts['configure']['ampl executable'] = defLocs[key]
            if key == 'pyomo':
                self.opts['configure']['pyomo executable'] = defLocs[key]
        return

    def runSamplelocation(self):
        logger = logging.getLogger('wst.grabsample.samplelocation')

        metric_file = self.getSampleLocationOption('nodes metric')
        if metric_file not in self.none_list:
            print "WARNING: weighted distinguishability not supported with tsg or tsi. Use signals"
        
        #print self.opts
        out_prefix = ''
        if self.getConfigureOption('output prefix') not in self.none_list:
            out_prefix += self.getConfigureOption('output prefix')+'_'
        #self.createInversionSimDat()
        cmd = 'samplelocation '
    
        is_inp_file = (self.getNetworkOption('epanet file') not in self.none_list)
        is_wqm_file = (self.getSampleLocationOption('wqm file') not in self.none_list)

        # Optional arguments are simulation duration and water quality timestep, which will
        # override what is in the EPANET input file.
        if is_inp_file and self.getNetworkOption('simulation duration') not in ['INP','Inp','inp']:
            cmd += '--simulation-duration-minutes='+str(self.getNetworkOption('simulation duration'))+' '
        if is_inp_file and self.getNetworkOption('water quality timestep') not in ['INP','Inp','inp']:
            cmd += '--quality-timestep-minutes='+str(self.getNetworkOption('water quality timestep'))+' '

        # substitude integers for node names in output files
        #cmd += '--output-merlion-labels '
        # the above command will produce the following file
        tmpdir = os.path.dirname(self.opts['configure']['output prefix'])
        prefix = os.path.basename(self.opts['configure']['output prefix'])

        # if merlion is being used substitude integers for node names in output files
        if self.getEventsOption('merlion'):
            cmd += '--merlion '
            cmd += '--output-merlion-labels '
            label_map_file = wst_util.declare_tempfile(os.path.join(tmpdir, prefix+"_MERLION_LABEL_MAP.txt"))
        else:
            cmd += '--output-epanet-labels '
            label_map_file = wst_util.declare_tempfile(os.path.join(tmpdir, prefix+"_EPANET_LABEL_MAP.txt"))


        # Prepend all output file names with this
        cmd += '--output-prefix='+out_prefix+' '
        
        # Dissable merlion warnings
        if  self.getEventsOption('ignore merlion warnings'):
            cmd += '--ignore-merlion-warnings '

        # Prepend fixed sensorsname
        logger.debug(self.getSampleLocationOption('fixed sensors'))
        if self.getSampleLocationOption('fixed sensors') not in self.none_list:
            try:
                enData = pyepanet.ENepanet()
                enData.ENopen(self.opts['network']['epanet file'],'tmp.rpt')
            except:
                raise RuntimeError("EPANET inp file not loaded using pyepanet")

            fixed_sensor_names, feasible_node_indices = wst_util.feasible_nodes(\
                                                                 self.getSampleLocationOption('fixed sensors'),\
                                                                 [], \
                                                                 True, enData)
            enData.ENclose()

            tmpdir = os.path.dirname(self.opts['configure']['output prefix'])
            tmpprefix = 'tmp_'+os.path.basename(self.opts['configure']['output prefix'])
            tmpSensorFile = pyutilib.services.TempfileManager.create_tempfile(prefix=tmpprefix, dir=tmpdir, suffix='.txt')
            # write nodes file
            fid = open(tmpSensorFile,'w')
            for n in fixed_sensor_names:
                fid.write(n + '\n')
            fid.close()
            
            cmd += '--fixed-sensors=' + tmpSensorFile + ' ' 
            self.opts['grabsample']['fixed sensors'] = tmpSensorFile

        # Allowed location filename
        if self.getSampleLocationOption('feasible nodes') not in self.none_list:
            try:
                enData = pyepanet.ENepanet()
                enData.ENopen(self.opts['network']['epanet file'],'tmp.rpt')
            except:
                raise RuntimeError("EPANET inp file not loaded using pyepanet")

            feasible_node_names, feasible_node_indices = wst_util.feasible_nodes(\
                                                                 self.getSampleLocationOption('feasible nodes'),\
                                                                 [], \
                                                                 True, enData)
            enData.ENclose()
            tmpdir = os.path.dirname(self.opts['configure']['output prefix'])
            tmpprefix = 'tmp_'+os.path.basename(self.opts['configure']['output prefix'])
            tmpNodesFile = pyutilib.services.TempfileManager.create_tempfile(prefix=tmpprefix, dir=tmpdir, suffix='.txt')
            # write nodes file
            fid = open(tmpNodesFile,'w')
            for n in feasible_node_names:
                fid.write(n + '\n')
            fid.close()

            cmd += '--allowed-nodes=' + tmpNodesFile + ' ' 
            self.opts['grabsample']['feasible nodes'] = tmpNodesFile

        #LOGIC VALIDATION
        is_tsg_file = (self.getEventsOption('tsg file') not in self.none_list)
        is_scn_file = (self.getEventsOption('scn file') not in self.none_list)
        assert(is_inp_file != is_wqm_file) # one and only one is true        
        assert(is_tsg_file != is_scn_file) # one and only one is true
        
        if self.getSampleLocationOption('model format') not in ['AMPL','PYOMO']:
            raise IOError("Invalid model format: "+self.getSampleLocationOption('model format'))
        
        #Check for the threshold
        if self.getSampleLocationOption('threshold') not in self.none_list:
            cmd += '--threshold=' + str(self.getSampleLocationOption('threshold')) + ' '
        
        #check for the number of samples
        if self.getSampleLocationOption('num samples') not in self.none_list:
            cmd += '--number-samples=' + str(self.getSampleLocationOption('num samples')) + ' '

        # Check for greedy selection algorithm     
        if self.getSampleLocationOption('greedy selection'):
            cmd += '--greedy-selection '

        # Check for greedy selection algorithm     
        if self.getSampleLocationOption('with weights'):
            cmd += '--with-weights '
            
        # The file defining the water quality model
        if is_inp_file:
            cmd += '--inp=' + self.getNetworkOption('epanet file') + ' '
        elif is_wqm_file:
            cmd += '--wqm=' + self.getSampleLocationOption('wqm file') + ' '
    
        if is_tsg_file:
            '''if len(open(self.getEventsOption('tsg file'),'r').readlines())<2:
                print '\nError: The events file must have more than one event.'
                exit(1)'''
            cmd += '--tsg=' + self.getEventsOption('tsg file') + ' '
        elif is_scn_file:
            '''if int(open(self.getEventsOption('scn file'),'r').readline())<2:
                print '\nError: The events file must have more than one event.'
                exit(1)'''                
            cmd += '--scn=' + self.getEventsOption('scn file') + ' '
    
        
        if self.getSampleLocationOption('sample time') in self.none_list:
            raise IOError("A sample time must be specified")
        else:
            cmd += str(self.getSampleLocationOption('sample time'))
            
        #print cmd
        logger.info("Launching samplelocation executable ...")
        logger.debug(cmd)
        out = pyutilib.services.TempfileManager.create_tempfile(dir=tmpdir, prefix='tmp_', suffix='samplelocation.out') 
        sim_timelimit = None
        sub_logger = logging.getLogger('wst.grabsample.samplelocation.exec')
        sub_logger.setLevel(logging.DEBUG)
        fh = logging.FileHandler(out, mode='w')
        sub_logger.addHandler(fh)
        p = pyutilib.subprocess.run(cmd,timelimit=sim_timelimit,stdout=pywst.common.problem.LoggingFile(sub_logger))
        if (p[0]):
            print '\nAn error occured when running the samplelocation executable'
            print 'Error Message: ', p[1]
            print 'Command: ', cmd, '\n'
            raise RuntimeError("An error occured when running the samplelocation executable")
                
        nodemap = {}
        if not self.getSampleLocationOption('greedy selection'):
            f = open(label_map_file,'r')
            for line in f:
                t = line.split()
                nodemap[t[1]] = t[0]
            f.close()
        return nodemap

    def run(self):
        logger.info("WST grabsample subcommand")
        logger.info("---------------------------")
        
        # set start time
        self.startTime = time.time()
        
        # validate input
        logger.info("Validating configuration file")
        self.validate()

        if self.getEventsOption('signals') not in self.none_list:
            files_folder = self.getEventsOption('signals')
            #TODO LIST SCENARIOS
            optimal_locations,objective = self.runSIGNALSanalysis(files_folder)
            # Convert node ID's to node names in Solution    
            Solution = list()
            Solution = [objective, optimal_locations]
        else:
            try:
                enData = pyepanet.ENepanet()
                enData.ENopen(self.opts['network']['epanet file'],'tmp.rpt')
                enData.ENclose()
            except:
                raise RuntimeError("EPANET inp file not loaded using pyepanet")

            # write tmp TSG file if ['scenario']['tsg file'] == none
            if self.opts['scenario']['tsi file'] in pywst.common.problem.none_list and self.opts['scenario']['tsg file'] in pywst.common.problem.none_list:
                tmpdir = os.path.dirname(self.opts['configure']['output prefix'])
                tmpprefix = 'tmp_'+os.path.basename(self.opts['configure']['output prefix'])
                tmpTSGFile = pyutilib.services.TempfileManager.create_tempfile(prefix=tmpprefix, dir=tmpdir, suffix='.tsg')
                # Check if scenario list contains one or less nodes.
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

            # Run samplelocation executable
            nodemap = self.runSamplelocation()
            Solution = ()
            greedy_solution = {}
            #
            if self.getConfigureOption('output prefix') not in self.none_list: 
                json_result_file = self.getConfigureOption('output prefix') + '_grabsample.json'
            else:
                json_result_file = 'grabsample.json'

            #
            # For greedy algorithm get solution directly from the json file
            if self.getSampleLocationOption('greedy selection'):
               data_from_results = open(json_result_file).read()
               greedy_solution = json.loads(data_from_results)
               Solution = (greedy_solution['objective'],[str(i['id']) for i in greedy_solution['Nodes']])
            #else run optimization algorithms 
            else:
                #Get fixed sensor list . Not to be included in final solution
                fixed_sensor_ID = []
                inv_nodemap = dict((v,k) for k,v in nodemap.iteritems())
                if self.getSampleLocationOption('fixed sensors') not in self.none_list:
                    sensors_filename = self.getSampleLocationOption('fixed sensors')
                    sensor_file = open(sensors_filename)
                    fixed_sensor_list = [line.strip() for line in sensor_file]
                    sensor_file.close()        

                    for node_name in fixed_sensor_list:
                        if len(node_name) > 0:
                            fixed_sensor_ID.append(inv_nodemap[node_name])

                solve_timelimit = None
                p = (1,"There was a problem with the 'model type' or 'model format' options")
                cmd = None

                not_allowed_nodes_set = set()
                """
                if self.getSampleLocationOption('not feasible nodes') not in self.none_list \
                   and len(open(self.getSampleLocationOption('not feasible nodes'),'r').readlines())!=0:
                    label_map_file = "_MERLION_LABEL_MAP.txt"
                    name_to_id={}
                    f = open(self.getConfigureOption('output prefix')+label_map_file,'r')
                    for line in f:
                        t = line.split()
                        name_to_id[t[0]] = t[1]
                    f.close()            
                    for line in open(self.getSampleLocationOption('not feasible nodes'),'r'):
                        l=line.split()
                        for n_ in l:
                            if name_to_id.has_key(n_)!=True:
                                print '\nERROR: Nodename ',n_,' specified in ',self.getSampleLocationOption('not feasible nodes')\
                                      ,' is not part of the network'
                                exit(1)
                            not_allowed_nodes_set.add(int(name_to_id[n_]))        
                """

                #run pyomo or ampl
                if self.getSampleLocationOption('model format') == 'AMPL':
                    exe = self.getConfigureOption('ampl executable')
                    if self.getConfigureOption('output prefix') not in self.none_list:
                        inp = self.getConfigureOption('output prefix')+'_ampl.run'
                        out = self.getConfigureOption('output prefix')+'_ampl.out'
                    else:
                        inp = 'ampl.run'
                        out = 'ampl.out'
                    results_file = self.createAMPLRun(inp, not_allowed_nodes_set)
                    print results_file
                    cmd = '%s %s'%(exe,inp)
                    logger.info("Launching AMPL ...")
                    sub_logger = logging.getLogger('wst.grabsample.models.ampl')
                    sub_logger.setLevel(logging.DEBUG)
                    fh = logging.FileHandler(out, mode='w')
                    sub_logger.addHandler(fh)
                    p = pyutilib.subprocess.run(cmd,timelimit=solve_timelimit,stdout=pywst.common.problem.LoggingFile(sub_logger))
                    if (p[0] or not os.path.isfile(results_file)):
                        message = 'An error occured when running the optimization problem.\n Error Message: '+ p[1]+'\n Command: '+cmd+'\n'
                        logger.error(message)
                        raise RuntimeError(message)
                    #try to load the results file
                    #print "results file: "+ results_file
                    filereader = open(results_file,'r')
                    line1=filereader.readline().split()
                    nodes=[]
                    for l in xrange(0,len(line1)):
                        if l>0 and line1[l] not in fixed_sensor_ID:
                            nodes.append(line1[l])
                    objective=float(filereader.readline().split()[1])
                    filereader.close()
                    Solution=(objective,nodes)
                    #print Solution

                elif self.getSampleLocationOption('model format') == 'PYOMO':
                    logger.info("Launching PYOMO ...")
                    Solution=self.runPYOMOmodel(fixed_sensor_ID, not_allowed_nodes_set)
             
            # Convert node ID's to node names in Solution    
            for i in xrange(0,len(Solution[1])):
                Solution[1][i] = nodemap[str(Solution[1][i])]


        # Get information to print results to the screen 
        if self.getEventsOption('scn file') not in self.none_list:
            events_filename=self.getEventsOption('scn file')
            scnfile = open(events_filename)
            content_scn = scnfile.read()
            scnfile.close()
            number_events = content_scn.count('scenario')
        elif self.getEventsOption('tsg file') not in self.none_list:
            events_filename=self.getEventsOption('tsg file')
            number_events=len([line for line in open(events_filename,'r')])
        elif self.getEventsOption('signals') not in self.none_list:
            events_filename = self.getEventsOption('signals')
            number_events= self.signal_scenarios

        # remove temporary files if debug = 0
        if self.opts['configure']['debug'] == 0:
            pyutilib.services.TempfileManager.clear_tempfiles()
                
        # write output file 
        prefix = os.path.basename(self.opts['configure']['output prefix'])
        logfilename = logger.parent.handlers[0].baseFilename
        outfilename = logger.parent.handlers[0].baseFilename.replace('.log','.yml')
        visymlfilename = logger.parent.handlers[0].baseFilename.replace('.log','_vis.yml')
        
        # Get node list from Solution
        node_list = Solution[1]

        #for i in xrange(0,len(Solution[1])):
        #    if self.getSampleLocationOption('greedy selection'):
        #        node_list.append(Solution[1][i])
        #    else:
        #        node_list.append(nodemap[str(Solution[1][i])])

        sample_time =  self.getSampleLocationOption('sample time')
        N_samples =  self.getSampleLocationOption('num samples')
        threshold =  self.getSampleLocationOption('threshold')
        
        # Write output yml file
        config = wst_config.output_config()
        module_blocks = ("general", "grabsample")
        template_options = {
            'general':{
                'cpu time': round(time.time() - self.startTime,3),
                'directory': os.path.dirname(logfilename),
                'log file': os.path.basename(logfilename)},
            'grabsample': {
                'nodes': node_list,
                'objective': Solution[0],
                'threshold': threshold,
                'count': N_samples,
                'time': sample_time} }
        if outfilename != None:
            self.saveOutput(outfilename, config, module_blocks, template_options)
        
        # Write output visualization yml file
        config = wst_config.master_config()
        module_blocks = ("network", "visualization", "configure")
        template_options = {
            'network':{
                'epanet file': os.path.abspath(self.opts['network']['epanet file'])},
            'visualization': {
                'layers': [{
                    'label': 'Optimal sample locations', 
                    'locations': "['grabsample']['nodes'][i]",
                    'file': outfilename,
                    'location type': 'node',  
                    'shape': 'circle',             
                    'fill': {
                        'color': 'blue',
                        'size': 15}}]},
            'configure': {
                'output prefix': os.path.abspath(self.opts['configure']['output prefix'])}} #os.path.join('vis', os.path.basename(self.opts['configure']['output prefix']))}}
        if visymlfilename != None:
            self.saveVisOutput(visymlfilename, config, module_blocks, template_options)        
        
        # Run visualization
        cmd = ['wst', 'visualization', visymlfilename]
        p = pyutilib.subprocess.run(cmd) # logging information should not be printed to the screen
        
        # print solution to screen
        logger.info("\nWST normal termination")
        logger.info("---------------------------")
        logger.info("Directory: "+os.path.dirname(logfilename))
        logger.info("Results file: "+os.path.basename(outfilename))
        logger.info("Log file: "+os.path.basename(logfilename))
        logger.info("Visualization configuration file: "+os.path.basename(visymlfilename)+'\n')
        
        return Solution

    def validate(self):
        output_prefix = self.getConfigureOption('output prefix')
        self.validateExecutables()

        if output_prefix == '':
            output_prefix = 'gsample'
            self.setConfigureOption('output prefix',output_prefix)

        return

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
    
    def createAMPLRun(self,filename=None,not_allowed_list=set([])):
        if filename is None: 
            filename = self.getConfigureOption('output prefix')+'_ampl.run'
        ampl_model = os.path.join(os.path.dirname(os.path.abspath(__file__)),'models','ampl','gs_model.mod')

        data_file_prefix = ''
        if self.getConfigureOption('output prefix') not in self.none_list:
            data_file_prefix += self.getConfigureOption('output prefix')+'_'

        fid = open(filename,'wt')
        fid.write('option presolve 0;\n')
        fid.write('option substout 0;\n')
        fid.write('\n')

        fid.write('# Grab sample optimization model\n')
        fid.write('model %s;\n'%ampl_model)
        fid.write('\n')

        wst_util.declare_tempfile(data_file_prefix+'GSP.dat')
        
        fid.write('# Grab sample data\n')
        fid.write('data '+data_file_prefix+'GSP.dat\n')
        fid.write('\n')

        fid.write('# Solve the problem\n')
        #fid.write('option solver cplexamp;\n')
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
                for (key,val) in self.getSolverOption('options').iteritems():
                    if val in self.none_list:
                        # this is the case where an option does not accept a value
                        fid.write(key+' ')
                    else:
                        fid.write(key+'='+str(val)+' ')
                fid.write('\';\n')
            else:
                print >> sys.stderr, ' '
                print >> sys.stderr, "WARNING: Solver options in AMPL are currently not handled for"
                print >> sys.stderr, "         the specified solver: ", self.getSolverOption('type')
                print >> sys.stderr, "         All solver options will be ignored."
                print >> sys.stderr, ' '

        if len(not_allowed_list)>0:
            fid.write('\n') 
            fid.write('set S_NOT_ALLOWED_NODES;\n')
            fid.write('let S_NOT_ALLOWED_NODES :={')
            i=0
            for node in not_allowed_list:
                fid.write(str(node))
                if(i<len(not_allowed_list)-1):
                    fid.write(',')
                i+=1
            fid.write('};\n')
            fid.write('\n')
            fid.write('fix{n in S_NOT_ALLOWED_NODES}s[n]:=0;\n\n')

        results_file = data_file_prefix+'grabsample.dat'
        fid.write('solve;\n')
        fid.write('printf "Nodes:" >'+results_file+';\n')
        fid.write('for{i in S_LOCATIONS: s[i] !=0}\n')
        fid.write('{\n')
        fid.write('\t printf "\t%s", i >>' + results_file + ';\n')
        fid.write('}\n')
        fid.write('printf "\\n" >>' + results_file + ';\n')
        fid.write('printf "objective:" >>' + results_file + ';\n')
        fid.write('printf "\t%q\\n",OBJECTIVE >>' + results_file + ';\n')
        fid.close()

        return results_file

    def runSIGNALSanalysis(self,scenarios_file):
        
        folder_name = scenarios_file
        # simulates if necessary
        if os.path.isfile(scenarios_file):
            simulator = Simulator(scenarios_file,"SCENARIO_SIGNALS")
            threshold = self.getSampleLocationOption('threshold')
            simulator.run_simulations(threshold)
            folder_name = os.path.abspath("SCENARIO_SIGNALS") 
        
        # define time horizon to load from simulated data
        meas_file = self.getMeasurementsOption('grab samples')
        load_start_time = self.getSampleLocationOption('sample time')
        load_end_time = int(self.getSampleLocationOption('sample time'))
        if meas_file not in self.none_list: 
            first_meas_time = get_earliest_meas_time(meas_file)
            if first_meas_time>=0 and first_meas_time<load_start_time:
                load_start_time = first_meas_time
        
        # object for manage signals files
        # only loads from signals data needed
        data_manager = DataManager(folder_name,
                                   report_start_time=load_start_time,
                                   report_end_time=load_end_time)
        

        # check time step agrees with time step of scenarios
        if self.getSampleLocationOption('sample time') in self.none_list:
            raise IOError("A sample time must be specified")
        
        time_to_sample = self.getSampleLocationOption('sample time')
        time_step = data_manager.time_step_min
        if time_to_sample%time_step!=0:
            raise RuntimeError('Sample time {} is not multiple of scenarios time step {}'.format(time_to_sample,time_step))

        # read measurements
        if meas_file:
            measurements = data_manager.read_measurement_file(meas_file)
        else:
            measurements = dict()
            measurements['positive'] = dict()
            measurements['negative'] = dict()

        # define scenarios
        file_scenario_ids = self.getSampleLocationOption('list scenario ids')
        if file_scenario_ids in self.none_list:
            filter_scenarios = self.getSampleLocationOption('filter scenarios')
            if filter_scenarios:
                list_scenarios_ids = data_manager.list_scenario_ids(measurements=measurements)
            else:
                list_scenarios_ids = data_manager.list_scenario_ids()
        else:
            list_scenarios_ids = data_manager.read_list_ids_file(file_scenario_ids)
            
        print "Loading signals files ..."
        hid_to_cids = dict(list_scenarios_ids)
        list_scenarios = data_manager.read_signals_files(hid_to_cids)

        metric_file = self.getSampleLocationOption('nodes metric')
        if metric_file not in self.none_list:
            data_manager.read_node_to_metric_map(metric_file)

        self.signal_scenarios = len(list_scenarios)
        # locations to consider for sampling
        locations = data_manager.idx_to_node_name.keys()
        n_samples =  int(self.getSampleLocationOption('num samples'))

        if self.getSampleLocationOption('fixed sensors') not in self.none_list:
            raise RuntimeError('grabsample with signals does not support fixed sensors option')

        if self.getSampleLocationOption('feasible nodes') not in self.none_list:
            raise RuntimeError('grabsample with signals does not support feasible nodes option')

        # object for determining sampling locations
        sample_locator = SamplingLocation(list_scenarios,data_manager)
        mip_solver = self.getSolverOption('type')
        
        print "Solving optimization problem ..."

        sample_criteria = self.getSampleLocationOption('sample criteria')
        if sample_criteria in self.none_list:
            sample_criteria = 'distinguish'
            
        if sample_criteria == 'random':
            opt_locations, objective = sample_locator.random_locations(locations,
                                                                       time_to_sample,
                                                                       n_samples)

            optimal_locations = [data_manager.idx_to_node_name[nid] for nid,t in opt_locations]
            return optimal_locations, objective
        elif sample_criteria == 'distinguish':
            greedy = self.getSampleLocationOption('greedy selection')
            if not greedy:
                opt_locations, objective = sample_locator.run_distinguishability_model(locations,
                                                                                time_to_sample,
                                                                                n_samples,
                                                                                solver=mip_solver)
            else:
                opt_locations, objective = sample_locator.greedySelection(locations,
                                                                   time_to_sample,
                                                                   n_samples)

            optimal_locations = [data_manager.idx_to_node_name[nid] for nid,t in opt_locations]
            return optimal_locations, objective
        
        elif sample_criteria == 'probability1' or sample_criteria == 'probability2':            
            if meas_file in self.none_list:
                print """WARNING: all scenarios are considered equally likely. 
                Providing measurements will adjust probabilities
                for better performance of the sampling probability formulations"""

            # update probabilities of scenarios and nodes
            pmf = 0.05
            scenario_probabilities = bayesian_update(list_scenarios,measurements, pmf)
            times = [time_to_sample]
            node_to_sn_set = build_nodes_contamination_scenarios_sets(list_scenarios,locations,times)
            node_probabilities = compute_node_probabilities(scenario_probabilities,node_to_sn_set)

            opt_locations, objective = sample_locator.run_probability_model(locations,
                                                                     node_probabilities,
                                                                     time_to_sample,
                                                                     n_samples,
                                                                     solver=mip_solver,
                                                                     model=sample_criteria)
            optimal_locations = [data_manager.idx_to_node_name[nid] for nid,t in opt_locations]
            return optimal_locations, objective
        else:
            raise RuntimeError("Wrong sample criteria. Choose between random, distinguish, probability1 and probability2")

        """
        # Check for greedy selection algorithm     
        if self.getSampleLocationOption('greedy selection'):
            opt_locations,objective = sample_locator.greedySelection(locations,time_to_sample,n_samples)
        else:
            logger.info("Launching PYOMO ...")
            pyomo_module = os.path.join(os.path.dirname(os.path.abspath(__file__)),'models','pyomo','gs_model')
            data_file_prefix = ''
            if self.getConfigureOption('output prefix') not in self.none_list:
                data_file_prefix += self.getConfigureOption('output prefix')+'_'
        
            pm = imp.load_source(os.path.basename(pyomo_module),pyomo_module+".py")
            model = pm.model	
            
            data=data_file_prefix+'GSP.dat'
            sample_locator.write_gs_datafile(data,
                                             locations,
                                             time_to_sample,
                                             n_samples)
            #TODO: add fixed sensors
            fixed_sensor_id = []
            solution = self.runPYOMOmodel(fixed_sensor_id)
            objective = solution[0]
            opt_locations = [int(nid) for nid in solution[1]]

        optimal_locations = [data_manager.idx_to_node_name[nid] for nid in opt_locations]
        return optimal_locations, objective
        """
        
    def runPYOMOmodel(self,fixed_sensor_ID, not_allowed_list=set([])):

        pyomo_module = os.path.join(os.path.dirname(os.path.abspath(__file__)),'models','pyomo','gs_model')
        data_file_prefix = ''
        if self.getConfigureOption('output prefix') not in self.none_list:
            data_file_prefix += self.getConfigureOption('output prefix')+'_'
        
        pm = imp.load_source(os.path.basename(pyomo_module),pyomo_module+".py")
        model = pm.model	

        data=data_file_prefix+'GSP.dat'
        #with open(data,'r') as f:
        #    for line in f:
        #        print line
        wst_util.declare_tempfile(data_file_prefix+'GSP.dat')

        modeldata=DataPortal()
        modeldata.load(model=model, filename=data)
        GSmod=model.create_instance(modeldata)

        #GSmod.pprint()
        if len(not_allowed_list)!=0:
            for n in not_allowed_list:
                GSmod.s[n].fixed=True
                GSmod.s[n].value=0
                
        #solver_option=SolverFactory("cplex")
        #solver_option=SolverFactory("glpk")
        solver_option=SolverFactory(self.getSolverOption('type'))
        if self.getSolverOption('options') not in self.none_list:
            for (key,val) in self.getSolverOption('options').iteritems():
                if val in self.none_list:
                    # this is the case where an option does not accept a value
                    solver_option.options[key] = ''
                else:
                    solver_option.options[key] = val

        GSmod.preprocess()
        results = solver_option.solve(GSmod)
        #GSmod.load(results)
        
        nodes=[]
        for location in GSmod.S_LOCATIONS:
            if(float(value(GSmod.s[location]))!=0 and str(location) not in fixed_sensor_ID):
                nodes.append(str(location))
        solution=(float(value(GSmod.OBECTIVE)),nodes)

        return solution
    
    def writeJsonResults(self, nodemap, Solution):
        output_prefix = self.getConfigureOption('output prefix')
        file_name = output_prefix + '_grabsample.json'

        sample_time =  self.getSampleLocationOption('sample time')
        N_samples =  self.getSampleLocationOption('num samples')
        threshold =  self.getSampleLocationOption('threshold')
        node_list = []
        for i in xrange(0,len(Solution[1])):
            node_list.append(nodemap[str(Solution[1][i])])
        results_object = {'sampleTime': sample_time,
                          'sampleCount': N_samples,
                          'threshold': threshold,
                          'objective': Solution[0],
                          'nodes': node_list,
                          'CPU time': time.time() - self.startTime}

        f = open(file_name,'w')
        json.dump(results_object,f,indent=2)
        f.close()
        
        return results_object   

    # General Option SET functions
    def setNetworkOption(self, name, value):
        self.opts['network'][name] = value
        return

    def setEventsOption(self, name, value):
        self.opts['events'][name] = value
        return

    def setsampleLocationOption(self, name, value):
        self.opts['samplelocation'][name] = value
        return

    def setConfigureOption(self, name, value):
        if name == 'output prefix' and value != '':
            output_prefix = os.path.splitext(os.path.split(value)[1])[0]
            value = output_prefix
        self.opts['configure'][name] = value
        return

    # General Option GET functions
    def getConfigureOption(self, name):
        return self.opts['configure'][name]

    def getSampleLocationOption(self, name):
        return self.opts['grabsample'][name]

    def getEventsOption(self, name):
        return self.opts['scenario'][name]

    def getNetworkOption(self, name):
        return self.opts['network'][name]

    def getSolverOption(self, name):
        return self.opts['solver'][name]

    def getCplexOption(self, name):
        return self.opts['solver']['cplex'][name]

    def getMeasurementsOption(self, name):
        return self.opts['measurements'][name]
    #def getInternalOption(self, name):
        #return self.opts['internal'][name]    
