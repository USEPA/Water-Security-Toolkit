# 
# All parameter and variables names are appended
# with unit where appropriate. 
#  _gpmin  = grams per minute
#  _gpm3   = grams per meter cubed (mg/L)
#  _m3pmin = meters cubed per minute
#  _g      = grams
#
from pyomo.environ import *

def define_model(alpha, max_stations):
    model = AbstractModel()

    ###
    # These are defined by Merlion in the dat file
    # for a particular water network
    ###
    
    # number of minutes for each timestep
    model.P_MINUTES_PER_TIMESTEP = Param();
    # all the nodes including tanks and reservoirs
    model.S_NODES = Set()
    # all time steps
    model.S_TIMES = Set()

    # The sparse indexing set of P_CONC_MATRIX_CSR_BOOSTER
    model.S_CONC_MATRIX_CSR_INDEX_BOOSTER = Set(model.S_NODES, model.S_TIMES, dimen=2)    
    def sparse_csr_index_rule(m):
        return ((n,t,nn,tt) for n in m.S_NODES \
                        for t in m.S_TIMES \
                        for (nn,tt) in m.S_CONC_MATRIX_CSR_INDEX_BOOSTER[n,t])
    model.S_SPARSE_CSR_INDEX_BOOSTER = Set(dimen=4,rule=sparse_csr_index_rule)
    # These two parameters form the water quality model
    # G*c+D*m=0 
    #   where c are the concentrations in g/m^3 (mg/L)
    #   and m are the mass injection terms in g/min
    # The concentration matrix (G) in CSR format
    model.P_CONC_MATRIX_CSR_BOOSTER = Param(model.S_SPARSE_CSR_INDEX_BOOSTER)
    # The injection matrix (D) which is diagonal
    model.P_INJ_MATRIX_DIAG_BOOSTER = Param(model.S_NODES, model.S_TIMES)

    # The sparse indexing set of P_CONC_MATRIX_CSR_TOXIN
    model.S_CONC_MATRIX_CSR_INDEX_TOXIN = Set(model.S_NODES, model.S_TIMES, dimen=2)    
    def sparse_csr_index_rule(m):
        return ((n,t,nn,tt) for n in m.S_NODES \
                        for t in m.S_TIMES \
                        for (nn,tt) in m.S_CONC_MATRIX_CSR_INDEX_TOXIN[n,t])
    model.S_SPARSE_CSR_INDEX_TOXIN = Set(dimen=4,rule=sparse_csr_index_rule)
    # These two parameters form the water quality model
    # G*c+D*m=0 
    #   where c are the concentrations in g/m^3 (mg/L)
    #   and m are the mass injection terms in g/min
    # The concentration matrix (G) in CSR format
    model.P_CONC_MATRIX_CSR_TOXIN = Param(model.S_SPARSE_CSR_INDEX_TOXIN)
    # The injection matrix (D) which is diagonal
    model.P_INJ_MATRIX_DIAG_TOXIN = Param(model.S_NODES, model.S_TIMES)
    
    # Node Demands [m^3/min]
    model.P_DEMANDS_m3pmin = Param(model.S_NODES,model. S_TIMES, default=0)
    
    # The set of candidate booster nodes
    # Note: if all nodes are valid booster candidates
    #       then this can be left as defaulted
    #       otherwise, it may be set based on 
    #       non-zero demand
    model.S_BOOSTER_CANDIDATES = Set()
    
    ###
    # These variables are set in the run file
    # (see examples)
    ###
    # Maximum number of booster stations allowed
    model.P_MAX_STATIONS = Param(initialize=max_stations)
    
    # Stoichiometric coefficient (mass booster agent/mass toxin)
    model.P_ALPHA = Param(initialize=alpha)
    
    ###
    # These variables are typically set in a 
    # separate dat file or the run file if they 
    # are small enough
    # (see examples)
    ###
    model.P_SCEN_DETECT_TIMESTEP = Param()
    model.P_SCEN_START_TIMESTEP = Param()
    model.P_SCEN_END_TIMESTEP = Param()
    
    
    def sparse_node_index_rule(m):
        return ( (n,t) for n in m.S_NODES \
                         for t in m.S_TIMES \
                         if (t >= m.P_SCEN_START_TIMESTEP) \
                         and (t <= m.P_SCEN_END_TIMESTEP) )
    model.S_SPARSE_NODE_INDEX = Set(dimen=2,rule=sparse_node_index_rule)
    
    # Booster injection profiles
    # Booster station injection strength [g/min]
    model.P_INJ_STRENGTH_BOOSTER_gpmin = Param()
    # This is used to determine the amount of mass
    # to inject for flowpaced injections.
    # If the injections are simple mass type injections
    # this parameter will be equal to 1.
    model.P_INJ_TYPE_MULTIPLIER_BOOSTER = Param(model.S_NODES, model.S_TIMES)
    model.P_TIMESCALE_FACTOR_BOOSTER = Param(model.S_TIMES, default=1)
    model.P_INJ_START_TIMESTEP_BOOSTER = Param()
    model.P_INJ_END_TIMESTEP_BOOSTER = Param()
    
    # Set the toxin injection profile for each of the scenarios
    # Note: These default to zero, so you only need to 
    #       set the values for the non-zero parts
    # Toxin mass injection profile [g/min] 
    model.P_MN_TOXIN_gpmin = Param(model.S_SPARSE_NODE_INDEX, default=0)
    
    ###
    # Model variables
    ###
    # toxin concentration for each node/time/scenario [g/m^3]
    model.c_toxin_gpm3 = Var(model.S_SPARSE_NODE_INDEX,within=NonNegativeReals)
    
    # booster agent concentration for each node/time/scenario [g/m^3]
    model.c_booster_gpm3 = Var(model.S_SPARSE_NODE_INDEX,within=NonNegativeReals)
    
    # booster agent mass injection for each node/time/scenario [g/min]
    model.m_booster_gpmin = Var(model.S_SPARSE_NODE_INDEX,within=NonNegativeReals)
    
    # mass toxin removed by reaction for each node/time/scenario [g/min]
    model.r_toxin_gpmin = Var(model.S_SPARSE_NODE_INDEX,within=NonNegativeReals)
    
    # boooster station selection variable
    model.y_booster = Var(model.S_BOOSTER_CANDIDATES,within=Binary)
    
    model.FirstStageCost = Var()
    model.SecondStageCost = Var()
    
    ###
    # Model objective and constraints
    ###
    
    def toxin_mass_balance_rule(m,n,t):
        return sum(m.P_CONC_MATRIX_CSR_TOXIN[n,t,nn,tt]*m.c_toxin_gpm3[nn,tt] for (nn,tt) in m.S_CONC_MATRIX_CSR_INDEX_TOXIN[n,t] \
                                                                         if tt >= m.P_SCEN_START_TIMESTEP) \
                    + m.P_INJ_MATRIX_DIAG_TOXIN[n,t]*(m.P_MN_TOXIN_gpmin[n,t]-m.r_toxin_gpmin[n,t]) == 0.0	
    model.TOXIN_MASS_BALANCE = Constraint(model.S_SPARSE_NODE_INDEX,rule=toxin_mass_balance_rule)
    
    def booster_mass_balance_rule(m,n,t):
        return sum(m.P_CONC_MATRIX_CSR_BOOSTER[n,t,nn,tt]*m.c_booster_gpm3[nn,tt] for (nn,tt) in m.S_CONC_MATRIX_CSR_INDEX_BOOSTER[n,t] \
                                                                         if tt >= m.P_SCEN_START_TIMESTEP) \
                    + m.P_INJ_MATRIX_DIAG_BOOSTER[n,t]*(m.m_booster_gpmin[n,t]-m.P_ALPHA*m.r_toxin_gpmin[n,t]) == 0.0
    model.BOOSTER_MASS_BALANCE = Constraint(model.S_SPARSE_NODE_INDEX,rule=booster_mass_balance_rule)
    
    # when there is no flow across the node, P_INJ_MATRIX_DIAG is zero... therefore
    # that particular r_toxin_gpmin does not show up in the mass balance equations 
    # and we should set them to zero for consistency
    def r_toxin_zero_rule(m,n,t):
        # This parameter will be -1 or 0, where 0 indicates no flow. 
        if m.P_INJ_MATRIX_DIAG_TOXIN[n,t] == 0:
            return m.r_toxin_gpmin[n,t] == 0.0
        return Constraint.NoConstraint
    model.R_TOXIN_ZERO_IF_NO_FLOW_IN_NODE = Constraint(model.S_SPARSE_NODE_INDEX,rule=r_toxin_zero_rule)
    
    # Booster injection profile must align with booster station selection(s)
    def booster_injection_rule(m,n,t):
        if (n in m.S_BOOSTER_CANDIDATES) and (t >= m.P_INJ_START_TIMESTEP_BOOSTER) and (t <= m.P_INJ_END_TIMESTEP_BOOSTER):
            return m.m_booster_gpmin[n,t] == m.y_booster[n]*m.P_INJ_STRENGTH_BOOSTER_gpmin*m.P_INJ_TYPE_MULTIPLIER_BOOSTER[n,t]*m.P_TIMESCALE_FACTOR_BOOSTER[t,s]
        else:
            return m.m_booster_gpmin[n,t] == 0.0
    model.BOOSTER_INJECTION = Constraint(model.S_SPARSE_NODE_INDEX, rule=booster_injection_rule)
    
    # Only MAX_STATIONS number of booster stations allowed.
    def booster_node_rule(m):
        return sum( m.y_booster[b] for b in m.S_BOOSTER_CANDIDATES ) <= m.P_MAX_STATIONS;
    model.BOOSTER_NODE = Constraint(rule=booster_node_rule)
    
    #
    # Stage-specific cost computations
    #
    
    def ComputeFirstStageCost_rule(m):
        return m.FirstStageCost == 0.0
    
    model.ComputeFirstStageCost = Constraint()
    
    def ComputeSecondStageCost_rule(m):
        expr = m.P_MINUTES_PER_TIMESTEP*sum(m.P_DEMANDS_m3pmin[n,t]*m.c_toxin_gpm3[n,t]
                                            for (n,t) in m.S_SPARSE_NODE_INDEX 
                                            if m.P_DEMANDS_m3pmin[n,t] > 0.0 )
        return (m.SecondStageCost - expr) == 0.0
    
    model.ComputeSecondStageCost = Constraint()
    
    #
    # Objective
    #
    
    def Total_Cost_Objective_rule(m):
        return m.FirstStageCost + m.SecondStageCost
    
    model.Total_Cost_Objective = Objective(sense=minimize)

    return model
