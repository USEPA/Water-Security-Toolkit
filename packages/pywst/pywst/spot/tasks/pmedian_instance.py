
import os
import sys
from os.path import abspath, dirname

import pywst.spot.core
from pyutilib.component.core import alias
from pyomo.scripting import util
from pyutilib.misc import Options


class pmedian_instance_from_data_Task(pywst.spot.core.Task):

    alias('pmedian_instance_from_data')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Task.__init__(self, *args, **kwds)
        #
        self.inputs.declare('dat_filename')
        self.inputs.declare('impact_data')
        #
        self.outputs.declare('pyomo_instance')

    def execute(self):
        pmedian_model = os.path.abspath(dirname(dirname(abspath(__file__)))+os.sep+"models"+os.sep+"pmedian.py")
        options=Options()
        if not self.dat_filename is None:
            util.apply_preprocessing(options, None, [pmedian_model, self.dat_filename])
            model = util.create_model(options, [pmedian_model, self.dat_filename])
            self.pyomo_instance = model.instance
        else:
            print "WARNING: it's not clear how to use a registered function for loading model data directly."    
            # Should util.create_model be extended ... ?


