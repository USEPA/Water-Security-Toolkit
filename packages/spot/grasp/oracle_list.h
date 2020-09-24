// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/**************************************************************
 *
 * OracleList: auxiliary data structure used by the distance
 * oracle. Used primarily to resort facilities (originally 
 * sorted by distance, resorted by labels in linear time).
 * 
 **************************************************************/

#ifndef ORACLE_LIST_H
#define ORACLE_LIST_H

#include <stdlib.h>
#include <stdio.h>

#include "basics.h"

class OracleList {
 private:
  typedef struct {int next, prev;} Node;
  Node *list; //the elements
  int n;      //maximum number of elements in the list

 public:
  //empty list
  void makeEmpty() {list[0].next = list[0].prev = 0;}
		
  //create a list of all values, from 1 to n
  inline void makeFull() {
    for (int i=1; i<=n; i++) {
      list[i].next = i+1;
      list[i].prev = i-1;
    }
    list[1].prev = list[n].next = 0;
    list[0].next = 1; list[0].prev = n;
  }

  //constructor
  OracleList (int _n) {
    n = _n;
    list = new Node [n+1];
  }

  //remove b from list, return predecessor
  inline int vanquish (int b) {
    int c, a;
    list[a=list[b].prev].next = (c=list[b].next);
    return (list[c].prev = a);
  }

  //inserts b after a (DOES NOT UPDATE PREV POINTERS)
  inline void lazyInsert (int b, int a) {
    list[b].next = list[a].next;
    list[a].next = b;
  }

  //copy the contents of the current list to 'array'
  //saves the size of the list in array[0]
  inline void dump (int *array) const {
    int p=0;
    for (int i=list[0].next; i!=0; i=list[i].next) {array[++p] = i;}
    array[0] = p; //save size
  }

  inline void dump (IntDouble *array) const {
    int p=0;
    for (int i=list[0].next; i!=0; i=list[i].next) {array[++p].id = i;}
    array[0].id = p;
  }

  //for debug purposes
  void output (FILE *file) {
    fprintf (file, "[");
    for (int i=list[0].next; i!=0; i=list[i].next) {fprintf (file, " %d", i);}
    fprintf (file, " ]\n");
  }
		
  ~OracleList () {delete [] list;}
};

#endif


/*
//unused functions
	

inline void insertBack (int b) {
int a = list[b].prev = list[0].prev;
list[0].prev = list[a].next = b;
list[b].next = 0; 
}

inline void insert (int b, int a) {
int c = list[b].next = list[a].next;
list[a].next = b;
list[c].prev = b;
list[b].prev = a;
}*/

