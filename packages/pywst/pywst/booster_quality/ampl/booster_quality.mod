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
set S_NODES ordered;
# all time steps
set S_TIMES ordered; 

# The sparse indexing set of P_CONC_MATRIX_CSR
set S_CONC_MATRIX_CSR_INDEX{S_NODES, S_TIMES} dimen 2 default {};
# These two parameters form the water quality model
# G*c+D*m=0 
#   where c are the concentrations in g/m^3 (mg/L)
#   and m are the mass injection terms in g/min
# The concentration matrix (G) in CSR format
param P_CONC_MATRIX_CSR{n in S_NODES, t in S_TIMES, S_CONC_MATRIX_CSR_INDEX[n,t]};
# The injection matrix (D) which is diagonal
param P_INJ_MATRIX_DIAG{n in S_NODES, t in S_TIMES};


# Sets distinguishing node types within the network
set S_JUNCTIONS within S_NODES;
set S_TANKS within S_NODES;
set S_RESERVOIRS within S_NODES;


# The set of candidate booster nodes
# Note: if all nodes are valid booster candidates
#       then this can be left as defaulted
#       otherwise, it may be set based on 
#       non-zero demand
set S_BOOSTER_CANDIDATES ordered default S_NODES;

###
# These variables are set in the run file
# (see examples)
###
# Maximum number of booster station allowed
param P_MAX_STATIONS;
# Minimum allowable booster agent concentration
param P_MIN_CONC_BOOSTER_gpm3;
# Maximum allowable booster agent concentration
param P_MAX_CONC_BOOSTER_gpm3;
# source nodes (fixed concentration)
set S_SOURCES_BOOSTER within S_NODES;
# source water quality
param P_SOURCE_CONC_BOOSTER_gpm3;
# setpoint water quality
param P_SETPOINT_CONC_BOOSTER_gpm3;
# Start time in minutes of quality bound constraints
param P_QUALITY_START_BOOSTER_min;
# Timesteps after the start of quality bounds
set S_TIMES_QUALITY ordered := {t in S_TIMES: t*P_MINUTES_PER_TIMESTEP >= P_QUALITY_START_BOOSTER_min};

###
# These variables are typically set in a 
# separate dat file or the run file if they 
# are small enough
# (see examples)
###
# Maximum Booster injection profiles
# Booster station injection strength [g/min]
param P_INJ_STRENGTH_BOOSTER_gpmin;
# This is used to determine the amount of mass
# to inject for flowpaced injections.
# If the injections are simple mass type injections
# this parameter will be equal to 1.
param P_INJ_TYPE_MULTIPLIER_BOOSTER{S_NODES, S_TIMES};

###
# Model variables
###
# booster agent concentration for each node/time [g/m^3]
# ***Note: relax the minimum concentration bounds at reservoirs since
#          these always drop to zero when flow stops
var c_booster_gpm3{n in S_NODES, t in S_TIMES} >= 0;

# booster agent mass injection for each node/time [g/min]
var m_booster_gpmin{n in S_NODES, t in S_TIMES} >= 0;
var m_booster_gpmin_{n in S_BOOSTER_CANDIDATES} >= 0;

# boooster station selection variable
var y_booster{S_BOOSTER_CANDIDATES} binary;

# The variable maximum concentration (usually fixed in the run file)
var c_max_booster_gpm3 >= 0;

###
# Model objective and constraints
###
# MASS_INJECTED_BOOSTER_g
minimize MASS:
         P_MINUTES_PER_TIMESTEP * sum{n in S_BOOSTER_CANDIDATES, t in S_TIMES_QUALITY} m_booster_gpmin[n,t];
# CONCENTRATION_RESIDUAL_gpm3
minimize SETPOINT:
         1.0/card(S_NODES)*1.0/card(S_TIMES_QUALITY)*sum{n in S_NODES, t in S_TIMES_QUALITY} (c_booster_gpm3[n,t]-P_SETPOINT_CONC_BOOSTER_gpm3)^2;
# Find the lowest maximum concentration where the model is feasible
minimize FEASIBILITY:
         c_max_booster_gpm3;
         
s.t.

	BOOSTER_MASS_BALANCE{n in S_NODES, t in S_TIMES_QUALITY}:
		sum{(nn,tt) in S_CONC_MATRIX_CSR_INDEX[n,t]}(P_CONC_MATRIX_CSR[n,t,nn,tt]*c_booster_gpm3[nn,tt])
		+ P_INJ_MATRIX_DIAG[n,t]*m_booster_gpmin[n,t] = 0;

	# Booster injection profile must align with booster station selection(s) and not exceed booster station
        # injection limitations
	BOOSTER_INJECTION{b in S_BOOSTER_CANDIDATES, t in S_TIMES_QUALITY}:
		m_booster_gpmin_[b] <= y_booster[b]*P_INJ_STRENGTH_BOOSTER_gpmin;

	# Only MAX_STATIONS number of booster stations allowed.
	BOOSTER_NODE:
		sum{b in S_BOOSTER_CANDIDATES}( y_booster[b] ) <= P_MAX_STATIONS;

        CONCENTRATION_BOUNDS_NODES_MIN{n in S_NODES, t in S_TIMES_QUALITY}:
                P_MIN_CONC_BOOSTER_gpm3 <= c_booster_gpm3[n,t];

        CONCENTRATION_BOUNDS_NODES_MAX{n in S_NODES, t in S_TIMES_QUALITY}:
                c_booster_gpm3[n,t] <= c_max_booster_gpm3;

        #CONCENTRATION_CYCLICAL{n in S_NODES}:
        #        c_booster_gpm3[n,last(S_TIMES)] = c_booster_gpm3[n,first(S_TIMES_QUALITY)];
        #        -0.01 <= c_booster_gpm3[n,last(S_TIMES_QUALITY)]-c_booster_gpm3[n,first(S_TIMES_QUALITY)-1] <= 0.01;
        
        JUNK{b in S_BOOSTER_CANDIDATES, t in S_TIMES_QUALITY}:
               m_booster_gpmin[b,t] = m_booster_gpmin_[b]*P_INJ_TYPE_MULTIPLIER_BOOSTER[b,t];

        DUMMY{n in S_NODES, t in (S_TIMES diff S_TIMES_QUALITY)}:
               -0.001 <= c_booster_gpm3[n,t] - c_booster_gpm3[n,t+round(24*60/P_MINUTES_PER_TIMESTEP)-1] <= 0.001;
        