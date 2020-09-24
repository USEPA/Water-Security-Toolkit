// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/********************
 *
 * Sorting algorithm
 *
 ********************/

#ifndef RFW_SORT_H
#define RFW_SORT_H

#define CUTOFF 10

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bossa_random.h"

#include <iostream>
#include <iomanip>

/*------------------------------------
  | Partition the array [left...right]
  | return a pointer to the first 
  | element of the right segment
  *-----------------------------------*/

template <class T> T* partition (T pvt, T *left, T *right) {
  T *i = left-1, *j = right+1;
	
  for (;;) {
    do {i++;} while (*i < pvt);    //move left pointer to the right
    do {j--;} while (pvt < *j);    //move right pointer to the left
    if (i >= j) break;             //if they have crossed, stop
    T tmp = *i; *i = *j; *j = tmp; //swap elements
  }

  return i;
}

/*------------------------------------
  | make sure the first k elements are 
  | sorted
  *-----------------------------------*/

template <class T> void partialSort (T *left, T *right, T *target) {
  T *p, *pvt;
  T *first = left;

  //first, partition until the right position is found
  if (target < right) {
    if (target < left) 
      {
	std::cerr << "Weird..." << std::endl;
      }
    for (;;) {
      pvt = &left[BossaRandom::getInteger (0,(int)(right-left))]; //get a random number between left and right
      p = partition (*pvt, left, right); //partition on a random pivot

      //p now points to the first element in the right segment
      if (p == target+1) break; //first p elements not greater than the rest --- what we want
      else {
	if (p>target) {right = p-1;} //target is somewhere in the left part
	else {left=p;} //first p-1 are ok, need to check from p on
      }
      if (left == right) break; //do I really need this?
    }
  }

  sort (first, target);
}


template <class T> bool isSorted (T *left, T *right) {
  for (; left<right; left++) {
    if (*(left+1) < *left) return false;
  }
  return true;
}


/*------------------------
  | sort array from l to r
  *-----------------------*/
template <class T> void sort (T *l, T *r) {
  //declarations
  const int cutoff=10;
  register T *i, *j;
  T pvt, tmp, *left=l, *right=r, *mid, **top, *stack[64];
  top = stack;

  for (;;) {
    if (right-left > cutoff) {

      // get median-of-three (in the process, sorts the three elements involved)
      pvt = *(mid = (left + (right - left) / 2)); //first, try middle element           
      if (pvt < *(i=left)) {  //i is initialized here
	pvt = *left; 
	*left = *mid; //transfer left to the middle
      };

      if (*right < pvt) { 
	tmp = *right; 
	*right = pvt;                                
	if (tmp < *left) {
	  pvt = *left; 
	  *left = tmp;
	} else pvt = tmp;
      }

      //define middle element
      *mid = *(j = right - 1); //j is initialized here

      //partition
      for (;;) {
	do {i++;} while (*i < pvt);  //move left pointer to the right
	do {j--;} while (pvt < *j);  //move right pointer to the left
	if (i >= j) break;           //if they have crossed, stop
	tmp = *i; *i = *j; *j = tmp; //swap elements
      }

      //we know that i>=pvt and j<=pvt
      if (i < right) {  
	*(right-1) = *i; //put i in the penultimate position 
	*i = pvt;        //put the pivot in i
      } else {             //i points to the last position...
	*(--i) = pvt;    //put pivot in penultimate position
      }

      //push subproblems
      if (i - left > right - i) { //left part is larger
	*top++ = left; //push left boundary of left part
	*top++ = i-1;  //push right boundary of left part
	left = i+1;    //set new left boundary of the right part
      } else { //right part is larger
	*top++ = i+1;   //push left boundary of right part
	*top++ = right; //push right boundary of right part
	right = i-1;    //set new right boundary of left part
      }
    } else { //not big enough: get a new subproblem to solve
      if (--top <	stack) break; //maybe there is none
      right = *top;     //pop the right pointer
      left  = *(--top); //pot the left pointer
    }		
  }

  //--------------------------------------------------------
  // done with the quicksort part, now we do insertion sort
  //--------------------------------------------------------

  //first, find a sentinel
  {
    T* minelem = l;
    //minelem = l;
    right = l + cutoff;
    if (right>r) right=r;                                    
    for (i=l+1; i<=right; i++) if (*i<*minelem) minelem = i; //find minimum element
    tmp = *l;       //make the first element the minimum
    *l = *minelem;                                           
    *minelem = tmp;
  }

  //now apply insertion sort to (l+1,r)
  {
    register T *j;
    T value;
    T *i;
    for (i = l+2; i <= r; i++) {
      value = *(j=i);
      while (value < *(--j)) {*(j+1)=*j;}
      *(j+1) = value;
    }
  }
}

#endif

