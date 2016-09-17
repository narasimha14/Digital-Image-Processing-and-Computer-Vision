#include <vector>
#include <deque>
#include <assert.h>
#include "MaxFlowMinCut.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////
/////////////// functions for computing max-flow    //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

//const int g_nnbrs = 6;

////// Node of a search graph used in BFS 
struct SearchNode{
  int color;          // node status:- not seen: -1, seen but not visited: 0, visited: 1
  int dist;           // distance from the source node
  int parent;         // immediate parent of the node
};

typedef vector<SearchNode> SearchGraph;   // Search graph is an array of search nodes
typedef vector<int> Path;                 // Path is a set of ints representing node ids between source and sink (reverse order)
typedef deque<int> NodeList;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// Initialize a search graph setting all the nodes except for the source node as not seen, at an infinite
////// distance from the source and no parents. Set the status of the source node as seen, distance from the
////// source as zero and no parent. 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void InitSearchGraph(SearchGraph* sgraph, int nnodes, int source)
{
   int i;
   SearchNode snode;
   for (i = 0; i < nnodes; i++)
   {
     if (i != source)
     {
       snode.color = -1;
       snode.dist = 999999999;
       snode.parent = -1;
     }
     else
     {
       snode.color = 0;
       snode.dist = 0;
       snode.parent = -100;
     }
     sgraph->push_back(snode);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// Find the nodes in the neighborhood of a node ina graph. Typically a node has 4 neighbors in a graph 
////// plus the two terminals.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GetNeighboringNodes(int n, const Graph& graph, vector<int>* nbr_nodes)
{
  nbr_nodes->clear();
  const Node& node = graph[n];
  int i;
  int nnbrs = node.edges.size();
  Edge edge;
  for ( i = 0; i < nnbrs; i++)
  {
    edge = node.edges[i];    
    if ((edge.m_node2 >= 0) && (edge.m_node2 < (int) graph.size()))
    {
      if (edge.m_weight > 0)
       nbr_nodes->push_back(edge.m_node2);
    }
  }
}

int GetParentNode(const SearchGraph& sgraph, int v)
{
  const SearchNode& snode = sgraph[v];
  return (snode.parent);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// Find augmenting path between source and sink nodes given a populated search graph.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FindAugmentingPath(const SearchGraph& sgraph, int source, int sink, Path* path)
{
  path->clear();
  bool stop = false;
  //Path::iterator p = path->begin();
  int parent, v = sink;
  path->push_back(sink);
  while (stop == false)
  {
    parent = GetParentNode(sgraph, v);
    if (parent != source)
      v = parent;
    else
      stop = true;

    path->push_back(parent);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// Performs a breadth first search to find shortest available path (with non-saturated path capacities) 
////// between a search node and a sink node. The function uses a search graph to perform the search and 
////// store parent-child relationship of the nodes in the graph.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PerformBFS(const Graph& graph,
           SearchGraph& sgraph,
           int source,
           int sink,
           Path* augpath)
{
  NodeList nlist;
  nlist.push_back(source);
  int u, v;
  int i;
  SearchNode snode_u, snode_v;
  bool found = false;
  vector<int> nbr_nodes;

  while (nlist.size() > 0)
  {
    u = nlist[0];
    GetNeighboringNodes(u, graph, &nbr_nodes);
    snode_u = sgraph[u];
    for (i = 0; i < (int) nbr_nodes.size(); i++)
    {
      v = nbr_nodes[i];
      snode_v = sgraph[v];
      
      if (snode_v.color == -1)
      {
        snode_v.color = 0;
        snode_v.dist = snode_u.dist + 1;
        snode_v.parent = u;
        sgraph[v] = snode_v;
        nlist.push_back(v);
      }
      if (v == sink)
      {
        found = true;
        break;
      }
    }
    nlist.pop_front();
    snode_u.color = 1;
    sgraph[u] = snode_u;
    if (found == true)
      break;
  }
  if (found == true)
    FindAugmentingPath(sgraph, source, sink, augpath);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// Function for searching a path between source and sink node
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SearchAugPath(const Graph& graph, int source, int sink, Path* augpath)
{
  augpath->clear();
  SearchGraph sgraph;
  const int nnodes = graph.size();
  InitSearchGraph(&sgraph, nnodes, source);
  PerformBFS(graph, sgraph, source, sink, augpath);
  if (augpath->size() > 1)
    return 1;
  else
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// Given a path, the function returns the value of a link (edge) in the path having the least capacity.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
double FindPathCapacity(const Graph& graph, int source, const Path& path)
{
  const int path_size = path.size();
  //assert(path[path_size-1] == source);
  int i, j;
  int nedges;
  double min_cap = 999999999, cap;
  for (i = 1; i < path_size; i++)
  {
    const Node& node = graph[path[i]];
    nedges = node.edges.size();
    for (j = 0; j < nedges; j++)
    {
      if (node.edges[j].m_node2 == path[i-1])
      {
        assert(node.edges[j].m_node1 == path[i]);
        cap = node.edges[j].m_weight;
        if (cap < min_cap)
          min_cap = cap;
      }
    } 
  }
  return min_cap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// Push a maximum possible flow through a given path which is equal to the maximum path capacity in the residual graph.
////// The capacities of the links along the forward direction (source to sink) are decreased by the amount equal to
////// the maximum path capacity while the capacities of the paths in the reverse direction are
////// incresed by the same amount.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RemoveAugmentingPath(Graph* graph, const Path& path, double path_cap)
{
  const int path_size = path.size();
  int i, j;
  int nedges;
  Node node;
  for (i = 1; i < path_size; i++)
  {
    node = (*graph)[path[i]];
    nedges = node.edges.size();
    for (j = 0; j < nedges; j++)
    {
      if (node.edges[j].m_node2 == path[i-1])
      {
        node.edges[j].m_weight = node.edges[j].m_weight - path_cap;
      }
    }
    (*graph)[path[i]] = node;
  }

  for (i = 0; i < path_size-1; i++)
  {
    node = (*graph)[path[i]];
    nedges = node.edges.size();
    for (j = 0; j < nedges; j++)
    {
      if (node.edges[j].m_node2 == path[i+1])
      {
        node.edges[j].m_weight = node.edges[j].m_weight + path_cap;
      }
    }
    (*graph)[path[i]] = node;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// Given a residual graph with no path from the source and sink node, the sunction outputs an assignment list
////// which assigns each node to either belonging to the source tree or the sink tree. It performs breadth first
////// search on the residual graph to find the children of the source tress. Any remaining nodes are aressigned
////// to the sink node.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ComputeAssignments(const Graph& graph, int source, int sink, vector<int>* assignments)
{
  SearchGraph sgraph;
  assignments->clear();
  const int nnodes = graph.size();
  InitSearchGraph(&sgraph, nnodes, source);
  assignments->resize(nnodes, -1);
  NodeList nlist;
  nlist.push_back(source);
  int u, v;
  int i;
  SearchNode snode_u, snode_v;
  bool found = false;
  vector<int> nbr_nodes;

  while (nlist.size() > 0)
  {
    u = nlist[0];
    (*assignments)[u] = 1;
    GetNeighboringNodes(u, graph, &nbr_nodes);
    snode_u = sgraph[u];
    for (i = 0; i < (int) nbr_nodes.size(); i++)
    {
      v = nbr_nodes[i];
      snode_v = sgraph[v];
      
      if (snode_v.color == -1)
      {
        snode_v.color = 0;
        snode_v.dist = snode_u.dist + 1;
        snode_v.parent = u;
        sgraph[v] = snode_v;
        nlist.push_back(v);
      }   
    }
    nlist.pop_front();
    snode_u.color = 1;
    sgraph[u] = snode_u;  
  }
}

////// function for printing edge weights ......for debugging purposes
void PrintStatus(const Graph& graph, int source, int sink)
{
  const Node& node = graph[sink];
  for (int i = 0; i < (int) node.edges.size(); i++)
  {
    const Edge& edge = node.edges[i];
    printf("\n %f ", edge.m_weight);
  }
  printf(" \n ");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// Function for computing max-flow in a graph. Inputs are:- a graph (an array of nodes) with neighboring 
////// assignments and path capacities are pre-calculated (initialized), a source node and a sink node. The 
////// returns node assignments (whether a node belongs to the source or the sink).
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ComputeMaxFlowMinCut(const Graph& graph, int source, int sink, vector<int>* assignments)
{
  assignments->clear();
  Path augpath;
  Graph res_graph = graph;
 // SearchGraph sgraph;
  bool stop = false;
  int nitr = 0;
 // const int nnodes = graph.size();

//  InitSearchGraph(&sgraph, nnodes, source);

  while( stop == false)
  {
    bool found = SearchAugPath(res_graph, source, sink, &augpath);
    double path_cap = FindPathCapacity(res_graph, source, augpath);
    RemoveAugmentingPath(&res_graph, augpath, path_cap); 

   if ((nitr == 50000) || (found == false))   
      stop = true;
    nitr++;  
  }
  ComputeAssignments(res_graph, source, sink, assignments);
  return 0;  // Fix this!
}
