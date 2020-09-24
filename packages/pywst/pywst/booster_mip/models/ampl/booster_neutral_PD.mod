# 
# All parameter and variables names are appended
# with unit where appropriate. 
#  _g  = grams
#  _gpm3   = grams per meter cubed (mg/L)
#  _m3pmin = meters cubed per minute
#

# number of minutes for each timestep
param P_MINUTES_PER_TIMESTEP;
# all the nodes including tanks and reservoirs
set S_NODES;
# all time steps
set S_TIMES; 

# The of set points considered in the problem as well
# as the objective function weights - SPACETIME_MASS_CONSUMED.dat
set S_SPACETIME dimen 3;
# mass dosed (grams)
param P_IMPACT{S_SPACETIME};

set S_DELTA_INDEX;

set S_NZP_NODES default {};
param P_NODE_POPULATION{S_NZP_NODES} default 0.0;
param P_VOLUME_INGESTED_m3{S_NZP_NODES, S_TIMES} default 0.0;

param P_DOSE_THRESHOLD_g default 0.0;

# Auxiliary data not needed to solve the problem - SCENARIO_SUMMARY.dat
# This parameter could be different from the the size of the set
# S_SCENARIOS. It represents the actual number of scenarios, before any 
# possible aggregation, that is used to compute expected value.
param P_NUM_DETECTED_SCENARIOS;
set S_SCENARIOS ordered;
param P_WEIGHT{S_SCENARIOS};

param P_INJECTION_SCALING{S_SCENARIOS};
param EPS default 1e-6;

param P_MAX_DOSE_g{S_NZP_NODES, S_SCENARIOS} default 0.0;

param P_ALWAYS_DOSED_g{S_NZP_NODES, S_SCENARIOS} default 0;

# Maps dosage term to delta variable
set S_REDUCED_MAP{S_NZP_NODES, S_SCENARIOS} default {};

# The set of booster nodes affecting a space-time point, i.e,
# a sparse representation of the Z-thresh matrix - CONTROLLER_NODES.dat
set S_CONTROLLER_BOOSTERS{S_DELTA_INDEX} default {};

# The set of candidate booster nodes - BOOSTER_CANDIDATES.dat
set S_BOOSTER_CANDIDATES;

# Maximum number of booster stations allowed - set in run file
param P_MAX_STATIONS;

#model variables
var delta{S_DELTA_INDEX} >= 0, <= 1;
var y_booster{S_BOOSTER_CANDIDATES} binary;
var z_dosed{S_NZP_NODES, S_SCENARIOS} binary;
var dose_g{n in S_NZP_NODES, s in S_SCENARIOS} >= 0;

minimize POPULATION_DOSED:
	 sum{s in S_SCENARIOS}(P_WEIGHT[s] * sum{n in S_NZP_NODES}(z_dosed[n,s]*P_NODE_POPULATION[n]));

s.t.
        # Dose above thresold Big-M constraint
        DOSE_BIGM_upper{n in S_NZP_NODES, s in S_SCENARIOS}:
                dose_g[n,s] <= z_dosed[n,s]*(P_MAX_DOSE_g[n,s]*(1+EPS) - P_DOSE_THRESHOLD_g*P_INJECTION_SCALING[s]) + P_DOSE_THRESHOLD_g*P_INJECTION_SCALING[s];

        # Dose over all timesteps
        TOTAL_DOSE{n in S_NZP_NODES, s in S_SCENARIOS}:
                P_ALWAYS_DOSED_g[n,s] + sum{i in S_REDUCED_MAP[n,s]}(P_IMPACT[n,s,i]*delta[i]) = dose_g[n,s];

	# If this node/time is effected by active booster then allow delta to go to its LB.
	REACTION{i in S_DELTA_INDEX}:
		delta[i] >= (1-sum{b in S_CONTROLLER_BOOSTERS[i]}( y_booster[b] ));

	# Only P_MAX_STATIONS number of booster stations allowed.
	BOOSTER_NODE:
		sum{b in S_BOOSTER_CANDIDATES}( y_booster[b] ) <= P_MAX_STATIONS;
