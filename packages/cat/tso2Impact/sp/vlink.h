/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top SPOT directory.
 *  _________________________________________________________________________
 */

#ifndef __vlink_h
#define __vlink_h

/* Stefan Chakerian
 * Sandia National Labs
 * Sep 2006
 *
 * vlink.h
 * template for sorted vector of (key, pointer) pairs
 * with binary search based on key, or direct access
 */

#include <iostream>
#include <vector>
using namespace std;

template <class A, class P> class vlink;

template <class A, class P>
class vlinknode
{
private:
   friend class vlink<A, P>;
   A key; // only 1 "letter" ... so single int if using vector<int>
   P* ptr;   // arbitrary pointer of type P*
};

template <class A, class P>
class vlink
{
public:
   vlink() { v.clear(); };
   ~vlink() { };
   bool find(A);
   int findloc(A);
   int insert(A);
   int insert(A, P*);
   int size();
   bool setlink(int, P*);
   A getval(int i)
   {
      return v[i].key;
   }
   P* getlink(int);
   P* operator[](int i)
   {
      return getlink(i);
   }
   void dump();
private:
   vector<vlinknode<A, P> > v;
};

template <class A, class P>
int vlink<A, P>::size()
{
   return v.size();
}

// find
template <class A, class P>
bool vlink<A, P>::find(A key)
{
   int i = this->findloc(key);

   if (v.size() == 0 || (i >= v.size()))
      return false;

   if (v[i].key == key)
      return true;
   else
      return false;
}

// findloc
template <class A, class P>
int vlink<A, P>::findloc(A key)
{
   int i = 0;
   int k = 0;
   int j = v.size();
   bool done = false;

   if (!j)      // size() much be checked by calling function
      return j;     // returns valid 0, but this is a failure

   for (;;)
   {
      k = (i + j) / 2;
      if (v[k].key == key)
         return k;
      if (v[k].key > key)
      {
         j = k;
      }
      else
      {
         i = k + 1;
      }
      if (i == j)
         break;
   }
   return i;
}

// insert(A)
template <class A, class P>
int vlink<A, P>::insert(A key)
{
//  cout << "begin vlink.insert (key="<<key<<")" << endl;
   typename vector<vlinknode<A, P> >::iterator p;
   int i;

   p = v.begin();
   i = findloc(key);

   vlinknode<A, P> n;
   n.key = key;
   n.ptr = NULL;

   if (v.size() && (i < v.size()) && v[i].key == key)
   {
//    printf("%d found at %d, skipping\n", key, i);
   }
   else
   {
//    printf("%d not found, inserting at %d\n", key, i);
      p += i;
      v.insert(p, n);
   }

   return i;
}

// insert(A,P*)
template <class A, class P>
int vlink<A, P>::insert(A key, P *ptr)
{
   typename vector<vlinknode<A, P> >::iterator p = v.begin();
   int i;
   vlinknode<A, P> n;

   n.key = key;
   n.ptr = ptr;

   i = this->findloc(key);

   if (v.size() && (i < v.size()) && v[i].key == key)
   {
//    printf("%d found at %d, skipping\n", key, i);
      v[i].ptr = ptr;
   }
   else
   {
//    printf("%d not found, inserting at %d\n", key, i);
      p += i;
      v.insert(p, n);
   }

   return i;
}

// setlink
template <class A, class P>
bool vlink<A, P>::setlink(int position, P *ptr)
{
   if (v.size() <= position || position < 0)
      return false;
   v[position].ptr = ptr;
   return true;
}

// getlink
template <class A, class P>
P* vlink<A, P>::getlink(int i)
{
   if (v.size() <= i || i < 0)
      return NULL;
   return v[i].ptr;
}

template <class A, class P>
void vlink<A, P>::dump()
{
   for (int i = 0; i < v.size(); i++)
   {
      cout << "key[" << i << "] " << v[i].key << ((v[i].ptr == NULL) ? " (NULL)" : " not NULL") << endl;
   }
}


#endif
