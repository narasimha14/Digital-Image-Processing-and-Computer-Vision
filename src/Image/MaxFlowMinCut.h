#ifndef __BLEPO_MAXFLOWMINCUT_H__
#define __BLEPO_MAXFLOWMINCUT_H__

#include <vector>

//////////////////////////////////////////////////////////////////////////////////////////////
// Data structures for computing maximum flow of a graph.
//
// A graph is a vector of nodes, where a node is a vector of edges,
// where an edge is a pair of node ids and a weight.  This is a
// redundant representation because the index into the graph yields 
// the id of one of the nodes shared by all the edges of that node.
// Although the representation could be improved, it is simple and it works.

struct Edge
{
  int m_node1;     // starting node
  int m_node2;     // ending node 
  double m_weight; // edge capacity
};

struct Node
{
  std::vector<Edge> edges; // node is set of edges
};

typedef std::vector<Node> Graph; // Graph is an array of nodes

/////////////////////////////////////////////////////////////////////////////////////////////
// Computes the maximum s-t flow (i.e., from the source to the sink) of a graph,
// and its accompanying minimum cut.  The min cut is the set of edges with minimum total
// weight that separates the source from the sink.
//
// Inputs:
//    graph:  A graph (an array of nodes with edges and edge weights)
//    source:  id of source node
//    sink:    id of sink node
// Outputs:
//    assignments:  This vector is resized to the number of nodes in the graph.  For
//                  each node, its assignment is either
//                     +1 meaning that it is connected to the source after removing the min cut edges, or
//                     -1 meaning that it is connected to the sink after removing the min cut edges.
// Returns the maximum flow (i.e., the sum of the weights of all the edges in the minimum cut)
int ComputeMaxFlowMinCut(const Graph& graph, int source, int sink, std::vector<int>* assignments);

#endif // __BLEPO_MAXFLOWMINCUT_H__
