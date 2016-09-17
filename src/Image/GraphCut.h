#pragma warning(disable: 4786)  // identifier was truncated to '255' characters
#include <vector>
#include <deque>
#include <assert.h>
#include <algorithm>

// Implementation of Ford-Fulkerson algorithm on an arbitrary graph.
//
// node 0:  source
// node 1:  sink

class sGraph
{
public:
  enum { SOURCE_NODE = 0, SINK_NODE = 1 };

  struct Edge { int u, v; };

  struct WEdge { int v, w;  WEdge(int vv, int ww): v(vv), w(ww) {} };

  typedef std::vector< WEdge > AdjacencyList;

  typedef std::vector< Edge > Cut;

  typedef std::deque<int> Path;

  std::vector< AdjacencyList > m_vertices;

  void SetVertices(int n)
  {
    assert( n >= 2 );
    m_vertices.resize(n);
  }

  void AddEdge(int u, int v, int weight)
  {
    assert( u >= 0 && u < m_vertices.size() );
    assert( v >= 0 && v < m_vertices.size() );
    m_vertices[u].push_back( WEdge( v, weight ) );
  }

  int FindWeight( int u, int v ) const
  {
    const AdjacencyList& adj = m_vertices[u];
    for (int j=0 ; j<adj.size() ; j++)
    {
      const WEdge& wedge = adj[j];
      if (wedge.v == v)  return wedge.w;
    }
    assert(0);  // edge not found
    return 0;
  }

  const WEdge& FindEdge( int u, int v ) const
  {
    const AdjacencyList& adj = m_vertices[u];
    for (int j=0 ; j<adj.size() ; j++)
    {
      const WEdge& wedge = adj[j];
      if (wedge.v == v)  return wedge;
    }
    assert(0);  // edge not found
    return adj[0];
  }

  void RemoveEdge( int u, int v )
  {
    WEdge& wedge = FindEdge( u, v );
    m_vertices[u].erase( &wedge );
  }

  WEdge& FindEdge( int u, int v )
  {
    AdjacencyList& adj = m_vertices[u];
    for (int j=0 ; j<adj.size() ; j++)
    {
      WEdge& wedge = adj[j];
      if (wedge.v == v)  return wedge;
    }
    assert(0);  // edge not found
    return adj[0];
  }

  int FindPath( Path* path )
  {
    std::deque< int > frontier;
    std::vector< int > pred( m_vertices.size(), -1 ); // predecessors
    frontier.push_back( SOURCE_NODE );
    while ( 1 )
    {
      if ( frontier.empty() )  return 0;  // no path found
      int u = frontier.back();
      frontier.pop_back();
      AdjacencyList& adj = m_vertices[u];
      for (int i=0 ; i<adj.size() ; i++)
      {
        int v = adj[i].v;
        if ( pred[v] == -1 )  // node has never been encountered (no need to allow loops)
        {
          pred[v] = u;
          if (v == SINK_NODE)  goto done;
//          frontier.push_front( v );  // 'push_front' is breadth-first
          frontier.push_back( v );  // 'push_back' is depth-first
        }        
      }
    }
done:
    // compute flow
    path->clear();
    int v = SINK_NODE, u;
    path->push_front( v );
    int flow = 99999999;
    do
    {
      u = pred[v];
      path->push_front( u );
      int w = FindWeight(u, v);
      if (w < flow)  flow = w;
      v = u;
    }
    while (u != SOURCE_NODE);
    return flow;
  }

  void ComputeResidualGraph( const Path& path, int path_flow )
  {
    for (int i=1 ; i<path.size() ; i++)
    {
      int u = path[i-1];
      int v = path[i];
      WEdge& wedge = FindEdge( u, v );
      assert( wedge.w >= path_flow );
      wedge.w -= path_flow;
      if (wedge.w == 0)  RemoveEdge( u, v );  //adj.erase( adj.begin() + j );
//
//
//      AdjacencyList& adj = m_vertices[u];
//      for (int j=0 ; j<adj.size() ; j++)
//      {
//        WEdge& wedge = adj[j];
//        if (wedge.v == v)
//        {
//          assert( wedge.w >= path_flow );
//          wedge.w -= path_flow;
//          if (wedge.w == 0)  adj.erase( adj.begin() + j );
//          break;
//        }
//      }
    }
  }

  int MaxFlowMinCut( Cut* mincut )
  {
    Path path;
    int flow = 0;
    while (1)
    {
      int path_flow = FindPath( &path );
      if ( path_flow == 0 )  break;
      flow += path_flow;
      ComputeResidualGraph( path, path_flow );
    }
    return flow;
  }
};


/*
#pragma warning(disable: 4786)  // identifier was truncated to '255' characters
#include <vector>
#include <deque>
#include <assert.h>
#include <algorithm>

// Note:  danger of depth first search is cycles in graph can cause infinite loop
// node 0:  source
// node 1:  sink

class sGraph
{
public:
  enum { SOURCE_NODE = 0, SINK_NODE = 1 };

  struct Edge { int u, v; };

  struct WEdge { int v, w;  WEdge(int vv, int ww): v(vv), w(ww) {} };

  typedef std::vector< WEdge > AdjacencyList;

  typedef std::vector< Edge > Cut;

  typedef std::vector<int> Path;

  std::vector< AdjacencyList > m_vertices;

  void SetVertices(int n)
  {
    assert( n >= 2 );
    m_vertices.resize(n);
  }

  void AddEdge(int u, int v, int weight)
  {
    assert( u >= 0 && u < m_vertices.size() );
    assert( v >= 0 && v < m_vertices.size() );
    m_vertices[u].push_back( WEdge( v, weight ) );
  }

  int FindWeight( int u, int v ) const
  {
    const AdjacencyList& adj = m_vertices[u];
    for (int j=0 ; j<adj.size() ; j++)
    {
      const WEdge& wedge = adj[j];
      if (wedge.v == v)  return wedge.w;
    }
    assert(0);  // edge not found
    return 0;
  }

  const WEdge& FindEdge( int u, int v ) const
  {
    const AdjacencyList& adj = m_vertices[u];
    for (int j=0 ; j<adj.size() ; j++)
    {
      const WEdge& wedge = adj[j];
      if (wedge.v == v)  return wedge;
    }
    assert(0);  // edge not found
    return adj[0];
  }

  void RemoveEdge( int u, int v )
  {
    WEdge& wedge = FindEdge( u, v );
    m_vertices[u].erase( &wedge );
  }

  WEdge& FindEdge( int u, int v )
  {
    AdjacencyList& adj = m_vertices[u];
    for (int j=0 ; j<adj.size() ; j++)
    {
      WEdge& wedge = adj[j];
      if (wedge.v == v)  return wedge;
    }
    assert(0);  // edge not found
    return adj[0];
  }

  // does not work, must be a better way
  bool DetectCycle( const Path& path )
  {
    for ( int i = 0, j = 1 ; j < path.size() ; i+=2, j++ )
    {
      if ( path[i] == path[j] )  return true;
    }
    return false;
  }

  int FindPath( Path* path )
  { // depth first search
    std::deque< Path > frontier;
    frontier.push_back( Path( 1, SOURCE_NODE ) );
    while ( 1 ) //new_node != SINK_NODE)
    {
      if ( frontier.empty() )  return 0;
      Path p = frontier.back();
      frontier.pop_back();
      if ( DetectCycle( p ) )  continue;
      int u = p.back();
      AdjacencyList& adj = m_vertices[u];
      for (int i=0 ; i<adj.size() ; i++)
      {
        *path = p;
        int v = adj[i].v;
        path->push_back( v );
        if (v == SINK_NODE)  goto done;
        else  frontier.push_front( *path );  // 'push_front' is breadth-first
      }
    }
done:
    // compute flow
    std::vector<int> flow_vals;
    for (int i=1 ; i<path->size() ; i++)
    {
      int u = (*path)[i-1];
      int v = (*path)[i];
      flow_vals.push_back( FindWeight(u, v) );
    }
    std::vector<int>::iterator flow = std::min_element( flow_vals.begin(), flow_vals.end() );
    return *flow;
  }

  void ComputeResidualGraph( const Path& path, int path_flow )
  {
    for (int i=1 ; i<path.size() ; i++)
    {
      int u = path[i-1];
      int v = path[i];
      WEdge& wedge = FindEdge( u, v );
      assert( wedge.w >= path_flow );
      wedge.w -= path_flow;
      if (wedge.w == 0)  RemoveEdge( u, v );  //adj.erase( adj.begin() + j );
//
//
//      AdjacencyList& adj = m_vertices[u];
//      for (int j=0 ; j<adj.size() ; j++)
//      {
//        WEdge& wedge = adj[j];
//        if (wedge.v == v)
//        {
//          assert( wedge.w >= path_flow );
//          wedge.w -= path_flow;
//          if (wedge.w == 0)  adj.erase( adj.begin() + j );
//          break;
//        }
//      }
    }
  }

  int MaxFlowMinCut( Cut* mincut )
  {
    Path path;
    int flow = 0;
    while (1)
    {
      int path_flow = FindPath( &path );
      if ( path_flow == 0 )  break;
      flow += path_flow;
      ComputeResidualGraph( path, path_flow );
    }
    return flow;
  }
};
*/
