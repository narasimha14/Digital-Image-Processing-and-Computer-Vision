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

#include "ImageAlgorithms.h"
#include "ImageOperations.h"
#include "Figure/Figure.h"
#include "Utilities/Math.h"
#include <math.h>  // sqrt, atan2
#include <algorithm> // std::sort()
#include <functional> // std::greater
#include "ImgIplImage.h"
#include "blepo_opencv.h" // OpenCV

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

void HornSchunck
(
  const jGaussDerivPyramid& pyr1, 
  const jGaussDerivPyramid& pyr2,
  ImgFloat* u_out,
  ImgFloat* v_out,
  float lambda
)
{
  assert( pyr1.NLevels() > 0 && pyr1.NLevels() == pyr2.NLevels() );
  assert( IsSameSize(pyr1.image[0], pyr2.image[0]) );

  const int nlevels = pyr1.NLevels();
  const int max_niter = 10; //310000;
  float ubar, vbar, unew, vnew, num, den, gamma;
  int i, x, y;

  float factor = 1.0f / powf(2.0f, nlevels-1.0f);

  const ImgFloat& img1 = pyr1.image[0];
  const ImgFloat& img2 = pyr2.image[0];
  const ImgFloat& gradx = pyr1.gradx[0];
  const ImgFloat& grady = pyr1.grady[0];
  ImgFloat diff;
  float diff_val;
  Subtract(img2, img1, &diff);
  ImgFloat& u = *u_out;
  ImgFloat& v = *v_out;
  u.Reset( img1.Width(), img1.Height() );
  v.Reset( img1.Width(), img1.Height() );
  Set(&u, 1);
  Set(&v, 0);

  for (i=0 ; i<max_niter ; i++)
  {
    for (y=0 ; y<img1.Height() ; y++)
    {
      for (x=0 ; x<img1.Width() ; x++)
      {
//        if (x==94 && y==4)   // 71 => 61
//        {
//          int kk=0;
//        }

        if (x>0 && y>0 && x<img1.Width()-1 && y<img1.Height()-1)
        {
          ubar = 0.25f * ( u(x-1,y) + u(x+1,y) + u(x,y-1) + u(x,y+1) );
          vbar = 0.25f * ( v(x-1,y) + v(x+1,y) + v(x,y-1) + v(x,y+1) );
        }
        else
        {
          ubar = u(x, y);
          vbar = v(x, y);
        }

#if 1  // standard way
        diff_val = diff(x, y);
#else
        {  // shift pixels by the current estimate of velocity; seems like it should work,
           // but produces strange results
          float val1 = img1(x, y);
          float val2 = Interp(img2, x + u(x,y), y + v(x,y));
          diff_val = val2 - val1;
        }
#endif
        num = gradx(x, y) * ubar + grady(x, y) * vbar + diff_val;
        den = lambda + gradx(x, y) * gradx(x, y) + grady(x, y) * grady(x, y);
        gamma = num / den;

        unew = ubar - gamma * gradx(x, y);
        vnew = vbar - gamma * grady(x, y);

#if 1  // Gauss-Seidel
        u(x,y) = unew;
        v(x,y) = vnew;
#else  // successive over relaxation (SOR)
        const float sor_fact = 1.9f;
        const float sor_omf = 1.0f - sor_fact;
        u(x,y) = sor_fact * unew + sor_omf * u(x,y); 
        v(x,y) = sor_fact * vnew + sor_omf * v(x,y);
#endif 
      }
    }
  }

}

};  // end namespace blepo

// run OpenCV's version of Horn-Schunck
void OpencvHS()
//(
//  const jGaussDerivPyramid& pyr1, 
//  const jGaussDerivPyramid& pyr2,
//  ImgFloat* u_out,
//  ImgFloat* v_out,
//  float lambda
//)
{
//  const int nlevels = 2;
//  const float lambda = 100.0f;//1000.0f; //0.1f; //1000000.0f;
  ImgFloat u, v;

  CString fname;
  ImgGray img1, img2;
//  jGaussDerivPyramid pyr1, pyr2;
  ImgBgr bgr;
  Figure fig, figu("u"), figv("v");
  std::vector<jFeature> features;

  Load("img0140.pgm", &img1);
  Load("img0140_r1.pgm", &img2);
  assert( IsSameSize(img1, img2) );

  u.Reset( img1.Width(), img1.Height() );
  v.Reset( img1.Width(), img1.Height() );
//  Load("yosemite00.pgm", &img1);
//  Load("yosemite01.pgm", &img2);
//  Load("yosemite00_r1.pgm", &img2);
//  jComputeGaussDerivPyramid(img1, &pyr1, nlevels);
//  jComputeGaussDerivPyramid(img2, &pyr2, nlevels);

//  uchar*  imgA = img1.Begin();
//  uchar*  imgB = img2.Begin();
//  int     imgStep = img1.Width() * sizeof(unsigned char);
//  CvSize imgSize = cvSize( img1.Width(), img1.Height() );
  int     usePrevious = 0;
//  float*  velocityX = u.Begin();
//  float*  velocityY = v.Begin();
//  int     velStep = u.Width() * sizeof(float);
  float   lambda = 100.0f;
  CvTermCriteria criteria = cvTermCriteria( CV_TERMCRIT_ITER, 100, 0.5 );

  ImgIplImage imgA( img1 );
  ImgIplImage imgB( img2 );
  ImgIplImage velocityX( u );
  ImgIplImage velocityY( v );

  cvCalcOpticalFlowHS(
    imgA,
    imgB,
    usePrevious,
    velocityX,
    velocityY,
    lambda,
    criteria );
  velocityX.CastToFloat(&u);
  velocityY.CastToFloat(&v);
  fig.Draw(img1);
  figu.Draw(u);
  figv.Draw(v);
  figu.PlaceToTheRightOf(fig);
  figv.PlaceToTheRightOf(figu);
//  fig.GrabMouseClick();

//  assert( pyr1.NLevels() > 0 && pyr1.NLevels() == pyr2.NLevels() );
//  assert( IsSameSize(pyr1.image[0], pyr2.image[0]) );
//
//  const int nlevels = pyr1.NLevels();
//  const int max_niter = 10; //310000;
//
//  icvCalcOpticalFlowHS_8u32fR( uchar*  imgA,
//                             uchar*  imgB,
//                             int     imgStep,
//                             CvSize imgSize,
//                             int     usePrevious,
//                             float*  velocityX,
//                             float*  velocityY,
//                             int     velStep,
//                             float   lambda,
//                             CvTermCriteria criteria )
}

