/* 
 * Copyright (c) 2005 Clemson University.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#pragma warning(disable: 4786)
#include "Image.h"
#include "ImageOperations.h"
#include "Utilities/Math.h"
#include "Figure/Figure.h"  // debugging
#include "EquivalenceTable.h"

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

using namespace blepo;

// ================> begin local functions (available only to this translation unit)
namespace
{

///////////////////////////////////////////////////////////////////////
// Split

class QuadNode
{
public:
  QuadNode(const Rect& rect) : m_rect(rect)
  {
    for (int i=0 ; i<4 ; i++)  m_children[i] = NULL;
  }
  QuadNode(const QuadNode& other);                  // disallow copy constructor
  QuadNode& operator=(const QuadNode& other);       // disallow assignment operator
  ~QuadNode()
  {
    for (int i=0 ; i<4 ; i++)  delete m_children[i];
  }
  void Split()
  {
    int l = m_rect.left, t = m_rect.top, r = m_rect.right, b = m_rect.bottom;
    int lr = l + (r-l)/2, tb = t + (b-t)/2;
    m_children[0] = new QuadNode(Rect(l,  t,  lr, tb));  // topleft
    m_children[1] = new QuadNode(Rect(lr, t,  r,  tb));  // topright
    m_children[2] = new QuadNode(Rect(lr, tb, r,  b ));  // bottomright
    m_children[3] = new QuadNode(Rect(l,  tb, lr, b ));  // bottomleft
  }
  const Rect& GetRect() const           { return m_rect; }
  QuadNode* GetChild(int i)             { return m_children[i]; }
  const QuadNode* GetChild(int i) const { return m_children[i]; }
  bool IsLeaf() const
  {
    // check to make sure either all children exist or no children exist
    assert( (GetChild(0) != 0) == (GetChild(1) != 0) 
      && (GetChild(0) != 0) == (GetChild(2) != 0) 
      && (GetChild(0) != 0) == (GetChild(3) != 0));
    return (GetChild(0) == 0);
  }
private:
  QuadNode* m_children[4];  // topleft, topright, bottomright, bottomleft
  Rect m_rect;              // pixel coordinates of rect enclosed by region
};

void RecursiveSplit(const ImgGray& img, QuadNode* node, float th)
{
  const Rect& r = node->GetRect();
  int w = (r.right - r.left);
  int h = (r.bottom - r.top);
  if (w > 0 && h > 0 && (w > 1 || h > 1))
  {
    float std = StandardDeviation(img, node->GetRect());
    if (std > th)
    {
      node->Split();
      for (int i=0 ; i<4 ; i++)  RecursiveSplit(img, node->GetChild(i), th);
    }
  }
}

struct Region
{
  Region() : n(0), sum(0), sum_squared(0) {}
  Region(int nn, float s, float ss) : n(nn), sum(s), sum_squared(ss) {}
  int n;
  double sum, sum_squared;
};

void RecursiveSetLabels(const QuadNode& node, const ImgGray& img, int* label, ImgInt* labels, std::vector<Region>* regions)
{
  if ( node.IsLeaf() )
  {
    // set labels of pixels in rect, and store region properties
    const Rect& r = node.GetRect();
    int& a = *label;
    Set(labels, r, a);
    assert( regions->size() == a );
    int n = r.Width() * r.Height();
    if (n > 0)
    {
      Region reg(n, (float) Sum(img, r), (float) SumSquared(img, r) );
      regions->push_back( reg );
      assert(reg.n > 0);
      a++;
    }
  }
  else
  {
    for (int i=0 ; i<4 ; i++)
    {
      RecursiveSetLabels(*node.GetChild(i), img, label, labels, regions);
    }
  }  
}

void RecursiveSetMeanValues(const ImgGray& img, const QuadNode& node, ImgGray* out)
{
  if ( node.IsLeaf() )
  {
    const Rect& r = node.GetRect();
    if (r.Width() > 0 && r.Height() > 0)
    {
      int mu = blepo_ex::Round(Mean(img, r));
      mu = blepo_ex::Max(0, blepo_ex::Min(255, mu));
      Set(out, r, mu);
    }
  }
  else
  {
    for (int i=0 ; i<4 ; i++)
    {
      RecursiveSetMeanValues(img, *node.GetChild(i), out);
    }
  }
}


/////////////////////////////////////////////////////////////////////////
// Merge

// returns the standard deviation of the region formed by combining 'reg1' and 'reg2'
// Note:  This function computes the biased mean b/c it divides by 'n'
float CombinedStandardDeviation(const Region& reg1, const Region& reg2)
{
  int n = ( reg1.n + reg2.n );
  double mu = ( reg1.sum + reg2.sum ) / n;
  return (float) sqrt( ( reg1.sum_squared + reg2.sum_squared ) / n - mu * mu );
}

void MergeRegions(Region* reg1, Region* reg2)
{
  reg1->n += reg2->n;
  reg1->sum += reg2->sum;
  reg1->sum_squared += reg2->sum_squared;

  reg2->n = 0;
  reg2->sum = 0;
  reg2->sum_squared = 0;
}

int GetNPixelsWithLabel(const ImgInt& labels, const EquivalenceTable& equiv_table, int label, bool trace = false)
{
  if (trace)
  {
    TRACE("label %d: ", label);
  }
  // for each pixel in the image, ...
  ImgInt::ConstIterator p;
  int n = 0;
  for (p = labels.Begin() ;  p != labels.End() ; p++)  
  {
    int a = equiv_table.GetEquivalentLabelRecursive( *p );
    if (a == label)  
    {
      if (trace)
      {
        int y = (p - labels.Begin()) / labels.Width();
        int x = (p - labels.Begin()) % labels.Width();
        TRACE("(%d,%d):%d,%d ", x, y, labels(x,y), equiv_table.GetEquivalentLabelRecursive(labels(x,y)));
      }
      n++;
    }
  }
  if (trace)
  {
    TRACE("\n");
    int x = 27, y = 8;
    TRACE("** (%d,%d):%d,%d\n", x, y, labels(x,y), equiv_table.GetEquivalentLabelRecursive(labels(x,y)));
  }
  return n;
}

// labels:  input and output
void Merge(const ImgGray& img, ImgInt* labels, std::vector<Region>* regs, float th)
{
  EquivalenceTable equiv_table;
  equiv_table.EnsureAllocated(regs->size()-1);  // to allocate memory in table

  // for each pixel in the label image, ...
  for (int y=0 ; y<labels->Height() ; y++)
  {
    for (int x=0 ; x<labels->Width() ; x++)
    {
      // get label of pixel and its neighbors
      int label = (*labels)(x, y);
      label = equiv_table.GetEquivalentLabelRecursive( label );

      if (y > 0)
      {
        int label_above = (*labels)(x, y-1);
        label_above = equiv_table.GetEquivalentLabelRecursive( label_above );
        if (label != label_above)
        {
          Region& reg1 = (*regs)[label];
          Region& reg2 = (*regs)[label_above];
          float std = CombinedStandardDeviation(reg1, reg2);
          if (std <= th)
          {
            if (label < label_above)
              MergeRegions(&reg1, &reg2);
            else
              MergeRegions(&reg2, &reg1);
            equiv_table.AddEquivalence(label, label_above);
          }
        }
      }
      label = equiv_table.GetEquivalentLabelRecursive( label );
      if (x > 0)
      {
        int label_left = (*labels)(x-1, y);
        label_left = equiv_table.GetEquivalentLabelRecursive( label_left );
        if (label != label_left)
        {
          Region& reg1 = (*regs)[label];
          Region& reg2 = (*regs)[label_left];
          float std = CombinedStandardDeviation(reg1, reg2);
          if (std <= th)
          {
            if (label < label_left)
              MergeRegions(&reg1, &reg2);
            else
              MergeRegions(&reg2, &reg1);
            equiv_table.AddEquivalence(label, label_left);
          }
        }
      }

    }
  }

  // traverse links in equivalence table
  equiv_table.TraverseLinks();
  equiv_table.RemoveGaps();

  // for each pixel in the image, ...
  ImgInt::Iterator p;
  for (p = labels->Begin() ;  p != labels->End() ; p++)  
  {
    // set the pixel's label to its equivalent label
    *p = equiv_table.GetEquivalentLabel( *p );
  }
}

};
// ================< end local functions

void TestSplitAndMergeOutput(const ImgGray& img, const ImgInt& labels, float th, float* split_score, float* merge_score)
{
  std::vector<Region> regs;
  { // gather region information
    ImgGray::ConstIterator p = img.Begin();
    ImgInt::ConstIterator q = labels.Begin();
    while (p != img.End())
    {
      int val = *p++;
      int i = *q++;
      if (i >= (int) regs.size())  regs.resize(i+1);
      regs[i].n++;
      regs[i].sum += val;
      regs[i].sum_squared += val * val;
    }
  }

  { // check individual regions
    int good = 0;
    for (int i=0 ; i<(int)regs.size() ; i++)
    {
      int n = regs[i].n;
      float mu = (float) (regs[i].sum / n);
      float var = (float) (regs[i].sum_squared / n - mu * mu);
      if (sqrt(var) <= th)  good++;
    }
    *split_score = ((float) good) / regs.size();
  }

  { // check neighboring regions
    int good = 0, total = 0;
    float std;
    for (int y=0 ; y<img.Height() ; y++)
    {
      for (int x=0 ; x<img.Width() ; x++)
      {
        int a = labels(x, y);
        if (y>0)
        {
          int b = labels(x, y-1);
          if (a != b)  
          {
            std = CombinedStandardDeviation(regs[a], regs[b]);
            if (std >= th)  good++;
            total++;
          }
        }
        if (x>0)
        {
          int b = labels(x-1, y);
          if (a != b)
          {
            std = CombinedStandardDeviation(regs[a], regs[b]);
            if (std >= th)  good++;
            total++;
          }
        }
      }
    }
    *merge_score = ((float) good) / total;
  }
}

//void SetMeans(const ImgInt& labels, const std::vector<Region>& regions, ImgGray* means)
//{
//  means->Reset(labels.Width(), labels.Height());
//  ImgInt::ConstIterator p = labels.Begin();
//  ImgGray::Iterator q = means->Begin();
//  while (p != labels.End())
//  {
//    const Region& reg = regions[*p++];
//    float mu = reg.sum / reg.n;
//    mu = blepo_ex::Clamp(mu, 0.0f, 255.0f);
//    *q++ = (unsigned char) mu;
//  }
//}


///////////////////////////////////////////////////////////////////////
// Split-And-Merge

namespace blepo
{

/**
  Split-and-merge semgentation using gray-level variance as the homogeneity
  criterion.  This is a modernized version of the Horowitz and Pavlidis algorithm.
  The split step recurses using a quad tree data structure, while the merge step
  scans the image in a manner similar to connected components.

  @author Stan Birchfield (STB)
*/

void SplitAndMergeSegmentation(const ImgGray& img, ImgInt* labels, float th_std)
{
//  static Figure fig1("after split"), fig2("mean gray levels");
  labels->Reset(img.Width(), img.Height());

  // split using quad-tree
  QuadNode root_node( Rect(0, 0, img.Width(), img.Height()) );
  RecursiveSplit(img, &root_node, th_std);

  // create labels image
  std::vector<Region> regions;
  Set(labels, 0);
  int label = 0;
  RecursiveSetLabels(root_node, img, &label, labels, &regions);

  // convert to pseudo-random colors
//  ImgBgr display;
//  PseudoColor(*labels, &display);
//  fig1.Draw(display);

//  ImgGray means(img.Width(), img.Height());
//  RecursiveSetMeanValues(img, root_node, &means);
//  fig2.Draw(means);

  // merge
  Merge(img, labels, &regions, th_std);

//  float split_score, merge_score;
//  TestSplitAndMergeOutput(img, *labels, th_std, &split_score, &merge_score);
//  CString msg;
//  msg.Format("score = %f %f", split_score, merge_score);
//  AfxMessageBox(msg);
}

};  // end namespace blepo




