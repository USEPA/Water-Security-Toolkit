#
# 2-stage sensor placement derived from p-median formulation
#

from pyomo.environ import *

#
# Model
#

model = AbstractModel()

#
# Parameters and Sets
#

model.alpha = Param(within=PercentFraction, default=0.5)

# Available sensors
model.NumStage1Sensors = Param(within=PositiveIntegers)
model.NumStage2Sensors = Param(within=NonNegativeIntegers)

# Index of events and reachable junctions
model.EventsXJunctions = Set(dimen=2)

model.Impact = Param(model.EventsXJunctions, within=NonNegativeReals)

def Events_rule(model):
    ans = set()
    for (e,j) in model.EventsXJunctions:
        ans.add(e)
    return ans
model.Events = Set(initialize=Events_rule)

def Junctions_rule(model):
    ans = set()
    for (e,j) in model.EventsXJunctions:
        if j != -1:
            ans.add(j)
    return ans
model.Junctions = Set(initialize=Junctions_rule)

# NOTE: this may be slow.  This data should be generated in the *.dat file.
def ReachableJunctions_rule(model, e):
    return [j for (e_,j) in model.EventsXJunctions if e_ == e]
model.ReachableJunctions = Set(model.Events, initialize=ReachableJunctions_rule)

def num_events_rule(model):
    return len(model.Events)
model.num_events = Param(initialize=num_events_rule)

#
# Variables
#

# Witness variables
model.x1 = Var(model.EventsXJunctions, bounds=(0.0,1.0))
model.x2 = Var(model.EventsXJunctions, bounds=(0.0,1.0))

# Sensor variables
model.Stage1Sensors = Var(model.Junctions, domain=Boolean)
model.Stage2Sensors = Var(model.Junctions, domain=Boolean)

# hidden variables to allow for stage by stage reporting of the objective
model.FirstStageCost = Var()
model.SecondStageCost = Var()


#
# Objective, etc.
#

def ComputeFirstStageCost_rule(model):
   return model.FirstStageCost == summation(model.Impact, model.x1) / model.num_events
model.ComputeFirstStageCost = Constraint()

def ComputeSecondStageCost_rule(model):
   return model.SecondStageCost == summation(model.Impact, model.x2) / model.num_events
model.ComputeSecondStageCost = Constraint()

def TotalCostObjective_rule(model):
   return (1-model.alpha)*model.FirstStageCost + model.alpha * model.SecondStageCost
model.TotalCostObjective = Objective(sense=minimize)


#
# Constraints
#

# every event is witnessed first by some sensor, but maybe the dummy
def cover1Constraint_rule(model, e):
    return sum([model.x1[e, j] for j in model.ReachableJunctions[e]]) == 1
model.cover1Constraint = Constraint(model.Events)

def cover2Constraint_rule(model, e):
    return sum([model.x2[e, j] for j in model.ReachableJunctions[e]]) == 1
model.cover2Constraint = Constraint(model.Events)

# assign the witness to real sensors only where there is a sensor
def discatjunc1Constraint_rule(model, e, j):
    if j == -1:
        return Constraint.Skip
    return model.x1[e, j] <= model.Stage1Sensors[j]
model.discatjunc1Constraint = Constraint(model.EventsXJunctions)

def discatjunc2Constraint_rule(model, e, j):
    if j == -1:
        return Constraint.Skip
    return model.x2[e, j] <= model.Stage2Sensors[j]
model.discatjunc2Constraint = Constraint(model.EventsXJunctions)

# staging
def stage_rule(model, j):
    return model.Stage2Sensors[j] >= model.Stage1Sensors[j]
model.stage = Constraint(model.Junctions)

# capacity
def p1Constraint_rule(model):
   return summation(model.Stage1Sensors) <= model.NumStage1Sensors
model.p1Constraint = Constraint()

def p2Constraint_rule(model):
   return summation(model.Stage2Sensors) <= model.NumStage1Sensors + model.NumStage2Sensors
model.p2Constraint = Constraint()

