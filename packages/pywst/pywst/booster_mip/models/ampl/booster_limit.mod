# 
# All parameter and variables names are appended
# with unit where appropriate. 
#  _gpmin  = grams per minute
#  _gpm3   = grams per meter cubed (mg/L)
#  _m3pmin = meters cubed per minute
#  _g      = grams
#

###
# These are defined by Merlion in the dat file
# for a particular water network
###

# number of minutes for each timestep
param P_MINUTES_PER_TIMESTEP;
# all the nodes including tanks and reservoirs
set S_NODES;
# all time steps
set S_TIMES; 

# The sparse indexing set of P_CONC_MATRIX_CSR_TOXIN
set S_CONC_MATRIX_CSR_INDEX_TOXIN{S_NODES, S_TIMES} dimen 2 default {};
# These two parameters form the water quality model
# G*c+D*m=0 
#   where c are the concentrations in g/m^3 (mg/L)
#   and m are the mass injection terms in g/min
# The concentration matrix (G) in CSR format
param P_CONC_MATRIX_CSR_TOXIN{n in S_NODES, t in S_TIMES, S_CONC_MATRIX_CSR_INDEX_TOXIN[n,t]};
# The injection matrix (D) which is diagonal
param P_INJ_MATRIX_DIAG_TOXIN{n in S_NODES, t in S_TIMES};

# The sparse indexing set of P_CONC_MATRIX_CSR_BOOSTER
set S_CONC_MATRIX_CSR_INDEX_BOOSTER{S_NODES, S_TIMES} dimen 2 default {};
# These two parameters form the water quality model
# G*c+D*m=0 
#   where c are the concentrations in g/m^3 (mg/L)
#   and m are the mass injection terms in g/min
# The concentration matrix (G) in CSR format
param P_CONC_MATRIX_CSR_BOOSTER{n in S_NODES, t in S_TIMES, S_CONC_MATRIX_CSR_INDEX_BOOSTER[n,t]};
# The injection matrix (D) which is diagonal
param P_INJ_MATRIX_DIAG_BOOSTER{n in S_NODES, t in S_TIMES};

# Node Demands [m^3/min]
param P_DEMANDS_m3pmin{S_NODES,S_TIMES} default 0;

# The set of tank node ids
set S_TANKS;
# Tank volumes [m^3]
param P_TANK_VOLUMES_m3{S_TANKS,S_TIMES};

# The set of candidate booster nodes
# Note: if all nodes are valid booster candidates
#       then this can be left as defaulted
#       otherwise, it may be set based on 
#       non-zero demand
set S_BOOSTER_CANDIDATES default S_NODES;

###
# These variables are set in the run file
# (see examples)
###
# Maximum number of booster stations allowed
param P_MAX_STATIONS;

# Stoichiometric coefficient (mass booster agent / mass toxin)
param P_ALPHA, default 1;

###
# These variables are typically set in a 
# separate dat file or the run file if they 
# are small enough
# (see examples)
###
# This parameter could be different from the the size of the set
# S_SCENARIOS. It represents the actual number of scenarios, before any 
# possible aggregation, that is used to compute expected value.
param P_NUM_DETECTED_SCENARIOS;
set S_SCENARIOS ordered;
param P_SCEN_DETECT_TIMESTEP{S_SCENARIOS};
param P_SCEN_START_TIMESTEP{S_SCENARIOS};
param P_SCEN_END_TIMESTEP{S_SCENARIOS};
param P_WEIGHT{S_SCENARIOS} default 1.0/P_NUM_DETECTED_SCENARIOS;

# Booster injection profiles
# Booster station injection strength [g/min]
param P_INJ_STRENGTH_BOOSTER_gpmin;
# This is used to determine the amount of mass
# to inject for flowpaced injections.
# If the injections are simple mass type injections
# this parameter will be equal to 1.
param P_INJ_TYPE_MULTIPLIER_BOOSTER{S_BOOSTER_CANDIDATES, S_TIMES};
# Accounts for rounding in the start and stop timestep, otherwise 1
param P_TIMESCALE_FACTOR_BOOSTER{S_TIMES, S_SCENARIOS} default 1;
param P_INJ_START_TIMESTEP_BOOSTER{S_SCENARIOS};
param P_INJ_END_TIMESTEP_BOOSTER{S_SCENARIOS};

# Set the toxin injection profile for each of the scenarios
# Note: These default to zero, so you only need to 
#       set the values for the non-zero parts
# Toxin mass injection profile [g/min] 
param P_MN_TOXIN_gpmin{n in S_NODES, t in S_TIMES, s in S_SCENARIOS: t >= P_SCEN_START_TIMESTEP[s] and t <= P_SCEN_END_TIMESTEP[s]} default 0;

#
# Node population and dose data
#
set S_NZP_NODES default {};
param P_NODE_POPULATION{S_NZP_NODES} default 0.0;
param P_VOLUME_INGESTED_m3{S_NZP_NODES, S_TIMES} default 0.0;
param P_MAX_DOSE_g{S_NZP_NODES, S_SCENARIOS} default 0.0;
param P_DOSE_THRESHOLD_g default 0.0;
# relax max dose computation by some floating point tolerance
param EPS default 1e-9;
param REGULARIZATION_EPS = 1e-4;

###
# Model variables
###
# toxin concentration for each node/time/scenario [g/m^3]
var c_toxin_gpm3{n in S_NODES, t in S_TIMES, s in S_SCENARIOS: t >= P_SCEN_START_TIMESTEP[s] and t <= P_SCEN_END_TIMESTEP[s]} >= 0;

# booster agent concentration for each node/time/scenario [g/m^3]
var c_booster_gpm3{n in S_NODES, t in S_TIMES, s in S_SCENARIOS: t >= P_SCEN_START_TIMESTEP[s] and t <= P_SCEN_END_TIMESTEP[s]} >= 0;

# booster agent mass injection for each node/time/scenario [g/min]
var m_booster_gpmin{n in S_NODES, t in S_TIMES, s in S_SCENARIOS: t >= P_SCEN_START_TIMESTEP[s] and t <= P_SCEN_END_TIMESTEP[s]} >= 0;

# mass toxin removed by reaction for each node/time/scenario [g/min]
var r_toxin_gpmin{n in S_NODES, t in S_TIMES, s in S_SCENARIOS: t >= P_SCEN_START_TIMESTEP[s] and t <= P_SCEN_END_TIMESTEP[s]} >= 0;

# boooster station selection variable
var y_booster{S_BOOSTER_CANDIDATES} binary;

# Dose consumed
var dose_g{n in S_NZP_NODES, s in S_SCENARIOS} >= 0, <= P_MAX_DOSE_g[n,s]*(1+EPS);

# Dose above threshold binary var
var z_dosed{S_NZP_NODES,S_SCENARIOS} binary;

###
# Model objective and constraints
###
minimize MASS_CONSUMED_g: 
         P_MINUTES_PER_TIMESTEP*sum{s in S_SCENARIOS} 
         ( 
           P_WEIGHT[s]*sum{n in S_NODES, t in S_TIMES: t >= P_SCEN_START_TIMESTEP[s] 
                                                       and t <= P_SCEN_END_TIMESTEP[s]
                                                       and P_DEMANDS_m3pmin[n,t] > 0} 
                       (
                         P_DEMANDS_m3pmin[n,t]*c_toxin_gpm3[n,t,s]
                       ) 
         );
minimize POPULATION_DOSED:
         sum{n in S_NZP_NODES, s in S_SCENARIOS}(P_WEIGHT[s]*z_dosed[n,s]*P_NODE_POPULATION[n]) + 
         sum{n in S_NODES, t in S_TIMES, s in S_SCENARIOS: t >= P_SCEN_START_TIMESTEP[s] 
                                                           and t <= P_SCEN_END_TIMESTEP[s]}(c_toxin_gpm3[n,t,s])*REGULARIZATION_EPS;
s.t.
	TOXIN_MASS_BALANCE{n in S_NODES, t in S_TIMES, s in S_SCENARIOS: t >= P_SCEN_START_TIMESTEP[s] and t <= P_SCEN_END_TIMESTEP[s]}:
		sum{(nn,tt) in S_CONC_MATRIX_CSR_INDEX_TOXIN[n,t]: tt >= P_SCEN_START_TIMESTEP[s]}(P_CONC_MATRIX_CSR_TOXIN[n,t,nn,tt]*c_toxin_gpm3[nn,tt,s])
		+ P_INJ_MATRIX_DIAG_TOXIN[n,t]*(P_MN_TOXIN_gpmin[n,t,s]-r_toxin_gpmin[n,t,s]) = 0;

	BOOSTER_MASS_BALANCE{n in S_NODES, t in S_TIMES, s in S_SCENARIOS: t >= P_SCEN_START_TIMESTEP[s] and t <= P_SCEN_END_TIMESTEP[s]}:
		sum{(nn,tt) in S_CONC_MATRIX_CSR_INDEX_BOOSTER[n,t]: tt >= P_SCEN_START_TIMESTEP[s]}(P_CONC_MATRIX_CSR_BOOSTER[n,t,nn,tt]*c_booster_gpm3[nn,tt,s])
		+ P_INJ_MATRIX_DIAG_BOOSTER[n,t]*(m_booster_gpmin[n,t,s]-P_ALPHA*r_toxin_gpmin[n,t,s]) = 0;

	# when there is no flow across the node, P_INJ_MATRIX_DIAG is zero... therefore
	# that particular toxin_r_gpmin does not show up in the mass balance equations 
	# and we should set them to zero for consistency
	TOX_R_ZERO_IF_NO_FLOW_IN_NODE{n in S_NODES, t in S_TIMES, s in S_SCENARIOS: P_INJ_MATRIX_DIAG_TOXIN[n,t] == 0 and t >= P_SCEN_START_TIMESTEP[s] and t <= P_SCEN_END_TIMESTEP[s]}:
		r_toxin_gpmin[n,t,s] = 0;

	# Booster injection profile must align with booster station selection(s)
	BOOSTER_INJECTION{n in S_NODES, t in S_TIMES, s in S_SCENARIOS: t >= P_SCEN_START_TIMESTEP[s] and t <= P_SCEN_END_TIMESTEP[s]}:
		m_booster_gpmin[n,t,s] = (if n in S_BOOSTER_CANDIDATES and t >= P_INJ_START_TIMESTEP_BOOSTER[s] and t <= P_INJ_END_TIMESTEP_BOOSTER[s]
				then  y_booster[n]*P_INJ_STRENGTH_BOOSTER_gpmin*P_INJ_TYPE_MULTIPLIER_BOOSTER[n,t]*P_TIMESCALE_FACTOR_BOOSTER[t,s]
				else  0);

	# Only MAX_STATIONS number of booster stations allowed.
	BOOSTER_NODE:
		sum{b in S_BOOSTER_CANDIDATES}( y_booster[b] ) <= P_MAX_STATIONS;

        # Dose over all timesteps
        TOTAL_DOSE{n in S_NZP_NODES, s in S_SCENARIOS}:
                sum{t in S_TIMES: t >= P_SCEN_START_TIMESTEP[s] and t <= P_SCEN_END_TIMESTEP[s]}(c_toxin_gpm3[n,t,s]*P_VOLUME_INGESTED_m3[n,t]) = dose_g[n,s];

        # Dose above thresold Big-M constraint
        DOSE_BIGM_lower{n in S_NZP_NODES, s in S_SCENARIOS}:
                P_DOSE_THRESHOLD_g*z_dosed[n,s] <= dose_g[n,s];
        DOSE_BIGM_upper{n in S_NZP_NODES, s in S_SCENARIOS}:
                dose_g[n,s] <= z_dosed[n,s]*(P_MAX_DOSE_g[n,s]*(1+EPS) - P_DOSE_THRESHOLD_g) + P_DOSE_THRESHOLD_g;
