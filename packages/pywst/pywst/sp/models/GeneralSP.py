#
# GeneralSP.py
#
# AMPL model for sensor placement that uses general impact factors
# and cost measures.
#
# This MILP model for sensor placement in water networks generalizes
# the p-median formulation in Dynamic.mod to allow for an arbitrary
# specification of the objective function as well as constraints.
# This model is only meaningful if either the objective is to minimize
# total cost (e.g.  number of sensors, or if the model is constrained
# by total cost.
# 
# This model allows for the following performance goals:
#	cost
#	nfd
#	ns
#   awd
# along with goals associated with any impact file.
# Note: we want to minimize all of these goals!
#
#
# This model allows for the computation of a variety of performance
# measures for these goals:
#	mean performance
#	var (value-at-risk)
#	cvar (conditional value-at-risk)
#	worst performance
#	total value
#

from pyomo.environ import *

model = AbstractModel()

##
## Objective information
##
#
# Generic goals
#
model.generic_goals = Set(initialize=["cost", "nfd", "ns"])
#
# The set of allowable performance goals
#
model.impacts = Set(initialize=set(["cost", "ec", "mc", "nfd", "ns", "pe", "pd", "pk", "td", "vc", "dec", "dmc", "dpe", "dpk", "dpd", "dtd", "dvc", "pd0", "pd1", "pd2", "pd3", "pd4", "pd5", "pd6", "pd7"]))
#
# The goals
#
model.goals = model.generic_goals | model.impacts
#
# Goal measures
# 
model.measures = Set(initialize=["mean", "var", "cvar", "worst", "total"], within=Any)
#
# The optimization objective's goal
#
model.objectiveGoal = Param(within=Any)
#
# The optimization objective's measure
#
model.objectiveMeasure = Param(within=model.measures, default="mean")
#
# The goals used for constraints
#
# Set(within=model.goals * model.measures - {(objectiveGoal, objectiveMeasure)};
#
model.goalConstraints = Set(dimen=2)
#
# Active goals
#
def ActiveGoals_rule(model):
    data = set([g for (g,m) in model.goalConstraints])
    data.add(value(model.objectiveGoal))
    return data - set(['ns','cost','awd'])
model.ActiveGoals = Set(initialize=ActiveGoals_rule) 
#
# The active goal/measure pairs
#
model.ActiveGMPair_aux = Set(initialize=lambda model:[(value(model.objectiveGoal), value(model.objectiveMeasure))], dimen=2)
model.ActiveGMPair = model.goalConstraints | model.ActiveGMPair_aux
#
# Thresholds used for computing superlocations.
# NOTE: this information is not required for optimization, but it provides information about how
# aggregation is done.  We used to require that this be non-negative.  Now we allow negative values
# to signal that aggregation has been disabled.
#
model.slThreshold = Param(model.ActiveGoals)
#
# Ratio of best member in all superlocations to the superlocation value.
# NOTE: this information is not required for optimization, but it can be used to compute
# a valid lower bound when no side-constraints are used.
#
model.slAggregationRatio = Param(model.ActiveGoals, within=PercentFraction)
#
# The number of weight categories used for the awd measure
#
model.NumWeightCategories = Param(default=1)
model.WEIGHT_CATEGORIES = RangeSet(model.NumWeightCategories)
#
# The target weighted distribution
#
model.TargetWeightDistribution = Param(model.WEIGHT_CATEGORIES, within=PercentFraction, default=1.0)
#
# CHECK ERRORS
#
##check {(g,m) in ActiveGMPair}: ((m = "total") && (g in {"ns","cost","awd"})) || ((m != "total") && !(g in {"ns","cost","awd"}));

##
## The water network elements. These are locations in the network where we can
## put sensors.  For now just number them consecutively.  
## Network elements are consecutive integers.  These are the node indices;
##
#
# Number of network locations
#
model.numNetworkLocations = Param(within=PositiveIntegers)

# A dummy location where that all events impact
#
model.dummyLocation = Param(within=Integers, default=-1)
model.dummyLocationSet = Set(initialize=lambda model: [value(model.dummyLocation)])
#
# Places where sensors can be placed.
#
def NetworkLocations_values(model):
    return sequence(1, value(model.numNetworkLocations))
model.NetworkLocations = Set(initialize=NetworkLocations_values)
#
# Predetermined fixed sensor locations
#
model.FixedSensorLocations = Set(within=model.NetworkLocations, initialize=SetOf([], dimen=1))
#
# Predetermined locations where sensors cannot be placed
#
model.InfeasibleLocations = Set(within=model.NetworkLocations, initialize=SetOf([], dimen=1))
#
# The set of feasible sensor locations
#
def _FeasibleLocations(model):
    return model.NetworkLocations - model.InfeasibleLocations
model.FeasibleLocations = Set(within=model.NetworkLocations, initialize=_FeasibleLocations)

#
# FEASIBILITY CHECKS
#
#check: FixedSensorLocations intersect InfeasibleLocations within {};


##
## Constraint information
##
#
# Bounds
#
model.GoalBound = Param(model.goalConstraints)
#
# The confidence interval, e.g. consider the worst 5% of cases
#
model.gamma = Param(within=PercentFraction, default=0.05)
#
# The per-location weight distribution
#
model.LocationWeightDistribution = Param(model.FeasibleLocations, model.WEIGHT_CATEGORIES, within=PercentFraction, default=1)


def goal_event_index(model):
    for g in model.ActiveGoals:
        for a in model.Events[g]:
            yield (g,a)
goal_event_index.dimen = 2

##
## Event scenarios are defined by an event location (for us, this will always be a node)
## and a time of day.  However, this model doesn't need to know these, so we'll
## just label events consecutively
##
#
# Number of events
#
model.numEvents = Param(model.ActiveGoals, within=PositiveIntegers)
#
# The set of event IDs
#
def Events_rule(model, g):
    return range(1, value(model.numEvents[g])+1)
model.Events = Set(model.ActiveGoals, initialize=Events_rule)
#
# Event weights.  You can think of this as a probability that the
# (single-location) event occurs at this location and time
#
def EventWeight_default(model, g, a):
    return 1.0/value(model.numEvents[g])
model.EventWeight = Param(goal_event_index, within=PercentFraction, initialize=EventWeight_default)
#
# Event location information (used only for printing sensible outputs (e.g. debugging))
#
model.EventLoc = Param(goal_event_index, default=0)
#
# Event time information (used only for printing sensible outputs (e.g. debugging))
#
model.EventTime = Param(goal_event_index, default=0)

##
## Revised location information using event information
##
#
# Locations that could actually see the impact of each event
# Remember that the dummyLocation is touched by all events.
#
model.EventTouchedLocations = Set(goal_event_index, within=model.NetworkLocations | model.dummyLocationSet)
#
# The IDs of aggregated network location classes.
# NOTE: these are independent of the event scenario, to facilitate 
# data compression.
#
model.SuperLocations = Set()
#
# The set of network locations that correspond to each super location 
#
model.SuperLocationMembers = Set(model.SuperLocations, within=model.NetworkLocations | model.dummyLocationSet)
#
# For each goal and event, the set of SuperLocation IDs that define that event
#
model.EventSuperLocations = Set(goal_event_index, within=model.SuperLocations)
#
# TODO: add checks here?
#

##
## Goal information
##
#
# The goal values used in either the objective or constraints.
#
def GoalMeasurement_index(model):
    for g in model.ActiveGoals:
        for a in model.Events[g]:
            for l in model.EventTouchedLocations[g,a]:
                yield (g,a,l)
GoalMeasurement_index.dimen=3
model.GoalMeasurement = Param(GoalMeasurement_index)
#
# The cost information used to compute the 'cost' goal
#
model.PlacementCost = Param(model.FeasibleLocations, default=0.0)
#
# ID of the element of a superlocation set that has the worst value
#
def validate_WorstSuperLocationMember(model, value, g, a, l):
    return value in model.SuperLocationMembers[l]
model.WorstSuperLocationMember = Param(goal_event_index, model.SuperLocations, validate=validate_WorstSuperLocationMember)
#
# Some data checks
#
#check {g in ActiveGoals, a in Events[g], L in EventSuperLocations[g,a], LL in EventSuperLocations[g,a]}: (L == LL) or (L != LL) and SuperLocationMembers[L] intersect SuperLocationMembers[LL] within {};

##
## Decision variables
##
#
# s[loc] = 1 if there's a sensor at location loc, 0 otherwise.
#
model.s = Var(model.FeasibleLocations, within=Binary)
#
# FirstSensorForEvent[EventTouchedLocations] = 1 if and only if this
# touched location is the first sensor hit for this event (among
# locations chosen to receive sensors, this one is the first hit by
# contamination among all those hit by this event.  These will be
# binary in practice (as needed) as long as the sensor placement
# decisions are binary.
#
def FirstSensorForEvent_index(model):
    for g in model.ActiveGoals:
        for a in model.Events[g]:
            for l in model.EventSuperLocations[g,a]:
                yield (g,a,l)
FirstSensorForEvent_index.dimen=3
model.FirstSensorForEvent = Var(FirstSensorForEvent_index, bounds=(0,1))
#
# Goal variable values
#
model.GoalValue = Var(model.ActiveGMPair)
#
# Variables used to compute the worst-case goal measurement
#
def WorstGoalMeasurement_index(model):
    for g in model.ActiveGoals:
        if (g,'worst') in model.goalConstraints:
            yield g
model.WorstGoalMeasurement = Var(WorstGoalMeasurement_index)
#
# Variables used to compute the Absolute Deviation goal
#
##var AbsDevWeightDistribution {1..NumWeightCategories} >=0;
##model.AbsDevWeightDistribution = Var(model.WEIGHT_CATEGORIES, within=NonNegativeReals)

#
# For CVaR we need to know both Value-At-Risk, VaR, and the extent to which VaR
# is exceeded in a given event, y. By definition, VaR <= CVaR and, for small
# Gamma ( < 50% ), VaR >= expected value.  Note, we need these variables for
# each goal/cvar pair.
#
def VaR_index(model):
    for g in model.ActiveGoals:
        if (g,'cvar') in model.ActiveGMPair:
            yield g
model.VaR = Var(VaR_index)
#
def y_index(model):
    for g in model.ActiveGoals:
        for a in model.Events[g]:
            if (g,'cvar') in model.ActiveGMPair:
                yield (g,a)
y_index.dimen=2
model.y = Var(y_index, within=NonNegativeReals)

##
## OBJECTIVE
##
def obj_rule(model):
    return model.GoalValue[value(model.objectiveGoal), value(model.objectiveMeasure)]
model.obj = Objective(rule=obj_rule)

##
## Set the fixed locations
##
def FixedSensors_rule(model, l):
    return model.s[l] == 1
model.FixedSensors = Constraint(model.FixedSensorLocations, rule=FixedSensors_rule)
    
##
## For each event, one sensor is tripped first (this could be the dummy, indicating no real sensor
## detects the event
##
def pickFirstSensor_rule(model, g, a):
    return sum(model.FirstSensorForEvent[g,a,L] for L in model.EventSuperLocations[g,a]) == 1
model.pickFirstSensor = Constraint(goal_event_index, rule=pickFirstSensor_rule)
##
## A sensor on a real location can't be the first tripped if that location isn't selected
##
def onlyPickReal_index(model):
    for g in model.ActiveGoals:
        for a in model.Events[g]:
            for L in model.EventSuperLocations[g,a]:
                if not value(model.dummyLocation) in model.SuperLocationMembers[L]:
                    yield (g,a,L)
onlyPickReal_index.dimen=3
def onlyPickReal_rule(model, g, a, L):
    return model.FirstSensorForEvent[g,a,L] <= sum(model.s[l] for l in model.SuperLocationMembers[L])
model.onlyPickReal = Constraint(onlyPickReal_index, rule=onlyPickReal_rule)

##
## A dummy location can't be the first tripped if a sensor has already selected this event
##
def limitDummySelection_index(model):
    for g in model.ActiveGoals:
        for a in model.Events[g]:
            for L in model.EventSuperLocations[g,a]:
                for l in model.FeasibleLocations:
                    if not value(model.dummyLocation) in model.SuperLocationMembers[L]:
                        continue
                    for LL in model.EventSuperLocations[g,a]:
                        if LL == L:
                            continue
                        if l in model.SuperLocationMembers[LL]:
                            yield (g,a,L,l)
                            break
limitDummySelection_index.dimen=4
def limitDummySelection_rule(model, g, a, L, l):
    return model.FirstSensorForEvent[g,a,L] <= 1 - model.s[l]
model.limitDummySelection = Constraint(limitDummySelection_index, rule=limitDummySelection_rule)

##
## Mean Goal Value
##
def meanGoals_index(model):
    for (g,m) in model.ActiveGMPair:
        if m=='mean' and g!='nfd':
            yield (g,m)
meanGoals_index.dimen=2
def meanGoals_rule(model, g, m):
    return model.GoalValue[g,m] == \
        sum(model.EventWeight[g,a] * \
            sum(model.GoalMeasurement[g, a, model.WorstSuperLocationMember[g,a,L]] * \
                model.FirstSensorForEvent[g,a,L] for L in model.EventSuperLocations[g,a]) \
            for a in model.Events[g])
model.meanGoals = Constraint(meanGoals_index, rule=meanGoals_rule)
##
## Mean nfd Goal Value
##
def nfdMeanGoal_rule(model):
    if not ('nfd','mean') in model.ActiveGMPair:
        return Constraint.Skip
    return model.GoalValue['nfd','mean'] >= \
            sum(model.FirstSensorForEvent[objectiveGoal,a,L] if value(model.dummyLocation) in model.SuperLocationMembers[L] else 0.0 \
                for a in model.Events[model.objectiveGoal] \
                for L in model.EventSuperLocations[model.objectiveGoal,a])
model.nfdMeanGoal = Constraint(rule=nfdMeanGoal_rule)

##
## Worst Goal Value
##
def worstGoals_index(model):
    for (g,m) in model.ActiveGMPair:
        #if m == 'worst' and g not in ('ns','cost','awd'):
        #    yield (g,m)
        if m != 'worst':
            continue
        if g in ['ns','cost','awd']:
            continue
        for a in model.Events[g]:
            yield (g,m,a)
worstGoals_index.dimen=3
def worstGoals_rule(model, g, m, a):
    return sum(model.GoalMeasurement[g,a,model.WorstSuperLocationMember[g,a,L]] * \
                                model.FirstSensorForEvent[g,a,L] for L in model.EventSuperLocations[g,a]) <= model.GoalValue[g,m]
model.worstGoals = Constraint(worstGoals_index, rule=worstGoals_rule)

##
## total ns Goal Value
##
def nsGoal_rule(model):
    if not ('ns','total') in model.ActiveGMPair:
        return Constraint.Skip
    return model.GoalValue['ns','total'] >= sum(model.s[l] for l in model.FeasibleLocations)
model.nsGoal = Constraint(rule=nsGoal_rule)

##
## total cost Goal Value
##
def costGoal_rule(model):
    total_cost = sum(model.PlacementCost[l]*model.s[l] for l in model.FeasibleLocations)
    if type(total_cost) in [int, float]:
        # If we get here, then total_cost is zero
        return Constraint.Skip
    if ('cost','total') in model.ActiveGMPair:
        return model.GoalValue['cost','total'] >= total_cost
    else:
        return sum(model.PlacementCost[l] for l in model.FeasibleLocations) >= total_cost
model.costGoal = Constraint(rule=costGoal_rule)

##
## Per-location absolute deviation of weight distribution
##
#subject to awdCalc1{i in 1..NumWeightCategories}:
#	if (("ns","total") in goalConstraints)
#	   then AbsDevWeightDistribution[i] * GoalBound["ns","total"] 
#	   else 0.0
#	>=
#	if (("ns","total") in goalConstraints)
#	   then TargetWeightDistribution[i] - (sum{l in FeasibleLocations}s[l]*LocationWeightDistribution[l,i])
#	   else 0.0;
#
#subject to awdCalc2{i in 1..NumWeightCategories}:
#	if (("ns","total") in goalConstraints)
#	   then AbsDevWeightDistribution[i] * GoalBound["ns","total"]
#	   else 0.0
#	>=
#	if (("ns","total") in goalConstraints)
#	   then - TargetWeightDistribution[i] + (sum{l in FeasibleLocations}s[l]*LocationWeightDistribution[l,i])
#	   else 0.0;

##
## total awd Goal Value
##
#subject to awdGoal:
#	if (("awd","total") in ActiveGMPair)
#	   then GoalValue["awd","total"]
#	   else 0.0
#	>=
#	if (("awd","total") in ActiveGMPair)
# 	   then sum{i in 1..NumWeightCategories} AbsDevWeightDistribution[i]
#	   else 0.0;

##
## CVaR Goal Value
##
def cvarGoals_index(model):
    for (g,m) in model.ActiveGMPair:
        if m == 'cvar':
            yield (g,m)
cvarGoals_index.dimen=2
def cvarGoals_rule(model, g, m):
    return model.GoalValue[g,m] == model.VaR[g] + (1.0/model.gamma) * \
                sum(model.EventWeight[g,a] * model.y[g,a] for a in model.Events[g])
model.cvarGoals = Constraint(cvarGoals_index, rule=cvarGoals_rule)
##
## Ylimit - set y to the max of excess consumption above VaR or 0.
##
def yplus_index(model):
    for g in model.ActiveGoals:
        for a in model.Events[g]:
            if (g,'cvar') in model.ActiveGMPair:
                yield (g,a)
yplus_index.dimen=2
def yplus_rule(model, g, a):
    return model.y[g,a] >= sum(model.GoalMeasurement[g,a,model.WorstSuperLocationMember[g,a,L]]*model.FirstSensorForEvent[g,a,L] for L in model.EventSuperLocations[g,a]) - model.VaR[g]
model.yplus = Constraint(yplus_index, rule=yplus_rule)

##
## Constraint limits
##
def goalBounds_rule(model, g, m):
    return model.GoalValue[g,m] <= model.GoalBound[g,m]
model.goalBounds = Constraint(model.goalConstraints, rule=goalBounds_rule)

