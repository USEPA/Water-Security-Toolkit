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
import os, sys
import pyutilib.subprocess
import yaml
import time
import logging

from string import lower

import pywst.common.problem
import pywst.common.wst_util as wst_util
import pywst.common.wst_config as wst_config
from pyutilib.misc.config import ConfigBlock

import pywst.visualization.inp2svg as inp2svg

logger = logging.getLogger('wst.sp')

none_list = wst_config.none_list

class Problem(pywst.common.problem.Problem):

    def __init__(self):
        pywst.common.problem.Problem.__init__(self, 'sp', ('impact data', 'cost', 'objective', 'constraint', 'aggregate', 'imperfect', 'sensor placement', 'solver', 'configure', ))
        self.filename = 'sp.yml'
        #
        self.opts = wst_config.master_config()
        #
        self.cost_goals = []
        self.impact_goals = []
        self.other_goals = ['ns', 'nfd']
        self.valid_statistics = [ "mean", "median", "var", "tce", "cvar", "total", "worst" ]
        #
        self.defLocs = {}
        self.epanetOkay = False
        #
        self.loadPreferencesFile()
        self.junction_map = []
        self.cost_map = []
        self.invalid_vals = set()
        self.fixed_vals = set()
        # currently, sp handles only one nodemap, cost, and gamma
        self.single_nodemap_file = None
        self.single_cost_file = None
        self.single_gamma = None
        self.none_list = wst_config.none_list
    
    def _loadPreferences(self):
        for key in defLocs.keys():
            if key == 'debug':
                self.opts['configure']['debug'] = defLocs[key]
            if key == 'temp directory':
                self.opts['configure']['temp directory'] = defLocs[key]
            if key == 'path':
                self.opts['configure']['path'] = defLocs[key]
            if key == 'version':
                self.opts['configure']['version'] = defLocs[key]
            if key == 'executable':
                for subkey in defLocs[key].keys():
                    if subkey == 'sp':
                        self.opts['configure']['sp executable'] = defLocs[key][subkey]
        
    def _load(self, filename):
        if filename is None: return False
        for key in self.opts.keys():
            if type(self.opts[key]).__name__=='dict':
                self.opts[key] = [self.opts[key]] # convert to list
        
        try:
            fid = open(filename,'r')
            options = yaml.load(fid)
            for key in options.keys():
                if type(options[key]).__name__=='dict':
                    options[key] = [options[key]] # convert to list
            for key in options.keys():
                for i in range(len(options[key])):
                    # append new list for key
                    if i > 0:
                        self.opts[key].append({})
                        for subkey in self.opts[key][0].keys():
                            self.opts[key][i].setdefault(subkey,'None')
                    suboptions = options[key][i]
                    for subkey in suboptions.keys():
                        # map options to self.opts
                        if (key == 'configure' and subkey in ['solver', 'sensor placement']) \
                                or (key == 'sensor placement' and subkey in ['objective','constraint', 'aggregate','imperfect']) \
                                or (key == 'constraint' and subkey in ['scenario']):
                            if type(options[key][i][subkey]).__name__=='str':
                                options[key][i][subkey] = [options[key][i][subkey]]
                        self.opts[key][i][subkey] = options[key][i][subkey]
                                
        except Exception, err:
            logger.error('Error reading "%s": %s' % (filename, str(err)))
            return False
        finally:
            fid.close()
        return True 
        
    def validate(self):
        # CONFIGURE
        if self.getConfigureOption('output prefix') in none_list:
            self.setConfigureOption('output prefix', 'wst_sp')  
            
        # TODO check all problem types for adaquate data
        #if self.getProblemOption('type') == 'P-MEDIAN':
        #       
        #elif self.getProblemOption('type') == '...':
        
        # IMPACT
        for i in range(len(self.opts['impact data'])):
            if self.getImpactOption('impact file',i) not in none_list:
                #if self.getImpactOption('directory',i) not in none_list:
                #    impact_file = os.path.join(self.getImpactOption('directory',i),self.getImpactOption('impact file',i))
                #    nodemap_file = os.path.join(self.getImpactOption('directory',i),self.getImpactOption('nodemap file',i))
                #else:
                impact_file = self.getImpactOption('impact file',i)
                nodemap_file = self.getImpactOption('nodemap file',i)
                
                # impact file
                if not os.path.exists(impact_file):
                    raise RuntimeError, "Impact file does not exist: "+impact_file
                else: # check number of impact columns and response time in the impact file
                    fid = open(impact_file, 'r')
                    while True:
                        impactLine1 = fid.readline()
                        if impactLine1.rstrip(): # skip blank line
                            if impactLine1.split()[0] is not '#': # skip comment lines
                                break
                    impactLine2 = fid.readline()
                    impactLine3 = fid.readline()
                    fid.close()
                    if impactLine2.split()[0] is not '1':
                        raise RuntimeError, "The impact file header indicates more than one response time.  wst sp currently supports single response time impact files"
                    elif len(impactLine2.split()) > 4:
                        raise RuntimeError, "The impact file contains more than four columns.  wst sp currently supports single response time impact files"
                    ## make sure that the response time equals value in the impact file
                    #elif str(self.getImpactOption('response time',i)) not in none_list:
                    #    if str(self.getImpactOption('response time',i)) is not impactLine2.split()[1]:
                    #        print "Warning: Response time changed to "+impactLine2.split()[1]+""
                    #self.setImpactOption('response time', i, impactLine2.split()[1])
                    
            else:
                raise RuntimeError, "Impact file required."
            
            # weight file
            if self.getImpactOption('weight file',i) not in none_list:
                #if self.getImpactOption('directory',i) not in none_list:
                #    weight_file = os.path.join(self.getImpactOption('directory',i),self.getImpactOption('weight file',i))
                #else:
                weight_file = self.getImpactOption('weight file',i)
                if not os.path.exists(weight_file):
                    raise RuntimeError, "Weight file does not exist"
            
            # add name to impact_goals list
            if not self.getImpactOption('name',i) in none_list:
                self.impact_goals.append(lower(self.getImpactOption('name',i)))
        
        # COST
        for i in range(len(self.opts['cost'])):
            if self.getCostOption('cost file', i) not in none_list:
                #if self.getCostOption('directory', i) not in none_list:
                #    cost_file = os.path.join(self.getCostOption('directory', i),self.getCostOption('cost file', i))
                #else:
                cost_file = self.getCostOption('cost file', i)
                if not os.path.exists(cost_file):
                    raise RuntimeError, "Cost file does not exist"
                
            # add name to cost_goals list
            if self.getCostOption('name',i) not in none_list:
                self.cost_goals.append(lower(self.getCostOption('name',i)))
        
        # OBJECTIVE
        for i in range(len(self.opts['objective'])):
            if lower(self.getObjectiveOption('goal', i)) not in self.impact_goals + self.cost_goals + self.other_goals:
                raise RuntimeError, "Invalid objective goal '%s'.  Valid goals = impact name, cost name, NS, NFD" % lower(self.getObjectiveOption('goal', i))
            if lower(self.getObjectiveOption('statistic', i)) not in self.valid_statistics:
                raise RuntimeError, "Invalid objective statistic.  Valid statistics = "+ ", ".join(self.valid_statistics)
            if lower(self.getObjectiveOption('statistic', i)) in ['var', 'cvar']:
                if self.getObjectiveOption('gamma',i) in none_list:
                    raise RuntimeError, "Gamma required for " + self.getObjectiveOption('statistic', i) +". gamma = 0.05 recommended"
            if lower(self.getObjectiveOption('goal', i)) in self.cost_goals + self.other_goals:
                if lower(self.getObjectiveOption('statistic', i)) not in ['total']:
                    raise RuntimeError, "Objective statistic must equal TOTAL when goal is " + self.getObjectiveOption('goal', i) +"."

        # CONSTRAINT
        for i in range(len(self.opts['constraint'])):
            if lower(self.getConstraintOption('goal', i)) not in self.impact_goals + self.cost_goals + self.other_goals:
                raise RuntimeError, "Invalid constraint goal.  Valid goals = impact name, cost name, NS, NFD"
            if lower(self.getConstraintOption('statistic', i)) not in self.valid_statistics:
                raise RuntimeError, "Invalid constraint statistic.  Valid statistics = " + ", ".join(self.valid_statistics)
            if lower(self.getConstraintOption('statistic', i)) in ['var', 'cvar']:
                if self.getConstraintOption('gamma', i) in none_list:
                    raise RuntimeError, "Gamma required for " + self.getConstraintOption('statistic', i) +". gamma = 0.05 recommended"
            if self.getConstraintOption('bound', i) in none_list:
                raise RuntimeError, "Constraint bound not defined"
            if lower(self.getConstraintOption('goal', i)) in self.cost_goals + self.other_goals:
                if lower(self.getConstraintOption('statistic', i)) not in ['total']:
                    raise RuntimeError, "Constraint statistic must equal TOTAL when goal is " + self.getConstraintOption('goal', i) +"."
        
        # AGGREGATE
        # can we put skeletonization input here?  type = SKELETON? to group nodes in the impact file?
        for i in range(len(self.opts['aggregate'])):
            if self.getAggregateOption('goal', i) not in none_list:
                if lower(self.getAggregateOption('goal', i)) not in self.impact_goals:
                    raise RuntimeError, "Invalid aggregation goal.  Valid goals = impact name"
                if lower(self.getAggregateOption('type', i)) not in ['threshold', 'percent', 'ratio']:
                    raise RuntimeError, "Invalid aggregation type.  Valid types = THRESHOLD, PERCENT, RATIO"
        
        # IMPERFECT
        for i in range(len(self.opts['imperfect'])):
            if self.getImperfectOption('sensor class file', i) not in none_list:
                #if self.getImperfectOption('directory', i) not in none_list:
                #    sensor_class_file = os.path.join(self.getImperfectOption('directory', i),self.getImperfectOption('sensor class file', i))
                #else:
                sensor_class_file = self.getCostOption('sensor class file', i)
                if not os.path.exists(sensor_class_file):
                    raise RuntimeError, "Sensor class file file does not exist"
            if self.getImperfectOption('junction class file', i) not in none_list:
                #if self.getImperfectOption('directory', i) not in none_list:
                #    junction_class_file = os.path.join(self.getImperfectOption('directory', i),self.getImperfectOption('junction class file', i))
                #else:
                junction_class_file = self.getCostOption('junction class file', i)
                if not os.path.exists(junction_class_file):
                    raise RuntimeError, "Junction class file file does not exist"
        
        # Default solver    
        #self.default_solver = None
        #for j in range(len(self.opts['solver'])):
            #if self.opts['solver'][j]['name'] == self.getConfigureOption('solver')[0]:
                #self.default_solver = j
                #break
        #if self.default_solver is None:
            #raise RuntimeError, "the configured solver name '%s' is not the name of a specified solver block" % self.opts['configure']['solver']
        
        # Default problem    
        #self.default_problem = None
        #for j in range(len(self.opts['sensor placement'])):
        #    if self.opts['sensor placement'][j]['name'] == self.getConfigureOption('sensor placement')[0]:
        #        self.default_problem = j
        #        break
        #if self.default_problem is None:
        #    raise RuntimeError, "The configured problem name '%s' is not the name of a specified problem block" % self.opts['configure'][0]['sensor placement']
        
    def run_sp(self, addDate=False):
        # current SP specifies network name, directory, weight and response time once
        network_opt = True
        # current SP specifies cost file once
        cost_opt = True
        # current SP specifies gamma once
        gamma_opt = True
        
        logger.info("WST spold subcommand")
        logger.info("---------------------------")
        
        # set start time
        startTime = time.time()
        
        # Call the validate() routine to initialize the problem data
        logger.info("Validating configuration file")
        self.validate() 
        
        # Setup the temporary data directory
        if not self.getConfigureOption('temp directory') in none_list:
            pyutilib.services.TempfileManager.tempdir = os.path.abspath(self.getConfigureOption('temp directory'))
        else:
            pyutilib.services.TempfileManager.tempdir = os.path.abspath(os.getcwd())
        
        # Setup object to store global data
        tmpdir = os.path.dirname(self.opts['configure']['output prefix'])
        tmpprefix = 'tmp_'+os.path.basename(self.opts['configure']['output prefix'])
        tmpdata_prefix = pyutilib.services.TempfileManager.create_tempfile(prefix=tmpprefix, dir=tmpdir)
        
        # Preprocess data
        logger.info("Preprocessing data")
        self.preprocess(tmpdata_prefix, True) # spold = True
        
        # EXECUTABLE
        cmd = self.getConfigureOption('sp executable') + ' --print-log'
        
        # PROBLEM, SOLVER
        prob_name = self.getConfigureOption('sensor placement')
        solver_name = self.getConfigureOption('solver')
        
        # OBJECTIVE
        for i in range(len(self.getProblemOption('objective'))): 
            obj_name = self.getProblemOption('objective', prob_name)
            
            # IMPACT objective
            if lower(self.getObjectiveOption('goal', obj_name)) in self.impact_goals:
                impact_name = self.getObjectiveOption('goal', obj_name)
                impactFile = self.getImpactOption('original impact file',impact_name)
                impactName = impactFile.rsplit('.',1)[0]
                impactNetwork = impactName.rsplit('_',1)[0]
                impactMetric = impactName.rsplit('_',1)[-1]
                if network_opt:
                    cmd = cmd + " --network="+impactNetwork
                    #if self.getImpactOption('directory', impact_name) not in none_list:
                    #    cmd = cmd + " --impact-dir="+self.getImpactOption('original directory', impact_name)
                    if self.getImpactOption('weight file', impact_name) not in none_list:
                        cmd = cmd + " --incident-weights="+str(self.getImpactOption('weight file', impact_name))
                    #if self.getImpactOption('response time', impact_name) not in none_list:
                        #cmd = cmd + " --responseTime="+str(self.getImpactOption('response time', impact_name))
                    network_opt = False
            
                cmd = cmd + " --objective="+impactMetric.lower()+"_"+self.getObjectiveOption('statistic', obj_name).lower()
                    
            # COST objective
            elif lower(self.getObjectiveOption('goal', obj_name)) in self.cost_goals:
                cmd = cmd + " --objective=cost" # statistic = total
                if cost_opt:
                    #cost_name = self.getObjectiveOption('goal', obj_name) 
                    #if self.getCostOption('directory', cost_name) not in none_list:
                    #    cmd = cmd + " --costs="+str(os.path.join(self.getCostOption('directory', cost_name),self.getCostOption('cost file', cost_name)))
                    #else:
                    #    cmd = cmd + " --costs="+str(self.getCostOption('cost file', cost_name))
                    cmd = cmd + " --costs="+self.single_cost_file
                    cost_opt = False

            # NFD, NS objective
            elif lower(self.getObjectiveOption('goal', obj_name)) in self.other_goals:
                cmd = cmd + " --objective="+self.getObjectiveOption('goal', obj_name).lower()+"_"+self.getObjectiveOption('statistic', obj_name).lower()
            
            else:
                raise RuntimeError, "Invalid objective goal '%s'.  Valid goals = %s, cost name, NS, NFD" % (self.getObjectiveOption('goal', obj_name), ', '.join(self.impact_goals))
        
            if gamma_opt:
                if lower(self.getObjectiveOption('statistic', obj_name)) in ['var', 'cvar']:
                    #cmd = cmd + " --gamma="+str(self.getObjectiveOption('gamma', obj_name))
                    cmd = cmd + " --gamma="+str(self.single_gamma)
                    gamma_opt = False
                    
        # CONSTRAINT
        for const_name in self.getProblemOption('constraint', prob_name): 
        
            # IMPACT constraint
            if lower(self.getConstraintOption('goal', const_name)) in self.impact_goals:
                impact_name = self.getConstraintOption('goal', const_name)
                impactFile = self.getImpactOption('original impact file',impact_name)
                impactName = impactFile.rsplit('.',1)[0]
                impactNetwork = impactName.rsplit('_',1)[0]
                impactMetric = impactName.rsplit('_',1)[-1]
                if network_opt:
                    cmd = cmd + " --network="+impactNetwork
                    #if self.getImpactOption('directory', impact_name) not in none_list:
                    #    cmd = cmd + " --impact-dir="+self.getImpactOption('original directory', impact_name)
                    if self.getImpactOption('weight file', impact_name) not in none_list:
                        cmd = cmd + " --incident-weights="+str(self.getImpactOption('weight file', impact_name))
                    #if self.getImpactOption('response time', impact_name) not in none_list:
                    #    cmd = cmd + " --responseTime="+str(self.getImpactOption('response time', impact_name))
                    network_opt = False
                else:
                    print "Inconsistent network name"
                        
                cmd = cmd + " --ub="+impactMetric.lower()+"_"+self.getConstraintOption('statistic', const_name).lower()+","+str(self.getConstraintOption('bound', const_name))
            
            # COST constraint
            elif lower(self.getConstraintOption('goal', const_name)) in self.cost_goals:
                cmd = cmd + " --ub=cost,"+str(self.getConstraintOption('bound', const_name)) # statistic = total
                if cost_opt:
                    #cost_name = self.getConstraintOption('goal', const_name)
                    #if self.getCostOption('directory', cost_name) not in none_list:
                    #    scmd = cmd + str(os.path.join(self.getCostOption('directory', cost_name),self.getCostOption('cost file', cost_name)))
                    #else:
                    #    cmd = cmd + str(self.getCostOption('cost file', cost_name))
                    cmd = cmd + " --costs="+self.single_cost_file
                    cost_opt = False
                
            # NFD, NS constraint
            elif lower(self.getConstraintOption('goal', const_name)) in self.other_goals:
                cmd = cmd + " --ub="+self.getConstraintOption('goal', const_name).lower()+","+str(self.getConstraintOption('bound', const_name))
                    
            else:
                raise RuntimeError, "Invalid constraint goal '%s'.  Valid goals = %s, %s, NS, NFD" % (self.getConstraintOption('goal', const_name), ', '.join(self.impact_goals), ', '.join(self.cost_goals))
        
            
            if gamma_opt:
                if lower(self.getConstraintOption('statistic', const_name)) in ['var', 'cvar']:
                    #cmd = cmd + " --gamma="+str(self.getConstraintOption('gamma', const_name))
                    cmd = cmd + " --gamma="+self.single_gamma
                    gamma_opt = False
        
        # SOLVER
        cmd = cmd + " --solver="+self.getSolverOption('type')
        if self.getSolverOption('seed') not in none_list:
            cmd = cmd + " --seed="+str(self.getSolverOption('seed'))
            
        # Are these solver specific?
        if self.getSolverOption('representation') not in none_list:
            cmd = cmd + " --grasp-representation="+str(self.getSolverOption('representation'))
        if self.getSolverOption('number of samples') not in none_list:
            cmd = cmd + " --numsamples="+str(self.getSolverOption('number of samples'))
        if self.getSolverOption('timelimit') not in none_list:
            cmd = cmd + " --runtime="+str(self.getSolverOption('timelimit'))
        if self.getSolverOption('notify') not in none_list:
            cmd = cmd + " --notify="+str(self.getSolverOption('notify'))
        if self.getProblemOption('compute bound'):
            cmd = cmd + " --compute-bound"
        if self.getConfigureOption('print log'):
            cmd = cmd + " --print-log"
        
        # PATH options (required for now)
        sp_problem_dir = os.path.dirname(os.path.abspath(__file__))
        wst_dir = sp_problem_dir+'/../../../..'
        cmd = cmd + " --path="+wst_dir+"/bin/"
        cmd = cmd + " --path="+wst_dir+"/etc/mod/"
        # TODO the above 4 lines should be replaced by
        #for path in self.getConfigureOption('path')
        #    cmd = cmd + " --path="+path

        # LOCATION options
        cmd = cmd + " --sensor-locations="+self.getConfigureOption('output prefix')+'_location'
        
        # AGGREGATE options
        if self.getProblemOption('aggregate') not in none_list:
            if len(self.getProblemOption('aggregate')) > 1: 
                print "Warning: Multiple aggregate blocks selected.  Using aggregate block " + self.getProblemOption('aggregate')[0]
            agg_name = self.getProblemOption('aggregate')
            
            # IMPACT
            if lower(self.getAggregateOption('goal', agg_name)) in self.impact_goals:
                impact_name = self.getAggregateOption('goal', agg_name)
                impactFile = self.getImpactOption('original impact file',impact_name)
                impactName = impactFile.rsplit('.',1)[0]
                impactNetwork = impactName.rsplit('_',1)[0]
                impactMetric = impactName.rsplit('_',1)[-1]  

                type = lower(self.getAggregateOption('type', agg_name))
                value = self.getAggregateOption('value', agg_name)
                if type == 'threshold':
                    cmd = cmd + " --aggregation-threshold="+impactMetric.lower()+','+str(value)
                if type == 'percent':
                    cmd = cmd + " --aggregation-percent="+impactMetric.lower()+','+str(value)
                if type == 'ratio':
                    cmd = cmd + " --aggregation-ratio="+impactMetric.lower()+','+str(value)
                if self.getAggregateOption('conserve memory', agg_name) not in none_list:
                    cmd = cmd + " --conserve-memory="+str(self.getAggregateOption('conserve memory', agg_name))
                if self.getAggregateOption('distinguish detection', agg_name) not in none_list:
                    cmd = cmd + " --distinguish-detection="+str(self.getAggregateOption('distinguish detection', agg_name))
                if self.getAggregateOption('disable aggregation', agg_name) not in none_list:
                    cmd = cmd + " --disable-aggregation="+str(self.getAggregateOption('disable aggregation', agg_name))
            
        # IMPERFECT options
        # TODO: modify sc and jc files based on fixed/infeasible data
        if self.getProblemOption('imperfect') not in none_list:
            if len(self.getProblemOption('imperfect')) > 1: 
                print "Warning: Multiple imperfect blocks selected.  Using imperfect block " + self.getProblemOption('imperfect')[0]
            imperf_name = self.getProblemOption('imperfect')[0]
            
            if self.getImperfectOption('sensor class file', imperf_name) not in none_list:
                cmd = cmd + " --imperfect-scfile="+self.getImperfectOption('sensor class file', imperf_name)
            if self.getImperfectOption('junction class file', imperf_name) not in none_list:
                cmd = cmd + " --imperfect-jcfile="+self.getImperfectOption('junction class file', imperf_name)
            
        # CONFIGURE options 
        cmd = cmd + " --tmp-file="+str(self.getConfigureOption('output prefix'))+"_tmp"
        cmd = cmd + " --output="+str(self.getConfigureOption('output prefix'))+".sensors"
        #cmd = cmd + " --summary="+str(self.getConfigureOption('output prefix', 0))+".summary"
        if self.getConfigureOption('memmon'):
            cmd = cmd + " --memmon"
        if self.getConfigureOption('memcheck') not in none_list:
            cmd = cmd + " --memcheck="+str(self.getConfigureOption('memcheck'))
        if self.getConfigureOption('format') not in none_list:
            cmd = cmd + " --format="+str(self.getConfigureOption('format'))
        #if self.getConfigureOption('path') not in none_list:
        #    cmd = cmd + " --path="+str(self.getConfigureOption('path'))
        #if self.getConfigureOption('ampl cplex path') not in none_list:
        #    cmd = cmd + " --amplcplexpath="+str(self.getConfigureOption('ampl cplex path'))
        #if self.getConfigureOption('pico path',) not in none_list:
        #    cmd = cmd + " --picopath="+str(self.getConfigureOption('pico path'))
        #if self.getConfigureOption('glpk path') not in none_list:
        #    cmd = cmd + " --glpkpath="+str(self.getConfigureOption('glpk path'))
        if self.getConfigureOption('ampl') not in none_list:
            cmd = cmd + " --ampl="+str(self.getConfigureOption('ampl'))
        if self.getConfigureOption('ampl data') not in none_list:
            cmd = cmd + " --ampldata="+str(self.getConfigureOption('ampl data'))
        if self.getConfigureOption('ampl model') not in none_list:
            cmd = cmd + " --amplmodel="+str(self.getConfigureOption('ampl model'))
        if self.getConfigureOption('debug'):
            cmd = cmd + " --debug"
        if self.getConfigureOption('gap') not in none_list:
            cmd = cmd + " --gap="+str(self.getConfigureOption('gap'))
        if self.getProblemOption('compute greedy ranking'):
            cmd = cmd + " --compute-greedy-ranking"
        if self.getConfigureOption('evaluate all'):
            cmd = cmd + " --eval-all"
        if self.getConfigureOption('version'):
            cmd = cmd + " --version"

        logger.info("Running spold")
        logger.debug(cmd)
        
        tmpdir = os.path.dirname(self.opts['configure']['output prefix'])
        out = os.path.join(tmpdir, 'spold.out') #pyutilib.services.TempfileManager.create_tempfile(dir=tmpdir, prefix='tmp_', suffix='spold.out') 
        sim_timelimit = None
        sub_logger = logging.getLogger('wst.sp.spold.exec')
        sub_logger.setLevel(logging.DEBUG)
        fh = logging.FileHandler(out, mode='w')
        sub_logger.addHandler(fh)
        try:
            p = pyutilib.subprocess.run(cmd,timelimit=sim_timelimit,stdout=pywst.common.problem.LoggingFile(sub_logger))
            #pyutilib.subprocess.run(cmd, self.getConfigureOption('output prefix')+".out")
            #rc, output = pyutilib.subprocess.run(cmd, tee=True)
            if p[0]:
                msg = "ERROR executing the 'sp' script"
                logger.error(msg)
                raise RuntimeError(msg)
                
            # remove temporary files if debug = 0
            if self.opts['configure']['debug'] == 0:
                pyutilib.services.TempfileManager.clear_tempfiles()
                
            # write output file 
            prefix = os.path.basename(self.opts['configure']['output prefix'])      
            logfilename = logger.parent.handlers[0].baseFilename
            outfilename = logger.parent.handlers[0].baseFilename.replace('.log','.yml')
            
            # YAML output
            config = wst_config.output_config()
            module_blocks = ("general", "sensor placement")
            template_options = {
                'general':{
                    'cpu time': time.time() - startTime,
                    'log file': None},
                'sensor placement': {} }
            
            if outfilename != None:
                self.saveOutput(outfilename, config, module_blocks, template_options)
            
            # print solution to screen
            logger.info("\nWST normal termination")
            logger.info("---------------------------")
            dir_ = os.path.dirname(logfilename)
            if dir_ == "":
                dir_ = '.'
            logger.info("Directory: " + dir_)
            logger.info("Results file: "+os.path.basename(outfilename))
            logger.info("Log file: "+os.path.basename(logfilename)+'\n')
        finally:
            sub_logger.removeHandler(fh)
            fh.close()
        
        

    def get_index(self, block, name):
        for i, val in enumerate(self.opts[block]):
            if val['name'] == name:
                return i
        raise RuntimeError, "There is no '%s' with name '%s'" % (block, name)
        #name_indexer = dict((p['name'], i) for i, p in enumerate(self.opts[block]))
        #try:
        #    return name_indexer[name]
        #except:
        #    raise RuntimeError, "There is no '%s' with name '%s'" % (block, name)
    
    def preprocess(self, tmpdata_prefix, spold=False):
        # Preprocess step includes:
        #       Read in nodemap file, create junction map
        #       Read in cost file, create cost map
        #       Remove comments from impact file
        #       Remove fixed and infeasible nodes from impact file
        #       Define single nodemap file, cost file and gamma to use in sp
        #       TODO condense nodemap index
        #       other?
        
        prob_name = self.getConfigureOption('sensor placement')
        
        # Collect impact files used in the problem and preprocess files
        impact_name = []
        for i in range(len(self.getProblemOption('objective'))): 
            obj_name = self.getProblemOption('objective', prob_name)
            if lower(self.getObjectiveOption('goal', obj_name)) in self.impact_goals:
                impact_name.append(self.getObjectiveOption('goal', obj_name))
                
        for const_name in self.getProblemOption('constraint', prob_name): 
            if lower(self.getConstraintOption('goal', const_name)) in self.impact_goals:
                impact_name.append(self.getConstraintOption('goal', const_name))

        impact_name = set(impact_name) # remove duplicates
        impact_name = list(impact_name) # convert back to list
    
        nodemap_flag = True
        
        for i in impact_name:
            # Get impact and nodemap file name
            #if self.getImpactOption('directory',i) in none_list:
            #    impact_file = self.getImpactOption('impact file',i)
            #    nodemap_file = self.getImpactOption('nodemap file',i)
            #else:
            impact_file = self.getImpactOption('impact file',i)
            nodemap_file = self.getImpactOption('nodemap file',i)
            # Define junction map and single nodemap file
            if nodemap_flag:
                if not os.path.exists(nodemap_file):
                    print "Warning: Nodemap file does not exist, using identity map"
                    [num_nodes, num_lines, node_names] = read_impact_file(impact_file)
                    self.junction_map = dict(zip(node_names,node_names))
                else:
                    self.junction_map = read_node_map(nodemap_file)
                    
                # Find invalid and fixed nodes
                [self.invalid_vals, self.fixed_vals] = self.read_sensor_locations(self.junction_map, prob_name)
                
                if self.getConfigureOption('debug'):
                    print "Using nodemap file " + nodemap_file + " from " + i
                #if self.getImpactOption('directory',i) in none_list:
                #    self.single_nodemap_file = self.getImpactOption('nodemap file',i)
                #else:
                self.single_nodemap_file = self.getImpactOption('nodemap file',i)
                nodemap_flag = False
            
            # Define temp impact and nodemap file name
            tmpImpactFile = tmpdata_prefix+os.path.basename(self.getImpactOption('impact file',i))
            tmpNodemapFile = tmpdata_prefix+os.path.basename(self.getImpactOption('nodemap file',i))
            pyutilib.services.TempfileManager.add_tempfile(tmpImpactFile, exists=False)
            pyutilib.services.TempfileManager.add_tempfile(tmpNodemapFile, exists=False)
            self.setImpactOption('original impact file', i, self.getImpactOption('impact file', i))
            self.setImpactOption('original nodemap file', i, self.getImpactOption('nodemap file', i))
            #self.setImpactOption('original directory', i, self.getImpactOption('directory', i))
            self.setImpactOption('impact file', i, tmpImpactFile)
            self.setImpactOption('nodemap file', i, tmpNodemapFile)
            #self.setImpactOption('directory', i, os.path.dirname(tmpImpactFile))
            
            # Copy nodemap file to tmp nodemap
            if not os.path.exists(tmpNodemapFile):
                self.solution_map = {}
                self.inverse_solution_map = {-1:-1}
                fidIn = open(nodemap_file, 'r')
                fidOut = open(tmpNodemapFile, 'w')
                num=1
                ctr = 0
                while True:
                    line = fidIn.readline()
                    if not line:
                        break
                    line = line.strip()
                    if line == '':
                        continue
                    if line[0] == '#':
                        continue
                    token = line.split(' ')
                    ctr += 1
                    if int(token[0]) in self.fixed_vals+self.invalid_vals:
                        continue
                    print >>fidOut, num, token[1]
                    self.solution_map[num] = ctr
                    self.inverse_solution_map[ctr] = num
                    num += 1
                fidIn.close() 
                fidIn.close() 
            
            # Create tmp impact file
            fidIn = open(impact_file, 'r')
            while True:
                impactLine1 = fidIn.readline()
                if impactLine1.rstrip(): # skip blank line
                    if impactLine1.split()[0] is not '#': # skip comment lines
                        break
            impactLine2 = fidIn.readline()

            fidOut = open(tmpImpactFile, 'w')
            fidOut.write("%d\n" % len(self.solution_map))
            #fidOut.write(impactLine1)
            fidOut.write(impactLine2)

            skip = None
            while True:
                lines = fidIn.readlines(100000) # read in subset of file
                if not lines:
                    break
                lines = map(str.rstrip,lines,'\n')
                lines = map(str.split,lines,' ')
                for line in lines:
                    line = [int(line[0]), int(line[1]), int(line[2]), float(line[3])]
                    
                    if not spold:
                        if line[0] == skip:
                            # Skip the rest of an impact event for which we have found a fixed sensor
                            continue
                        if line[1] in self.invalid_vals:
                            # Skip invalid locations
                            continue
                        if line[1] in self.fixed_vals:
                            # If we have a fixed sensor, then treat this as the dummy for that event
                            fidOut.write("%i -1 %i %-7g\n" % (line[0],line[2],line[3]))
                            skip = line[0]
                        else:
                            # Simply copy the line from the previous impact file
                            fidOut.write("%i %i %i %-7g\n" % (line[0],self.inverse_solution_map[line[1]],line[2],line[3]))
                            skip = None
                    else:
                        fidOut.write("%i %i %i %-7g\n" % (line[0],line[1],line[2],line[3]))
            
            # TODO Condense set of nodes in impact and nodemap file            
            fidIn.close()
            fidOut.close()
        
        # Get number of sensors
        const_name = []
        for tmp_name in self.getProblemOption('constraint', prob_name): 
            if lower(self.getConstraintOption('goal', tmp_name)) in 'ns':
                const_name.append(tmp_name)

        const_name = set(const_name) # remove duplicates
        const_name = list(const_name) # convert back to list
        
        for i in const_name:
            if self.getConstraintOption('bound', i) <= 0:
                raise RuntimeError, "Number of sensors equals 0"    
            if self.getConstraintOption('bound', i) > len(self.junction_map):
                raise RuntimeError, "Number of sensors is greater than the number of nodes"    
            elif not spold: # Decrease the number of sensors by len(self.fixed_vals)
                new_ns = self.getConstraintOption('bound', i) - len(self.fixed_vals)
                self.setConstraintOption('bound',i,new_ns)
            
        if spold:
            # create location file
            self.setFeasibleNodes(prob_name)
            
        # Number of feasible locations must be > 0
        if len(self.junction_map) - len(self.invalid_vals) - len(self.fixed_vals) <= 0:
            raise RuntimeError, "Number of feasible locations equals 0"
                
        # Collect cost files used in the problem and define cost map
        cost_name = []
        for i in range(len(self.getProblemOption('objective'))): 
            obj_name = self.getProblemOption('objective', prob_name)
            if lower(self.getObjectiveOption('goal', obj_name)) in self.cost_goals:
                cost_name.append(self.getObjectiveOption('goal', obj_name))
                
        for const_name in self.getProblemOption('constraint', prob_name): 
            if lower(self.getConstraintOption('goal', const_name)) in self.cost_goals:
                cost_name.append(self.getConstraintOption('goal', const_name))
                
        cost_name = set(cost_name) # remove duplicates
        cost_name = list(cost_name) # convert back to list 
        
        if len(cost_name) > 0:
            if len(cost_name) > 1: # for now, we only use one cost file
                print "Warning: Multiple cost blocks selected.  Using cost block " + self.getCostOption('name', cost_name[0])
            #if self.getCostOption('directory', cost_name[0]) not in none_list:
            #    self.single_cost_file = os.path.join(self.getCostOption('directory', cost_name[0]),self.getCostOption('cost file', cost_name[0]))
            #else:
            self.single_cost_file = self.getCostOption('cost file', cost_name[0])
            print "Using cost file " + self.single_cost_file + " from " + cost_name[0]
            
            self.orig_cost_map = read_costs(self.single_cost_file)
            newd = dict()
            # Translate cost map to index
            default_list = ['__default__']
            for k,v in self.orig_cost_map.iteritems():
                if k not in default_list:
                    newd[self.junction_map[k]] = v
                else:
                    newd['-1'] = v
            self.cost_map = newd
        
        # Define gamma
        gamma_flag = True
        for i in range(len(self.getProblemOption('objective'))): 
            obj_name = self.getProblemOption('objective', prob_name)
            if self.getObjectiveOption('gamma', obj_name) not in none_list:
                self.single_gamma = self.getObjectiveOption('gamma', obj_name)
                #print "Gamma = " +str(self.single_gamma)
                gamma_flag = False
                break
        if gamma_flag:
            for const_name in self.getProblemOption('constraint', prob_name): 
                if self.getConstraintOption('gamma', const_name) not in none_list:
                    self.single_gamma = self.getConstraintOption('gamma', const_name)
                    print "Using gamma = " + str(self.single_gamma) + " from " + const_name
                    break
        
    def setFeasibleNodes(self, prob_name):
        fid = open(self.getConfigureOption('output prefix')+'_location','wt')
        
        for i in range(len(self.opts['sensor placement'][0]['location'])):
            # set feasible locations 
            list_feas = []
            feasible = self.getLocationOption('feasible nodes', i)
            if feasible == 'ALL':
                fid.write('feasible ALL\n')
            elif feasible.__class__ is list:
                fid.write('feasible ')
                for j in feasible:
                    fid.write(str(j)+' ')
                fid.write('\n')
            elif feasible in none_list:
                # prevents entering next 'elif' block
                pass
            elif feasible.__class__ is str:
                try:
                    fid = open(feasible,'r')
                except:
                    raise RuntimeError, "Feasible nodes file did not load"
                list_feas = fid.read()
                fid.close()
                list_feas = list_feas.splitlines()
                fid.write('feasible ')
                for j in list_feas:
                    fid.write(str(j)+' ')
                fid.write('\n')
            else:
                print >> sys.stderr, "Unsupported feasible nodes, setting option to None"
                self.setLocationOption('feasible nodes', i, None)
            
            # set infeasible locations 
            list_infeas = []
            infeasible = self.getLocationOption('infeasible nodes', i)
            if infeasible == 'ALL':
                fid.write('infeasible ALL\n')
            elif infeasible.__class__ is list:
                fid.write('infeasible ')
                for j in infeasible:
                    fid.write(str(j)+' ')
                fid.write('\n')
            elif infeasible in none_list:
                # prevents entering next 'elif' block
                pass
            elif infeasible.__class__ is str:
                try:
                    fid = open(infeasible,'r')
                except:
                    raise RuntimeError, "Infeasible nodes file did not load"
                list_infeas = fid.read()
                fid.close()
                list_infeas = list_infeas.splitlines()
                fid.write('infeasible ')
                for j in list_infeas:
                    fid.write(str(j)+' ')
                fid.write('\n')
            else:
                print "Unsupported infeasible nodes, setting option to None"
                self.setLocationOption('infeasible nodes', None, i)
            
            # set fixed locations
            list_fixed = []
            fixed = self.getLocationOption('fixed nodes', i)
            if fixed == 'ALL':
                fid.write('fixed ALL\n')
            elif fixed.__class__ is list:
                fid.write('fixed ')
                for j in fixed:
                    fid.write(str(j)+' ')
                fid.write('\n')
            elif fixed in none_list:
                # prevents entering next 'elif' block
                pass
            elif fixed.__class__ is str:
                try:
                    fid = open(fixed,'r')
                except:
                    raise RuntimeError, "Fixed nodes file did not load"
                list_fixed = fid.read()
                fid.close()
                list_fixed = list_fixed.splitlines()
                fid.write('fixed ')
                for j in list_fixed:
                    fid.write(str(j)+' ')
                fid.write('\n')
            else:
                print "Unsupported fixed nodes, setting option to None"
                self.setLocationOption('fixed nodes', i, None)
            
            # set unfixed locations
            list_unfixed = []
            unfixed = self.getLocationOption('unfixed nodes', i)
            if unfixed == 'ALL':
                fid.write('unfixed ALL\n')
            elif unfixed.__class__ is list:
                fid.write('unfixed ')
                for j in unfixed:
                    fid.write(str(j)+' ')
                fid.write('\n')
            elif unfixed in none_list:
                # prevents entering next 'elif' block
                pass
            elif unfixed.__class__ is str:
                try:
                    fid = open(unfixed,'r')
                except:
                    raise RuntimeError, "Unfixed nodes file did not load"
                list_unfixed = fid.read()
                fid.close()
                list_unfixed = list_unfixed.splitlines()
                fid.write('unfixed ')
                for j in list_unfixed:
                    fid.write(str(j)+' ')
                fid.write('\n')
            else:
                print "Unsupported unfixed nodes, setting option to None"
                self.setLocationOption('unfixed nodes', i, None)
            
        fid.close()

    # General Option SET functions
    def setConfigureOption(self, name, value):
        if name == 'output prefix' and value != '':
            output_prefix = os.path.splitext(os.path.split(value)[1])[0]
            value = output_prefix
        self.opts['configure'][name] = value
    
    def setImpactOption(self, name, i, value):
        if isinstance(i, basestring):
            i = self.get_index('impact data', i)
        if i >= len(self.opts['impact data']):
            self.createNew('impact data', i)
        self.opts['impact data'][i][name] = value
    
    def setCostOption(self, name, i, value):
        if i >= len(self.opts['cost']):
            self.createNew('cost', i)
        self.opts['cost'][i][name] = value
    
    def setObjectiveOption(self, name, i, value):
        if i >= len(self.opts['objective']):
            self.createNew('objective', i)
        self.opts['objective'][i][name] = value
    
    def setConstraintOption(self, name, i, value):
        if isinstance(i, basestring):
            i = self.get_index('constraint', i)
        if i >= len(self.opts['constraint']):
            self.createNew('constraint', i)
        self.opts['constraint'][i][name] = value
    
    def setLocationOption(self, name, i, value):
        while i >= len(self.opts['sensor placement'][0]['location']):
            self.opts['sensor placement'][0]['location'].append({})
        self.opts['sensor placement'][0]['location'][i][name] = value

    def setAggregateOption(self, name, i, value):
        if i >= len(self.opts['aggregate']):
            self.createNew('aggregate', i)
        self.opts['aggregate'][i][name] = value
    
    def setImperfectOption(self, name, i, value):
        if i >= len(self.opts['imperfect']):
            self.createNew('imperfect', i)
        self.opts['imperfect'][i][name] = value
        
    def setSolverOption(self, name, i, value):
        if i:
            raise RuntimeError("the 'solver' block no longer supports list of solvers")
        #if i >= len(self.opts['solver']):
        #    self.createNew('solver', i)
        self.opts['solver'][name] = value

    def setProblemOption(self, name, i, value):
        if i >= len(self.opts['sensor placement']):
            self.createNew('sensor placement', i)
        self.opts['sensor placement'][i][name] = value

    def setSolverOptionDefault(self, name, value):
        self.opts['solver'][name] = value

    def setProblemOptionDefault(self, name, value):
        self.opts['sensor placement'][name] = value

    def createNew(self, key, i):
        while i >= len(self.opts[key]):
            self.opts[key].append({})
        #for subkey in self.opts[key][0].keys():
        #    self.opts[key][i].setdefault(subkey,'None')
            
    # General Option GET functions
    def getNetworkOption(self, name):
        val = self.opts['network'].get(name, None)
        if val:
            val = val.value()
        return val

    def getConfigureOption(self, name):
        val = self.opts['configure'].get(name, None)
        if val:
            val = val.value()
        return val

    def getImpactOption(self, name, i):
        if isinstance(i, basestring):
            i = self.get_index('impact data', i)
        val = self.opts['impact data'][i].get(name, None)
        if val:
            val = val.value()
        return val
    
    def getCostOption(self, name, i):
        if isinstance(i, basestring):
            i = self.get_index('cost', i)
        val = self.opts['cost'][i].get(name, None)
        if val:
            val = val.value()
        return val
        
    def getObjectiveOption(self, name, i):
        if isinstance(i, basestring):
            i = self.get_index('objective', i)
        val = self.opts['objective'][i].get(name, None)
        if val:
            val = val.value()
        return val
        
    def getConstraintOption(self, name, i):
        if isinstance(i, basestring):
            i = self.get_index('constraint', i)
        val = self.opts['constraint'][i].get(name, None)
        if val:
            val = val.value()
        return val
    
    def getLocationOption(self, name, i):
        val = self.opts['sensor placement'][0]['location'][i].get(name, None)
        if val:
            val = val.value()
        return val

    def getAggregateOption(self, name, i):
        if isinstance(i, basestring):
            i = self.get_index('aggregate', i)
        val = self.opts['aggregate'][i].get(name, None)
        if val:
            val = val.value()
        return val
    
    def getImperfectOption(self, name, i):
        if isinstance(i, basestring):
            i = self.get_index('imperfect', i)
        val = self.opts['imperfect'][i].get(name, None)
        if val:
            val = val.value()
        return val
        
    def getSolverOption(self, name, i=None):
        if i is not None and i is not 0:
            raise RuntimeError("The 'solver' block no longer takes a list of solvers")
        val = self.opts['solver'].get(name, None)
        if val:
            val = val.value()
        return val
        
    def getProblemOption(self, name, i=None):
        if i is None:
            i = 0
        if isinstance(i, basestring):
            i = self.get_index('sensor placement', i)
        val = self.opts['sensor placement'][i].get(name, None)
        if val:
            val = val.value()
        return val
        
    def translate_solutions(self, solutions):
        #
        # From: script.py line 1567
        #
        # Transform the locations to original location IDs before invalid
        # locations were removed.
        #
        # If we created aggregated impact files, the mapping done in the
        # location_map() function mapped chosen locations back to original
        # locations, so this transformation is not necessary.
        #
        if len(solutions) == 0:
            return []
        translated = []
        for i in range(len(solutions)):
            translated.append( self.translate_solution( solutions[i] ) )
        return translated

    def translate_solution(self, soln):
        # Extend each solution with the fixed locations
        for j in range(len(soln)):
            soln[j] = self.solution_map[soln[j]]
        soln.extend(self.fixed_vals)
        # The junction map is always created, even if it equals 1:1
        if len(self.junction_map) == 0:
            return soln
        #
        tmp = []
        for val in soln:
            for k, v in self.junction_map.iteritems():
                if v == str(val):
                    tmp.append(k)
                    break
        #
        return tmp
            
    def create_wst_configfile(self, prefix):
        (num_nodes, numlines) = read_impact_files(self)
        impact = {}
        for i in range(len(self.opts['impact data'])):
            impact[self.getImpactOption('name',i)] = i
        cost = {}
        for i in range(len(self.opts['cost'])):
            cost[self.getCostOption('name',i)] = i
        aggregate = {}
        for i in range(len(self.opts['aggregate'])):
            aggregate[self.getAggregateOption('goal',i)] = i

        config_file = prefix+'.config'
        OUTPUT = open(config_file, "w")

        # name of files written by aggregateImpacts if it runs
        ##map_to_first_file = prefix + "_earliest.config"
        ##map_to_all_file = prefix + "_map.config"

        # the two-pass approach is necessary to handle cases where
        # uses specify side constraints on different statistics of
        # a primary objective goal.
        # TODO: should we be ignoring the delay value?
        print >>OUTPUT, len(self.solution_map), 0

        # output the number of goals (objectives or side constraints)
        print >>OUTPUT, len(self.opts['objective'])+len(self.opts['constraint'])

        # output the objective and any side constraints related to the objective goal
        for i in range(len(self.opts['objective'])):
            goal = lower(self.getObjectiveOption('goal',i))
            if goal in cost:
                print >>OUTPUT, 'cost',     # BUG?
            else:
                print >>OUTPUT, goal,
            #
            if goal == "ns" or goal == "cost" or goal == "awd":
               print >>OUTPUT, "none",
            elif goal in impact:
               #dir = self.getImpactOption('directory',impact[goal]) 
               #if dir in none_list:
               #     dir = ""
               #else:
               #     dir = dir + os.sep
               #print >>OUTPUT, dir+self.getImpactOption('impact file',impact[goal]),
               print >>OUTPUT, self.getImpactOption('impact file',impact[goal]),
            elif goal in cost:
               #dir = self.getCostOption('directory',cost[goal])
               #if dir in none_list:
               #     dir = ""
               print >>OUTPUT, 'none',
            #
            if goal in aggregate and lower(self.getAggregateOption('type', aggregate[goal])) == 'threshold' and not self.getAggregateOption('value', aggregate[goal]) in none_list:
               print >>OUTPUT, self.getAggregateOption('value', aggregate[goal]),
            else:
               # TODO: why is this nonzero?
               print >>OUTPUT, 1e-7,
            #
            if goal in aggregate and lower(self.getAggregateOption('type', aggregate[goal])) == 'percent' and not self.getAggregateOption('value', aggregate[goal]) in none_list:
               print >>OUTPUT, self.getAggregateOption('value', aggregate[goal]),
            else:
               print >>OUTPUT, 0.0,
            #
            if goal in aggregate and lower(self.getAggregateOption('type', aggregate[goal])) == 'ratio' and not self.getAggregateOption('value', aggregate[goal]) in none_list:
               print >>OUTPUT, self.getAggregateOption('value', aggregate[goal]),
            else:
               print >>OUTPUT, 0.0,
            #
            if goal in aggregate and not self.getAggregateOption('distinguish detection', aggregate[goal]) in none_list:
               print >>OUTPUT, self.getAggregateOption('distinguish detection', aggregate[goal]),
            else:
               print >>OUTPUT, 0,
            # only one measure for the objective goal can be specified
            print >>OUTPUT, 1,
            print >>OUTPUT, lower(self.getObjectiveOption('statistic', i)),
            print >>OUTPUT, "o",
            print >>OUTPUT, "-99999",
            print >>OUTPUT, ""

        # output the non-objective goal side constraints
        for i in range(len(self.opts['constraint'])):
            goal = lower(self.getConstraintOption('goal',i))
            if goal in cost:
                print >>OUTPUT, 'cost',
            else:
                print >>OUTPUT, goal,
            #
            if goal == "ns" or goal == "cost" or goal == "awd":
               print >>OUTPUT, "none",
            elif goal in impact:
               #dir = self.getImpactOption('directory',impact[goal])
               #if dir in none_list:
               #     dir = ""
               #else:
               #     dir = dir + os.sep
               #print >>OUTPUT, dir+self.getImpactOption('impact file',impact[goal]),
               print >>OUTPUT, self.getImpactOption('impact file',impact[goal]),
            elif goal in cost:
               #dir = self.getCostOption('directory',cost[goal])
               #if dir in none_list:
               #     dir = ""
               #else:
               #     dir = dir + os.sep
               print >>OUTPUT, 'none',
            #
            if goal in aggregate and lower(self.getAggregateOption('type', aggregate[goal])) == 'threshold' and not self.getAggregateOption('value', aggregate[goal]) in none_list:
               print >>OUTPUT, self.getAggregateOption('value', aggregate[goal]),
            else:
               print >>OUTPUT, 0.0,
            #
            if goal in aggregate and lower(self.getAggregateOption('type', aggregate[goal])) == 'percent' and not self.getAggregateOption('value', aggregate[goal]) in none_list:
               print >>OUTPUT, self.getAggregateOption('value', aggregate[goal]),
            else:
               print >>OUTPUT, 0.0,
            #
            if goal in aggregate and lower(self.getAggregateOption('type', aggregate[goal])) == 'ratio' and not self.getAggregateOption('value', aggregate[goal]) in none_list:
               print >>OUTPUT, self.getAggregateOption('value', aggregate[goal]),
            else:
               print >>OUTPUT, 0.0,
            #
            if goal in aggregate and not self.getAggregateOption('distinguish detection', aggregate[goal]) in none_list:
               print >>OUTPUT, self.getAggregateOption('distinguish detection', aggregate[goal]),
            else:
               print >>OUTPUT, 0,
            # only one measure for the objective goal can be specified
            print >>OUTPUT, 1, lower(self.getConstraintOption('statistic', i)),
            print >>OUTPUT, "c",
            if goal in cost:
                cost_bound = self.getConstraintOption('bound', i)
                for loc in self.fixed_vals:
                    if str(loc) in self.cost_map:
                        cost_bound -= self.cost_map[str(loc)]
                    else:
                        cost_bound -= self.cost_map['-1']
                print >>OUTPUT, cost_bound, 
            else:
                print >>OUTPUT, self.getConstraintOption('bound', i),
            print >>OUTPUT, ""

        print >>OUTPUT, 0
        #print >>OUTPUT, len(self.fixed_vals),
        #for val in self.fixed_vals:
          #print >>OUTPUT, val,
        #print >>OUTPUT, ""

        print >>OUTPUT, 0
        #print >>OUTPUT, len(self.invalid_vals),
        #for val in self.invalid_vals:
          #print >>OUTPUT, val,
        #print >>OUTPUT, ""

        print >>OUTPUT, len(self.cost_map),
        for val in self.cost_map:
            ival = int(val)
            if ival == -1:
                print >>OUTPUT, "%s %d " % ('-1', self.cost_map[val]),
            elif ival in self.solution_map:
                print >>OUTPUT, "%d %d " % (self.solution_map[ival], self.cost_map[val]),
        print >>OUTPUT, ""

        OUTPUT.close()
        return config_file
    
    def read_sensor_locations(self, junction_map, prob_name):
        # This function was adopted from the read_sensor_locations function that
        # reads in a location file.  "location" is now defined in the yml file
        # using a listed dictonary.  Each line in the list can have 4 dict keys: 
        # feasible, infeasible, fixed and unfixed.  The dictonary is read in 
        # this order, additional entries in the list overwrite node status.
        junctionstatus = {}
        fixed=[]
        invalid=[]
        for key in junction_map.keys():
            junctionstatus[ junction_map[key] ] = "fu"  # feasible-unfixed
        
        self.opts['sensor placement'][0]['location'].display()
        for i in range(len(self.opts['sensor placement'][0]['location'])):
            # feasible
            piece = self.getLocationOption('feasible nodes', i)
            status = "fu"
            if piece == 'ALL':
                for key in junctionstatus.keys():
                    junctionstatus[key] = status
            elif piece.__class__ is list:
                for j in piece:
                    if str(j) not in junction_map.keys():
                        raise RuntimeError, "Invalid junction ID in sensor locations file: " + str(j)
                    else:
                        junctionstatus[junction_map[str(j)]] = status
            
            # infeasible
            piece = self.getLocationOption('infeasible nodes', i)
            status = "iu"
            if piece == 'ALL':
                for key in junctionstatus.keys():
                    junctionstatus[key] = status
            elif piece.__class__ is list:
                for j in piece:
                    if str(j) not in junction_map.keys():
                        raise RuntimeError, "Invalid junction ID in sensor locations file: " + str(j)
                    else:
                        junctionstatus[junction_map[str(j)]] = status
                
            # fixed
            piece = self.getLocationOption('fixed nodes', i)
            status = "ff"
            if piece == 'ALL':
                for key in junctionstatus.keys():
                    junctionstatus[key] = status
            elif piece.__class__ is list:
                for j in piece:
                    if str(j) not in junction_map.keys():
                        raise RuntimeError, "Invalid junction ID in sensor locations file: " + str(j)
                    else:
                        junctionstatus[junction_map[str(j)]] = status
            
            # unfixed
            piece = self.getLocationOption('unfixed nodes', i)
            status = "fu"
            if piece == 'ALL':
                for key in junctionstatus.keys():
                    junctionstatus[key] = status
            elif piece.__class__ is list:
                for j in piece:
                    if str(j) not in junction_map.keys():
                        raise RuntimeError, "Invalid junction ID in sensor locations file: " + str(j)
                    else:
                        junctionstatus[junction_map[str(j)]] = status
            
        # Setup the invalid_vals and fixed_vals arrays
        for key in junctionstatus.keys():
            if junctionstatus[key] == "iu":
                invalid = invalid + [ eval(key) ]
            elif junctionstatus[key] == "ff":
                fixed = fixed + [ eval(key) ]
        invalid.sort()
        fixed.sort()

        return [invalid, fixed]


def count_lines(file):
  count = 0
  for line in open(file,"r"):
    count = count + 1
  return count

def read_impact_files(prob):
  num_nodes = 0
  valid_delay=False
  got_num_nodes=False
  # We look for several files, but only save num_nodes & num_lines
  # from the first found.  What's the right thing to do? TODO
  for i in range(len(prob.opts['impact data'])):
      #dir = prob.getImpactOption('directory',i)
      #if dir in prob.none_list:
      #  dir = ""
      #else:
      #  dir = dir + os.sep
      #fname = dir+prob.getImpactOption('impact file', i)
      fname = prob.getImpactOption('impact file', i)
      if prob.getConfigureOption('debug'):
        print "read_impact_files: ",fname
      if not os.path.exists(fname):
            raise RuntimeError, "Cannot find impact file '%s'" % fname
      if not os.path.isfile(fname):
            raise RuntimeError, "Impact file '%s' is not a text file" % fname
      if got_num_nodes == False:
        INPUT = open(fname,"r")
        line = INPUT.readline()
        line = line.strip()
        num_nodes = eval(line.split(" ")[0])
        line = INPUT.readline()
        line = line.strip()
        vals = line.split(" ")
        INPUT.close()
        num_lines = count_lines(fname)
        got_num_nodes = True

  return [num_nodes, num_lines]

def read_impact_file(fname):
    INPUT = open(fname,"r")
    line = INPUT.readline()
    line = line.strip()
    num_nodes = eval(line.split(" ")[0])
    line = INPUT.readline()
    line = line.strip()
    vals = line.split(" ")

    num_lines = 2
    node_names = []
    for line in INPUT:
        node_names.append(line.split(" ")[1])
        num_lines = num_lines + 1
    node_names = set(node_names) # remove duplicates
    node_names.discard('-1') # remove dummy
    node_names = list(node_names) # convert back to list
    
    return [num_nodes, num_lines, node_names]
  
def read_node_map(fname):
  INPUT = open(fname)
  jmap={}
  for line in INPUT.xreadlines():
    vals = line.split(" ");
    vals[1] = vals[1].strip()
    #print ":" + vals[1] + ":" + vals[0] + ":"
    jmap[vals[1]] = vals[0]
  INPUT.close()
  return jmap
  
def read_costs(fname):
  INPUT = open(fname,"r")
  result = {}
  for line in INPUT.xreadlines():
    piece = line.split()
    #print piece[0],piece[1]
    result[ piece[0].strip() ] = eval(piece[1])
  INPUT.close()
  return result

