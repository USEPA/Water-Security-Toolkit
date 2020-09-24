
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

# measurements set
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
set S_IMPACT dimen 2;
# Set of all impact nodes
set S_IMPACT_NODES;
#set of impact times for a certain impact node
set S_IMPACT_TIMES{S_IMPACT_NODES} ordered;
# set of all impact times
set S_ALL_TIMES = union {n in S_IMPACT_NODES} S_IMPACT_TIMES[n] ordered;

# Threshold values
param P_TH_POS, default 100;
param P_TH_NEG, default 1e-1;

#Number of injections
param N_INJECTIONS, default 1;
#Big M parameter
param B, default 100000;
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
var y{S_IMPACT_NODES,S_ALL_TIMES} binary;
var strength >=0;

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

	MAX_STRENGTH{(n,t) in S_IMPACT}:
		mn_tox_gpmin[n,t]<=B*y[n,t];

	INJECTION_STRENGTH_LOWER{(n,t) in S_IMPACT}:
		strength-B*(1-y[n,t]) <= mn_tox_gpmin[n,t];

	INJECTION_STRENGTH_UPPER{(n,t)in S_IMPACT}:
		mn_tox_gpmin[n,t] <= strength;
  		
	INCREASE{n in S_IMPACT_NODES, t in S_ALL_TIMES: t != first(S_ALL_TIMES)}:
		y[n,t] >= y[n,prev(t,S_ALL_TIMES)];

	NUMBER_INJECTIONS{t in S_ALL_TIMES}:
		sum{n in S_IMPACT_NODES}y[n,t]<=N_INJECTIONS;		   
