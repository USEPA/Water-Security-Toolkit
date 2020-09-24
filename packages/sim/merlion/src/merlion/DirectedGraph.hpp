#ifndef MERLION_DIRECTEDGRAPH_HPP__
#define MERLION_DIRECTEDGRAPH_HPP__

#include <vector>
#include <set>
#include <string>

class DirectedGraph
{
public:
   DirectedGraph(int N);

   ~DirectedGraph();

   void AddEdge(int parent_idx, int child_idx);

   void PrintGraph() const;

   bool PerformTopologicalSorting();

   const std::vector< int >& Permutation() const
   { return permutation_; }

private:
   class DAGNode
   {
   public:
      std::set< DAGNode* > child_nodes;
      bool visited;
      bool checked;
      bool orphan;
      int idx;
   };

   void PrintNode(const DAGNode* n, std::string prefix) const;
   bool CheckCircularReference(DAGNode *n, std::vector< DAGNode* >& circular_list);
   void Visit(DAGNode* n);

   std::vector< DAGNode > nodes_;
   std::set< DAGNode* > top_level_nodes_;
   std::vector< int > permutation_;
   int current_idx_;
};

#endif
