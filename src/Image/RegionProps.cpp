/* 
 * Copyright (c) 2005,2007 Clemson University.
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
#include "ImageAlgorithms.h"
#include <math.h>  // sqrt

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

};
// ================< end local functions

namespace blepo
{

void RegionProps(const ImgBinary& img, RegionProperties* props)
{
  double& m00 = props->m00;
  int x, y;

  // compute non-central moments and bounding rect
  props->bounding_rect = Rect( img.Width(), img.Height(), -1, -1 );
  props->m00 = 0;
  props->m10 = 0;
  props->m01 = 0;
  props->m11 = 0;
  props->m20 = 0;
  props->m02 = 0;
  ImgBinary::ConstIterator p = img.Begin();
  for (y = 0 ; y < img.Height() ; y++)
  {
    for (x = 0 ; x < img.Width() ; x++)
    {
      int val = *p++;
      if (val)
      {
        props->m00 += 1;
        props->m10 += x;
        props->m01 += y;
        props->m11 += x * y;
        props->m20 += x * x;
        props->m02 += y * y;
        if (x >= props->bounding_rect.right )   props->bounding_rect.right = x+1;
        if (x <  props->bounding_rect.left  )   props->bounding_rect.left  = x;
        if (y >= props->bounding_rect.bottom)   props->bounding_rect.bottom = y+1;
        if (y <  props->bounding_rect.top   )   props->bounding_rect.top  = y;
      }
    }
  }

  // area
  props->area = props->m00;

  // centroid
  props->xc = (props->m10) / (props->m00);
  props->yc = (props->m01) / (props->m00);

  // central moments
  props->mu00 = props->m00;
  props->mu10 = 0;
  props->mu01 = 0;
  props->mu11 = props->m11 - props->yc * props->m10;
  props->mu20 = props->m20 - props->xc * props->m10;
  props->mu02 = props->m02 - props->yc * props->m01;

  // eccentricity, or elongatedness
  double lambda1, lambda2;
  {
    double a = props->mu20 + props->mu02;
    double b = props->mu20 - props->mu02;
    double c = sqrt( 4 * props->mu11 * props->mu11 + b * b );
    lambda1 = (a+c) / (2 * props->mu00);
    lambda2 = (a-c) / (2 * props->mu00);
    props->eccentricity = sqrt( (lambda1 - lambda2) / lambda1 );
  }

  // direction
  props->direction = 0.5 * (atan2( 2 * props->mu11, ( props->mu20 - props->mu02 ) ) );

//  { // test
//    double costheta = cos( 2 * props->direction );
//    double sintheta = sin( 2 * props->direction );
//    double a = 2 * props->mu11;
//    double b = props->mu20 - props->mu02;
//    double c = a / sqrt( a*a + b*b );
//    double d = b / sqrt( a*a + b*b );
//  }

  // major and minor axes
  props->major_axis_x = sqrt( lambda1 ) * cos( props->direction );
  props->major_axis_y = sqrt( lambda1 ) * sin( props->direction );
  props->minor_axis_x = sqrt( lambda2 ) * sin( props->direction );
  props->minor_axis_y = sqrt( lambda2 ) * (-cos( props->direction ));
}

};  // end namespace blepo

