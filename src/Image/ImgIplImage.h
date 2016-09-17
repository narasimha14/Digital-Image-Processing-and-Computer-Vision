/* 
 * Copyright (c) 2004,2005 Clemson University.
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

#ifndef __BLEPO_IMGIPLIMAGE_H__
#define __BLEPO_IMGIPLIMAGE_H__

#include "Image.h"

// old:
// uncomment this line to use Intel's IPL library (this is separate from OpenCV)
// #define BLEPO_PLEASE_I_WANT_TO_USE_IPL_FOR_TESTING

// ----------------------------- begin Intel IPL include stuff
#if defined(WIN32)
#include <afx.h>  // must be included before including "ipl.h", or else will get some strange error in "afxv_w32.h"; error also goes away if <stdafx.h> is included
#endif //defined(WIN32)
#ifdef BLEPO_PLEASE_I_WANT_TO_USE_IPL_FOR_TESTING
#include "../../../blepo_internal/external/IPLib/ipl.h"
#include "../../../blepo_internal/external/IPLib/iplError.h"  // not needed for most applications, but contains some error codes (like IPL_StsOK) that are occasionally checked
#endif // BLEPO_PLEASE_I_WANT_TO_USE_IPL_FOR_TESTING
#if defined(WIN32)
#include <afxwin.h>
#endif //defined(WIN32)
#include "blepo_opencv.h" // OpenCV
// ----------------------------- end Intel IPL include stuff

namespace blepo
{

/**
@class ImgIplImage

This class wraps an IplImage, which is the structure used in Intel's Image Processing
Library (IPL) and OpenCV.  The most common way of using this class is to instantiate it
from an existing image, then to pass it any IPL or OpenCV function.  Note that it makes
a complete copy of the image data, and it takes care of byte-alignment automatically.

@author Stan Birchfield (STB)
*/

//#ifndef BLEPO_PLEASE_I_WANT_TO_USE_IPL_FOR_TESTING

class ImgIplImage
{
public:
  enum ImageType { IMGIPL_BGR, IMGIPL_BINARY, IMGIPL_FLOAT, IMGIPL_GRAY, IMGIPL_INT };

  ImgIplImage() : m_ipl_img(NULL) {}
  ImgIplImage(const ImgBgr& img);
  ImgIplImage(const ImgBinary& img);
  ImgIplImage(const ImgFloat& img);
  ImgIplImage(const ImgGray& img);
  ImgIplImage(const ImgInt& img);
  ImgIplImage(int width, int height, ImageType type);
  ImgIplImage(const ImgIplImage& other);
  virtual ~ImgIplImage();

  ImgIplImage& operator=(const ImgIplImage& other);

  void Reset(const ImgBgr& img);
  void Reset(const ImgBinary& img);
  void Reset(const ImgFloat& img);
  void Reset(const ImgGray& img);
  void Reset(const ImgInt& img);
  void Reset(int width, int height, ImageType type);

  int Width()  const { return m_ipl_img ? m_ipl_img->width  : 0; }
  int Height() const { return m_ipl_img ? m_ipl_img->height : 0; }

  /// Get pointer to raw data, as unsigned char*
  //@{
  const unsigned char* BytePtr() const { return m_ipl_img ? reinterpret_cast<const unsigned char*>(m_ipl_img->imageData) : 0; }
  unsigned char* BytePtr()             { return m_ipl_img ? reinterpret_cast<unsigned char*>(m_ipl_img->imageData) : 0; }
  //@}

  /// Automatic conversion to Intel's IplImage struct
  /// This function is provided for convenience but be careful not 
  /// to call any IPL or OpenCV function that would
  /// render this class inconsistent, e.g., changing the width or
  /// height, origin location, number of channels, pixel type, etc.
  operator IplImage*() const { return m_ipl_img; }

  /// @name Functions to cast back into one of the basic image classes.
  ///       It is the caller's responsibility to ensure that the correct
  ///       function is called, otherwise the function will throw an
  ///       exception.
  //@{
  void CastToBgr(ImgBgr* out);
  void CastToBinary(ImgBinary* out);
  void CastToFloat(ImgFloat* out);
  void CastToGray(ImgGray* out);
  void CastToInt(ImgInt* out);
  //@}
  static void CastIplImgToBgr(IplImage* ipl, ImgBgr* out);
private:
  IplImage* m_ipl_img;  //< Intel IplImage structure
  ImageType m_type;
  void Reset(int width, int height, const unsigned char* data, ImageType type);
  void DestroyImage();
  void CheckBeforeCasting(ImageType type);
};

//#else // BLEPO_PLEASE_I_WANT_TO_USE_IPL_FOR_TESTING
//
//class ImgIplImage
//{
//public:
//  ImgIplImage() : m_ipl_img(NULL) {}
//  ImgIplImage(const ImgBgr& img, bool copy_data = true);
//  ImgIplImage(const ImgBinary& img, bool copy_data = true);
//  ImgIplImage(const ImgFloat& img, bool copy_data = true);
//  ImgIplImage(const ImgGray& img, bool copy_data = true);
//  ImgIplImage(const ImgInt& img, bool copy_data = true);
//  virtual ~ImgIplImage();
//
//  void Reset(const ImgBgr& img, bool copy_data = true);
//  void Reset(const ImgBinary& img, bool copy_data = true);
//  void Reset(const ImgFloat& img, bool copy_data = true);
//  void Reset(const ImgGray& img, bool copy_data = true);
//  void Reset(const ImgInt& img, bool copy_data = true);
//
//  /// Automatic conversion to Intel's IplImage struct
//  /// This function is provided for convenience but be careful not 
//  /// to call any IPL or OpenCV function that would
//  /// render this class inconsistent, e.g., changing the width or
//  /// height, origin location, number of channels, pixel type, etc.
//  operator IplImage*() const { return m_ipl_img; }
//
//  /// @name Functions to cast back into one of the basic image classes.
//  ///       It is the caller's responsibility to ensure that the correct
//  ///       function is called, otherwise the function will throw an
//  ///       exception.
//  //@{
//  void CastToBgr(ImgBgr* out);
//  void CastToBinary(ImgBinary* out);
//  void CastToFloat(ImgFloat* out);
//  void CastToGray(ImgGray* out);
//  void CastToInt(ImgInt* out);
//  //@}
//
//private:
//  void iCreateIplImg(int nchannels, unsigned depth, 
//                      const char* color_model,
//                      const char* channel_seq,
//                      int width, int height,
//                      void *data);
//  void iDestroyIplImg();
//  IplImage* m_ipl_img;  //< Intel IplImage structure
//  bool m_data_has_been_copied;
////  enum ImageType { IMGIPL_BGR, IMGIPL_BINARY, IMGIPL_FLOAT, IMGIPL_GRAY, IMGIPL_INT };
////  ImageType m_type;
////  void Reset(int width, int height, const unsigned char* data, int nbits_per_pixel, ImageType type);
////  void DestroyImage();
////  void CheckBeforeCasting(ImageType type);
//};
//
//#endif // BLEPO_PLEASE_I_WANT_TO_USE_IPL_FOR_TESTING


};  // end namespace blepo

#endif //__BLEPO_IMGIPLIMAGE_H__
