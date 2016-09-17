/* 
 * Copyright (c) 2004, 2005 Clemson University.
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
#include <limits.h>  // INT_MIN, INT_MAX
#include <float.h>  // FLT_MAX

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------


namespace blepo
{

// ImgBgr
const int ImgBgr::NBITS_PER_PIXEL = 24;
const int ImgBgr::NCHANNELS = 3;
const ImgBgr::Pixel ImgBgr::MIN_VAL = Bgr(0,0,0);
const ImgBgr::Pixel ImgBgr::MAX_VAL = Bgr(0xFF, 0xFF, 0xFF);

// ImgFloat
const int ImgFloat::NBITS_PER_PIXEL = 32;
const int ImgFloat::NCHANNELS = 1;
const ImgFloat::Pixel ImgFloat::MIN_VAL = -FLT_MAX;
const ImgFloat::Pixel ImgFloat::MAX_VAL =  FLT_MAX;  // = 3.402823466e+38F

// ImgGray
const int ImgGray::NBITS_PER_PIXEL = 8;
const int ImgGray::NCHANNELS = 1;
const ImgGray::Pixel ImgGray::MIN_VAL = 0;
const ImgGray::Pixel ImgGray::MAX_VAL = 255;

// ImgInt
const int ImgInt::NBITS_PER_PIXEL = 32;
const int ImgInt::NCHANNELS = 1;
const ImgInt::Pixel ImgInt::MIN_VAL = INT_MIN;  // = -2147483648
const ImgInt::Pixel ImgInt::MAX_VAL = INT_MAX;  // =  2147483647

// ImgBinary
const int ImgBinary::NBITS_PER_PIXEL = 1;
const int ImgBinary::NCHANNELS = 1;
const ImgBinary::Pixel ImgBinary::MIN_VAL = 0;
const ImgBinary::Pixel ImgBinary::MAX_VAL = 1;

Bgr Bgr::BLUE    (255,   0,   0);
Bgr Bgr::GREEN   (  0, 255,   0);
Bgr Bgr::RED     (  0,   0, 255);
Bgr Bgr::YELLOW  (  0, 255, 255);
Bgr Bgr::CYAN    (255, 255,   0);
Bgr Bgr::MAGENTA (255,   0, 255);
Bgr Bgr::WHITE   (255, 255, 255);
Bgr Bgr::BLACK   (  0,   0,   0);

};  // end namespace blepo

