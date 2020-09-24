// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#include "instance_factory.h"

#include "instance.h"
#include "matrix_instance.h"
#include "euclidean_instance.h"
#include "cover_instance.h"
#include "graph_instance.h"
#include "geo_instance.h"
#include "hybrid_instance.h"
#include "impact_instance.h"

#include <stdio.h>
#include <stdlib.h>

void fatal(const char *func, const char *msg) 
{
  std::cout << func << ": " << msg << std::endl;
  exit(-1);
}

PMInstance *readInstance(const char *filename, int p)
{
  PMInstance *instance=0;

  enum {NONE, DIMACS, TSP, PMI, PMM, GEO, MSC, IMP, UFL, IMPACT} itype = NONE;

  if (strlen(filename)>=4) 
    {
      if      (strcmp (&filename[strlen(filename)-4], ".tsp") == 0) itype = TSP;
      else if (strcmp (&filename[strlen(filename)-4], ".pmm") == 0) itype = PMM;
      else if (strcmp (&filename[strlen(filename)-4], ".pmi") == 0) itype = PMI;
      else if (strcmp (&filename[strlen(filename)-4], ".msc") == 0) itype = MSC; //set cover
      else if (strcmp (&filename[strlen(filename)-4], ".imp") == 0) itype = IMP; //implicit pmedian
      else if (strcmp (&filename[strlen(filename)-4], ".geo") == 0) itype = GEO;
      else if (strcmp (&filename[strlen(filename)-4], ".ufl") == 0) itype = UFL;
      else if (strcmp (&filename[strlen(filename)-7], ".dimacs") == 0) itype = DIMACS;
      else if (strcmp (&filename[strlen(filename)-7], ".impact") == 0) itype = IMPACT;
    }
	
  FILE *input = fopen (filename, "r");
  if (input == 0) fatal("readInstance", "could not open input file");

  switch (itype) 
    {
    case UFL:

      instance = new PMMatrixInstance();
      ((PMMatrixInstance*)instance)->readUFL(input);
      break;

    case PMM:

      instance = new PMMatrixInstance();
      ((PMMatrixInstance*)instance)->readPMM(input,p);
      break;

    case TSP:

      instance = new PMEuclideanInstance();
      ((PMEuclideanInstance*)instance)->readTSP(input,p);
      break;

    case MSC:

      instance = new PMCoverInstance(true); //use the extra element
      ((PMCoverInstance*)instance)->readCover(input,p);
      break;

    case IMP:

      instance = new PMCoverInstance(false); //don't use the extra element
      ((PMCoverInstance*)instance)->readCover(input,p);
      break;

    case PMI:

      instance = new PMEuclideanInstance();
      ((PMEuclideanInstance*)instance)->readPMI(input,p);
      break;

    case GEO:

      instance = new PMGeoInstance();
      ((PMGeoInstance*)instance)->readGEO(input,p);
      break;

    case DIMACS:

      instance = new PMGraphInstance();
      ((PMGraphInstance*)instance)->readDimacs(input,p);
      break;

    case IMPACT:

      instance = new PMImpactInstance();
      ((PMImpactInstance*)instance)->readImpact(input,p);
      break;

    case NONE:

      fatal("readInstance", "format not supported");
    }
  fclose(input);
  return instance;
}
