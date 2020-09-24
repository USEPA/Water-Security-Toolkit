// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/*********************************************
 *
 * SearchTables: keeps loss, gain, and extra, 
 * used by the local search procedure.
 *
 * Can operate in two "modes": matrix or list
 * (sparse matrix).
 *
 *********************************************/

#ifndef SEARCH_TABLES_H
#define SEARCH_TABLES_H

#include "bossa_allocator.h"
#include "solution.h"

#include <assert.h>

/***********************************
 *
 * nodes used in the sparse matrix
 *
 ***********************************/

class ExtraNode {
 public:
  int node;
  double value;
  ExtraNode *next;

  inline void set (int _node, double _value, ExtraNode *_next) {
    node = _node;
    value = _value;
    next = _next;
  }

};

/**********************************
 *
 * The SearchTables class itself
 *
 **********************************/

class SearchTables {
 private:
  //node used in the sparse matrix representations
  int m;
  double tolerance;
  BossaAllocator <ExtraNode> *allocator;
  bool using_list;
  void fatal (const char *func, const char *msg);
  inline ExtraNode *allocate() {return allocator->allocate();}
  void deallocate(ExtraNode *x) {allocator->deallocate(x);}
		
  int cur_center; //vicinity we're looking at
  double *cur_vector; //vector for that vicinity (used in matrix mode)
  ExtraNode *cur_node; //used in list node
  ExtraNode *add (ExtraNode *current, int i, double v); //v to node i (which must be in list headed by current)

 public:
  double *save;     //a linear array
  double *lose;     //a linear array

  //only one of the following is used...
  double **extra;   //a matrix
  ExtraNode **list; //a list

  inline bool usingList() {return using_list;}
  SearchTables (int _m, int _size, bool _using_list); //con

  void setCurrent (int i) {
    cur_center = i;
    if (using_list) {cur_node = list[cur_center];}
    else {cur_vector = extra[cur_center];}
  }
		
  void deleteRow(int i);
		
  ~SearchTables ();
		
  inline void addExtra (int i, double v) {
    if (using_list) {cur_node = add (cur_node, i, v);} 
    else {cur_vector[i] += v;}
  }

  void reportMove (int ins, int rem) {
    if (usingList()) {
      if (rem>0) deleteRow(rem);
    } else {
      for (int i=1; i<=m; i++) {
	if (extra[rem][i]>EPSILON) {
	  fprintf (stderr, "(%lg)", extra[rem][i]);
	  assert (false);
	}
	extra[rem][i]=0;
      }

      //WARNING: THIS CHANGED
      //extra[ins] = extra[rem];
      //extra[rem] = 0;
    }
  }

  int countNonZeros () {
    int nz = 0;
    if (using_list) {
      for (int f=1; f<=m; f++) {
	for (ExtraNode *n=list[f]->next; n->node<=m; n=n->next) {
	  nz++;
	}
      }
    } else {
      for (int f=1; f<=m; f++) {
	if (extra[f]!=0) {
	  for (int i=1; i<=m; i++) {
	    if (extra[f][i]>EPSILON) nz++;
	  }
	}
      }

    }
    return nz;
  }

  void init (PMSolution *s);
};


#endif
