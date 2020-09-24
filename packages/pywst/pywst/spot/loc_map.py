#!/usr/bin/env python
#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________
#
# The aggregateImpacts executable creates superlocations composed
# of one or more real locations.  It writes two files:
#  problem_earliest.config
#  problem_map.config
#
# The first file maps each superlocation ID to the real location
# IDs that have earliest witness for each event the superlocation is
# defined for.
#
# The second file contains a full map - the full list of real locations
# that are represented by the superlocation.
#
# def location_map(superlocs, fearliest, fmap):
# 
# returns a list of original location IDs (the IDs that were in the
# impact files) for each super location ID in "superlocs".  It will
# read the open file "fearliest" to do the mapping.  This should usually
# work.  If it does not, it will read the open file "fmap" to map
# each superlocation ID to a real ID.
#
# It returns a list of original location IDs, in the same order as
# the corresponding super location ID in "superlocs".

import sys 

def min_len_in_dict(l):
  min = -1
  for lrec in l.keys():
    if (min == -1) or (len(l[lrec]) < min):
      min = len(l[lrec])
  return min

def make_list(superlocs, slassign):
  l = []
  for sl in superlocs:
    l.append(slassign[sl])
  return l

def location_map_trial(slIDs, origIDs):

  slAssign = {}
  origAssign = {}

  numsl = len(slIDs.keys())

  while len(slAssign.keys()) < numsl:
    newslids = set([])
    neworigids = set([])

    # process only slIDs with the shortest length list of origIDs

    shortestSL = min_len_in_dict(slIDs)

    if shortestSL == 1:
      for slid in slIDs.keys():
        lsize = len(slIDs[slid])
        if lsize == 1:
          oid = list(slIDs[slid])[0]
          if origAssign.has_key(oid):
            break
          origAssign[oid] = slid
          slAssign[slid] = oid
          newslids.add(slid)
          neworigids.add(oid)

    else:
      shortSLs = []
      oids = set([])
      for slid in slIDs.keys():
        if len(slIDs[slid]) == shortestSL:
          shortSLs.append(slid)
          oids = oids.union(slIDs[slid])

      # we want to assign an oid that is in fewest superlocations

      minoid = set([])
      minsize = -1
      for oid in oids:
        if (minsize == -1) or (len(origIDs[oid]) < minsize):
          minsize = len(origIDs[oid])
      for oid in oids:
        if len(origIDs[oid]) == minsize:
          minoid.add(oid)

      for slid in shortSLs:
        minids = slIDs[slid].intersection(minoid)
        if len(minids) == 0:
          continue
        oid = list(minids)[0]
        origAssign[oid] = slid
        slAssign[slid] = oid
        newslids.add(slid)
        neworigids.add(oid)
        break   

    if len(newslids) == 0:    # failure
      break       

    # remove the assigned SL and original ids from the lists

    for slid in newslids:
      del slIDs[slid]

    for origid in neworigids:
      del origIDs[origid]

    for slid in slIDs.keys():
      slIDs[slid] = slIDs[slid].difference(neworigids)
      if len(slIDs[slid]) == 0:
        break

    for origid in origIDs.keys():
      origIDs[origid] = origIDs[origid].difference(newslids)
      if len(origIDs[origid]) == 0:
        del origIDs[origid]

  if len(slAssign.keys()) < numsl:
    return {}

  return slAssign

def location_map(superlocs, fearliest, fmap):
  origlocs = []

  if len(superlocs) == 0:
    return origlocs

  # Map each superlocation to one of the original locations
  # it was created from.
  #
  # Try the list of "earliest impact" original locations first.
  # This is shorter and it should usually work.  If this fails,
  # use the list of all original locations for each superlocation.
  #
  # superlocs are given to us sorted in increasing order.
  # fearliest is also sorted in increasing slID order.
  # fmap is not in order

  slIDs = {}
  origIDs = {}
  idx = 0
  preamble = 1
  sl = superlocs[0]
  for line in fearliest.xreadlines():
    line = line.strip()
    tokens = line.split()
    if preamble:
      if line[0] == "#":   # comment
        continue
      if len(tokens) == 1: # count of sl IDs
        continue
    if int(tokens[0]) != sl:
      continue
    preamble = 0
    slIDs[sl] = set([])
    ids = tokens[3::3]   # real IDs
    for val in ids:
      id = int(val)
      if not id in slIDs[sl]:
        slIDs[sl].add(id)
        if origIDs.has_key(id):
          origIDs[id].add(sl)
        else:
          origIDs[id] = set([sl])
    idx = idx + 1
    if idx == len(superlocs):
      break
    sl = superlocs[idx]

  if len(slIDs.keys()) < len(superlocs):
    print "ERROR: Can't find all locations in earliest file"
    sys.exit(0)

  slAssign = location_map_trial(slIDs, origIDs)

  if len(slAssign.keys()) == len(superlocs):
    return make_list(superlocs,slAssign)

  slIDs = {}
  origIDs = {}
  preamble = 1
  nlocs = 0
  for line in fmap.xreadlines():
    line = line.strip()
    tokens = line.split()
    if preamble:
      if line[0] == "#":   # comment
        continue
      if len(tokens) == 1: # count of sl IDs
        continue
    sl = int(tokens[0])
    if not sl in superlocs:
      continue
    preamble = 0
    ids = tokens[1:]   # real IDs
    slIDs[sl] = set([])
    for val in ids:
      id = int(val)
      slIDs[sl].add(id)
      if origIDs.has_key(id):
        origIDs[id].add(sl)
      else:
        origIDs[id] = set([sl])
    nlocs = nlocs + 1
    if nlocs == len(superlocs):
      break

  if len(slIDs.keys()) < len(superlocs):
    print "ERROR: Can't find all locations in map file"
    sys.exit(0)

  slAssign = location_map_trial(slIDs, origIDs)

  if len(slAssign.keys()) != len(superlocs):
    print "ERROR: Unable to map superlocations to original locations"
    print "ERROR: If possible run your problem without aggregation (--disable-aggregation)"
    sys.exit(0)

  return make_list(superlocs,slAssign)
