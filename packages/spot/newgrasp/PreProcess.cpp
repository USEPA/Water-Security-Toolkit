/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the LGPL License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README file in the top software directory.
 *  _________________________________________________________________________
 */

#include "PreProcess.h"
#include "redefs.h"

using namespace NewSensorPlacement;

PreProcess::PreProcess(ProblemInstance *inst, bool isSort) :
  myInstance(inst),
  numN(myInstance->getN()),
  numM(myInstance->getM()) 
{
  if (isSort) {  // if we want use the sorted table.  
    #ifdef OPTIMER
      tempList = new int[numM+1];
      sortedList = new int*[numN+1];
      distList = new double*[numN+1];
      for (int i=0; i<=numN; i++){
        sortedList[i] = new int[numM+1];
        distList[i] = new double[numM+1];
      }
      newTempList = new myPair[numM+1];
    #else
      tempList = std::vector<int>(numM+1, 0);
      sortedList = std::vector<std::vector<int> >(numN+1, std::vector<int>(numM+1, 0));
      distList = std::vector<std::vector<double> >(numN+1, std::vector<double>(numM+1, 0.0));
      newTempList = std::vector<myPair>(numM+1, myPair(0,0.0));
    #endif
    for (int i=1; i<=numN; i++)
      for (int j=1; j<=numM; j++) {
        sortedList[i][j] = j;
        distList[i][j] = myInstance->getDist(i,j);
      }
    sortTable();
    std::cout<<"sort is done------------------------------------------------\n";
  }
}


PreProcess::~PreProcess(){
  #ifdef OPTIMER
  for (int i=0; i<=numN; i++){
    delete [] sortedList[i];
    delete [] distList[i];
  }
  delete [] sortedList;
  delete [] distList;
  delete [] tempList;
  delete [] newTempList;
  #endif
}





int PreProcess::pivot(int *a, int first, int last) {
  int tem = random() % (last-first) + first;
  swap(a, first, tem);
  int p = a[first];  // value of pivot 
  double pDist = getDist(p);
  int ploc = first;  // current position of pivot
  int i = first+1;    // location we're comparing.
      // note: i > p always
  while (i <= last) {
    if (getDist(a[i]) < pDist) {
      rotate(a,ploc,i);
      ploc++;
    }
    i++;
  }
  return ploc;
}

void PreProcess::quicksort(int *a, int first, int last) {
  if (first >= last) return;
  int v=last -first;
  if (v==1){  // if only 2 element left,then just compare it without recusor.
    if (getDist(a[first]) > getDist(a[last]))
      swap(a,first,last);
    return;
  }
  int tem = random() % (last -first) + first;
  //int ploc = partitionList(a,a[tem], first,last);
  int ploc = pivot(a,first,last);
  quicksort(a,first,ploc-1);
  quicksort(a,ploc+1,last);
}


void PreProcess::sortOneCust(int custId) {
  temCustId = custId;
  mergeSort(sortedList[custId], tempList, 1, numM);
  //quicksort((sortedList[custId]), 1, numM);
}


void PreProcess::mergeSort(myInt1& numbers, myInt1& temp, int left, int right){
  int mid;
  if (right > left){
    mid = (right + left) / 2;
    mergeSort(numbers, temp, left, mid);
    mergeSort(numbers, temp, mid+1, right);
    merge(numbers, temp, left, mid+1, right);
  }
}

void PreProcess::merge(myInt1& numbers, myInt1& temp, int left, int mid, int right){
  int i, left_end, num_elements, tmp_pos;
  left_end = mid - 1;
  tmp_pos = left;
  num_elements = right - left + 1;
  while ((left <= left_end) && (mid <= right)){
    if (getDist(numbers[left]) <= getDist(numbers[mid]))  {
      temp[tmp_pos] = numbers[left];
      tmp_pos = tmp_pos + 1;
      left = left +1;
    }
    else{
      temp[tmp_pos] = numbers[mid];
      tmp_pos = tmp_pos + 1;
      mid = mid + 1;
    }
  }
  while (left <= left_end) {
    temp[tmp_pos] = numbers[left];
    left = left + 1;
    tmp_pos = tmp_pos + 1;
  }
  while (mid <= right){
    temp[tmp_pos] = numbers[mid];
    mid = mid + 1;
    tmp_pos = tmp_pos + 1;
  }
  for (i=0; i <= num_elements; i++){
    numbers[right] = temp[right];
    right = right - 1;
  }
}


void PreProcess::sortTable() {
  for (int i=1; i<=numN; i++)
    sortOneCust(i);
}



myPair1 PreProcess::getNewPreFac(int cust, double dist){
  temCustId = cust;
  int temFac;
  double temDist;
  int pos = 1;
  bool isContinue = true;
  while ((pos <= numM) && (isContinue)) {
    temFac = sortedList[cust][pos];
    temDist = getDist(temFac);
    if (temDist < dist) {
      newTempList[pos].num = temFac;
      newTempList[pos].dist = temDist;
      pos++;
    }
    else 
      isContinue = false;
  }
  newTempList[0].num = pos-1;    
  return (newTempList);
}

