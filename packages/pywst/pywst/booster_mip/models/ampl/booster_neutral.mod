# 
# All parameter and variables names are appended
# with unit where appropriate. 
#  _g  = grams
#  _gpm3   = grams per meter cubed (mg/L)
#  _m3pmin = meters cubed per minute
#



# The of set points considered in the problem as well
# as the objective function weights - SPACETIME_MASS_CONSUMED.dat
set S_SPACETIME dimen 3;
# mass consumed (grams)
param P_IMPACT{S_SPACETIME};

# The set of booster nodes affecting a space-time point, i.e,
# a sparse representation of the Z-thresh matrix - CONTROLLER_NODES.dat
set S_CONTROLLER_BOOSTERS{S_SPACETIME} default {};

# The set of candidate booster nodes - BOOSTER_CANDIDATES.dat
set S_BOOSTER_CANDIDATES;

# Maximum number of booster stations allowed - set in run file
param P_MAX_STATIONS;

# Auxiliary data not needed to solve the problem - SCENARIO_SUMMARY.dat
# This parameter could be different from the the size of the set
# S_SCENARIOS. It represents the actual number of scenarios, before any 
# possible aggregation, that is used to compute expected value.
param P_NUM_DETECTED_SCENARIOS;
set S_SCENARIOS ordered;
param P_SCEN_DETECT_TIMESTEP{S_SCENARIOS};
param P_SCEN_START_TIMESTEP{S_SCENARIOS};
param P_SCEN_END_TIMESTEP{S_SCENARIOS};
param P_PRE_OMITTED_OBJ_g{S_SCENARIOS};
param P_POST_OMITTED_OBJ_g{S_SCENARIOS};
param P_WEIGHT{S_SCENARIOS};

#model variables
var delta{S_SPACETIME} >= 0;
var y_booster{S_BOOSTER_CANDIDATES} binary;

minimize MASS_CONSUMED_g: 
	 sum{s in S_SCENARIOS}(P_PRE_OMITTED_OBJ_g[s] + P_POST_OMITTED_OBJ_g[s]) 
	 + sum{(n,t,s) in S_SPACETIME}(delta[n,t,s]*P_IMPACT[n,t,s]);

s.t.
	# If this node/time is effected by active booster then allow delta to go to its LB.
	REACTION{(n,t,s) in S_SPACETIME}:
		delta[n,t,s] >= (1-sum{b in S_CONTROLLER_BOOSTERS[n,t,s]}( y_booster[b] ));		
		
	# Only P_MAX_STATIONS number of booster stations allowed.
	BOOSTER_NODE:
		sum{b in S_BOOSTER_CANDIDATES}( y_booster[b] ) <= P_MAX_STATIONS;
