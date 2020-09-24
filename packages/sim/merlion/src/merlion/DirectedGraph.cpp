#include <merlion/DirectedGraph.hpp>

#include <iostream>

DirectedGraph::DirectedGraph(int N)
   :
   nodes_(N),
   permutation_(N, -1)
{
   std::vector< DAGNode >::iterator n;
   int idx = 0;
   for (n=nodes_.begin(); n!=nodes_.end(); n++) {
      n->visited = false;
      n->checked = false;
      n->orphan = false;
      n->idx = idx;
      idx++;
      top_level_nodes_.insert(&(*n));
   }
}

DirectedGraph::~DirectedGraph()
{
}

void DirectedGraph::AddEdge(int parent_idx, int child_idx)
{
   nodes_[parent_idx].child_nodes.insert(&(nodes_[child_idx]));
   top_level_nodes_.erase(&(nodes_[child_idx]));
}

void DirectedGraph::PrintGraph() const
{
   std::cout << "Printing graph:" << std::endl;
   std::set< DAGNode* >::const_iterator n;
   for (n = top_level_nodes_.begin(); n != top_level_nodes_.end(); n++) {
      PrintNode(*n, "");
   }
}    

bool DirectedGraph::PerformTopologicalSorting()
{
   current_idx_ = 0;


   std::set< DAGNode* >::iterator n;
   for (n = top_level_nodes_.begin(); n != top_level_nodes_.end(); n++) {
      Visit(*n);
   }

   if (current_idx_ == (int)nodes_.size()) {
      return true; 
   }
   else {
      for (unsigned int i=0; i< nodes_.size(); i++) {
         if ((nodes_[i].visited == false) && (nodes_[i].checked == false)) {
            std::vector< DAGNode* > CircularRef;
            bool circular = CheckCircularReference(&(nodes_[i]), CircularRef);
            if (circular == true) {
               std::cout << "WARNING! Circular reference found with nodes: " << std::endl;
               std::vector<DAGNode *>::iterator pos;
               for (pos = CircularRef.begin(); pos != CircularRef.end(); ++pos) {
                  DAGNode *n = *pos;
                  std::cout << " " << n->idx; 
               }
               std::cout << "\n";
            }
            else {
               std::cout << "WARNING! Not all nodes were visited but no circular reference was found." << std::endl;
            }
         }
      }
      return false;
   }
}

bool DirectedGraph::CheckCircularReference(DAGNode *n, std::vector< DAGNode* >& circular_list)
{
   if ((n->checked == false) || ((n->checked == true) && (n->orphan == true))) {
      n->checked = true;
      std::set< DAGNode* >::iterator c;
      for (c=n->child_nodes.begin(); c!=n->child_nodes.end(); ++c) {
         if (CheckCircularReference(*c, circular_list)) {
            n->orphan = true;
            circular_list.push_back(n);
            return true;
         }
      }
      return false;
   }
   else {
      return true;
   }
}

void DirectedGraph::PrintNode(const DAGNode* n, std::string prefix) const
{ 
   std::cout << prefix << n->idx << std::endl;
   std::set< DAGNode* >::const_iterator c;
   std::string newprefix = std::string("   ") + prefix;
   for (c=n->child_nodes.begin(); c!=n->child_nodes.end(); ++c) {
      PrintNode(*c, newprefix);
   }
}

void DirectedGraph::Visit(DAGNode* n)
{
   //  std::cout << "Visiting: " << n->idx << std::endl;
   if (n->visited == false) {
      
      n->visited = true;
      std::set< DAGNode* >::iterator c;
      for (c=n->child_nodes.begin(); c!=n->child_nodes.end(); c++) {
         Visit(*c);
      }

   //    std::cout << "Setting permutation_[" << current_idx_ << "]=" << n->idx << std::endl;
      permutation_[current_idx_] = n->idx;
      current_idx_++;
   }
}
