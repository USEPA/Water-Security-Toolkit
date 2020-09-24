from pyomo.environ import*

model = AbstractModel()

#SETS
# set of events
model.S_EVENTS = Set()
# set of grab sample locations
model.S_LOCATIONS = Set()
# pair-wise set of all candidate events. The number of elements of this set is S_EVENTS combined 2
model.S_PAIR_WISE = Set(dimen=2)
# set of sample locations that distinguish i and j
model.S_DISTINGUISH_LOCATIONS = Set(model.S_PAIR_WISE)
# set of fixed sensor locations
model.S_FIXED_LOCATIONS = Set()

#PARAMETERS
# Maximum number of samples per cycle
model.P_MAX_SAMPLES = Param(default=0.9)

model.P_WEIGHT_FACTORS = Param(model.S_PAIR_WISE,default=1.0)

#VARIABLES
# Binary variables. 1-> sampled node, 0-> otherwise
model.s = Var(model.S_LOCATIONS, within=Binary)
# Continuous variables. 1->if event i is distinguishable from event j, 0->otherwise
model.d = Var(model.S_PAIR_WISE,within=NonNegativeReals)

#Objective function
def obj(model):
    return sum(model.P_WEIGHT_FACTORS[i,j]*model.d[i,j] for (i,j) in model.S_PAIR_WISE)

model.OBECTIVE = Objective(rule=obj, sense=maximize)

#CONSTRAINS
#Number of samples to be taken
def num_samples(model):
    return sum(model.s[n] for n in model.S_LOCATIONS) <= model.P_MAX_SAMPLES

model.MAX_SAMPLES = Constraint(rule=num_samples)

# Last constraint
def named(model,i,j):
    return sum(model.s[n] for n in model.S_DISTINGUISH_LOCATIONS[i,j]) >= model.d[i,j]

model.TO_BE_NAMED = Constraint(model.S_PAIR_WISE, rule=named)

# Upper boundary of dij

def upper(model,i,j):
    return model.d[i,j] <=1

model.UPPER = Constraint(model.S_PAIR_WISE, rule=upper)

# Fixed sensor locations are always selected

def fixed(model,i):
    return model.s[i] == 1

model.FIXED = Constraint(model.S_FIXED_LOCATIONS, rule=fixed)
