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
import pywst.common.wst_config as wst_config
from pyutilib.misc.config import ConfigBlock
import pywst.common.wst_util as wst_util

from pywst.common.signals import Simulator
from pywst.common.signals import DataManager
import pywst.common.signals_output as s_out
from pywst.common.mpi_util import *
from auxiliary.uq import (bayesian_update,
                          update_scenario_matches,
                          build_nodes_contamination_scenarios_sets,
                          compute_node_probabilities)

logger = logging.getLogger('wst.uq')
EMPTY_LINE = ['\n', '\t\n', ' \n', '']

try:
    import pyepanet
except ImportError:
    pass

class Problem(pywst.common.problem.Problem):
    filename = 'uq.yml'

    # Trying handle all possible ways we may encounter None coming from the yaml parser

    none_list = ['none','','None','NONE', None] 
    defLocs = {}
    epanetOkay = False

    def __init__(self):
        pywst.common.problem.Problem.__init__(self, 'uq', ("scenario","uq","measurements", "configure")) 
        self.loadPreferencesFile()
        self.validateEPANET()
        self._run_simulations = False
        self._parallel = False
        return

    def validateEPANET(self):
        """try:
            enData = pyepanet.ENepanet()
            enData.ENopen(self.opts['network']['epanet file'],'tmp.rpt')
            enData.ENclose()
        except:
            raise RuntimeError("EPANET inp file not loaded using pyepanet")

        """
        return

    def loadPreferencesFile(self):
        for key in self.defLocs.keys():
            if key == 'pyomo':
                self.opts['configure']['pyomo executable'] = defLocs[key]
        return

    def runAnalysis(self):
        logger = logging.getLogger('wst.uq.analysis')
        
        #print self.opts
        out_prefix = ''
        if self.getConfigureOption('output prefix') not in self.none_list:
            out_prefix += self.getConfigureOption('output prefix')+'_'
        
        #logger.info("Launching samplelocation executable ...")
        #logger.debug(cmd)
        
        return 
    
    def run(self):
        if rank==0:
            logger.info("WST uq subcommand")
            logger.info("---------------------------")
        
        # set start time
        self.startTime = time.time()
        
        # validate input
        # Parse options and setup output directories
        ##############################################################################
        if rank == 0:
            logger.info("Validating configuration file")
        self.validate()
        
        threshold = self.getUQOption('threshold')
        parallel = 1 if size>1 and found_mpi else 0
        output_dir = self.getConfigureOption('output directory')

        # run simulations if required
        ##############################################################################
        if self._run_simulations:
            scenarios_file = self.getEventsOption('signals')
            simulator = Simulator(scenarios_file,output_dir)
            simulator.run_simulations(threshold,
                                      parallel=parallel)
            files_folder = output_dir
        else:
            files_folder = self.getEventsOption('signals')

        if parallel:
            comm.barrier() # all to wait until sims are done
            
        # Done simulating now reading signal files
        ##############################################################################
        
        meas_file = self.getMeasurementsOption('grab samples')
        load_start_time = self.getUQOption('analysis time')
        load_end_time = int(self.getUQOption('analysis time'))
        if meas_file not in self.none_list: 
            first_meas_time = get_earliest_meas_time(meas_file)
            if first_meas_time>=0 and first_meas_time<load_start_time:
                load_start_time = first_meas_time
        
        self.data_manager = DataManager(files_folder,
                                        report_start_time=load_start_time,
                                        report_end_time=load_end_time)
        
        meas_filename = self.getMeasurementsOption('grab samples')
        if meas_filename:
            measurements = self.data_manager.read_measurement_file(meas_filename)
        else:
            measurements = dict()
            measurements['positive'] = dict()
            measurements['negative'] = dict()

        filter_scenarios = self.getUQOption('filter scenarios')

        if filter_scenarios:
            list_scenarios_ids = self.data_manager.list_scenario_ids(measurements=measurements,
                                                                      parallel=parallel)
        else:
            list_scenarios_ids = self.data_manager.list_scenario_ids(parallel=parallel)

        hid_to_cids = dict(list_scenarios_ids)
	list_scenarios = self.data_manager.read_signals_files(hid_to_cids,parallel=parallel)
        
        # Ready for analysis of data
        ##############################################################################
        
        if rank == 0:
            time_to_sample = self.getUQOption('analysis time')
            time_step = self.data_manager.time_step_min
            if time_to_sample%time_step!=0:
                raise RuntimeError('Sample time {} is not multiple of time step {}'.format(time_to_sample,time_step))
            
            sample_locations = self.data_manager.idx_to_node_name.keys()
            all_nodes = self.data_manager.idx_to_node_name.keys()
            
            confidence = self.getUQOption('confidence')
            pmf = self.getUQOption('measurement failure')
            
            # scenario probilities
            scenario_probabilities = bayesian_update(list_scenarios,measurements, pmf)
            update_scenario_matches(list_scenarios,measurements)
            
            # node probabilities
            times = [time_to_sample]
            node_to_sn_set = build_nodes_contamination_scenarios_sets(list_scenarios,all_nodes,times)
            node_probabilities = compute_node_probabilities(scenario_probabilities,node_to_sn_set)

            #s_out.print_scenarios_probability(list_scenarios,self.data_manager,limit=40)            
            s_out.print_node_probabilities(node_probabilities,self.data_manager,confidence,detailed=True)
            #print_impact_matrix(list_scenarios,self.data_manager,time_to_sample)
            
            filename = self.getConfigureOption('output prefix') + '_uq_scenarios.yml'
            self.save_scenario_results(filename,list_scenarios)
            filename = self.getConfigureOption('output prefix') + '_uq_nodes.yml'
            self.save_node_results(filename,node_probabilities,confidence)
            
        
        # write output file 
        prefix = os.path.basename(self.opts['configure']['output prefix'])
        logfilename = logger.parent.handlers[0].baseFilename
        outfilename = logger.parent.handlers[0].baseFilename.replace('.log','.json')
    
        self.runAnalysis()

        # print solution to screen
        if rank==0:
            logger.info("\nWST normal termination")
            logger.info("---------------------------")
            logger.info("Directory: "+os.path.dirname(logfilename))
            logger.info("Results file: "+os.path.basename(self.getConfigureOption('output prefix') + '_nodes.yml'))
            logger.info("Log file: "+os.path.basename(logfilename))
            
        return 

    def validate(self):
        output_prefix = self.getConfigureOption('output prefix')
        # validate scenarios
        #scenarios = self.getUQOption('scenarios')
        scenarios = self.getEventsOption('signals')
        if scenarios in self.none_list:
            raise RuntimeError('Set the scenarios option in the configuration file')
        
        abs_scenarios = os.path.abspath(scenarios)
        if os.path.isdir(abs_scenarios):
            self._run_simulations = False

        elif os.path.isfile(abs_scenarios):
            self.validate_list_scenarios_file(abs_scenarios)
            output_dir = self.getConfigureOption('output directory')
            self._run_simulations = True
        else:
            raise RuntimeError('scenarios should be either a folder with the scenario realization or a file with list of scenarios to simulate')
        
        if output_prefix == '':
            output_prefix = 's'
            self.setConfigureOption('output prefix',output_prefix)
    
        return

    def validate_time_step_scenario(self,filename):
        with open(filename,'r') as f:
            header = f.readline()
            l = header.split()
            time_step = int(l[1])
            sample_time = self.getUQOption('sample time')
            if sample_time%time_step!=0:
                raise RuntimeError('Sample time {} is not multiple of time step {}'.format(sample_time,time_step))
            
    def validate_list_scenarios_file(self,filename):
        f = open(filename,'r')
        try:
            for counter,line in enumerate(f):
                if '#' not in line:
                    if line not in EMPTY_LINE:
                        l =line.split()
                        if len(l)!=3:
                            raise RuntimeError('Error in line {} of list_scenarios file'.format(counter))
                        for j in xrange(1,3):
                            name = l[j]
                            if not os.path.exists(name):
                                raise RuntimeError('Error in line {} of list_scenarios file. File {} cannot be found'.format(counter,name))
        except Exception, e:
            raise
        else:
            pass
        finally:
            f.close()

    def save_scenario_results(self,filename,scenarios_container):
        scenarios = {}
        for scenario in scenarios_container:
            key = '({},{})'.format(scenario.hid,scenario.cid)
            scenarios[key] = scenario.probability

        #with open(filename, 'w') as f:
        #    json.dump(scenarios, f)

        with open(filename, 'w') as outfile:
            yaml.dump(scenarios, outfile)

    def save_node_results(self,filename,node_probabilities,confidence):
        color_nodes = dict()
        color_nodes['nodes'] = {}
	color_nodes['nodes']['green'] = dict()
	color_nodes['nodes']['yellow'] = dict()
	color_nodes['nodes']['red'] = dict()
        color_nodes['confidence'] = confidence
        lower_tail = (1-confidence)*0.5
        upper_tail = 1-lower_tail
        for i,p in enumerate(node_probabilities):
            if i>0:
                node_name = self.data_manager.idx_to_node_name[i]
                if p>=upper_tail:
                    color_nodes['nodes']['red'][node_name] = p
                    
                elif p<=lower_tail:
                    color_nodes['nodes']['green'][node_name] = p
                else:
                    color_nodes['nodes']['yellow'][node_name] = p

        with open(filename, 'w') as outfile:
                yaml.dump(color_nodes, outfile)
        #with open(filename, 'w') as f:
        #    json.dump(color_nodes, f)
        
    # General Option SET functions
    def setNetworkOption(self, name, value):
        self.opts['network'][name] = value
        return

    def setEventsOption(self, name, value):
        self.opts['events'][name] = value
        return

    def setUQOption(self, name, value):
        self.opts['uq'][name] = value
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

    def getUQOption(self, name):
        return self.opts['uq'][name]

    def getEventsOption(self, name):
        return self.opts['scenario'][name]

    def getNetworkOption(self, name):
        return self.opts['network'][name]

    def getSolverOption(self, name):
        return self.opts['solver'][name]

    def getMeasurementsOption(self, name):
        return self.opts['measurements'][name]
        
