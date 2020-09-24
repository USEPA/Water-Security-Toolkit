from texttable import Texttable
from math import log10
import numpy as np

def print_impact_matrix(scenarios_container, data_manager, t):
    from texttable import Texttable
    sample_locations = data_manager.idx_to_node_name.keys()
    my_table = Texttable()
    list_rows = list()
    header = list()
    for l in sample_locations:
        header.append(data_manager.idx_to_node_name[l])

    try:
        first_row = sorted(header, key=lambda x: float(x))
    except:
        first_row = header
    list_rows.append(['0'] + first_row)
    for scenario in scenarios_container:
        hid = scenario.hid
        row_scenario = [
            "H" + str(hid) + "-I" + data_manager.idx_to_node_name[scenario.injection.source]]

        for location_name in first_row:
            location = data_manager.node_name_to_idx[location_name]
            if scenario.is_contaminated(location, t):
                row_scenario.append(1)
            else:
                row_scenario.append(0)

        list_rows.append(row_scenario)

    can_print = True
    for r in list_rows:
        if len(r) > 30:
            can_print = False
            break

    print "IMPACT MATRIX", t
    if can_print:
        my_table.add_rows(list_rows)
        print my_table.draw()
    else:
        for r in list_rows:
            print r


def print_alpha_matrix(scenarios_container, data_manager, t, node_probabilities,log=False):
    from texttable import Texttable
    sample_locations = data_manager.idx_to_node_name.keys()
    my_table = Texttable()
    list_rows = list()
    header = list()
    for l in sample_locations:
        header.append(data_manager.idx_to_node_name[l])

    try:
        first_row = sorted(header, key=lambda x: float(x))
    except:
        first_row = header
    list_rows.append(['0'] + first_row)
    for scenario in scenarios_container:
        hid = scenario.hid
        row_scenario = [
            "H" + str(hid) + "-I" + data_manager.idx_to_node_name[scenario.injection.source]]

        for location_name in first_row:
            location = data_manager.node_name_to_idx[location_name]
            p_node = node_probabilities[location]
            if scenario.is_contaminated(location, t):
                if not log:
                    row_scenario.append(p_node)
                else:
                    row_scenario.append(log10(p_node))
            else:
                if not log:
                    row_scenario.append(1-p_node)
                else:
                    row_scenario.append(log10(1-p_node))

        list_rows.append(row_scenario)

    can_print = True
    for r in list_rows:
        if len(r) > 30:
            can_print = False
            break

    if not log:
        print "ALPHA MATRIX"
    else:
        print "LOG ALPHA MATRIX"
    if can_print:
        my_table.add_rows(list_rows)
        print my_table.draw()
    else:
        for r in list_rows:
            print r

def get_sorted_ids(list_scenarios):
    probabilities = np.array([s.probability for s in list_scenarios])
    indices = np.argsort(probabilities)
    sorted_indices = indices[::-1]
    return sorted_indices
            
def print_scenarios_probability(scenarios_container, data_manager,limit=20,sorted=True,confidence=1.0):
    my_table = Texttable()
    my_table.set_precision(6)
    #print "\nNUMBER OF SCENARIOS", len(scenarios_container)
    probabilities = [s.probability for s in scenarios_container]
    if sorted:
        scenario_ids = get_sorted_ids(scenarios_container)
    else:
        scenario_ids = range(len(scenarios_container))

    header = ['HID','SOURCE','START (min)','STOP (min)','PROBABILITY','MATCHES']
    list_rows = list()
    list_rows.append(header)
    accum = 0.0
    for j,scenario_id in enumerate(scenario_ids):
        if j<limit:
            scenario = scenarios_container[scenario_id]
            source = data_manager.idx_to_node_name[scenario.injection.source]
            hid = scenario.hid
            n_matches = scenario.matches
            n_unmatches = scenario.miss_matches
            probability = scenario.probability
            inj_start = int(scenario._injection.start_sec/60)
            inj_end = int(scenario._injection.end_sec/60)
            list_rows.append([hid,source,inj_start,inj_end,probability,n_matches])
            accum +=probability
            if sorted and accum>confidence:
                break
        else:
            break
    my_table.add_rows(list_rows)
    print my_table.draw()

def print_node_probabilities(node_probabilities,data_manager,confidence,detailed=False):
    color_nodes = dict()
    color_nodes['green'] = set()
    color_nodes['yellow'] = set()
    color_nodes['red'] = set()
    lt = (1-confidence)*0.5
    rt = 1-lt
    confidence_interval = (lt,rt)
    for i,p in enumerate(node_probabilities):
	if i>0:
	    if p>=confidence_interval[1]:
		color_nodes['red'].add((i,p))
	    elif p<=confidence_interval[0]:
		color_nodes['green'].add((i,p))
	    else:
		color_nodes['yellow'].add((i,p))
    
    n_green = len(color_nodes['green'])
    n_yellow = len(color_nodes['yellow'])
    n_red = len(color_nodes['red'])
    print("\nNODE PROBABILITIES\n")
    print("Number of red nodes {}".format(n_red))
    print("Number of yellow nodes {}".format(n_yellow))
    print("Number of green nodes {}".format(n_green))
    
    node_qual = {'red':'LY','green':'LN','yellow':'UN'}
    if detailed:
        my_table = Texttable()
        my_table.set_precision(6)
        header = ['Node','Probability','Classification']
        list_rows = list()
        list_rows.append(header)
	for key,val in color_nodes.iteritems():
	    for nodeid,prob in val:
		name = data_manager.idx_to_node_name[nodeid]
		list_rows.append([name,prob,node_qual[key]])

        my_table.add_rows(list_rows)
        print my_table.draw()
