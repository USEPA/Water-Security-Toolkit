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

#ifndef __vectrie_h
#define __vectrie_h

/* Stefan Chakerian
 * Sandia National Labs
 * Sep 2006
 *
 * VecTrie.h
 * template for vector-based trie (uses vectors as keys)
 * Stores arbitrary data (pointers) in nodes
 * called with VecTrie<Alphabet_type key, Data_type data>
 * Keys are vector<Alphabet_type>, Data_type is arbitrary reference
 *
 * requires vlink.h
 */



#include <iostream>
#include <vector>

#include "vlink.h"

using namespace std;

template <class A> class VecTrieKey;
template <class A, class T> class VecTrieNode;
template <class A, class T> class VecTrie;

template<class A, class T>
class DefaultTrieVisitor
{
public:
   void discover(VecTrieNode<A, T>& n, A nextchar) {}
   void finish(VecTrieNode<A, T>& n) {}
};

template<class A, class T>
class PrintingTrieVisitor : public DefaultTrieVisitor<A, T>
{
public:
   PrintingTrieVisitor() {}
   void discover(VecTrieNode<A, T>* n, A nextchar)
   {
      key.push_back(nextchar);
      std::cout << "TrieNode: ";
      for (int i = 0; i < key.size(); i++)
         std::cout << key[i] << " ";
      std::cout << std::endl;
      if (n->hasdata())
         std::cout << " has data";
      std::cout << std::endl;
      if (n->numlinks() == 0)
         key.pop_back();
   }
private:
   vector<A> key;
};

//================
//  VecTrieKey <A>
//================

template <class A>
class VecTrieKey
{
public:
   VecTrieKey() { };
   ~VecTrieKey() { };
   VecTrieKey(vector<A> &vec) : k(vec)
   {
      cout << "VecTrieKey(vec)" << endl;
   };
   VecTrieKey(VecTrieKey &src) : k(src.k)
   {
      cout << "VecTrieKey(VecTrieKey)" << endl;
   };
   A operator[](int index)
   {
      return k[index];
   }
   VecTrieKey<A> operator=(vector<A> &vec)
   {
      cout << "VecTrieKey::op=(vec)" << endl;
      k = vec;
      return *this;
   }
   bool operator==(VecTrieKey &key)
   {
      return (this.k == key.k);
   }
   vector<A> getkey()
   {
      return k;
   }
   int size()
   {
      return k.size();
   }
   void setkey(vector<A> &veckey)
   {
      k = veckey;
   }
   void setkey(VecTrieKey<A> &key)
   {
      k = key.k;
   }

private:
   vector<A> k;
};


template <class A, class T>
class VecTrie;          // forward decl

//================
//  VecTrieNode <A,T>
//================


template <class A, class T>
class VecTrieNode
{
public:
   VecTrieNode();
   ~VecTrieNode();

   T&      getdata();
   void setdata(T);
   bool hasdata() { return has_data; };  // data is valid
   int     numlinks();       // links.size()
   A    link(int);        // direct access links[]
   VecTrieNode<A, T>*   linknode(int);   // direct access links[]
private:
   friend class VecTrie<A, T>;
   T    data;
   bool has_data;
   vlink<A, VecTrieNode> links;
};

template <class A, class T>
VecTrieNode<A, T>::VecTrieNode()
{
   has_data = false;
};

// getdata
template <class A, class T>
T& VecTrieNode<A, T>::getdata()
{
   if (!has_data)
      cout << "VecTrieNode::getdata on empty node?" << endl;
   return data;
}

// setdata
template <class A, class T>
void VecTrieNode<A, T>::setdata(T newdata)
{
   data = newdata;
   has_data = true;
   return;
}

// link
template <class A, class T>
A VecTrieNode<A, T>::link(int i)
{
   return links.getval(i);
}

// linknode
template <class A, class T>
VecTrieNode<A, T>* VecTrieNode<A, T>::linknode(int i)
{
   return links[i];
}

// numlinks
template <class A, class T>
int VecTrieNode<A, T>::numlinks()
{
   return links.size();
}

//================
//  VecTrie <A,T>
//================

template <class A, class T>
class VecTrie
{
public:
   VecTrie();
   virtual ~VecTrie() { };
   virtual VecTrieNode<A, T>* insert(VecTrieKey<A>&, T);
   virtual T&       getdata(VecTrieKey<A>&);
   virtual void     setdata(VecTrieKey<A>&, T data);
   virtual VecTrieNode<A, T>* getnode(VecTrieKey<A>&);
   virtual bool     delnode(VecTrieKey<A>&);
   bool       hasdata(VecTrieKey<A>& key);
   template <class visitor>
   void       dfs(visitor vis);
// headnode() allows operations on trie as VecTrieNode
   VecTrieNode<A, T>*      headnode();

// disable copy and assignment
   VecTrie(const VecTrie<A, T>&);           // fake copy
   VecTrie       operator=(const VecTrie<A, T>&);  // fake =
private:
   VecTrieNode<A, T>* head;
   template <class visitor>
   void       dfs(VecTrieNode<A, T>*, A, visitor vis);
};


// constructor
template <class A, class T>
VecTrie<A, T>::VecTrie()
{
   head = new VecTrieNode<A, T>;
};

// insert
template <class A, class T>
VecTrieNode<A, T>* VecTrie<A, T>::insert(VecTrieKey<A>& key, T data)
{
   VecTrieNode<A, T> *p, *t;
   int i, j;
   p = head;

   if (key.size() == 0)
   {
      cout << "insert: empty key?" << endl;
      return NULL;
   }
   for (i = 0; i < key.size(); ++i)
   {
      j = p->links.insert(key[i]); // inserts link if not there, returns loc
      if ((t = p->links.getlink(j)) == NULL)
      {
         t = new VecTrieNode<A, T>();
         p->links.setlink(j, t);
      }
      p = t;
   }
   p->setdata(data);
   return p;
}


// getnode
template <class A, class T>
VecTrieNode<A, T>* VecTrie<A, T>::getnode(VecTrieKey<A>& key)
{
   VecTrieNode<A, T> *p, *t;
   int i, j;
   p = head;

   if (key.size() == 0)
   {
      cout << "getnode: empty key?" << endl;
      return NULL;
   }
   for (i = 0; i < key.size(); ++i)
   {
      if (!p || p->links.size() == 0 || ! p->links.find(key[i]))
         return NULL;
      j = p->links.findloc(key[i]);
      t = p->links.getlink(j);
      if (i + 1 == key.size())
         return t;
      p = t;
   }
   return NULL; // not reached, I hope
}

// hasdata
template <class A, class T>
bool VecTrie<A, T>::hasdata(VecTrieKey<A>& key)
{
   VecTrieNode<A, T> *p = this->getnode(key);
   if (p)
      return p->hasdata();
   else
      return NULL;
}


// getdata
template <class A, class T>
T&VecTrie<A, T>::getdata(VecTrieKey<A>& key)
{
   VecTrieNode<A, T> *p = getnode(key);
   if (p != NULL && p->hasdata())
   {
      return p->getdata();
   }
}


// setdata
template <class A, class T>
void VecTrie<A, T>::setdata(VecTrieKey<A>& key, T data)
{
   VecTrieNode<A, T> *p = getnode(key);
   if (p != NULL && p->hasdata())
   {
      p->setdata(data);
   }
}

template <class A, class T>
bool VecTrie<A, T>::delnode(VecTrieKey<A>& key)
{
   // don't do much atm
   VecTrieNode<A, T> *p = getnode(key);
   if (p != NULL)
   {
      p->has_data = false;
      return true;
   }
   else
   {
      return false;
   }
}

template <class A, class T>
VecTrieNode<A, T>* VecTrie<A, T>::headnode()
{
   return head;
}


// dfs
// DFS on node

template <class A, class T>
template <class visitor>
void VecTrie<A, T>::dfs(visitor vis)
{
   VecTrieNode<A, T> *p, *t;
   int i, j;

   // don't visit the root; it represents the empty string
   j = head->numlinks();
   for (i = 0; i < j; ++i)
   {
      dfs(head->linknode(i), head->link(i), vis);
   }
}

template <class A, class T>
template <class visitor>
void VecTrie<A, T>::dfs(VecTrieNode<A, T>* node, A nextchar, visitor vis)
{
   VecTrieNode<A, T> *p, *t;
   int i, j;

   vis.discover(node, nextchar);
   j = node->numlinks();
   for (i = 0; i < j; ++i)
   {
      dfs(node->linknode(i), node->link(i), vis);
   }
   vis.finish(node);
}


/*
  cout << "node (" << node << ")";
  for (i=0; i< k.size(); ++i) {
    cout << " " << k[i];
  }
  cout << endl;
  cout << "  data: ";
  if (node->hasdata())
    ; //cout << node->getdata() << endl;
  else
    cout << "(none)" << endl;
  if (j = node->numlinks() ) {
    cout << "  " << j << " links: ";
    for (i=0; i < j; ++i) {
      cout << node->link(i) << " ";
    }
    cout << endl;
  } else {
    cout << "  no links" << endl;
  }
  for (i=0; i < j; ++i) {
    cout << endl << "Link " << node->link(i) << endl;
    k.push_back(node->link(i));
    dumpnode(node->linknode(i), k);
    k.pop_back();
  }
*/


#endif
