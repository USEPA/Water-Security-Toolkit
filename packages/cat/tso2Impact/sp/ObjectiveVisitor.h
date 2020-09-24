/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top SPOT directory.
 *  _________________________________________________________________________
 */

#ifndef __ObjectiveVisitor_h
#define __ObjectiveVisitor_h

#include <list>
#include <sp/Impact.h>

#define OBJECTIVE_VISITOR

class ObjectiveVisitor
{
public:

   std::string impactOutputFileName;

   virtual void outputScenarioImpactData(std::list<Impact>& myImpacts, int scenarioIndex) = 0;

   virtual void finalize() = 0;

};

#endif
