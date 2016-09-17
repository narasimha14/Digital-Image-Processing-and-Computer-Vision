/* 
 * Copyright (c) 2008 Clemson University.
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
//#include "Utilities/Math.h"  // blepo_ex::Round
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

//  Calculates optical flow for two images by block matching method
void OpticalFlowBlockMatchOpencv(const ImgGray& img1, const ImgGray& img2,
                                 const CSize& block_size,
                                 const CSize& shift_size,
                                 const CSize& max_range,
                                 bool use_previous,
                                 ImgFloat* velx,
                                 ImgFloat* vely)
{
  assert( IsSameSize( img1, img2 ) );

  int vel_width = (int) (ceil(img1.Width() / (double) block_size.cx));
  int vel_height = (int) (ceil(img1.Height() / (double) block_size.cy));
  velx->Reset( vel_width, vel_height );
  vely->Reset( vel_width, vel_height );

  ImgIplImage pimg1( img1 );
  ImgIplImage pimg2( img2 );
  ImgIplImage pvelx( *velx );
  ImgIplImage pvely( *vely );
  cvCalcOpticalFlowBM( pimg1, pimg2, 
                       cvSize( block_size.cx, block_size.cy ),
                       cvSize( shift_size.cx, shift_size.cy ), 
                       cvSize( max_range.cx, max_range.cy ),
                       use_previous,
                       pvelx, pvely);
  pvelx.CastToFloat( velx );
  pvely.CastToFloat( vely );
}

// from OpenCV manual:
//
//void cvCalcOpticalFlowBM( const CvArr* prev, const CvArr* curr, CvSize block_size,
//                          CvSize shift_size, CvSize max_range, int use_previous,
//                          CvArr* velx, CvArr* vely );
//prev
//    First image, 8-bit, single-channel. 
//curr
//    Second image, 8-bit, single-channel. 
//block_size
//    Size of basic blocks that are compared. 
//shift_size
//    Block coordinate increments. 
//max_range
//    Size of the scanned neighborhood in pixels around block. 
//use_previous
//    Uses previous (input) velocity field. 
//velx
//    Horizontal component of the optical flow of
//    floor((prev->width - block_size.width)/shiftSize.width) × floor((prev->height - block_size.height)/shiftSize.height) size, 32-bit floating-point, single-channel. 
//vely
//    Vertical component of the optical flow of the same size velx, 32-bit floating-point, single-channel. 
//
//The function cvCalcOpticalFlowBM calculates optical flow for overlapped blocks block_size.width×block_size.height pixels each, thus the velocity fields are smaller than the original images. For every block in prev the functions tries to find a similar block in curr in some neighborhood of the original block or shifted by (velx(x0,y0),vely(x0,y0)) block as has been calculated by previous function call (if use_previous=1) 



};  // end namespace blepo

