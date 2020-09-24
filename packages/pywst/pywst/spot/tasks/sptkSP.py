
import pywst.spot.core
from pyutilib.component.core import alias
from pywst.spot.core import TaskFactory


class sptkSP_workflow(pywst.spot.core.Workflow):

    alias('pywst.spot.sptk.sp', doc='Sensor placement command')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Workflow.__init__(self, *args, **kwds)
        #
        self.create_pmedian_workflow()

    def create_pmedian_workflow(self):
        #
        T0 = TaskFactory('sptkSP.set_input')
        #
        #T1 = TaskFactory('run_tso2impact')
        #
        #T2_1 = TaskFactory('workflow.selection')
        #T2_1.inputs.data = T1.outputs.impact_data
        #T2_1.inputs.index.set_value(imp)
        #
        T2_2 = TaskFactory('parse_simple_impact')
        T2_2.inputs.impact_filename = T0.outputs.impact_filename
        #
        T2 = TaskFactory('impact2dat_pmedian')
        T2.inputs.impact_data = T2_2.outputs.impact_data
        T2.inputs.num_sensors = T0.outputs.num_sensors
        #
        T3 = TaskFactory('pmedian_instance_from_data')
        T3.inputs.dat_filename = T2.outputs.dat_filename
        #
        T4 = TaskFactory('pyomoInstance2results')
        T4.inputs.pyomo_instance = T3.outputs.pyomo_instance
        T4.inputs.solver = T0.outputs.solver
        #
        T5 = TaskFactory('pywst.spot.results2sensors')
        T5.inputs.results = T4.outputs.results
        #
        #T6 = TaskFactory('pywst.spot.run_evalsensor')
        #T6.inputs.tso_impact_data = T1.outputs.impact_data
        #T6.inputs.sensor_filename = T5.outputs.sensor_filename
        #
        # Create workflow
        #
        self.add(T0)

    def execute(self):
        print "Starting SP"
        pywst.spot.core.Workflow.execute(self)
        print "Finishing SP"


class sptkSP_task(pywst.spot.core.Task):

    alias('sptkSP.set_input')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Task.__init__(self, *args, **kwds)
        #
        self.inputs.declare('impact_filename', optional=True)
        self.inputs.declare('num_sensors') 
        self.inputs.declare('solver')
        
        self.add_argument('--impact_file', dest='impact_filename', default=None, help=" A file that contains the impact data concerning contamination incidents. ")
        self.add_argument('--num_sensors', dest='num_sensors', default=None, help="Number of sensors")
        self.add_argument('--solver', dest='solver', default=None, help="This option specifies the type of solver that is used to find sensor placement(s).  The following solver types are currently supported: AT&T multistart local search heuristic (att_grasp), TEVA-SPOT license-free grasp clone (snl_grasp), lagrangian relaxation heuristic solver (lagrangian), mixed-integer programming solver (pico), mixed-integer programming solver (glpk), MIP solver with AMPL (picoamp), commercial MIP solver (cplexamp). The default solver is snl_grasp.")
        
        self.outputs.declare('impact_filename')
        self.outputs.declare('num_sensors')
        self.outputs.declare('solver')

    def execute(self):
        self.outputs.impact_filename = self.inputs.impact_filename
        self.outputs.num_sensors = self.inputs.num_sensors
        self.outputs.solver = self.inputs.solver
