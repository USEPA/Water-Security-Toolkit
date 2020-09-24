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
    T2_1.inputs.index.set_value(imp)
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


def create_impact_workflow(imp):
    #
    T1 = TaskFactory('run_tso2impact')
    #
    T2_1 = TaskFactory('workflow.selection')
    T2_1.inputs.data = T1.outputs.impact_data
    T2_1.inputs.index.set_value(imp)
    #
    T2_2 = TaskFactory('parse_simple_impact')
    T2_2.inputs.tso_impact_data = T2_1.outputs.selection
    #
    # Create workflow
    #
    wf = pyutilib.workflow.Workflow()
    wf.add(T1)
    #
    return wf



def create_tuning_workflow(imp, impact_data):
    #T1 = TaskFactory('parse_simple_impact')
    #T1.inputs.tso_impact_data = impact_data
    #
    T2 = TaskFactory('impact2dat_tuning')
    T2.inputs.impact_data = impact_data
    #
    T3 = TaskFactory('tuning_instance_from_data')
    T3.inputs.dat_filename = T2.outputs.dat_filename
    #
    T4 = TaskFactory('pyomoInstance2results')
    T4.inputs.instance = T3.outputs.pyomo_instance
    #
    T5 = TaskFactory('pywst.spot.results2sensors')
    T5.inputs.results = T4.outputs.results
    #
    T6 = TaskFactory('pywst.spot.tuning_summary')
    T6.inputs.results = T4.outputs.results
    T6.inputs.pyomo_instance = T3.outputs.pyomo_instance
    #
    # Create workflow
    #
    wf = pyutilib.workflow.Workflow()
    wf.add(T2)
    #
    return wf


def test1():
    pyutilib.services.TempfileManager.tempdir = os.path.abspath(os.path.dirname(os.path.abspath(__file__))+os.sep+'tmp')
    os.environ['PATH'] += os.pathsep+'/home/wehart/src/spot/trunk/bin'

    imp='ec'
    wf = create_pmedian_workflow(imp)
    #options, args = create_parser().parse_args()
    print wf.options()
    print str(wf)
    wf(tso_filename='foo.tso', num_sensors=2, impact_list=[imp], solver='glpk')

def test2():
    pyutilib.services.TempfileManager.tempdir = os.path.abspath(os.path.dirname(os.path.abspath(__file__))+os.sep+'tmp')
    #os.environ['PATH'] += os.pathsep+'/home/wehart/src/spot/trunk/bin'

    imp='mc'
    wf = create_impact_workflow(imp)
    print wf.options()
    print str(wf)
    ans = {}
    #print wf(tso_filename='foo.tso', impact_list=[imp], responseTime=0.0)['impact_data'].keys()
    ans[1] = wf(tso_filename='foo.tso', impact_list=[imp], responseTime=0.0)['impact_data']
    #print ans[1].keys()
    ans[2] = wf(tso_filename='foo.tso', impact_list=[imp], responseTime=30.0)['impact_data']
    ans[3] = wf(tso_filename='foo.tso', impact_list=[imp], responseTime=60.0)['impact_data']
    #
    fp = {}
    fp['low'] = 0.01
    fp['medium'] = 0.025
    fp['high'] = 0.05
    #
    wf = create_tuning_workflow(imp,ans)
    wf(impact_data=ans, num_sensors=2, num_levels=3, solver='glpk')
    


if __name__ == '__main__':
    test2()

