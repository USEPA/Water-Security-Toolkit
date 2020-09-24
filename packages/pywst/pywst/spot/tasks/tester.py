import os
from pywst.spot.core import TaskFactory
import pyutilib.workflow
import pyutilib.services

def create_pmedian_workflow(imp):
    #
    T1 = TaskFactory('run_tso2impact')
    #
    T2_1 = TaskFactory('workflow.selection')
    T2_1.inputs.data = T1.outputs.impact_data
    #print 'x',imp
    T2_1.inputs.index.set_value(imp)
    #T2_1.inputs.index.pprint()
    #
    T2_2 = TaskFactory('parse_simple_impact')
    T2_2.inputs.tso_impact_data = T2_1.outputs.selection
    #
    T2 = TaskFactory('impact2dat_pmedian')
    T2.inputs.impact_data = T2_2.outputs.impact_data
    #
    T3 = TaskFactory('pmedian_instance_from_data')
    T3.inputs.dat_filename = T2.outputs.dat_filename
    #
    T4 = TaskFactory('pyomoInstance2results')
    T4.inputs.instance = T3.outputs.pyomo_instance
    #
    T5 = TaskFactory('pywst.spot.results2sensors')
    T5.inputs.results = T4.outputs.results
    #
    T6 = TaskFactory('pywst.spot.run_evalsensor')
    T6.inputs.tso_impact_data = T1.outputs.impact_data
    T6.inputs.sensor_filename = T5.outputs.sensor_filename
    #
    # Create workflow
    #
    wf = pyutilib.workflow.Workflow()
    wf.add(T1)
    #
    return wf


if __name__ == '__main__':
    pyutilib.services.TempfileManager.tempdir = os.path.abspath(os.path.dirname(os.path.abspath(__file__))+os.sep+'tmp')
    os.environ['PATH'] += os.pathsep+'/home/wehart/src/spot/trunk/bin'

    imp='ec'
    wf = create_workflow(imp)
    #options, args = create_parser().parse_args()
    print wf.options()
    print str(wf)
    wf(tso_filename='foo.tso', num_sensors=2, impact_list=[imp], solver='glpk')

