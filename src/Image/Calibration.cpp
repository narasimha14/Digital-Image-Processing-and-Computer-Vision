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
 *
 * Portions of this code were adapted from sample code in the OpenCV library.
 */

#include "Image.h"
#include "ImgIplImage.h"
#include "ImageAlgorithms.h"
#include "blepo_opencv.h" // OpenCV

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

namespace blepo {

// Finds the corners of a chessboard viewed in the image.
// The coordinates of the corners are refined to subpixel accuracy.
//
// 'img':  Image
// 'grid_dims':  number of rows and columns of interior corners in the chess board
// 'pts':  output array of points
// Returns 1 if all points in grid found, 0 if only some or none were found.
bool FindChessboardCorners(const ImgGray& img, const Size& grid_dims, Cvptarr* pts)
{
  ImgIplImage iimg(img);
  pts->Reset(grid_dims.cx * grid_dims.cy);
  int corner_count;
  int ret = cvFindChessboardCorners(iimg, cvSize(grid_dims.cx, grid_dims.cy), pts->Begin(), &corner_count);
  pts->Delete( corner_count, -1 );
  if (corner_count > 0)
  {
    cvFindCornerSubPix( iimg, pts->Begin(), corner_count, cvSize(5, 5), cvSize(-1, -1),
                                 cvTermCriteria( CV_TERMCRIT_ITER, 5, 0 ) );
  }

  if (ret != 0)
  {
    assert(corner_count == pts->Len());
    assert(corner_count == grid_dims.cx * grid_dims.cy);
  }
  return ret != 0;
}

// Overlays the corners found by FindChessboardCorners() onto an image
void DrawChessboardCorners(ImgBgr* img, const Size& grid_dims, bool all_found, const Cvptarr& pts)
{
  ImgIplImage iimg(*img);
  cvDrawChessboardCorners( iimg, cvSize(grid_dims.cx, grid_dims.cy), const_cast<CvPoint2D32f*>(pts.Begin()), pts.Len(), all_found );
  iimg.CastToBgr(img);
}

// Transforms an array of image coordinates of chessboard corners to an array of calibration points,
// using the assumption of a rectangular chessboard.
// 'pts':  Image coordinates of chessboard corners in a single image
// 'grid_dims':  the number of corners horizontally and vertically in the calibration target
// 'cpts':  array of calibration points (image and object coordinates)
void TransformChessboardPoints(const Cvptarr& pts, const Size& grid_dims, CalibrationPointArr* cpts)
{
  const int square_size = 1;  // width of a square on the calibration target 
                              // (does not affect results of calibration anyway because of loss of scale in the imaging process, so set it to anything you want)

  // determine sizes
  const int n = pts.Len();  // number of features in this frame
  assert(n == grid_dims.cx * grid_dims.cy);

  cpts->Reset();

  int j = 0;
  for (int y=0 ; y<grid_dims.cy ; y++)
  {
    for (int x=0 ; x<grid_dims.cx ; x++)
    {
      CalibrationPoint p;

      // object coordinates
      p.x = square_size * x;
      p.y = square_size * y;
      p.z = 0;

      // image coordinates
      p.u = pts[j].x;
      p.v = pts[j].y;

      cpts->Push( p );

      j++;
    }
  }
}

void CalibrateCamera(const std::vector<CalibrationPointArr>& pts, const Size& img_size, CalibrationParams* out)
{
  int m = pts.size();  // number of images
  int ntot = 0;  // total number of points
  {
    for (int i=0 ; i<m ; i++)  ntot += pts[i].Len();
  }

  // create OpenCV matrices
  CvMat* object_points = cvCreateMat(ntot, 3, CV_32F);
  CvMat* image_points  = cvCreateMat(ntot, 2, CV_32F);
  CvMat* point_counts  = cvCreateMat(m, 1, CV_32S);
  CvSize image_size = cvSize( img_size.cx, img_size.cy );
  CvMat* intrinsic_matrix = cvCreateMat(3, 3, CV_32F);
  CvMat* distortion_coeffs = cvCreateMat(4, 1, CV_32F);

  // convert 'pts' to matrices

  {
    int k = 0;
    // for each image, ...
    for (int i=0 ; i<m ; i++)
    {
      const CalibrationPointArr& cpts = pts[i];  // calibration points in this image
      const int n = cpts.Len();

      // for each calibration point in this image, ...
      point_counts->data.i[i] = n; //, i, 0, n);
      for (int j=0 ; j<n ; j++)
      {
        const CalibrationPoint& p = cpts[j];
        cvmSet(object_points, k, 0, p.x);
        cvmSet(object_points, k, 1, p.y);
        cvmSet(object_points, k, 2, p.z);
        cvmSet(image_points, k, 0, p.u);
        cvmSet(image_points, k, 1, p.v);
        k++;
      }
    }
  }

  // calibrate camera
  cvCalibrateCamera2( object_points,
                      image_points,
                      point_counts,
                      image_size,
                      intrinsic_matrix,
                      distortion_coeffs);

  // convert results back to 'out' format
  {
    MatDbl& im = out->intrinsic_matrix;
    im.Reset(3, 3);
    for (int r=0 ; r<3 ; r++)
    for (int c=0 ; c<3 ; c++)
    {
      im(c, r) = cvmGet(intrinsic_matrix, r, c);
    }
  }
  {
    Array<double>& dc = out->distortion_coeffs;
    dc.Reset(4);
    for (int i=0 ; i<4 ; i++)
    {
      dc(i) = cvmGet(distortion_coeffs, i, 0);
    }
  }
}

};  // end namespace blepo

