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

#include "ImgIplImage.h"
#include "Image.h"
#include "Utilities/Exception.h"

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

//#ifndef BLEPO_PLEASE_I_WANT_TO_USE_IPL_FOR_TESTING

// ================> begin local functions (available only to this translation unit)
namespace
{
using namespace blepo;

// copy data, pixel-by-pixel, from a packed binary (no alignment)
// to an IplImage, using the ImgBinary iterator for convenience
// Could be made faster in the case of an already-aligned image
void iCopyUnalignedBinaryData(int width, int height, 
                             const unsigned char* data,
                             IplImage* ipl)
{
  for (int y=0; y<height ; y++)
  {
    ImgBinary::ConstIterator p(y*width, data);
    char* q = ipl->imageData + y * ipl->widthStep;
    unsigned char byte = 0;
    unsigned char bitmask = 0x80;
    for (int x=0 ; x<width ; x++)
    {
      bool val = *p++;
      if (val)  byte |= bitmask;
      bitmask >>= 1;
      if (bitmask==0)
      {
        *q++ = byte;
        byte = 0;
        bitmask = 0x80;
      }
    }
    if (bitmask!=0x80)  *q++ = byte;
  }
}
  
// copy data from a packed image (no alignment) to an IplImage
void iCopyData(int width, int height, 
               const unsigned char* data,
               int nbits_per_pixel,
               IplImage* ipl)
{
  const int nbytes_per_row = width * nbits_per_pixel / 8;
  int width_step = ipl->widthStep;
  const unsigned char* p = data;
  char* q = ipl->imageData;

  if (nbytes_per_row == width_step)
  {
    // there are no alignment pixels, so just copy it all directly
    memcpy(q, p, nbytes_per_row * height);
  }
  else if (ipl->depth == IPL_DEPTH_1U && width % 8 != 0)
  {
    // special test for binary images:
    // not only are there alignment pixels, but the first pixel of each
    // row does not begin a new byte, so do a special bit-by-bit copy
    iCopyUnalignedBinaryData(width, height, data, ipl);
  }
  else
  {
    // copy each row separately to make room for the alignment pixels
    for (int y=0; y<height ; y++)
    {
      memcpy(q, p, nbytes_per_row);
      p += nbytes_per_row;
      q += width_step;
    }
  }
}

// copy data, pixel-by-pixel, from an IplImage to a packed binary 
// (no alignment), using the ImgBinary iterator for convenience.
// Could be made faster in the case of an already-aligned image
void iCopyUnalignedBinaryData(IplImage* ipl,
                              int width, int height, 
                              unsigned char* data)
{
  const char* q = ipl->imageData;
  ImgBinary::Iterator p(0, data); //out->Begin();
  for (int y=0; y<height ; y++)
  {
    const char* qq = q;
    int x = 0;
    while (1)
    {
      unsigned char byte = *qq++;
      unsigned char bitmask = 0x80;
      for (int i=0 ; i<8 ; i++)
      {
        *p++ = ((byte & bitmask) != 0);
        bitmask >>= 1;
        x++;
        if (x >= width)  goto end_of_row;
      }
    }
end_of_row:
    q += ipl->widthStep;
  }
}
  
// copy data from an IplImage to a packed image (no alignment)
void iCopyData(IplImage* ipl,
               int width, int height, 
               int nbits_per_pixel,
               unsigned char* data)
{
  const int nbytes_per_row = width * nbits_per_pixel / 8;
  int width_step = ipl->widthStep;
  unsigned char* p = data;
  const char* q = ipl->imageData;

  if (nbytes_per_row == width_step)
  {
    // there are no alignment pixels, so just copy it all directly
    memcpy(p, q, nbytes_per_row * height);
  }
  else if (ipl->depth == IPL_DEPTH_1U && width % 8 != 0)
  {
    // special test for binary images:
    // not only are there alignment pixels, but the first pixel of each
    // row does not begin a new byte, so do a special bit-by-bit copy
    iCopyUnalignedBinaryData(ipl, width, height, data);
  }
  else
  {
    // copy each row separately to make room for the alignment pixels
    for (int y=0; y<height ; y++)
    {
      memcpy(p, q, nbytes_per_row);
      p += nbytes_per_row;
      q += width_step;
    }
  }
}

int iGetNBitsPerPixel(ImgIplImage::ImageType type)
{
  switch (type)
  {
  case ImgIplImage::IMGIPL_BGR:    return ImgBgr   ::NBITS_PER_PIXEL;
  case ImgIplImage::IMGIPL_BINARY: return ImgBinary::NBITS_PER_PIXEL;
  case ImgIplImage::IMGIPL_GRAY:   return ImgGray  ::NBITS_PER_PIXEL;
  case ImgIplImage::IMGIPL_FLOAT:  return ImgFloat ::NBITS_PER_PIXEL;
  case ImgIplImage::IMGIPL_INT:    return ImgInt   ::NBITS_PER_PIXEL;
  default:  assert(0);             return 0;
  }
}

int iGetDepth(ImgIplImage::ImageType type)
{
  switch (type)
  {
  case ImgIplImage::IMGIPL_BGR:    return IPL_DEPTH_8U;
  case ImgIplImage::IMGIPL_BINARY: return IPL_DEPTH_1U;
  case ImgIplImage::IMGIPL_GRAY:   return IPL_DEPTH_8U;
  case ImgIplImage::IMGIPL_FLOAT:  return IPL_DEPTH_32F;
  case ImgIplImage::IMGIPL_INT:    return IPL_DEPTH_32S;
  default:  assert(0);  return 0;
  }
}

int iGetNChannels(ImgIplImage::ImageType type)
{
  switch (type)
  {
  case ImgIplImage::IMGIPL_BGR:    return 3;
  case ImgIplImage::IMGIPL_BINARY: return 1;
  case ImgIplImage::IMGIPL_GRAY:   return 1;
  case ImgIplImage::IMGIPL_FLOAT:  return 1;
  case ImgIplImage::IMGIPL_INT:    return 1;
  default:  assert(0);  return 0;
  }
}

};
// ================< end local functions


namespace blepo
{

void ImgIplImage::Reset(const ImgBgr& img)
{
  Reset(img.Width(), img.Height(), img.BytePtr(), IMGIPL_BGR);
}

void ImgIplImage::Reset(const ImgBinary& img)
{
  Reset(img.Width(), img.Height(), img.BytePtr(), IMGIPL_BINARY);
}

void ImgIplImage::Reset(const ImgFloat& img)
{
  Reset(img.Width(), img.Height(), img.BytePtr(), IMGIPL_FLOAT);
}

void ImgIplImage::Reset(const ImgGray& img)
{
  Reset(img.Width(), img.Height(), img.BytePtr(), IMGIPL_GRAY);
}

void ImgIplImage::Reset(const ImgInt& img)
{
  Reset(img.Width(), img.Height(), img.BytePtr(), IMGIPL_INT);
}

void ImgIplImage::Reset(int width, int height, ImageType type)
{
  if (type == m_type && m_ipl_img && width == m_ipl_img->width && height == m_ipl_img->height)
    return;  // already initialized
  
  DestroyImage();
  CvSize sz = cvSize(width, height);
  m_ipl_img = cvCreateImage( sz, iGetDepth(type), iGetNChannels(type) );
  m_type = type;

  if (type == IMGIPL_BGR)
  {
    assert(strncmp(m_ipl_img->channelSeq, "BGR", 3) == 0);
  }
}

void ImgIplImage::Reset(int width, int height, const unsigned char* data, ImageType type)
{
  Reset(width, height, type);
  iCopyData(width, height, data, iGetNBitsPerPixel(type), m_ipl_img);
}

ImgIplImage::ImgIplImage(const ImgBgr& img)    : m_ipl_img(NULL) { Reset(img); }
ImgIplImage::ImgIplImage(const ImgBinary& img) : m_ipl_img(NULL) { Reset(img); }
ImgIplImage::ImgIplImage(const ImgFloat& img)  : m_ipl_img(NULL) { Reset(img); }
ImgIplImage::ImgIplImage(const ImgGray& img)   : m_ipl_img(NULL) { Reset(img); }
ImgIplImage::ImgIplImage(const ImgInt& img)    : m_ipl_img(NULL) { Reset(img); }
ImgIplImage::ImgIplImage(int width, int height, ImageType type) { Reset(width, height, type); }

ImgIplImage::ImgIplImage(const ImgIplImage& other)
{
  *this = other;
}

ImgIplImage& ImgIplImage::operator=(const ImgIplImage& other)
{
  Reset(other.Width(), other.Height(), other.BytePtr(), other.m_type);
  return *this;
}

ImgIplImage::~ImgIplImage()
{
  DestroyImage();
}

void ImgIplImage::DestroyImage()
{
  if (m_ipl_img)  cvReleaseImage(&m_ipl_img);
}

void ImgIplImage::CheckBeforeCasting(ImageType type)
{
  if (m_ipl_img == NULL)  BLEPO_ERROR("Cannot cast from a NULL IPL image");
  if (m_type != type)  BLEPO_ERROR("Cannot cast IPL image to a different type");
  // could also check 'm_ipl_img' here to make sure it's consistent with type,
  // as a redundant check
}

void ImgIplImage::CastToBgr(ImgBgr* out)
{
  CheckBeforeCasting(IMGIPL_BGR);
  out->Reset(m_ipl_img->width, m_ipl_img->height);
  iCopyData(m_ipl_img, out->Width(), out->Height(), out->NBITS_PER_PIXEL, out->BytePtr());
}

void ImgIplImage::CastToBinary(ImgBinary* out)
{
  CheckBeforeCasting(IMGIPL_BINARY);
  out->Reset(m_ipl_img->width, m_ipl_img->height);
  iCopyData(m_ipl_img, out->Width(), out->Height(), out->NBITS_PER_PIXEL, out->BytePtr());
}

void ImgIplImage::CastToFloat(ImgFloat* out)
{
  CheckBeforeCasting(IMGIPL_FLOAT);
  out->Reset(m_ipl_img->width, m_ipl_img->height);
  iCopyData(m_ipl_img, out->Width(), out->Height(), out->NBITS_PER_PIXEL, out->BytePtr());
}

void ImgIplImage::CastToGray(ImgGray* out)
{
  CheckBeforeCasting(IMGIPL_GRAY);
  out->Reset(m_ipl_img->width, m_ipl_img->height);
  iCopyData(m_ipl_img, out->Width(), out->Height(), out->NBITS_PER_PIXEL, out->BytePtr());
}

void ImgIplImage::CastToInt(ImgInt* out)
{
  CheckBeforeCasting(IMGIPL_INT);
  out->Reset(m_ipl_img->width, m_ipl_img->height);
  iCopyData(m_ipl_img, out->Width(), out->Height(), out->NBITS_PER_PIXEL, out->BytePtr());
}


void ImgIplImage::CastIplImgToBgr(IplImage* ipl, ImgBgr* out)
{
  out->Reset(ipl->width, ipl->height);
  iCopyData(ipl, out->Width(), out->Height(), out->NBITS_PER_PIXEL, out->BytePtr());  
}
};  // end namespace blepo

//#else // BLEPO_PLEASE_I_WANT_TO_USE_IPL_FOR_TESTING
//
//namespace blepo
//{
//
//// IPL code that works just fine but requires IPL library
//
//ImgIplImage::ImgIplImage(const ImgBgr& img, bool copy_data)
//  : m_ipl_img(NULL), m_data_has_been_copied(false) 
//{
//  Reset(img, copy_data);
//}
//
//ImgIplImage::ImgIplImage(const ImgBinary& img, bool copy_data)
//  : m_ipl_img(NULL), m_data_has_been_copied(false) 
//{
//  Reset(img, copy_data);
//}
//
//ImgIplImage::ImgIplImage(const ImgFloat& img, bool copy_data)
//  : m_ipl_img(NULL), m_data_has_been_copied(false) 
//{
//  Reset(img, copy_data);
//}
//
//ImgIplImage::ImgIplImage(const ImgGray& img, bool copy_data)
//  : m_ipl_img(NULL), m_data_has_been_copied(false) 
//{
//  Reset(img, copy_data);
//}
//
//ImgIplImage::ImgIplImage(const ImgInt& img, bool copy_data)
//  : m_ipl_img(NULL), m_data_has_been_copied(false) 
//{
//  Reset(img, copy_data);
//}
//
//void ImgIplImage::Reset(const ImgBgr& img, bool copy_data)
//{
//  if (copy_data)
//  {
//    iCreateIplImg(3, IPL_DEPTH_8U, "RGB", "BGR",     
//                  img.Width(), img.Height(), NULL);
//    char* po = m_ipl_img->imageData;
//    for (int y=0; y<img.Height() ; y++)
//    {
//      const unsigned char* pi = reinterpret_cast<const unsigned char*>(img.Begin(0, y));
//      memcpy(po, pi, img.Width()*img.NBITS_PER_PIXEL/8);
//      po += m_ipl_img->widthStep;
//    }
//  }
//  else 
//  {
//    if ((img.Width()*img.NBITS_PER_PIXEL) % 64 != 0)  BLEPO_ERROR("Image is not quad-word aligned");
//    iCreateIplImg(3, IPL_DEPTH_8U, "RGB", "BGR",     
//                  img.Width(), img.Height(), const_cast<void*>(static_cast<const void*>(img.Begin())));
//  }
//  m_data_has_been_copied = copy_data;
//}
//
//void ImgIplImage::Reset(const ImgBinary& img, bool copy_data)
//{
//  if (copy_data)
//  {
//    iCreateIplImg(1, IPL_DEPTH_1U, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), NULL);
//    char* po = m_ipl_img->imageData;
//    for (int y=0; y<img.Height() ; y++)
//    {
//      ImgBinary::ConstIterator it = img.Begin(0, y);
//      char* poo = po;
//      unsigned char byte = 0;
//      unsigned char bitmask = 0x80;
//      for (int x=0 ; x<img.Width() ; x++)
//      {
//        bool val = *it++;
//        if (val)  byte |= bitmask;
//        bitmask >>= 1;
//        if (bitmask==0)
//        {
//          *poo++ = byte;
//          byte = 0;
//          bitmask = 0x80;
//        }
//      }
//      if (bitmask!=0x80)  *poo++ = byte;
//      po += m_ipl_img->widthStep;
//    }
//  }
//  else 
//  {
//    if ((img.Width()*img.NBITS_PER_PIXEL) % 64 != 0)  BLEPO_ERROR("Image is not quad-word aligned");
//    iCreateIplImg(1, IPL_DEPTH_1U, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), const_cast<void*>(static_cast<const void*>(img.BytePtr())));
//  }
//  m_data_has_been_copied = copy_data;
//}
//
//void ImgIplImage::Reset(const ImgFloat& img, bool copy_data)
//{
//  if (copy_data)
//  {
//    iCreateIplImg(1, IPL_DEPTH_32F, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), NULL);
//    char* po = m_ipl_img->imageData;
//    for (int y=0; y<img.Height() ; y++)
//    {
//      const float* pi = img.Begin(0, y);
//      memcpy(po, pi, img.Width()*img.NBITS_PER_PIXEL/8);
//      po += m_ipl_img->widthStep;
//    }
//  }
//  else 
//  {
//    if ((img.Width()*img.NBITS_PER_PIXEL) % 64 != 0)  BLEPO_ERROR("Image is not quad-word aligned");
//    iCreateIplImg(1, IPL_DEPTH_32F, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), const_cast<void*>(static_cast<const void*>(img.Begin())));
//  }
//  m_data_has_been_copied = copy_data;
//}
//
//void ImgIplImage::Reset(const ImgGray& img, bool copy_data)
//{
//  if (copy_data)
//  {
//    iCreateIplImg(1, IPL_DEPTH_8U, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), NULL);
//    char* po = m_ipl_img->imageData;
//    for (int y=0; y<img.Height() ; y++)
//    {
//      const unsigned char* pi = img.Begin(0, y);
//      memcpy(po, pi, img.Width()*img.NBITS_PER_PIXEL/8);
//      po += m_ipl_img->widthStep;
//    }
//  }
//  else 
//  {
//    if ((img.Width()*img.NBITS_PER_PIXEL) % 64 != 0)  BLEPO_ERROR("Image is not quad-word aligned");
//    iCreateIplImg(1, IPL_DEPTH_8U, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), const_cast<void*>(static_cast<const void*>(img.Begin())));
//  }
//  m_data_has_been_copied = copy_data;
//}
//
//void ImgIplImage::Reset(const ImgInt& img, bool copy_data)
//{
//  if (copy_data)
//  {
//    iCreateIplImg(1, IPL_DEPTH_32S, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), NULL);
//    char* po = m_ipl_img->imageData;
//    for (int y=0; y<img.Height() ; y++)
//    {
//      const int* pi = img.Begin(0, y);
//      memcpy(po, pi, img.Width()*img.NBITS_PER_PIXEL/8);
//      po += m_ipl_img->widthStep;
//    }
//  }
//  else 
//  {
//    if ((img.Width()*img.NBITS_PER_PIXEL) % 64 != 0)  BLEPO_ERROR("Image is not quad-word aligned");
//    iCreateIplImg(1, IPL_DEPTH_32S, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), const_cast<void*>(static_cast<const void*>(img.Begin())));
//  }
//  m_data_has_been_copied = copy_data;
//}
//
//ImgIplImage::~ImgIplImage()
//{
//  iDestroyIplImg();
//}
//
//void ImgIplImage::CastToBgr(ImgBgr* out)
//{
//  if (m_ipl_img==NULL)  BLEPO_ERROR("not initialized");
//  if (m_ipl_img->nChannels != 3
//    || m_ipl_img->depth != IPL_DEPTH_8U 
//    || strncmp(m_ipl_img->colorModel, "RGB", 4) != 0
//    || strncmp(m_ipl_img->channelSeq, "BGR", 4) != 0)
//  {
//    BLEPO_ERROR("Cannot cast IPL image to a different type");
//  }
//  out->Reset(m_ipl_img->width, m_ipl_img->height);
//  const char* po = m_ipl_img->imageData;
//  for (int y=0; y<out->Height() ; y++)
//  {
//    unsigned char* pi = reinterpret_cast<unsigned char*>(out->Begin(0, y));
//    memcpy(pi, po, out->Width()*out->NBITS_PER_PIXEL/8);
//    po += m_ipl_img->widthStep;
//  }
//}
//
//void ImgIplImage::CastToBinary(ImgBinary* out)
//{
//  if (m_ipl_img==NULL)  BLEPO_ERROR("not initialized");
//  if (m_ipl_img->nChannels != 1
//    || m_ipl_img->depth != IPL_DEPTH_1U 
//    || strncmp(m_ipl_img->colorModel, "GRAY", 4) != 0
//    || strncmp(m_ipl_img->channelSeq, "GRAY", 4) != 0)
//  {
//    BLEPO_ERROR("Cannot cast IPL image to a different type");
//  }
//  out->Reset(m_ipl_img->width, m_ipl_img->height);
//  const char* po = m_ipl_img->imageData;
//  ImgBinary::Iterator pi = out->Begin();
//  for (int y=0; y<out->Height() ; y++)
//  {
//    const char* poo = po;
//    int x = 0;
//    while (1)
//    {
//      unsigned char byte = *poo++;
//      unsigned char bitmask = 0x80;
//      for (int i=0 ; i<8 ; i++)
//      {
//        *pi++ = ((byte & bitmask) != 0);
//        bitmask >>= 1;
//        x++;
//        if (x >= out->Width())  goto end_of_row;
//      }
//    }
//end_of_row:
//    po += m_ipl_img->widthStep;
//  }
//}
//
//void ImgIplImage::CastToFloat(ImgFloat* out)
//{
//  if (m_ipl_img==NULL)  BLEPO_ERROR("not initialized");
//  if (m_ipl_img->nChannels != 1
//    || m_ipl_img->depth != IPL_DEPTH_32F 
//    || strncmp(m_ipl_img->colorModel, "GRAY", 4) != 0
//    || strncmp(m_ipl_img->channelSeq, "GRAY", 4) != 0)
//  {
//    BLEPO_ERROR("Cannot cast IPL image to a different type");
//  }
//  out->Reset(m_ipl_img->width, m_ipl_img->height);
//  const char* po = m_ipl_img->imageData;
//  for (int y=0; y<out->Height() ; y++)
//  {
//    unsigned char* pi = reinterpret_cast<unsigned char*>(out->Begin(0, y));
//    memcpy(pi, po, out->Width()*out->NBITS_PER_PIXEL/8);
//    po += m_ipl_img->widthStep;
//  }
//}
//
//void ImgIplImage::CastToGray(ImgGray* out)
//{
//  if (m_ipl_img==NULL)  BLEPO_ERROR("not initialized");
//  if (m_ipl_img->nChannels != 1
//    || m_ipl_img->depth != IPL_DEPTH_8U 
//    || strncmp(m_ipl_img->colorModel, "GRAY", 4) != 0
//    || strncmp(m_ipl_img->channelSeq, "GRAY", 4) != 0)
//  {
//    BLEPO_ERROR("Cannot cast IPL image to a different type");
//  }
//  out->Reset(m_ipl_img->width, m_ipl_img->height);
//  const char* po = m_ipl_img->imageData;
//  for (int y=0; y<out->Height() ; y++)
//  {
//    unsigned char* pi = out->Begin(0, y);
//    memcpy(pi, po, out->Width()*out->NBITS_PER_PIXEL/8);
//    po += m_ipl_img->widthStep;
//  }
//}
//
//void ImgIplImage::CastToInt(ImgInt* out)
//{
//  if (m_ipl_img==NULL)  BLEPO_ERROR("not initialized");
//  if (m_ipl_img->nChannels != 1
//    || m_ipl_img->depth != IPL_DEPTH_32S 
//    || strncmp(m_ipl_img->colorModel, "GRAY", 4) != 0
//    || strncmp(m_ipl_img->channelSeq, "GRAY", 4) != 0)
//  {
//    BLEPO_ERROR("Cannot cast IPL image to a different type");
//  }
//  out->Reset(m_ipl_img->width, m_ipl_img->height);
//  const char* po = m_ipl_img->imageData;
//  for (int y=0; y<out->Height() ; y++)
//  {
//    unsigned char* pi = reinterpret_cast<unsigned char*>(out->Begin(0, y));
//    memcpy(pi, po, out->Width()*out->NBITS_PER_PIXEL/8);
//    po += m_ipl_img->widthStep;
//  }
//}
//
//// 'data':  if NULL, then allocates data, too; otherwise just sets pointer
//void ImgIplImage::iCreateIplImg(int nchannels, unsigned depth, 
//                                const char* color_model,
//                                const char* channel_seq,
//                                int width, int height,
//                                void *data)
//{
//  // destroy old ipl image
//  if (m_ipl_img)  iDestroyIplImg();
//
//  // create ipl image header
//  m_ipl_img = iplCreateImageHeader(
//                nchannels,                // number of channels
//                0,                        // alpha channel
//                depth,                    // type of pixel data (1u, 8u, 8s, 16u, 16s, 32s, or 32f)
//                const_cast<char*>(color_model),              // color model (e.g., "GRAY", "RGB", "YUV", "CMYK")
//                const_cast<char*>(channel_seq),              // channel sequence (e.g., "Gray")
//                IPL_DATA_ORDER_PIXEL,     // channel arrangement
//                IPL_ORIGIN_TL,            // top left orientation
//                IPL_ALIGN_QWORD,          // 8 bytes align
//                width, height,            // image width and height
//                NULL, NULL, NULL, NULL);  // no ROI, no mask ROI, no image ID, not tiled   
//
//  // set ipl data pointer, allocating if necessary
//  if (data)
//  {
//    m_ipl_img->imageData = reinterpret_cast<char*>(data);
//    m_ipl_img->imageDataOrigin = reinterpret_cast<char*>(data);
//  }
//  else
//  {
//		if (depth == IPL_DEPTH_32F)		iplAllocateImageFP(m_ipl_img, 0, 0);
//		else									    		iplAllocateImage  (m_ipl_img, 0, 0);
//  }
//}
//
//void ImgIplImage::iDestroyIplImg()
//{
//  if (m_ipl_img)
//  {
//    iplDeallocate(m_ipl_img, m_data_has_been_copied ? IPL_IMAGE_ALL : IPL_IMAGE_HEADER);
//    m_ipl_img = NULL;
//  }
//}
//
//};  // end namespace blepo
//
//#endif // BLEPO_PLEASE_I_WANT_TO_USE_IPL_FOR_TESTING























//
//
//
// old code:
//
//
////  const unsigned char* pi = img.BytePtr();
////  const int nbytes_per_row = img.Width()*img.NBITS_PER_PIXEL/8;
//
//void ImgIplImage::Reset(const ImgBgr& img, bool copy_data)
//{
//  m_ipl_img = cvCreateImage( cvSize(img.Width(), img.Height()), IPL_DEPTH_8U, 3 );
//  assert(strncmp(m_ipl_img->channelSeq, "BGR", 3) == 0);
//  assert(m_ipl_img->origin == 0);
//  CopyData
//  char* po = m_ipl_img->imageData;
//  const int nbytes_per_row = img.Width()*img.NBITS_PER_PIXEL/8;
//  const unsigned char* pi = img.BytePtr(); //reinterpret_cast<const unsigned char*>(img.Begin(0, y));
//  for (int y=0; y<img.Height() ; y++)
//  {
//    memcpy(po, pi, nbytes_per_row);
//    pi += nbytes_per_row;
//    po += m_ipl_img->widthStep;
//  }
//  m_data_has_been_copied = copy_data;
//}
//
//void ImgIplImage::Reset(const ImgBinary& img, bool copy_data)
//{
//  m_ipl_img = cvCreateImage( cvSize(img.Width(), img.Height()), IPL_DEPTH_1U, 1 );
////  assert(strncmp(m_ipl_img->channelSeq, "GRAY", 3) == 0);
//  assert(m_ipl_img->origin == 0);
//  if (img.Width() % 8 == 0)
//  {
//    char* po = m_ipl_img->imageData;
//    const int nbytes_per_row = img.Width()*img.NBITS_PER_PIXEL/8;
//    const unsigned char* pi = img.BytePtr(); //reinterpret_cast<const unsigned char*>(img.Begin(0, y));
//    for (int y=0; y<img.Height() ; y++)
//    {
//      memcpy(po, pi, nbytes_per_row);
//      pi += nbytes_per_row;
//      po += m_ipl_img->widthStep;
//    }
//  }
//  else
//  {
//    // convert from no alignment format to aligned format
//    char* po = m_ipl_img->imageData;
//    for (int y=0; y<img.Height() ; y++)
//    {
//      ImgBinary::ConstIterator p = img.Begin(0, y);
//      char* poo = po;
//      unsigned char byte = 0;
//      unsigned char bitmask = 0x80;
//      for (int x=0 ; x<img.Width() ; x++)
//      {
//        bool val = *p++;
//        if (val)  byte |= bitmask;
//        bitmask >>= 1;
//        if (bitmask==0)
//        {
//          *poo++ = byte;
//          byte = 0;
//          bitmask = 0x80;
//        }
//      }
//      if (bitmask!=0x80)  *poo++ = byte;
//      po += m_ipl_img->widthStep;
//    }
//  }
//
////
////
////
////
////  if (copy_data)
////  {
////    iCreateIplImg(1, IPL_DEPTH_1U, "GRAY", "GRAY",     
////                  img.Width(), img.Height(), NULL);
////    char* po = m_ipl_img->imageData;
////    for (int y=0; y<img.Height() ; y++)
////    {
////      ImgBinary::ConstIterator it = img.Begin(0, y);
////      char* poo = po;
////      unsigned char byte = 0;
////      unsigned char bitmask = 0x80;
////      for (int x=0 ; x<img.Width() ; x++)
////      {
////        bool val = *it++;
////        if (val)  byte |= bitmask;
////        bitmask >>= 1;
////        if (bitmask==0)
////        {
////          *poo++ = byte;
////          byte = 0;
////          bitmask = 0x80;
////        }
////      }
////      if (bitmask!=0x80)  *poo++ = byte;
////      po += m_ipl_img->widthStep;
////    }
////  }
////  else 
////  {
////    if ((img.Width()*img.NBITS_PER_PIXEL) % 64 != 0)  BLEPO_ERROR("Image is not quad-word aligned");
////    iCreateIplImg(1, IPL_DEPTH_1U, "GRAY", "GRAY",     
////                  img.Width(), img.Height(), const_cast<void*>(static_cast<const void*>(img.BytePtr())));
////  }
////  m_data_has_been_copied = copy_data;
//}
//
//void ImgIplImage::Reset(const ImgFloat& img, bool copy_data)
//{
//  if (copy_data)
//  {
//    iCreateIplImg(1, IPL_DEPTH_32F, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), NULL);
//    char* po = m_ipl_img->imageData;
//    for (int y=0; y<img.Height() ; y++)
//    {
//      const float* pi = img.Begin(0, y);
//      memcpy(po, pi, img.Width()*img.NBITS_PER_PIXEL/8);
//      po += m_ipl_img->widthStep;
//    }
//  }
//  else 
//  {
//    if ((img.Width()*img.NBITS_PER_PIXEL) % 64 != 0)  BLEPO_ERROR("Image is not quad-word aligned");
//    iCreateIplImg(1, IPL_DEPTH_32F, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), const_cast<void*>(static_cast<const void*>(img.Begin())));
//  }
//  m_data_has_been_copied = copy_data;
//}
//
//void ImgIplImage::Reset(const ImgGray& img, bool copy_data)
//{
//  if (copy_data)
//  {
//    iCreateIplImg(1, IPL_DEPTH_8U, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), NULL);
//    char* po = m_ipl_img->imageData;
//    for (int y=0; y<img.Height() ; y++)
//    {
//      const unsigned char* pi = img.Begin(0, y);
//      memcpy(po, pi, img.Width()*img.NBITS_PER_PIXEL/8);
//      po += m_ipl_img->widthStep;
//    }
//  }
//  else 
//  {
//    if ((img.Width()*img.NBITS_PER_PIXEL) % 64 != 0)  BLEPO_ERROR("Image is not quad-word aligned");
//    iCreateIplImg(1, IPL_DEPTH_8U, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), const_cast<void*>(static_cast<const void*>(img.Begin())));
//  }
//  m_data_has_been_copied = copy_data;
//}
//
//void ImgIplImage::Reset(const ImgInt& img, bool copy_data)
//{
//  if (copy_data)
//  {
//    iCreateIplImg(1, IPL_DEPTH_32S, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), NULL);
//    char* po = m_ipl_img->imageData;
//    for (int y=0; y<img.Height() ; y++)
//    {
//      const int* pi = img.Begin(0, y);
//      memcpy(po, pi, img.Width()*img.NBITS_PER_PIXEL/8);
//      po += m_ipl_img->widthStep;
//    }
//  }
//  else 
//  {
//    if ((img.Width()*img.NBITS_PER_PIXEL) % 64 != 0)  BLEPO_ERROR("Image is not quad-word aligned");
//    iCreateIplImg(1, IPL_DEPTH_32S, "GRAY", "GRAY",     
//                  img.Width(), img.Height(), const_cast<void*>(static_cast<const void*>(img.Begin())));
//  }
//  m_data_has_been_copied = copy_data;
//}
//
//ImgIplImage::~ImgIplImage()
//{
//  iDestroyIplImg();
//}
//
//void ImgIplImage::CastToBgr(ImgBgr* out)
//{
//  if (m_ipl_img==NULL)  BLEPO_ERROR("not initialized");
//  if (m_ipl_img->nChannels != 3
//    || m_ipl_img->depth != IPL_DEPTH_8U 
//    || strncmp(m_ipl_img->colorModel, "RGB", 4) != 0
//    || strncmp(m_ipl_img->channelSeq, "BGR", 4) != 0)
//  {
//    BLEPO_ERROR("Cannot cast IPL image to a different type");
//  }
//  out->Reset(m_ipl_img->width, m_ipl_img->height);
//  const char* po = m_ipl_img->imageData;
//  for (int y=0; y<out->Height() ; y++)
//  {
//    unsigned char* pi = reinterpret_cast<unsigned char*>(out->Begin(0, y));
//    memcpy(pi, po, out->Width()*out->NBITS_PER_PIXEL/8);
//    po += m_ipl_img->widthStep;
//  }
//}
//
//void ImgIplImage::CastToBinary(ImgBinary* out)
//{
//  if (m_ipl_img==NULL)  BLEPO_ERROR("not initialized");
//  if (m_ipl_img->nChannels != 1
//    || m_ipl_img->depth != IPL_DEPTH_1U 
//    || strncmp(m_ipl_img->colorModel, "GRAY", 4) != 0
//    || strncmp(m_ipl_img->channelSeq, "GRAY", 4) != 0)
//  {
//    BLEPO_ERROR("Cannot cast IPL image to a different type");
//  }
//  out->Reset(m_ipl_img->width, m_ipl_img->height);
//  const char* po = m_ipl_img->imageData;
//  ImgBinary::Iterator pi = out->Begin();
//  for (int y=0; y<out->Height() ; y++)
//  {
//    const char* poo = po;
//    int x = 0;
//    while (1)
//    {
//      unsigned char byte = *poo++;
//      unsigned char bitmask = 0x80;
//      for (int i=0 ; i<8 ; i++)
//      {
//        *pi++ = ((byte & bitmask) != 0);
//        bitmask >>= 1;
//        x++;
//        if (x >= out->Width())  goto end_of_row;
//      }
//    }
//end_of_row:
//    po += m_ipl_img->widthStep;
//  }
//}
//
//void ImgIplImage::CastToFloat(ImgFloat* out)
//{
//  if (m_ipl_img==NULL)  BLEPO_ERROR("not initialized");
//  if (m_ipl_img->nChannels != 1
//    || m_ipl_img->depth != IPL_DEPTH_32F 
//    || strncmp(m_ipl_img->colorModel, "GRAY", 4) != 0
//    || strncmp(m_ipl_img->channelSeq, "GRAY", 4) != 0)
//  {
//    BLEPO_ERROR("Cannot cast IPL image to a different type");
//  }
//  out->Reset(m_ipl_img->width, m_ipl_img->height);
//  const char* po = m_ipl_img->imageData;
//  for (int y=0; y<out->Height() ; y++)
//  {
//    unsigned char* pi = reinterpret_cast<unsigned char*>(out->Begin(0, y));
//    memcpy(pi, po, out->Width()*out->NBITS_PER_PIXEL/8);
//    po += m_ipl_img->widthStep;
//  }
//}
//
//void ImgIplImage::CastToGray(ImgGray* out)
//{
//  if (m_ipl_img==NULL)  BLEPO_ERROR("not initialized");
//  if (m_ipl_img->nChannels != 1
//    || m_ipl_img->depth != IPL_DEPTH_8U 
//    || strncmp(m_ipl_img->colorModel, "GRAY", 4) != 0
//    || strncmp(m_ipl_img->channelSeq, "GRAY", 4) != 0)
//  {
//    BLEPO_ERROR("Cannot cast IPL image to a different type");
//  }
//  out->Reset(m_ipl_img->width, m_ipl_img->height);
//  const char* po = m_ipl_img->imageData;
//  for (int y=0; y<out->Height() ; y++)
//  {
//    unsigned char* pi = out->Begin(0, y);
//    memcpy(pi, po, out->Width()*out->NBITS_PER_PIXEL/8);
//    po += m_ipl_img->widthStep;
//  }
//}
//
//void ImgIplImage::CastToInt(ImgInt* out)
//{
//  if (m_ipl_img==NULL)  BLEPO_ERROR("not initialized");
//  if (m_ipl_img->nChannels != 1
//    || m_ipl_img->depth != IPL_DEPTH_32S 
//    || strncmp(m_ipl_img->colorModel, "GRAY", 4) != 0
//    || strncmp(m_ipl_img->channelSeq, "GRAY", 4) != 0)
//  {
//    BLEPO_ERROR("Cannot cast IPL image to a different type");
//  }
//  out->Reset(m_ipl_img->width, m_ipl_img->height);
//  const char* po = m_ipl_img->imageData;
//  for (int y=0; y<out->Height() ; y++)
//  {
//    unsigned char* pi = reinterpret_cast<unsigned char*>(out->Begin(0, y));
//    memcpy(pi, po, out->Width()*out->NBITS_PER_PIXEL/8);
//    po += m_ipl_img->widthStep;
//  }
//}
//
//// 'data':  if NULL, then allocates data, too; otherwise just sets pointer
//void ImgIplImage::iCreateIplImg(int nchannels, unsigned depth, 
//                                const char* color_model,
//                                const char* channel_seq,
//                                int width, int height,
//                                void *data)
//{
//  // destroy old ipl image
//  if (m_ipl_img)  iDestroyIplImg();
//
//  // create ipl image header
//  m_ipl_img = iplCreateImageHeader(
//                nchannels,                // number of channels
//                0,                        // alpha channel
//                depth,                    // type of pixel data (1u, 8u, 8s, 16u, 16s, 32s, or 32f)
//                const_cast<char*>(color_model),              // color model (e.g., "GRAY", "RGB", "YUV", "CMYK")
//                const_cast<char*>(channel_seq),              // channel sequence (e.g., "Gray")
//                IPL_DATA_ORDER_PIXEL,     // channel arrangement
//                IPL_ORIGIN_TL,            // top left orientation
//                IPL_ALIGN_QWORD,          // 8 bytes align
//                width, height,            // image width and height
//                NULL, NULL, NULL, NULL);  // no ROI, no mask ROI, no image ID, not tiled   
//
//  // set ipl data pointer, allocating if necessary
//  if (data)
//  {
//    m_ipl_img->imageData = reinterpret_cast<char*>(data);
//    m_ipl_img->imageDataOrigin = reinterpret_cast<char*>(data);
//  }
//  else
//  {
//		if (depth == IPL_DEPTH_32F)		iplAllocateImageFP(m_ipl_img, 0, 0);
//		else									    		iplAllocateImage  (m_ipl_img, 0, 0);
//  }
//}
//
//void ImgIplImage::iDestroyIplImg()
//{
//  if (m_ipl_img)
//  {
//    iplDeallocate(m_ipl_img, m_data_has_been_copied ? IPL_IMAGE_ALL : IPL_IMAGE_HEADER);
//    m_ipl_img = NULL;
//  }
//}

