
# All parameter and variables names are appended
# with units where appropriate. 
#  _gpmin  = grams per minute
#  _gpm3   = grams per meter cubed (mg/L)
#  _m3pmin = meters cubed per minute
#  _g      = grams
#

###
# These are defined by Merlion in the dat file
# for a particular water network
###

#SETS
# Measurements tuples set
set S_MEAS dimen 2;
# The sparse indexing set of P_INV_CONC_MATRIX_CSR
set S_INV_CONC_MATRIX_CSR_INDEX{(n,t) in S_MEAS} dimen 2 default {};
# These two parameters form the water quality model
# c=A*m 
#   where c are the concentrations in g/m^3 (mg/L)
#   and m are the mass injection terms in g/min
# The concentration matrix (A) in CSR format
param P_INV_CONC_MATRIX_CSR{(n,t) in S_MEAS, S_INV_CONC_MATRIX_CSR_INDEX[n,t]};

# Set of (node,time) pairs having an impact on at least one of the measurement values
set S_IMPACT dimen 2; # = union {(n,t) in S_MEAS} S_INV_CONC_MATRIX_CSR_INDEX[n,t];
#set of impacted nodes
set S_IMPACT_NODES;
#set of impact times of a certain node
set S_IMPACT_TIMES{S_IMPACT_NODES} ordered;

# Threshold values
param P_TH_POS, default 100;
param P_TH_NEG, default 1e-1;
# The water quality time step
param P_MINUTES_PER_TIMESTEP;
#Simulation Duration in time steps
param P_TIME_STEPS;

# Set of positive and negative measurements
set S_POS_MEAS within S_MEAS dimen 2 default {};
set S_NEG_MEAS within S_MEAS dimen 2 default {};

###
# Model variables
###
# toxin concentration for each node/time/scenario [g/m^3]
var cn_tox_gpm3{(n,t) in S_MEAS} >= 0;
# Toxin mass injection profile [g/min]
var mn_tox_gpmin{(n,t) in S_IMPACT} >= 0;
var r{S_POS_MEAS} >=0;
var q{S_NEG_MEAS} >=0;

###
# Model objective and constraints
###
minimize OBJ: sum{(n,t) in S_NEG_MEAS}(q[n,t]) + sum{(n,t) in S_POS_MEAS}(r[n,t]);

s.t.
	TOXIN_MASS_BALANCE{(n,t) in S_MEAS}:
		cn_tox_gpm3[n,t] = sum{(nn,tt) in S_INV_CONC_MATRIX_CSR_INDEX[n,t]}(P_INV_CONC_MATRIX_CSR[n,t,nn,tt]*mn_tox_gpmin[nn,tt]);

	Q_EQNS{(n,t) in S_NEG_MEAS}:
		q[n,t] >= cn_tox_gpm3[n,t]-P_TH_NEG;

	R_EQNS{(n,t) in S_POS_MEAS}:
		r[n,t] >= P_TH_POS-cn_tox_gpm3[n,t];
		
	INCREASE{n in S_IMPACT_NODES, t in S_IMPACT_TIMES[n]: t != first(S_IMPACT_TIMES[n])}:
		mn_tox_gpmin[n,t] >= mn_tox_gpmin[n,prev(t,S_IMPACT_TIMES[n])];		   
