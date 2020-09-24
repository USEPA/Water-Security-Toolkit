
import pywst.spot.core
from pyutilib.component.core import alias

class sptkHW_workflow(pywst.spot.core.Workflow):

    alias('pywst.spot.sptk.hw', doc='A "Hello World" task example')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Workflow.__init__(self, *args, **kwds)
        self.add( pywst.spot.core.TaskFactory('pywst.spot.hw') )


class sptkHW_task(pywst.spot.core.Task):

    alias('pywst.spot.hw', doc='A "Hello World" task example')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Task.__init__(self, *args, **kwds)
        #
        self.inputs.declare('input', optional=True)
        self.add_argument('--input', dest='input', default=None, help="An input value that is printed after HELLO WORLD")
        #
        #self.outputs.declare('out')

    def execute(self):
        print "HELLO WORLD" ,self.input
        #self.out = "DONE"

