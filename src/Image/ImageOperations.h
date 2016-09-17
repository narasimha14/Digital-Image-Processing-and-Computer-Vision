/* 
 * Copyright (c) 2004,2005,2006,2007 Clemson University.
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

#ifndef __BLEPO_IMAGEOPERATIONS_H__
#define __BLEPO_IMAGEOPERATIONS_H__

#include "Image.h"
#include "Utilities/PointSizeRect.h"
#include <afxwin.h>  // HDC
#include <vector>
#include "Matrix/Matrix.h"
#include "Utilities/Array.h"
//#include "ImageAlgorithms.h"  // KLT, FastFeatureTracker

/**
  This file contains standard operations on images of all types.

  @author Stan Birchfield (STB)
*/

namespace blepo
{

// min / max ('inplace' is allowed)
ImgFloat::Pixel Min(const ImgFloat& img);
ImgGray ::Pixel Min(const ImgGray & img);
ImgInt  ::Pixel Min(const ImgInt  & img);
ImgFloat::Pixel Max(const ImgFloat& img);
ImgGray ::Pixel Max(const ImgGray & img);
ImgInt  ::Pixel Max(const ImgInt  & img);
void Min(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out);
void Min(const ImgGray & img1, const ImgGray & img2, ImgGray * out);
void Min(const ImgInt  & img1, const ImgInt  & img2, ImgInt  * out);
void Max(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out);
void Max(const ImgGray & img1, const ImgGray & img2, ImgGray * out);
void Max(const ImgInt  & img1, const ImgInt  & img2, ImgInt  * out);
void MinMax(const ImgGray & img, ImgGray ::Pixel* minn, ImgGray ::Pixel* maxx);
void MinMax(const ImgInt  & img, ImgInt  ::Pixel* minn, ImgInt  ::Pixel* maxx);
void MinMax(const ImgFloat& img, ImgFloat::Pixel* minn, ImgFloat::Pixel* maxx);

/// bitwise logical operations ('inplace' is allowed)
void And(const ImgBgr   & img1, const ImgBgr   & img2, ImgBgr   * out);
void And(const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out);
void And(const ImgGray  & img1, const ImgGray  & img2, ImgGray  * out);
void And(const ImgInt   & img1, const ImgInt   & img2, ImgInt   * out);
void Or (const ImgBgr   & img1, const ImgBgr   & img2, ImgBgr   * out);
void Or (const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out);
void Or (const ImgGray  & img1, const ImgGray  & img2, ImgGray  * out);
void Or (const ImgInt   & img1, const ImgInt   & img2, ImgInt   * out);
void Xor(const ImgBgr   & img1, const ImgBgr   & img2, ImgBgr   * out);
void Xor(const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out);
void Xor(const ImgGray  & img1, const ImgGray  & img2, ImgGray  * out);
void Xor(const ImgInt   & img1, const ImgInt   & img2, ImgInt   * out);
void Not(const ImgBgr   & img, ImgBgr   * out);
void Not(const ImgBinary& img, ImgBinary* out);
void Not(const ImgGray  & img, ImgGray  * out);
void Not(const ImgInt   & img, ImgInt   * out);

void And(const ImgBgr & img, const ImgBinary& mask, ImgBgr * out);
void And(const ImgGray& img, const ImgBinary& mask, ImgGray* out);
void And(const ImgInt & img, const ImgBinary& mask, ImgInt * out);

/// bitwise logical operations with constant value ('inplace' is allowed)
void And(const ImgBgr   & img, ImgBgr   ::Pixel val, ImgBgr   * out);
void And(const ImgBinary& img, ImgBinary::Pixel val, ImgBinary* out);
void And(const ImgGray  & img, ImgGray  ::Pixel val, ImgGray  * out);
void And(const ImgInt   & img, ImgInt   ::Pixel val, ImgInt   * out);
void Or (const ImgBgr   & img, ImgBgr   ::Pixel val, ImgBgr   * out);
void Or (const ImgBinary& img, ImgBinary::Pixel val, ImgBinary* out);
void Or (const ImgGray  & img, ImgGray  ::Pixel val, ImgGray  * out);
void Or (const ImgInt   & img, ImgInt   ::Pixel val, ImgInt   * out);
void Xor(const ImgBgr   & img, ImgBgr   ::Pixel val, ImgBgr   * out);
void Xor(const ImgBinary& img, ImgBinary::Pixel val, ImgBinary* out);
void Xor(const ImgGray  & img, ImgGray  ::Pixel val, ImgGray  * out);
void Xor(const ImgInt   & img, ImgInt   ::Pixel val, ImgInt   * out);

// arithmetic operations ('inplace' allowed)
void AbsDiff(const ImgBgr  & img1, const ImgBgr  & img2, ImgBgr  * out);
void AbsDiff(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out);
void AbsDiff(const ImgGray & img1, const ImgGray & img2, ImgGray * out);
void AbsDiff(const ImgInt  & img1, const ImgInt  & img2, ImgInt  * out);
void Add(const ImgBgr  & img1, const ImgBgr  & img2, ImgBgr  * out);
void Add(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out);
void Add(const ImgGray & img1, const ImgGray & img2, ImgGray * out);
void Add(const ImgInt  & img1, const ImgInt  & img2, ImgInt  * out);
void Subtract(const ImgBgr  & img1, const ImgBgr  & img2, ImgBgr  * out);
void Subtract(const ImgGray & img1, const ImgGray & img2, ImgGray * out);
void Subtract(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out);
void Subtract(const ImgInt  & img1, const ImgInt  & img2, ImgInt  * out);
void Multiply(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out);
void Multiply(const ImgInt  & img1, const ImgInt  & img2, ImgInt  * out);
//void Multiply(const ImgInt& img1, const ImgInt& img2, ImgInt* out);
//void Divide(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out);
//void Divide(const ImgInt& img1, const ImgInt& img2, ImgInt* out);
// arithmetic operations with constant value ('inplace' allowed)
//void Add(const ImgBgr& img, ImgBgr::Pixel val, ImgBgr* out);
void Add(const ImgGray& img, ImgGray::Pixel val, ImgGray* out);
void Add(const ImgFloat& img, ImgFloat::Pixel val, ImgFloat* out);
void Add(const ImgInt& img, ImgInt::Pixel val, ImgInt* out);
//void Subtract(const ImgBgr& img, ImgBgr::Pixel val, ImgBgr* out);
void Subtract(const ImgGray& img, ImgGray::Pixel val, ImgGray* out);
void Subtract(const ImgFloat& img, ImgFloat::Pixel val, ImgFloat* out);
void Subtract(const ImgInt& img, ImgInt::Pixel val, ImgInt* out);
void Invert(const ImgBgr& img, ImgBgr* out);
void Invert(const ImgFloat& img, ImgFloat* out);
void Invert(const ImgGray& img, ImgGray* out);
void Multiply(const ImgGray & img, ImgGray ::Pixel val, ImgGray * out);
void Multiply(const ImgFloat& img, ImgFloat::Pixel val, ImgFloat* out);
void Multiply(const ImgInt  & img, ImgInt  ::Pixel val, ImgInt  * out);
void Divide  (const ImgFloat& img, ImgFloat::Pixel val, ImgFloat* out);
void Divide  (const ImgInt  & img, ImgInt  ::Pixel val, ImgInt  * out);
void Log10(const ImgFloat& img, ImgFloat* out);
void LinearlyScale(const ImgGray & img, ImgGray ::Pixel minval, ImgGray ::Pixel maxval, ImgGray * out);
void LinearlyScale(const ImgFloat& img, ImgFloat::Pixel minval, ImgFloat::Pixel maxval, ImgFloat* out);
void LinearlyScale(const ImgInt  & img, ImgInt  ::Pixel minval, ImgInt  ::Pixel maxval, ImgInt  * out);
void Clamp(const ImgGray & img, ImgGray ::Pixel minval, ImgGray ::Pixel maxval, ImgGray * out);
void Clamp(const ImgFloat& img, ImgFloat::Pixel minval, ImgFloat::Pixel maxval, ImgFloat* out);
void Clamp(const ImgInt  & img, ImgInt  ::Pixel minval, ImgInt  ::Pixel maxval, ImgInt  * out);

// binary morphological operations
// 3x3 means all ones in kernel
// (The ImgGray versions only look at the least significant bit of each pixel.
//  As a result, they work if the pixel values are {0,1} or {0,255}.)
// 'inplace' okay
void Erode3x3(const ImgBinary& img, ImgBinary* out);
void Erode3x3(const ImgGray& img, ImgGray* out);
void Erode3x3Cross(const ImgBinary& img, ImgBinary* out);
void Erode3x3Cross(const ImgGray& img, ImgGray* out);
void Dilate3x3(const ImgBinary& img, ImgBinary* out);
void Dilate3x3(const ImgGray& img, ImgGray* out);
void Dilate3x3Cross(const ImgBinary& img, ImgBinary* out);
void Dilate3x3Cross(const ImgGray& img, ImgGray* out);

// grayscale morphological operations
// (uses a 3x3 structuring element, where all values equal 'val', which is usually non-negative)
// Note:  These are not simply binary morphological operations using a grayscale image;
// instead, they are the grayscale extensions of the morphological operations.
void GrayscaleErode3x3(const ImgGray& img, int val, ImgGray* out);
void GrayscaleDilate3x3(const ImgGray& img, int val, ImgGray* out);

// thinning operation
void Thin3x3(ImgBinary* bin_img);

// find T-junctions in a binary edge image (assumes 1-pixel thick edges).
void FindJunctions(const ImgBinary& edges, Array<Point>* pts);

// sets all pixels to 'nonmax_val' unless they are local maxima
// (using 4 or 8 neighbors)
// 'inplace' okay
void NonmaxSuppress4(const ImgFloat& img, ImgFloat* out, ImgFloat::Pixel nonmax_val = 0);
void NonmaxSuppress8(const ImgFloat& img, ImgFloat* out, ImgFloat::Pixel nonmax_val = 0);

/*
Write compressed jpeg data to write_buffer and return number of bytes written.
Caller is responsible for allocating write_buffer of sufficient size 
(suggested allocation size is width*height*3, which is very conservative ).
'quality' is in range [0,100] inclusive (if outside range, then default value is used)
-Neeraj Kanhere
*/
size_t SaveJpegToMemory(const ImgBgr& img, BYTE* write_buffer, bool save_as_bgr = true, int quality = -1);

/*
Uncompress the jpeg data stored in jpeg_data buffer of length jpeg_data_length and 
write the output to a ImgBgr.
-Neeraj Kanhere
*/
void LoadJpegFromMemory(const BYTE* jpeg_data, size_t jpeg_data_length, ImgBgr* out);


// loading / saving an image
void Load(const CString& filename, ImgBgr* out, bool use_opencv=false);
void Load(const CString& filename, ImgGray* out);
void LoadImgInt(const CString& fname, ImgInt* out, bool binary = true);
void Save(const ImgBgr& img, const CString& filename, const char* filetype = NULL);
void Save(const ImgGray& img, const CString& filename, const char* filetype = NULL);
void Save(const ImgBinary& img, const CString& filename, const char* filetype = NULL);
//Save as .dep file for data
void SaveImgInt(const ImgInt& img, const CString& filename, bool binary = true);
void SaveAsText(const ImgFloat& img, const CString& filename, const char* fmt = "%10.4f ");

//typedef enum { ROUND, TRUNCATE } RoundMode;

//@name conversion between image types
//@{
// gray <=> bgr
void Convert(const ImgGray& img, ImgBgr* out);
void Convert(const ImgBgr& img, ImgGray* out);
// gray <=> binary
void Convert(const ImgGray& img, ImgBinary* out);
void Convert(const ImgBinary& img, ImgGray* out, ImgGray::Pixel val0, ImgGray::Pixel val1);
inline void Convert(const ImgBinary& img, ImgGray* out) { Convert(img, out, 0, 255); }
// gray <=> int
void Convert(const ImgGray& img, ImgInt* out);
void Convert(const ImgInt& img, ImgGray* out);
// gray <=> float
void Convert(const ImgGray& img, ImgFloat* out);
void Convert(const ImgFloat& img, ImgGray* out, bool linearly_scale = false);
// int <=> float
void Convert(const ImgInt& img, ImgFloat* out);
void Convert(const ImgFloat& img, ImgInt* out, bool linearly_scale = false);
// binary <=> bgr
void Convert(const ImgBinary& img, ImgBgr* out, const ImgBgr::Pixel& val0, const ImgBgr::Pixel& val1);
inline void Convert(const ImgBinary& img, ImgBgr* out) { Convert(img, out, Bgr(0,0,0), Bgr(255,255,255)); }
void Convert(const ImgBgr& img, ImgBinary* out);
// int <=> bgr
void Convert(const ImgInt& img, ImgBgr* out, Bgr::IntType format);
void Convert(const ImgBgr& img, ImgInt* out, Bgr::IntType format);
// float <=> bgr
void Convert(const ImgFloat& img, ImgBgr* out, bool linearly_scale = false);
void Convert(const ImgBgr& img, ImgFloat* out);
// binary <=> int
void Convert(const ImgBinary& img, ImgInt* out, ImgInt::Pixel val0, ImgInt::Pixel val1);
inline void Convert(const ImgBinary& img, ImgInt* out) { Convert(img, out, 0, 1); }
void Convert(const ImgInt& img, ImgBinary* out);
// binary <=> float
void Convert(const ImgBinary& img, ImgFloat* out, ImgFloat::Pixel val0, ImgFloat::Pixel val1);
inline void Convert(const ImgBinary& img, ImgFloat* out) { Convert(img, out, 0, 1); }
void Convert(const ImgFloat& img, ImgBinary* out);
//@}

// set all pixels to constant value
void Set(ImgBgr   * out, ImgBgr   ::Pixel val);
void Set(ImgBinary* out, ImgBinary::Pixel val);
void Set(ImgFloat * out, ImgFloat ::Pixel val);
void Set(ImgGray  * out, ImgGray  ::Pixel val);
void Set(ImgInt   * out, ImgInt   ::Pixel val);
// set contiguous list of pixels to constant value
void Set(ImgBgr   * out, ImgBgr   ::Pixel val, int x, int y, int n);
void Set(ImgBinary* out, ImgBinary::Pixel val, int x, int y, int n);
void Set(ImgFloat * out, ImgFloat ::Pixel val, int x, int y, int n);
void Set(ImgGray  * out, ImgGray  ::Pixel val, int x, int y, int n);
void Set(ImgInt   * out, ImgInt   ::Pixel val, int x, int y, int n);
// set all pixels inside 'rect' to constant value
void Set(ImgBgr   * out, const Rect& rect, ImgBgr   ::Pixel val);
void Set(ImgBinary* out, const Rect& rect, ImgBinary::Pixel val);
void Set(ImgFloat * out, const Rect& rect, ImgFloat ::Pixel val);
void Set(ImgGray  * out, const Rect& rect, ImgGray  ::Pixel val);
void Set(ImgInt   * out, const Rect& rect, ImgInt   ::Pixel val);
// set all pixels specified by mask to constant value
void Set(ImgBgr   * out, const ImgBinary& mask, ImgBgr   ::Pixel val);
void Set(ImgBinary* out, const ImgBinary& mask, ImgBinary::Pixel val);
void Set(ImgFloat * out, const ImgBinary& mask, ImgFloat ::Pixel val);
void Set(ImgGray  * out, const ImgBinary& mask, ImgGray  ::Pixel val);
void Set(ImgInt   * out, const ImgBinary& mask, ImgInt   ::Pixel val);
// set all pixels outside 'rect' to constant value
template <typename T> void SetOutside(Image<T>* out, const Rect& rect, typename Image<T>::Pixel val);
// set using another image:  copies entire 'img' to 'out' at location 'pt'
void Set(ImgBgr   * out, const Point& pt, const ImgBgr   & img);
void Set(ImgBinary* out, const Point& pt, const ImgBinary& img);
void Set(ImgFloat * out, const Point& pt, const ImgFloat & img);
void Set(ImgGray  * out, const Point& pt, const ImgGray  & img);
void Set(ImgInt   * out, const Point& pt, const ImgInt   & img);
// set using another image:  copies 'rect' of 'img' to 'out' at location 'pt'
void Set(ImgBgr   * out, const Point& pt, const ImgBgr   & img, const Rect& rect);
void Set(ImgBinary* out, const Point& pt, const ImgBinary& img, const Rect& rect);
void Set(ImgFloat * out, const Point& pt, const ImgFloat & img, const Rect& rect);
void Set(ImgGray  * out, const Point& pt, const ImgGray  & img, const Rect& rect);
void Set(ImgInt   * out, const Point& pt, const ImgInt   & img, const Rect& rect);
//set pixels having value old_value to new_value
void Set(ImgBgr   * out, const ImgBgr   ::Pixel old_value, const ImgBgr   ::Pixel new_value);
void Set(ImgBinary* out, const ImgBinary::Pixel old_value, const ImgBinary::Pixel new_value);
void Set(ImgFloat * out, const ImgFloat ::Pixel old_value, const ImgFloat ::Pixel new_value);
void Set(ImgGray  * out, const ImgGray  ::Pixel old_value, const ImgGray  ::Pixel new_value);
void Set(ImgInt   * out, const ImgInt   ::Pixel old_value, const ImgInt   ::Pixel new_value);

// extract 'rect' of pixels from 'img' to 'out'
void Extract(const ImgBgr   & img, const Rect& rect, ImgBgr   * out);
void Extract(const ImgBinary& img, const Rect& rect, ImgBinary* out);
void Extract(const ImgFloat & img, const Rect& rect, ImgFloat * out);
void Extract(const ImgGray  & img, const Rect& rect, ImgGray  * out);
void Extract(const ImgInt   & img, const Rect& rect, ImgInt   * out);

// extract 'rect' of pixels from 'img' to 'out'
// (xc,yc):  center of rectangle of size 2*hw+1 x 2*hh+1
// hw and hh are half-width and half-height
void Extract(const ImgBgr   & img, int xc, int yc, int hw, int hh, ImgBgr   * out);
void Extract(const ImgBinary& img, int xc, int yc, int hw, int hh, ImgBinary* out);
void Extract(const ImgFloat & img, int xc, int yc, int hw, int hh, ImgFloat * out);
void Extract(const ImgGray  & img, int xc, int yc, int hw, int hh, ImgGray  * out);
void Extract(const ImgInt   & img, int xc, int yc, int hw, int hh, ImgInt   * out);

// extract 'rect' of pixels from 'img' to 'out'
// (xc,yc):  center of rectangle of size hwl+hwr+1 x hwt+hwb+1
// hwl and hwr are the half-widths to the left and right of xc
// hht and hhb are the half-heights to the top and bottom of yc
// scale specifies zoom factor in original image
void Extract(const ImgBgr   & img, int xc, int yc, int hwl, int hwr, int hht, int hhb, double scale, ImgBgr   * out);
void Extract(const ImgBinary& img, int xc, int yc, int hwl, int hwr, int hht, int hhb, double scale, ImgBinary* out);
void Extract(const ImgFloat & img, int xc, int yc, int hwl, int hwr, int hht, int hhb, double scale, ImgFloat * out);
void Extract(const ImgGray  & img, int xc, int yc, int hwl, int hwr, int hht, int hhb, double scale, ImgGray  * out);
void Extract(const ImgInt   & img, int xc, int yc, int hwl, int hwr, int hht, int hhb, double scale, ImgInt   * out);

template <typename T, typename U>
inline bool IsSameSize(const Image<T>& img1, const Image<U>& img2)  { return img1.Width()==img2.Width() && img1.Height()==img2.Height(); }

bool IsIdentical(const ImgBgr& img1, const ImgBgr& img2);
bool IsIdentical(const ImgBinary& img1, const ImgBinary& img2);
bool IsIdentical(const ImgGray& img1, const ImgGray& img2);
bool IsIdentical(const ImgInt& img1, const ImgInt& img2);
bool IsIdentical(const ImgFloat& img1, const ImgFloat& img2);

void Equal(const ImgBgr& img1, const ImgBgr& img2, ImgBinary* out);
void Equal(const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out);
void Equal(const ImgFloat& img1, const ImgFloat& img2, ImgBinary* out);
void Equal(const ImgGray& img1, const ImgGray& img2, ImgBinary* out);
void Equal(const ImgInt& img1, const ImgInt& img2, ImgBinary* out);
void Equal(const ImgBgr& img1, const ImgBgr::Pixel& img2, ImgBinary* out);
void Equal(const ImgBinary& img1, const ImgBinary::Pixel& img2, ImgBinary* out);
void Equal(const ImgFloat& img1, const ImgFloat::Pixel& img2, ImgBinary* out);
void Equal(const ImgGray& img1, const ImgGray::Pixel& img2, ImgBinary* out);
void Equal(const ImgInt& img1, const ImgInt::Pixel& img2, ImgBinary* out);
void NotEqual(const ImgBgr& img1, const ImgBgr& img2, ImgBinary* out);
void NotEqual(const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out);
void NotEqual(const ImgFloat& img1, const ImgFloat& img2, ImgBinary* out);
void NotEqual(const ImgGray& img1, const ImgGray& img2, ImgBinary* out);
void NotEqual(const ImgInt& img1, const ImgInt& img2, ImgBinary* out);
void NotEqual(const ImgBgr& img1, const ImgBgr::Pixel& img2, ImgBinary* out);
void NotEqual(const ImgBinary& img1, const ImgBinary::Pixel& img2, ImgBinary* out);
void NotEqual(const ImgFloat& img1, const ImgFloat::Pixel& img2, ImgBinary* out);
void NotEqual(const ImgGray& img1, const ImgGray::Pixel& img2, ImgBinary* out);
void NotEqual(const ImgInt& img1, const ImgInt::Pixel& img2, ImgBinary* out);
void LessThan(const ImgFloat& img1, const ImgFloat& img2, ImgBinary* out);
void LessThan(const ImgGray& img1, const ImgGray& img2, ImgBinary* out);
void LessThan(const ImgInt& img1, const ImgInt& img2, ImgBinary* out);
void LessThan(const ImgFloat& img1, const ImgFloat::Pixel& img2, ImgBinary* out);
void LessThan(const ImgGray& img1, const ImgGray::Pixel& img2, ImgBinary* out);
void LessThan(const ImgInt& img1, const ImgInt::Pixel& img2, ImgBinary* out);
void GreaterThan(const ImgFloat& img1, const ImgFloat& img2, ImgBinary* out);
void GreaterThan(const ImgGray& img1, const ImgGray& img2, ImgBinary* out);
void GreaterThan(const ImgInt& img1, const ImgInt& img2, ImgBinary* out);
void GreaterThan(const ImgFloat& img1, const ImgFloat::Pixel& img2, ImgBinary* out);
void GreaterThan(const ImgGray& img1, const ImgGray::Pixel& img2, ImgBinary* out);
void GreaterThan(const ImgInt& img1, const ImgInt::Pixel& img2, ImgBinary* out);
void LessThanOrEqual(const ImgFloat& img1, const ImgFloat& img2, ImgBinary* out);
void LessThanOrEqual(const ImgGray& img1, const ImgGray& img2, ImgBinary* out);
void LessThanOrEqual(const ImgInt& img1, const ImgInt& img2, ImgBinary* out);
void LessThanOrEqual(const ImgFloat& img1, const ImgFloat::Pixel& img2, ImgBinary* out);
void LessThanOrEqual(const ImgGray& img1, const ImgGray::Pixel& img2, ImgBinary* out);
void LessThanOrEqual(const ImgInt& img1, const ImgInt::Pixel& img2, ImgBinary* out);
void GreaterThanOrEqual(const ImgFloat& img1, const ImgFloat& img2, ImgBinary* out);
void GreaterThanOrEqual(const ImgGray& img1, const ImgGray& img2, ImgBinary* out);
void GreaterThanOrEqual(const ImgInt& img1, const ImgInt& img2, ImgBinary* out);
void GreaterThanOrEqual(const ImgFloat& img1, const ImgFloat::Pixel& img2, ImgBinary* out);
void GreaterThanOrEqual(const ImgGray& img1, const ImgGray::Pixel& img2, ImgBinary* out);
void GreaterThanOrEqual(const ImgInt& img1, const ImgInt::Pixel& img2, ImgBinary* out);

// comparison with constant pixel value  (needed for Image<bool>)
//template<typename T> void Equal(const Image<T>& img, const typename Image<T>::Pixel& pix, ImgBinary* out);
//void Equal(const Image<bool>& img, const Image<bool>::Pixel& pix, ImgBinary* out);

/// thresholding:  values >= threshold are set to 1
/// For Bgr, the sum of the B, G, and R values are compared with the threshold
void Threshold(const ImgBgr&   img, int threshold,           ImgBinary* out);
void Threshold(const ImgGray&  img, unsigned char threshold, ImgBinary* out);
void Threshold(const ImgInt&   img, int threshold,           ImgBinary* out);
void Threshold(const ImgFloat& img, float threshold,         ImgBinary* out);

/// compute threshold using Ridler-Calvard iterative algorithm on graylevel histogram
double ComputeThreshold(const ImgGray&  img);

/// resample an image (currently just uses nearest neighbor)
void Resample(const ImgBgr&    img, int new_width, int new_height, ImgBgr* out);
void Resample(const ImgBinary& img, int new_width, int new_height, ImgBinary* out);
void Resample(const ImgFloat&  img, int new_width, int new_height, ImgFloat* out);
void Resample(const ImgGray&   img, int new_width, int new_height, ImgGray* out);
void Resample(const ImgInt&    img, int new_width, int new_height, ImgInt* out);
void Upsample(const ImgBinary& img, int factor_x,  int factor_y, ImgBinary* out);
void Upsample(const ImgBgr&    img, int factor_x,  int factor_y, ImgBgr* out);
void Upsample(const ImgFloat&  img, int factor_x,  int factor_y, ImgFloat* out);
void Upsample(const ImgGray&   img, int factor_x,  int factor_y, ImgGray* out);
void Upsample(const ImgInt&    img, int factor_x,  int factor_y, ImgInt* out);
void Downsample(const ImgBinary& img, int factor_x, int factor_y, ImgBinary* out);
void Downsample(const ImgBgr&    img, int factor_x, int factor_y, ImgBgr* out);
void Downsample(const ImgFloat&  img, int factor_x, int factor_y, ImgFloat* out);
void Downsample(const ImgGray&   img, int factor_x, int factor_y, ImgGray* out);
void Downsample(const ImgInt&    img, int factor_x, int factor_y, ImgInt* out);
void Downsample2x2(const ImgBinary& img, ImgBinary* out);
void Downsample2x2(const ImgBgr&    img, ImgBgr* out);
void Downsample2x2(const ImgFloat&  img, ImgFloat* out);
void Downsample2x2(const ImgGray&   img, ImgGray* out);
void Downsample2x2(const ImgInt&    img, ImgInt* out);

// bilinear interpolation at a single pixel
// if (x,y) is out of bounds, then the value of the nearest pixel is used
ImgBgr   ::Pixel Interp(const ImgBgr   & img, float x, float y);
ImgBinary::Pixel Interp(const ImgBinary& img, float x, float y);
ImgFloat ::Pixel Interp(const ImgFloat & img, float x, float y);
ImgGray  ::Pixel Interp(const ImgGray  & img, float x, float y);
ImgInt   ::Pixel Interp(const ImgInt   & img, float x, float y);

// bilinear interpolation in a rectangle
// (xc,yc) is center of window, whose size is 2*hw+1 x 2*hh+1
// hw is half-width, hh is half-height
void InterpRectCenter(const ImgBgr  & img, float xc, float yc, int hw, int hh, ImgBgr  * out);
void InterpRectCenter(const ImgFloat& img, float xc, float yc, int hw, int hh, ImgFloat* out);
void InterpRectCenter(const ImgGray & img, float xc, float yc, int hw, int hh, ImgGray * out);
void InterpRectCenter(const ImgInt  & img, float xc, float yc, int hw, int hh, ImgInt  * out);

// bilinear interpolation in an asymmetric rectangle
// (xc,yc) is center of window, whose size is (hwl+hwr+1) x (hht+hhb+1)
// hwl is half-width to the left
// hwr is half-width to the right
// hht is half-height to the top
// hhb is half-height to the bottom
void InterpRectCenterAsymmetric(const ImgBgr  & img, float xc, float yc, int hwl, int hwr, int hht, int hhb, ImgBgr  * out);
void InterpRectCenterAsymmetric(const ImgFloat& img, float xc, float yc, int hwl, int hwr, int hht, int hhb, ImgFloat* out);
void InterpRectCenterAsymmetric(const ImgGray & img, float xc, float yc, int hwl, int hwr, int hht, int hhb, ImgGray * out);
void InterpRectCenterAsymmetric(const ImgInt  & img, float xc, float yc, int hwl, int hwr, int hht, int hhb, ImgInt  * out);

// bilinear interpolation in a rectangle
// (xl,yt) is top-left of window, whose size is width x height
void InterpRect(const ImgBgr  & img, float xl, float yt, int width, int height, ImgBgr* out);
void InterpRect(const ImgFloat& img, float xl, float yt, int width, int height, ImgFloat* out);
void InterpRect(const ImgGray & img, float xl, float yt, int width, int height, ImgGray* out);
void InterpRect(const ImgInt  & img, float xl, float yt, int width, int height, ImgInt* out);

// create random image with uniform distribution
void Rand(int width, int height, ImgBgr* out);
void Rand(int width, int height, ImgBinary* out);
void Rand(int width, int height, ImgGray* out);
void Rand(int width, int height, ImgInt* out);

// create ramp image with slope of 1
void RampX(int width, int height, ImgFloat* out);
void RampY(int width, int height, ImgFloat* out);

// generate a random color
Bgr GetRandColor();

// generate a random saturated color (e.g., red, green, blue, ...), for displaying annotations on grayscale image
Bgr GetRandSaturatedColor();

// produce a pseudo-colored image by randomly assigning colors to values
void PseudoColor(const ImgInt& img, ImgBgr* out);

// returns whether the three color channels are identical for all pixels
bool IsGrayscale(const ImgBgr& img);

// drawing routines
void Draw(const ImgBgr   & img, HDC hdc, int x, int y);
void Draw(const ImgBgr   & img, CDC& dc, int x, int y);
void Draw(const ImgBinary& img, HDC hdc, int x, int y);
void Draw(const ImgBinary& img, CDC& dc, int x, int y);
void Draw(const ImgFloat & img, HDC hdc, int x, int y);
void Draw(const ImgFloat & img, CDC& dc, int x, int y);
void Draw(const ImgGray  & img, HDC hdc, int x, int y);
void Draw(const ImgGray  & img, CDC& dc, int x, int y);
void Draw(const ImgInt   & img, HDC hdc, int x, int y);
void Draw(const ImgInt   & img, CDC& dc, int x, int y);
void Draw(const ImgBgr   & img, HDC hdc, const Rect& src, const Rect& dst);
void Draw(const ImgBgr   & img, CDC& dc, const Rect& src, const Rect& dst);
void Draw(const ImgBinary& img, HDC hdc, const Rect& src, const Rect& dst);
void Draw(const ImgBinary& img, CDC& dc, const Rect& src, const Rect& dst);
void Draw(const ImgFloat & img, HDC hdc, const Rect& src, const Rect& dst);
void Draw(const ImgFloat & img, CDC& dc, const Rect& src, const Rect& dst);
void Draw(const ImgGray  & img, HDC hdc, const Rect& src, const Rect& dst);
void Draw(const ImgGray  & img, CDC& dc, const Rect& src, const Rect& dst);
void Draw(const ImgInt   & img, HDC hdc, const Rect& src, const Rect& dst);
void Draw(const ImgInt   & img, CDC& dc, const Rect& src, const Rect& dst);

/// @class TextDrawer
/// Draws text on an image by changing the pixel values.  Uses a vectorized Hershey font.
/// This class provides a simplified interface to the OpenCV text drawing functions but
/// does not include some of the more complex possibilities, such as
///   * non-uniform scaling in x and y directions
///   * slant
///   * italics
///   * other fonts (complex, triplex, script_complex, etc.)
class TextDrawer
{
public:
  /// Constructor initializes font
  /// 'height':  Height of font; approximately height of lower case letter 'o' in pixels
  /// 'thickness':  Thickness of stroke
  TextDrawer(int height=15, int thickness=2);

  ~TextDrawer();

  /// Returns the size, in pixels, of the bounding box enclosing the text
  Size GetTextSize(const char* text);

  /// Returns the bounding rectangle, in pixels, enclosing the text
  /// 'pt':  top-left corner of the rectangle
  Rect GetTextBoundingRect(const char* text, const Point& pt);

  /// Draws text on an image, replacing the pixels
  /// 'img':  both input and output
  /// 'pt':  top-left corner of region in which to draw text
  /// 'color':  color of text
  void DrawText(ImgBgr* img, const char* text, const Point& pt, const Bgr& color);

  /// Same as previous function, but first clears the bounding box of the text
  /// with a solid color.
  /// 'background_color':  The color with which to fill the background before drawing text.
  void DrawText(ImgBgr* img, const char* text, const Point& pt, const Bgr& color, const Bgr& background_color);

private:
  void* m_font;  ///< font (type of void* prevents header leak)
};

// statistics
int   Sum(const ImgBinary& img, const Rect& rect);
int   Sum(const ImgBinary& img, const ImgBinary& mask);
int   Sum(const ImgBinary& img);
int   Sum(const ImgGray& img, const Rect& rect);
int   Sum(const ImgGray& img, const ImgBinary& mask);
int   Sum(const ImgGray& img);
float Sum(const ImgFloat& img, const Rect& rect);
float Sum(const ImgFloat& img, const ImgBinary& mask);
float Sum(const ImgFloat& img);
int   Sum(const ImgInt& img, const Rect& rect);
int   Sum(const ImgInt& img, const ImgBinary& mask);
int   Sum(const ImgInt& img);
void  Sum(const ImgBgr& img, const Rect& rect, float* bsum, float* gsum, float* rsum);
void  Sum(const ImgBgr& img, const ImgBinary& mask, float* bsum, float* gsum, float* rsum);
void  Sum(const ImgBgr& img, float* bsum, float* gsum, float* rsum);
int   SumSquared(const ImgGray& img, const Rect& rect);
double SumSquared(const ImgFloat& img, const Rect& rect);
float Mean(const ImgGray& img, const Rect& rect);
float Mean(const ImgGray& img, const ImgBinary& mask);
float Mean(const ImgFloat& img, const Rect& rect);
float Mean(const ImgFloat& img, const ImgBinary& mask);
void  Mean(const ImgBgr& img, const Rect& rect, float* bmean, float* gmean, float* rmean);
void  Mean(const ImgBgr& img, const ImgBinary& mask, float* bmean, float* gmean, float* rmean);
Bgr   Mean(const ImgBgr& img, const Rect& rect);
Bgr   Mean(const ImgBgr& img, const ImgBinary& mask);
float Variance(const ImgGray& img, const Rect& rect);
float Variance(const ImgGray& img, const ImgBinary& mask);
double Variance(const ImgFloat& img, const Rect& rect);
double Variance(const ImgFloat& img, const ImgBinary& mask);
float StandardDeviation(const ImgGray& img, const Rect& rect);
float StandardDeviation(const ImgGray& img, const ImgBinary& mask);
float StandardDeviation(const ImgFloat& img, const Rect& rect);
float StandardDeviation(const ImgFloat& img, const ImgBinary& mask);

/// Draw rectangle on the image
void DrawDot(const Point& pt, ImgBgr  * out, const Bgr&    color, int size = 3, bool inside_image_check = false);  // draws a filled-in square
void DrawDot(const Point& pt, ImgFloat* out, float         color, int size = 3, bool inside_image_check = false);
void DrawDot(const Point& pt, ImgGray * out, unsigned char color, int size = 3, bool inside_image_check = false);
void DrawDot(const Point& pt, ImgInt  * out, int           color, int size = 3, bool inside_image_check = false);
void DrawLine(const Point& pt1, const Point& pt2, ImgBgr  * out, const Bgr&    color, int thickness=1);
void DrawLine(const Point& pt1, const Point& pt2, ImgFloat* out, float         color, int thickness=1);
void DrawLine(const Point& pt1, const Point& pt2, ImgGray * out, unsigned char color, int thickness=1);
void DrawLine(const Point& pt1, const Point& pt2, ImgInt  * out, int           color, int thickness=1);
void DrawLine(const Point& pt1, const Point& pt2, ImgBinary * out, int color, int thickness=1);
void DrawRect(const Rect& rect, ImgBgr  * out, const Bgr&    color, int thickness=1);
void DrawRect(const Rect& rect, ImgFloat* out, float         color, int thickness=1);
void DrawRect(const Rect& rect, ImgGray * out, unsigned char color, int thickness=1);
void DrawRect(const Rect& rect, ImgInt  * out, int           color, int thickness=1);
void DrawCircle(const Point& center, int radius, ImgBgr  * out, const Bgr&    color, int thickness=1);
void DrawCircle(const Point& center, int radius, ImgFloat* out, float         color, int thickness=1);
void DrawCircle(const Point& center, int radius, ImgGray * out, unsigned char color, int thickness=1);
void DrawCircle(const Point& center, int radius, ImgInt  * out, int           color, int thickness=1);
void DrawEllipse(const Point& center, int major_axis, int minor_axis, double angle, ImgBgr  * out, const Bgr&    color, int thickness=1);
void DrawEllipse(const Point& center, int major_axis, int minor_axis, double angle, ImgFloat* out, float         color, int thickness=1);
void DrawEllipse(const Point& center, int major_axis, int minor_axis, double angle, ImgGray * out, unsigned char color, int thickness=1);
void DrawEllipse(const Point& center, int major_axis, int minor_axis, double angle, ImgInt  * out, int           color, int thickness=1);
void DrawEllipticArc(const Point& center, int major_axis, int minor_axis, double angle, double start_angle, double end_angle, ImgBgr  * out, const Bgr&    color, int thickness=1);
void DrawEllipticArc(const Point& center, int major_axis, int minor_axis, double angle, double start_angle, double end_angle, ImgFloat* out, float         color, int thickness=1);
void DrawEllipticArc(const Point& center, int major_axis, int minor_axis, double angle, double start_angle, double end_angle, ImgGray * out, unsigned char color, int thickness=1);
void DrawEllipticArc(const Point& center, int major_axis, int minor_axis, double angle, double start_angle, double end_angle, ImgInt  * out, int           color, int thickness=1);

// 'inplace' okay
void FlipVertical  (const ImgBgr  & img, ImgBgr  * out);
void FlipVertical  (const ImgFloat& img, ImgFloat* out);
void FlipVertical  (const ImgGray & img, ImgGray * out);
void FlipVertical  (const ImgInt  & img, ImgInt  * out);
void FlipHorizontal(const ImgBgr  & img, ImgBgr  * out);
void FlipHorizontal(const ImgFloat& img, ImgFloat* out);
void FlipHorizontal(const ImgGray & img, ImgGray * out);
void FlipHorizontal(const ImgInt  & img, ImgInt  * out);
void Transpose     (const ImgFloat& img, ImgFloat* out);

// Finds locations of pixels with value = 'value' and stores it in a Point array
template <typename T>
void FindPixels(const Image<T>& img, typename Image<T>::Pixel value, std::vector<Point>* loc);

/* 
  These functions compute vertical and horizontal 1D Gaussian kernel.
  Kernel length is automatically determined by the formula length ~= 5 * sigma.
  Kernel length is guaranteed to be odd.
*/
void GaussVert (float sigma, ImgFloat* out);
void GaussHoriz(float sigma, ImgFloat* out);

/* 
  Functions for 2D Gaussian kernel. Similar to functions presented above.
  Horizontal and vertical kernels are returned separately.
*/
void Gauss(float sigma, ImgFloat* outx, ImgFloat* outy);

/* 
  These functions compute vertical and horizontal 1D derivative of Gaussian kernel.
*/
void GaussDerivVert (float sigma, ImgFloat* out);
void GaussDerivHoriz(float sigma, ImgFloat* out);

/* 
  Functions for 2D derivative of Gaussian kernel. Similar to functions 
  presented above. Horizontal and vertical kernels are returned separately.
*/
void GaussDeriv(float sigma, ImgFloat* outx, ImgFloat* outy);

/*
  Smooth an image by convolving with Gaussian kernel. 'sigma' is 
  standard deviation of the Gaussian.
*/
void Smooth(const ImgFloat& img, float sigma, ImgFloat* img_smoothed);


//**************************************************************************************//
/*
  Smooth an image using rank filtering (based on bin-sort).
  - Inputs: ImgGray image, window radius (rx, ry) and rank.
  - Output: Rank-filtered image (same size as input, border pixels set to 0).
  ** Inplace NOT ok

  -Rank should be an integer between 1 and total number of pixels in a window (2*rx+1)x(2*ry+1).
   For missing or negative value of the rank argument, median value is used (median filter).

  @author Neeraj Kanhere  nkanher@clemson.edu
  Sept 2007
*/
void RankFilter(const ImgGray& img, int rx, int ry, ImgGray* out, int rank=-1);
void RankFilter(const ImgBgr& img, int rx, int ry, ImgBgr* out, int rank=-1);
//**************************************************************************************//

//**************************************************************************************//
/*
  Smooth an image by convolving with specific hardcoded Gaussian kernel (for speed). 
  Kernels are either one-dimensional (horizontal or vertical) or two-dimensional
  (both horizontal and vertical, using separability of Gaussian).  Kernel values obtained
  from Pascal's triangle:
      ( 1/4 ) *   [1 2 1]     ( 3-element kernel, with sigma = 1 / sqrt(2) )
      ( 1/16) * [1 4 6 4 1]   ( 5-element kernel, with sigma = 1 )
  @author Yi Zhou  yiz@clemson.edu
  May 2007
*/
void SmoothGaussHoriz3(const ImgInt& img, ImgInt* img_smoothed);
void SmoothGaussVert3 (const ImgInt& img, ImgInt* img_smoothed);
void SmoothGaussHoriz5(const ImgInt& img, ImgInt* img_smoothed);
void SmoothGaussVert5 (const ImgInt& img, ImgInt* img_smoothed);
void SmoothGauss3x3   (const ImgInt& img, ImgInt* img_smoothed);
void SmoothGauss5x5   (const ImgInt& img, ImgInt* img_smoothed);
void SmoothGaussHoriz3(const ImgGray& img, ImgGray* img_smoothed);
void SmoothGaussVert3 (const ImgGray& img, ImgGray* img_smoothed);
void SmoothGaussHoriz5(const ImgGray& img, ImgGray* img_smoothed);
void SmoothGaussVert5 (const ImgGray& img, ImgGray* img_smoothed);
void SmoothGauss3x3   (const ImgGray& img, ImgGray* img_smoothed);
//**************************************************************************************//

// smooth by convolving with a 5x5 Gaussian
void SmoothGauss5x5(const ImgGray& img, ImgGray* out);

void SmoothGauss3x1WithBorders(const ImgFloat& img, ImgFloat* out);
void SmoothGauss1x3WithBorders(const ImgFloat& img, ImgFloat* out);
void SmoothGauss5x1WithBorders(const ImgFloat& img, ImgFloat* out);
void SmoothGauss1x5WithBorders(const ImgFloat& img, ImgFloat* out);

// smooth by convolving with a 3x3 box kernel (all ones, normalized)
//void SmoothBoxHoriz3(const ImgFloat& img, ImgFloat* out);
//void SmoothBoxVert3 (const ImgFloat& img, ImgFloat* out);
//void SmoothBox3x3   (const ImgFloat& img, ImgFloat* out);
void SmoothBox3x1WithBorders(const ImgFloat& img, ImgFloat* out);
void SmoothBox1x3WithBorders(const ImgFloat& img, ImgFloat* out);
void SmoothBox3x3WithBorders(const ImgFloat& img, ImgFloat* out, ImgFloat* work = NULL);

// sum by convolving with a 3x3 box kernel (all ones, NOT normalized)
void SumBox3x1WithBorders(const ImgFloat& img, ImgFloat* out);
void SumBox1x3WithBorders(const ImgFloat& img, ImgFloat* out);
void SumBox3x3WithBorders(const ImgFloat& img, ImgFloat* out, ImgFloat* work = NULL);

// smooth by convolving with a 5x5 box kernel (all ones, normalized by 16)
void ConvolveBox5x5 (const ImgGray& img, ImgGray* out);
// smooth by convolving with a NxN box kernel (all ones, normalized by NxN)
void ConvolveBoxNxN (const ImgGray& img, ImgGray* out, int winsize);

/*
  Computes horizontal and vertical gradients of an image using Gaussian and 
  derivative of Gaussian kernels. 'sigma' is standard deviation of the Gaussian, 
*/   
void Gradient(const ImgFloat& img, float sigma, ImgFloat* gradx, ImgFloat* grady);
void GradMag (const ImgFloat& img, float sigma, ImgFloat* gradmag, ImgFloat* gradphase = NULL);

// Smooths image by convolving with a 5x5 Gaussian,
// and simultaneously computes gradient using anisotropic kernel
//   (sigma = 1 in smoothing direction and sigma = 1.8 in differentiating direction).
// Fast algorithm that requires only two separable convolutions and some subtractions.
// Note:  Output gradx and grady will be 4 times the gradient (i.e., not normalized).
// 'tmp':  temporary workspace.
// This is still experimental.
void FastSmoothAndGradientApprox
(
  const ImgFloat& img, 
  ImgFloat* smoothed,
  ImgFloat* gradx,
  ImgFloat* grady,
  ImgFloat* tmp
);

// Smooths image by convolving with a Gaussian, and simultaneously 
// computes gradient by convolving with a Gaussian derivative
void SmoothAndGradient
(
  const ImgFloat& img, 
  float sigma,
  ImgFloat* smoothed,
  ImgFloat* gradx,
  ImgFloat* grady,
  ImgFloat* tmp
);

// Convolves image with 3x3 Gaussian directional derivatives,
//   using kernel [k1 k0 k1] * [-1 0 1]^T
// Uses separability and symmetry for speed.
// If 'smooth' is not NULL, then also computes the smoothed image
//   by convolving with 3x3 Gaussian.
void Gradient3x3
(
  const ImgFloat& img, 
  float k0,             // center weight in Gaussian kernel (times 0.5 to include the Gauss deriv normalization) 
  float k1,             // off-center weight in Gaussian kernel (times 0.5 to include the Gauss deriv normalization)
  ImgFloat* gradx,
  ImgFloat* grady,
  ImgFloat* smooth = NULL
);

// compute gradient using Sobel kernel (1/8) * [1 2 1] * [-1 0 1]^T (sigma^2 = 0.5)
void GradientSobel(const ImgFloat& img, ImgFloat* gradx, ImgFloat* grady, ImgFloat* smooth = NULL);

// compute gradient using Sharr kernel (1/32) * [3 10 3] * [-1 0 1]^T (sigma^2 = 1.1)
void GradientSharr(const ImgFloat& img, ImgFloat* gradx, ImgFloat* grady, ImgFloat* smooth = NULL);

void RectToMagPhase(const ImgFloat& gradx, const ImgFloat& grady,
                    ImgFloat* mag, ImgFloat* phase);
void MagPhaseToRect(const ImgFloat& mag, const ImgFloat& phase,
                    ImgFloat* gradx, ImgFloat* grady);

// compute gradient using Prewitt operator
void GradMagPrewitt(const ImgGray& img, ImgGray* out);
void GradMagSobel(const ImgGray& img, ImgGray* out);
void GradPrewittX(const ImgGray& img, ImgFloat* out);
void GradPrewittY(const ImgGray& img, ImgFloat* out);
void GradPrewitt(const ImgGray& img, ImgFloat* gradx, ImgFloat* grady);

// sets 'out' to 1 wherever a pixel's value is different from its neighbor
// above or to the left
void FindTransitionPixels(const ImgBinary& in, ImgBinary* out);
void FindTransitionPixels(const ImgBgr   & in, ImgBinary* out);
void FindTransitionPixels(const ImgFloat & in, ImgBinary* out);
void FindTransitionPixels(const ImgGray  & in, ImgBinary* out);
void FindTransitionPixels(const ImgInt   & in, ImgBinary* out);

//Cross-correlates an image with a kernel.
// 'normalized':  whether to use normalized cross correlation
enum CorrelateType { BPO_CORR_STANDARD, BPO_CORR_NORMALIZED, BPO_CORR_SSD };
void Correlate(const ImgInt& img, const ImgInt& kernel, ImgInt* out);
void Correlate(const ImgFloat& img, const ImgFloat& kernel, ImgFloat* out, CorrelateType type);

//Convolves an image with a kernel.
void Convolve(const ImgInt& img, const ImgInt& kernel, ImgInt* out);
void Convolve(const ImgFloat& img, const ImgFloat& kernel, ImgFloat* out);

// Enlarge image equally on all sides by extending values
void EnlargeByExtension(const ImgBgr   & img, int border, ImgBgr   * out);
void EnlargeByExtension(const ImgBinary& img, int border, ImgBinary* out);
void EnlargeByExtension(const ImgFloat & img, int border, ImgFloat * out);
void EnlargeByExtension(const ImgGray  & img, int border, ImgGray  * out);
void EnlargeByExtension(const ImgInt   & img, int border, ImgInt   * out);

// Enlarge image equally on all sides by reflecting values
void EnlargeByReflection(const ImgBgr   & img, int border, ImgBgr   * out);
void EnlargeByReflection(const ImgBinary& img, int border, ImgBinary* out);
void EnlargeByReflection(const ImgFloat & img, int border, ImgFloat * out);
void EnlargeByReflection(const ImgGray  & img, int border, ImgGray  * out);
void EnlargeByReflection(const ImgInt   & img, int border, ImgInt   * out);

// Reduce image equally on all sides (This is a weird name, but goes with the other Enlarge functions)
void EnlargeByCropping(const ImgBgr   & img, int border, ImgBgr   * out);
void EnlargeByCropping(const ImgBinary& img, int border, ImgBinary* out);
void EnlargeByCropping(const ImgFloat & img, int border, ImgFloat * out);
void EnlargeByCropping(const ImgGray  & img, int border, ImgGray  * out);
void EnlargeByCropping(const ImgInt   & img, int border, ImgInt   * out);


// Computes the Laplacian of Gaussian
void LaplacianOfGaussian(const ImgGray& img, ImgGray* out); // scales to fit in [0,255]
void LaplacianOfGaussian(const ImgGray& img, ImgInt* out);

// Computes the zero crossings of an image; call LaplacianOfGaussian first
void ZeroCrossings(const ImgGray& img, ImgBinary* out);
void ZeroCrossings(const ImgInt& img, ImgBinary* out);

// Computes the sign of the Laplacian of Gaussian
void Slog(const ImgGray& img, ImgBinary* out, int th = 10);

// For each byte (8 pixels) in a binary image, reverse the bits so that the
// most significant bit becomes the least significant bit, and so on.
// This is useful to ensure that pixels that are adjacent in the image are
// also adjacent in a register after a little-endian load.
// 'inplace' is okay.
void ReverseBits(const ImgBinary& img, ImgBinary* out);

// Reverses the bytes in a binary image so that the first byte becomes the last
// byte, the last byte becomes the first byte, and so on.
// 'inplace' is NOT okay.
void ReverseBytes(const ImgBinary& img, ImgBinary* out);

// Shift binary image by 'shift_amount' bits, filling extra row(s) or column(s) with zeros
void ShiftLeft (const ImgBinary& img, int shift_amount, ImgBinary* out);
void ShiftRight(const ImgBinary& img, int shift_amount, ImgBinary* out);
void ShiftUp   (const ImgBinary& img, int shift_amount, ImgBinary* out);
void ShiftDown (const ImgBinary& img, int shift_amount, ImgBinary* out);

// Cross-correlation between two images using slogs
void SlogCorrelate(const ImgGray& img1, const ImgGray& img2, 
                   ImgGray* dx, ImgGray* dy, ImgGray* confidence);

// Unoptimized code to test the Convolve routines.  Should produce exactly the same results.
//void ConvolveSlow(const ImgInt& img,const ImgInt& kernel,ImgInt* out);
//void ConvolveSlow(const ImgFloat& img,const ImgFloat& kernel,ImgFloat* out);

//void MedianFilter(const ImgGray& img, const int kernel_width, const int kernel_height, ImgGray* out);
//void MedianFilter(const ImgInt & img, const int kernel_width, const int kernel_height, ImgInt * out);

// Uses least-squares to compute the affine matrix that best fits the data:
//     minimizes |out * pt1 - pt2|^2
// 'out':  affine matrix with 3 columns, 2 rows
void AffineFit(const Array<Point2d>& pt1, const Array<Point2d>& pt2, MatDbl* out);

void AffineFit(const std::vector<Point2d>& pt1, const std::vector<Point2d>& pt2, MatDbl* out);

// Do weighted least squares.
void WeightedAffineFit(
  const Array<Point2d>& pt1, 
  const Array<Point2d>& pt2, 
  const Array<double>& weights,
  MatDbl* out);

// Uses least-squares to compute the translation & scale that best fits the data:
//     minimizes |zoom * pt1 + (dx,dy) - pt2|^2
void TransScaleFit(const std::vector<Point2d>& pt1, const std::vector<Point2d>& pt2, 
                   double* zoom, double* dx, double* dy);

// Uses normalized Direct Linear Transform (DLT) to compute the homography matrix that best fits the data.
// 'out':  homography matrix with 3 columns, 3 rows
void HomographyFit(const std::vector<Point2d>& pt1, const std::vector<Point2d>& pt2, MatDbl* out);
void HomographyFit(const std::vector<CPoint>& pt1, const std::vector<CPoint>& pt2, MatDbl* out);

// Do weighted least squares.
void WeightedAffineFit(
  const std::vector<Point2d>& pt1, 
  const std::vector<Point2d>& pt2, 
  const std::vector<double>& weights,
  MatDbl* out);

// Computes the affine matrix A (3 columns, 2 rows) so that
//
// A * [x] = [1 a] * [sx 0] * [cos(theta) -sin(theta)] * [x + dx1] + [dx2]
//     [y]   [b 1] * [0 sy]   [sin(theta)  cos(theta)]   [y + dy1]   [dy2]
//
// The meaning of the parameters is
//    - (dx1, dy1):  initial offset 
//    - theta:  counter-clockwise rotation
//    - sx, sy:  non-uniform scale
//    - a, b:  skew
//    - (dx2, dy2):  final offset
void ComputeAffine(double dx1, double dy1, double theta, double sx, double sy, double a, double b, double dx2, double dy2, MatDbl* out);

// Applies an affine warp to a set of points
void AffineMultiply(const MatDbl& aff, const Array<Point2d>& pt1, Array<Point2d>* pt2);

// From an affine matrix, computes warp coordinates for Warp().
// 'a':  3 columns, 2 rows
// 'width', 'height':  the desired size of 'fx' and 'fy'
//
// [ fx(i,j) ]       [ i ]
// [ fy(i,j) ] = a * [ j ]
//                   [ 1 ]  
// Example:
//   InitWarpAffine(a, &fx, &fy);
//   Warp(img, fx, fy, &out);
void InitWarpAffine(const MatDbl& a, int width, int height, ImgFloat* fx, ImgFloat* fy);

void InitWarpTransScale(double zoom, double dx, double dy, int width, int height, ImgFloat* fx, ImgFloat* fy);

// Compute warp coordinates that will produce an image of size
//    width x height
// with pixels centered at (xc,yc) in the original image.
// The scale factor 'zoom'
//     zoom=1.0:  leaves scale unchanged 
//     zoom>1.0:  zooms in on image
//     zoom<1.0:  zooms out
void InitWarpTransScaleCenter(double zoom, double xc, double yc, int width, int height, ImgFloat* fx, ImgFloat* fy);

// Compute warp coordinates that will produce an image of size
//    width x height
// with pixels rotated about (xc,yc) by an angle theta, then translated by (tx,ty)
// angle is in radians
void InitWarpTransRotCenter(double theta, double xc, double yc, double tx, double ty, int width, int height, ImgFloat* fx, ImgFloat* fy);

// From a homography matrix, computes warp coordinates for Warp().
// 'a':  3 columns, 3 rows
// 'width', 'height':  the desired size of 'fx' and 'fy'
//
// [ b(i,j) ]       [ i ]          [ fx(i,j) ] = [ b(i,j) / d(i,j) ]
// [ c(i,j) ] = a * [ j ]    =>    [ fy(i,j) ] = [ c(i,j) / d(i,j) ]
// [ d(i,j) ]       [ 1 ]  
// Example:
//   InitWarpHomography(a, &fx, &fy);
//   Warp(img, fx, fy, &out);
void InitWarpHomography(const MatDbl& a, int width, int height, ImgFloat* fx, ImgFloat* fy);

// Warp an image according to a precomputed pixel-wise function, stored in 'fx' and 'fy'.
// 'fx', and 'fy' should be the same size.
// 'out' will be same size as 'fx' and 'fy'
// (*out)(i,j) = img(fx(i,j), fy(i,j)), with bilinear interpolation
// inplace NOT okay
template <typename T>
void Warp(const Image<T>& img, const ImgFloat& fx, const ImgFloat& fy, Image<T>* out);

template <typename T>
inline void WarpAffine(const Image<T>& img, const MatDbl& a, Image<T>* out)
{
  ImgFloat fx, fy;
  InitWarpAffine(a, img.Width(), img.Height(), &fx, &fy);
  Warp(img, fx, fy, out);
}

template <typename T>
inline void WarpTransScale(const Image<T>& img, double zoom, double dx, double dy, Image<T>* out)
{
  ImgFloat fx, fy;
  InitWarpTransScale(zoom, dx, dy, img.Width(), img.Height(), &fx, &fy);
  Warp(img, fx, fy, out);
}

template <typename T>
inline void WarpTransScaleCenter(const Image<T>& img, double zoom, double xc, double yc, int width, int height, Image<T>* out)
{
  ImgFloat fx, fy;
  InitWarpTransScaleCenter(zoom, xc, yc, width, height, &fx, &fy);
  Warp(img, fx, fy, out);
}

template <typename T>
inline void WarpHomography(const Image<T>& img, const MatDbl& a, Image<T>* out)
{
  ImgFloat fx, fy;
  InitWarpHomography(a, img.Width(), img.Height(), &fx, &fy);
  Warp(img, fx, fy, out);
}

// Compute the integral image of an image.  Each pixel (X,Y) in the integral image
// is the sum of all pixels x <= X and y <= Y in the original image.
void ComputeIntegralImage(const ImgInt& img, ImgInt* integral_image);
void ComputeIntegralImage(const ImgBinary& img, ImgInt* integral_image);
void ComputeIntegralImage(const ImgGray& img, ImgInt* integral_image);

// Returns the sum of all the pixels in the image inside 'rect'
// (including left and top, but excluding right and bottom).
// The first parameter should be the result of ComputeIntegralImage.
int UseIntegralImage(const ImgInt& integral_image, const Rect& rect);

void LocalMaxima(const ImgInt& img, ImgBinary* out);

// Two-dimensional Fast Fourier Tranform (FFT) of an image
void ComputeFFT(const ImgFloat& in_img, ImgFloat* out_img_real, ImgFloat* out_img_imag);
void ComputeFFT(const ImgFloat& in_img_real,const ImgFloat& in_img_imag, ImgFloat* out_img_real, ImgFloat* out_img_imag);
void ComputeInverseFFT(const ImgFloat& in_img_real,const ImgFloat& in_img_imag, ImgFloat* out_img_real, ImgFloat* out_img_imag);

// Follows the boundary pixels of the first region encountered (in raster order) with 
// pixels having value 'label'.
void WallFollow(const ImgInt& in, int label, std::vector<Point> *chain);

// converts BGR to RGB (reverses order of red and blue bytes.  Note that in the 
// resulting image, B is actually red and R is blue.  'inplace' okay.
void BgrToRgb(const ImgBgr& img, ImgBgr* out);

// Bgr <--> Hsv conversion.  Bgr is in range [0,255], Hsv is in range [0,1].
void BgrToHsv(const ImgBgr& img, ImgFloat* h, ImgFloat* s, ImgFloat* v);
void HsvToBgr(const ImgFloat& h, const ImgFloat& s, const ImgFloat& v, ImgBgr* out);


void ExtractBgr(const ImgBgr& img, ImgGray* b, ImgGray* g, ImgGray* r);
void CombineBgr(const ImgGray& b, const ImgGray& g, const ImgGray& r, ImgBgr* img);

//template <typename T>
//struct PrintFormat
//{
//  public:
//    PrintFormat();
//    const char*() { return (const char*) m_fmt; }
//  private:
//    CString m_fmt;
//};
//
//template <typename T>

template <typename T>
void Print(const Image<T>& img, CString* out, const char* fmt = "%10.4f ")
{
  *out = "";
  const CString eol = "\r\n";
  CString tmp;
  for (int y=0 ; y<img.Height() ; y++)
  {
    for (int x=0 ; x<img.Width() ; x++)
    {
      tmp.Format(fmt, img(x,y));
      *out += tmp;
    }
    *out += eol;
  }
}

template <typename T>
void Display(const Image<T>& img)
{
  CString str;
  Print(img, &str);
  AfxMessageBox(str);
}

// Replaced by class in Capture directory.
//class CaptureImageSequence
//{
//  CString m_fname_fmt;
//  int m_index, m_start, m_end;
//
//public:
//  // fmt:  format string for image filename, e.g., "dir/img%04d.bmp"
//  // start:  first frame
//  // end:  last frame + 1
//  CaptureImageSequence(const char* fmt, int start, int end)
//    : m_fname_fmt(fmt), m_start(start), m_index(start), m_end(end) 
//  { assert(m_start < m_end); }
//
//  // returns the number of the image loaded, or -1 if failure
//  int GrabNextImage(ImgBgr* out)
//  {
//    if (m_index >= m_end)  return -1;
//    CString fname;
//    fname.Format(m_fname_fmt, m_index++);
//    Load(fname, out);
//    return m_index - 1;
//  }
//
//  // returns the number of the image loaded, or -1 if failure
//  int GrabNextImage(ImgGray* out)
//  {
//    if (m_index >= m_end)  return -1;
//    CString fname;
//    fname.Format(m_fname_fmt, m_index++);
//    Load(fname, out);
//    return m_index - 1;
//  }
//
//  int GetStart() const { return m_start; }
//  int GetEnd() const { return m_end; }
//  int GetNFrames() const { assert(m_start < m_end);  return m_end - m_start; }
//};

// Compute Delaunay triangulation
// Returns pairs of point coordinates, but loses the correspondence between edges and 
// the indices of the input points.
//   'points':  the list of points
//   'rect':  all points must lie within this rectangle
//   'out':  the edges represented as pairs of point coordinates
typedef std::pair<Point2d, Point2d> DelaunayPointPair;
void ComputeDelaunayOpencv(const std::vector<Point2d>& points, const Rect& rect, std::vector<DelaunayPointPair>* out);

// Compute Delaunay triangulation
// This is the preferred interface, because it retains the identity (index) of the
// points.
// Unfortunately, not yet implemented.
//typedef std::pair<int, int> DelaunayEdge;
//void ComputeDelaunay(std::vector<Point2d>& points, std::vector<DelaunayEdge>* out);

// Given corresponding points in two images, computes the fundamental matrix.
// The number of points in 'pt1' and 'pt2' should be the same.
// The fundamental matrix is defined as $pt1^T * F * pt2 = 0$
// Bug:  Current implementation does not normalize coordinates first before using them
void ComputeFundamentalMatrix(const std::vector<Point>& pt1, const std::vector<Point>& pt2, MatDbl* out);

// Given a point in image1 and the fundamental matrix $pt1^T * F * pt2 = 0$, computes the epipolar
// line in image2 as $pt1^T * F$.  The line is represented in homogeneous coordinates as ax + by + c = 0.
void ComputeEpipolarLine(double x1, double y1, const MatDbl& fundamental_matrix, double* a, double* b, double* c);



// Quantize an image. Number of levels = 2^(8-bit_shifts)
// For example calling function with bit_shifts = 5 would results in image quantized
// in 2^(8-5) = 8 levels
void QuantizeImage(unsigned short bit_shifts, ImgGray* img);
void QuantizeImage(unsigned short bit_shifts, ImgBgr* img);

bool RectInsideImage(const Rect& rect, int width, int height);
bool PointInsideImage(const blepo::Point& pt, int width, int height);

void ExtractImageFromEpsEncoding(const char* str, int width, int height, ImgBgr* out);
void ExtractImageFromEpsEncodedRawFile(const CString& fname, int width, int height, ImgBgr* out);

// inplace ok
// 0 <= p <= 1 is probability of corruption
void CorruptImageSaltNoise(const ImgGray& img, ImgGray* out, float p);

};  // end namespace blepo

#endif // __BLEPO_IMAGEOPERATIONS_H__
