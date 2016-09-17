/* 
 * Copyright (c) 2007 Clemson University.
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

// To do:
//  Speed up by
//    * replacing sqrt in select (perhaps use determinant instead)
//    * convolve with [-0.5 0 0 0 0.5] is slow
//    * copy entire pyramid in main loop

#include "ImageAlgorithms.h"
#include "ImageOperations.h"
#include "Figure/Figure.h"
#include "Matrix/LinearAlgebra.h"
#include "Matrix/MatrixOperations.h"
#include "Utilities/Math.h"
#include "Utilities/Utilities.h"
//#include "../../blepo_internal/src/Image/ComputationalGeometry.h"
#include <math.h>  // sqrt, atan2
//#include <algorithm> // std::sort()
//#include <functional> // std::greater

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

// Computes "cornerness" for each pixel over 3x3 window.  Does not perform normalization,
// so computed value is 9 times the actual value.
void iComputeEigenMap3x3(const ImgFloat& gradx, const ImgFloat& grady, FeatureDetectMeasure fdm, ImgFloat* eigenmap)
{
  ImgFloat gxx, gyy, gxy, work;
  Multiply(gradx, gradx, &gxx);
  Multiply(grady, grady, &gyy);
  Multiply(gradx, grady, &gxy);
  SumBox3x3WithBorders(gxx, &gxx, &work);
  SumBox3x3WithBorders(gyy, &gyy, &work);
  SumBox3x3WithBorders(gxy, &gxy, &work);

  const int w = gradx.Width(), h = gradx.Height();
  eigenmap->Reset( w, h );
  float* pxx = gxx.Begin();
  float* pyy = gyy.Begin();
  float* pxy = gxy.Begin();
  float* q = eigenmap->Begin();
  float* end = gxx.End();
  while (pxx != end)
  {
    float gxx = *pxx;
    float gyy = *pyy;
    float gxy = *pxy;
    switch (fdm)
    {
//#if 0   // Shi-Tomasi's minimum eigenvalue
    case FD_MINEIG:
    *q++ = ((gxx + gyy - sqrt((gxx - gyy)*(gxx - gyy) + 4*gxy*gxy))/2.0f);
    break;
//#elif 1   // combination of minimum and maximum eigenvalue, for joint tracking
    case FD_MINMAXEIG:
      {
    float frac = 0.1f;
    float mineig = ((gxx + gyy - sqrt((gxx - gyy)*(gxx - gyy) + 4*gxy*gxy))/2.0f);
    float maxeig = ((gxx + gyy + sqrt((gxx - gyy)*(gxx - gyy) + 4*gxy*gxy))/2.0f);
    *q++ = blepo_ex::Max(mineig, frac * maxeig);
      }
    break;
//#elif 1  // Harris corner detector
    case FD_HARRIS:
      {
    float det = gxx * gxy - gxy * gxy;
    float trace = gxx + gyy;
    *q++ = det - 0.04f * trace * trace;
      }
    break;
//#else   // modification of Harris
    case FD_TRACEDET:
      {
    float det = gxx * gxy - gxy * gxy;
    float trace = gxx + gyy;
    *q++ = det / trace;  // divide by zero is okay, will set to infinity
      }
    break;
//#endif
    default:
      assert(0);
    }
    pxx++;  pyy++;  pxy++;
  }
}

// if 'nfeatures' is negative, then finds all the features
void iFindFeatures(const ImgFloat& eigenmap, int nfeatures, std::vector<jFeature>* features)
{
  features->clear();
  const int w = eigenmap.Width(), h = eigenmap.Height();
  const float* r = eigenmap.Begin(1, 1);
  int x, y;
  for (y=1 ; y<h-1 ; y++)
  {
    for (x=1 ; x<w-1 ; x++)
    {
      float val = r[0];
      if ( val >= r[-1]
        && val >= r[1]
        && val >= r[-w-1]
        && val >= r[-w]
        && val >= r[-w+1]
        && val >= r[w-1]
        && val >= r[w]
        && val >= r[w+1])
      {
        features->push_back( jFeature( x, y, val) );
      }
      r++;
    }
    r += 2;
  }
  std::sort(features->begin(), features->end(), std::greater<jFeature>() );
  if (nfeatures >= 0)
  {
    if ((int) features->size() >= nfeatures)
    {
      features->erase(features->begin()+nfeatures, features->end());
    }
    else if ((int) features->size() < nfeatures)
    {
      features->resize(nfeatures);  // set value of additional slots to -1
    }
  }
}

inline void iCompute2x2GradientMatrix
(
  const ImgFloat& gradx_window, 
  const ImgFloat& grady_window, 
  float* gxx, 
  float* gxy, 
  float* gyy
)
{
  assert( IsSameSize( gradx_window, grady_window ) );
  *gxx = *gxy = *gyy = 0;
  const float* px = gradx_window.Begin();
  const float* py = grady_window.Begin();
  while (px != gradx_window.End())
  {
    *gxx += (*px) * (*px);
    *gxy += (*px) * (*py);
    *gyy += (*py) * (*py);
    px++;  py++;
  }
}

inline void iComputeSumsForLkhs
(
  const ImgFloat& img1_window, 
  const ImgFloat& img2_window, 
  const ImgFloat& gradx_window, 
  const ImgFloat& grady_window, 
  float* gxsum, 
  float* gysum, 
  float* itsum
)
{
  assert( IsSameSize( gradx_window, grady_window ) );
  assert( IsSameSize( img1_window, img2_window ) );
  assert( IsSameSize( img1_window, grady_window ) );
  *gxsum = *gysum = *itsum = 0;
  const float* px = gradx_window.Begin();
  const float* py = grady_window.Begin();
  const float* q1 = img1_window.Begin();
  const float* q2 = img2_window.Begin();
  while (px != gradx_window.End())
  {
    *gxsum += (*px);
    *gysum += (*py);
    *itsum += (*q1) - (*q2);
    px++;  py++;  q1++;  q2++;
  }
}

inline void iCompute2x1ErrorVector
(
  const ImgFloat& img1_window, 
  const ImgFloat& img2_window, 
  const ImgFloat& gradx_window, 
  const ImgFloat& grady_window, 
  float* ex, 
  float* ey
)
{
  assert( IsSameSize( gradx_window, grady_window ) );
  assert( IsSameSize( img1_window, img2_window ) );
  assert( IsSameSize( img1_window, grady_window ) );
  *ex = *ey = 0;
  const float* px = gradx_window.Begin();
  const float* py = grady_window.Begin();
  const float* q1 = img1_window.Begin();
  const float* q2 = img2_window.Begin();
  while (px != gradx_window.End())
  {
    *ex += (*px) * (*q1 - *q2);
    *ey += (*py) * (*q1 - *q2);
    px++;  py++;  q1++;  q2++;
  }
}

inline float iCompute2x2Determinant(float gxx, float gxy, float gyy)
{
  return gxx * gyy - gxy * gxy;
}

inline void iSolve2x2EquationSymm
(
  float gxx, float gxy, float gyy,
  float  ex, float  ey,
  float invdet,
  float *dx, float *dy
)
{
  *dx = (gyy * ex - gxy * ey) * invdet;
  *dy = (gxx * ey - gxy * ex) * invdet;
}

// Return SSD between two windows
inline float iComputeResidue
(
  const ImgFloat& img1_window, 
  const ImgFloat& img2_window
)
{
  assert( IsSameSize( img1_window, img2_window ) );
  const float* q1 = img1_window.Begin();
  const float* q2 = img2_window.Begin();
  float sum = 0;
  while (q1 != img1_window.End())
  {
    float diff = (*q1 - *q2);
    sum += diff * diff;
    q1++;  q2++;
  }
  return sum;
}

inline void iCompute6x1And6x6GradientMatrix
(
  const ImgFloat& gradx_window, 
  const ImgFloat& grady_window, 
  MatDbl* g6x6_aff,
  MatDbl* g6x1_aff
)
{
  assert( IsSameSize( gradx_window, grady_window ) );
  assert( gradx_window.Width() % 2 == 1 );
  assert( gradx_window.Height() % 2 == 1 );
  const int hw = gradx_window.Width() / 2;
  const int hh = gradx_window.Height() / 2;
  g6x6_aff->Reset(6, 6);
  g6x1_aff->Reset(6);
  Set(g6x6_aff, 0);
  Set(g6x1_aff, 0);
  MatDbl v(6), w, u;
  const float* px = gradx_window.Begin();
  const float* py = grady_window.Begin();
  for (int y=-hh ; y<=hh ; y++)
  {
    for (int x=-hw ; x<=hw ; x++)
    {
      v(0) = x * (*px);
      v(1) = y * (*px);
      v(2) =     (*px);
      v(3) = x * (*py);
      v(4) = y * (*py);
      v(5) =     (*py);
      Add(*g6x1_aff, v, g6x1_aff);
      Transpose(v, &w);
      MatrixMultiply(v, w, &u);
      Add(*g6x6_aff, u, g6x6_aff);
      px++;  py++;
    }
  }
  assert(px == gradx_window.End());
}

//inline void iCompute6x6GradientMatrix
//(
//  const ImgFloat& gradx_window, 
//  const ImgFloat& grady_window, 
//  MatDbl* g_aff
//)
//{
//  assert( IsSameSize( gradx_window, grady_window ) );
//  assert( gradx_window.Width() % 2 == 1 );
//  assert( gradx_window.Height() % 2 == 1 );
//  const int hw = gradx_window.Width() / 2;
//  const int hh = gradx_window.Height() / 2;
//  g_aff->Reset(6, 6);
//  Set(g_aff, 0);
//  MatDbl v(6), w, u;
//  const float* px = gradx_window.Begin();
//  const float* py = grady_window.Begin();
//  for (int y=-hh ; y<=hh ; y++)
//  {
//    for (int x=-hw ; x<=hw ; x++)
//    {
//      v(0) = x * (*px);
//      v(1) = y * (*px);
//      v(2) =     (*px);
//      v(3) = x * (*py);
//      v(4) = y * (*py);
//      v(5) =     (*py);
//      Transpose(v, &w);
//      MatrixMultiply(v, w, &u);
//      Add(*g_aff, u, g_aff);
//      px++;  py++;
//    }
//  }
//  assert(px == gradx_window.End());
//}

//inline void iCompute6x1ErrorVector
//(
//  const ImgFloat& img1_window, 
//  const ImgFloat& img2_window, 
//  const ImgFloat& gradx_window, 
//  const ImgFloat& grady_window, 
//  MatDbl* e_aff
//)
//{
//  assert( IsSameSize( gradx_window, grady_window ) );
//  assert( IsSameSize( img1_window, img2_window ) );
//  assert( IsSameSize( img1_window, grady_window ) );
//  assert( gradx_window.Width() % 2 == 1 );
//  assert( gradx_window.Height() % 2 == 1 );
//  const int hw = gradx_window.Width() / 2;
//  const int hh = gradx_window.Height() / 2;
//  e_aff->Reset(6);
//  Set(e_aff, 0);
//  const float* px = gradx_window.Begin();
//  const float* py = grady_window.Begin();
//  const float* q1 = img1_window.Begin();
//  const float* q2 = img2_window.Begin();
//  for (int y=-hh ; y<=hh ; y++)
//  {
//    for (int x=-hw ; x<=hw ; x++)
//    {
//      (*e_aff)(0) += x * (*px) * (*q1 - *q2);
//      (*e_aff)(1) += y * (*px) * (*q1 - *q2);
//      (*e_aff)(2) +=     (*px) * (*q1 - *q2);
//      (*e_aff)(3) += x * (*py) * (*q1 - *q2);
//      (*e_aff)(4) += y * (*py) * (*q1 - *q2);
//      (*e_aff)(5) +=     (*py) * (*q1 - *q2);
//      px++;  py++;  q1++;  q2++;
//    }
//  }
//  assert(px == gradx_window.End());
//}

inline void iCompute6x1ErrorVector
(
  const ImgFloat& img1_window, 
  const ImgFloat& img2_window, 
  const MatDbl& g6x1_aff,
  MatDbl* e_aff
)
{
  assert( IsSameSize( img1_window, img2_window ) );
  assert( g6x1_aff.Width() == 1 && g6x1_aff.Height() == 6 );
  e_aff->Reset(6);
  Set(e_aff, 0);
  const float* q1 = img1_window.Begin();
  const float* q2 = img2_window.Begin();
  const double* g = g6x1_aff.Begin();
  double* e = e_aff->Begin();
  while (q2 != img2_window.End())
  {
    float diff = *q1 - *q2;
    e[0] += g[0] * diff;
    e[1] += g[1] * diff;
    e[2] += g[2] * diff;
    e[3] += g[3] * diff;
    e[4] += g[4] * diff;
    e[5] += g[5] * diff;
    q1++;  q2++;
  }
}

inline void iSolve6x6EquationSymm
(
  const MatDbl& g_aff,
  const MatDbl& e_aff,
  MatDbl* d_aff
)
{
  SolveLinear(g_aff, e_aff, d_aff);
}

void iInitAffineStuff(const jGaussDerivPyramid& pyr, jFeature* feat)
{
  const int hw = 3, hh = 3;  // bad idea!!! this appears in two places
  const int npix = (2*hw+1) * (2*hh+1);
  const float small_det = 10.0f * npix; // ??

  assert(pyr.NLevels() > 0);
  const ImgFloat& img1 = pyr.image[0];
  const ImgFloat& gradx1 = pyr.gradx[0];
  const ImgFloat& grady1 = pyr.grady[0];

  const float x1=feat->x, y1=feat->y;
  ImgFloat gradx_window, grady_window, img1_window;

  // note:  interpolation not needed, b/c x1 and y1 are integers
  InterpRectCenter(gradx1, x1, y1, hw, hh, &gradx_window);
  InterpRectCenter(grady1, x1, y1, hw, hh, &grady_window);
  InterpRectCenter(img1, x1, y1, hw, hh, &img1_window);  

  MatDbl g_aff, e_aff;
  iCompute6x1And6x6GradientMatrix(gradx_window, grady_window, &g_aff, &e_aff);

  feat->m_aff_initial_window = img1_window;
  feat->m_aff_grad6x1 = e_aff;
  feat->m_aff_grad6x6 = g_aff;
}

// returns true for success
bool iTrackFeature
(
  const jFeature& feat, 
  const ImgFloat& img1,
  const ImgFloat& gradx1,
  const ImgFloat& grady1,
  const ImgFloat& img2,
  float* dx,  // input and output
  float* dy
)
{
  assert( IsSameSize( img1, img2 ) );
  assert( IsSameSize( gradx1, grady1 ) );
  assert( IsSameSize( img1, gradx1 ) );
  // constants
  const int hw = 3, hh = 3;
  const int npix = (2*hw+1) * (2*hh+1);
  const float small_det = 10.0f * npix; // ??
  const float max_residue = 400.0f * npix;  // ??
  const int max_niter = 10;
  const float mindisp = 0.5f;
  const float step_size = 1.0f;  // may not be needed
  int niter = 0;

  const float x1=feat.x, y1=feat.y;
  float ddx, ddy;
  float gxx, gxy, gyy, ex, ey;
  ImgFloat gradx_window, grady_window, img1_window, img2_window;

  InterpRectCenter(gradx1, x1, y1, hw, hh, &gradx_window);
  InterpRectCenter(grady1, x1, y1, hw, hh, &grady_window);
  iCompute2x2GradientMatrix(gradx_window, grady_window, &gxx, &gxy, &gyy);
  float det = iCompute2x2Determinant(gxx, gxy, gyy);
  if (det < small_det)  return false;  // error, untrackable
  float invdet = 1 / det;
  InterpRectCenter(img1, x1, y1, hw, hh, &img1_window);
  do
  {
    InterpRectCenter(img2, x1+(*dx), y1+(*dy), hw, hh, &img2_window);
//    {
//      Figure fig1, fig2, fig3, fig4, fig5, fig6, fig7, fig8;
//      fig1.Draw(img1);
//      fig2.Draw(img2);
//      fig3.Draw(gradx1);
//      fig4.Draw(grady1);
//      fig5.Draw(gradx_window);
//      fig6.Draw(grady_window);
//      fig7.Draw(img1_window);
//      fig8.Draw(img2_window);
//      fig8.GrabMouseClick();
//    }
    iCompute2x1ErrorVector(img1_window, img2_window, gradx_window, grady_window, &ex, &ey);
    iSolve2x2EquationSymm(gxx, gxy, gyy, ex, ey, invdet, &ddx, &ddy);
    *dx += step_size * ddx;  *dy += step_size * ddy;
  }
  while ((fabs(ddx) > mindisp || fabs(ddy) > mindisp) && ++niter < max_niter);
  
//  if (niter >= max_niter)  return false;

//  int xx = blepo_ex::Round(feat.x + *dx);
//  int yy = blepo_ex::Round(feat.y + *dy);
//  if ( xx < 0 || xx >= img1.Width() ||
//       yy < 0 || yy >= img1.Height())
//  {
//    return false;  // out of bounds
//  }
  float xx = feat.x + *dx;
  float yy = feat.y + *dy;
  if ( xx < 0.0f || xx > img1.Width()-1.0f ||
       yy < 0.0f || yy > img1.Height()-1.0f)
  {
    return false;  // out of bounds
  }

  // this tests needs to be fixed by first shifting 'img2_window'
  float residue = iComputeResidue(img1_window, img2_window);
  if (residue > max_residue)
  {
    return false;  // residue too high
  }

#if 0
  // compute affine warp

  img1_window = feat.m_aff_initial_window;  // copy is unnecessary!!

  // actually img1_window should come from the first image in which feature was detected
  // 6x6 gradient matrix can be precomputed at that time as well.
  MatDbl g_aff, e_aff, d_aff;
//  iCompute6x6GradientMatrix(gradx_window, grady_window, &g_aff);
//  Interp(img1, x1, y1, hw, hh, &img1_window);
  do
  {
    // need to use affine warp for interpolation
    Interp(img2, x1+(*dx), y1+(*dy), hw, hh, &img2_window);
//    {
//      Figure fig1, fig2, fig3, fig4, fig5, fig6, fig7, fig8;
//      fig1.Draw(img1);
//      fig2.Draw(img2);
//      fig3.Draw(gradx1);
//      fig4.Draw(grady1);
//      fig5.Draw(gradx_window);
//      fig6.Draw(grady_window);
//      fig7.Draw(img1_window);
//      fig8.Draw(img2_window);
//      fig8.GrabMouseClick();
//    }

    iCompute6x1ErrorVector(img1_window, img2_window, feat.m_aff_grad6x1, &e_aff);
    iSolve6x6EquationSymm(feat.m_aff_grad6x6, e_aff, &d_aff);
//    *dx += step_size * ddx;  *dy += step_size * ddy;
  }
  // need to change the termination criterion !!!
  while ((fabs(ddx) > mindisp || fabs(ddy) > mindisp) && ++niter < max_niter);
#endif

  return true;
}

void iDrawPyramid(const jGaussDerivPyramid& pyr)
{
  const int n = 3; //pyr.size();
  assert(n == pyr.image.size());
//  std::vector<Figure> fig( n );
  Figure fig[n];
  for (int i=0 ; i < (int) pyr.image.size() ; i++)
  {
    fig[i].Draw(pyr.image[i]);
  }
}

//void iDrawPyramid(const std::vector<ImgFloat>& pyr)
//{
//  const int n = 3; //pyr.size();
////  std::vector<Figure> fig( n );
//  Figure fig[n];
//  for (int i=0 ; i<pyr.size() ; i++)
//  {
//    fig[i].Draw(pyr[i]);
//  }
//}
//
};
// ================< end local functions

namespace blepo
{

void jComputeGaussDerivPyramid(const ImgGray& img, jGaussDerivPyramid* out, int nlevels)
{
  assert(nlevels >= 1);
  if (out->image.size() != nlevels)  out->image.resize( nlevels );
  if (out->gradx.size() != nlevels)  out->gradx.resize( nlevels );
  if (out->grady.size() != nlevels)  out->grady.resize( nlevels );

  ImgFloat tmp, smoothed;
  int i = 0;

  Convert(img, &out->image[i]);

  while (1)
  {
#if 1  // small fast sigma
//    float sigma = 0.6f;
//    SmoothAndGradient(out->image[i], sigma, &smoothed, &out->gradx[i], &out->grady[i], &tmp);
//    GradientSobel(out->image[i], &out->gradx[i], &out->grady[i], &smoothed);
    GradientSharr(out->image[i], &out->gradx[i], &out->grady[i], &smoothed);
#elif 0  // small sigma
    // convolve with Gaussian
    SmoothGauss3x1WithBorders(out->image[i], &tmp);
    SmoothGauss1x3WithBorders(tmp, &smoothed);

    // compute gradient
    ImgFloat kernel(3,1);
    {
      float k[] = { 0.5, 0, -0.5 };
      memcpy(kernel.Begin(), k, 3 * sizeof(k[0]));
    }
    Convolve(smoothed, kernel, &out->gradx[i]);
    Transpose(kernel, &kernel);
    Convolve(smoothed, kernel, &out->grady[i]);
#else  // large sigma
    FastSmoothAndGradientApprox(out->image[i], &smoothed, &out->gradx[i], &out->grady[i], &tmp);
    Multiply(out->gradx[i], 0.25f, &out->gradx[i]);
    Multiply(out->grady[i], 0.25f, &out->grady[i]);
//    // convolve with Gaussian
//    SmoothGauss5x1WithBorders(out->image[i], &tmp);
//    SmoothGauss1x5WithBorders(tmp, &smoothed);
//
//    // compute gradient
//    ImgFloat kernel(5,1);
//    {
//      float k[] = { 0.25, 0, 0, 0, -0.25 };
//      memcpy(kernel.Begin(), k, 5 * sizeof(k[0]));
//    }
//    Convolve(smoothed, kernel, &out->gradx[i]);
//    Transpose(kernel, &kernel);
//    Convolve(smoothed, kernel, &out->grady[i]);
//    Figure fig1, fig2, fig3, fig4;
//    fig1.Draw(out->image[i]);
//    fig2.Draw(smoothed);
//    fig3.Draw(out->gradx[i]);
//    fig4.Draw(out->grady[i]);
//    fig4.GrabMouseClick();
#endif

    if (++i >= nlevels)  break;

    // downsample
    Downsample2x2(smoothed, &out->image[i]);
  }
}

void jDetectFeatures(const jGaussDerivPyramid& pyr, int nfeatures, std::vector<jFeature>* features, FeatureDetectMeasure fdm)
{
  assert( pyr.NLevels() > 0 );
  ImgFloat eigenmap;
  iComputeEigenMap3x3(pyr.gradx[0], pyr.grady[0], fdm, &eigenmap);
  iFindFeatures(eigenmap, nfeatures, features);

  // initialize affine stuff for later
//  for (int i=0 ; i<features->size() ; i++)
//  {
//    jFeature& feat = (*features)[i];
//    if (feat.val >= 0)  iInitAffineStuff( pyr, &feat );
//  }

}

// Track features independently using standard Lucas-Kanade
void jTrackFeatures(               
  std::vector<jFeature>* features, 
  const jGaussDerivPyramid& pyr1, 
  const jGaussDerivPyramid& pyr2
)
{
  assert( pyr1.NLevels() > 0 && pyr1.NLevels() == pyr2.NLevels() );
  assert( IsSameSize(pyr1.image[0], pyr2.image[0]) );

  const int nlevels = pyr1.NLevels();
  int i, n;

  float factor = 1.0f / powf(2.0f, nlevels-1.0f);

  for (i=0 ; i < (int) features->size() ; i++)
  {
    jFeature& feat = (*features)[i];

    if (feat.val < 0)  continue;
    
    // just for now, ignore features near boundary
    const int b = 10;
    if (!(feat.x > b && feat.x < pyr1.image[0].Width()-b && feat.y > b && feat.y < pyr1.image[0].Height()-b))  { feat.val = -1; continue; }

    feat.x *= factor;
    feat.y *= factor;

//    float dx=feat.dx * factor, dy=feat.dy * factor;
    float dx=0, dy=0;

    bool success = true;
    for (n=nlevels-1 ; n>=0 ; n--)
    {
      success = iTrackFeature(feat, pyr1.image[n], pyr1.gradx[n], pyr1.grady[n], pyr2.image[n], &dx, &dy);
      if (!success)  break;

      if (n>0)
      {
        dx *= 2;
        dy *= 2;
        feat.x *= 2;
        feat.y *= 2;
      }
    }
    feat.x += dx;
    feat.y += dy;
    feat.dx = dx;
    feat.dy = dy;
    feat.val = success ? 0.0f : -1.0f;
  }
}

void iComputeAverageVelocity
(
  const std::vector<jFeature>& features, 
  int index,
  const std::vector<float>& vdx,
  const std::vector<float>& vdy,
  float* ubar,
  float* vbar
)
{
  const int n = features.size();
  assert(n == vdx.size());
  assert(n == vdy.size());
  assert(index >= 0 && index < n);
  const jFeature& feat = features[index];
  assert(feat.val >= 0);

  std::vector<float> dist(n);
  int i;
  for (i=0 ; i<n ; i++)
  {
    const jFeature& f = features[i];
    if (f.val >= 0)
    {
      dist[i] = (f.x - feat.x) * (f.x - feat.x) + (f.y - feat.y) * (f.y - feat.y);
    }
    else
    {
      dist[i] = 9999999;
    }
  }
  std::vector<int> indices;

  SortAscending(&dist, &indices);

  const int nneighbors = 4;  // use 4 closest neighbors
  assert(n >= nneighbors);  
  float sum_dx = 0, sum_dy = 0;
  for (i=1 ; i<nneighbors ; i++)  // skip dist[0], which will always be zero (i == index)
  {
    float dx = vdx[ indices[i] ]; 
    float dy = vdy[ indices[i] ]; 
    sum_dx += dx;
    sum_dy += dy;
  }

  *ubar = sum_dx / nneighbors;
  *vbar = sum_dy / nneighbors;

//  float mindist = dist[1];  // skip dist[0], which will always be zero (i == index)
}

// This structure contains the distance from a particular feature (not captured
// here but implied by whoever owns the instance) to the feature specified in
// this class.
struct jFeatDist 
{ 
  jFeatDist() : index(-1), dist(-1) {}
  jFeatDist(int i, float d) : index(i), dist(d) {}
  int index;  // index of feature
  float dist; // distance to feature
};
//struct jFeatDist 
//{ 
//  jFeatDist() : index(-1), x(-1), y(-1), dist(-1) {}
//  jFeatDist(int i, float xx, float yy, float d) : index(i), x(xx), y(yy), dist(d) {}
//  int index;  // index of feature
//  float x, y;  // coordinates of feature
//  float dist; // distance to feature
//};

typedef std::vector<jFeatDist> jFeatDistances;

//void jFindFeaturesWithinRadius(
//  const std::vector<jFeature>& features,
//  const std::vector<DelaunayEdge>& edges,
//  std::vector<FeatDistances>* out)
//{
//  out->resize( features.size() );
//  for (int i=0 ; i<features.size() ; i++)
//  {
//    const jFeature& feat = features[i];
//
//  }
//}

// Find all features within a certain radius of a feature
// INPUT:
//   features:  a vector of features
//   radius:  the cut-off distance
// OUTPUT:
//   out:  a vector of feature distances, one per feature 
//         Each feature distance is itself a vector of pairs
//             of feature index and distance.
//         Thus, (*out)[i] is the vector for feature i
//               Feature (*out)[i][j].index is a distance of
//                       (*out)[i][j].dist from feature i,
//                       where j is a dummy index variable   
void jFindFeaturesWithinRadius(
  const std::vector<jFeature>& features,
  float radius,
  std::vector<jFeatDistances>* out)
{
  const int n = features.size();
  out->resize( n );
  for (int i=0 ; i<n ; i++)
  {
    const jFeature& feati = features[i];
    for (int j=i+1 ; j<n ; j++)
    {
      const jFeature& featj = features[j];
      assert(feati.val >= 0 && featj.val >= 0);  // if not true, then becomes more complicated
      float dx = feati.x - featj.x;
      float dy = feati.y - featj.y;
      float dist = sqrtf(dx*dx + dy*dy);
      if (dist < radius)
      {
        (*out)[i].push_back( jFeatDist( j, dist ) );
        (*out)[j].push_back( jFeatDist( i, dist ) );
//        (*out)[i].push_back( jFeatDist( j, featj.x, featj.y, dist ) );
//        (*out)[j].push_back( jFeatDist( i, feati.x, feati.y, dist ) );
      }
    }
  }
}

//void jFitAffineMotionToFeaturesWithinRadius(
//  const jFeature& feat1,
//  const std::vector<jFeature>& features2,
//  const jFeatDistances& fdist,  // specifies which features to consider
//  float sigma,  // for weighted least squares
//  MatDbl* out)  // affine parameters
//{
//  Array<Point2d> pt1;
//  Array<Point2d> pt2;
//  for (int i=0 ; i<fdist.size() ; i++)
//  {
//    const jFeatDist& fd = fdist[i];
//
//  }
//  AffineFit(pt1, pt2, out);
//}

int TestDegenerate(const std::vector<Point2d>& pts)
{
  const double th = 5 * 5;
  int n = pts.size();
  double xx=0, xy=0, yy=0;
  for (int i=0 ; i<n ; i++)
  {
    const Point2d& p = pts[i];
    xx += p.x * p.x;
    xy += p.x * p.y;
    yy += p.y * p.y;
  }
  xx /= n;
  xy /= n;
  yy /= n;
  double mineig = ((xx + yy - sqrt((xx - yy)*(xx - yy) + 4*xy*xy))/2.0f);
//  double maxeig = ((xx + yy + sqrt((xx - yy)*(xx - yy) + 4*xy*xy))/2.0f);
  double theta = 0.5 * atan2( 2 * xy, xx - yy );

  const double pi4 = 3.14159 / 4.0;
  if (mineig > th)
  {
    return 0; // not degenerate
  }
  else if (fabs(theta) > pi4 && fabs(theta) < 3*pi4)
  {
    return 1;  // degenerate, primarily vertical
  }
  else
  {
    return -1;  // degenerate, primarily horizontal
  }
}

bool SolveDegenerate(const jFeature& feat,
                     const std::vector<Point2d>& pt1, 
                     const std::vector<Point2d>& pt2, 
                     const std::vector<double>& weights, 
                     float* ubar, 
                     float* vbar)
{
  assert(pt1.size() == pt2.size());
  assert(pt1.size() >= 2);
  int deg = TestDegenerate(pt1);

  const int n = pt1.size();

#if 1  // compute weighted average
  double sumx=0, sumy=0, sum=0;
  for (int i=0 ; i<n ; i++)
  {
    const Point2d& p1 = pt1[i];
    const Point2d& p2 = pt2[i];
    const double w = weights[i];
    sumx += w * (p2.x - p1.x);
    sumy += w * (p2.y - p1.y);
    sum  += w;
  }
  *ubar = static_cast<float>(sumx / sum);
  *vbar = static_cast<float>(sumy / sum);

#else  // fit two lines to dx and dy
  MatDbl xs1(2, n);
  MatDbl ys1(2, n);
  MatDbl xs2(1, n);
  MatDbl ys2(1, n);
  for (int i=0 ; i<n ; i++)
  {
    xs1(0, i) = pt1[i].x;
    ys1(0, i) = pt1[i].y;
    xs1(1, i) = 1;
    ys1(1, i) = 1;
    xs2(0, i) = pt2[i].x;
    ys2(0, i) = pt2[i].y;
  }

  double x1i = feat.x;
  double y1i = feat.y;
  double x2i, y2i;

  if (deg > 0)
  {  // vertical
    MatDbl a, b, tmp_residue;
    SolveLinearQr(xs1, xs2, &a, &tmp_residue);
    SolveLinearQr(ys1, ys2, &b, &tmp_residue);
    x2i = a(0,0) * x1i + a(0,1);
    y2i = b(0,0) * y1i + b(0,1);
//    SolveLinearQr(ys1, xs2, &a, &tmp_residue);
//    SolveLinearQr(ys1, ys2, &b, &tmp_residue);
//    x2i = a(0,0) * y1i + a(0,1);
//    y2i = b(0,0) * y1i + b(0,1);
  }
  else
  {  // horizontal
    MatDbl a, b, tmp_residue;
    SolveLinearQr(xs1, xs2, &a, &tmp_residue);
    SolveLinearQr(ys1, ys2, &b, &tmp_residue);
    x2i = a(0,0) * x1i + a(0,1);
    y2i = b(0,0) * y1i + b(0,1);
//    SolveLinearQr(xs1, xs2, &a, &tmp_residue);
//    SolveLinearQr(xs1, ys2, &b, &tmp_residue);
//    x2i = a(0,0) * x1i + a(0,1);
//    y2i = b(0,0) * x1i + b(0,1);
  }

  // subtract to get expected motion
  *ubar = x2i - x1i;
  *vbar = y2i - y1i;
#endif

  return (deg != 0);
}

// For all features, compute the expected (u,v) velocity by fitting an affine
// model to the neighbors
void iComputeExpectedVelocitiesAffine
(
  int pyr_level,  // just for debugging
  const std::vector<jFeature>& features, 
  const std::vector<float>& vdx,
  const std::vector<float>& vdy,
  float affine_sigma,  // standard deviation of Gaussian for weighted least squares
                       // (not used yet)
  const std::vector<jFeatDistances>& fdist,  // distance to other features
  std::vector<float>* ubars,
  std::vector<float>* vbars,
  std::vector<bool>* valid_uvbars  // whether ubars / vbars are valid and should be used
)
{
  double norm = 1 / (2 * affine_sigma * affine_sigma);

  const int n = features.size();
  ubars->resize( n );
  vbars->resize( n );
  valid_uvbars->resize( n );
  assert(fdist.size() == n);

  // for each feature ...
  for (int i=0 ; i<n ; i++)
  {
//    if (i==487)
//    {
//      int kkk=0;
//    }
    bool valid;
    float ubar, vbar;
    const jFeature& feati = features[i];
    if (feati.val >= 0)
    {
      const jFeatDistances& fdd = fdist[i];
      std::vector<Point2d> pt1;
      std::vector<Point2d> pt2;
      std::vector<double> weights;

      // for each neighbor of the feature ...
      for (int j=0 ; j < (int) fdd.size() ; j++)
      {
        const int index = fdd[j].index;
        const float dist = fdd[j].dist;
        const jFeature& featj = features[index];

        float x1 = featj.x;
        float y1 = featj.y;
        float x2 = x1 + vdx[index];
        float y2 = y1 + vdy[index];
//        assert(x2 == x1 && y2 == y1);  // just for now

        // store the coordinates in frame1 and frame2
        pt1.push_back( Point2d(x1, y1) );
        pt2.push_back( Point2d(x2, y2) );
        
        /*
        nkanher 01/22/08

        //weights.push_back( exp( - dist * dist * norm ) ); // this gives error
        
        If I uncomment the line above, I get a fatal error:  
        C:\research\code\vc6\alpha1\src\Image\JointFeatureTracker.cpp(1045) : fatal error C1001: INTERNAL COMPILER ERROR
        (compiler file 'E:\8168\vc98\p2\src\P2\main.c', line 494)

        Following works however
        */

        double tmp = exp( - dist * dist * norm ); // this works
        weights.push_back(tmp);                   //
      }

      bool degenerate = false;
      if (pt1.size() >= 4)  // 4 is somewhat arbitrary number
      {
        degenerate = SolveDegenerate(feati, pt1, pt2, weights, &ubar, &vbar);
      }

      if (degenerate)
      {
        valid = true;
      }
      else if (pt1.size() >= 6)  // 6 is somewhat arbitrary number
      {
        // fit an affine model to the neighbors
        // Note:  Could do a weighted least squares here to achieve
        //        better results; need to use affine_sigma above
        MatDbl affparam;  // affine parameters
#if 0
        AffineFit(pt1, pt2, &affparam);
#else
        WeightedAffineFit(pt1, pt2, weights, &affparam);
#endif

        // apply affine model to feature to get expected location in frame2
        float x1i = feati.x;
        float y1i = feati.y;
        float x2i = static_cast<float>( affparam(0,0) * x1i + affparam(1,0) * y1i + affparam(2,0) );
        float y2i = static_cast<float>( affparam(0,1) * x1i + affparam(1,1) * y1i + affparam(2,1) );

        // subtract to get expected motion
        ubar = x2i - x1i;
        vbar = y2i - y1i;

//        // debugging:  these are occluded pixels
//        if (i==571 || i==295 || i==279 || i==462 || i==988 || i==697 || i==718 || i==261 || i==462 || i==355 || i==988 || i==153 || i==521 || i==261 || i==308)
//        {
//          ubar = 3 * powf(0.5f, pyr_level);
//          vbar = 0;
//        }

        valid = true;
      }
      else
      {  // just use average of pixels
        valid = true;
      }
    }
    else
    {
      // This is an invalid feature; should not be a problem, because ubar and vbar
      // will never be looked at anyway, because invalid features are not tracked.
      ubar = 0;
      vbar = 0;
    }
    (*ubars)[i] = ubar;
    (*vbars)[i] = vbar;
    (*valid_uvbars)[i] = valid;
  }
}

//void jComputeDelaunayOfFeatures(
//  const std::vector<jFeature>& features1,
//  std::vector<DelaunayEdge>* edges)
//{
//  // copy valid features to points
//  // Note:  If any of the features is invalid, then we will need to 
//  //        compute a separate vector to maintain correspondence 
//  //        between indices of points and indices of features.
//  //        Ignoring this for now since we don't need it for our simple tests.
//  std::vector<Point2d> points;
//  for (int i=0 ; i<features1.size() ; i++)
//  {
//    const jFeature& feat = features1[i];
//    assert(feat.val >= 0);  // see comment above
//    points.push_back( Point2d( feat.x, feat.y ) );
//  }
//  // compute Delaunay
//  ComputeDelaunayShewchuk( points, edges, NULL );
//}                            

void jTrackFeaturesJointly
(
  std::vector<jFeature>* features, 
  const jGaussDerivPyramid& pyr1, 
  const jGaussDerivPyramid& pyr2
)
{
  assert( pyr1.NLevels() > 0 && pyr1.NLevels() == pyr2.NLevels() );
  assert( IsSameSize(pyr1.image[0], pyr2.image[0]) );

  const float smallnum = 0.01f; // ??
  const int max_niter = 10;
  const float mindisp = 0.1f;
  const float step_size = 1.0f;  // may not be needed
  const int hw = 3, hh = 3;
  const int window_size = (2*hw+1) * (2*hh+1);
  const float affine_sigma = 10.0f;  // standard deviation of Gaussian for weighted least squares
                                     // (not used yet)
  int niter = 0;
  float ddx, ddy;
  float gxx, gxy, gyy, ex, ey;
  float disp;
  int i, n;
  const float max_residue = 20.0f * 20.0f * window_size;
  ImgFloat gradx_window, grady_window, img1_window, img2_window;
  float lambda = 50.0f * window_size; //0.1f;
//  float lambda = 0;
//  std::vector<DelaunayEdge> edges;

//  std::vector<ImgFloat> gradx_windows( features->size());
//  std::vector<ImgFloat> grady_windows( features->size());
//  std::vector<ImgFloat> img1_windows( features->size());
  std::vector<float> ubars( features->size(), 0 );
  std::vector<float> vbars( features->size(), 0 );
  std::vector<bool> valid_uvbars( features->size(), 0 );
  float ubar, vbar;

//  jComputeDelaunayOfFeatures(*features, &edges);

  std::vector<float> vdx( features->size(), 0 );  // holds dx and dy for this pyramid level
  std::vector<float> vdy( features->size(), 0 );
  std::vector<float> residues( features->size(), 0 );
//  std::vector<float> vvdx( features->size(), 0 );  // holds overall dx and dy
//  std::vector<float> vvdy( features->size(), 0 );

  const int nlevels = pyr1.NLevels();
  float factor = 1.0f / powf(2.0f, nlevels-1.0f);

  std::vector<jFeatDistances> fdist;  // distance to other features
  const float radius = affine_sigma * 3;  // radius in which to consider motions of neighboring features
  jFindFeaturesWithinRadius(*features, radius, &fdist);

  // scale all features down to smallest pyramid level
  for (i=0 ; i < (int) features->size() ; i++)
  {
    jFeature& feat = (*features)[i];
    if (feat.val < 0)  continue;

    feat.x *= factor;
    feat.y *= factor;
  }
  
  // do pyramidal tracking
  for (n=nlevels-1 ; n>=0 ; n--)
  {
    const ImgFloat& img1 = pyr1.image[n];
    const ImgFloat& img2 = pyr2.image[n];
    const ImgFloat& gradx1 = pyr1.gradx[n];
    const ImgFloat& grady1 = pyr1.grady[n];

//    // precompute image windows and matrices for img1 (unfinished)
//    for (i=0 ; i<features->size() ; i++)
//    {
//      jFeature& feat = (*features)[i];
//      if (feat.val < 0)  continue;
//    }
  
    for (niter=0 ; niter<max_niter ; niter++)
    {
//      bool use_lambda = (niter > 2) && lambda != 0;  // ignore lambda for the first few iterations
//      bool use_lambda = (niter > 0) && lambda != 0;  // ignore lambda for the first few iterations
      bool use_lambda = lambda != 0;  // ignore lambda if not needed (for speed)

      disp = 0;  // used to cancel the iterations prematurely


      if (use_lambda)  iComputeExpectedVelocitiesAffine(n, *features, vdx, vdy, affine_sigma, fdist, &ubars, &vbars, &valid_uvbars);

      TRACE("nfeatures = %d\n", features->size());
      for (i=0 ; i < (int) features->size() ; i++)
      {
        if (i%1000==0)
        {
          TRACE("iter=%d feat=%d\n", niter, i);
        }
        if (i==487)
        {
          int kkk=0;
        }
  //      if (i==features->size()-1) 
  //      {
  //        int k=5;
  //      }
        jFeature& feat = (*features)[i];
        if (feat.val < 0)  continue;

        const float x1=feat.x, y1=feat.y;
//        float* dx = vdx.begin() + i;
//        float* dy = vdy.begin() + i;
        std::vector<float>::iterator dx = vdx.begin() + i;
        std::vector<float>::iterator dy = vdy.begin() + i;
        int hwl = blepo_ex::Min( (int) blepo_ex::Min( x1, x1+(*dx) ), hw );  // left
        int hht = blepo_ex::Min( (int) blepo_ex::Min( y1, y1+(*dy) ), hh );  // top
        int hwr = blepo_ex::Min( img1.Width()  - 1 - (int) ceil(blepo_ex::Max( x1, x1+(*dx) )), hw );  // right
        int hhb = blepo_ex::Min( img1.Height() - 1 - (int) ceil(blepo_ex::Max( y1, y1+(*dy) )), hh );  // bottom
//        assert(hwl >= 0 && hwl <= hw);
//        assert(hwr >= 0 && hwr <= hw);
//        assert(hht >= 0 && hht <= hh);
//        assert(hhb >= 0 && hhb <= hh);
        InterpRectCenterAsymmetric(gradx1, x1, y1, hwl, hwr, hht, hhb, &gradx_window);
        InterpRectCenterAsymmetric(grady1, x1, y1, hwl, hwr, hht, hhb, &grady_window);
//        InterpRectCenter(gradx1, x1, y1, hw, hh, &gradx_window);
//        InterpRectCenter(grady1, x1, y1, hw, hh, &grady_window);
//        if (i==100)
//        {
//          int K = 5;
//        }
        // use equation in original Horn-Schunck paper
        iCompute2x2GradientMatrix(gradx_window, grady_window, &gxx, &gxy, &gyy);
        if (use_lambda && valid_uvbars[i])
        {
          gxx += lambda;
          gyy += lambda;
        }
        float det = iCompute2x2Determinant(gxx, gxy, gyy);
        if (det < smallnum)  
        { // error, untrackable
          feat.val = -1;
          continue;
        }
        float invdet = 1 / det;
        InterpRectCenterAsymmetric(img1, x1, y1, hwl, hwr, hht, hhb, &img1_window);
        InterpRectCenterAsymmetric(img2, x1+(*dx), y1+(*dy), hwl, hwr, hht, hhb, &img2_window);
//        InterpRectCenter(img1, x1, y1, hw, hh, &img1_window);
//        InterpRectCenter(img2, x1+(*dx), y1+(*dy), hw, hh, &img2_window);
        iCompute2x1ErrorVector(img1_window, img2_window, gradx_window, grady_window, &ex, &ey);
        if (use_lambda && valid_uvbars[i])
        {
//          iComputeAverageVelocity(*features, i, vdx, vdy, &ubar, &vbar);
          ubar = ubars[i];  vbar = vbars[i];
          ubar -= *dx;
          vbar -= *dy;
          ex += lambda * ubar;
          ey += lambda * vbar;
        }
        iSolve2x2EquationSymm(gxx, gxy, gyy, ex, ey, invdet, &ddx, &ddy);
        *dx += step_size * ddx;  *dy += step_size * ddy;
        feat.val = 0;  // assume successfully tracked

        {
//          int xx = blepo_ex::Round(feat.x + *dx);
//          int yy = blepo_ex::Round(feat.y + *dy);
          float xx = feat.x + *dx;
          float yy = feat.y + *dy;
          if ( xx < 0.0f || xx > img1.Width()-1.0f ||
               yy < 0.0f || yy > img1.Height()-1.0f)
          {
            feat.val = -1;  // out of bounds
          }

        }
    
        if (n==0)
        {
          residues[i] = iComputeResidue(img1_window, img2_window);
        }

        disp = blepo_ex::Max(disp, fabsf(ddx));
        disp = blepo_ex::Max(disp, fabsf(ddy));

//        if (niter == max_iter-1)
//        {
//          *dx = 2;  *dy = 1;
//        }
      }
//      if (disp < mindisp)  break;
    }

    for (i=0 ; i < (int) features->size() ; i++)
    {
      jFeature& feat = (*features)[i];
      if (feat.val < 0)  continue;
      if (n>0)
      {
        feat.x *= 2;
        feat.y *= 2;
        vdx[i] *= 2;
        vdy[i] *= 2;
//        vdx[i] = 0;
//        vdy[i] = 0;
      }
//      vvdx[i] += vdx[i];
//      vvdy[i] += vdy[i];
    }
  } // end of tracking

//  CString str;
//  str.Format("done tracking.  nfeatures=%d %d", features->size(), jCountValidFeatures(*features));
//  AfxMessageBox(str);
  for (i=0 ; i < (int) features->size() ; i++)
  {
    jFeature& feat = (*features)[i];
#if 1
    if (residues[i] > max_residue)  feat.val = -1;  // detect occluded (or bad) pixels
#endif
    if (feat.val >= 0)
    {
      feat.x += vdx[i];
      feat.y += vdy[i];
    }
    else
    {
      feat.x += ubars[i];  // not a very good idea, but doesn't hurt (since feat is invalid anyway)
      feat.y += vbars[i];
    }
//    feat.x += vvdx[i];
//    feat.y += vvdy[i];
  }

//
//  const int nlevels = pyr1.NLevels();
//  int i, n;
//  int iter, max_iter;
//
//  bool success = true;
//  for (n=nlevels-1 ; n>=0 ; n--)
//  {
//    for (i=0 ; i<features->size() ; i++)
//    {
//      jFeature& feat = (*features)[i];
//    }
//
//  }
//
   
}

//#else  // use equation in L/K meets H/S Bruhn et al. 2005
//        Interp(img1, x1, y1, hw, hh, &img1_window);
//        Interp(img2, x1+(*dx), y1+(*dy), hw, hh, &img2_window);
//        iComputeAverageVelocity(*features, i, vdx, vdy, &ubar, &vbar);
//        ubar -= *dx;
//        vbar -= *dy;
//        float gxsum=0, gysum=0, itsum=0;
//        iComputeSumsForLkhs(img1_window, img2_window, gradx_window, grady_window, &gxsum, &gysum, &itsum);
//        ddx = (lambda * ubar - gysum * (*dy) - itsum) / (gxsum + lambda);
//        ddy = (lambda * vbar - gxsum * (*dx) - itsum) / (gysum + lambda);
//#endif

int jCountValidFeatures(const std::vector<jFeature>& features)
{
  int i, count = 0;
  for (i=0 ; i < (int) features.size() ; i++)
  {
    if (features[i].val >= 0)  count++;
  }
  return count;
}

void jReplaceFeatures(std::vector<jFeature>* features, const jGaussDerivPyramid& pyr, int mindist)
{
  assert( pyr.NLevels() > 0 );
  std::vector<jFeature> newfeat;
  jDetectFeatures(pyr, -1, &newfeat);

  // create mask
  ImgBinary mask( pyr.image[0].Width() / mindist, pyr.image[0].Height() / mindist );
  Set(&mask, 0);
  int i, j;

  // initialize mask
  for (i=0 ; i < (int) features->size() ; i++)
  {
    jFeature& feat = (*features)[i];
    if (feat.val >= 0)
    {
      int xx = blepo_ex::Round(feat.x) / mindist;
      int yy = blepo_ex::Round(feat.y) / mindist;
      if (xx>=0 && xx<mask.Width() && yy>=0 && yy<mask.Height())
      {
        mask( xx, yy ) = 1;
      }
      else
      {
        feat.val = -1;  // out of bounds
      }
    }
  }

  // replace lost features
  j = 0;
  for (i=0 ; i < (int) features->size() ; i++)
  {
    jFeature& feat = (*features)[i];
    if (feat.val < 0)
    {
      while (1)
      {
        jFeature& nf = newfeat[j];
        int xx = blepo_ex::Round(nf.x) / mindist;
        int yy = blepo_ex::Round(nf.y) / mindist;
        if (nf.val >=0 && !mask( xx, yy ) )
        {
          feat = nf;
          mask( xx, yy ) = 1;
          break;
        }
        j++;
        if (j == newfeat.size())  return;
      }
    }
  }
}

void jDrawFeatures(ImgBgr* img, const std::vector<jFeature>& features)
{
  for (int i=0 ; i < (int) features.size() ; i++)
  {
    const jFeature& a = features[i];
    if (a.val >= 0)
    {
      Point pt( blepo_ex::Round(a.x), blepo_ex::Round(a.y));
      Bgr color = (a.val > 0) ? Bgr(0,255,0) : Bgr(0,0,255);
      if (i==68)
      {
        color = Bgr(0,255,255);
      }
      if (pt.x >= 1 && pt.x < img->Width()-1 &&
          pt.y >= 1 && pt.y < img->Height()-1)
      {
        DrawDot(pt, img, color, 3);
      }
      else if (pt.x >= 0 && pt.x < img->Width() &&
               pt.y >= 0 && pt.y < img->Height())
      {
        DrawDot(pt, img, color, 1);
      }
    }
  }
}

int jFindClosestValidFeature(float x, float y, const std::vector<jFeature>& features)
{
  int index = -1;
  float mindist = 999999.0f;
  for (int i=0 ; i < (int) features.size() ; i++)
  {
    if (features[i].val >= 0)
    {
      float dx = (features[i].x-x);
      float dy = (features[i].y-y);
      float dist = dx*dx + dy*dy;
      if (dist < mindist)
      {
        mindist = dist;
        index = i;
      }
    }
  }
  return index;
}

void jReadFeaturesAscii(const char* fname, std::vector<jFeature>* features)
{
  FILE* fp = fopen(fname, "rt");
  if (fp)
  {
    int n;
    fscanf(fp, "%d\n", &n);
    features->resize(n);
    for (int i=0 ; i<n ; i++)
    {
      float x, y;
      fscanf(fp, "%f %f\n", &x, &y);
      (*features)[i].x = x;
      (*features)[i].y = y;
      (*features)[i].val = 0;
    }
    fclose(fp);
  }
  else
  {
    assert(0);
  }
}

void jConvertFromFeatureList(const KltFeatureList& fl, std::vector<jFeature>* features)
{
  features->resize( fl.GetNFeatures() );
  for (int i=0 ; i<fl.GetNFeatures() ; i++)
  {
    KLT_Feature feat = fl[i];
    (*features)[i] = jFeature( feat->x, feat->y, feat->val );
  }
}

void jConvertToFeatureList(const std::vector<jFeature>& features, KltFeatureList* fl)
{
  fl->Reset( features.size() );
  for (int i=0 ; i < (int) features.size() ; i++)
  {
    const jFeature& feat = features[i];
    KLT_Feature f = (*fl)[i];
    f->x = feat.x;
    f->y = feat.y;
    f->val = static_cast<int>( feat.val );
  }
}

void jConvertFromFeatureArr(const FastFeatureTracker::FeatureArr& fa, std::vector<jFeature>* features)
{
  features->resize( fa.Len() );
  for (int i=0 ; i<fa.Len() ; i++)
  {
    const FastFeatureTracker::Feature& feat = fa[i];
    (*features)[i] = jFeature( feat.x, feat.y, feat.status );
  }
}

void jConvertToFeatureArr(const std::vector<jFeature>& features, FastFeatureTracker::FeatureArr* fa)
{
  fa->Reset( features.size() );
  for (int i=0 ; i < (int) features.size() ; i++)
  {
    const jFeature& feat = features[i];
    FastFeatureTracker::Feature& f = (*fa)[i];
    f.x = feat.x;
    f.y = feat.y;
    int val = blepo_ex::Min((int) feat.val, 1);
    f.status = (FastFeatureTracker::FeatureStatus) val;
  }
}

};  // end namespace blepo

