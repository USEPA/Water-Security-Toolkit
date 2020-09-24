# 
# All parameter and variables names are appended
# with unit where appropriate. 
#  _g  = grams
#  _gpm3   = grams per meter cubed (mg/L)
#  _m3pmin = meters cubed per minute
#
from pyomo.environ import *

def define_model(SCENARIO_SUMMARY, BOOSTER_CANDIDATES, CONTROLLER_NODES, IMPACTS, max_stations):
    model = ConcreteModel()
    
    # The set points considered in the problem as well
    # as the objective function weights - IMPACTS.dat
    model.S_SPACETIME = Set(initialize=list(IMPACTS.P_IMPACT.keys()))
    # mass consumed (grams)
    model.P_IMPACT = IMPACTS.P_IMPACT
    
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
    model.P_NUM_DETECTED_SCENARIOS = SCENARIO_SUMMARY.P_NUM_DETECTED_SCENARIOS
    model.S_SCENARIOS = SCENARIO_SUMMARY.S_SCENARIOS
    model.P_SCEN_DETECT_TIMESTEP = SCENARIO_SUMMARY.P_SCEN_DETECT_TIMESTEP
    model.P_SCEN_START_TIMESTEP = SCENARIO_SUMMARY.P_SCEN_START_TIMESTEP
    model.P_SCEN_END_TIMESTEP = SCENARIO_SUMMARY.P_SCEN_END_TIMESTEP
    model.P_PRE_OMITTED_OBJ_g = SCENARIO_SUMMARY.P_PRE_OMITTED_OBJ_g
    model.P_POST_OMITTED_OBJ_g = SCENARIO_SUMMARY.P_POST_OMITTED_OBJ_g
    model.P_WEIGHT = SCENARIO_SUMMARY.P_WEIGHT
    
    #model variables
    model.delta = Var(model.S_SPACETIME, bounds=(0,None))
    model.y_booster = Var(model.S_BOOSTER_CANDIDATES, within=Binary)
    
    def obj_rule(m):
        return sum(m.P_PRE_OMITTED_OBJ_g[s] + m.P_POST_OMITTED_OBJ_g[s] \
                   for s in m.S_SCENARIOS) + \
               sum(m.delta[n,t,s]*m.P_IMPACT[n,t,s] \
                   for (n,t,s) in m.S_SPACETIME)
    model.MASS_CONSUMED_g = Objective(rule=obj_rule)

    # If this node/time is effected by active booster then allow delta to go to its LB.
    def reaction_rule(m,n,t,s):
        return (1.0,m.delta[n,t,s]+sum(m.y_booster[b] for b in m.S_CONTROLLER_BOOSTERS[n,t,s]),None)
    model.REACTION = Constraint(model.S_SPACETIME, rule=reaction_rule)
    
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
        solution['expected mass consumed grams'] = value(instance.MASS_CONSUMED_g)
        assert objective_type == 'MC'
        import yaml
        with open(results_filename,'w') as f:
            yaml.dump(solution,f)
    return postprocess
