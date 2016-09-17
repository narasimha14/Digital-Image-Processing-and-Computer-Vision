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

//#pragma warning(disable: 4786)
#include "Image.h"
#include "ImageOperations.h"
#include "Quick/Quick.h"
#include "Utilities/Math.h"  // Clamp
#include "Figure/Figure.h"  // debugging

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


};
// ================< end local functions

namespace blepo
{

/**

  @author Stan Birchfield (STB)
*/

void RealTimeStereo(const ImgGray& img_left, const ImgGray& img_right, ImgGray* disparity_map, int max_disp, int winsize)
{
//  {
//    //    Testing ConvolveBox5x5
//    ImgGray img(7, 7);
//    unsigned char* p;
//    int i;
//    for (p = img.Begin(), i = 0 ; p != img.End() ; )  *p++ = i++;
//    ConvolveBox5x5(img, &img);
////    Verify(img(2,2)==25);
//  }

//  static Figure fig("dispmap left"), fig2("dispmap right"), fig3("dispmap after check");
//  static Figure fig, fig3;
  if (!IsSameSize(img_left, img_right))  BLEPO_ERROR("Images must be the same size for stereo correspondence");
  int i;

  disparity_map->Reset(img_left.Width(), img_left.Height());

  // compute difference images (one for each disparity)
  std::vector<ImgGray> abs_diff_left(max_disp + 1);
  for (i = 0 ; i <= max_disp ; i++)
  {
    const unsigned char* p1 = img_left.Begin(i);
    const unsigned char* p2 = img_right.Begin();
    abs_diff_left[i].Reset(img_left.Width(), img_left.Height());
    unsigned char* po = abs_diff_left[i].Begin(i);
    const int nbytes = img_left.NBytes() - i;
//      int n_quadwords = nbytes / 8;
//      mmx_absdiff(p1, p2, po, n_quadwords);
    int n_doublequadwords = nbytes / 16;
    xmm_absdiff(p1, p2, po, n_doublequadwords);

//    fig2.Draw(abs_diff_left[i]);
	if(winsize==5)
		ConvolveBox5x5(abs_diff_left[i], &abs_diff_left[i]);
	else ConvolveBoxNxN(abs_diff_left[i], &abs_diff_left[i], winsize);
//    fig.Draw(abs_diff_left[i]);
//    fig.GrabMouseClick();
  }

  // grab best disparity
  {
    std::vector<unsigned char*> p(max_disp + 1);
    for (i = 0 ; i <= max_disp ; i++)  p[i] = abs_diff_left[i].Begin();
    unsigned char* po = disparity_map->Begin();
    unsigned char* pe = abs_diff_left[0].End();
    int disp, minval;
    while (p[0] != pe)
    {
      minval = 9999;
      for (i = 0 ; i <= max_disp ; i++)  
      {
        int val = *(p[i])++;
        if (val < minval)
        {
          minval = val;
          disp = i;
        }
      }
      *po++ = disp;
    }
  }

  ImgInt foo;
  Convert(*disparity_map, &foo);
//  fig.Draw(foo); //*disparity_map);

  // grab best disparity for right image
  ImgGray right_disparity_map;
  right_disparity_map.Reset(img_left.Width(), img_left.Height());
  {
    std::vector<unsigned char*> p(max_disp + 1);
    for (i = 0 ; i <= max_disp ; i++)  p[i] = abs_diff_left[i].Begin(i);
    unsigned char* po = right_disparity_map.Begin();
    unsigned char* pe = abs_diff_left[0].End() - max_disp;
    int disp, minval;
    while (p[0] != pe)
    {
      minval = 9999;
      for (i = 0 ; i <= max_disp ; i++)  
      {
        int val = *(p[i])++;
        if (val < minval)
        {
          minval = val;
          disp = i;
        }
      }
      *po++ = disp;
    }
    while (po != right_disparity_map.End())  *po++ = 0;
  }  

  Convert(right_disparity_map, &foo);
//  fig2.Draw(foo); //*disparity_map);

  // left-right consistency check
  {
    unsigned char* pl = disparity_map->Begin();
    unsigned char* pr = right_disparity_map.Begin();
    while (pl != disparity_map->End())
    {
      unsigned char *p = pr - *pl;
      if (p < right_disparity_map.Begin() || *p != *pl)  *pl = 0;
      pl++;
      pr++;
    }
  }

//  {
//    ImgGray display = *disparity_map;
//    for (unsigned char* p = display.Begin() ; p != display.End() ; p++)  *p <<= 4;
////    fig3.Draw(display);
//    *disparity_map = display;
//  }
  Convert(*disparity_map, &foo);
//  fig3.Draw(foo); //*disparity_map);

}




//void RealTimeStereo(const ImgGray& img_left, const ImgGray& img_right, ImgGray* disparity_map, int max_disp)
//{
////  {
////    //    Testing ConvolveBox5x5
////    ImgGray img(7, 7);
////    unsigned char* p;
////    int i;
////    for (p = img.Begin(), i = 0 ; p != img.End() ; )  *p++ = i++;
//    ConvolveBox5x5(img, &img);
//////    Verify(img(2,2)==25);
////  }
//
//  static Figure fig, fig2, fig3;
//  if (!IsSameSize(img_left, img_right))  BLEPO_ERROR("Images must be the same size for stereo correspondence");
//  int i;
//
//  disparity_map->Reset(img_left.Width(), img_left.Height());
//
//  // compute difference images (one for each disparity)
//  std::vector<ImgGray> abs_diff_left(max_disp + 1);
//  for (i = 0 ; i <= max_disp ; i++)
//  {
//    const unsigned char* p1 = img_left.Begin(i);
//    const unsigned char* p2 = img_right.Begin();
//    abs_diff_left[i].Reset(img_left.Width(), img_left.Height());
//    unsigned char* po = abs_diff_left[i].Begin(i);
//    const int nbytes = img_left.NBytes() - i;
//    int n_quadwords = nbytes / 8;
//    mmx_diff(p1, p2, po, n_quadwords);
//
////    fig2.Draw(abs_diff_left[i]);
////    ConvolveBox5x5(abs_diff_left[i], &abs_diff_left[i]);
////    fig.Draw(abs_diff_left[i]);
////    fig.GrabMouseClick();
//  }
//
//  // grab best disparity
//  {
//    std::vector<unsigned char*> p(max_disp + 1);
//    for (i = 0 ; i <= max_disp ; i++)  p[i] = abs_diff_left[i].Begin();
//    unsigned char* po = disparity_map->Begin();
//    unsigned char* pe = abs_diff_left[0].End();
//    int disp, minval;
//    while (p[0] != pe)
//    {
//      minval = 9999;
//      for (i = 0 ; i <= max_disp ; i++)  
//      {
//        int val = *(p[i])++;
//        if (val < minval)
//        {
//          minval = val;
//          disp = i;
//        }
//      }
//      *po++ = disp;
//    }
//  }
//
//  ImgInt foo;
//  Convert(*disparity_map, &foo);
//  fig.Draw(foo); //*disparity_map);
//
//  // grab best disparity for right image
//  ImgGray right_disparity_map;
//  right_disparity_map.Reset(img_left.Width(), img_left.Height());
//  {
//    std::vector<unsigned char*> p(max_disp + 1);
//    for (i = 0 ; i <= max_disp ; i++)  p[i] = abs_diff_left[i].Begin(i);
//    unsigned char* po = right_disparity_map.Begin();
//    unsigned char* pe = abs_diff_left[0].End() - max_disp;
//    int disp, minval;
//    while (p[0] != pe)
//    {
//      minval = 9999;
//      for (i = 0 ; i <= max_disp ; i++)  
//      {
//        int val = *(p[i])++;
//        if (val < minval)
//        {
//          minval = val;
//          disp = i;
//        }
//      }
//      *po++ = disp;
//    }
//    while (po != right_disparity_map.End())  *po++ = 0;
//  }  
//
//  Convert(right_disparity_map, &foo);
//  fig2.Draw(foo); //*disparity_map);
//
//  // left-right consistency check
//  {
//    unsigned char* pl = disparity_map->Begin();
//    unsigned char* pr = right_disparity_map.Begin();
//    while (pl != disparity_map->End())
//    {
//      if ( *(pr + *pl) != *pl )  *pl = 0;
//      pl++;
//      pr++;
//    }
//  }
//
//  Convert(*disparity_map, &foo);
//  fig3.Draw(foo); //*disparity_map);
//
//}

};  // end namespace blepo




