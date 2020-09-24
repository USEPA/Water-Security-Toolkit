
import os
import pywst.spot.core
from pyutilib.component.core import alias
import pyutilib.workflow


class evalsensor_Task(pywst.spot.core.Task):

    alias('pywst.spot.run_evalsensor')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Task.__init__(self, *args, **kwds)
        #
        #self.inputs.declare('tso_impact_data', optional=True)
        self.inputs.declare('impact_filename')
        self.inputs.declare('nodemap_filename', optional=True)
        self.inputs.declare('debug', optional=True)
        self.inputs.declare('format', optional=True)
        self.inputs.declare('response_time', optional=True)
        self.inputs.declare('sensor_filename', optional=True)
        self.inputs.declare('evalsensor_logfile', optional=True)
        #
        self.outputs.declare('evalsensor_logfile') # in/out
        #
        self.add_resource( pyutilib.workflow.ExecutableResource('evalsensor') )

    def execute(self):
        #
        args = ""
        if not self.debug is None and self.debug:
            args += " --debug"
        if not self.format is None:
            args += " --format %s" % self.format
        if not self.response_time is None:
            args += " --responseTime %s" % self.response_time
        #if not self.nodemap is None:
        #    args += " --nodemap %s" % self.nodemap
        #args += " --nodemap %s" % self.tso_impact_data[self.tso_impact_data.keys()[0]].nodemap

        if len(self.sensor_filename) > 0:
            args += " %s" % self.sensor_filename
        #for imp in self.tso_impact_data:
        #    args += " "+self.tso_impact_data[imp].impact
        args += " "+self.impact_filename
        if self.evalsensor_logfile is None:
            self.evalsensor_logfile = pyutilib.services.TempfileManager.create_tempfile(prefix='evalsensor_', suffix=".log")
        # Manage debugging in tasks
        self.resource('evalsensor').run(args, logfile=self.evalsensor_logfile, debug=True)

class evalsensor_SPTK(evalsensor_Task):
    
    alias('pywst.spot.sptk.evalsensor', doc='Run evalsensor')

    def __init__(self, *args, **kwds):
        evalsensor_Task.__init__(self, *args, **kwds)
        self.add_argument('--impact_file', dest='impact_filename', default=[], help="A file that contains the impact data concerning contamination incidents.  If one or more impact files are specified, then evaluations are performed for each impact separately.")
        self.add_argument('--sensor_file', dest='sensor_filename', default=[], help="A sensor placement file, which contains one or more sensor placements that will be evaluated.  If 'none' is specified, then evalsensor will evaluate impacts with no sensors.")
        
        
