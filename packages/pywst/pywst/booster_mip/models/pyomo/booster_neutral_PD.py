# 
# All parameter and variables names are appended
# with unit where appropriate. 
#  _g  = grams
#  _gpm3   = grams per meter cubed (mg/L)
#  _m3pmin = meters cubed per minute
#
from pyomo.environ import *

def define_model(SCENARIO_SUMMARY, BOOSTER_CANDIDATES, CONTROLLER_NODES, IMPACTS, WQM_HEADERS, POPULATION_DOSAGE, DELTA_INDEX, OBJECTIVE_TERMS, SCENARIO_SCALING, max_stations, pd_threshold):
    model = ConcreteModel()

    # number of minutes for each timestep
    model.P_MINUTES_PER_TIMESTEP = WQM_HEADERS.P_MINUTES_PER_TIMESTEP
    # all the nodes including tanks and reservoirs
    model.S_NODES = WQM_HEADERS.S_NODES
    # all time steps
    model.S_TIMES = WQM_HEADERS.S_TIMES
    
    # The set points considered in the problem as well
    # as the objective function weights - IMPACTS.dat
    model.S_SPACETIME = Set(initialize=list(IMPACTS.P_IMPACT.keys()))
    # mass dosed (grams)
    model.P_IMPACT = IMPACTS.P_IMPACT

    model.S_DELTA_INDEX = Set(initialize=DELTA_INDEX.S_DELTA_INDEX)

    # Node population and usage data
    # A default=0 "Param"
    model.S_NZP_NODES = Set(initialize=POPULATION_DOSAGE.S_NZP_NODES, ordered=True)
    model.P_NODE_POPULATION = dict((n,0) for n in model.S_NZP_NODES)
    model.P_NODE_POPULATION.update(POPULATION_DOSAGE.P_NODE_POPULATION)
    # A default=0 "Param"
    model.P_VOLUME_INGESTED_m3 = dict(((n,t),0)
                                      for n in model.S_NZP_NODES
                                      for t in model.S_TIMES)
    model.P_VOLUME_INGESTED_m3.update(POPULATION_DOSAGE.P_VOLUME_INGESTED_m3)
    model.P_DOSE_THRESHOLD_g = pd_threshold
    
    # The set of booster nodes affecting a space-time point, i.e,
    # a sparse representation of the Z-thresh matrix - CONTROLLER_NODES.dat
    model.S_CONTROLLER_BOOSTERS = CONTROLLER_NODES.S_CONTROLLER_BOOSTERS

    # The set of candidate booster nodes - BOOSTER_CANDIDATES.dat
    model.S_BOOSTER_CANDIDATES = Set(initialize=BOOSTER_CANDIDATES.S_BOOSTER_CANDIDATES)
    
    # Maximum number of booster stations allowed - set in run file
    model.P_MAX_STATIONS = max_stations
    
    # Auxiliary data not needed to solve the problem - SCENARIO_SUMMARY.dat
    # This parameter could be different from the the size of the set
    # S_SCENARIOS. It represents the actual number of scenarios, before any 
    # possible aggregation, that is used to compute expected value.
    model.S_SCENARIOS = Set(initialize=list(SCENARIO_SUMMARY.P_WEIGHT.keys()))
    model.P_WEIGHT = SCENARIO_SUMMARY.P_WEIGHT
    
    model.P_INJECTION_SCALING = SCENARIO_SCALING.P_INJECTION_SCALING
    # relax max dose computation by some floating point tolerance
    model.EPS = 1e-6

    # A default=0 "Param"
    model.P_MAX_DOSE_g = dict(((n,s),0)
                              for n in model.S_NZP_NODES 
                              for s in model.S_SCENARIOS)
    model.P_MAX_DOSE_g.update(POPULATION_DOSAGE.P_MAX_DOSE_g)

    # A default=0 "Param"
    model.P_ALWAYS_DOSED_g = dict(((n,s),0)
                              for n in model.S_NZP_NODES 
                              for s in model.S_SCENARIOS)
    model.P_ALWAYS_DOSED_g.update(SCENARIO_SUMMARY.P_ALWAYS_DOSED_g)

    # A default=[] "Set"
    model.S_REDUCED_MAP = dict(((n,s),[])
                              for n in model.S_NZP_NODES 
                              for s in model.S_SCENARIOS)
    model.S_REDUCED_MAP.update(OBJECTIVE_TERMS.S_REDUCED_MAP)
    
    #model variables
    model.delta = Var(model.S_DELTA_INDEX, bounds=(0,1))
    model.y_booster = Var(model.S_BOOSTER_CANDIDATES, within=Binary)
    # Dose consumed
    model.dose_g = Var(model.S_NZP_NODES,model.S_SCENARIOS,within=NonNegativeReals, initialize=0.0)
    # Dose above threshold binary var
    model.z_dosed = Var(model.S_NZP_NODES,model.S_SCENARIOS,within=Binary,initialize=0) 

    def PD_obj_rule(m):
        return sum(m.P_WEIGHT[s]*m.z_dosed[n,s]*m.P_NODE_POPULATION[n] \
                   for n in m.S_NZP_NODES for s in m.S_SCENARIOS)
    model.POPULATION_DOSED = Objective(rule=PD_obj_rule)

    # Dose over all timesteps
    def total_dose_rule(m,n,s):
        return (m.dose_g[n,s] - m.P_ALWAYS_DOSED_g[n,s] - \
                sum(m.P_IMPACT[n,s,i]*m.delta[i] \
                    for i in m.S_REDUCED_MAP[n,s]), 0.0)
    model.TOTAL_DOSE = Constraint(model.S_NZP_NODES, model.S_SCENARIOS, rule=total_dose_rule)

    # Dose above thresold Big-M constraint
    def dose_bigM_rule_upper(m,n,s):
        return m.dose_g[n,s] <= \
            m.z_dosed[n,s]*(m.P_MAX_DOSE_g[n,s]*(1+m.EPS) - m.P_DOSE_THRESHOLD_g*m.P_INJECTION_SCALING[s]) + \
            m.P_DOSE_THRESHOLD_g*m.P_INJECTION_SCALING[s]
    model.DOSE_BIGM_upper = Constraint(model.S_NZP_NODES, model.S_SCENARIOS, rule=dose_bigM_rule_upper)

    # If this node/time is effected by active booster then allow delta to go to its LB.
    def reaction_rule(m,i):
        return (1.0,m.delta[i]+sum(m.y_booster[b] for b in m.S_CONTROLLER_BOOSTERS[i]),None)
    model.REACTION = Constraint(model.S_DELTA_INDEX, rule=reaction_rule)
    
    # Only P_MAX_STATIONS number of booster stations allowed.
    def booster_node_rule(m):
        return (None,summation(m.y_booster),m.P_MAX_STATIONS)
    model.BOOSTER_NODE = Constraint(rule=booster_node_rule)
    
    return model

def create_solution_function(results_filename, objective_type):
    def postprocess(options=None,instance=None,results=None):
        #instance.load(results)
        solution = {}
        solution['booster ids'] = [i for i in instance.S_BOOSTER_CANDIDATES if instance.y_booster[i].value > 0.9]
        if objective_type == 'MC':
            solution['expected mass consumed grams'] = value(instance.MASS_CONSUMED_g)
        elif objective_type == 'PD':
            solution['expected population dosed'] = value(instance.POPULATION_DOSED)
        else:
            assert False

        import yaml
        with open(results_filename,'w') as f:
            yaml.dump(solution,f)
        for (n,s) in instance.dose_g:
            if instance.dose_g[n,s]() > value(instance.P_MAX_DOSE_g[n,s]):
                print instance.dose_g.cname(True), instance.dose_g[n,s](), value(instance.P_MAX_DOSE_g[n,s])
    return postprocess
