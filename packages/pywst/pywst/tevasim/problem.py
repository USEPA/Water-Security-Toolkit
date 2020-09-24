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

import pywst.common.problem
import pywst.common.wst_config as wst_config
from pyutilib.misc.config import ConfigBlock
import pywst.common.wst_util as wst_util

logger = logging.getLogger('wst.tevasim')

class Problem(pywst.common.problem.Problem):
    
    def __init__(self):
        pywst.common.problem.Problem.__init__(self, 'tevasim', ("network", "scenario", "configure"))
        
    def _loadPreferences(self):
        for key in defLocs.keys():
            if key == 'tevasim':
                self.opts['configure']['tevasim executable'] = defLocs[key]

    def validate(self):
        # executables
        self._validateExecutable('tevasim')
        
        # epanet inp file
        if self.opts['network']['epanet file'] in pywst.common.problem.none_list:
            msg = "Error: tevasim requires an EPANET inp file"
            logger.error(msg)
            raise RuntimeError(msg) 
        elif not os.path.exists(self.opts['network']['epanet file']):
            msg = "Error: " +self.opts['network']['epanet file']+ " does not exist"
            logger.error(msg)
            raise RuntimeError(msg) 
        
        # erd compression method
        if self.opts['scenario']['erd compression'] in pywst.common.problem.none_list:
            pass
        elif self.opts['scenario']['erd compression'] not in ['RLE','rle','LZMA','lzma']:
            msg = "Error: tevasim requires a valid erd compression scheme. Choices are: (1) RLE, or (2) LZMA"
            logger.error(msg)
            raise RuntimeError(msg) 
        else:
            self.opts['scenario']['erd compression'] = self.opts['scenario']['erd compression'].lower()
        
        # scenario file
        if not self.opts['scenario']['tsi file'] in pywst.common.problem.none_list:
            if not os.path.exists(self.opts['scenario']['tsi file']):
                msg = "Error: " +self.opts['scenario']['tsi file']+ " does not exist"
                logger.error(msg)
                raise RuntimeError(msg) 
        elif not self.opts['scenario']['tsg file'] in pywst.common.problem.none_list:
            if not os.path.exists(self.opts['scenario']['tsg file']):
                msg = "Error: " +self.opts['scenario']['tsg file']+ " does not exist"
                logger.error(msg)
                raise RuntimeError(msg) 
        else:
            if self.opts['scenario']['location'] in pywst.common.problem.none_list:
                msg = "Error: Scenario location missing"
                logger.error(msg)
                raise RuntimeError(msg) 
            if self.opts['scenario']['type'] in pywst.common.problem.none_list:
                msg = "Error: Scenario type missing"
                logger.error(msg)
                raise RuntimeError(msg) 
            if self.opts['scenario']['strength'] in pywst.common.problem.none_list:
                msg = "Error: Scenario strength missing"
                logger.error(msg)
                raise RuntimeError(msg) 
            if self.opts['scenario']['start time'] in pywst.common.problem.none_list:
                msg = "Error: Scenario start time missing"
                logger.error(msg)
                raise RuntimeError(msg) 
            if self.opts['scenario']['end time'] in pywst.common.problem.none_list:
                msg = "Error: Scenario end time missing"
                logger.error(msg)
                raise RuntimeError(msg) 
            
        # dvf file
        if self.opts['scenario']['dvf file'] not in pywst.common.problem.none_list and not os.path.exists(self.opts['scenario']['dvf file']):
            msg = "Error: " +self.opts['scenario']['dvf file']+ " does not exist"
            logger.error(msg)
            raise RuntimeError(msg) 
        
        # msx file    
        if self.opts['scenario']['msx file'] not in pywst.common.problem.none_list and not os.path.exists(self.opts['scenario']['msx file']):
            msg = "Error: " +self.opts['scenario']['msx file']+ " does not exist"
            logger.error(msg)
            raise RuntimeError(msg) 
        
        # merlion option
        if self.opts['scenario']['merlion'] not in [True,False]:
            msg = "Error: invalid merlion option"
            logger.error(msg)
            raise RuntimeError(msg) 

        # merlion nsims option
        if not ((self.opts['scenario']['merlion nsims'].__class__ is int) and (self.opts['scenario']['merlion nsims'] >= 1)):
            msg = "Error: 'merlion nsims' option must be a positive integer"
            logger.error(msg)
            raise RuntimeError(msg) 
        
        # ignore merlion warnings
        if self.opts['scenario']['ignore merlion warnings'] not in [True,False]:
            msg = "Error: 'ignore merlion warnings' option must be True or False"
            logger.error(msg)
            raise RuntimeError(msg) 

        # output prefix
        if self.opts['configure']['output prefix'] in pywst.common.problem.none_list:
            self.opts['configure']['output prefix'] = 'tevasim'
        
        return
        
    def run(self,**kwds):

        logger.info("WST tevasim subcommand")
        logger.info("---------------------------")
        
        # set start time
        self.startTime = time.time()
        
        # validate input
        logger.info("Validating configuration file")
        self.validate()

        # create tevasim command line
        cmd = self.opts['configure']['tevasim executable']
         
        if self.opts['scenario']['tsi file'] not in pywst.common.problem.none_list:
            cmd = cmd + " --tsi="+self.opts['scenario']['tsi file']
        elif self.opts['scenario']['tsg file'] not in pywst.common.problem.none_list:
            cmd = cmd + " --tsg="+self.opts['scenario']['tsg file']
        else: # write tmp TSG file
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
            cmd = cmd + " --tsg="+self.opts['scenario']['tsg file']

        if self.opts['scenario']['dvf file'] not in pywst.common.problem.none_list:
            cmd = cmd + " --dvf="+self.opts['scenario']['dvf file']
        if self.opts['scenario']['msx file'] not in pywst.common.problem.none_list:
            cmd = cmd + " --msx="+self.opts['scenario']['msx file']
        if self.opts['scenario']['msx species'] not in pywst.common.problem.none_list:
            cmd = cmd + " --mss="+self.opts['scenario']['msx species']
        if self.opts['scenario']['merlion'] is True:
            cmd = cmd + " --merlion"
            cmd = cmd + " --merlion-nsims="+str(self.opts['scenario']['merlion nsims'])
            if self.opts['scenario']['ignore merlion warnings'] is True:
                cmd += " --merlion-ignore-warnings"

        if self.opts['scenario']['erd compression'] in ['rle','RLE']:
            cmd += " --rle"
        elif self.opts['scenario']['erd compression'] in pywst.common.problem.none_list:
            # To maintain backward compatibility with those using EPANET
            # we leave the erd compression scheme as lzma when no option is
            # specified. In the case that Merlion is used, we want to perform
            # simulations as fast as possible so we default to rle.
            if self.opts['scenario']['merlion']:
                cmd += " --rle"
            
        cmd = cmd + " "+self.opts['network']['epanet file']
        cmd = cmd + " "+self.opts['configure']['output prefix']+".rpt"
        cmd = cmd + " "+self.opts['configure']['output prefix']
        
        # run tevasim
        logger.info("Running contaminant transport simulations")
        logger.debug(cmd)
        sub_logger = logging.getLogger('wst.tevasim.exec')
        sub_logger.setLevel(logging.DEBUG)
        p = pyutilib.subprocess.run(cmd,stdout=pywst.common.problem.LoggingFile(sub_logger))
        if (p[0]):
            msg = 'An error occured when running the tevasim executable.\n Error Message: '+ p[1]+'\n Command: '+cmd+'\n'
            logger.error(msg)
            raise RuntimeError(msg)
        
        # remove temporary files if debug = 0
        if self.opts['configure']['debug'] == 0:
            pyutilib.services.TempfileManager.clear_tempfiles()
            if os.path.exists('./hydraulics.hyd'):
                os.remove('./hydraulics.hyd')
        
        # write output file 
        prefix = os.path.basename(self.opts['configure']['output prefix'])      
        logfilename = logger.parent.handlers[0].baseFilename
        outfilename = logger.parent.handlers[0].baseFilename.replace('.log','.yml')

        config = wst_config.output_config()
        module_blocks = ("general", "tevasim")
        template_options = {
            'general':{
                'cpu time': round(time.time() - self.startTime,3),
                'directory': os.path.dirname(logfilename),
                'log file': os.path.basename(logfilename)},
            'tevasim': {
                'report file': prefix+'.rpt',
                'header file': prefix+'.erd',
                'hydraulic file': prefix+'-1.hyd.erd',
                'water quality file': prefix+'-1.qual.erd',
                'index file': prefix+'.index.erd'}}
        if outfilename != None:
            self.saveOutput(outfilename, config, module_blocks, template_options)
        
        # print solution to screen
        logger.info("\nWST normal termination")
        logger.info("---------------------------")
        logger.info("Directory: "+os.path.dirname(logfilename))
        logger.info("Results file: "+os.path.basename(outfilename))
        logger.info("Log file: "+os.path.basename(logfilename)+'\n')
        
        return self.opts['configure']['output prefix']+'.erd'
