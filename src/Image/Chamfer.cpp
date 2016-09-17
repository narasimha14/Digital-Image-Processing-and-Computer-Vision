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
//#include "ImageAlgorithms.h"  // Floodfill
#include "Utilities/Math.h"  // Min, Max
#include <vector>

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

//// ================> begin local functions (available only to this translation unit)
//namespace
//{
//using namespace blepo;
//
//};
//// ================< end local functions

namespace blepo {

/**
  Chamfer distance.
  @author Stan Birchfield (STB)
*/

void Chamfer(const ImgGray& img, ImgInt* chamfer_dist)
{
  chamfer_dist->Reset(img.Width(), img.Height());
  int bignum = img.Width() * img.Height() + 1;
//  Set(chamfer_dist, bignum);
  int x, y;

  // forward pass
  for (y=0 ; y<img.Height() ; y++)
  {
    for (x=0 ; x<img.Width() ; x++)
    {
      if (img(x, y))  (*chamfer_dist)(x, y) = 0;
      else
      {
        int dist = bignum;
        if (y > 0)  dist = blepo_ex::Min( dist, (*chamfer_dist)(x, y-1) + 1);
        if (x > 0)  dist = blepo_ex::Min( dist, (*chamfer_dist)(x-1, y) + 1);
        (*chamfer_dist)(x, y) = dist;
      }
    }
  }

  // backward pass
  for (y=img.Height()-1 ; y>=0 ; y--)
  {
    for (x=img.Width()-1 ; x>=0 ; x--)
    {
      if (img(x, y))  (*chamfer_dist)(x, y) = 0;
      else
      {
        int dist = (*chamfer_dist)(x, y);
        if (y < img.Height()-1)  dist = blepo_ex::Min( dist, (*chamfer_dist)(x, y+1) + 1);
        if (x < img.Width ()-1)  dist = blepo_ex::Min( dist, (*chamfer_dist)(x+1, y) + 1);
        (*chamfer_dist)(x, y) = dist;
      }
    }
  }
}

};  // end namespace blepo

