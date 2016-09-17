/* 
 * Copyright (c) 2004,2005 Clemson University.
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

#include "ImageAlgorithms.h"
#include "Utilities/Math.h"
#include "EquivalenceTable.h"
#include <vector>

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

// ================> begin local functions (available only to this translation unit)
namespace
{
using namespace blepo;

template <typename T>
void iUpdateProperties(std::vector<ConnectedComponentProperties<T> >* props, int label, int x, int y, T val)
{
  assert(label >= 0 && label <= (int) props->size());
  if (label == props->size())  
  {
    // new component
    ConnectedComponentProperties<T> p( Rect(x, y, x+1, y+1), Point(x, y), 1, val );
    props->push_back(p);
  }
  else
  {
    // existing component
    ConnectedComponentProperties<T>& p = (*props)[label];
    Rect& r = p.bounding_rect;
    if (x <  r.left )   r.left = x;
    if (x >= r.right)   r.right = x+1;
    if (y <  r.top  )   r.top = y;
    if (y >= r.bottom)  r.bottom = y+1;
    p.npixels++;
    assert(p.value == val);
  }
}

template <typename T>
void iMergeProperties(std::vector<ConnectedComponentProperties<T> >* props, const EquivalenceTable& equiv_table)
{
  int n = props->size();
  for (int a = 0 ; a < n ; a++)
  {
    int b = equiv_table.GetEquivalentLabel(a);
    assert(b >= 0 && b <= (int) props->size());
    assert(b <= a);
    if (a != b)
    {
      // merge region [a] and [b], storing it in [b]
      ConnectedComponentProperties<T>& aa = (*props)[a];
      ConnectedComponentProperties<T>& bb = (*props)[b];
      assert(aa.value == bb.value);
      Rect tmp;
      tmp.UnionRect(aa.bounding_rect, bb.bounding_rect);
      bb.bounding_rect = tmp;
      bb.npixels += aa.npixels;
      // empty the merged region [a]
      aa.npixels = 0;
    }
  }
}

template <typename T>
void iRemoveGaps(std::vector<ConnectedComponentProperties<T> >* props)
{
  // condense properties vector by removing gaps
  int a, next = 0;
  for (a = 0 ; a < (int) props->size() ; a++) 
  {
    if ( (*props)[a].npixels != 0)
    {
      (*props)[next++] = (*props)[a];
    } 
  }
  int iter = props->size() - next;
  for ( ; iter > 0 ; iter--)  {
    props->pop_back();
  }
}

template <typename T>
void iAddAdjacency(int lab1, int lab2, std::vector<ConnectedComponentProperties<T> >* props)
{
  assert((int) props->size() > lab1 && (int) props->size() > lab2);
  int i;
  { // insert lab2 into lab1's list, keeping labels in ascending order
    // if lab2 has already been added previously, then just return
    Array<int>& adj = (*props)[lab1].adjacent_regions;
    for (i=0 ; i<adj.Len(); i++)
    {
      if (adj[i] == lab2)  return;
      if (adj[i] > lab2)
      {
        break;
      }
    }
    adj.Insert(lab2, i);
  }

  { // insert lab1 into lab2's list, keeping labels in ascending order
    // if we get to this point, then lab1 should not have been added previously
    // (I.e., if lab1 doesn't know about lab2, then lab2 shouldn't know about lab1 -- yet)
    Array<int>& adj = (*props)[lab2].adjacent_regions;
    for (i=0 ; i<adj.Len(); i++)
    {
      assert(adj[i] != lab1);
      if (adj[i] > lab1)
      {
        break;
      }
    }
    adj.Insert(lab1, i);
  }
}

template <typename T>
void iComputeAdjacency4(const ImgInt& labels, std::vector<ConnectedComponentProperties<T> >* props)
{
  // for each pixel in the image, ...
  for (int y=0 ; y<labels.Height() ; y++)
  {
    for (int x=0 ; x<labels.Width() ; x++)
    {
      int lab1 = labels(x, y);
      if (y>0)  
      { 
        int lab2 = labels(x, y-1);
        if (lab1 != lab2)  iAddAdjacency(lab1, lab2, props);
      }
      if (x>0)  
      { 
        int lab2 = labels(x-1, y);
        if (lab1 != lab2)  iAddAdjacency(lab1, lab2, props);
      }
    }
  }
}

template <typename T>
int iConnectedComponents4(const Image<T>& img, ImgInt* labels, std::vector<ConnectedComponentProperties<T> >* props)
{
  assert((void*) labels != (void*) &img);  // 'inplace' not okay
  if (props)  props->clear();
  labels->Reset(img.Width(), img.Height());
  EquivalenceTable equiv_table;
  int new_label_number = 0;
  int max_label_number;

  // for each pixel in the image, ...
  for (int y=0 ; y<img.Height() ; y++)
  {
    for (int x=0 ; x<img.Width() ; x++)
    {

      // get value of pixel and its neighbors
      Image<T>::Pixel pix = img(x, y);
      bool same_as_above = (y==0) ? false : pix == img(x, y-1);
      bool same_as_left  = (x==0) ? false : pix == img(x-1, y);

      // determine the label of the current pixel
      int label;
      if (same_as_above && !same_as_left)
      {
        label = (*labels)(x, y-1);
      } 
      else if (same_as_left && !same_as_above)
      {
        label = (*labels)(x-1, y);
      } 
      else if (same_as_left && same_as_above)
      {
        int label_above = (*labels)(x, y-1);
        int label_left  = (*labels)(x-1, y);
//        label = blepo_ex::Min(label_above, label_left);
        label = label_above;  // set to one of the labels arbitrarily

        // add equivalence relation to equivalent table
        equiv_table.AddEquivalence(label_left, label_above);
      } 
      else
      {
        label = new_label_number++;
        equiv_table.EnsureAllocated(label);
      }

      // set the label of the current pixel
      (*labels)(x, y) = label;
      if (props)  iUpdateProperties(props, label, x, y, pix);
    }
  }

  // clean up equivalence and regions:
  //   1. traverse links in equivalence table
  //   2. merge properties of regions with the same label
  //   3. ensure that the labels are in sequential order:  0, 1, 2, ...
  equiv_table.TraverseLinks();
  if (props)  iMergeProperties(props, equiv_table);
  if (props)  iRemoveGaps(props);
  max_label_number = equiv_table.RemoveGaps();

  // for each pixel in the image, ...
  ImgInt::Iterator p;
  for (p = labels->Begin() ;  p != labels->End() ; p++)  
  {
    // set the pixel's label to its equivalent label
    *p = equiv_table.GetEquivalentLabel( *p );
  }

  // compute adjaceny of regions
  if (props)  iComputeAdjacency4(*labels, props);

  return max_label_number;
}

template <typename T>
void iComputeAdjacency8(const ImgInt& labels, std::vector<ConnectedComponentProperties<T> >* props)
{
  // for each pixel in the image, ...
  for (int y=0 ; y<labels.Height() ; y++)
  {
    for (int x=0 ; x<labels.Width() ; x++)
    {
      int lab1 = labels(x, y);
      if (y>0)  
      { 
        int lab2 = labels(x, y-1);
        if (lab1 != lab2)  iAddAdjacency(lab1, lab2, props);
      }
      if (x>0)  
      { 
        int lab2 = labels(x-1, y);
        if (lab1 != lab2)  iAddAdjacency(lab1, lab2, props);
      }
      if (x>0 && y>0)  
      { 
        int lab2 = labels(x-1, y-1);
        if (lab1 != lab2)  iAddAdjacency(lab1, lab2, props);
      }
      if (x<labels.Width()-1 && y>0)  
      { 
        int lab2 = labels(x+1, y-1);
        if (lab1 != lab2)  iAddAdjacency(lab1, lab2, props);
      }
    }
  }
}

template <typename T>
int iConnectedComponents8(const Image<T>& img, ImgInt* labels, std::vector<ConnectedComponentProperties<T> >* props)
{
  assert((void*) labels != (void*) &img);  // 'inplace' not okay
  if (props)  props->clear();
  labels->Reset(img.Width(), img.Height());
  EquivalenceTable equiv_table;
  int new_label_number = 0;
  int max_label_number;

  // for each pixel in the image, ...
  for (int y=0 ; y<img.Height() ; y++)
  {
    for (int x=0 ; x<img.Width() ; x++)
    {

      // get value of pixel and its neighbors
      Image<T>::Pixel pix = img(x, y);
      bool same_as_above = (y==0        ) ? false : pix == img(x, y-1);
      bool same_as_left  = (x==0        ) ? false : pix == img(x-1, y);
      bool same_as_ul    = (x==0 || y==0) ? false : pix == img(x-1, y-1);
      bool same_as_ur    = (x==img.Width()-1 || y==0) ? false : pix == img(x+1, y-1);

      int label = -1;
      if (same_as_above)
      {
        label = (*labels)(x, y-1);
      }
      if (same_as_left)
      {
        int label2 = (*labels)(x-1, y);
        if (label != -1)  equiv_table.AddEquivalence(label, label2);
        label = label2;
      }
      if (same_as_ul)
      {
        int label2 = (*labels)(x-1, y-1);
        if (label != -1)  equiv_table.AddEquivalence(label, label2);
        label = label2;
      }
      if (same_as_ur)
      {
        int label2 = (*labels)(x+1, y-1);
        if (label != -1)  equiv_table.AddEquivalence(label, label2);
        label = label2;
      } 
      if (label == -1)
	  {
		  label = new_label_number++;
		  equiv_table.EnsureAllocated(label);
	  }
      // set the label of the current pixel
      (*labels)(x, y) = label;
      if (props)  iUpdateProperties(props, label, x, y, pix);
    }
  }

  // clean up equivalence and regions:
  //   1. traverse links in equivalence table
  //   2. merge properties of regions with the same label
  //   3. ensure that the labels are in sequential order:  0, 1, 2, ...
  equiv_table.TraverseLinks();
  if (props)  iMergeProperties(props, equiv_table);
  if (props)  iRemoveGaps(props);
  max_label_number = equiv_table.RemoveGaps();

  // for each pixel in the image, ...
  ImgInt::Iterator p;
  for (p = labels->Begin() ;  p != labels->End() ; p++)  
  {
    // set the pixel's label to its equivalent label
    *p = equiv_table.GetEquivalentLabel( *p );
  }

  // compute adjaceny of regions
  if (props)  iComputeAdjacency8(*labels, props);

  return max_label_number;
}

};
// ================< end local functions

namespace blepo {

/**
  Connected components.
  @author Stan Birchfield (STB)
*/

int ConnectedComponents4(const ImgBgr& img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgBgr::Pixel> >* props)
{
  return iConnectedComponents4(img, labels, props);
}

int ConnectedComponents4(const ImgBinary& img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgBinary::Pixel> >* props)
{
  return iConnectedComponents4(img, labels, props);
}

int ConnectedComponents4(const ImgGray& img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgGray::Pixel> >* props)
{
  return iConnectedComponents4(img, labels, props);
}

int ConnectedComponents4(const ImgInt& img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgInt::Pixel> >* props)
{
  return iConnectedComponents4(img, labels, props);
}

int ConnectedComponents8(const ImgBgr& img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgBgr::Pixel> >* props)
{
  return iConnectedComponents8(img, labels, props);
}

int ConnectedComponents8(const ImgBinary& img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgBinary::Pixel> >* props)
{
  return iConnectedComponents8(img, labels, props);
}

int ConnectedComponents8(const ImgGray& img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgGray::Pixel> >* props)
{
  return iConnectedComponents8(img, labels, props);
}

int ConnectedComponents8(const ImgInt& img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgInt::Pixel> >* props)
{
  return iConnectedComponents8(img, labels, props);
}

};  // end namespace blepo

