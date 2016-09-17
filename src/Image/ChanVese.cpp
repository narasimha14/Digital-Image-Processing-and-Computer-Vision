/* 
 * Copyright (c) 2009 Clemson University.
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
#include "Figure/Figure.h"
#include "ImageOperations.h"  // Set, ...
#include "ImageAlgorithms.h"  // ChanVeseParams
//#include <vector>

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

float g_sigma = 0.5f;

void iComputeMeanValues(const ImgGray& img, const ImgFloat& phi, float* ci, float* co)
{
//  int w = img.Width(), h = img.Height();
  assert(IsSameSize(img, phi));
  //w == phi.Width() && h == phi.Height());
  const unsigned char* p = img.Begin();
  const float* q = phi.Begin();
  *ci = 0;
  *co = 0;
  int ni = 0, no = 0;

  while (p != img.End())
  {
    if (*q >= 0)  
    {
      *ci += *p;  
      ni++;
    }
    else 
    {
      *co += *p;
      no++;
    }
    p++;  q++;
  }
  *ci /= ni;
  *co /= no;
}

// Not the fastest implementation, but will do for now
void iComputeDivergence(const ImgFloat& gradx, const ImgFloat& grady, ImgFloat* div)
{
  assert(IsSameSize(gradx, grady));
  ImgFloat ggx, ggy, tmp;

  Gradient(gradx, g_sigma, &ggx, &tmp);
  Gradient(grady, g_sigma, &tmp, &ggy);
  Add(ggx, ggy, div);
}

// This function
//   1.  Computes the gradient magnitude
//   2.  Normalizes the gradient by the gradient magnitude
//   3.  Computes the divergence of the normalized gradient
void iComputeGradMagAndDivergenceOfNormalizedGrad(const ImgFloat& phi, ImgFloat* gradmag, ImgFloat* div)
{
  ImgFloat gradx, grady;
  gradmag->Reset( phi.Width(), phi.Height() );
  div->Reset( phi.Width(), phi.Height() );

  GradMag(phi, g_sigma, gradmag);
  Gradient(phi, g_sigma, &gradx, &grady);

  const float* g  = gradmag->Begin();
  float* gx = gradx.Begin();
  float* gy = grady.Begin();
  
//  Figure figf("gradmagfoo");
//  figf.Draw(*gradmag);
//  figf.GrabMouseClick();
//  figf.Draw(gradx);
//  figf.GrabMouseClick();
//  figf.Draw(grady);
//  figf.GrabMouseClick();

  // normalize gradient
  while (g != gradmag->End())
  {
    if (*g >= 0.01f)  // avoid divide by zero
    {
      *gx /= *g;
      *gy /= *g;
    }
    g++;  gx++;  gy++;
  }

//  figf.Draw(gradx);
//  figf.GrabMouseClick();
//  figf.Draw(grady);
//  figf.GrabMouseClick();

  // compute divergence
  iComputeDivergence(gradx, grady, div);

//  figf.Draw(*div);
//  figf.GrabMouseClick();
}

#include <float.h>  // _isnan

void iComputeDeltaPhi(const ImgGray& img, float ci, float co, const ImgFloat& phi, const ChanVeseParams& params, ImgFloat* delta_phi)
{
  assert(IsSameSize(img, phi));
  delta_phi->Reset( img.Width(), img.Height() );
  ImgFloat gradmagphi, div;

  iComputeGradMagAndDivergenceOfNormalizedGrad(phi, &gradmagphi, &div);

  const unsigned char* p = img.Begin();
  const float* q = phi.Begin();
  const float* g = gradmagphi.Begin();
  const float* d = div.Begin();
  float* r = delta_phi->Begin();

  while (p != img.End())
  {
    float vi = (*p - ci) * (*p - ci);
    float vo = (*p - co) * (*p - co);
    if (_isnan(*d) || fabs(*d) > 99999)
    {
      int k = 0;
    }
//    *r = - 0.01f * (*g) * ( params.nu + params.lambda_i * vi - params.lambda_o * vo - params.mu * (*d) );
    *r = - (*g) * ( params.nu + params.lambda_i * vi - params.lambda_o * vo - params.mu * (*d) );
    if (_isnan( *r ) || (*r < -9999999) || (*r > 9999999))
    {
      TRACE("g=%f vi=%f vo=%f d=%f\n", *g, vi, vo, *d);
    }
    p++;  q++;  g++;  d++;  r++;
  }
}

void iComputeZeroLevelSet(const ImgFloat& phi, ImgBinary* boundary)
{
  boundary->Reset( phi.Width(), phi.Height() );
  Set(boundary, 0);
  for (int y=1 ; y<phi.Height() ; y++)
  {
    for (int x=1 ; x<phi.Width() ; x++)
    {
      if ( phi(x, y) * phi(x-1, y) <= 0 )  (*boundary)(x, y) = 1;
      if ( phi(x, y) * phi(x, y-1) <= 0 )  (*boundary)(x, y) = 1;
    }
  }
}

void iReinitializePhi(const ImgBinary& boundary, ImgFloat* phi)
{
  assert( IsSameSize( *phi, boundary ) );
  ImgGray gray;
  Convert(boundary, &gray);
  ImgInt chamf;
  Chamfer(gray, &chamf);

  float* p = phi->Begin();
  const int* q = chamf.Begin();
  while (p != phi->End())
  {
    *p = static_cast<float>( (*p >= 0) ? *q : -*q );
    p++;  q++;
  }

//  Convert(chamf, phi);
}

void iDisplayResult(const ImgGray& img, const ImgBinary& boundary)
{
  static Figure fig("boundary");
  ImgBgr bgr;
  Convert(img, &bgr);
  Set(&bgr, boundary, Bgr(0, 0, 255));
  fig.Draw(bgr);
//  fig.GrabMouseClick();
}

};
// ================< end local functions

namespace blepo {

/** 
Implements the Chan-Vese segmentation algorithm, described in 
T. F. Chan and L. A. Vese, Active Contours without Edges, IEEE Trans. on Image Processing, 10(2), 2001.

@author Stan Birchfield
*/
void ChanVese(const ImgGray& img, ImgBinary* out, const ChanVeseParams& params)
{
  static Figure fig2("phi"), fig3("dphi");

//  Figure figf;
//  figf.Draw(img);
//  figf.GrabMouseClick();

  int w = img.Width(), h = img.Height();

  // Initialize implicit function to
  //    +1 for interior pixels
  //    -1 for pixels near the image boundary
  ImgFloat phi(w, h); // implicit function (positive inside, negative outside)
  ImgFloat delta_phi(w, h);
  Set(&phi, -1);
  const int b = params.init_border;  // initial border
  Set(&phi, Rect(b, b, w-b, h-b), 1); 

  ImgBinary boundary(w, h), boundary_prev(w, h);
  float ci, co;  // interior and exterior mean graylevels

  // initial display
  iComputeZeroLevelSet(phi, &boundary);
  if (params.display)  iDisplayResult(img, boundary);

  int iter = 0;
  while (1)
  {
    iComputeMeanValues(img, phi, &ci, &co);

//    fig2.Draw(phi);
//    fig2.GrabMouseClick();

    iComputeDeltaPhi(img, ci, co, phi, params, &delta_phi);

//    {
//      CString str;
//      str.Format("aa_phi%02d.pgm", iter);
//      Save(phi, str);
//      str.Format("aa_deltaphi%02d.pgm", iter);
//      Save(delta_phi, str);
//    }


    TRACE("iter=%d ci=%f co=%f\n", iter, ci, co);
    if (iter == 11)
    {
      int k= 0 ;
    }
//    fig2.Draw(phi);
//    fig3.Draw(delta_phi);
//    fig3.GrabMouseClick();

    Add(phi, delta_phi, &phi);

//    fig2.Draw(phi);
//    fig2.GrabMouseClick();

    iComputeZeroLevelSet(phi, &boundary);

    iReinitializePhi(boundary, &phi);

    if (IsIdentical(boundary, boundary_prev))  break;
    if (++iter >= params.max_niter)  break;

    boundary_prev = boundary;

    if (params.display)  iDisplayResult(img, boundary);
//    fig3.GrabMouseClick();
  }
}

};  // end namespace blepo
