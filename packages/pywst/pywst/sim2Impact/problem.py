#
import os, sys, datetime
import pyutilib.subprocess
import yaml
import time
import logging

import pywst.common.problem
import pywst.common.wst_config as wst_config
from pyutilib.misc.config import ConfigBlock

logger = logging.getLogger('wst.sim2Impact')
        
class Problem(pywst.common.problem.Problem):           
    
    def __init__(self):
        pywst.common.problem.Problem.__init__(self, 'sim2Impact', ("impact", "configure"))
    
    def _loadPreferences(self):
        for key in defLocs.keys():
            if key == 'sim2Impact':
                self.opts['configure']['sim2Impact executable'] = defLocs[key]

    def validate(self):
        # executables
        self._validateExecutable('sim2Impact')
        
        # erd file
        if self.opts['impact']['erd file'].__class__ is list:
            for s in self.opts['impact']['erd file']:
                if s in pywst.common.problem.none_list:
                    raise RuntimeError("Error: sim2Impact requires an ERD or TSO file")
                elif not os.path.exists(s):
                    raise RuntimeError("Error: " +s+ " does not exist")
        else:
            if self.opts['impact']['erd file'] in pywst.common.problem.none_list:
                raise RuntimeError("Error: sim2Impact requires an ERD or TSO file")
            elif not os.path.exists(self.opts['impact']['erd file']):
                raise RuntimeError("Error: " +self.opts['impact']['erd file']+ " does not exist")
            
        # metric
        if self.opts['impact']['metric'].__class__ is list:
            for s in self.opts['impact']['metric']:
                if s not in pywst.common.problem.valid_metrics:
                    raise RuntimeError("Error: invalid impact metric " + s + ".  Valid metrics = "+ ", ".join(pywst.common.problem.valid_metrics))
                elif s in ['DPD', 'DPE', 'DPK', 'PD', 'PE', 'PK']:
                    if not os.path.exists(self.opts['impact']['tai file']):
                        raise RuntimeError("Error: " +self.opts['impact']['tai file']+ " does not exist")
        else:
            if self.opts['impact']['metric'] not in pywst.common.problem.valid_metrics:
                raise Exception("Error: invalid impact metric " + self.opts['impact']['metric'] + ".  Valid metrics = "+ ", ".join(pywst.common.problem.valid_metrics))
            elif self.opts['impact']['metric'] in ['DPD', 'DPE', 'DPK', 'PD', 'PE', 'PK']:
                if not os.path.exists(self.opts['impact']['tai file']):
                    raise RuntimeError("Error: " +self.opts['impact']['tai file']+ " does not exist\n")
        
        # dvf file
        if self.opts['impact']['dvf file'] not in pywst.common.problem.none_list and not os.path.exists(self.opts['impact']['dvf file']):
            raise RuntimeError("Error: " +self.opts['impact']['dvf file']+ " does not exist\n")
        
        # output prefix
        if self.opts['configure']['output prefix'] in pywst.common.problem.none_list:
            self.opts['configure']['output prefix'] = 'sim2Impact'

        return
        
    def run(self):
        logger.info("WST sim2Impact subcommand")
        logger.info("---------------------------")
        
        # set start time
        self.startTime = time.time()
        
        # validate input
        logger.info("Validating configuration file")
        self.validate()

        # create sim2Impact command line
        cmd = self.opts['configure']['sim2Impact executable']
        
        # required options
        if self.opts['impact']['metric'].__class__ is list:
            for s in self.opts['impact']['metric']:
                cmd = cmd + " --" + s.lower()
        else:
            cmd = cmd + " --" + self.opts['impact']['metric'].lower()
            
        # optional options
        useDVF = False
        if self.opts['impact']['metric'].__class__ is list:
            for s in self.opts['impact']['metric']:
                if s not in ['EC','TEC']:
                    useDVF = True
        else:
            if self.opts['impact']['metric'] not in ['EC','TEC']:
                useDVF = True
        if useDVF:
            if self.opts['impact']['dvf file'] not in pywst.common.problem.none_list:
                cmd = cmd + " --dvf=" + self.opts['impact']['dvf file']
        if self.opts['impact']['detection confidence'] not in pywst.common.problem.none_list:
            cmd = cmd + " --detectionConfidence=" + str(self.opts['impact']['detection confidence'])
        if self.opts['impact']['detection limit'].__class__ is list:
            for s in self.opts['impact']['detection limit']:
                cmd = cmd + " --detectionLimit=" + str(s)
        #if self.opts['impact']['detection limit'] not in pywst.common.problem.none_list:
        #   cmd = cmd + " --detectionLimit=" + str(self.opts['impact']['detection limit'])
        if self.opts['impact']['response time'] not in pywst.common.problem.none_list:
            cmd = cmd + " --responseTime=" + str(self.opts['impact']['response time'])
        if self.opts['impact']['msx species'] not in pywst.common.problem.none_list:
            cmd = cmd + " --species=" + str(self.opts['impact']['msx species'])
            
        # arguments
        cmd = cmd + " "+self.opts['configure']['output prefix']
        if self.opts['impact']['erd file'].__class__ is list:
            for s in self.opts['impact']['erd file']:
                cmd = cmd + " "+s
        else:
            cmd = cmd + " "+self.opts['impact']['erd file']
        if self.opts['impact']['metric'].__class__ is list:
            for s in self.opts['impact']['metric']:
                if s in ['DPD', 'DPE', 'DPK', 'PD', 'PE', 'PK']:
                    cmd = cmd + " " + self.opts['impact']['tai file']
                    break
        else:
            if self.opts['impact']['metric'] in ['DPD', 'DPE', 'DPK', 'PD', 'PE', 'PK']:
                cmd = cmd + " " + self.opts['impact']['tai file']
        
        # run sim2Impact
        logger.info("Computing impact assessment")
        logger.debug(cmd)
        sub_logger = logging.getLogger('wst.sim2Impact.exec')
        sub_logger.setLevel(logging.DEBUG)
        p = pyutilib.subprocess.run(cmd,stdout=pywst.common.problem.LoggingFile(sub_logger))
        if p[0]:
            message = 'An error occured when running the sim2Impact executable.\n Error Message: '+ p[1]+'\n Command: '+cmd+'\n'
            logger.error(message)
            raise RuntimeError(message)
        
        # remove temporary files if debug = 0
        if self.opts['configure']['debug'] == 0:
            pyutilib.services.TempfileManager.clear_tempfiles()
        
        # write output file  
        prefix = os.path.basename(self.opts['configure']['output prefix'])      
        logfilename = logger.parent.handlers[0].baseFilename
        outfilename = logger.parent.handlers[0].baseFilename.replace('.log','.yml')
        
        impact_files = []
        id_files = []
        prefix = os.path.basename(self.opts['configure']['output prefix'])
        for s in self.opts['impact']['metric']:
            impact_files.append(prefix+'_'+s.lower()+'.impact')
            id_files.append(prefix+'_'+s.lower()+'.id')
            
        config = wst_config.output_config()
        module_blocks = ("general", "sim2Impact")
        template_options = {
            'general': {
                'cpu time': round(time.time() - self.startTime,3),
                'directory': os.path.dirname(logfilename),
                'log file': os.path.basename(logfilename)},
            'sim2Impact': {
                'impact file': impact_files,
                'id file': id_files,
                'nodemap file': prefix+'.nodemap',
                'scenariomap file': prefix+'.scenariomap'}}
        self.saveOutput(outfilename, config, module_blocks, template_options)
        
        # print solution to screen
        logger.info("\nWST normal termination")
        logger.info("---------------------------")
        logger.info("Directory: "+os.path.dirname(logfilename))
        logger.info("Results file: "+os.path.basename(outfilename))
        logger.info("Log file: "+os.path.basename(logfilename)+'\n')
        
        return
