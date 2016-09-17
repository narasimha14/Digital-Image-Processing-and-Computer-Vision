/* 
 * Copyright (c) 2006 Clemson University.
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

#include "Image.h"
#include "ImageOperations.h"  // Set, ...
#include "ImageAlgorithms.h"  // Floodfill
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

// Returns the label of a neighbor of 'pt' with nonnegative label.
// Returns -1 if there is no such neighbor.
int iGetLabelNeighbor(const Point& pt, const ImgInt& labels)
{
  int w = labels.Width()-1;
  int h = labels.Height()-1;
  int tmp;
  if (pt.x > 0) { tmp = labels(pt.x-1, pt.y);  if (tmp >= 0)  return tmp; }
  if (pt.y > 0) { tmp = labels(pt.x, pt.y-1);  if (tmp >= 0)  return tmp; }
  if (pt.x < w) { tmp = labels(pt.x+1, pt.y);  if (tmp >= 0)  return tmp; }
  if (pt.y < h) { tmp = labels(pt.x, pt.y+1);  if (tmp >= 0)  return tmp; }
  return -1;
}

void iExpand(int x, int y, unsigned char g, int label, 
            const ImgGray& img, ImgInt* labels,
            Array<Point>* frontier)
{
  // <= is necessary for marker-based and does not harm non-marker-based
  if ( img(x, y) <= g && (*labels)(x, y) < 0)
  {
    (*labels)(x, y) = label;
    frontier->Push( Point(x, y) );
  }
}

};
// ================< end local functions

namespace blepo {

/** 
Simplified implementation of Vincent-Soilles watershed algorithm,
with 8-neighbor connectedness.
You will probably want to call this function with a quantized version 
of the gradient magnitude of the original image.

@author Stan Birchfield
*/
void WatershedSegmentation(const ImgGray& img, ImgInt* labels, bool marker_based)
{
  int next_label = 0;
  labels->Reset( img.Width(), img.Height() );
  ImgInt iimg;
  Convert(img, &iimg);
  Set(labels, -1);
  const int ngray = 256;

  // Precompute array of pixels for each graylevel
  // (pixels[g] contains the coordinates of all the pixels with graylevel g)
  Array<Point> pixels[ngray];
  {
    const unsigned char* p = img.Begin();
    for (int y=0 ; y<img.Height() ; y++)
      for (int x=0 ; x<img.Width() ; x++)
        pixels[ *p++ ].Push( Point(x, y) );
  }

  // For each graylevel, ...
  for (int g=0 ; g<ngray ; g++)
  {

    // Create frontier of all the pixels with graylevel g that border a 
    // pixel in an existing catchment basin
    Array<Point> frontier, frontier2;
    int i;
    for (i=0 ; i<pixels[g].Len() ; i++)
    {
      const Point& pt = pixels[g][i];
      int lab = iGetLabelNeighbor(pt, *labels);
      if (lab >= 0)
      {
        (*labels)(pt.x, pt.y) = lab;
        frontier.Push( pt );
      }
    }

    // Expand all the pixels on the frontier, moving a distance of one pixel
    // each time to correctly handle the situation when a region is adjacent
    // to more than one catchment basin.
    do
    {
      while (frontier.Len() > 0)
      {
        Point pt = frontier.Pop();
        int lab = (*labels)(pt.x, pt.y);
        Point pt2;
        const int w = img.Width()-1;
        const int h = img.Height()-1;
        if (pt.x > 0) iExpand(pt.x-1, pt.y, g, lab, img, labels, &frontier2);
        if (pt.x < w) iExpand(pt.x+1, pt.y, g, lab, img, labels, &frontier2);
        if (pt.y > 0) iExpand(pt.x, pt.y-1, g, lab, img, labels, &frontier2);
        if (pt.y < h) iExpand(pt.x, pt.y+1, g, lab, img, labels, &frontier2);

        if (pt.x > 0 && pt.y > 0) iExpand(pt.x-1, pt.y-1, g, lab, img, labels, &frontier2);
        if (pt.x < w && pt.y < h) iExpand(pt.x+1, pt.y+1, g, lab, img, labels, &frontier2);
        if (pt.x > 0 && pt.y < h) iExpand(pt.x-1, pt.y+1, g, lab, img, labels, &frontier2);
        if (pt.x < w && pt.y > 0) iExpand(pt.x+1, pt.y-1, g, lab, img, labels, &frontier2);
      }
      frontier = frontier2;
      frontier2.Reset();
    } while (frontier.Len() > 0);

    // For each connected region with graylevel g that does not yet belong to a 
    // catchment basin, declare a new catchment basin
    if (!marker_based || g==0)
    {
      for (i=0 ; i<pixels[g].Len() ; i++)
      {
        const Point& pt = pixels[g][i];
        if ((*labels)(pt.x, pt.y) < 0)  
        {
          FloodFill8(iimg, pt.x, pt.y, next_label++, labels);
        }
      }
    }
  }
}

};  // end namespace blepo
