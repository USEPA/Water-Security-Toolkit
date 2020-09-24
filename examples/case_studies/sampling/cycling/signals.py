from pywst.common.signals_output import print_scenarios_probability
from pywst.grabsample.signals_sampler import SamplingLocation
from pywst.common.signals import (Simulator, DataManager)
from visualization import probability_map
from pywst.common.mpi_util import *
from pywst.uq.auxiliary.uq import (compute_Nu,
                                   clasify_nodes,
                                   bayesian_update,
                                   unique_measurements,
                                   update_scenario_matches,
                                   mark_checked_measurements,
                                   compute_node_probabilities,
                                   build_nodes_contamination_scenarios_sets,
                                   build_source_scenarios_sets,
                                   compute_percentage_true_plume)
from util import *
import logging
import argparse
import logging
import yaml
import json
import sys


########
#from pyutilib.services import TempfileManager
#TempfileManager.tempdir = "./TMP"
########

logging.basicConfig(filename='cycles.log', level=logging.DEBUG)

none_list = ['none','','None','NONE', None]

prog = r"cycles script"

description = r"""
Runs a sequence of cycles to predict number of uncertain nodes over time
"""

parser = argparse.ArgumentParser(description=description)
#parser.add_argument('--template', dest='template', default=False, metavar='template', help='Write template file for inputs')
subparsers = parser.add_subparsers(title='subcommands')
template = subparsers.add_parser('template')
run = subparsers.add_parser('cycle')
sample = subparsers.add_parser('sample')
simulate = subparsers.add_parser('simulate')
template.add_argument('filename', type=str, help='Filename for the template')
run.add_argument('filename', type=str,help='Filename of inputs for running the cycles')
simulate.add_argument('filename', type=str,help='Filename of inputs for running simulations')
sample.add_argument('filename', type=str,help='Filename of inputs for running simulations')

template.set_defaults(func=write_template)
run.set_defaults(func=read_template)
simulate.set_defaults(func=read_sim_inputs)
sample.set_defaults(func=read_sample_inputs)

if __name__ == "__main__":
    
    args = parser.parse_args()
    inputs = args.func(args)
    if not isinstance(inputs,dict):
        print "DONE"
        sys.exit(0)

    # parsing inputs
    threshold = inputs.pop('threshold',1e-2)
    n_samples = inputs.pop('number of samples',1.0)
    meas_file = inputs.pop('measurements',None)
    start_time_sim = inputs.pop('start time sim',-1)
    step_sim = inputs.pop('step sim',-1)
    start_time = inputs.pop('start time',0)
    delta_t = inputs.pop('delta t',60)
    confidence_n = inputs.pop('confidence nodes',0.9)
    confidence_s = inputs.pop('confidence scenarios',0.9)
    scenarios_file = inputs['set scenarios']
    true_scenario_file = inputs['true scenario']
    num_cycles = inputs.pop('num cycles',1)
    pre_defined = inputs.pop('sample locations',None)
    filter_scenarios = inputs.pop('filter scenarios',True)
    mip_solver = inputs.pop('mip solver','glpk')
    map_file = inputs.pop('node metric map',None)
    only_sim = inputs.pop('only sim',False)
    only_sample = inputs.pop('only sample',False)
    with_merlion = inputs.pop('merlion',False)
    with_weights = inputs.pop('with weights',False)
    greedy = inputs.pop('greedy',False)
    sample_criteria = inputs.pop('sample criteria','random') # to change after modifying gs
    prune = inputs.pop('prune scenarios',False)
    output_solver = inputs.pop('display iter solver',False)
    shifted_times_sim = inputs.pop('shift sim times',False)
    shifted_times_load = inputs.pop('shift load times',False)

    if pre_defined is not None:
        num_cycles = len(pre_defined)+1
    true_scenario_file = os.path.abspath(true_scenario_file)
    scenarios_file = os.path.abspath(scenarios_file)
    folder_name = scenarios_file
    folder_true_scenario = true_scenario_file
    parallel = 1 if size>1 and found_mpi else 0
    
    collector_name = inputs.pop('collector filename','summary_results.json')
    flip = inputs.pop('number of flips',0)
    # simulate if necessary
    ######################################################################
    if os.path.isfile(scenarios_file):
        simulator = Simulator(scenarios_file,"SCENARIO_SIGNALS")
        if shifted_times_sim:
            simulator.run_simulations(threshold,
                                      parallel=parallel,
                                      with_merlion=with_merlion,
                                      start_time=start_time_sim,
                                      report_time=step_sim)
        else:
            simulator.run_simulations(threshold,
                                      parallel=parallel,
                                      with_merlion=with_merlion)
        folder_name = os.path.abspath("SCENARIO_SIGNALS") 
    
    # wait till all are done simulating
    if parallel:
        comm.barrier()
    
    # simulate true scenario 
    if os.path.isfile(true_scenario_file):
        t_simulator = Simulator(true_scenario_file,"TRUE_SIGNALS")
        if shifted_times_sim:
            t_simulator.run_simulations(threshold,
                                        parallel=parallel,
                                        message='TRUE_SCENARIO',
                                        start_time=start_time_sim,
                                        report_time=step_sim)
        else:
            t_simulator.run_simulations(threshold,
                                        parallel=parallel,
                                        message='TRUE_SCENARIO')
        folder_true_scenario = os.path.abspath("TRUE_SIGNALS")
        
        # all wait for simulation of true scenario
        if parallel:
            comm.barrier()
        
        if shifted_times_load:
            load_start_time = start_time
            if meas_file not in none_list: 
                first_meas_time = get_earliest_meas_time(meas_file)
                if first_meas_time>=0 and first_meas_time<start_time:
                    load_start_time = first_meas_time

            t_data_manager = DataManager(folder_true_scenario,
                                         report_time_step_min=delta_t,
                                         report_start_time=load_start_time)
        else:
            t_data_manager = DataManager(folder_true_scenario)

        t_list_ids = t_data_manager.list_scenario_ids(parallel=parallel,message='TRUE_SCENARIO')
        hid_to_cids = dict(t_list_ids)
        if rank==0:
            single_scenario = t_data_manager.read_signals_files(hid_to_cids,message='TRUE_SCENARIO')
            if len(single_scenario)!=1:
                raise RuntimeError('More than one scenario in the true scenario file.')
            true_scenario = single_scenario[0]
            #true_scenario.pprint(t_data_manager.idx_to_node_name)
            #cont = true_scenario.get_contaminated_nodes_at_time(720)
            #named_cont = [t_data_manager.idx_to_node_name[n] for n in cont]
            #print named_cont
    else:
        if not only_sample:
            raise RuntimeError('For running cycles the true scenario is required')

    if only_sim:
        if rank == 0:
            print "DONE"
        if parallel:
            comm.barrier()
        sys.exit(0)
    
    ######################################################################
    # read signals files
    if shifted_times_load:
        load_start_time = start_time
        if meas_file not in none_list: 
            first_meas_time = get_earliest_meas_time(meas_file)
            if first_meas_time>=0 and first_meas_time<start_time:
                load_start_time = first_meas_time

        data_manager = DataManager(folder_name,
                                   report_time_step_min=delta_t,
                                   report_start_time=load_start_time)
    else:
        data_manager = DataManager(folder_name)
    if rank==0:
        print "Listing signals files to read ..."
    if meas_file not in none_list: 
        measurements = data_manager.read_measurement_file(meas_file)
        if measurements['positive']:
            if filter_scenarios:
                list_scenarios_ids = data_manager.list_scenario_ids(measurements=measurements,
                                                                    parallel=parallel)
            else:
                list_scenarios_ids = data_manager.list_scenario_ids(parallel=parallel)
        else:
            print "WARNINNG: No initial detection measurements"
            list_scenarios_ids = data_manager.list_scenario_ids(parallel=parallel)
    else:
        measurements = dict()
        measurements['positive'] = dict()
        measurements['negative'] = dict()
        if rank==0:
            print "WARNINNG: No initial detection measurements"
        list_scenarios_ids = data_manager.list_scenario_ids(parallel=parallel)
        if not list_scenarios_ids:
            raise RuntimeError('None of the scenarios match the measurements provided in the measurement file')
    
    hid_to_cids = dict(list_scenarios_ids)
    if rank==0:
        print "Reading {} files".format(len(hid_to_cids))
    list_scenarios = data_manager.read_signals_files(hid_to_cids,parallel=parallel,root=0)
    #########################################################################

    if rank==0:
        #list_scenarios = data_manager.read_signals_files(hid_to_cids)
        print "N_SCENARIOS",len(list_scenarios)
        #list_scenarios = data_manager.read_signals_files(hid_to_cids)
        if pre_defined is not None:
            predefined_locations = translate_predefined_locations(pre_defined,data_manager)
        
        #pickle_scenarios_jeff(list_scenarios,data_manager)
        if map_file:
            data_manager.read_node_to_metric_map(map_file)

        if start_time<data_manager.start_time:
            start_time = data_manager.start_time
            print "WARNING: The start time is less that the start time in the signals files. Starting at {}".format(start_time)

        t = start_time
        last_t = t if only_sample else t_data_manager.times[-1]
        meas_tuples = []
        #print data_manager.times
        #list_scenarios[0].pprint(data_manager.idx_to_node_name,detail=True)

        #print data_manager.times
        # add fisrt detection measurements
        """
        for b in measurements.iterkeys():
            for tt,nodes in measurements[b].iteritems():
                for nid in nodes:
                    meas_tuples.append((nid,tt))
        """
        if not only_sample:
            measurements = dict()
            measurements['positive'] = dict()
            measurements['negative'] = dict()

        checked_measurements = set()
        
        ### temporal ###
        n_meas, cycles, nu_nodes, n_scenarios, uces, s_probs= [],[],[],[],[],[]
        max_p_diff, percentage_ident = [],[]
        container_node_probs = []
        n_probs = []
        source_probs = []
        scenario_hids = []
        scenario_sources = []
        meas_taken = []
        results_collector = dict()
        ######
        cycle = 0
        time_step = data_manager.time_step_min
        nodes = data_manager.idx_to_node_name.keys()
        n_nodes= len(nodes)
        pmf = 0.05

        logger.info('Start cycling')
        while cycle<num_cycles and t<=last_t:
            print "################# cycle {}: time {} #################".format(cycle,t/60)
            total_meas = [t_data_manager.idx_to_node_name[m] for m,tt in meas_tuples]
            cycle_meas = [n for jj,n in enumerate(total_meas) if jj>=(cycle-1)*n_samples]
            print "Total number of measurements:",n_samples*cycle
            print "New measurements taken at:",cycle_meas
            
            if t%time_step!=0:
                raise RuntimeError('Sample time {} is not multiple of time step {}'.format(t,time_step))

            # Bayesian calculation
            u_measurements = unique_measurements(measurements,checked_measurements)
            logger.info('CYCLE {} done checking unique measurements'.format(cycle))
            scenario_probabilities = bayesian_update(list_scenarios,u_measurements, pmf)
            update_scenario_matches(list_scenarios,u_measurements)
            logger.info('CYCLE {} done bayesian update'.format(cycle))
            mark_checked_measurements(measurements,checked_measurements)
            
            # Node probabilities
            times = [t]
            node_to_sn_set = build_nodes_contamination_scenarios_sets(list_scenarios,nodes,times)
            node_probabilities = compute_node_probabilities(scenario_probabilities,node_to_sn_set)
            logger.info('CYCLE {} done computing node probability'.format(cycle))
            
            # source probabilities
            source_to_sn_set = build_source_scenarios_sets(list_scenarios,nodes)
            source_probabilities = compute_node_probabilities(scenario_probabilities,source_to_sn_set)

            if with_weights:
                data_manager.node_to_metric_map = compute_weights_from_Pn(node_probabilities,
                                                                          10.0,
                                                                          0.001,
                                                                          confidence_n)
                
            Nu = compute_Nu(node_probabilities,confidence_n)
            per_ident = compute_percentage_true_plume(node_probabilities,true_scenario,t,confidence_n)
            if not prune:
                scenarios_in_confidence = count_scenarios_in_confidence(list_scenarios,confidence_s)
            if cycle==0:
                scenarios_in_confidence = len(list_scenarios)
            print "Number of scenarios in confidence interval:",scenarios_in_confidence
            print "Percentage of nodes correctly identified:",per_ident
            print "Total number of scenarios",len(list_scenarios)

            # print for debugging
            print_scenarios_probability(list_scenarios,data_manager,limit=50,sorted=True)
            #print_node_probabilities(node_probabilities,data_manager,confidence_n,detailed=only_sample)

            #if scenarios_in_confidence==1:
            #    sample_criteria = 'random'
            scenario_hids.append([s.hid for s in list_scenarios])
            scenario_sources.append([s._injection.source for s in list_scenarios])
            
            nu_nodes.append(Nu)
            n_meas.append(len(meas_tuples))
            max_p_diff.append(max(scenario_probabilities))

            if cycle<num_cycles-1 or only_sample:
                sampler = SamplingLocation(list_scenarios,data_manager)
                if pre_defined:
                    sample_tuples, objective = sampler.predefined_locations(predefined_locations[cycle],t)
                else:
                    if sample_criteria == 'random':
                        sample_tuples, objective = sampler.random_locations(nodes,t,n_samples)
                    elif sample_criteria == 'distinguish' or sample_criteria == 'wdistinguish':
                        if not greedy:
                            options = {}
                            sample_tuples, objective = sampler.run_distinguishability_model(nodes,
                                                                                            t,
                                                                                            n_samples,
                                                                                            solver=mip_solver,
                                                                                            solver_opts=options,
                                                                                            tee=output_solver,
                                                                                            with_weights=with_weights)
                        else:
                            sample_tuples, objective = sampler.greedySelection(nodes,
                                                                               t,
                                                                               n_samples,
                                                                               with_weights=with_weights)

                    elif sample_criteria == 'probability1' or sample_criteria == 'probability2':
                        options = {}
                        #options['threads'] = 24
                        #options['mip.tolerances.mipgap'] = 1e-4
                        sample_tuples, objective = sampler.run_probability_model(nodes,
                                                                                 node_probabilities,
                                                                                 t,
                                                                                 n_samples,
                                                                                 solver=mip_solver,
                                                                                 model=sample_criteria,
                                                                                 solver_opts=options,
                                                                                 tee=output_solver,
                                                                                 weight_obj=False)
                    elif sample_criteria == 'probabilityt':
                        # Node probabilities
                        list_times = []
                        probability_node_l = []
                        accum_time = t
                        while accum_time<t+delta_t:
                            times = [accum_time]
                            node_to_sn_set_t = build_nodes_contamination_scenarios_sets(list_scenarios,nodes,times)
                            node_probabilities_t = compute_node_probabilities(scenario_probabilities,node_to_sn_set)
                            list_times.append(accum_time)
                            probability_node_l.append(node_probabilities_t)
                            accum_time+=data_manager.time_step_min
                        

                        sample_tuples, objective = sampler.run_probability_time_model(nodes,
                                                                                      probability_node_l,
                                                                                      list_times,
                                                                                      n_samples,
                                                                                      solver=mip_solver,
                                                                                      tee=True)
                    else:
                        raise RuntimeError('Unknown sample criteria {}. Choose between probability1-2, random or distinguish')
                logger.info('CYCLE {} done sampling'.format(cycle))
                for p in sample_tuples:
                    meas_tuples.append(p)
                
                if not only_sample:
                    meas_filename = "MEAS.dat"
                    """
                    cycle_meas_tuples = [(nn,tt) for nn,tt in meas_tuples if tt==t]
                    if cycle>1:
                        flip=0
                    """
                    true_scenario.write_measurements(meas_filename,meas_tuples,t_data_manager.idx_to_node_name,flip=flip)
                    measurements = data_manager.read_measurement_file(meas_filename)
                    
                    """
                    print "TRUE SCENARIO"
                    if measurements['positive'].has_key(t):
                        print 'positive',measurements['positive'][t]
                    if measurements['negative'].has_key(t):
                        print 'negative',measurements['negative'][t]
                    the_scenario = list_scenarios[0] 
                    for s in list_scenarios:
                        if data_manager.idx_to_node_name[s._injection.source]=='979':
                            the_scenario=s
                    print "THE SCENARIO"
                    if measurements['positive'].has_key(t):
                        for m in measurements['positive'][t]:
                            if m in the_scenario.time_to_nodes[t]:
                                print data_manager.idx_to_node_name[m], 'match contaminated'
                            else:
                                print data_manager.idx_to_node_name[m], 'mismatch contaminated'
                    if measurements['negative'].has_key(t):
                        for m in measurements['negative'][t]:
                            if m in the_scenario.time_to_nodes[t]:
                                print data_manager.idx_to_node_name[m], 'mismatch clean'
                            else:
                                print data_manager.idx_to_node_name[m], 'match clean'
                    """
                    if prune:
                        list_scenarios = prune_scenarios(list_scenarios,
                                                          confidence_s)
                        scenarios_in_confidence = len(list_scenarios)
                    
            
            #write_tsg("injections.tsg", list_scenarios,data_manager.idx_to_node_name)
            #if map_file:
            #    uce = compute_uEC(list_scenarios,data_manager.node_to_metric_map,t,cumulative=True)
            #    print "uEC",uce
            #    uces.append(uce)
            color_nodes = clasify_nodes(node_probabilities,confidence_n,data_manager)
            inp = data_manager.get_inpfiles()[0]
            meas_locations = [t_data_manager.idx_to_node_name[m] for m,tt in meas_tuples]
            

            container_node_probs.append(color_nodes)
            
            probability_map(inp,str(cycle),
                            color_nodes,
                            sampled_nodes = meas_locations)
                
            s_probs.append(scenario_probabilities)
            n_probs.append(node_probabilities)
            source_probs.append(source_probabilities)
            meas_taken.append(cycle_meas)
            cycles.append(cycle)
            n_scenarios.append(scenarios_in_confidence)
            percentage_ident.append(per_ident)
            if only_sample and num_cycles>1:
                break
            t= t+delta_t
            cycle+=1

        if only_sample:
            print [data_manager.idx_to_node_name[nid] for nid,t in sample_tuples]
            print "DONE"
        else:
            print "---------------------SUMMARY---------------------"
            print "Number of cycles:", cycle
            print "Total number of measurements:", n_samples*cycle
            print "Number of measurements", n_meas
            print "Uncertain Nodes in each cycle", nu_nodes
            print "Percentage of nodes correctly identified",percentage_ident
            print "Number of scenarios in confidence interval in each cycle",n_scenarios
            print "Largest probability of scenario per cycle",max_p_diff
            # save results
            results_collector['classify nodes'] = container_node_probs 
            results_collector['node probabilities'] = [list(l) for l in n_probs]
            results_collector['scenario probabilities'] = [list(l) for l in s_probs]
            results_collector['source probabilities'] = [list(l) for l in source_probs]
            results_collector['measurement locations'] = meas_taken
            results_collector['number of measurements'] = n_meas
            results_collector['uncertain nodes'] = nu_nodes
            results_collector['percentage identified'] = percentage_ident
            results_collector['scenarios in confidence'] = n_scenarios
            results_collector['scenario max prob'] = max_p_diff
            results_collector['node map'] = data_manager.idx_to_node_name
            #print scenario_hids
            #results_collector['hids'] = scenario_hids
            results_collector['sources'] = scenario_sources
            with open(collector_name, 'w') as outfile:
                json.dump(results_collector, outfile)
