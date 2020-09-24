
__all__ = ['tevasim_Task']

import os
import pywst.spot.core
from pyutilib.component.core import alias
import pyutilib.workflow


class tevasim_Task(pywst.spot.core.Task):

    alias('run_tevasim')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Task.__init__(self, *args, **kwds)
        #
        self.inputs.declare('tsg_filename', optional=True)
        self.inputs.declare('tsi_filename', optional=True)
        self.inputs.declare('tso_filename', optional=True)
        self.inputs.declare('epanet_model')
        self.inputs.declare('epanet_output', optional=True)
        self.inputs.declare('tevasim_logfile', optional=True)
        #
        self.outputs.declare('epanet_output') # in/out
        self.outputs.declare('tso_filename')
        self.outputs.declare('sdx_filename')
        self.outputs.declare('tevasim_logfile') # in/out
        #
        self.add_resource( pyutilib.workflow.ExecutableResource('tevasim') )

    def execute(self):
        if self.tso_filename is None:
            self.tso_filename = pyutilib.services.TempfileManager.create_tempfile(suffix=".tso")
        self.sdx_filename = os.path.splitext(self.tso_filename)[0]+".sdx"
        if self.tsg_filename is None and self.tsi_filename is None:
            raise IOError, "tevasim_Task requires either a TSG filename or TSI filename"
        #
        args = ""
        if not self.tsg_filename is None:
            args += " --tsg %s" % self.tsg_filename
        if not self.tsi_filename is None:
            args += " --tsi %s" % self.tsi_filename
        if not self.tso_filename is None:
            args += " --tso %s" % self.tso_filename
        args += " " + self.epanet_model
        if not self.epanet_output is None:
            args += " " + self.epanet_output
        else:
            self.epanet_output = pyutilib.services.TempfileManager.create_tempfile(prefix="EPANET_log", suffix=".out")
            args += " " + self.epanet_output
        if self.tevasim_logfile is None:
            self.tevasim_logfile = pyutilib.services.TempfileManager.create_tempfile(prefix='tevasim_', suffix=".log")
        # Manage debugging in tasks
        self.resource('tevasim').run(args, logfile=self.tevasim_logfile, debug=True)


if __name__ == 'X__main__':
    os.environ['PATH'] += os.pathsep+'/home/wehart/src/spot/trunk/bin'
    task = tevasim_Task()
    import pyutilib.workflow
    wf = pyutilib.workflow.Workflow()
    wf.add(task)
    #
    output = wf(tsg_filename='foo.tsg', epanet_model='Net3.inp')
    print output
    pyutilib.services.TempfileManager.clear_tempfiles()
    #
    output = wf(tsg_filename='foo.tsg', epanet_model='Net3.inp', tso_filename='foo.tso')
    print output
    pyutilib.services.TempfileManager.clear_tempfiles()
    #
    output = wf(tsg_filename='foo.tsg', epanet_model='Net3.inp', epanet_output="Net3.out", tso_filename='foo.tso', logfile='foo.log')
    print output
    pyutilib.services.TempfileManager.clear_tempfiles()
    #
