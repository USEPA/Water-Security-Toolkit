#
# All parameter and variables names are appended
# with unit where appropriate.#  _gpmin  = grams per minute
#  _gpm3   = grams per meter cubed (mg/L)
#  _m3pmin = meters cubed per minute
#  _g      = grams
#

from pyomo.environ import *
import time

def toxin_mass_balance_rule(m,n,t,s):
    return (sum(m.P_CONC_MATRIX_CSR_TOXIN[n,t,nn,tt] * m.c_toxin_gpm3[nn,tt,s]\
                for (nn,tt) in m.S_CONC_MATRIX_CSR_INDEX_TOXIN[n,t] \
                if tt >= m.P_SCEN_START_TIMESTEP[s]) \
            + m.P_INJ_MATRIX_DIAG_TOXIN[n,t] * \
            (m.P_MN_TOXIN_gpmin[n,t,s]-m.r_toxin_gpmin[n,t,s]), 0.0)

def booster_mass_balance_rule(m,n,t,s):
    return (sum(m.P_CONC_MATRIX_CSR_BOOSTER[n,t,nn,tt] * m.c_booster_gpm3[nn,tt,s]\
                for (nn,tt) in m.S_CONC_MATRIX_CSR_INDEX_BOOSTER[n,t] \
                if tt >= m.P_SCEN_START_TIMESTEP[s]) \
            + m.P_INJ_MATRIX_DIAG_BOOSTER[n,t] * \
            (m.m_booster_gpmin[n,t,s]-m.P_ALPHA*m.r_toxin_gpmin[n,t,s]), 0.0)

# when there is no flow across the node, P_INJ_MATRIX_DIAG is
# zero... therefore that particular r_toxin_gpmin does not show up in
# the mass balance equations and we should set them to zero for
# consistency
def r_toxin_zero_rule(m,n,t,s):
    # This parameter will be -1 or 0, where 0 indicates no flow.
    if m.P_INJ_MATRIX_DIAG_TOXIN[n,t] == 0:
        return (m.r_toxin_gpmin[n,t,s],0.0)
    return Constraint.NoConstraint

# Booster injection profile must align with booster station
# selection(s)
def booster_injection_rule(m,n,t,s):
    if (n in m.S_BOOSTER_CANDIDATES) and \
       (t >= m.P_INJ_START_TIMESTEP_BOOSTER[s]) and \
       (t <= m.P_INJ_END_TIMESTEP_BOOSTER[s]):
        return (m.m_booster_gpmin[n,t,s] - \
                m.y_booster[n] * \
                m.P_INJ_STRENGTH_BOOSTER_gpmin * \
                m.P_INJ_TYPE_MULTIPLIER_BOOSTER[n,t] * \
                m.P_TIMESCALE_FACTOR_BOOSTER[t,s], 0.0)
    else:
        return (m.m_booster_gpmin[n,t,s], 0.0)

# Only MAX_STATIONS number of booster stations allowed.
def booster_node_rule(m):
    return (None,
            sum( m.y_booster[b] for b in m.S_BOOSTER_CANDIDATES),
            m.P_MAX_STATIONS)

def define_base_limit_model(SCENARIO_SUMMARY,
                            WQM,
                            DEMANDS,
                            TANK_VOLUMES,
                            BOOSTER_CANDIDATES,
                            BOOSTER_INJECTION,
                            TOXIN_INJECTION,
                            alpha,
                            max_stations):

    model = ConcreteModel()

    ###
    # These are defined by Merlion in the dat file
    # for a particular water network
    ###
    
    # number of minutes for each timestep
    model.P_MINUTES_PER_TIMESTEP = WQM.P_MINUTES_PER_TIMESTEP
    # all the nodes including tanks and reservoirs
    model.S_NODES = WQM.S_NODES
    # all time steps
    model.S_TIMES = WQM.S_TIMES

    # The sparse indexing set of P_CONC_MATRIX_CSR_BOOSTER
    model.S_CONC_MATRIX_CSR_INDEX_BOOSTER = WQM.S_CONC_MATRIX_CSR_INDEX_BOOSTER
    # These two parameters form the water quality model
    # G*c+D*m=0
    #   where c are the concentrations in g/m^3 (mg/L)
    #   and m are the mass injection terms in g/min
    # The concentration matrix (G) in CSR format
    model.P_CONC_MATRIX_CSR_BOOSTER = WQM.P_CONC_MATRIX_CSR_BOOSTER
    # The injection matrix (D) which is diagonal
    model.P_INJ_MATRIX_DIAG_BOOSTER = WQM.P_INJ_MATRIX_DIAG_BOOSTER

    # The sparse indexing set of P_CONC_MATRIX_CSR_TOXIN
    model.S_CONC_MATRIX_CSR_INDEX_TOXIN = WQM.S_CONC_MATRIX_CSR_INDEX_TOXIN
    # These two parameters form the water quality model
    # G*c+D*m=0
    #   where c are the concentrations in g/m^3 (mg/L)
    #   and m are the mass injection terms in g/min
    # The concentration matrix (G) in CSR format
    model.P_CONC_MATRIX_CSR_TOXIN = WQM.P_CONC_MATRIX_CSR_TOXIN
    # The injection matrix (D) which is diagonal
    model.P_INJ_MATRIX_DIAG_TOXIN = WQM.P_INJ_MATRIX_DIAG_TOXIN

    # Node Demands [m^3/min]
    model.P_DEMANDS_m3pmin = DEMANDS.P_DEMANDS_m3pmin

    # The set of tank node ids
    model.S_TANKS = TANK_VOLUMES.S_TANKS;
    # Tank volumes [m^3]
    model.P_TANK_VOLUMES_m3 = TANK_VOLUMES.P_TANK_VOLUMES_m3;

    # The set of candidate booster nodes
    # Note: if all nodes are valid booster candidates then this can be
    #       left as defaulted otherwise, it may be set based on
    #       non-zero demand
    model.S_BOOSTER_CANDIDATES = Set(initialize=BOOSTER_CANDIDATES.S_BOOSTER_CANDIDATES, ordered=True)

    ###
    # These variables are set in the run file (see examples)
    ###
    # Maximum number of booster stations allowed
    model.P_MAX_STATIONS = max_stations

    # Stoichiometric coefficient (mass booster agent/mass toxin)
    model.P_ALPHA = alpha

    ###
    # These variables are typically set in a separate dat file or the
    # run file if they are small enough (see examples)
    ###
    # This parameter could be different from the the size of the set
    # S_SCENARIOS. It represents the actual number of scenarios,
    # before any possible aggregation, that is used to compute
    # expected value.
    model.P_NUM_DETECTED_SCENARIOS = SCENARIO_SUMMARY.P_NUM_DETECTED_SCENARIOS
    model.S_SCENARIOS = Set(initialize=SCENARIO_SUMMARY.S_SCENARIOS, ordered=True)
    model.P_SCEN_DETECT_TIMESTEP = SCENARIO_SUMMARY.P_SCEN_DETECT_TIMESTEP
    model.P_SCEN_START_TIMESTEP = SCENARIO_SUMMARY.P_SCEN_START_TIMESTEP
    model.P_SCEN_END_TIMESTEP = SCENARIO_SUMMARY.P_SCEN_END_TIMESTEP
    model.P_WEIGHT = SCENARIO_SUMMARY.P_WEIGHT

    model.S_SPARSE_NODE_INDEX = \
        Set(initialize=[ (n,t,s) for n in model.S_NODES \
                         for t in model.S_TIMES \
                         for s in model.S_SCENARIOS \
                         if (t >= model.P_SCEN_START_TIMESTEP[s]) \
                         and (t <= model.P_SCEN_END_TIMESTEP[s]) ], ordered=True)

    # Booster injection profiles
    # Booster station injection strength [g/min]
    model.P_INJ_STRENGTH_BOOSTER_gpmin = BOOSTER_INJECTION.P_INJ_STRENGTH_BOOSTER_gpmin
    # This is used to determine the amount of mass
    # to inject for flowpaced injections.
    # If the injections are simple mass type injections
    # this parameter will be equal to 1.
    model.P_INJ_TYPE_MULTIPLIER_BOOSTER = BOOSTER_INJECTION.P_INJ_TYPE_MULTIPLIER_BOOSTER
    # A default=1 "Param"
    model.P_TIMESCALE_FACTOR_BOOSTER = dict(((t,s),1)
                                            for t in model.S_TIMES
                                            for s in model.S_SCENARIOS
                                            if (t >= model.P_SCEN_START_TIMESTEP[s])
                                            and (t <= model.P_SCEN_END_TIMESTEP[s]))
    model.P_TIMESCALE_FACTOR_BOOSTER.update(BOOSTER_INJECTION.P_TIMESCALE_FACTOR_BOOSTER)
    model.P_INJ_START_TIMESTEP_BOOSTER = BOOSTER_INJECTION.P_INJ_START_TIMESTEP_BOOSTER
    model.P_INJ_END_TIMESTEP_BOOSTER = BOOSTER_INJECTION.P_INJ_END_TIMESTEP_BOOSTER

    # Set the toxin injection profile for each of the scenarios
    # Note: These default to zero, so you only need to
    #       set the values for the non-zero parts
    # Toxin mass injection profile [g/min]
    model.P_MN_TOXIN_gpmin = Param(model.S_SPARSE_NODE_INDEX, default=0, initialize=TOXIN_INJECTION.P_MN_TOXIN_gpmin)

    ###
    # Model variables
    ###
    # toxin concentration for each node/time/scenario [g/m^3]
    model.c_toxin_gpm3 = Var(model.S_SPARSE_NODE_INDEX,within=NonNegativeReals, initialize = 0.0)

    # booster agent concentration for each node/time/scenario [g/m^3]
    model.c_booster_gpm3 = Var(model.S_SPARSE_NODE_INDEX,within=NonNegativeReals, initialize = 0.0)

    # booster agent mass injection for each node/time/scenario [g/min]
    model.m_booster_gpmin = Var(model.S_SPARSE_NODE_INDEX,within=NonNegativeReals, initialize = 0.0)

    # mass toxin removed by reaction for each node/time/scenario
    # [g/min]
    model.r_toxin_gpmin = Var(model.S_SPARSE_NODE_INDEX,within=NonNegativeReals, initialize = 0.0)

    # boooster station selection variable
    model.y_booster = Var(model.S_BOOSTER_CANDIDATES,within=Binary, initialize=0)

    ###
    # Model constraints
    ###
    model.TOXIN_MASS_BALANCE = Constraint(model.S_SPARSE_NODE_INDEX,rule=toxin_mass_balance_rule)

    model.BOOSTER_MASS_BALANCE = Constraint(model.S_SPARSE_NODE_INDEX,rule=booster_mass_balance_rule)

    # when there is no flow across the node, P_INJ_MATRIX_DIAG is
    # zero... therefore that particular r_toxin_gpmin does not show up
    # in the mass balance equations and we should set them to zero for
    # consistency
    model.R_TOXIN_ZERO_IF_NO_FLOW_IN_NODE = Constraint(model.S_SPARSE_NODE_INDEX,rule=r_toxin_zero_rule)

    # Booster injection profile must align with booster station selection(s)
    model.BOOSTER_INJECTION = Constraint(model.S_SPARSE_NODE_INDEX, rule=booster_injection_rule)

    # Only MAX_STATIONS number of booster stations allowed.
    model.BOOSTER_NODE = Constraint(rule=booster_node_rule)

    return model

def MC_obj_rule(m):
    return m.P_MINUTES_PER_TIMESTEP * \
        sum(m.P_WEIGHT[s]*m.P_DEMANDS_m3pmin[n,t]*m.c_toxin_gpm3[n,t,s] \
            for (n,t,s) in m.S_SPARSE_NODE_INDEX \
            if m.P_DEMANDS_m3pmin[n,t] > 0.0)

def define_MC_LIMIT_model(SCENARIO_SUMMARY,
                          WQM,
                          DEMANDS,
                          TANK_VOLUMES,
                          BOOSTER_CANDIDATES,
                          BOOSTER_INJECTION,
                          TOXIN_INJECTION,
                          alpha,
                          max_stations):

    model = define_base_limit_model(SCENARIO_SUMMARY,
                                    WQM,
                                    DEMANDS,
                                    TANK_VOLUMES,
                                    BOOSTER_CANDIDATES,
                                    BOOSTER_INJECTION,
                                    TOXIN_INJECTION,
                                    alpha,
                                    max_stations)

    ###
    # Model objective
    ###
    model.MASS_CONSUMED_g = Objective(rule=MC_obj_rule)

    return model

def PD_obj_rule(m):
    return sum(m.P_WEIGHT[s]*m.z_dosed[n,s]*m.P_NODE_POPULATION[n] \
               for n in m.S_NZP_NODES for s in m.S_SCENARIOS) + \
           sum(m.c_toxin_gpm3[n,t,s] \
            for (n,t,s) in m.S_SPARSE_NODE_INDEX)*m.REGULARIZATION_EPS



def total_dose_rule(m,n,s):
    return (m.dose_g[n,s] - \
            m.P_NODE_POPULATION[n]*sum(m.c_toxin_gpm3[n,t,s] * \
                m.P_VOLUME_INGESTED_m3[n,t] \
                for t in m.S_TIMES \
                if (t >= m.P_SCEN_START_TIMESTEP[s]) and \
                (t <= m.P_SCEN_END_TIMESTEP[s])), 0.0)

def dose_bigM_rule_lower(m,n,s):
    return m.P_NODE_POPULATION[n]*m.P_DOSE_THRESHOLD_g*m.z_dosed[n,s] <= m.dose_g[n,s]

def dose_bigM_rule_upper(m,n,s):
    return m.dose_g[n,s] <= \
        m.z_dosed[n,s]*m.P_NODE_POPULATION[n]*(m.P_MAX_DOSE_g[n,s]*(1+m.EPS) - m.P_DOSE_THRESHOLD_g) + \
        m.P_NODE_POPULATION[n]*m.P_DOSE_THRESHOLD_g


def define_PD_LIMIT_model(SCENARIO_SUMMARY,
                          WQM,
                          DEMANDS,
                          TANK_VOLUMES,
                          BOOSTER_CANDIDATES,
                          BOOSTER_INJECTION,
                          TOXIN_INJECTION,
                          POPULATION_DOSAGE,
                          alpha,
                          max_stations,
                          pd_threshold):

    model = define_MC_LIMIT_model(SCENARIO_SUMMARY,
                                  WQM,
                                  DEMANDS,
                                  TANK_VOLUMES,
                                  BOOSTER_CANDIDATES,
                                  BOOSTER_INJECTION,
                                  TOXIN_INJECTION,
                                  alpha,
                                  max_stations)

    model.MASS_CONSUMED_g.deactivate()

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
    # A default=0 "Param"
    model.P_MAX_DOSE_g = dict(((n,s),0)
                              for n in model.S_NZP_NODES 
                              for s in model.S_SCENARIOS)
    model.P_MAX_DOSE_g.update(POPULATION_DOSAGE.P_MAX_DOSE_g)
    model.P_DOSE_THRESHOLD_g = pd_threshold
    # relax max dose computation by some floating point tolerance
    model.EPS = 1
    model.REGULARIZATION_EPS = 1e-4

    ###
    # Model variables
    ###
    # Dose consumed
    model.dose_g = Var(model.S_NZP_NODES,model.S_SCENARIOS,within=NonNegativeReals, initialize=0.0)

    # Dose above threshold binary var
    model.z_dosed = Var(model.S_NZP_NODES,model.S_SCENARIOS,within=Binary,initialize=0)

    ###
    # Model objective and constraints
    ###
    model.POPULATION_DOSED = Objective(rule=PD_obj_rule)

    # Dose over all timesteps
    model.TOTAL_DOSE = Constraint(model.S_NZP_NODES, model.S_SCENARIOS, rule=total_dose_rule)

    # Dose above thresold Big-M constraint
    model.DOSE_BIGM_lower = Constraint(model.S_NZP_NODES, model.S_SCENARIOS, rule=dose_bigM_rule_lower)
    model.DOSE_BIGM_upper = Constraint(model.S_NZP_NODES, model.S_SCENARIOS, rule=dose_bigM_rule_upper)

    # Do a bit of preprocessing for the dose_g and z_dosed variables
    for n in model.S_NZP_NODES:
        for s in model.S_SCENARIOS:
            model.dose_g[n,s].setub(model.P_NODE_POPULATION[n]*model.P_MAX_DOSE_g[n,s]*(1+model.EPS))
            if model.dose_g[n,s].ub < model.P_DOSE_THRESHOLD_g*model.P_NODE_POPULATION[n]:
                model.z_dosed[n,s].fix(0)

    return model

def create_solution_function(results_filename, objective_type):
    def postprocess(options=None,instance=None,results=None):
        #instance.load(results)
        solution = {}
        solution['booster ids'] = \
            [i for i in instance.S_BOOSTER_CANDIDATES \
             if instance.y_booster[i].value > 0.9]
        solution['mass consumed grams'] = \
            dict((s,sum(instance.P_DEMANDS_m3pmin[n,t] * instance.c_toxin_gpm3[n,t,s].value \
                 for n in instance.S_NODES
                 for t in instance.S_TIMES \
                 if t >= instance.P_SCEN_START_TIMESTEP[s] \
                 and  t <= instance.P_SCEN_END_TIMESTEP[s] \
                 and instance.P_DEMANDS_m3pmin[n,t] > 0) * \
             instance.P_MINUTES_PER_TIMESTEP) for s in instance.S_SCENARIOS)
        solution['expected mass consumed grams'] = value(instance.MASS_CONSUMED_g)

        if objective_type == 'PD':
            solution['expected population dosed'] = value(instance.POPULATION_DOSED)
            solution['population dosed'] = \
                dict((s,sum(instance.z_dosed[n,s].value * instance.P_NODE_POPULATION[n] \
                     for n in instance.S_NZP_NODES)) \
                 for s in instance.S_SCENARIOS)
        elif objective_type != 'MC':
            raise ValueError

        solution['tank mass grams'] = \
            dict((s,sum(max(0.0,instance.c_toxin_gpm3[n,instance.P_SCEN_END_TIMESTEP[s],s].value - \
                     (1.0/instance.P_ALPHA) * instance.c_booster_gpm3[n,instance.P_SCEN_END_TIMESTEP[s],s].value) * \
                 instance.P_TANK_VOLUMES_m3[n,instance.P_SCEN_END_TIMESTEP[s]] \
                 for n in instance.S_TANKS)) \
             for s in instance.S_SCENARIOS)

        import yaml
        with open(results_filename,'w') as f:
            yaml.dump(solution,f)
    return postprocess
