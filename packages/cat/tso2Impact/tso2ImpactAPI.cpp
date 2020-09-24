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

#ifdef HAVE_CONFIG_H
#include <teva_config.h>
#endif
#include "sp/ObjectiveBase.h"

void finalizeTheObjectives(std::vector<ObjectiveBase*>& theObjectives)
{
   for (size_t i = 0; i < theObjectives.size(); i++)
   {
      theObjectives[i]->finalize();
      delete theObjectives[i];
   }

   theObjectives.clear();
}
