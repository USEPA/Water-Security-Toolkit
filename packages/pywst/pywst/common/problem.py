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
import yaml
import wst_config
from wst_config import none_list
from wst_config import valid_metrics
import wst_util
from pyutilib.misc.config import ConfigBlock
import logging

logging.getLogger('wst')

class LoggingFile(object):
    # direct logger to file
    def __init__(self, logger):
        self.logger = logger
    def write(self, x):
        self.logger.debug(x.rstrip())
    def flush(self):
        pass
    
class Problem(object):
    
    def __init__(self, name="unknown", blocks=()):
        self.module = name
        self.module_blocks = blocks
        self.filename = None
        self.opts = wst_config.master_config()
        self.defLocs = {}
        self.loadPreferencesFile()
        self._logger_fh = None
        self._logger_ch = None

    def loadPreferencesFile(self):
        if os.name in ['nt','win','win32','win64','dos']:
            rcPath = os.path.join(os.path.abspath(os.environ['APPDATA']), '.wstrc')
        else:
            rcPath = os.path.join(os.path.abspath(os.environ['HOME']), '.wstrc')
        if os.path.exists(rcPath) and os.path.isfile(rcPath):
            fid = open(rcPath,'r')
            defLocs = yaml.load(fid)
            fid.close()
            self.defLocs = defLocs
            self._loadCustomPreferences()

    def _loadCustomPreferences(self):
        """Customized by a subclass to load module-specific preferences"""
        """Customized by a subclass to load module-specific preferences"""
        pass

    def _get_prefixed_filename(self, name, tempfile=False):
        new_name = None
        prefix = self.opts['configure']['output prefix']
        if prefix is None:
            new_name = name
        else:
            new_name = prefix+name
        if tempfile:
            wst_util.declare_tempfile(new_name)
        return new_name

    def _get_tempfile(self, suffix):

        prefix = self.opts['configure']['output prefix']

        return wst_util.get_tempfile(prefix, suffix)

    def load(self, filename):
        if filename is None: return False
        try:
            fid = open(filename,'r')
            options = yaml.load(fid)
            self.opts['__config_file_location__'] = os.path.dirname(filename)
            self.opts.set_value(options)
        except Exception, err:
            print 'Error reading "%s": %s' % (filename, str(err))
            raise
        finally:
            fid.close()
        return True
        
    def setLogger(self, module=None):
        self.resetLogger()
        logger = logging.getLogger('wst')
        logger.setLevel(logging.DEBUG)
        # create file handler
        # write debug/info/warning/error/critical to the file
        logfile = self._get_prefixed_filename(module+'_output.log')
        fh = logging.FileHandler(logfile, mode='w')
        fh.setLevel(logging.DEBUG)
        # create console handler 
        # direct info/warning/error/critical to the screen
        ch = logging.StreamHandler(sys.stdout)
        if self.opts['configure']['debug']:
            ch.setLevel(logging.DEBUG)
        else:
            ch.setLevel(logging.INFO)
        # create formatter and add it to the handlers
        formatter = logging.Formatter('%(name)s - %(levelname)s - %(message)s')
        fh.setFormatter(formatter)
        formatter = logging.Formatter('%(message)s')
        ch.setFormatter(formatter)

        # add the handlers to the logger
        self._logger_fh = fh
        self._logger_ch = ch
        logger.addHandler(fh)
        logger.addHandler(ch)

    def resetLogger(self, module=None):
        '''
        Remove any existing handlers
        '''
        logger = logging.getLogger('wst')
        if self._logger_fh is not None:
            self._logger_fh.flush()
            self._logger_fh.close()
            logger.removeHandler(self._logger_fh)
            self._logger_fh = None
        if self._logger_ch is not None:
            self._logger_ch.flush()
            logger.removeHandler(self._logger_ch)
            self._logger_ch = None

    def saveTemplate(self, filename=None, template_options=None):
        '''
        Save the project as a YAML formatted file. Uses the filename stored in
        opts['configure']['output prefix'], use setConfigureOption('output prefix',f)
        to set this name.
        '''
        if filename is None:
            filename = self.opts['configure']['output prefix']+'.yml'
            
        from pywst.common.wst_config import Path
        _SPE = True
        Path.SuppressPathExpansion, _SPE = _SPE, Path.SuppressPathExpansion
        try:
            fid = open(filename,'wt')
            templ = ConfigBlock("%s configuration template" % self.module)
            for k in self.module_blocks:
                templ.declare( k, self.opts[k]() )
            if template_options is not None:
                templ.set_value(template_options)
            fid.write(templ.generate_yaml_template(indent_spacing=2, width=95))
            fid.close()
        except:
            print 'Error saving "%s"'%filename
            raise
        finally:
            Path.SuppressPathExpansion, _SPE = _SPE, Path.SuppressPathExpansion
            
        print "Writing template file " + filename
        return True
        
    def saveAll(self, filename=None):
        if filename is None:
            filename = self.opts['configure']['output prefix']+'.yml'
        try:
            fid = open(filename,'wt')
            fid.write('# YML template: %s\n' % self.module)
            fid.write('\n')
            fid.write(yaml.dump(self.opts.value(),default_flow_style=False))
            fid.close()
        except:
            print 'Error saving "%s"'%filename
            return False
        return True   
    
    def saveOutput(self, outfilename, config, module_blocks, template_options):
        fid = open(outfilename,'wt')
        templ = ConfigBlock("%s output" % self.module)
        for k in module_blocks:
            templ.declare( k, config[k]() )
        if template_options is not None:
            templ.set_value(template_options)
        if self.opts['configure']['debug']:
            fid.write('---\n')
        fid.write(templ.generate_yaml_template(indent_spacing=2, width=95))
        if self.opts['configure']['debug']:
            fid.write('---\n')
            fid.write("#%s input\n" % self.module)
            out = self.opts.display(content_filter='userdata')
            fid.write(out)
        fid.close()
        
    def saveVisOutput(self, outfilename, config, module_blocks, template_options):
        fid = open(outfilename,'wt')
        templ = ConfigBlock("# YML input file for visualization")
        for k in module_blocks:
            templ.declare( k, config[k]() )
        if template_options is not None:
            templ.set_value(template_options)
        fid.write(templ.generate_yaml_template(indent_spacing=2, width=95))
        fid.close()
    
    def _validateExecutable(self, keyname):
        filenameExe = self.opts['configure'][keyname+' executable']
        if not os.path.exists(filenameExe) and not os.path.exists(filenameExe+'.exe'):
            if keyname in self.defLocs.keys():
                filenameExe = self.defLocs[keyname]
            else:
                filenameExe = os.path.split(filenameExe)[1]
                for p in os.sys.path:
                    f = os.path.join(p,filenameExe)
                    if os.path.exists(f) and os.path.isfile(f):
                        filenameExe = f
                        break
                    f = os.path.join(p,filenameExe+'.exe')
                    if os.path.exists(f) and os.path.isfile(f):
                        filenameExe = f
                        break
        self.opts['configure'][keyname+' executable'] = filenameExe

    def validate(self):
        """Perform validation of configuration options"""
        pass
        
    def run(self,**kwds):
        """Execute the solvers"""
        pass

