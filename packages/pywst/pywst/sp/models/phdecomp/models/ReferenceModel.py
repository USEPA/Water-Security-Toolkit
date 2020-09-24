#
# DLW version of sensor placement p-median formulation
#

from pyomo.environ import *

#
# Model
#

model = Model()

#
# Parameters and Sets
#

# \mathcal{E}
model.Events = Set()

# \mathcal{L}
model.Junctions = Set()

# q (a set element to add to Junctions
qMember = 'q'

model.ReachableJunctions = Set(model.Events)

# \mathcal{L} \union q
def JPD_init(i, model):
   result = []
   for junction in model.ReachableJunctions[i]:
      result.append(junction)
   result.append('q')
   return result
model.ReachableJunctionsWithDummy = Set(model.Events, initialize=JPD_init)

# p: Sensors Available to Place
model.NumSensorsAvailable = Param(within=Integers)

# d_ej
model.DiscoveryHarm = Param(model.Events, model.Junctions, within=NonNegativeReals)

# sparse index set for detection (x) variables and associated constraint.
def event_cross_junction_index_rule(model):
   result = []
   for event in model.Events:
      for junction in model.ReachableJunctions[event]:
         new_tuple = (event, junction)
         result.append(new_tuple)
   return result
model.EventCrossJunctionIndex = Set(dimen=2, initialize=event_cross_junction_index_rule)

def event_cross_junction_with_q_index_rule(model):
   result = []
   for event in model.Events:
      for junction in model.ReachableJunctionsWithDummy[event]:
         new_tuple = (event, junction)
         result.append(new_tuple)
   return result
model.EventCrossJunctionWithQIndex = Set(dimen=2, initialize=event_cross_junction_with_q_index_rule)

#
# Variables
#

model.x = Var(model.EventCrossJunctionWithQIndex, bounds=(0.0,1.0))

model.y = Var(model.Junctions, domain=Boolean)

# "hidden" vars to allow for stage by stage reporting of the objective

model.FirstStageCost = Var()
model.SecondStageCost = Var()

#
# Objective, etc.
#

def first_stage_cost_rule(model):
   return(0.0, model.FirstStageCost, 0.0)
model.ComputeFirstStageCost = Constraint(rule=first_stage_cost_rule)

def second_stage_cost_rule(model):
   retval = summation(model.DiscoveryHarm, model.x)
   return model.SecondStageCost - retval == 0.0
model.ComputeSecondStageCost = Constraint(rule=second_stage_cost_rule)

def total_cost_rule(model):
   return model.SecondStageCost

model.TotalCostObjective = Objective(rule = total_cost_rule, sense=minimize)


#
# Constraints
#

# every event gets discovered first by some sensor, but maybe the dummy
def coverdiscover_rule(event, model):
   testval = sum([model.x[event, i] for i in model.ReachableJunctionsWithDummy[event]])
   return testval == 1
model.coverConstraint = Constraint(model.Events, rule=coverdiscover_rule)

# assign discovery to real sensors only where there is a sensor
def discoverjunction_rule(event, junction, model):
   return model.x[event, junction] < model.y[junction]
model.discatjuncConstraint = Constraint(model.EventCrossJunctionIndex, rule=discoverjunction_rule)

# capacity
def p_rule(model):
   return summation(model.y) <= model.NumSensorsAvailable
model.pConstraint = Constraint(rule=p_rule)

