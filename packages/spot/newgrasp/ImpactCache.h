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

#ifndef impactcache_h
#define impactcache_h

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>


#include <map>
#include <vector>
#include <iostream>
#include <iomanip>

/******************************************************************************
 * The ImpactCacheData class stores one chunck (either a row or a column) of 
 * impact data laded from a file.  This class has both next and prev pointers
 * to allow for fast (constant time) manipulation of the MRU list
 *****************************************************************************/
namespace NewSensorPlacement {
class ImpactCacheData
{
  private:
    // the impact data from the file
	double *data;
	// the row or column index
	int idx;
  public:
    // pointer to the previous element in the MRU list
	ImpactCacheData *prev;
    // pointer to the next element in the MRU list
	ImpactCacheData *next;
  public:
	ImpactCacheData(int index,int numElems) {
		idx=index;
		data = new double[numElems];
		prev=NULL;
		next=NULL;
	}
	
	double *getData() {return data;}
	int getIndex() {return idx;}
	~ImpactCacheData() {
		delete []data;
	}
};
/******************************************************************************
 * The ImpactCache class is a representation of the impact data that stores
 * the impact data on disk and only loads in wht has been recently requested.
 * It creates 2 file sin the current directory:
 *     impevents.bin - holds the data in event-major order (each event's data
 *                     is stored in a cintiguous chunk in the file)  This
 *                     corresponds to a row in the 2-dimensional impact
 *                     data array
 *     impnodes.bin  - holds the data in node-major order (each node's data
 *                     is stored in a contiguous chunk in the file).  This
 *                     corresponds to a column in the 2-dimensional impact
 *                     data array
 * The data is stored this way because at different points in the program
 * accessing this data, it was observed that sometimes the data is accessed
 * event by event, and sometimes it was accessed basically by column.
 * In order to most efficiently handle the requests for data, a vector for
 * each type of data is created, one of length numNodes, and one of numEvents
 * length.  There is also a list maintained that keeps the most recently used
 * row or column at the front, so the least recently used element can be
 * discarded.  The number of elements in each of those lists is controlled by
 * the maxLoaded field.
 *
 * Currently the loading mechanism will create an array large enough to hold
 * all of the impact data so it can be written to the impnodes.bin file.
 * This will cause a limitation because there may not be enough memory to hold
 * that amount of data - even temporarily.  Other methods were used to create
 * the impnodes.bin file, but they were all very slow.  I figured that if it
 * couldn't at least hold this much data even temporarily, then it wouldn't
 * likely run thge rest of the code anyway.  If this becomes a problem, it can
 * be investigated in the future.
 *****************************************************************************/
class ImpactCache
{
  private:

    const static int CREATING=-1;
    const static int EVENT_MAJOR=0;
    const static int NODE_MAJOR=1;
    int mode;  // 0=row/event  1=col/node -1=creating
    // pointer to the file containing the event-major data
    FILE *fpEventMajor;
    // pointer to the file containing the node-major data
    FILE *fpNodeMajor;
    // the number of events in this impact file
    int numEvents;
    // the number of nodes in this impact file
    int numNodes;
    // the maximum number of elements in either list
    int maxLoaded;
    // the currently loaded number of elements
    int numLoaded;
    // The vector of event-mjaor data.  Only currently loaded indices will be non-null
    std::vector<ImpactCacheData * > eventArraysIndex;
    // The vector of node-mjaor data.  Only currently loaded indices will be non-null
    std::vector<ImpactCacheData * > nodeArraysIndex;
    // the MRU list for the event-major data
    ImpactCacheData* eventArraysLoaded;
    // the MRU list for the node-major data
    ImpactCacheData* nodeArraysLoaded;
    // Holds the node-major data during loading of the file.
 	double *colData;
    
    // position the file to the row,col
    void positionFile(FILE *fp,int row, int col, int rowLen);
    // retrieve the row or column data from the cache, loading it from file if necessary
	double *getImpactData(int i, std::vector<ImpactCacheData*> &index, ImpactCacheData** loaded, FILE *fp, int dataElemCount);
	// method to create a new event data row - only used during file creation
    void newEventRow(int eventNum);
    // writes the current impact data - only used during file creation
	void writeImpactData();
	// clears the cache for the specified data.  This is called when switching from event-major to node-major and also in the destructor to clean up when done
    void clearCacheData(std::vector<ImpactCacheData*> &index, ImpactCacheData** loaded);
  public:
    // construct a new ImpactCache
	ImpactCache(int nnodes, int nevents, int max=100);
	// tell the object that the impact data is beginning to be loaded from the impact file
    void startCreation();
    // a new event is about to be loaded - this signifies that the previous event is done being loaded.  Only used during creation
    void newEvent(int eventNum, double undetectedImpact);
    // Sets the impact value.  Only used during creation
    void setImpact(int c, int f, double impact);
    // tells the object that the impact file has now been completely loaded.  Only used during creation
    void created();
    // set the cache to handle event major processing
    void setEventMajor();
    // set the cache to handle node major processing
    void setNodeMajor();
    // get the impact value for the specified c and f values.
    double getImpact(int c, int f);
    // clean up
    ~ImpactCache();
};
};
#endif
