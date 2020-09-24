/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README file in the top software directory.
 *  _________________________________________________________________________
 */

#ifndef tevasp_Detection_h
#define tevasp_Detection_h

#ifdef _MSC_VER
#include <algorithm>
#endif

#include <iostream>
#include <tevautil.h>
//#include <tevatypes.h>
#include <vector>
#include <cmath>

class Detection {

protected:
  PNetInfo net;
  PSourceData source;
  PNodeInfo node;
  PLinkInfo link;
  double detectionDelay;   // minutes
  double reportStep;       // minutes
  int nSteps;
  int detectionDelaySteps;
  int speciesIndex;

public:
  /// Constructor
  Detection(void): net(NULL), source(NULL), node(NULL), link(NULL), detectionDelay(0.0), speciesIndex(0) {}

  /// Destructor
  ~Detection(void){}

  /// Provide event scenario.  Detection object assumes these
  /// pointers point to valid network and event data when the
  /// Calculate method is called.

  void SetNetwork(PNetInfo n){ 
    net = n;
    reportStep = int(n->stepSize * 60);
    detectionDelaySteps = int(ceil(detectionDelay/reportStep));
    nSteps = n->numSteps;
  }
  void SetSpeciesIndex(int si){ speciesIndex = si;}
  void SetSources(PSourceData s){ source = s;}
  void SetNodes(PNodeInfo n){ node = n;}
  void SetLinks(PLinkInfo l){ link = l;}
  void SetDetectionDelay(double dd){ detectionDelay = dd;}

  /// Calculate event detection at nodes and links.
  virtual int Calculate() = 0;

  /// For each time step, list of nodes that detect first at that time
  std::vector< std::vector<int> > detectionNodeList;
};

class DetectionMinQuality : public Detection{
  /// Detection occurs when quality equals or exceeds minQuality
  double minQuality;
public:
  /// Constructor
  DetectionMinQuality(void) : minQuality(0.0) {}
  DetectionMinQuality(double q) : minQuality(q) {}

  /// Destructor
  ~DetectionMinQuality(void) {}

  /// Set minimum quality to trigger detection
  void SetMinimumQuality(float mq){minQuality = mq;}

  /// Calculate the detectionNodeList
  int Calculate();
};

class DetectionNWay: public Detection{
  /// Detection occurs only after N sensors have detected contaminant 
  double minQuality;
  int N;
public:
  /// Constructor
  DetectionNWay(int ways=2) : minQuality(0.0), N(ways) {}

  /// Destructor
  ~DetectionNWay(void) {}

  /// Set minimum quality to trigger detection
  void SetMinimumQuality(float mq){minQuality = mq;}

  /// Set number of sensors required before detection occurs
  void SetNumSensorsToDetect(int n){N = n;}

  /// Calculate the detectionNodeList
  int Calculate();
};

#endif
