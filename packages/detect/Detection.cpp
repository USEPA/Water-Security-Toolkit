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

#include <Detection.h>
#include <list>
#include <map>

int DetectionMinQuality::Calculate(){

    // Check that Set* have been called
    this->detectionNodeList.clear();
    this->detectionNodeList.resize(this->nSteps, std::vector<int>());

    std::vector<bool> nodeHits(this->net->numNodes, false);

    // each source node detects 
    PSource sources=this->source->source;
	// holds the earlies start time for each node index that injects the primary species
	std::map<int, long> minDetSteps;
    std::map <int, long> :: const_iterator mds;
    for (int i=0; i<this->source->nsources; i++){
      // if this is the species we are detecting
      if(sources[i].speciesIndex==speciesIndex) {
        int nodeIndex = sources[i].sourceNodeIndex - 1;
        long detStep = long((sources[i].sourceStart / 60.0) / this->reportStep);
        if (detStep >= this->nSteps){
          detStep = this->nSteps - 1;
        }
		mds=minDetSteps.find(nodeIndex);
		// if the nodeIndex doesn't exist or this source start is earlier
		// than one we alread have, the set it to the current detStep
		if(mds == minDetSteps.end() || detStep < mds->second) {
			minDetSteps[nodeIndex]=detStep;
		}
	  }
	}
	// for each node that injected the primary species...
	for(mds=minDetSteps.begin();mds != minDetSteps.end();mds++) {
	  int nodeIndex=mds->first;
	  long detStep=mds->second;
	  // add the node index to the detectionNodeList for its detection time.
      this->detectionNodeList[detStep].push_back(nodeIndex);
      nodeHits[nodeIndex] = true;
    }
	float**c = net->qualResults->nodeC[this->speciesIndex];
    for (int nodeIndex = 0; nodeIndex < this->net->numNodes; nodeIndex++){
      if (nodeHits[nodeIndex] == false){
        for (int ts=0; ts < this->nSteps; ts++){
          double q = c[nodeIndex][ts];
          if ( (q > 0.0) && (q > this->minQuality)){
            this->detectionNodeList[ts].push_back(nodeIndex);
            break;
          }
        }
      }
    }
    return 0;  // success
} 

int DetectionNWay::Calculate(){
  
    this->detectionNodeList.clear();
    this->detectionNodeList.resize(this->nSteps, std::vector<int>());

    std::vector<bool> nodeHits(this->net->numNodes, false);
    std::list<int> nodeIDs;
    int numHits = 1;  // the source nodes

    // Find the source nodes
    for (int i=0; i<source->nsources; i++){
      int nodeIndex = this->source->source[i].sourceNodeIndex - 1;
      nodeHits[nodeIndex] = true;
      nodeIDs.push_back(nodeIndex);
    }
    //
    // Don't begin recording detection until at least N nodes have detected contaminant
    //
	float**c = net->qualResults->nodeC[this->speciesIndex];
    for (int ts=0; ts < this->nSteps; ts++){
      for (int nodeIndex = 0; nodeIndex < this->net->numNodes; nodeIndex++){
        double q = c[nodeIndex][ts];
        if ( (q > 0.0) && (q > this->minQuality)){
          if (nodeHits[nodeIndex] == false){   // first time this node has contaminant
            if (++numHits > this->N){
              this->detectionNodeList[ts].push_back(nodeIndex);
              if (nodeIDs.size() > 0){
                // Allow the previous nodes to detect now as well
                std::list<int>::iterator iter;
                for (iter=nodeIDs.begin(); iter != nodeIDs.end(); iter++){
                  this->detectionNodeList[ts].push_back(*iter);
                }
                nodeIDs.clear();
              }
            }
            else{
              nodeIDs.push_back(nodeIndex);
            }
            nodeHits[nodeIndex] = true;
          }
        }
      }
    }

    return 0;  // success
}
