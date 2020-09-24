#
# Version 2.2
#
# This is a second model for sensor placement in water networks.  We wish to 
# decouple sensor placement from assumptions of network flow. We are minimizing the
# (expected) consumption of contaminants as a surrogate for population exposure
# (in this model, it's difficult to get the necessary population data).
# A detailed description will appear in "A General Integer Programming Based Framework for
# Sensor Placement in Municipal Water Networks", at the Symposium on Water Distribution Systems
# Analysis (Salt Lake City, UT, June 2004).
#
# I'm starting with the old EWRI 2003 model since we should have graph .dat files to conform
# to this.
#
# For all sets that are just consecutive sets of integers, we keep flexibility 
# in setting the interval start.  This can be useful when interfacing to other 
# codes and people.


#
# The water network elements. These are locations in the network where we can
# put sensors.  For now just number them consecutively.  
# Network elements are consecutive integers.  These are the node indices;

param numTrueNetworkLocations > 0;
param Loc1st;
# We'll also have a dummy network location, used to indicate no 
# detection for a given scenario

param dummyLocation := Loc1st + numTrueNetworkLocations;
set TrueLocations := Loc1st .. dummyLocation;

set SuperLocations;

set SuperLocationMembers {SuperLocations};

param Budget;

# Event scenarios are defined by an event location (for us, this will always be a node)
# and a time of day.  However, this model doesn't need to know these, so we'll
# just label events consecutively

param numEvents > 0;
param Event1st;
set Events := Event1st .. Event1st + numEvents-1;

# Event weights.  You can think of this as a probability that the
# (single-location) event occurs at this location and time

param EventWeight{Events} >= 0 <= 1;

# The maximum number of sensors available.

param numSensors;

# Locations touched by contaminants in a particular event according to the simulation.  For the
# first pass, this is likely to be any location touched within a specified time window such as
# three days. This set is likely to  be sparse in real networks. 
# Remember that the dummyLocation is touched by all events.

set EventTouchedLocPair within Events cross SuperLocations;

# Used in the objective function.
# ConsumptionBeforeAlarmAt[AnEventTouchedLocPair] is the amount of
# water (in gallons, e.g) that is consumed before detection if the
# event specified in this (event, touched location) pair occurs and
# the contamination is first discovered at this location.

param ConsumptionBeforeAlarmAt{EventTouchedLocPair};
param LocPlacementCost{TrueLocations};

#
# The following variables are not used in the computation; they are 
# modeled to ease the transition to the validation phase post-solution.
#

param EventLoc  {Events};
param EventTime {Events};

# the variables

# s[loc] = 1 if there's a sensor at location loc, 0 otherwise.
# Sensors are can detect in any direction.

var s{TrueLocations} binary;

# FirstSensorForEvent[EventTouchedLocPair] = 1 if and only if this
# touched location is the first sensor hit for this event (among
# locations chosen to receive sensors, this one is the first hit by
# contamination among all those hit by this event.  These will be
# binary in practice (as needed) as long as the sensor placement
# decisions are binary.

var FirstSensorForEvent{EventTouchedLocPair} >= 0 <= 1;

# Objective

minimize volumeConsumed:
        sum{(a,l) in EventTouchedLocPair} (EventWeight[a] * ConsumptionBeforeAlarmAt[a,l] *
        FirstSensorForEvent[a,l]);

# For each event, one sensor is tripped first (this could be the dummy, indicating no real sensor
# detects the event

subject to pickFirstSensor{a in Events}:
        sum{(a,l) in EventTouchedLocPair} FirstSensorForEvent[a,l] = 1;

# A sensor on a real location can't be the first tripped if that location isn't selected

subject to onlyPickReal{(a,l) in EventTouchedLocPair: l != dummyLocation}:
        FirstSensorForEvent[a,l] <= sum {i in SuperLocationMembers[l]} s[i];

# The dummy location doesn't count as a real sensor.  It can be chosen
# for free to satisfy the first constraint, but it'll only be chosen
# if absolutely required to meet that constraint since it will
# contribute more to the objective than any other valid sensor
# placement for that event.

subject to limitedSensors:
  sum{l in TrueLocations: l != dummyLocation} 
			s[l]*LocPlacementCost[l] <= Budget;
