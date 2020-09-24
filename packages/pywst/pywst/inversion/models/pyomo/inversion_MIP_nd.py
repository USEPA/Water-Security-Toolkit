
# All parameter and variables names are appended
# with units where appropriate. 
#  _gpmin  = grams per minute
#  _gpm3   = grams per meter cubed (mg/L)
#  _m3pmin = meters cubed per minute
#  _g      = grams
#
from pyomo.environ import*
model=AbstractModel()

###
# These are defined by Merlion in the dat file
# for a particular water network
###

#SETS
# Measurements tuples set
model.S_MEAS=Set(dimen=2)
# The sparse indexing set of P_INV_CONC_MATRIX_CSR
model.S_INV_CONC_MATRIX_CSR_INDEX = Set(model.S_MEAS,dimen=2)

def sparse_csr_index_rule(model):
    return ((n,t,nn,tt) for (n,t) in model.S_MEAS \
                    for(nn,tt) in model.S_INV_CONC_MATRIX_CSR_INDEX[n,t])
    
model.S_SPARSE_CSR_INDEX= Set(dimen=4,initialize=sparse_csr_index_rule)

# Set of (node,time) pairs having an impact on at least one of the measurement values
model.S_IMPACT= Set(dimen=2)
#set of impacted nodes
model.S_IMPACT_NODES= Set()
#set of impact times of a certain node
model.S_IMPACT_TIMES= Set(model.S_IMPACT_NODES,ordered=True)
#PARAMETERS
# These two parameters form the water quality model
# c=A*m 
#   where c are the concentrations in g/m^3 (mg/L)
#   and m are the mass injection terms in g/min
# The concentration matrix (A) in CSR format
model.P_INV_CONC_MATRIX_CSR=Param(model.S_SPARSE_CSR_INDEX)

# Threshold values
model.P_TH_POS=Param(default=100)
model.P_TH_NEG=Param(default=1e-1)

# Number of injections
model.N_INJECTIONS = Param(default=1.0, mutable=True)
# Big M parameter
model.BigM = Param(default=100000)

# The water quality time step
model.P_MINUTES_PER_TIMESTEP=Param()
#Simulation Duration in time steps
model.P_TIME_STEPS=Param()

# Sets of positive and negative measurements  
model.S_POS_MEAS= Set(dimen=2, within=model.S_MEAS, initialize = [])
model.S_NEG_MEAS= Set(dimen=2, within=model.S_MEAS, initialize = [])

###
# Model variables
###
# toxin concentration for each node/time/scenario [g/m^3]
model.cn_tox_gpm3 = Var(model.S_MEAS, within=NonNegativeReals)

# Toxin mass injection profile [g/min]
model.mn_tox_gpmin = Var(model.S_IMPACT, within=NonNegativeReals)

model.r= Var(model.S_POS_MEAS, within=NonNegativeReals)
model.q= Var(model.S_NEG_MEAS, within=NonNegativeReals)

# binary variables
model.y= Var(model.S_IMPACT_NODES, within=Binary)

###
# Model objective and constraints
###
def obj(model):
    return sum(model.q[n,t] for (n,t) in model.S_NEG_MEAS) +sum(model.r[n,t] for (n,t) in model.S_POS_MEAS)

model.OBJ= Objective(rule=obj,sense=minimize)
    
def toxin_mass_balance(model,n,t):
    return model.cn_tox_gpm3[n,t] == sum(model.P_INV_CONC_MATRIX_CSR[n,t,nn,tt]*model.mn_tox_gpmin[nn,tt] for (nn,tt) in model.S_INV_CONC_MATRIX_CSR_INDEX[n,t])
    
model.TOXIN_MASS_BALANCE=Constraint(model.S_MEAS, rule=toxin_mass_balance)

def q_equations(model,n,t):
    return model.q[n,t] >= model.cn_tox_gpm3[n,t]-model.P_TH_NEG
    
model.Q_EQNS = Constraint(model.S_NEG_MEAS, rule=q_equations)

def r_equations(model,n,t):
    return model.r[n,t] >=  model.P_TH_POS- model.cn_tox_gpm3[n,t]
    
model.R_EQNS = Constraint(model.S_POS_MEAS, rule=r_equations)

def cons_n_injections(model):
    return sum(model.y[n] for n in model.S_IMPACT_NODES) <= model.N_INJECTIONS
    
model.CONS_N_INJECTIONS = Constraint(rule=cons_n_injections)

def big_m(model,n,t):
    return model.y[n]*model.BigM>=model.mn_tox_gpmin[n,t]
    
model.BIG_M = Constraint(model.S_IMPACT, rule=big_m)

def increase_index_rule(model):
    return ( (n,i) for n in model.S_IMPACT_NODES for i in xrange(1,len(model.S_IMPACT_TIMES[n])+1) if i != 1 )
model.increase_index = Set(dimen=2,initialize=increase_index_rule)

def increase_rule(model,n,i):
    t = model.S_IMPACT_TIMES[n][i]
    t_prev = model.S_IMPACT_TIMES[n][i-1]
    return model.mn_tox_gpmin[n,t] >= model.mn_tox_gpmin[n,t_prev]

model.INCREASE = Constraint(model.increase_index,rule=increase_rule)

    
    
