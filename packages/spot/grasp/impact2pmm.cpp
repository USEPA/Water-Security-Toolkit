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

#include <map>

#include <iostream>
#include <fstream>
#include <iomanip>

int main(int argc, char **argv)
{
  if(argc!=3)
    {
      std::cerr << "***Usage: impact2pmm impact-input-file pmm-output-file" << std::endl;
      return 0;
    }

  std::ifstream impactFile(argv[1]);
  if(!impactFile)
    {
      std::cerr << "***Failed to open input impact file=" << argv[1] << std::endl;
      return 0;
    }

  std::map<int,std::map<int,double> > detectedImpacts; 
  std::map<int,double> undetectedImpacts;     

  int numCustomers=0;
  int numFacilities=0;

  int scenarioIndex;
  int sourceNodeIndex; // not used
  int injectionStartTime; // not used
  int nodeIndex;
  double thisImpact;  

  impactFile >> scenarioIndex >> nodeIndex >> thisImpact;
  
  while(!(!impactFile))
    {
      numCustomers=std::max(numCustomers,scenarioIndex);
      numFacilities=std::max(numFacilities,nodeIndex);

      if(nodeIndex==-1)
	{
	  if(undetectedImpacts.find(scenarioIndex)!=undetectedImpacts.end())
	    {
	      std::cerr << "***Undetected impact for scenario index=" << scenarioIndex << " already defined" << std::endl;
	      return 0;
	    }
	  
	  undetectedImpacts[scenarioIndex]=thisImpact;
	}
      else
	{
	  if(detectedImpacts.find(scenarioIndex)==detectedImpacts.end())
	    {
	      detectedImpacts[scenarioIndex]=std::map<int,double>();
	    }
	      
	  std::map<int,double> &thisSubmap(detectedImpacts[scenarioIndex]);
	      
	  if(thisSubmap.find(nodeIndex)!=thisSubmap.end())
	    {
	      std::cerr << "***Detected impact for scenario index=" << scenarioIndex << ", node index=" << nodeIndex << " already defined" << std::endl;
	      return 0;
	    }

	  thisSubmap[nodeIndex]=thisImpact;
	}

      impactFile >> scenarioIndex >> sourceNodeIndex >> injectionStartTime >> nodeIndex >> thisImpact;
    }

  impactFile.close();

  std::cout << "Number of customers=" << numCustomers << std::endl;
  std::cout << "Number of facilities=" << numFacilities << std::endl;

  std::ofstream pmmFile(argv[2]);
  if(!pmmFile)
    {
      std::cerr << "***Failed to open output pmm file=" << argv[2] << std::endl;
      return 0;
    }

  pmmFile << "p" << " " << numCustomers << " " << numFacilities << std::endl;
  for(int i=1;i<=numCustomers;i++)
    {
      for(int j=1;j<=numFacilities;j++)
	{
	  double distance=0.0;

	  std::map<int,std::map<int,double> >::iterator detectedIter=detectedImpacts.find(i);
	  if(detectedIter==detectedImpacts.end())
	    {
	      // the scenario is completely undetectable - assign to worse-case impact
	      std::map<int,double>::iterator undetectedIter(undetectedImpacts.find(i));
	      if(undetectedIter==undetectedImpacts.end())
		{
		  std::cerr << "***No undetected entry for scenario" << std::endl;
		  return 0;
		}
	      distance=(*undetectedIter).second;
	    }
	  else
	    {
	      // the scenario is possibly detectable
	      std::map<int,double> &subMap=(*detectedIter).second;
	      std::map<int,double>::iterator impactIter=subMap.find(j);
	      if(impactIter==subMap.end())
		{
		  // the scenario is undetectable - assign to worse-case impact
		  std::map<int,double>::iterator undetectedIter(undetectedImpacts.find(i));
		  if(undetectedIter==undetectedImpacts.end())
		    {
		      std::cerr << "***No undetected entry for scenario" << std::endl;
		      return 0;
		    }
		  distance=(*undetectedIter).second;
		}
	      else
		{
		  distance=(*impactIter).second;
		}	  
	    }

	  pmmFile << "a" << " " << i << " " << j << " " << distance << std::endl;
	}
    }

  pmmFile.close();

  return 1;
}
