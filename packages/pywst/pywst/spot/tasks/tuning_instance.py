
import os
import sys
from os.path import abspath, dirname

import pywst.spot.core
from pyutilib.component.core import alias
from pyomo.scripting import util
from pyutilib.misc import Options


class tuning_instance_from_data_Task(pywst.spot.core.Task):

    alias('tuning_instance_from_data')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Task.__init__(self, *args, **kwds)
        #
        self.inputs.declare('dat_filename')
        self.inputs.declare('impact_data')
        #
        self.outputs.declare('pyomo_instance')

    def execute(self):
        tuning_model = os.path.abspath(dirname(dirname(abspath(__file__)))+os.sep+"models"+os.sep+"tuning.py")
        options=Options()
        #pyomo.core.set_debugging('all')
        print "X1"
        if not self.dat_filename is None:
            util.apply_preprocessing(options, None, [tuning_model, self.dat_filename])
            print "X2"
            model = util.create_model(options, [tuning_model, self.dat_filename])
            print "X3"
            self.pyomo_instance = model.instance
            print "X4"
        else:
            print "WARNING: it's not clear how to use a registered function for loading model data directly."    
            # Should util.create_model be extended ... ?


class tuning_summary_Task(pywst.spot.core.Task):

    alias('pywst.spot.tuning_summary')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Task.__init__(self, *args, **kwds)
        #
        self.inputs.declare('results')
        self.inputs.declare('pyomo_instance')

    def execute(self):
        print self.results
        print ""
        for i in range(0,len(self.results.solution)):
            vars = self.results.solution[i].variable
            ctr = 0
            for id in vars.y:
                if vars.y[id].value == 1 and not '_1' in id:
                    ctr += 1
            print "CTR",ctr
            print "P",self.pyomo_instance.P()
            print "FAR",vars.far.value
            print "IMPACT", self.results.solution[i].objective.f.value
            for id in vars.y:
                if vars.y[id].value == 1 and not '_1' in id:
                    print "Y",id
 
