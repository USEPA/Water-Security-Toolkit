// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#ifndef graph_h
#define graph_h

#include "basics.h"
#include "bossa_heap.h"
#include "bossa_allocator.h"

class PMGraphNode {
 public:
  double value;
  int neighbor;
  PMGraphNode *next;
};

class PMGraph {
 private:
  int n; //number of vertices
  int m; //number of edges
  int added; //number of edges actually added
  BossaAllocator <PMGraphNode> *allocator;
  PMGraphNode **nodes;
  BossaVertexHeap <double> *heap;

  void addNeighbor (int i, int j, double v) {
    PMGraphNode *node = allocator->allocate();
    node->value = v;
    node->neighbor = j;
    node->next = nodes[i];
    nodes[i] = node;
  }

  void deleteNeighborhood (int i) {
    PMGraphNode *x = nodes[i];
    for (x=nodes[i]; x!=0; x=x->next) {
      allocator->deallocate(x); //this doesn't destroy x
    }
    nodes[i] = 0;
  }


 public:

  int getNEdges() {return m;}

  /*
    void allPairs (double **d) {
    int v, w;
    for (v=1; v<=n; v++) {
    for (w=1; w<=n; w++) {
    d[v][w] = POPSTAR_INFINITY;
    }
    d[v][v] = 0;
    }

    double cost;

    for (int r=1; r<=n; r++) {
    heap->reset();
    for (v=1; v<=n; v++) {
    cost = d[v][r];
    heap->pushBack (v, cost);
    }
    heap->heapify();
    while (!heap->isEmpty()) {
    heap->removeFirst (v, cost);
    d[r][v] = cost;
    for (PMGraphNode *x = nodes[v]; x!=0; x=x->next) {
    heap->insert (x->neighbor, x->value + cost, false);
    }
    }
    }
    }*/


  void dijkstra (int r, double *d) {
    heap->reset();
    heap->insert (r,0);
			
    while (!heap->isEmpty()) {
      int v; double cost; 
      heap->removeFirst(v, cost);
      d[v] = cost;
      for (PMGraphNode *x=nodes[v]; x!=0; x=x->next) {
	heap->insert (x->neighbor, x->value + cost, false);
      }
    }
  }

  PMGraph (int _n, int _m) {
    n = _n; 
    m = _m;
    heap = new BossaVertexHeap <double> (n+1);
    nodes = new PMGraphNode* [n+1];
    for (int i=1; i<=n; i++) nodes[i] = 0;
    allocator = new BossaAllocator <PMGraphNode> (2*m);
  }

  void addEdge (int i, int j, double v) {
    addNeighbor (i, j, v);
    addNeighbor (j, i, v);
  }

  ~PMGraph() {	
    for (int i=1; i<=n; i++)    //this is not strictly necessary, but...
      deleteNeighborhood (i); 
    delete [] nodes;
    delete allocator;
    delete heap;
  }

  void output (FILE *file) {
    for (int i=1; i<=n; i++) {
      fprintf (file, "%d: ", i);
      for (PMGraphNode *x=nodes[i]; x!=0; x=x->next) {
	fprintf (file, "%d ", x->neighbor);
      }
      fprintf (file, "\n");
    }
  }
};

#endif
