import copy
import numpy as np
from operator import itemgetter
from pywst.common.mpi_util import *

def build_nodes_contamination_scenarios_sets(scenarios_container,nodes,times):
	node_to_sn_set = dict()
	for nodeid in nodes:
		node_to_sn_set[nodeid] = set()
		for i,scenario in enumerate(scenarios_container):
			for time in times:
				if scenario.is_contaminated(nodeid,time):
					node_to_sn_set[nodeid].add(i)
					break

	return node_to_sn_set

def build_source_scenarios_sets(scenarios_container,nodes):
	node_to_sn_set = dict()
	for nodeid in nodes:
		node_to_sn_set[nodeid] = set()
		for i,scenario in enumerate(scenarios_container):
                        if scenario._injection.source == nodeid:
                                node_to_sn_set[nodeid].add(i)
                                break

	return node_to_sn_set

def compute_node_probabilities(scenarios_probabilities,nodes_to_sn_sets):
	n_nodes = len(nodes_to_sn_sets)
	nodeid_to_probability = np.zeros(n_nodes+1)
	for nodeid,scenario_idxs in nodes_to_sn_sets.iteritems():
		accumulate = 0.0
		for idx in scenario_idxs:
			accumulate+=scenarios_probabilities[idx]
		nodeid_to_probability[nodeid] = accumulate
	return nodeid_to_probability

def clasify_nodes_true_scenario(true_scenario,time_min,data_manager,named=True):
        color_nodes = dict()
        color_nodes['yellow'] = list()
        idx_to_name = data_manager.get_node_map_idx_to_name()
        nodes = set(idx_to_name.keys())
        contaminated = true_scenario.get_contaminated_nodes_at_time(time_min)
        if named:
                color_nodes['red'] = [(idx_to_name[nid],1.0) for nid in contaminated]
                color_nodes['green'] = [(idx_to_name[nid],0.0) for nid in nodes.difference(contaminated)]
        else:
                color_nodes['red'] = list(contaminated)
                color_nodes['green'] = list(nodes.difference(contaminated))
        return color_nodes

def clasify_nodes(node_probabilities,confidence,data_manager=None):
        color_nodes = dict()
        color_nodes['green'] = list()
        color_nodes['yellow'] = list()
        color_nodes['red'] = list()
        lt = (1-confidence)*0.5
        rt = 1-lt
        confidence_interval = (lt,rt)
        for i,p in enumerate(node_probabilities):
	        if i>0:
                        if data_manager:
                                name = data_manager.idx_to_node_name[i]
                        else:
                                name = i
	                if p>=confidence_interval[1]:
		                color_nodes['red'].append((name,p))
	                elif p<=confidence_interval[0]:
		                color_nodes['green'].append((name,p))
	                else:
		                color_nodes['yellow'].append((name,p))
        return color_nodes

def compute_percentage_true_plume(node_probabilities,true_scenario,time_min,confidence=0.90):
        colored_nodes = clasify_nodes(node_probabilities,confidence)
        n_nodes = len(node_probabilities)-1
        likely_clean = set([i for i,p in colored_nodes['green']])
        likely_dirt = set([i for i,p in colored_nodes['red']])
        true_dirt = true_scenario.get_contaminated_nodes_at_time(time_min)
        overlap_dirt = len(true_dirt.intersection(likely_dirt))
        overlap_clean = len(likely_clean.difference(true_dirt))
        return (overlap_dirt+overlap_clean)/float(n_nodes)
        
def compute_Nu(node_probabilities,confidence):
        lt = (1.0-confidence)*0.5
        rt = 1.0-lt
        probs = node_probabilities
        in_plume = len(probs[probs>=rt])
        not_in_plume = len(probs[probs<=lt])
        return len(probs)-in_plume-not_in_plume

def count_number_of_matches(scenario,measurements):
        counter = 0
        for t, measures in measurements['positive'].iteritems():
		for nid in measures:
			if scenario.is_contaminated(nid,t):
				counter+=1
                                        
	for t, measures in measurements['negative'].iteritems():
		for nid in measures:
			if not scenario.is_contaminated(nid,t):
				counter+=1
	return counter

def bayesian_update_single_measurement(prior_probabilities, scenarios_container, binary_measurement,nid,t,failure_probability):
	pf = failure_probability
	pm = 1-pf

	p_s_given_m = np.array([pm if scenario.is_contaminated(nid,t)==binary_measurement else pf for scenario in scenarios_container])
	prior_probabilities *= p_s_given_m
	reciprocal = 1.0/sum(prior_probabilities)
	prior_probabilities *= reciprocal
	
def bayesian_update(scenarios_container,measurements, failure_probability):
        
	prior_probabilities = np.array([scenario.probability for scenario in scenarios_container])
	for t,node_ids in measurements['positive'].iteritems():
		for nid in node_ids:
			bayesian_update_single_measurement(prior_probabilities, scenarios_container,1,nid,t, failure_probability)

	for t,node_ids in measurements['negative'].iteritems():
		for nid in node_ids:
			bayesian_update_single_measurement(prior_probabilities, scenarios_container,0,nid,t,failure_probability)


        for i,scenario in enumerate(scenarios_container):
                scenario.probability = prior_probabilities[i]

	return prior_probabilities

def unique_measurements(measurements,checked_measurements):
        meas_to_check = dict()
        meas_to_check['positive'] = dict()
        meas_to_check['negative'] = dict()

        for type_meas in measurements.iterkeys():
                checked_times = set()
                for t,nids in measurements[type_meas].iteritems():
                        for nid in nids:
                                if (t,nid) not in checked_measurements:
                                        if t in checked_times:
                                                meas_to_check[type_meas][t].add(nid)
                                        else:
                                                meas_to_check[type_meas][t] = set()
                                                meas_to_check[type_meas][t].add(nid)
                                                checked_times.add(t)
        return meas_to_check

def mark_checked_measurements(measurements, checked_measurements):
        for type_meas in measurements.iterkeys():
                for t,nids in measurements[type_meas].iteritems():
                        for nid in nids:
                                checked_measurements.add((t,nid))

def update_scenario_matches(scenarios_container,measurements,append=True):
        
	n_measurements = 0
	for vals in measurements.itervalues():
		for nodes in vals.itervalues():
			n_measurements+=len(nodes)

	for scenario in scenarios_container:
		n_matches = count_number_of_matches(scenario,measurements)
		n_unmatches = n_measurements-n_matches
                if append:
		        scenario.matches+=n_matches
		        scenario.miss_matches+=n_unmatches
                else:
                        scenario.matches=n_matches
		        scenario.miss_matches=n_unmatches
                        
def update_scenarios_probability(scenario_probabilities,scenarios_container):
	for i,scenario in enumerate(scenarios_container):
		probability = scenario_probabilities[i]
		scenario.probability = probability


def compute_uEC(list_scenarios,node_map,t,cumulative=True):
        union_set = set()
        intersection_set = list_scenarios[0].get_contaminated_nodes_at_time(t,cumulative=cumulative)
        for s in list_scenarios:
                nodes = s.get_contaminated_nodes_at_time(t,cumulative=cumulative)
                union_set.update(nodes)
                intersection_set.intersection_update(nodes)
        difference_set = union_set.difference(intersection_set)
        return sum(n*node_map[n] for n in difference_set)

def all_impact_scenarios(list_scenarios,measurements):
        
        scenario_ids = list()
        if measurements['positive']:
                for i,s in enumerate(list_scenarios):
                        added = False
                        for t,nodes in measurements['positive'].iteritems():
                                for nid in nodes:
                                        if s.is_contaminated(nid,t):
                                                scenario_ids.append(i)
                                                added=True
                                                break
                                if added:
                                        break
                return scenario_ids
        else:
                return range(len(list_scenarios))

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


"""
def numpy_inversion(scenarios_container, measurements, failure_probability):
	n_scenarios = float(len(scenarios_container))

	updated_probability = dict()
	n_measurements = 0
        
	for vals in measurements.itervalues():
		for nodes in vals.itervalues():
			n_measurements+=len(nodes)

	pf = failure_probability
	n_matches = np.array([count_number_of_matches(scenario,measurements) for scenario in scenarios_container])
	n_unmatches = n_measurements - n_matches
	p_e_given_m = (1-pf)**n_matches*pf**n_unmatches
	scenarios_p = np.array([scenario.probability for scenario in scenarios_container])
	scenario_probabilities = p_e_given_m*scenarios_p
	scenario_probabilities_normalized = scenario_probabilities/sum(scenario_probabilities)

	return scenario_probabilities_normalized

def inversion(scenarios_container,measurements, failure_probability, uniform_prior = False):

	n_scenarios = float(len(scenarios_container))
	if uniform_prior:
		scenarios_p = 1/n_scenarios

	updated_probability = dict()
	n_measurements = 0
        
	for vals in measurements.itervalues():
		for nodes in vals.itervalues():
			n_measurements+=len(nodes)

	pf = failure_probability
	accumulate = 0 
	scenario_probabilities = []
	for scenario in scenarios_container:
		n_matches = count_number_of_matches(scenario,measurements)
		n_unmatches = n_measurements-n_matches
		p_e_given_m = (1-pf)**n_matches*pf**n_unmatches
		if not uniform_prior:
			scenarios_p = scenario.probability
		numerator = p_e_given_m*scenarios_p
		accumulate += numerator
		scenario_probabilities.append(numerator)

	reciprocal_accumulate = float(1.0/accumulate)

	for i, s_probability in enumerate(scenario_probabilities):
		probability = s_probability*reciprocal_accumulate
		scenario_probabilities[i] = probability
                
	return scenario_probabilities

"""


