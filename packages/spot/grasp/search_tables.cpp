// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/*********************************************
 *
 * SearchTables: implementation
 *
 *********************************************/

#include "search_tables.h"
#include "basics.h"

#include <iostream>
#include <iomanip>

/*-------------
  | constructor
  *------------*/

SearchTables::SearchTables (int _m, int size, bool _using_list) 
{
  using_list = _using_list;
  m = _m;

  tolerance = EPSILON / (double)m;
  save = new double [m+1];
  lose = new double [m+1];
  extra = 0;
  allocator = 0;
  list = 0;

  if (using_list) {
    list = new ExtraNode *[m+1];

    //WARNING: NOT USING "size"
    //we are allocating size "real" nodes plus 2 dummy nodes per position
    allocator = new BossaAllocator <ExtraNode> (2*m + m*m); 
		
    //initialize each of the m lists
    for (int i=1; i<=m; i++) {
      ExtraNode *first = allocator->allocate();
      ExtraNode *last  = allocator->allocate();
      list[i] = first;    //set first
      first->node = 0;    //label smaller than all valid labels
      first->next = last; //points to the other dummy node
      last->node  = m+1;  //label greater than all valid labels
      last->next  = 0; //last node, really
    }
  } else {
    extra = new double *[m+1];
  }
}

	
/*--------------------------------------------
  | delete an entire row, keeping dummy nodes;
  | save objects for future use...
  *-------------------------------------------*/

void SearchTables::deleteRow(int i) {
  ExtraNode *x = list[i]->next; //list[i] is just a dummy node
  while (x->node <= m) {
    deallocate(x);
    x = x->next; //(the node is still there)
  }
  list[i]->next = x; //make first point to last!!!
}
	
void SearchTables::init (PMSolution *s) {
  for (int i=1; i<=m; i++) save[i] = lose[i] = -s->getInstance()->getFacCost(i); 

  if (!usingList()) {
    for (int i=1; i<=m; i++) { //allocate the rows we need in extra
      //WARNING: I CHANGED THIS
      //if (s->contains(i)) {
      extra[i] = new double[m+1];
      for (int j=1; j<=m; j++) extra[i][j] = 0;
      //} else extra[i] = 0;
    }
  }
}


/*------------
  | destructor
  *-----------*/

SearchTables::~SearchTables () {
  delete [] save;
  delete [] lose;
  if (using_list) {
    for (int i=1; i<=m; i++) {
      ExtraNode *x = list[i];
      while (x != 0) {
	deallocate(x);
	x = x->next; //'deallocate' does not change x
      }
    }
    delete [] list;
    delete allocator;
  } else {
    for (int i=1; i<=m; i++) {
      if (extra[i] != 0) delete [] extra[i];
    }
    if (extra!=0) delete [] extra;
  }	
}


/***************************************************************
 *
 * add node corresponding to vertex i
 * ('current' MUST point to a node that precedes i in the list)
 * returns a pointer to a node that is guaranteed to precede 
 * the node used in the next call (if any)
 *
 ***************************************************************/

ExtraNode *SearchTables::add (ExtraNode *current, int i, double v) {
  if (i<1 || i>m) fatal ("add", "node out of range");
	
  //find the right node
  while (current->next->node < i) current = current->next;
  ExtraNode *cn = current->next; //we'll do something with this node --- or before it

  //first case -> node is not in the list
  if (cn->node > i) { //node i is not present -> insert a new one
    ExtraNode *x = allocate(); //allocate new node
    x->set(i,v,cn);            //node will represent i, with value v, and will point to cn
    current->next = x;         //current will now point to x
    return x;                  //we've advanced to the new node
  }

  //it's in the list; get the new value
  v += cn->value; 
		
  //second case: value was there, but sum is zero
  if ((v<tolerance) && (v>-tolerance)) { //new zero -> remove it
    current->next = cn->next; //get the new successor of current
    deallocate(cn);           //get rid of the node
    return current;           //return a a pointer to current
  }

  //third case: value was there, sum still not zero
  cn->value = v; //just update
  return cn;     //and advance a little
}


/*********************************************
 *
 * standard error message
 *
 *********************************************/

void SearchTables::fatal (const char *func, const char *msg) 
{
  std::cerr << "SearchTables::" << func << ": " << msg << std::endl;
  exit (1);	
}
