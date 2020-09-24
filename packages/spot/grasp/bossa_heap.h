// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

///***************************************
//
// BossaVertexHeap
//
//***************************************
#ifndef BOSSA_VERTEX_HEAP_H
#define BOSSA_VERTEX_HEAP_H

#include <stdlib.h>
//#include "bossa_types.h"
#include "bossa_array.h"

template <class T> class BossaVertexHeapUnit {
 public:
  int vertex;   //label of the vertex
  T distance;  //distance from the other vertices
};

template <class T> class BossaVertexHeap {
 private:
  inline void up (int);   //standard heap operation
  inline void down (int); //standard heap operation
  inline void swap (int, int);  //swap two positions

  int nextpos;	//position of next insertion
  BossaVertexHeapUnit <T> *h;
  BossaArray <int> *heappos;
  //position of each element in the heap
  //convention: 
  //   0 -> not yet inserted
  //   lastpos+1 -> already removed

  int lastpos;

 public:

  inline T getValue (int pos) {return h[pos].distance;}
  inline int getVertex (int pos) {return (h[pos].vertex);}; //vertex in position pos
  inline bool alreadyProcessed (int v) {return (heappos->getValue(v) == lastpos + 1);}

  //new stuff
  inline T getElementValue (int v) {return (getValue(heappos->getValue(v)));}
  inline bool contains (int v) {return ((heappos->getValue(v)!=0)&&(heappos->getValue(v)!=lastpos+1));}

  inline bool isEmpty () {return (nextpos == 1);}
  inline int getSize() {return (nextpos-1);}
  inline int getMaxSize() {return (lastpos);}

  void removeElement (int);
  void removeFirst (int&, T&);  //O(log n)
  bool insert (int vertex, T distance, bool force = true); //O(log n)
  void update (int vertex, T distance);
  void heapify ();
  bool check();
  bool pushBack (int vertex, T distance);

  void reset () {nextpos = 1; heappos->reset();} //O(1)

  BossaVertexHeap (int n); //constructor O(n)
  ~BossaVertexHeap (); //O(1)
  void outputHeap (int); //O(n)
};

//-------------
// Constructor
//-------------
template <class T> BossaVertexHeap<T>::BossaVertexHeap (int n) {
  nextpos = 1;
  lastpos = n;
  h = new BossaVertexHeapUnit <T> [n+1];
  heappos = new BossaArray <int> (n+1, 0);
}

//------------
// Destructor
//------------
template <class T> BossaVertexHeap<T>::~BossaVertexHeap() {
  delete [] h;
  delete heappos;
}

//--------------------------
// Output heap in pre-order
//--------------------------
template <class T> void BossaVertexHeap<T>::outputHeap (int pos) {
  if (pos < nextpos) {
    printf("(%d %d)",getValue(pos),getVertex(pos));
    outputHeap(2*pos);
    outputHeap(2*pos+1);
  }
  else printf("-");
}

//------------------------------------
// Swap elements in positions a and b
//------------------------------------
template <class T> void BossaVertexHeap<T>::swap (int a, int b) {
  BossaVertexHeapUnit <T> tmp = h[a];
  h[a] = h[b];
  h[b] = tmp;
  heappos -> setValue (h[a].vertex, a);
  heappos -> setValue (h[b].vertex, b);
}

//------------------------------------
// Standard up operation performed on 
// the element in the n-th position
//------------------------------------
template <class T> void BossaVertexHeap<T>::up (int n) {
  if ((n>1) && (getValue(n/2) > getValue(n))) { 
    swap (n,n/2);                           
    up (n/2);										
  }
}

//------------------------------------------
// Move the element currently in position n
// down to an appropriate position.
//------------------------------------------
template <class T> void BossaVertexHeap<T>::down (int n) {
  BossaVertexHeapUnit <T> tmp;
  T minchild;
  int two_n;

  if ((two_n = 2*n) < nextpos) {
    tmp = h[n];
    do {
      minchild = getValue(two_n);
      if (((two_n+1)<nextpos) && (minchild>getValue(two_n+1))) minchild = getValue(two_n+1);
      if (tmp.distance > minchild) { 
	if (minchild == getValue(two_n)) {		
	  h[n] = h[two_n];
	  heappos->setValue (h[n].vertex, n);
	  n = two_n;               
	} else {
	  h[n] = h[two_n+1];
	  heappos->setValue (h[n].vertex,n);
	  n = two_n + 1;		
	}
	two_n = 2*n;
      } else break;
    } while (two_n < nextpos);
    h[n] = tmp;
    heappos->setValue (h[n].vertex, n);
  }
}

//---------------------------------------------------
// Remove the element with the highest priority from 
// the heap. Its label and  value are returned.
//---------------------------------------------------
template <class T> void BossaVertexHeap<T>::removeFirst (int& vertex, T& value) {
  vertex = h[1].vertex;  
  value = getValue(1);

  h[1] = h[nextpos-1];	

  heappos -> setValue (h[1].vertex, 1);
  heappos -> setValue (vertex, lastpos + 1);	

  nextpos --;					
  down(1);					
}

//------------------------------
// heapify operation
//------------------------------
template <class T> void BossaVertexHeap<T>::heapify () {
  int i;
  for (i=(nextpos-1)/2; i >= 1; i--) down(i);
}

template <class T> bool BossaVertexHeap<T>::check() {
  int i;
  for (i = nextpos - 1; i > 1; i--) {
    if (h[i].distance < h[i/2].distance) {
      fprintf (stderr, "%d (%d) is child of %d (%d). There are %d elements.\n", 
	       h[i].distance, i, h[i/2].distance, i/2, nextpos - 1);
      return false;
    }
  }

  for (int i = 1; i < nextpos; i++) {
    if (heappos->getValue (h[i].vertex) != i) {
      fprintf (stderr, "Element %d thinks it is in position %d, but it isn't.\n", h[i].vertex, i);
      return false;
    }
  }

  for (int i = 1; i <= lastpos; i++) {
    int pos = heappos->getValue (i);
    if ((pos > 0) && (pos <= lastpos)) {
      if (pos >= nextpos) {
	fprintf (stderr, "Element %d's reported position (%d) is out of bounds (there are only %d elements).\n", i, pos, nextpos - 1);
	return false;
      }
      if (h[pos].vertex != i) {
	fprintf (stderr, "Element %d is not in the position he thinks he is (%d); %d is there.\n", i, pos, h[pos].vertex);
	return false;
      }
    }
  }
		

  return true;
}

template <class T> bool BossaVertexHeap<T>::pushBack (int v, T distance) {
  if (contains(v)) return false;
  h[nextpos].vertex = v;
  h[nextpos].distance = distance;
  heappos->setValue (v, nextpos);
  nextpos++;
  return true;
}



template <class T> void BossaVertexHeap<T>::removeElement (int vertex) {
  int pos = heappos->getValue (vertex);

  //fprintf (stderr, "Removing element %d, value=%d, position=%d, nextpos=%d, lastpos=%d\n", vertex, h[pos].distance, pos, nextpos, lastpos);	
	
  if ((pos!=0) && (pos!=lastpos+1)) { //element actually belongs to the heap

    heappos->setValue (vertex, lastpos+1); //element will no longer belong to the heap
    nextpos --;  //one fewer element

    if ((nextpos != 1) && (nextpos != pos)) { //not empty, not last
      //update heap

      h[pos] = h[nextpos]; //last element replaces the removed one
      heappos->setValue (h[pos].vertex, pos); //let the element know his new position
		
      if ((pos>1) && (h[pos].distance<=h[pos/2].distance)) up(pos);
      else down(pos);
    }
  }
}







//--------------------------------------------
// Insert an element into the heap
// (if the element is already in the heap,
// its priority is updated as necessary)
//--------------------------------------------

template <class T> bool BossaVertexHeap<T>::insert (int vertex, T distance, bool force) {
  int prevpos = heappos->getValue (vertex);
  if ((prevpos != 0) && (prevpos != lastpos + 1)) { //already in heap
    if (h[prevpos].distance < distance) return false;
    else {
      h[prevpos].distance = distance;
      up (prevpos);
      return true;
    }
  }
  if ((prevpos == lastpos + 1) && !force) return false;

  h[nextpos].vertex = vertex;
  h[nextpos].distance = distance;
  heappos -> setValue (vertex, nextpos);
  up(nextpos);
  nextpos++;
  return true;
}

template <class T> void BossaVertexHeap<T>::update (int vertex, T distance) {
  if (!contains(vertex)) insert (vertex, distance, true);
  else {
    T current_distance = getElementValue (vertex);
    if (distance == current_distance) return; //nothing to do
    if (distance < current_distance) insert (vertex, distance, true); //increase priority
    if (distance > current_distance) { //decrease priority
      int current_pos = heappos->getValue (vertex);
      h[current_pos].distance = distance;
      down(current_pos);
    }
  }
}

#endif

