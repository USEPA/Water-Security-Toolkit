// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/***********************************************************************
 *
 * ALLOCATOR
 *
 * - This is a list of preallocated objects of type T. They are 
 *   contiguously allocated, avoiding memory fragmentation. 
 *
 * - T can be any type with a default constructor (i.e., a constructor 
 *   that does not require any parameters)
 * 
 * - use 'allocate' to get an element, and 'deallocate' to return 
 *   it to the allocator
 *
 ***********************************************************************/

#ifndef bossa_allocator_h
#define bossa_allocator_h

#include "bossa_stack.h"
#include <stdio.h>

template <class T> class BossaAllocator {
 private:
  int maxsize;
  BossaStack <T*> *available;
  T *object_list;

  void fatal (const char *func, const char *msg) {
    fprintf (stderr, "BossaAllocator::%s: %s.\n", func, msg); exit (-1);
  }

  void initStack () {
    for (int i=0; i<maxsize; i++) available->push(&object_list[i]);
  }		

 public:
  BossaAllocator (int size) {
    maxsize = size;
    object_list = new T [maxsize];
    available = new BossaStack <T*> (maxsize);
    initStack();
  }

  T *allocate () {
    if (available->isEmpty()) fatal ("allocate", "no object available");
    return available->pop();
  }
		
  void deallocate (T *obj) {
    if (obj<&object_list[0] || obj>=&object_list[maxsize]) {fatal ("deallocate", "invalid object");}
    available->push (obj);
  }

  ~BossaAllocator () {
    delete [] object_list;
    delete available;
  }
};

#endif
