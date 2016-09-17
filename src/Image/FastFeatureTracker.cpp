/* 
 * Copyright (c) 2006 Clemson University
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
 *
 * Portions of this code were adapted from sample code in the OpenCV library.
 */

#include "Image.h"
#include "ImgIplImage.h"
#include "ImageAlgorithms.h"
#include "Utilities/Math.h"  // blepo_ex::Round
#include "blepo_opencv.h" // OpenCV

// This file contains a wrapper around OpenCV's face detector

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

// Copies only those features that are not lost from 'points' to 'out'.  
// 'out' is compact array of good features.  'ind' is array of indices,
// same size as 'out', so that ind[i] is index in 'points' for each element
// of 'out'.
void iFeatMakeDense(const FastFeatureTracker::FeatureArr& points, ArrInt* ind, Cvptarr* out)
{
  int npoints = points.Len();
  out->Reset( npoints );
  CvPoint2D32f* p = out->Begin();
  int n = 0;
  for (int i=0 ; i<npoints ; i++)  
  {
    const FastFeatureTracker::Feature& pt = points[i];
    if (pt.status != FastFeatureTracker::FEAT_LOST)  
    {
      p->x = pt.x;
      p->y = pt.y;
      p++;
      n++;
      ind->Push(i);
    }
  }
  out->Reset(n);
  assert(ind->Len() == n);
}

// Copies the compact array 'pts' back to feature array 'feat'.  Does not resize 'feat'.    
void iFeatCopyBack(FastFeatureTracker::FeatureArr* feat, const Cvptarr& pts, const Array<char>& status, const ArrInt& ind)
{
  assert(pts.Len() == status.Len() && pts.Len() == ind.Len());
  int npoints = pts.Len();
  for (int i=0 ; i<npoints ; i++)
  {
    const CvPoint2D32f& p = pts[i];
    int j = ind[i];
    FastFeatureTracker::Feature& q = (*feat)[j];
    q.x = p.x;
    q.y = p.y;
    q.status = (status[i]) ? FastFeatureTracker::FEAT_TRACKED : FastFeatureTracker::FEAT_LOST;
  }
}

void iSelectFeatures(const ImgIplImage& img, 
                     ImgIplImage* tmp1,
                     ImgIplImage* tmp2,
                     int max_npoints, 
                     const FastFeatureTracker::Params& params,
                     FastFeatureTracker::FeatureArr* points)
{
  if (max_npoints == 0)  { points->Reset();  return; }

  tmp1->Reset( img.Width(), img.Height(), ImgIplImage::IMGIPL_FLOAT );
  tmp2->Reset( img.Width(), img.Height(), ImgIplImage::IMGIPL_FLOAT );
  int count = max_npoints;
  Cvptarr pts(max_npoints);
  cvGoodFeaturesToTrack( img, *tmp1, *tmp2, pts.Begin(), &count,
                         params.quality, params.min_distance, 0, params.block_size, params.use_harris, params.k );
  if (params.refine_corners)
  {
    assert( params.window_hw >= 1 );
    assert( params.window_hh >= 1 );
    cvFindCornerSubPix( img, pts.Begin(), count,
                        cvSize( params.window_hw, params.window_hh ), cvSize(-1,-1),
                        cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 20, 0.03) );
  }

  points->Reset(count);
  for (int i=0 ; i<count ; i++)  (*points)[i] = FastFeatureTracker::Feature(pts[i].x, pts[i].y, FastFeatureTracker::FEAT_NEW);
}

void iTrackFeatures(const ImgIplImage& img1,
                    const ImgIplImage& img2,
                    ImgIplImage* pyr1, 
                    ImgIplImage* pyr2, 
                    FastFeatureTracker::FeatureArr* points, 
                    const FastFeatureTracker::Params& params)
{
  assert(img1.Width() == img2.Width() && img1.Height() == img2.Height());
  pyr1->Reset( img1.Width(), img1.Height(), ImgIplImage::IMGIPL_GRAY );
  pyr2->Reset( img2.Width(), img2.Height(), ImgIplImage::IMGIPL_GRAY );

  Cvptarr pts;
  ArrInt ind;
  iFeatMakeDense(*points, &ind, &pts);
  int tmp_check = points->Len();
  int npoints = pts.Len();
  Cvptarr pts_out(npoints);
  Array<char> status ( npoints );
  if (npoints > 0)
  {
    assert( params.window_hw >= 1 );
    assert( params.window_hh >= 1 );
    assert( params.npyramid_levels >= 1 );
    cvCalcOpticalFlowPyrLK( img1, img2, *pyr1, *pyr2,
                            pts.Begin(), pts_out.Begin(), npoints, 
                            cvSize(params.window_hw, params.window_hh), params.npyramid_levels-1, status.Begin(), 0,
                            cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,
                            20, 0.03), params.flags );
  }

  iFeatCopyBack(points, pts_out, status, ind);
  assert(points->Len() == tmp_check);
}

void iReplaceFeatures(FastFeatureTracker::FeatureArr* points,
                      const ImgIplImage& img,
                      ImgIplImage* tmp1,
                      ImgIplImage* tmp2,
                      const FastFeatureTracker::Params& params)
{
  FastFeatureTracker::FeatureArr tmp_pts;
  iSelectFeatures(img, tmp1, tmp2, points->Len(), params, &tmp_pts);
  assert( params.min_distance >= 0 );
  double mdsq = params.min_distance * params.min_distance;

  // This is a really slow way to enforce minimum distance
  for (int i=0 ; i<tmp_pts.Len() ; i++)
  {
    const FastFeatureTracker::Feature& p = tmp_pts[i];
    bool ok = true;
    for (int j=0 ; j<points->Len() ; j++)
    {
      const FastFeatureTracker::Feature& q = (*points)[i];
      if (q.status != FastFeatureTracker::FEAT_LOST)
      {
        double dx = p.x - q.x;
        double dy = p.y - q.y;
        if (dx * dx + dy * dy < mdsq)  { ok = false;  break; }
      }
    }
    if (ok)
    {
      for (int j=0 ; j<points->Len() ; j++)
      {
        FastFeatureTracker::Feature& q = (*points)[i];
        if (q.status == FastFeatureTracker::FEAT_LOST) { q = p;  break; }
      }
    }
  }
}

struct FfImages
{
  ImgIplImage m_grey, m_tmp1, m_tmp2;
  ImgIplImage m_img1, m_img2, m_pyr1, m_pyr2;
};

};
// ================< end local functions

// This macro makes it easier to access 'm_data', which is void* in the header file to prevent header leak.
#define FFIMG (static_cast<FfImages*>( m_data ))
 
namespace blepo
{

FastFeatureTracker::Params::Params()
  : quality ( 0.01 ), 
    min_distance ( 10 ), 
    k ( 0.04 ), 
    block_size ( 3 ), 
    use_harris ( false ), 
    refine_corners ( true ),
    window_hw ( 3 ),
    window_hh ( 3 ),
    flags ( 0 ),
    npyramid_levels ( 3 )
{
};

FastFeatureTracker::FastFeatureTracker()
{
  m_data = new FfImages();
}

FastFeatureTracker::~FastFeatureTracker()
{
  delete FFIMG;
}

void FastFeatureTracker::SelectFeatures(const ImgGray& img, int max_npoints, FeatureArr* points)
{
  FFIMG->m_img1.Reset( img );
  iSelectFeatures( FFIMG->m_img1, &FFIMG->m_tmp1, &FFIMG->m_tmp2, max_npoints, m_params, points );
}

void FastFeatureTracker::TrackFeatures(const ImgGray& img, FeatureArr* points, bool replace_lost_features)
{
  FFIMG->m_img2.Reset( img );
  iTrackFeatures( FFIMG->m_img1, FFIMG->m_img2, &FFIMG->m_pyr1, &FFIMG->m_pyr2, points, m_params );
  m_params.flags |= CV_LKFLOW_PYR_A_READY;
  FFIMG->m_img1 = FFIMG->m_img2;
  FFIMG->m_pyr1 = FFIMG->m_pyr2;

  if (replace_lost_features)
  {
    iReplaceFeatures(points, FFIMG->m_img2, &FFIMG->m_tmp1, &FFIMG->m_tmp2, m_params);
  }
}

void FastFeatureTracker::TrackFeatures(const ImgGray& img1, const ImgGray& img2, FeatureArr* points, bool replace_lost_features)
{
  FFIMG->m_img1.Reset( img1 );
  FFIMG->m_img2.Reset( img2 );
  m_params.flags &= ~CV_LKFLOW_PYR_A_READY;
  iTrackFeatures( FFIMG->m_img1, FFIMG->m_img2, &FFIMG->m_pyr1, &FFIMG->m_pyr2, points, m_params );
  FFIMG->m_img1 = FFIMG->m_img2;
  FFIMG->m_pyr1 = FFIMG->m_pyr2;

  if (replace_lost_features)
  {
    iReplaceFeatures(points, FFIMG->m_img2, &FFIMG->m_tmp1, &FFIMG->m_tmp2, m_params);
  }
}

void FastFeatureTracker::DrawFeatures(ImgBgr* img, const FeatureArr& points, const Bgr& color)
{
  for (int i=0 ; i<points.Len() ; i++)
  {
    const Feature& p = points[i];
    if (p.status != FEAT_LOST)
    {
      int x = blepo_ex::Round( p.x );
      int y = blepo_ex::Round( p.y );
      Point pt( x, y );
      DrawCircle(pt, 3, img, color, 1);
    }
  }
}
void FastFeatureTracker::SetParams(const Params& new_params)
{
  m_params = new_params;
}

FastFeatureTracker::Params FastFeatureTracker::GetParams()
{
  return m_params;
}

bool FastFeatureTracker::SaveFeatures(const FeatureArr& points, const CString& filename)
{
  FILE *fp = _wfopen(filename, L"wt");
  if (fp)
  {
    for (int i=0 ; i<points.Len() ; i++)
    {
      const Feature& p = points[i];
      fwprintf(fp, L"x = %f, y = %f, status = %f, index = %d\n", p.x, p.y, (double)p.status, i);
    }
    fclose(fp);
    return true;
  }
  else
  {
    return false;
  }
}
#undef FFIMG

};  // end namespace blepo

