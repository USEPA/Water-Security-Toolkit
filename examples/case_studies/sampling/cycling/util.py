import yaml
import sys
import copy
import pyutilib.subprocess
import pyutilib
import os
import math as pymath
import numpy as np
import pickle as pk

def write_template(args):
    filename = args.filename
    inputs = dict()
    inputs['true scenario'] = 'PATH' 
    inputs['set scenarios'] = 'PATH'
    inputs['measurements'] = 'PATH'
    inputs['start time'] = 'time in min'
    inputs['num cycles'] = 1
    inputs['delta t'] = 'time in min'
    inputs['confidence nodes'] = 0.95
    inputs['confidence scenarios'] = 0.95
    inputs['number of samples'] = 1
    inputs['threshold'] = 0.01
    inputs['node metric map'] = None
    inputs['filer scenarios'] = True
    inputs['with weights'] = False
    inputs['sample criteria'] = 'distinguish'
    inputs['mip solver'] = 'glpk'
    inputs['node to metric'] = None
    with open(filename, 'w') as yaml_file:
        yaml.dump(inputs,yaml_file,default_flow_style=False)

    
def read_template(args):
    filename = args.filename
    inputs = dict()
    with open(filename, 'r') as stream:
        try:
            inputs = yaml.load(stream)
        except yaml.YAMLError as exc:
            print(exc)
    return inputs

def read_sim_inputs(args):
    inputs = read_template(args)
    inputs['only sim'] = True
    return inputs

def read_sample_inputs(args):
    inputs = read_template(args)
    inputs['only sample'] = True
    return inputs

def translate_predefined_locations(list_locations,data_manager):
    locations = []
    if isinstance(eval(list_locations[0]),tuple):
        list_locations = [eval(l) for l in list_locations]
        for tuple_l in list_locations:
            idx_tuple = []
            for l in tuple_l:
                idx = data_manager.node_name_to_idx[l]
                idx_tuple.append(idx)
            locations.append(tuple(idx_tuple))
    else:
        for l in list_locations:
            idx = data_manager.node_name_to_idx[l]
            locations.append((idx,))
    return locations


def write_grabsample_input(filename,options):
    inp = options.pop('inp',None)
    tsg = options.pop('tsg',None)
    signals_folder = options.pop('signals',None)
    signals_ids = options.pop('list scenario ids',None)
    merlion = options.pop('merlion',False)
    s_time = options.pop('time',None)
    threshold = options.pop('threshold',1e-2)
    greedy = options.pop('greedy',True)
    n_samples = options.pop('num samples',1)
    weights = options.pop('weights',False)
    solver = options.pop('solver','gurobi_ampl')
    prefix = options.pop('prefix','')
    template_options = {
        'network': {'epanet file': inp},
        'scenario': {'tsg file':tsg,
                     'signals':signals_folder,
                     'merlion':merlion},
        'grabsample':{'sample time':s_time,
                      'model format':'PYOMO',
                      'threshold':threshold,
                      'greedy selection':greedy,
                      'num samples':n_samples,
                      'list scenario ids':signals_ids,
                      'with weights':weights},
        'solver': {'type': solver},
        'configure': {'output prefix': prefix}
    }
    yaml_file = open(filename,'w')
    yaml.dump(template_options,yaml_file,default_flow_style=False)
    yaml_file.close()
    return prefix

def declare_tempfile(filename):
    abs_fname = os.path.join(os.path.abspath(os.curdir), filename)
    pyutilib.services.TempfileManager.add_tempfile(filename, exists=False)
    return abs_fname

def remove_tempfiles():
    pyutilib.services.TempfileManager.clear_tempfiles()

def gaussian_peak(x,alpha,beta,gamma):
    """
    helper function to generate absorption data based on 
    lorentzian parameters
    """
    return alpha*pymath.exp(-(x-beta)**2/gamma)

def compute_weights_from_Pn(probabilities,alpha,gamma,confidence):
    l_tail = (1.0-confidence)*0.5
    r_tail = 1.0-l_tail
    node_to_weight = dict()
    for n,p in enumerate(probabilities):
        if n>0:
            node_to_weight[n] = 1.0 + gaussian_peak(p,alpha,l_tail,gamma) + gaussian_peak(p,alpha,r_tail,gamma)

    return node_to_weight 


def prune_scenarios(list_scenarios,confidence):

    probabilities = np.array([s.probability for s in list_scenarios])

    indices = np.argsort(probabilities)
    sorted_indices = indices[::-1]
    sorted_probabilities = probabilities[sorted_indices]

    if sorted_probabilities[0]>sorted_probabilities[-1]:
        accum = np.cumsum(sorted_probabilities,dtype=float)
        in_confidence_interval = np.where(accum<=confidence)[0]
        if accum[0]>confidence:
            new_list_scenarios = [list_scenarios[sorted_indices[0]]]
            new_list_scenarios[0].probability = 1.000
        else:
            last_element = in_confidence_interval[-1]
            inv_conf = 1.0/accum[last_element]
            indices_to_keep = set([sorted_indices[i] for i in in_confidence_interval])
            new_list_scenarios = list()
            for j,s in enumerate(list_scenarios):
                if j in indices_to_keep:
                    new_list_scenarios.append(s)
            for s in new_list_scenarios:
                s.probability*=inv_conf
        return new_list_scenarios
    else:
        # does not prune becase all scenarios have the same probability
        return list_scenarios    

def get_scenarios_in_confidence(list_scenarios,confidence):
    probabilities = np.array([s.probability for s in list_scenarios])

    indices = np.argsort(probabilities)
    sorted_indices = indices[::-1]
    sorted_probabilities = probabilities[sorted_indices]
    accum = np.cumsum(sorted_probabilities,dtype=float)
    in_confidence_interval = np.where(accum<confidence)[0]
    print in_confidence_interval
    if sorted_probabilities[0]>confidence:
        new_list_scenarios = [list_scenarios[sorted_indices[0]]]
        return new_list_scenarios
    
    indices_to_keep = set([sorted_indices[i] for i in in_confidence_interval])
    new_list_scenarios = list()
    for j,s in enumerate(list_scenarios):
        if j in indices_to_keep:
            new_list_scenarios.append(s)
    return new_list_scenarios

def count_scenarios_in_confidence(list_scenarios,confidence,sorted=False):
        probabilities = np.array([s.probability for s in list_scenarios])
        if not sorted:
                increasing_order = np.sort(probabilities)
                probs = increasing_order[::-1]
        else:
                probs = probabilities
        accum = np.cumsum(probs,dtype=float)
        in_confidence_interval = np.where(accum<=confidence)[0]
        n_in_interval = len(in_confidence_interval)
        return  n_in_interval if n_in_interval!=0 else 1 

def pickle_scenarios_jeff(list_scenarios,data_manager):


    d_max = 10*data_manager.times[-1]
    times = sorted(data_manager.times)
    nodeids = data_manager.idx_to_node_name.keys()
    for s in list_scenarios:
        s.nid_to_detection_time = dict()
        for nid in nodeids:
            for t in times:
                if nid in s.time_to_nodes[t]:
                    s.nid_to_detection_time[nid] = float(t)
                    break

    
    jeff_data = dict()
    jeff_data['times'] = data_manager.times
    jeff_data['scenarios'] = dict()
    for i,s in enumerate(list_scenarios):
        jeff_data['scenarios'][i] = dict()
        jeff_data['scenarios'][i]['alpha'] = s.probability
        jeff_data['scenarios'][i]['d_max'] = float(d_max)
        jeff_data['scenarios'][i]['d'] = s.nid_to_detection_time
    jeff_data['average'] = sum([len(s.nid_to_detection_time) for s in list_scenarios])/float(len(list_scenarios))
    jeff_data['sources'] = [s._injection.source for s in list_scenarios]
    jeff_data['idx_to_node_name'] = data_manager.idx_to_node_name
    jeff_data['node_name_to_idx'] = data_manager.node_name_to_idx
    with open('jeff_data.yml', 'w') as yaml_file:
        yaml.dump(jeff_data,yaml_file,default_flow_style=False)


def get_earliest_meas_time(filename):
    meas_times = list()
    EMPTY_LINE = ['\n', '\t\n', ' \n', '']
    try:
        f = open(filename, 'r')
        for line in f:
            if line not in EMPTY_LINE:
                l = line.split()
                t = int(int(l[1]) / 60.0)
                meas_times.append(t)
    except IOError:
        print "ERROR reading measurement file"

    ordered_times = sorted(meas_times)
    if ordered_times:
        return ordered_times[0]
    else:
        return -1
