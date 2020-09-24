# MILP Model for the grab sample problem

# set of events
set S_EVENTS;
# set of grab sample locations
set S_LOCATIONS;
# set of fixed sensor locations
set S_FIXED_LOCATIONS;
# pair-wise set of all candidate events. The number of elements of this set is S_EVENTS combined by 2 
set S_PAIR_WISE within {S_EVENTS,S_EVENTS};
# set of sample locations that distinguish i and j
set S_DISTINGUISH_LOCATIONS{S_PAIR_WISE};

# Maximum number of samples per cycle
param P_MAX_SAMPLES;
param P_WEIGHT_FACTORS{S_PAIR_WISE} default 1.0;

# Binary variables. 1-> sampled node, 0-> otherwise
var s{S_LOCATIONS} binary;
# Continuous variables. 1->if event i is distinguishable from event j, 0->otherwise
var d{S_PAIR_WISE} >=0, <=1;

# Objective function

maximize OBJECTIVE: sum{(i,j) in S_PAIR_WISE} d[i,j];

s.t. 
	MAX_SAMPLES:
		sum{n in S_LOCATIONS} s[n] <= P_MAX_SAMPLES; 
	
	TO_BE_NAMED{(i,j) in S_PAIR_WISE}:
      		sum{n in S_DISTINGUISH_LOCATIONS[i,j]} s[n] >= d[i,j];
	FIXED_SENSORS{n in S_FIXED_LOCATIONS}:
	        s[n] = 1; 
