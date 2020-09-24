
__all__ = ['tso2Impact_Task']

import os
import pywst.spot.core
from pyutilib.component.core import alias
import pyutilib.workflow
from pyutilib.misc import Container


class tso2Impact_Task(pywst.spot.core.Task):

    alias('run_tso2impact')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Task.__init__(self, *args, **kwds)
        #
        self.inputs.declare('impact_list')
        self.inputs.declare('detectionLimit', optional=True)
        self.inputs.declare('responseTime', optional=True)
        self.inputs.declare('output_prefix', optional=True)
        self.inputs.declare('tso_filename')
        self.inputs.declare('tai_filename', optional=True)
        self.inputs.declare('logfile', optional=True)
        #
        self.outputs.declare('impact_data')
        self.outputs.declare('logfile') # in/out
        #
        self.add_resource( pyutilib.workflow.ExecutableResource('tso2Impact') )

    def execute(self):
        if len(self.impact_list) is 0:
            raise IOError, "tso2Impact_Task expected non-zero list for input 'impact_list'"
        #
        args = ""
        for impact in self.impact_list:
            args += " --%s" % str(impact)
        if not self.detectionLimit is None:
            args += " --detectionLimit=%s" % str(self.detectionLimit)
        if not self.responseTime is None:
            args += " --responseTime=%s" % str(self.responseTime)
        if self.output_prefix is None:
            impact_tempfiles=True
            self.output_prefix = pyutilib.services.TempfileManager.create_tempfile(prefix='tso2Impact_')
        else:
            impact_tempfiles=False
        args += " %s" % str(self.output_prefix)
        args += " %s" % str(self.tso_filename)
        if not self.tai_filename is None:
            args += " %s" % str(self.tai_filename)
        #
        if self.logfile is None:
            self.logfile = pyutilib.services.TempfileManager.create_tempfile(suffix=".log")
        # TODO: manage debugging in tasks
        self.resource('tso2Impact').run(args, logfile=self.logfile, debug=True)
        #
        self.impact_data = {}
        for impact in self.impact_list:
            data = Container(type=str(impact), impact="%s_%s.impact" % (self.output_prefix, impact), id="%s_%s.impact.id" % (self.output_prefix, impact), nodemap="%s.nodemap" % self.output_prefix, scenariomap="%s.scenariomap" % self.output_prefix)
            pyutilib.services.TempfileManager.add_tempfile(data.impact)
            pyutilib.services.TempfileManager.add_tempfile(data.id)
            pyutilib.services.TempfileManager.add_tempfile(data.nodemap)
            pyutilib.services.TempfileManager.add_tempfile(data.scenariomap)
            self.impact_data[impact] = data


class tso2Impact_SPTK(tso2Impact_Task):

    alias('pywst.spot.sptk.tso2Impact', doc='Run tso2Impact')

    def __init__(self, *args, **kwds):
        tso2Impact_Task.__init__(self, *args, **kwds)
        self.add_argument('--tso_file', dest='tso_filename', default=[], help="This argument indicates either a TSO file or a directory name for TSO files.  If the later, then the filenames must be specified with the --tsoPattern option.")
        self.add_argument('--impact', dest='impact_list', default=[], action='append', help="Specify an impact value.  This option can be specified multiple times to define a list of impact values.  Options include detected extent of contamination (dec), detected mass consumed (dmc), detected population dosed (dpd), detected population exposed (dpe), detected population killed (dpk), detected time-to-detection (dtd), detected volume consumed (dvc), extent of contamination (ec), mass consumed (mc), number-of-failed detections (nfd), population dosed (pd), population exposed (pe), population killed (pk), time-to-detection (td), timed extent of contamination (tec), volume consumed (vc).")
        self.add_argument('--detectionLimit', dest='detectionLimit', default=None, help="If this is specified, then only concentration values above this limit are archived.")
        self.add_argument('--responseTime', dest='responseTime', default=None, help="The number of minutes that are needed to respond to the detection of a contaminant.")


if __name__ == '__main__':
    os.environ['PATH'] += os.pathsep+'/home/wehart/src/spot/trunk/bin'
    task = tso2Impact_Task()
    import pyutilib.workflow
    wf = pyutilib.workflow.Workflow()
    wf.add(task)
    #
    output = wf(impact_list=['mc', 'ec'], output_prefix="bar", tso_filename='foo.tso', logfile='foo.log')
    print output
    pyutilib.services.TempfileManager.clear_tempfiles()
    #
    output = wf(detectionLimit=1000.0, impact_list=['mc', 'ec'], output_prefix="bar", tso_filename='foo.tso', logfile='foo.log')
    print output
    pyutilib.services.TempfileManager.clear_tempfiles()
    
    output = wf(responseTime=360.0, impact_list=['mc', 'ec'], output_prefix="bar", tso_filename='foo.tso', logfile='foo.log')
    print output
    #pyutilib.services.TempfileManager.clear_tempfiles()
    #
