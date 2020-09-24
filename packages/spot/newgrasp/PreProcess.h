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

// 
// File: PreProcess.h
// Author: Haixia Jia
//
// Created on June 12 2008.
// A helper class for supply the facility list which is closer to a 
// customer than certain distance. For optimazation only. 



#ifndef PREPROCESS_H
#define PREPROCESS_H

#include "ProblemInstance.h"
#include "solution.h"

namespace NewSensorPlacement {

class myPair {
  public:
    myPair(int num1, double dist1) {num = num1; dist = dist1;};
    myPair(){};
    int num;
    double dist;
};

#ifdef OPTIMER
  typedef myPair* myPair1;
#else
  typedef std::vector<myPair> myPair1;
#endif


class PreProcess {
  private:
    ProblemInstance *myInstance;
    int numN; // number of customers
    int numM; // number of potential facilities
    // temparory list for result;
    myPair1 newTempList;
    myInt1 tempList; 
    // the whole sorted facilities for each customer according distance.
    myInt2 sortedList;
    // table of distance from each customer to each facility;
    myDouble2 distList; 
    int temCustId;
    inline void rotate(int *a, int i, int j) {
      int temp = a[i];
      a[i] = a[j];
      a[j] = a[i+1];
      a[i+1]=temp;
    }
    // swap the i'th and j'th elements
    inline void swap(int *a, int i, int j) {
      int temp = a[i];
      a[i] = a[j];
      a[j] = temp;
    }
    // return distance from num facility to current customer;
    double getDist(int num) {
      return (distList[temCustId][num]);}
    int pivot(int *a, int first, int last);
    void quicksort(int *a, int first, int last);
    // sort one customer's facility list;
    void sortOneCust(int custId);
    // sort the whole table;
    void sortTable();
    void  merge(myInt1& numbers, myInt1& temp, int left, int mid, int right);
    void mergeSort(myInt1& numbers, myInt1& temp, int left, int right);
  public:
    PreProcess(ProblemInstance *inst, bool isSort);
    ~PreProcess();
    // Given a customer id, and a distance, return the list of all facilities
    // who is closer to this customer than the defined distance.
    myPair1 getNewPreFac(int cust, double dist); 
};
}; // namespace

#endif //PREPROCESS_H

