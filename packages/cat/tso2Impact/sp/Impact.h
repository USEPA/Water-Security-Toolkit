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

#ifndef tevasp_Impact_h
#define tevasp_Impact_h

///
class Impact
{
public:

   /// The node id
   int nodeID;

   /// The value of the impact at that node
   double impact;

   /// The time of detection (in minutes)
   double timeOfDetection;
};

#ifdef IMPOSE_IMPACT_NODE_ORDER

// imposes a global node order - sort by
// impact first, and node ID second.
namespace std
{
///
template <> struct less<Impact>
{
   ///
   bool operator()(const Impact &a, const Impact &b)const
   {
      if (a.impact < b.impact)
      {
         return true;
      }
      else if (a.impact > b.impact)
      {
         return false;
      }
      else if (a.nodeID == UNDETECTED_NODE_INDEX)
      {
         return false;
      }
      else if (b.nodeID == UNDETECTED_NODE_INDEX)
      {
         return true;
      }
      else
      {
         return a.nodeID < b.nodeID;
      }

      // UNREACHABLE
   }
};
}

#endif

#endif
