
import pywst.spot.core
from pyutilib.component.core import alias
from pyomo.scripting import util
from pyutilib.misc import Options


class pyomoInstance2results_Task(pywst.spot.core.Task):

    alias('pyomoInstance2results')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Task.__init__(self, *args, **kwds)
        #
        self.inputs.declare('pyomo_instance')
        self.inputs.declare('solver')
        self.inputs.declare('solver_options')
        #
        self.outputs.declare('results')

    def execute(self):
        options = Options()
        options.keepfiles=True
        options.solver = self.solver
        options.solver_options = self.solver_options
        self.results, opt = util.apply_optimizer(options, self.instance)
        print self.results
        #print "HERE",type(self.results), self.results
        #print "HERE",type(opt),opt

