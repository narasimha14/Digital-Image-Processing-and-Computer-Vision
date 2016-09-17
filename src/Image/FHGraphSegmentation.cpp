/*
Copyright (C) 2006 Pedro Felzenszwalb

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
      You should have received a copy of the GNU General Public License
      along with this program; if not, write to the Free Software
      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

  Code downloaded from http://people.cs.uchicago.edu/~pff/segment/
  and ported to Blepo by Zhichao Chen 2008.
*/

#include "ImageAlgorithms.h"
#include "ImageOperations.h"
#include "Image.h"
#include <algorithm>

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

// added by STB 8/13/2010
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

// ================> begin local functions (available only to this translation unit)
namespace
{
using namespace blepo;

#define WIDTH 4.0
#define THRESHOLD(size, c) (c/size)

template <class T>
inline T square(const T &x) { return x*x; }; 

typedef struct {
  float w;
  int a, b;
} Edge;

typedef struct
{
  int rank;
  int p;
  int size;
} uni_elt;

class Universe
{
public:
  Universe(int elements)
  {
    num = elements;
    for (int i = 0; i < elements; i++)
    {
      uni_elt elt;
      elt.rank = 0;
      elt.size = 1;
      elt.p = i;
      elts.push_back(elt);
    }
  }
  ~Universe(){};
  int find(int x)
  {
    int y = x;
    while (y != elts[y].p)
      y = elts[y].p;
    elts[x].p = y;
    return y;
  };  
  void join(int x, int y)
  {
    if (elts[x].rank > elts[y].rank)
    {
      elts[y].p = x;
      elts[x].size += elts[y].size;
    } 
    else
    {
      elts[x].p = y;
      elts[y].size += elts[x].size;
      if (elts[x].rank == elts[y].rank)
        elts[y].rank++;
    }
    num--;
  }
  int size(int x) const { return elts[x].size; }
  int num_sets() const { return num; }
  
private:
  std::vector<uni_elt>elts;
  int num;
};
  
/* make filters */
#define MAKE_FILTER(name, fun)                                \
  std::vector<float> make_ ## name (float sigma)       \
  {                                                           \
  sigma = max(sigma, 0.01F);			                            \
  int len = (int)ceil(sigma * WIDTH) + 1;                     \
  std::vector<float> mask(len);                               \
  for (int i = 0; i < len; i++)                               \
  {                                                           \
  mask[i] = fun;                                              \
  }                                                           \
  return mask;                                                \
  }
  
MAKE_FILTER(fgauss, (float) exp(-0.5*square(i/sigma)));

void normalize(std::vector<float> &mask)
{
  int len = mask.size();
  float sum = 0;
  int i;
  for (i = 1; i < len; i++) 
  {
    sum += fabs(mask[i]);
  }
  sum = 2*sum + fabs(mask[0]);
  for (i = 0; i < len; i++)
  {
    mask[i] /= sum;
  }
}

/* convolve src with mask.  dst is flipped! */
void convolve_even(const ImgFloat& src, ImgFloat *dst, std::vector<float> &mask)
{
  int width = src.Width();
  int height = src.Height();
  int len = mask.size();
  
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      float sum = mask[0] * src(x, y);
      for (int i = 1; i < len; i++) {
        sum += mask[i] * (src(max(x-i,0), y) + src(min(x+i, width-1), y));
      }
      (*dst)(y, x) = sum;
    }
  }
}

float iDiff(const ImgFloat& r, const ImgFloat& g, const ImgFloat& b,
                         int x1, int y1, int x2, int y2) 
{

  return sqrt(square( r(x1,y1) - r(x2,y2) ) + 
              square( g(x1,y1) - g(x2,y2) ) + 
              square( b(x1,y1) - b(x2,y2) ) );
//  return fabs( r(x1,y1) - r(x2,y2) ) + 
//         fabs( g(x1,y1) - g(x2,y2) ) + 
//         fabs( b(x1,y1) - b(x2,y2) );
  
};

float iDiffDepth(const ImgFloat& r, const ImgFloat& g, const ImgFloat& b, const ImgFloat& d,
                         int x1, int y1, int x2, int y2) 
{

  return sqrt(square( r(x1,y1) - r(x2,y2) ) + 
              square( g(x1,y1) - g(x2,y2) ) + 
			  square( b(x1,y1) - b(x2,y2) ) + 
              square( d(x1,y1) - d(x2,y2) ) );
  
};

void iBuildGraph(const ImgFloat& smooth_r, 
                 const ImgFloat& smooth_g,
                 const ImgFloat& smooth_b,
                 std::vector<Edge> *edges,
                 int *num_edges)
{
  int width = smooth_r.Width();
  int height = smooth_r.Height();
  int num = 0;
  int x, y;
  edges->clear();
  for ( y = 0; y < height; y++)
  {
    for ( x = 0; x < width; x++)
    {
      if (x < width-1)
      {
        Edge edge;
        edge.a = y * width + x;
        edge.b = y * width + (x+1);
        edge.w = iDiff(smooth_r, smooth_g, smooth_b, x, y, x+1, y);
        edges->push_back(edge);
        num++;
      }
      
      if (y < height-1)
      {
        Edge edge;
        edge.a = y * width + x;
        edge.b = (y+1) * width + x;
        edge.w = iDiff(smooth_r, smooth_g, smooth_b, x, y, x, y+1);
        edges->push_back(edge);
        num++;
      }
      
      if ((x < width-1) && (y < height-1)) 
      {
        Edge edge;
        edge.a = y * width + x;
        edge.b = (y+1) * width + (x+1);
        edge.w = iDiff(smooth_r, smooth_g, smooth_b, x, y, x+1, y+1);
        edges->push_back(edge);
        num++;
      }
      
      if ((x < width-1) && (y > 0))
      {
        Edge edge;
        edge.a  = y * width + x;
        edge.b  = (y-1) * width + (x+1);
        edge.w  = iDiff(smooth_r, smooth_g, smooth_b, x, y, x+1, y-1);
        edges->push_back(edge);
        num++;
      }
    }
  }
  *num_edges = num;
}

void iBuildDepthGraph(const ImgFloat& smooth_r, 
                 const ImgFloat& smooth_g,
                 const ImgFloat& smooth_b,
				 const ImgFloat& smooth_d, 
                 std::vector<Edge> *edges,
                 int *num_edges)
{
  int width = smooth_r.Width();
  int height = smooth_r.Height();
  int num = 0;
  int x, y;
  edges->clear();
  for ( y = 0; y < height; y++)
  {
    for ( x = 0; x < width; x++)
    {
      if (x < width-1)
      {
        Edge edge;
        edge.a = y * width + x;
        edge.b = y * width + (x+1);
        edge.w = iDiffDepth(smooth_r, smooth_g, smooth_b, smooth_d, x, y, x+1, y);
        edges->push_back(edge);
        num++;
      }
      
      if (y < height-1)
      {
        Edge edge;
        edge.a = y * width + x;
        edge.b = (y+1) * width + x;
        edge.w = iDiffDepth(smooth_r, smooth_g, smooth_b, smooth_d, x, y, x, y+1);
        edges->push_back(edge);
        num++;
      }
      
      if ((x < width-1) && (y < height-1)) 
      {
        Edge edge;
        edge.a = y * width + x;
        edge.b = (y+1) * width + (x+1);
        edge.w = iDiffDepth(smooth_r, smooth_g, smooth_b, smooth_d, x, y, x+1, y+1);
        edges->push_back(edge);
        num++;
      }
      
      if ((x < width-1) && (y > 0))
      {
        Edge edge;
        edge.a  = y * width + x;
        edge.b  = (y-1) * width + (x+1);
        edge.w  = iDiffDepth(smooth_r, smooth_g, smooth_b, smooth_d, x, y, x+1, y-1);
        edges->push_back(edge);
        num++;
      }
    }
  }
  *num_edges = num;
}


void iExtractRGBColorSpace(const ImgBgr& img, 
					   ImgFloat* B, 
					   ImgFloat* G,
					   ImgFloat* R)
{
  const int ISIZEX = img.Width(), ISIZEY = img.Height();
	const unsigned char *ptri = img.BytePtr();
	float *ptr1 = B->Begin();
  float *ptr2 = G->Begin();
	float *ptr3 = R->Begin();
	int b, g ,r;
	int i;

	for (i=0 ; i<ISIZEX*ISIZEY ; i++)
  {
		b = *ptri++;
		g = *ptri++;
		r = *ptri++;

		*ptr1++ = (float)b;
		*ptr2++ = (float)g;
		*ptr3++ = (float)r;
	}
}

void iSmooth(const ImgFloat &src, float sigma, ImgFloat *out)
{
  std::vector<float> mask = make_fgauss(sigma);
  normalize(mask);
  ImgFloat tmp(src.Height(),src.Width());
  convolve_even(src, &tmp, mask);
  convolve_even(tmp, out, mask);
}

bool lessThan (const Edge& a, const Edge& b) {
  return a.w < b.w;
}

void iSegment_graph(int num_vertices, int num_edges, std::vector<Edge>& edges, float c, Universe *u)
{ 
  // sort edges by weight
  std::sort(&edges[0], &edges[num_edges-1], lessThan);

  // make a disjoint-set forest
   
  // init thresholds
  float *threshold = new float[num_vertices];
  int i;
  for (i = 0; i < num_vertices; i++)
    threshold[i] = THRESHOLD(1,c);
  
  // for each edge, in non-decreasing weight order...
  for (i = 0; i < num_edges; i++) 
  {
    Edge edge = edges[i];
      // components conected by this edge
    int a = u->find(edge.a);
    int b = u->find(edge.b);
    if (a != b) {
//      float foo1 = threshold[a];
//      float foo2 = threshold[b];
      if ((edge.w <= threshold[a]) &&
        (edge.w <= threshold[b])) {
        u->join(a, b);
        a = u->find(a);
        threshold[a] = edge.w + THRESHOLD(u->size(a), c);
//        float foo = threshold[a];
      }
//      else
//      {
//        int foo3 = 0;
//      }
    }
  }
  
  // free up
  delete threshold;
}

void random_rgb(Bgr *c)
{ 
  c->r = rand() % 255 + 1;
  c->g = rand() % 255 + 1;
  c->b = rand() % 255 + 1;
}

// returns index of 'item' if found in 'v'; -1 otherwise
template <typename T>
int iFind(const std::vector<T>& v, T item)
{
  std::vector<T>::const_iterator i;
  for (i=v.begin() ; i!=v.end() ; i++)
  {
    if (*i == item)  return (i-v.begin());
  }
  return -1;
}

};
// ================< end local functions

namespace blepo
{

int FHGraphSegmentation(
        const ImgBgr& img, 
        float sigma, 
        float c, 
        int min_size,
        ImgInt *out_labels, 
        ImgBgr *out_pseudocolors) 
{
  int width = img.Width();
  int height = img.Height();
  int x, y;

  ImgFloat R(width, height),G(width, height),B(width, height);
  iExtractRGBColorSpace(img, &B, &G, &R);
  ImgFloat smooth_R(width, height), smooth_G(width, height), smooth_B(width, height);
  out_labels->Reset(width, height);
  out_pseudocolors->Reset(width, height);
  iSmooth(B, sigma, &smooth_B);
  iSmooth(G, sigma, &smooth_G);
  iSmooth(R, sigma, &smooth_R);
  
  std::vector<Edge> edges;
  int num_edges;
  iBuildGraph(smooth_R, smooth_G, smooth_B, &edges, &num_edges);
  Universe u(width*height);
  iSegment_graph(width*height, num_edges, edges, c, &u);
  
  int i;
  for (i = 0; i < num_edges; i++)
  {
    int a = u.find(edges[i].a); 
    int b = u.find(edges[i].b);
    if ((a != b) && ((u.size(a) < min_size) || (u.size(b) < min_size)))
    {
      u.join(a, b);
    }
  }
  
  int num_ccs = u.num_sets();

  std::vector<Bgr> colors;
  for (i = 0; i < width*height; i++)
  {
    Bgr color;
    random_rgb(&color);
    colors.push_back(color);
  }
  
  for (y = 0; y < height; y++)
  {
    for ( x = 0; x < width; x++)
    {
      int comp = u.find(y * width + x);
      (*out_labels)(x, y) = comp;
      (*out_pseudocolors)(x, y) = colors[comp];
    }
  }  

  return num_ccs;
}

int FHGraphSegmentDepth(
        const ImgBgr& img, 
        const ImgGray& depth,
        float sigma, 
        float c, 
        int min_size,
        ImgInt *out_labels, 
        ImgBgr *out_pseudocolors) 
{
  int width = img.Width();
  int height = img.Height();
  int x, y;

  ImgFloat R(width, height),G(width, height),B(width, height);
  iExtractRGBColorSpace(img, &B, &G, &R);
  ImgFloat smooth_R(width, height), smooth_G(width, height), smooth_B(width, height);
  ImgFloat D(width, height), smooth_D(width, height);
  out_labels->Reset(width, height);
  out_pseudocolors->Reset(width, height);
  Convert(depth,&D);
  iSmooth(B, sigma, &smooth_B);
  iSmooth(G, sigma, &smooth_G);
  iSmooth(R, sigma, &smooth_R);
  iSmooth(D, sigma, &smooth_D);
  
  std::vector<Edge> edges;
  int num_edges;
  iBuildDepthGraph(smooth_R, smooth_G, smooth_B, smooth_D, &edges, &num_edges);
  Universe u(width*height);
  iSegment_graph(width*height, num_edges, edges, c, &u);
  
  int i;
  for (i = 0; i < num_edges; i++)
  {
    int a = u.find(edges[i].a); 
    int b = u.find(edges[i].b);
    if ((a != b) && ((u.size(a) < min_size) || (u.size(b) < min_size)))
    {
      u.join(a, b);
    }
  }
  
  int num_ccs = u.num_sets();

  std::vector<Bgr> colors;
  for (i = 0; i < width*height; i++)
  {
    Bgr color;
    random_rgb(&color);
    colors.push_back(color);
  }
  
  for (y = 0; y < height; y++)
  {
    for ( x = 0; x < width; x++)
    {
      int comp = u.find(y * width + x);
      (*out_labels)(x, y) = comp;
      (*out_pseudocolors)(x, y) = colors[comp];
    }
  }  

  return num_ccs;
}

// 'inplace' is okay
void RemoveGapsFromLabelImage(const ImgInt& labels, ImgInt* out)
{
  int width = labels.Width();
  int height = labels.Height();
  out->Reset(width, height);

  std::vector<int> used;
  int x, y;

  // scan image and create vector of all labels found
  for (y=0 ; y<height ; y++)
  {
    for (x=0 ; x<width ; x++)
    {
      int a = labels(x,y);
      if (iFind(used, a) < 0)
      {
        used.push_back(a);
      }
    }
  }

  // scan image, replacing labels
  for (y=0 ; y<height ; y++)
  {
    for (x=0 ; x<width ; x++)
    {
      int a = labels(x,y);
      int newlabel = iFind(used, a);
      assert( newlabel >= 0 );
      (*out)(x,y) = newlabel;
    }
  }
}

};