/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the LGPL License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top software directory.
 *  _________________________________________________________________________
 */

#ifndef REDEFS_H
#define REDEFS_H

#include <teva_config.h>
#include <math.h>

#ifndef TEVA_SPOT_HAVE_RINT
#define rint(x) (floor((x)+.05))
#endif

#ifndef TEVA_SPOT_HAVE_SRANDOM
#define srandom(x) (srand(x))
#define random() (rand())
#endif

#ifndef TEVA_SPOT_HAVE_LOG2
#define log2(x) (log(x) / log((double)2))
#endif

#endif
