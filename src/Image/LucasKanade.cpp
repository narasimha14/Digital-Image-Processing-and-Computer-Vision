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

// This file contains really simple code for Lucas-Kanade feature detection
// and tracking for teaching purposes.  For a research-grade implementation,
// see KLT or FastFeatureTracker (based upon OpenCV implementation).
// Never finished.

//#pragma warning(disable: 4786)
#include "Image.h"
#include "ImageAlgorithms.h"
#include "ImageOperations.h"
//#include "Quick/Quick.h"
#include "Utilities/Math.h"  // Clamp, Min, Max, sqrt
#include "Figure/Figure.h"  // debugging
#include <vector>
#include <algorithm> // std::sort()

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


bool iCompareFp(const FeaturePoint& feat1, const FeaturePoint& feat2)
{
  return feat1.val > feat2.val;
}

void iComputeGradients(const ImgFloat& img,
                       ImgFloat* gradx, ImgFloat* grady, 
                       ImgFloat* gradxx, ImgFloat* gradxy, ImgFloat* gradyy)
{
  float sigma = 1.0f;

  // compute gradient images
  Gradient(img, sigma, gradx, grady);
  Multiply(*gradx, *gradx, gradxx);
  Multiply(*grady, *grady, gradyy);
  Multiply(*gradx, *grady, gradxy);
}

// 'ww':  halfwidth
void iSumGradients(int ww, ImgFloat* gradxx, ImgFloat* gradxy, ImgFloat* gradyy)
{
  ImgFloat tmp;

  // sum over window
  int w = 2*ww+1;
  ImgFloat kernel_horiz(w,1), kernel_vert(1,w);
  Set(&kernel_horiz, 1);
  Set(&kernel_vert, 1);
  Convolve(*gradxx, kernel_horiz, &tmp);
  Convolve(tmp, kernel_vert, gradxx);
  Convolve(*gradyy, kernel_horiz, &tmp);
  Convolve(tmp, kernel_vert, gradyy);
  Convolve(*gradxy, kernel_horiz, &tmp);
  Convolve(tmp, kernel_vert, gradxy);
}

void iComputeE(const ImgFloat& simg1, const ImgFloat& simg2, 
               const ImgFloat& sgradx, const ImgFloat& sgrady, 
               float* ex, float* ey)
{
  assert(IsSameSize(simg1, simg2));
  assert(IsSameSize(simg1, sgradx));
  assert(IsSameSize(simg1, sgrady));

  *ex = 0;
  *ey = 0;
  const float* p1 = simg1.Begin();
  const float* p2 = simg2.Begin();
  const float* px = sgradx.Begin();
  const float* py = sgrady.Begin();
  while (p1 != simg1.End())
  {
    float diff = *p2++ - *p1++;
    *ex += diff * (*px++);
    *ey += diff * (*py++);
  }
}

// like 'Extract', but uses bilinear interpolation
void iExtract(const ImgFloat& img, float cx, float cy, int ww, ImgFloat* simg2)
{
  int w = 2*ww+1;
  simg2->Reset(w, w);
  for (int y = 0 ; y < w ; y++)
  {
    for (int x = 0 ; x < w ; x++)
    {
      (*simg2)(x, y) = Interp(img, x+cx-ww, y+cy-ww);
    }
  }
}


};
// ================< end local functions

namespace blepo
{

//void IntegralImage(const ImgFloat& img, ImgDouble* out)
//{
//  // slow implementation; should rewrite this with pointers
//  *out = img;
//  for (int y=1 ; y<img.Height() ; y++)
//  {
//    for (int x=1 ; x<img.Width() ; x++)
//    {
//      (*out)(x,y) = img(x,y) - (*out)(x-1,y-1) + (*out)(x-1,y) + (*out)(x,y-1);
//    }
//  }
//}

/**

  @author Stan Birchfield (STB)
*/

void DetectFeatures(const ImgGray& img, std::vector<FeaturePoint>* features)
{
  features->clear();
  static Figure fig, fig2, fig3, fig4;

  int ww = 2;  // halfwidth
  int min_dist = 10;
  float min_eigenvalue = 100.0f;
  int nfeatures = 100;

  ImgFloat imgf, gradx, grady, gradxx, gradyy, gradxy, mineig_map, tmp;
  float mineig;

  Convert(img, &imgf);
  iComputeGradients(imgf, &gradx, &grady, &gradxx, &gradxy, &gradyy);
  iSumGradients(ww, &gradxx, &gradxy, &gradyy);

//  // compute gradients in all directions
//  Convert(img, &imgf);
//  Gradient(imgf, 2.0f, 5, &gradx, &grady);
//  Multiply(gradx, gradx, &gradxx);
//  Multiply(grady, grady, &gradyy);
//  Multiply(gradx, grady, &gradxy);
//
//  // sum over 5x5 window
//  ImgFloat kernel_horiz(5,1), kernel_vert(1,5);
//  Set(&kernel_horiz, 1);
//  Set(&kernel_vert, 1);
//  Convolve(gradxx, kernel_horiz, &tmp);
//  Convolve(tmp, kernel_vert, &gradxx);
//  Convolve(gradyy, kernel_horiz, &tmp);
//  Convolve(tmp, kernel_vert, &gradyy);
//  Convolve(gradxy, kernel_horiz, &tmp);
//  Convolve(tmp, kernel_vert, &gradxy);

//
//
//  ImgDouble igradxx, igradyy, igradxy;
//  IntegralImage(gradxx, &igradxx);
//  IntegralImage(gradyy, &igradyy);
//  IntegralImage(gradxy, &igradxy);

  std::vector<FeaturePoint> points;
  mineig_map.Reset(img.Width(), img.Height());
  Set(&mineig_map, 0);
  for (int y=ww ; y<img.Height()-ww ; y++)
  {
    for (int x=ww ; x<img.Width()-ww ; x++)
    {
      float gxx = gradxx(x, y);
      float gyy = gradyy(x, y);
      float gxy = gradxy(x, y);
      mineig = (float) ((gxx + gyy - sqrt((gxx - gyy)*(gxx - gyy) + 4*gxy*gxy))/2.0f);
      mineig_map(x, y) = mineig;
      points.push_back(FeaturePoint((float)x, (float)y, mineig));
    }
  }

  // sort descending
  std::sort( points.begin(), points.end(), iCompareFp );

  // fill 'features' with the 'n' best features
  ImgGray filled(img.Width(), img.Height());
  Set(&filled, 0);
  for (int i=0 ; i<(int)points.size() ; i++)
  {
    const FeaturePoint& f = points[i];
    if (f.val < min_eigenvalue)  break;
    if (features->size() == nfeatures)  break;
		if (!filled(blepo_ex::Round( f.x ), blepo_ex::Round( f.y )))  
    {
      features->push_back(f);
      int left   = blepo_ex::Max((int) f.x - min_dist, 0);
      int top    = blepo_ex::Max((int) f.y - min_dist, 0);
      int right  = blepo_ex::Min((int) f.x + min_dist, img.Width());
      int bottom = blepo_ex::Min((int) f.y + min_dist, img.Height());
      Set(&filled, Rect(left, top, right, bottom), 1);
    }
  }

//  fig.Draw(img);
//  ImgBgr bimg;
//  Convert(img, &bimg);
//  for (i = 0 ; i<features->size() ; i++)
//  {
////    fig.GrabMouseClick();
//    FeaturePoint& f = (*features)[i];
//    DrawDot(Point(f.x, f.y), &bimg, Bgr(0,0,255));
//  }
//    fig.Draw(bimg);
//  fig2.Draw(gradx);
//  fig3.Draw(gradxx);
//  fig4.Draw(mineig_map);
}

//void iExtract(const ImgGray& img, const Rect& r, &simg1);


//  // compute elements of 'e' vector
//  for (int y = f.y-ww ; y <= f.y+ww ; y++)
//  {
//    for (int x = f.x-ww ; x <= f.x+ww ; x++)
//    {
//
//
//    }
//  }

void TrackFeatures(std::vector<FeaturePoint>* features, const ImgGray& img1, const ImgGray& img2)
{
  int maxiter = 10;
  float minshift = 0.1f;
  int ww = 2;  // window halfwidth

  ImgFloat img1f, img2f, gradx, grady, gradxx, gradyy, gradxy;
  ImgFloat sgradx, sgrady, simg1, simg2;  // small images

  // convert to float
  Convert(img1, &img1f);
  Convert(img2, &img2f);

  // compute gradients
  iComputeGradients(img1f, &gradx, &grady, &gradxx, &gradxy, &gradyy);
  iSumGradients(ww, &gradxx, &gradxy, &gradyy);

  for (int i=0 ; i<(int)features->size() ; i++)
  {
    FeaturePoint& f = (*features)[i];

    float dx_total = 0, dy_total = 0;

    // compute elements of Z matrix
		float gxx = gradxx(blepo_ex::Round( f.x ), blepo_ex::Round( f.y) );
    float gyy = gradyy(blepo_ex::Round( f.x ), blepo_ex::Round( f.y) );
    float gxy = gradxy(blepo_ex::Round( f.x ), blepo_ex::Round( f.y) );

    // compute windows in two images
    Rect r(blepo_ex::Round( f.x-ww ), blepo_ex::Round( f.y-ww ), blepo_ex::Round( f.x+ww+1 ), blepo_ex::Round( f.y+ww+1 ));
    Extract(img1f, r, &simg1);
    Extract(img2f, r, &simg2);
    Extract(gradx, r, &sgradx);
    Extract(grady, r, &sgrady);

    // compute elements of 'e' vector
    float ex, ey;
    iComputeE(simg1, simg2, sgradx, sgrady, &ex, &ey);

    i = 0;
    bool done = false;
    while (!done)
    {
      // solve Zd = e
      float det = gxx * gyy - gxy * gxy;  // determinant of Z matrix
      float dx = (gyy * ex - gxy * ey) / det;
      float dy = (gxx * ey - gxy * ex) / det;
      dx_total += dx;
      dy_total += dy;

      // shift image and recompute 'e'
      iExtract(img2f, f.x+dx_total, f.y+dy_total, ww, &simg2);
      iComputeE(simg1, simg2, sgradx, sgrady, &ex, &ey);
      
      // decide whether we are done
      done = (i > maxiter) || ((fabs(dx) < minshift) && (fabs(dy) < minshift));
    }

    f.x += dx_total;
    f.y += dy_total;
  }


}

};  // end namespace blepo




