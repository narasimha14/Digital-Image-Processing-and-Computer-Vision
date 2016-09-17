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

#include "ImageAlgorithms.h"
#include "ImageOperations.h"
#include "Figure/Figure.h"
#include "Utilities/Math.h"
#include <math.h>  // sqrt, atan2
#include <algorithm> // std::sort()
#include <functional> // std::greater

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

// operates in place:
//   'x_mag'  :  gradx (input) and magnitude (output)
//   'y_phase':  grady (input) and phase (output)
void ComputeMagnitudeAndPhase(ImgFloat* x_mag, ImgFloat* y_phase)
{
  assert(IsSameSize(*x_mag, *y_phase));

  ImgFloat::Iterator px = x_mag->Begin();
  ImgFloat::Iterator py = y_phase->Begin();

  while (px != x_mag->End())
  {
    float mag = sqrt(*px * *px + *py * *py);
    float phase = atan2(*py, *px);
    *px++ = mag;
    *py++ = phase;
  }
}

// returns the offset pixel in the direction of the gradient (module 180 degrees)
inline void GetDirection(float phase, int* dx, int* dy)
{
  if (phase < 0)  phase += (float) blepo_ex::Pi;  // ignore sign of gradient

  double pi8 = blepo_ex::Pi / 8;
  
  if      (phase <     pi8)  { *dx = 1;  *dy =  0; }
  else if (phase < 3 * pi8)  { *dx = 1;  *dy =  1; }
  else if (phase < 5 * pi8)  { *dx = 0;  *dy =  1; }
  else if (phase < 7 * pi8)  { *dx = 1;  *dy = -1; }
  else                       { *dx = 1;  *dy =  0; }
}

void NonMaximumSuppression(const ImgFloat& mag, const ImgFloat& phase, ImgFloat* out)
{
  assert(IsSameSize(mag, phase));
  out->Reset(mag.Width(), mag.Height());
  Set(out, 0);  // only for borders

  int dx, dy;
  for (int y = 1 ; y < mag.Height()-1 ; y++)
  {
    for (int x = 1 ; x < mag.Width()-1 ; x++)
    {
      GetDirection( phase(x, y), &dx, &dy );
      float val0 = mag(x, y);
      float val1 = mag(x+dx, y+dy);
      float val2 = mag(x-dx, y-dy);
      (*out)(x, y) = (val0 >= val1 && val0 >= val2) ? val0 : 0;
    }
  }
}

// returns true if there are any (non-zero) edge pixels in 'edges'
bool DetermineThresholds(const ImgFloat& edges, float perc, float ratio, 
                         float* th_low, float* th_high)
{
  assert(perc > 0.0f && perc < 1.0f);
  assert(ratio > 1.0f);

  // store all gradient values in a vector
  std::vector<float> vals;
  const float* p = edges.Begin();
  for ( ; p != edges.End() ; p++)
  {
    if (*p != 0)  vals.push_back(*p);
  }

  if (vals.size() > 0)
  {
    // sort ascending
    std::sort( vals.begin(), vals.end() );

    // compute thresholds as a function of 'perc' and 'ratio'
    int npix = vals.size();
    int index = blepo_ex::Clamp( blepo_ex::Round( perc * npix ), 0, npix-1 );
    *th_high = vals[index];
    *th_low = *th_high / ratio;
    return true;
  }
  else
  { // there are no edge pixels
    *th_low = *th_high = 1;
    return false;
  }
}

inline void Expand(const ImgFloat& edges, float th, std::vector<Point>* frontier, const Point& p, ImgBinary* out)
{
  ImgBinary::PixelRef pix = (*out)(p.x, p.y);
  if ( edges(p.x, p.y) >= th && !pix)
  {
    frontier->push_back(p);
    pix = 1;
  }
}

void DoubleThreshold(const ImgFloat& img, float th_low, float th_high, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  Set(out, 0);  // only for borders

  std::vector<Point> frontier;
  int w = img.Width()-1;
  int h = img.Height()-1;
  Point p;

  for (int y = 0 ; y < img.Height() ; y++)
  {
    for (int x = 0 ; x < img.Width() ; x++)
    {
      if ( img(x, y) >= th_high )  
      {
        frontier.push_back(Point(x, y));
        (*out)(x, y) = 1;
      }
    }
  }

  while (frontier.size() != 0)
  {
    p = frontier.back();
    frontier.pop_back();

    if (p.x > 0)  Expand(img, th_low, &frontier, Point(p.x-1, p.y  ), out);
    if (p.x < w)  Expand(img, th_low, &frontier, Point(p.x+1, p.y  ), out);
    if (p.y > 0)  Expand(img, th_low, &frontier, Point(p.x,   p.y-1), out);
    if (p.y < h)  Expand(img, th_low, &frontier, Point(p.x,   p.y+1), out);
    
    if (p.x > 0 && p.y > 0)  Expand(img, th_low, &frontier, Point(p.x-1, p.y-1), out);
    if (p.x > 0 && p.y < h)  Expand(img, th_low, &frontier, Point(p.x-1, p.y+1), out);
    if (p.x < w && p.y > 0)  Expand(img, th_low, &frontier, Point(p.x+1, p.y-1), out);
    if (p.x < w && p.y < h)  Expand(img, th_low, &frontier, Point(p.x+1, p.y+1), out);
  }
}

};
// ================< end local functions

namespace blepo
{

void Canny(const ImgGray& img, ImgBinary* out, float sigma, float perc, float ratio)
{
  ImgFloat fimg, gradx, grady, edges;
  ImgFloat &mag = gradx, &phase = grady;
  float th_low, th_high;
//  Figure fig1("gradx"), fig2("grady"), fig3("mag"), fig4("phase"), fig5("nonmax");

  // compute gradient
  Convert(img, &fimg);
  if (sigma == -1)
  {
    GradientSobel(fimg, &gradx, &grady);
  } else if (sigma == -2)
  {
    ImgFloat tmp_smoothed, &tmp = edges;
    FastSmoothAndGradientApprox(fimg, &tmp_smoothed, &gradx, &grady, &tmp);
  }
  else
  {
    assert( sigma > 0 );
    Gradient(fimg, sigma, &gradx, &grady);
  }
//  fig1.Draw(gradx);
//  fig2.Draw(grady);
  ComputeMagnitudeAndPhase(&mag, &phase);

//  fig3.Draw(mag);
//  fig4.Draw(phase);

  // non-maximum suppression
  NonMaximumSuppression(mag, phase, &edges);

//  fig5.Draw(edges);

  // threshold
  bool any_edges = DetermineThresholds(edges, perc, ratio, &th_low, &th_high);
  if (any_edges)
  {
    DoubleThreshold(edges, th_low, th_high, out);
  }
}


};  // end namespace blepo

