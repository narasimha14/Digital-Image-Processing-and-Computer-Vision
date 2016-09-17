// Still need to incorporate:
//void mmx_const_diff(const unsigned char *src , const unsigned char val  , unsigned char *dst, int num_quadwords      );
//void xmm_const_diff(const unsigned char *src , const unsigned char val  , unsigned char *dst, int num_doublequadwords);
//void mmx_const_sum (const unsigned char *src , const unsigned char val  , unsigned char *dst, int num_quadwords      );
//void xmm_const_sum (const unsigned char *src , const unsigned char val  , unsigned char *dst, int num_doublequadwords);
//void mmx_convolve_prewitt_horiz_abs(const unsigned char* src, unsigned char* dst, int width, int height);
//void mmx_convolve_prewitt_vert_abs (const unsigned char* src, unsigned char* dst, int width, int height);
//void mmx_gauss_1x3(const unsigned char *src, unsigned char *dst, int width, int height);
//void mmx_gauss_3x1(const unsigned char *src, unsigned char *dst, int width, int height);

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

#include "Image.h"
#include "ImageOperations.h"
#include "Utilities/Array.h"
#include "Utilities/Exception.h"
#include "Utilities/Math.h"
//#include "Utilities/Array.h"
#include "Figure/Figure.h"  // for debugging
#include "Quick/Quick.h"
#include "Matrix/LinearAlgebra.h"
#include "Matrix/MatrixOperations.h"  // matrix multiply using *
#include "ImgIplImage.h"
#include "BmpFile.h"
#include "JpegFile.h"
#include "PnmFile.h"
#include "../../external/FFTW/fftw3.h"
//#include "highgui.h"  // LoadOpenCV -- OpenCV
#include "blepo_opencv.h"  // cvFindHomography -- OpenCV
//extern "C" {
//#include "KltBase/pnmio.h"  // load/save pgm
//}


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

// This number should be slightly greater than 1.  Used by Interp()
// to handle case of pixels being near the right or bottom border.
// It would probably be better to split up the program to handle these
// cases separately, but it would require more code.
float g_interp_extra = 1.1f;

// Returns the number of bytes needed to hold a double-word-aligned image with the given
// width, height, and bit depth.   (32 bits in a double word)
int iComputeAlignment32(int width, int height, int nbits_per_pixel)
{ 
  assert(width>=0 && height>=0 && nbits_per_pixel>=0);
  const int ndwords_per_row = ((width*nbits_per_pixel)+31) >> 5;
  return height * (ndwords_per_row << 2);
}

/**
This class facilitates 'inplace' operations with a temporary buffer.
If '*out' points to 'in',
   then the constructor causes '*out' to point to a temporary instead, and
        the destructor copies the temporary to 'in'
Else,
   this class does nothing.
(We may want to pull this class out of this file at some point, because it 
might be applicable to other types of problems.)
*/
template <typename T>
class InPlaceSwapper
{
public:
  InPlaceSwapper(const T& in, T** out)
  {
    m_inplace = (*out == &in);
    if (m_inplace)  { m_out = *out;  *out = &m_tmp; }
  }
  ~InPlaceSwapper() { if (m_inplace)  *m_out = m_tmp; }
private:
  T m_tmp, *m_out;
  bool m_inplace;
};

/**
Load and save JPEG/BMP Images
  Uses code in Bmpfile, Jpefile, and the Jpeglib folder, all of which 
  come from http://www.smalleranimals.com/jpegfile.htm .
@author Prashant Oswal
*/

void iLoadJpeg(const CString& fname, ImgBgr* out)
{
  unsigned int width, height;
  BYTE* data_ptr = JpegFile::JpegFileToRGB(fname, &width, &height);
  //Memory for data_ptr is created in function JpegFileToRGB. We must clean up memory later!!
  if (!data_ptr)  BLEPO_ERROR(StringEx("Unable to load JPEG file '%s'", fname));
  BYTE* data_ptr_temp = data_ptr;
  out->Reset(width, height);
  ImgBgr::Iterator imgbgr_ptr;
  //Converting from RGB to BGR format
  for (imgbgr_ptr = out->Begin() ; imgbgr_ptr != out->End() ; imgbgr_ptr++)
  {
    imgbgr_ptr->r = *data_ptr++;
    imgbgr_ptr->g = *data_ptr++;
    imgbgr_ptr->b = *data_ptr++;
  }
  delete[] data_ptr_temp;
}

// 'save_as_bgr':  true means save as BGR, false means save as grayscale
void iSaveJpeg(const ImgBgr& img, const CString& fname, bool save_as_bgr)
{
  BYTE* data_ptr;
  data_ptr = (BYTE *)new BYTE[img.Width() * 3 * img.Height()];
  assert(data_ptr);
  BYTE* data_ptr_temp = data_ptr;
  ImgBgr::ConstIterator imgbgr_ptr;
  //Converting from BGR to RGB format.
  for (imgbgr_ptr = img.Begin() ; imgbgr_ptr != img.End() ; imgbgr_ptr++)
  {
    *data_ptr++ = imgbgr_ptr->r;
    *data_ptr++ = imgbgr_ptr->g;
    *data_ptr++ = imgbgr_ptr->b;
  }
  BOOL ok_save=JpegFile::RGBToJpegFile(fname,data_ptr_temp,img.Width(),img.Height(),save_as_bgr);
  assert(ok_save);
  delete[] data_ptr_temp;
}

//LoadBMP will NOT work for compressed BMP imgaes!!
void iLoadBmp(const CString& fname, ImgBgr* out)
{
  unsigned int width, height;
  BYTE* data_ptr = BMPFile::LoadBMP(fname,&width,&height);
  if (data_ptr == NULL)
  {
	  BLEPO_ERROR(StringEx("Sorry, cannot read BMP file.  May be 32 bits per pixel?"));
  }
  //Memory for data_ptr is cretaed in this function. We must clean up memory later!!
  assert(data_ptr);
  BYTE* data_ptr_temp = data_ptr;
  out->Reset(width, height);
  ImgBgr::Iterator imgbgr_ptr;
  //Converting from RGB to BGR format
  for (imgbgr_ptr = out->Begin() ; imgbgr_ptr != out->End() ; imgbgr_ptr++)
  {
    imgbgr_ptr->r = *data_ptr++;
    imgbgr_ptr->g = *data_ptr++;
    imgbgr_ptr->b = *data_ptr++;
  }
  delete[] data_ptr_temp;
}

void iSaveBmpRgb(const ImgBgr& img, const CString& fname)
{
  BYTE* data_ptr;
  data_ptr = (BYTE *)new BYTE[img.Width() * 3 * img.Height()];
  assert(data_ptr);
  BYTE* data_ptr_temp = data_ptr;
	ImgBgr::ConstIterator imgbgr_ptr;
  //SaveBMP needs BGR format and vertically flipped!!!!!!!!!!!
	for (imgbgr_ptr = img.Begin(); imgbgr_ptr != img.End() ; imgbgr_ptr++)
	{
		*data_ptr++ = imgbgr_ptr->b;
		*data_ptr++ = imgbgr_ptr->g;
		*data_ptr++ = imgbgr_ptr->r;
	}
  BOOL ok_flip=JpegFile::VertFlipBuf(data_ptr_temp,img.Width()*3,img.Height());
  assert(ok_flip);
  BOOL ok_save = BMPFile::SaveBMP(fname,data_ptr_temp,img.Width(),img.Height());
  assert(ok_save);
  delete[] data_ptr_temp;
}

void iSaveBmpGray(const ImgGray& img, const CString& fname)
{
  ImgGray img_copy = img;  // temp image so we can flip the data, which is required by BMP
  int i;
  RGBQUAD colormap[256];
  
  for(i=0;i<=255;i++)
  {
    colormap[i].rgbRed = i;
    colormap[i].rgbGreen = i;
    colormap[i].rgbBlue = i;
    colormap[i].rgbReserved = i; //This is not used.
  }
  BOOL ok_flip = JpegFile::VertFlipBuf(img_copy.Begin(),img_copy.Width(),img_copy.Height());
  assert(ok_flip);
  BOOL ok_save = BMPFile::SaveBMP(fname,img_copy.Begin(),img_copy.Width(),img_copy.Height(),8,256,colormap);
  assert(ok_save);
}

void iSaveEpsGray(const ImgGray& img, const CString& fname)
{
  // Write grayscale image to EPS (code thanks to Donald Tanguay)

  // open file
  int width = img.Width(), height = img.Height();
  FILE* fp = _wfopen(fname, L"wt");
  if (fp==NULL)  BLEPO_ERROR(StringEx("Cannot open file %s for writing!", fname));

  // write header
  fprintf(fp, "%%!PS-Adobe-2.0 EPSF-2.0\n");
  fprintf(fp, "%%%%Creator: Blepo (Thanks to Donald Tanguay)\n");
  fprintf(fp, "%%%%BoundingBox: 0 0 %d %d\n", width, height);
  fprintf(fp, "%%%%EndComments\n");
  fprintf(fp, "%%%%EndProlog\n\n");

  fprintf(fp, "/prevstate save def\n");
  fprintf(fp, "5 dict begin\n");
  fprintf(fp, "/scanline %d string def\n", width);
  //fprintf(fp, "translate\n");
  fprintf(fp, "%d %d scale\n", width, height);
  fprintf(fp, "%d %d 8\n", width, height);
  fprintf(fp, "[%d 0 0 -%d 0 %d]\n", width, height, height);
  fprintf(fp, "{currentfile scanline readhexstring pop}\n");
  //				if (isRGB)
  //					fprintf(fp, "false 3 color");
  fprintf(fp, "image\n\n");

  // write the actual data in hex characters.
  int pos = 0;
  for (int y=0 ; y<height ; y++)
  {
    ImgGray::ConstIterator ptr = img.Begin(0, y);
    for (int x=0 ; x<width ; x++) 
    {
      fprintf(fp, "%02hx", *ptr++);
      if (++pos >= 32)
      {
        fprintf(fp, "\n");
        pos = 0;
      }
    }
  }

  // finish it
  if (pos)  fprintf(fp, "\n");
  //				fprintf(fp, "\nshowpage\n");
  fprintf(fp, "end\n");
  fprintf(fp, "prevstate restore\n");
  fprintf(fp, "\n%%%%Trailer\n");
  fclose(fp);
}

void iSaveEpsBgr(const ImgBgr& img, const CString& fname)
{
  // Write Bgr image to EPS (code thanks to Donald Tanguay)

  // open file
  int width = img.Width(), height = img.Height();
  FILE* fp = _wfopen(fname, L"wt");
  if (fp==NULL)  BLEPO_ERROR(StringEx("Cannot open file %s for writing!", fname));

  // write header
  fprintf(fp, "%%!PS-Adobe-2.0 EPSF-2.0\n");
  fprintf(fp, "%%%%Creator: Blepo (Thanks to Donald Tanguay)\n");
  fprintf(fp, "%%%%BoundingBox: 0 0 %d %d\n", width, height);
  fprintf(fp, "%%%%EndComments\n");
  fprintf(fp, "%%%%EndProlog\n\n");

  fprintf(fp, "/prevstate save def\n");
  fprintf(fp, "5 dict begin\n");
  fprintf(fp, "/scanline %d string def\n", width);
  //fprintf(fp, "translate\n");
  fprintf(fp, "%d %d scale\n", width, height);
  fprintf(fp, "%d %d 8\n", width, height);
  fprintf(fp, "[%d 0 0 -%d 0 %d]\n", width, height, height);
  fprintf(fp, "{currentfile scanline readhexstring pop}\n");
  fprintf(fp, "false 3 color");
  fprintf(fp, "image\n\n");

  // write the actual data in hex characters.
  int pos = 0;
  for (int y=0 ; y<height ; y++)
  {
    ImgBgr::ConstIterator ptr = img.Begin(0, y);
    for (int x=0 ; x<width ; x++) 
    {
      fprintf(fp, "%02hx", ptr->r);
      fprintf(fp, "%02hx", ptr->g);
      fprintf(fp, "%02hx", ptr->b);
      ptr++;
      pos += 3;
      if (pos >= 32)
      {
        fprintf(fp, "\n");
        pos = 0;
      }
    }
  }

  // finish it
  if (pos)  fprintf(fp, "\n");
  //				fprintf(fp, "\nshowpage\n");
  fprintf(fp, "end\n");
  fprintf(fp, "prevstate restore\n");
  fprintf(fp, "\n%%%%Trailer\n");
  fclose(fp);
}


void iLoadPgm(const CString& fname, ImgGray* out)
{
  int ncols, nrows;
  CStringA fnamea;
  fnamea = fname;
  unsigned char* data = pgmReadFile(fnamea, NULL, &ncols, &nrows);
  unsigned char* p = data;

  out->Reset(ncols, nrows);
  for (ImgGray::Iterator q = out->Begin() ; q != out->End() ; )  *q++ = *p++;
  free(data);
}

void iSavePgm(const ImgGray& img, const CString& fname)
{
  CStringA fnamea;
  fnamea = fname;
  pgmWriteFile(fnamea, const_cast<unsigned char*>(img.Begin()), img.Width(), img.Height());
}

void iLoadPpm(const CString& fname, ImgBgr* out)
{
  int ncols, nrows;
  CStringA fnamea;
  fnamea = fname;
  unsigned char* data = ppmReadFile(fnamea, NULL, &ncols, &nrows);
  unsigned char* p = data;

  out->Reset(ncols, nrows);
  for (ImgBgr::Iterator q = out->Begin() ; q != out->End() ;  q++)
  {
    q->r = *p++;
    q->g = *p++;
    q->b = *p++;
  }
  free(data);
}

void iSavePpm(const ImgBgr& img, const CString& fname)
{
  Array<unsigned char> data(img.NBytes());
  unsigned char* p = data.Begin();
  for (ImgBgr::ConstIterator q = img.Begin() ; q != img.End() ;  q++)
  {
    *p++ = q->r;
    *p++ = q->g;
    *p++ = q->b;
  }
  CStringA fnamea;
  fnamea = fname;
  ppmWriteFile(fnamea, const_cast<unsigned char*>(data.Begin()), img.Width(), img.Height());
}

//struct iAndOperator
//{
//  static void XmmOp(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
//  {
//    blepo::xmm_and(src1, src2, dst, nbytes);
//  }
//  static void MmxOp(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
//  {
//    blepo::mmx_and(src1, src2, dst, nbytes);
//  }
//  static unsigned char SingleOp(const unsigned char src1, const unsigned char src2)
//  {
//    return src1 & src2;
//  }
//};
//
//struct iOrOperator
//{
//  static void XmmOp(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
//  {
//    blepo::xmm_or(src1, src2, dst, nbytes);
//  }
//  static void MmxOp(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
//  {
//    blepo::mmx_or(src1, src2, dst, nbytes);
//  }
//  static unsigned char SingleOp(const unsigned char src1, const unsigned char src2)
//  {
//    return src1 | src2;
//  }
//};
//
//struct iXorOperator
//{
//  static void XmmOp(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
//  {
//    blepo::xmm_xor(src1, src2, dst, nbytes);
//  }
//  static void MmxOp(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
//  {
//    blepo::mmx_xor(src1, src2, dst, nbytes);
//  }
//  static unsigned char SingleOp(const unsigned char src1, const unsigned char src2)
//  {
//    return src1 ^ src2;
//  }
//};
//
//struct iAbsDiffOperator
//{
//  static void XmmOp(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
//  {
//    blepo::xmm_absdiff(src1, src2, dst, nbytes);
//  }
//  static void MmxOp(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
//  {
//    blepo::mmx_absdiff(src1, src2, dst, nbytes);
//  }
//  static unsigned char SingleOp(const unsigned char src1, const unsigned char src2)
//  {
//    return blepo_ex::Abs(src1 - src2);
//  }
//};
//
//struct iSaturatedSumOperator
//{
//  static void XmmOp(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
//  {
//    blepo::xmm_sum(src1, src2, dst, nbytes);
//  }
//  static void MmxOp(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
//  {
//    blepo::mmx_sum(src1, src2, dst, nbytes);
//  }
//  static unsigned char SingleOp(const unsigned char src1, const unsigned char src2)
//  {
//    return blepo_ex::Min(src1 + src2, 255);
//  }
//};
//
//struct iSaturatedSubtractOperator
//{
//  static void XmmOp(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
//  {
//    blepo::xmm_subtract(src1, src2, dst, nbytes);
//  }
//  static void MmxOp(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
//  {
//    blepo::mmx_subtract(src1, src2, dst, nbytes);
//  }
//  static unsigned char SingleOp(const unsigned char src1, const unsigned char src2)
//  {
//    return (src1 > src2) ? src1 - src2 : 0;
//  }
//};
//
//// Perform the operation specified by 'T' on two input arrays, assuming memory has
//// already been allocated for the output.  Any class that has the following static
//// member functions implemented can be passed for 'T':  XmmOp, MmxOp, and SingleOp.
//template <typename T>
//void iOp(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
//{
//  int m = nbytes, skip = 0;
//
//  if (blepo::CanDoSse2() && nbytes>=16)
//  {
//    // operate on 16 bytes simultaneously
//    T::XmmOp(src1, src2, dst, nbytes/16);
//    m = nbytes % 16;
//    if (m==0)  return;
//    skip = nbytes - m;
//  }
//  else if (blepo::CanDoMmx() && nbytes>=8)
//  {
//    // operate on 8 bytes simultaneously
//    T::MmxOp(src1, src2, dst, nbytes/8);
//    m = nbytes % 8;
//    if (m==0)  return;
//    skip = nbytes - m;
//  }
//
//  // operate on individual pixels
//  src1 += skip;
//  src2 += skip;
//  dst += skip;
//  for (int i=0 ; i<m ; i++)  *dst++ = T::SingleOp( *src1++, *src2++ );
//}

void iAbsDiff(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
{
  int m = nbytes, skip = 0;

  if (blepo::CanDoSse2() && nbytes>=16)
  {
    // operate on 16 bytes simultaneously
    blepo::xmm_absdiff(src1, src2, dst, nbytes/16);
    m = nbytes % 16;
    if (m==0)  return;
    skip = nbytes - m;
  }
  else if (blepo::CanDoMmx() && nbytes>=8)
  {
    // operate on 8 bytes simultaneously
    blepo::mmx_absdiff(src1, src2, dst, nbytes/8);
    m = nbytes % 8;
    if (m==0)  return;
    skip = nbytes - m;
  }

  // operate on individual pixels
  src1 += skip;
  src2 += skip;
  dst += skip;
  for (int i=0 ; i<m ; i++)  *dst++ = blepo_ex::Abs((*src1++) - (*src2++));
}

void iSaturatedSum(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
{
  int m = nbytes, skip = 0;

  if (blepo::CanDoSse2() && nbytes>=16)
  {
    // operate on 16 bytes simultaneously
    blepo::xmm_sum(src1, src2, dst, nbytes/16);
    m = nbytes % 16;
    if (m==0)  return;
    skip = nbytes - m;
  }
  else if (blepo::CanDoMmx() && nbytes>=8)
  {
    // operate on 8 bytes simultaneously
    blepo::mmx_sum(src1, src2, dst, nbytes/8);
    m = nbytes % 8;
    if (m==0)  return;
    skip = nbytes - m;
  }

  // operate on individual pixels
  src1 += skip;
  src2 += skip;
  dst += skip;
  for (int i=0 ; i<m ; i++)  *dst++ = blepo_ex::Clamp((*src1++) + (*src2++), 0, 255);
}

void iSaturatedSubtract(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
{
  int m = nbytes, skip = 0;

  if (blepo::CanDoSse2() && nbytes>=16)
  {
    // operate on 16 bytes simultaneously
    blepo::xmm_subtract(src1, src2, dst, nbytes/16);
    m = nbytes % 16;
    if (m==0)  return;
    skip = nbytes - m;
  }
  else if (blepo::CanDoMmx() && nbytes>=8)
  {
    // operate on 8 bytes simultaneously
    blepo::mmx_subtract(src1, src2, dst, nbytes/8);
    m = nbytes % 8;
    if (m==0)  return;
    skip = nbytes - m;
  }

  // operate on individual pixels
  src1 += skip;
  src2 += skip;
  dst += skip;
  for (int i=0 ; i<m ; i++)  *dst++ = blepo_ex::Clamp((*src1++) - (*src2++), 0, 255);
}


void iAnd(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
{
  int m = nbytes, skip = 0;

  if (blepo::CanDoSse2() && nbytes>=16)
  {
    // operate on 16 bytes simultaneously
    blepo::xmm_and(src1, src2, dst, nbytes/16);
    m = nbytes % 16;
    if (m==0)  return;
    skip = nbytes - m;
  }
  else if (blepo::CanDoMmx() && nbytes>=8)
  {
    // operate on 8 bytes simultaneously
    blepo::mmx_and(src1, src2, dst, nbytes/8);
    m = nbytes % 8;
    if (m==0)  return;
    skip = nbytes - m;
  }

  // operate on individual pixels
  src1 += skip;
  src2 += skip;
  dst += skip;
  for (int i=0 ; i<m ; i++)  *dst++ = (*src1++) & (*src2++);
}

void iOr(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
{
  int m = nbytes, skip = 0;

  if (blepo::CanDoSse2() && nbytes>=16)
  {
    // operate on 16 bytes simultaneously
    blepo::xmm_or(src1, src2, dst, nbytes/16);
    m = nbytes % 16;
    if (m==0)  return;
    skip = nbytes - m;
  }
  else if (blepo::CanDoMmx() && nbytes>=8)
  {
    // operate on 8 bytes simultaneously
    blepo::mmx_or(src1, src2, dst, nbytes/8);
    m = nbytes % 8;
    if (m==0)  return;
    skip = nbytes - m;
  }

  // operate on individual pixels
  src1 += skip;
  src2 += skip;
  dst += skip;
  for (int i=0 ; i<m ; i++)  *dst++ = (*src1++) | (*src2++);
}

void iXor(const unsigned char* src1, const unsigned char* src2, unsigned char* dst, int nbytes)
{
  int m = nbytes, skip = 0;

  if (blepo::CanDoSse2() && nbytes>=16)
  {
    // operate on 16 bytes simultaneously
    blepo::xmm_xor(src1, src2, dst, nbytes/16);
    m = nbytes % 16;
    if (m==0)  return;
    skip = nbytes - m;
  }
  else if (blepo::CanDoMmx() && nbytes>=8)
  {
    // operate on 8 bytes simultaneously
    blepo::mmx_xor(src1, src2, dst, nbytes/8);
    m = nbytes % 8;
    if (m==0)  return;
    skip = nbytes - m;
  }

  // operate on individual pixels
  src1 += skip;
  src2 += skip;
  dst += skip;
  for (int i=0 ; i<m ; i++)  *dst++ = (*src1++) ^ (*src2++);
}

void iNot(const unsigned char* src, unsigned char* dst, int nbytes)
{
  int m = nbytes, skip = 0;

  if (blepo::CanDoSse2() && nbytes>=16)
  {
    // operate on 16 bytes simultaneously
    blepo::xmm_not(src, dst, nbytes/16);
    m = nbytes % 16;
    if (m==0)  return;
    skip = nbytes - m;
  }
  else if (blepo::CanDoMmx() && nbytes>=8)
  {
    // operate on 8 bytes simultaneously
    blepo::mmx_not(src, dst, nbytes/8);
    m = nbytes % 8;
    if (m==0)  return;
    skip = nbytes - m;
  }

  // operate on individual pixels
  src += skip;
  dst += skip;
  for (int i=0 ; i<m ; i++)  *dst++ = ~(*src++);
}

void iConstAnd(const unsigned char* src1, const unsigned char val, unsigned char* dst, int nbytes)
{
  int m = nbytes, skip = 0;

  if (blepo::CanDoSse2() && nbytes>=16)
  {
    // operate on 16 bytes simultaneously
    blepo::xmm_const_and(src1, val, dst, nbytes/16);
    m = nbytes % 16;
    if (m==0)  return;
    skip = nbytes - m;
  }
  else if (blepo::CanDoMmx() && nbytes>=8)
  {
    // operate on 8 bytes simultaneously
    blepo::mmx_const_and(src1, val, dst, nbytes/8);
    m = nbytes % 8;
    if (m==0)  return;
    skip = nbytes - m;
  }

  // operate on individual pixels
  src1 += skip;
  dst += skip;
  for (int i=0 ; i<m ; i++)  *dst++ = (*src1++) & val;
}

void iConstOr(const unsigned char* src1, const unsigned char val, unsigned char* dst, int nbytes)
{
  int m = nbytes, skip = 0;

  if (blepo::CanDoSse2() && nbytes>=16)
  {
    // operate on 16 bytes simultaneously
    blepo::xmm_const_or(src1, val, dst, nbytes/16);
    m = nbytes % 16;
    if (m==0)  return;
    skip = nbytes - m;
  }
  else if (blepo::CanDoMmx() && nbytes>=8)
  {
    // operate on 8 bytes simultaneously
    blepo::mmx_const_or(src1, val, dst, nbytes/8);
    m = nbytes % 8;
    if (m==0)  return;
    skip = nbytes - m;
  }

  // operate on individual pixels
  src1 += skip;
  dst += skip;
  for (int i=0 ; i<m ; i++)  *dst++ = (*src1++) | val;
}

void iConstXor(const unsigned char* src1, const unsigned char val, unsigned char* dst, int nbytes)
{
  int m = nbytes, skip = 0;

  if (blepo::CanDoSse2() && nbytes>=16)
  {
    // operate on 16 bytes simultaneously
    blepo::xmm_const_xor(src1, val, dst, nbytes/16);
    m = nbytes % 16;
    if (m==0)  return;
    skip = nbytes - m;
  }
  else if (blepo::CanDoMmx() && nbytes>=8)
  {
    // operate on 8 bytes simultaneously
    blepo::mmx_const_xor(src1, val, dst, nbytes/8);
    m = nbytes % 8;
    if (m==0)  return;
    skip = nbytes - m;
  }

  // operate on individual pixels
  src1 += skip;
  dst += skip;
  for (int i=0 ; i<m ; i++)  *dst++ = (*src1++) ^ val;
}

template <typename T>
inline void iFlipVertical(const Image<T>& img, Image<T>* out)
{
  const int w = img.Width();
  const int h = img.Height();
  if (&img == out)
  {  
    // in place
    Image<T> tmp(w, 1);
    Image<T>::Iterator tt = tmp.Begin();
    const int n = w * sizeof(Image<T>::Pixel);
    Image<T>::Iterator p1 = out->Begin();
    Image<T>::Iterator p2 = out->Begin(0, h-1);
    for (int y = 0 ; y < h/2 ; y++)
    {
      memcpy(tt, p1, n);
      memcpy(p1, p2, n);
      memcpy(p2, tt, n);
      p1 += w;
      p2 -= w;
    }
  }
  else
  {
    // not in place
    out->Reset(w, h);
    const int n = w * sizeof(Image<T>::Pixel);
    Image<T>::ConstIterator p1 = img.Begin();
    Image<T>::Iterator p2 = out->Begin(0, h-1);
    for (int y = 0 ; y < h ; y++)
    {
      memcpy(p2, p1, n);
      p1 += w;
      p2 -= w;
    }
  }
}

template <typename T>
inline void iFlipHorizontal(const Image<T>& img, Image<T>* out)
{
  const int w = img.Width();
  const int h = img.Height();
  if (&img == out)
  {  
    // in place
    Image<T>::Pixel tmp;
    Image<T>::Iterator p1;
    Image<T>::Iterator p2;
    for (int y = 0 ; y < h ; y++)
    {
      p1 = out->Begin(0, y);
      p2 = out->Begin(w-1, y);
      for (int x = 0 ; x < w/2 ; x++)
      {
        tmp = *p1;
        *p1++ = *p2;
        *p2-- = tmp;
      }
    }
  }
  else
  {
    // not in place
    out->Reset(w, h);
    Image<T>::ConstIterator p1;
    Image<T>::Iterator p2;
    for (int y = 0 ; y < h ; y++)
    {
      p1 = img.Begin(0, y);
      p2 = out->Begin(w-1, y);
      for (int x = 0 ; x < w ; x++)
      {
        *p2-- = *p1++;
      }
    }
  }
}

template <typename T>
inline void iTranspose(const Image<T>& img, Image<T>* out)
{
  const int w = img.Width();
  const int h = img.Height();
  if (&img == out)
  {  
    // in place
    if (w==1 || h==1)
    { // vector
      out->Reset(h, w);
    }
    else
    {
      assert(0);  // need to implement
    }
  }
  else
  {
    // not in place
    assert(0);  // need to implement
  }
}

//template <typename U, typename T>
//void iiOp(const Image<T>& img1, const Image<T>& img2, Image<T>* out)
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<U>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}


//template <typename T>
//void iiAnd(const Image<T>& img1, const Image<T>& img2, Image<T>* out)
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iAnd(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//template <typename T>
//void iiOr(const Image<T>& img1, const Image<T>& img2, Image<T>* out)
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOr(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//template <typename T>
//void iiXor(const Image<T>& img1, const Image<T>& img2, Image<T>* out)
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iXor(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}

//template <typename T>
//void iiNot(const Image<T>& img, Image<T>* out)
//{
//  out->Reset(img.Width(), img.Height());
//  int nbytes = img.NBytes();
//  iNot(img.BytePtr(), out->BytePtr(), nbytes); 
//}

//template <typename T>
//void iiAbsDiff(const Image<T>& img1, const Image<T>& img2, Image<T>* out)
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iAbsDiff(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}

//template <typename T>
//void iiConstAnd(const Image<T>& img, unsigned char val, Image<T>* out)
//{
//  out->Reset(img.Width(), img.Height());
//  int nbytes = img.NBytes();
//  iConstAnd(img.BytePtr(), val, out->BytePtr(), nbytes); 
//}
//
//template <typename T>
//void iiConstOr(const Image<T>& img, unsigned char val, Image<T>* out)
//{
//  out->Reset(img.Width(), img.Height());
//  int nbytes = img.NBytes();
//  iConstOr(img.BytePtr(), val, out->BytePtr(), nbytes); 
//}
//
//template <typename T>
//void iiConstXor(const Image<T>& img, unsigned char val, Image<T>* out)
//{
//  out->Reset(img.Width(), img.Height());
//  int nbytes = img.NBytes();
//  iConstXor(img.BytePtr(), val, out->BytePtr(), nbytes); 
//}

template <typename T>
typename Image<T>::Pixel iMin(const Image<T>& img)
{
  assert(img.Width() > 0 && img.Height() > 0);
  Image<T>::Pixel minn = img(0, 0);
  for (Image<T>::ConstIterator p = img.Begin() ; p != img.End() ; p++)  minn = blepo_ex::Min(minn, *p);
  return minn;
}

template <typename T>
typename Image<T>::Pixel iMax(const Image<T>& img)
{
  assert(img.Width() > 0 && img.Height() > 0);
  Image<T>::Pixel maxx = img(0, 0);
  for (Image<T>::ConstIterator p = img.Begin() ; p != img.End() ; p++)  maxx = blepo_ex::Max(maxx, *p);
  return maxx;
}

template <typename T>
void iMinMax(const Image<T>& img, typename Image<T>::Pixel* minn, typename Image<T>::Pixel* maxx)
{
  assert(img.Width() > 0 && img.Height() > 0);
  Image<T>::ConstIterator p = img.Begin();
  *minn = *maxx = *p++;
  for ( ; p != img.End() ; p++)
  {
    Image<T>::Pixel pix = *p;
    *minn = blepo_ex::Min(*minn, pix);
    *maxx = blepo_ex::Max(*maxx, pix);
  }
}

template <typename T>
void iMin(const Image<T>& img1, const Image<T>& img2, Image<T>* out)
{
  assert(IsSameSize(img1, img2));
  out->Reset(img1.Width(), img1.Height());
  Image<T>::ConstIterator p1 = img1.Begin();
  Image<T>::ConstIterator p2 = img2.Begin();
  Image<T>::Iterator po = out->Begin();
  for ( ; po != out->End() ; )  *po++ = blepo_ex::Min(*p1++, *p2++);
}

template <typename T>
void iMax(const Image<T>& img1, const Image<T>& img2, Image<T>* out)
{
  assert(IsSameSize(img1, img2));
  out->Reset(img1.Width(), img1.Height());
  Image<T>::ConstIterator p1 = img1.Begin();
  Image<T>::ConstIterator p2 = img2.Begin();
  Image<T>::Iterator po = out->Begin();
  for ( ; po != out->End() ; )  *po++ = blepo_ex::Max(*p1++, *p2++);
}

// set all pixels outside 'rect'
template <typename T>
void iSetOutside(Image<T>* out, const Rect& rect, typename Image<T>::Pixel val)
{
  assert(rect.left<=rect.right && rect.top<=rect.bottom && rect.left>=0 && rect.top>=0 && rect.right<=out->Width() && rect.bottom<=out->Height());
  Image<T>::Iterator p, pe;
  // set pixels above rect
  for (p = out->Begin(), pe = out->Begin(rect.left, rect.top) ; p != pe ; )  *p++ = val;
  // set pixels left and right of rect
  const int skip = (rect.right - rect.left);
  const int n = out->Width()-skip;
  for (int y = rect.top ; y<rect.bottom ; y++)
  {
    p += skip;
    for (int i=n  ; i>0 ; i--)  *p++ = val;
  }
  // set pixels below rect
  while (p != out->End())  *p++ = val;
}

template<typename T>
void iEqual(const Image<T>& img1, const typename Image<T>& img2, ImgBinary* out)
{
  assert(IsSameSize(img1, img2));
  out->Reset(img1.Width(), img1.Height());
  Image<T>::ConstIterator p1 = img1.Begin();
  Image<T>::ConstIterator p2 = img2.Begin();
  ImgBinary::Iterator po = out->Begin();
  for ( ; po != out->End() ; )  *po++ = *p1++ == *p2++;
}

template<typename T>
void iEqual(const Image<T>& img, const typename Image<T>::Pixel& pix, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  Image<T>::ConstIterator p = img.Begin();
  ImgBinary::Iterator q = out->Begin();
  while (q != out->End())  *q++ = (*p++ == pix);
}

template<typename T>
void iNotEqual(const Image<T>& img1, const Image<T>& img2, ImgBinary* out)
{
  assert(IsSameSize(img1, img2));
  out->Reset(img1.Width(), img1.Height());
  Image<T>::ConstIterator p1 = img1.Begin();
  Image<T>::ConstIterator p2 = img2.Begin();
  ImgBinary::Iterator po = out->Begin();
  for ( ; po != out->End() ; )  *po++ = *p1++ != *p2++;
}

template<typename T>
void iNotEqual(const Image<T>& img, const typename Image<T>::Pixel& pix, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  Image<T>::ConstIterator p = img.Begin();
  ImgBinary::Iterator q = out->Begin();
  while (q != out->End())  *q++ = (*p++ != pix);
}

template<typename T>
void iLessThan(const Image<T>& img1, const Image<T>& img2, ImgBinary* out)
{
  assert(IsSameSize(img1, img2));
  out->Reset(img1.Width(), img1.Height());
  Image<T>::ConstIterator p1 = img1.Begin();
  Image<T>::ConstIterator p2 = img2.Begin();
  ImgBinary::Iterator po = out->Begin();
  for ( ; po != out->End() ; )  *po++ = *p1++ < *p2++;
}

template<typename T>
void iLessThan(const Image<T>& img, const typename Image<T>::Pixel& pix, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  Image<T>::ConstIterator p = img.Begin();
  ImgBinary::Iterator q = out->Begin();
  while (q != out->End())  *q++ = (*p++ < pix);
}

template<typename T>
void iGreaterThan(const Image<T>& img1, const Image<T>& img2, ImgBinary* out)
{
  assert(IsSameSize(img1, img2));
  out->Reset(img1.Width(), img1.Height());
  Image<T>::ConstIterator p1 = img1.Begin();
  Image<T>::ConstIterator p2 = img2.Begin();
  ImgBinary::Iterator po = out->Begin();
  for ( ; po != out->End() ; )  *po++ = *p1++ > *p2++;
}

template<typename T>
void iGreaterThan(const Image<T>& img, const typename Image<T>::Pixel& pix, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  Image<T>::ConstIterator p = img.Begin();
  ImgBinary::Iterator q = out->Begin();
  while (q != out->End())  *q++ = (*p++ > pix);
}

template<typename T>
void iLessThanOrEqual(const Image<T>& img1, const Image<T>& img2, ImgBinary* out)
{
  assert(IsSameSize(img1, img2));
  out->Reset(img1.Width(), img1.Height());
  Image<T>::ConstIterator p1 = img1.Begin();
  Image<T>::ConstIterator p2 = img2.Begin();
  ImgBinary::Iterator po = out->Begin();
  for ( ; po != out->End() ; )  *po++ = *p1++ <= *p2++;
}

template<typename T>
void iLessThanOrEqual(const Image<T>& img, const typename Image<T>::Pixel& pix, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  Image<T>::ConstIterator p = img.Begin();
  ImgBinary::Iterator q = out->Begin();
  while (q != out->End())  *q++ = (*p++ <= pix);
}

template<typename T>
void iGreaterThanOrEqual(const Image<T>& img1, const Image<T>& img2, ImgBinary* out)
{
  assert(IsSameSize(img1, img2));
  out->Reset(img1.Width(), img1.Height());
  Image<T>::ConstIterator p1 = img1.Begin();
  Image<T>::ConstIterator p2 = img2.Begin();
  ImgBinary::Iterator po = out->Begin();
  for ( ; po != out->End() ; )  *po++ = *p1++ >= *p2++;
}

template<typename T>
void iGreaterThanOrEqual(const Image<T>& img, const typename Image<T>::Pixel& pix, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  Image<T>::ConstIterator p = img.Begin();
  ImgBinary::Iterator q = out->Begin();
  while (q != out->End())  *q++ = (*p++ >= pix);
}

template <typename T>
void iFindPixels(const Image<T>& img, typename Image<T>::Pixel value, std::vector<Point>* loc)
{
  int width = img.Width();
  int height = img.Height();

  Point pt;
  Image<T>::ConstIterator p;
  int i=0,j=0; 
  for(p=img.Begin(); p!=img.End(); p++)
  {
    if(*p == value)
    {
      pt.x = i;
      pt.y = j;
      loc->push_back(pt);
    }

    i++;
    if(i == width)
    {
      i=0;
      j++;
    }
  }
}

template<typename T>
void iResample(const Image<T>& img, int new_width, int new_height, Image<T>* out)
{
  InPlaceSwapper< Image<T> > inplace(img, &out);
  out->Reset(new_width, new_height);
  float fx = static_cast<float>(img.Width()) / out->Width();
  float fy = static_cast<float>(img.Height()) / out->Height();
  for (int y=0 ; y<out->Height() ; y++)
  {
    for (int x=0 ; x<out->Width() ; x++)
    {
      int xx = blepo_ex::Round(x * fx);
      int yy = blepo_ex::Round(y * fy);
      if (xx<0)  xx=0;
      if (xx>=img.Width())  xx=img.Width()-1;
      if (yy<0)  yy=0;
      if (yy>=img.Height())  yy=img.Height()-1;
      (*out)(x, y) = img(xx, yy);
    }
  }
}

template<typename T>
void iUpsample(const Image<T>& img, int factor_x, int factor_y, Image<T>* out)
{
  assert(factor_x >= 1 && factor_y >= 1);
  InPlaceSwapper< Image<T> > inplace(img, &out);
  out->Reset(img.Width()*factor_x, img.Height()*factor_y);
  for (int y=0 ; y<out->Height() ; y++)
  {
    for (int x=0 ; x<out->Width() ; x++)
    {
      (*out)(x, y) = img(x / factor_x, y / factor_y);
    }
  }
}

template<typename T>
void iDownsample(const Image<T>& img, int factor_x, int factor_y, Image<T>* out)
{
  assert(factor_x >= 1 && factor_y >= 1);
  InPlaceSwapper< Image<T> > inplace(img, &out);
  const int nw = (int) ceil(img.Width() / (double) factor_x);
  const int nh = (int) ceil(img.Height() / (double) factor_y);
  out->Reset(nw, nh);
  for (int y=0 ; y<nh ; y++)
  {
    for (int x=0 ; x<nw ; x++)
    {
      (*out)(x, y) = img(x * factor_x, y * factor_y);
    }
  }
}

template<typename T>
void iDownsample2x2(const Image<T>& img, Image<T>* out)
{
//  iDownsample(img, 2, 2, out);
  InPlaceSwapper< Image<T> > inplace(img, &out);
  out->Reset((img.Width()+1)/2, (img.Height()+1)/2);
  Image<T>::ConstIterator p = img.Begin();
  Image<T>::Iterator q = out->Begin();
  Image<T>::Iterator rowend = q + out->Width();
  int skip = img.Width() % 2;
  while (q != out->End())
  {
    while (q != rowend)
    {
      *q++ = *p++;
      p++;
    }
    p += img.Width() - skip;
    rowend += out->Width();
  }

#ifndef NDEBUG
  {
    Image<T> foo;
    iDownsample(img, 2, 2, &foo);
    assert( IsIdentical(foo, *out) );
  }
#endif
}

//template<typename T>
//void iDownsample2x2(const Image<T>& img, Image<T>* out)
//{
////  iDownsample(img, 2, 2, out);
//  InPlaceSwapper< Image<T> > inplace(img, &out);
//  out->Reset(img.Width()/2, img.Height()/2);
//  Image<T>::ConstIterator p = img.Begin();
//  Image<T>::Iterator q = out->Begin();
//  Image<T>::Iterator rowend = q + out->Width();
//  int skip = img.Width() % 2;
//  while (q != out->End())
//  {
//    while (q != rowend)
//    {
//      *q++ = *p++;
//      p++;
//    }
//    p += img.Width() + skip;
//    rowend += out->Width();
//  }
//
//#ifndef NDEBUG
//  {
//    Image<T> foo;
//    iDownsample(img, 2, 2, &foo);
//    assert( IsIdentical(foo, *out) );
//  }
//#endif
//}

//
//
//
//
//  Image<T>::ConstIterator p = img.Begin();
//  Image<T>::Iterator q = out->Begin();
//  int skip = img.Width() % 2;
//  for (int y=0 ; y<out->Height() ; y++)
//  {
//    for (int x=0 ; x<out->Width() ; x++)
//    {
//      assert(*p == img(x*2,y*2));
//      *q++ = *p++;
//      p++;
//    }
//    p += img.Width() + skip;
//  }
//
//  {
//    Image<T> foo = *out;
//    Image<T>::ConstIterator p = img.Begin();
//    Image<T>::Iterator q = foo.Begin();
//    Image<T>::Iterator rowend = q + out->Width();
//    int skip = img.Width() % 2;
//    while (q != foo.End())
//    {
//      while (q != rowend)
//      {
//        *q++ = *p++;
//        p++;
//      }
//      p += img.Width() + skip;
//      rowend += foo.Width();
//    }
//    assert( IsIdentical(foo, *out) );
//  }
//

// Class to convert Blepo image to Microsoft's BITMAPINFO
class iImgBitmapinfo
{
public:
  iImgBitmapinfo(const ImgBgr& img)
  {
    int width = img.Width();
    int height = img.Height();
    int nbits_per_pixel = ImgBgr::NBITS_PER_PIXEL;
    int nbytes = iComputeAlignment32(width, height, nbits_per_pixel);

    BITMAPINFOHEADER* bh = &(m_bmi.bmiHeader);
    bh->biSize = sizeof(BITMAPINFOHEADER);  
    bh->biWidth = width;
    bh->biHeight = -height;  // negative sign indicates top-down (right-side-up)
    bh->biPlanes = 1;
    bh->biBitCount = (WORD) nbits_per_pixel;
    bh->biCompression = BI_RGB;
    bh->biSizeImage = nbytes;
    bh->biXPelsPerMeter = 0;
    bh->biYPelsPerMeter = 0;
    bh->biClrUsed = 0;
    bh->biClrImportant = 0;	

    if (nbytes == width * height * 3)
    {
      m_data_ptr = reinterpret_cast<const unsigned char*>(img.Begin());
    }
    else
    {
      // align data to match expectations of StretchDIBits
      m_local_data.Reset(nbytes);
      ImgBgr::ConstIterator pi = img.Begin();
      for (int y=0 ; y<img.Height() ; y++)
      {
        unsigned char* po = m_local_data.Begin() + y*(nbytes / height);
        for (int x=0 ; x<img.Width() ; x++)
        {
          const ImgBgr::Pixel& pix = *pi++;
          *po++ = pix.b;
          *po++ = pix.g;
          *po++ = pix.r;
        }
      }
      m_data_ptr = m_local_data.Begin();
    }
  }
  const BITMAPINFO* GetBitmapInfo() const { return &m_bmi; }
  const unsigned char* GetDataPtr() const { return m_data_ptr; }
//  unsigned char* GetDataPtr() { return m_data_ptr; }

private:
  BITMAPINFO m_bmi;
  const unsigned char* m_data_ptr;
  Array<unsigned char> m_local_data;  // used only if bytes are not aligned
};

// From Gonzalez and Woods, Digital Image Processing, 2nd edition, 2002
inline void iBgrToHsv(double b, double g, double r, double* h, double* s, double* v)
{
  // h
  double numerator = 0.5 * ((r - g) + (r - b));
  double denominator = sqrt( (r-g)*(r-g) + (r-b)*(g-b) );
  double theta = acos( numerator / denominator ) / (2*blepo_ex::Pi);
  *h = (float) ( (b <= g) ? theta : 1 - theta );

  // s
  double minrgb = (r < g) ? r : g;
  minrgb = (minrgb < b) ? minrgb : b;
  *s = (float) ( 1 - 3 * minrgb / (r + g + b) );

  // v
  *v = (float) ( (r + g + b) / 3 );
}

inline void iHsvToBgr(double h, double s, double v, double* b, double* g, double* r)
{
  assert( h >= 0.0 && h <= 1.0 );
  assert( s >= 0.0 && s <= 1.0 );
  assert( v >= 0.0 && v <= 1.0 );
  static const double onethird  = 1.0 / 3.0;  // 120 degrees  ('static' improves computation speed)
  static const double twothirds = 2.0 / 3.0;  // 240 degrees
  static const double onesixth  = 1.0 / 6.0;  // 60 degrees
  double hh;

  // RG sector
  if (h < onethird)
  {
    hh = h;
  }

  // GB sector
  else if (h < twothirds)
  {
    hh = h - onethird;
  }

  // BR sector
  else
  {
    hh = h - twothirds;
  }

  double val1 = v * ( 1 - s );
  double val2 = v * ( 1 + ( s * cos(hh) ) / cos (onesixth - hh) );
  double val3 = 3 * v - ( val1 + val2 );
  
  // RG sector
  if (h < onethird)
  {
    *b = val1;
    *g = val3;
    *r = val2;
  }

  // GB sector
  else if (h < twothirds)
  {
    *b = val3;
    *g = val2;
    *r = val1;
  }

  // BR sector
  else
  {
    *b = val2;
    *g = val1;
    *r = val3;
  }
  
  // clamp
  *b = blepo_ex::Clamp((*b) * 255.0, 0.0, 255.0);
  *g = blepo_ex::Clamp((*g) * 255.0, 0.0, 255.0);
  *r = blepo_ex::Clamp((*r) * 255.0, 0.0, 255.0);
}

// templated version using pointers written by Peng Xu Oct 2008, based on version by STB
template <class T>
void iEnlargeByExtension(const T& img, int border, T* out)
{
  T::ConstIterator iter1, iter2;
  T::Iterator iterOut1,iterOut2;
  InPlaceSwapper< T > inplace(img, &out);
  
  assert( border >= 0 );
  const int w = img.Width(), h = img.Height();
  out->Reset(w + 2*border, h + 2*border);
  
  Set(out, Point(border, border), img);
  
  int i;
  int n;
  
  // Extend left right 
  iterOut1 = out->Begin(0, border);
  iterOut2 = out->Begin(border+w, border);
  
  iter1 = out->Begin(border, border);
  iter2 = out->Begin(border+w-1, border);
  
  // in order to save time, rearrange cycle order to access memory in sequence
  for ( i = border; i < h + border; i++)
  {
    for (n = border; n >0; n--)
    {
      *iterOut1++=*iter1;
      *iterOut2++=*iter2;
    }
    iter1+=w+2* border;
    iter2+=w+2* border;
    iterOut1+= w+border;
    iterOut2+= w+border;
  }
  
  // Extend top botom
  iterOut1 = out->Begin();
  iterOut2 = out->Begin(0, h + border);
  
  iter1 = out->Begin(0 , border);
  iter2 = out->Begin(0 , h+border-1);
  
  for (n = border; n >0; n--)
  {
    for ( i = 0; i < w + 2* border; i++)
    {
      *iterOut1++=*iter1++;
      *iterOut2++=*iter2++;
    }
    iter1-=w + 2* border;
    iter2-=w + 2* border;
  }
}

// templated version using pointers written by Peng Xu Oct 2008, based on version by STB
template <class T>
void iEnlargeByReflection(const T& img, int border, T* out)
{
  T::ConstIterator iter1, iter2;
  T::Iterator iterOut1,iterOut2;
  InPlaceSwapper< T > inplace(img, &out);
  
  assert( border >= 0 );
  
  const int w = img.Width(), h = img.Height();
  assert( border <= w && border<=h );
  out->Reset(w + 2*border, h + 2*border);
  Set(out, Point(border, border), img);
  
  int i;
  int n;
  
  // Extend left right 
  iterOut1 = out->Begin(0, border);
  iterOut2 = out->Begin(border+w, border);
  
  iter1 = out->Begin(2* border-1, border);
  iter2 = out->Begin(border+w-1, border);
  
  // in order to save time, rearrange cycle order to access memory in sequence
  for ( i = border; i < h + border; i++)
  {
    for (n = border; n >0; n--)
    {
      *iterOut1++=*iter1--;
      *iterOut2++=*iter2--;
    }
    iter1+=w+ 3* border;
    iter2+=w+ 3* border;
    iterOut1+= w+border;
    iterOut2+= w+border;
  }
  
  // Extend top botom
  iterOut1 = out->Begin();
  iterOut2 = out->Begin(0, h + border);
  
  iter1 = out->Begin(0 , 2* border-1);
  iter2 = out->Begin(0 , h+border-1);
  
  for (n = border; n >0; n--)
  {
    for ( i = 0; i < w + 2* border; i++)
    {
      *iterOut1++=*iter1++;
      *iterOut2++=*iter2++;
    }
    iter1-=2* (w + 2* border);
    iter2-=2* (w + 2* border);
  }
}

template <class T>
void iEnlargeByCropping(const T& img, int border, T* out)
{
  assert(border > 0);
  assert(img.Width() >= 2*border);
  assert(img.Height() >= 2*border);
  Extract(img, Rect(border, border, img.Width()-border, img.Height()-border), out);
}

//-------------------- nkk -------------------------------//
// in-place IS ok.
template <class T>
void iDilate3x3(const Image<T>& in, Image<T>* out)
{
  InPlaceSwapper< Image<T> > inplace(in, &out);
  *out = in;
  int i;
  
  Image<T>::ConstIterator n[8];
  n[0] = in.Begin(0,0);
  n[1] = in.Begin(1,0);
  n[2] = in.Begin(2,0);
  n[3] = in.Begin(0,1);
  n[4] = in.Begin(2,1);
  n[5] = in.Begin(0,2);
  n[6] = in.Begin(1,2);
  n[7] = in.Begin(2,2);
  Image<T>::Iterator out_iter = out->Begin(1,1);
  
  for(int v=1; v < in.Height()-1; v++)
  {		
    for(int u=1; u < in.Width()-1; u++)
    {
			Image<T>::Pixel max_val = *out_iter;
      for(i=0; i < 8; i++)
      {
				if(max_val == Image<T>::MAX_VAL)
				{
					break;
				}
        if(*(n[i]) > max_val)
        {
					max_val = *(n[i]);
        }
      }// i
      *out_iter = Image<T>::Pixel(max_val);

      for(i=0; i < 8; i++)
      {
        n[i]++;
      }
      
      out_iter++;
    }//u
    
    for(i=0; i < 8; i++)
    {
      n[i] += 2;
    }
    out_iter += 2;
  }
}

//-------------------- nkk -------------------------------//
// in-place IS ok.
template <class T>
void iErode3x3(const Image<T>& in, Image<T>* out)
{
  InPlaceSwapper< Image<T> > inplace(in, &out);
  *out = in;
  int i;
  
  Image<T>::ConstIterator n[8];
  n[0] = in.Begin(0,0);
  n[1] = in.Begin(1,0);
  n[2] = in.Begin(2,0);
  n[3] = in.Begin(0,1);
  n[4] = in.Begin(2,1);
  n[5] = in.Begin(0,2);
  n[6] = in.Begin(1,2);
  n[7] = in.Begin(2,2);
  Image<T>::Iterator out_iter = out->Begin(1,1);
  
  for(int v=1; v < in.Height()-1; v++)
  {		
    for(int u=1; u < in.Width()-1; u++)
    {
			Image<T>::Pixel min_val = *out_iter;
      for(i=0; i < 8; i++)
      {
				if(min_val == Image<T>::MIN_VAL)
				{
					break;
				}
        if(*(n[i]) < min_val)
        {
					min_val = *(n[i]);
        }
      }// i
      
			*out_iter = Image<T>::Pixel(min_val);

      for(i=0; i < 8; i++)
      {
        n[i]++;
      }
      
      out_iter++;
    }// u
    
    for(i=0; i < 8; i++)
    {
      n[i] += 2;
    }
    
    out_iter +=2;
  }
  
}

template <class T>
void iDilate3x3Cross(const Image<T>& in, Image<T>* out)
{
  assert(0);
  AfxMessageBox(L"non-MMX Dilate3x3Cross not implemented yet");
}

template <class T>
void iErode3x3Cross(const Image<T>& in, Image<T>* out)
{
  assert(0);
  AfxMessageBox(L"non-MMX Dilate3x3Cross not implemented yet");
}

template <class T>
void iFindTransitionPixels(const Image<T>& in, ImgBinary* out)
{
  out->Reset( in.Width(), in.Height() );
  Set(out, 0);
  for (int y=1 ; y<in.Height() ; y++)
  {
    for (int x=1 ; x<in.Width() ; x++)
    {
      const Image<T>::Pixel& pix = in(x,y);
      if ( (in(x-1, y) != pix) || (in(x, y-1) != pix) )  (*out)(x,y) = 1;
    }
  }
}

};
// ================< end local functions

namespace blepo
{

ImgFloat::Pixel Min(const ImgFloat& img) { return iMin(img); }
ImgGray ::Pixel Min(const ImgGray&  img) { return iMin(img); }
ImgInt  ::Pixel Min(const ImgInt&   img) { return iMin(img); }
ImgFloat::Pixel Max(const ImgFloat& img) { return iMax(img); }
ImgGray ::Pixel Max(const ImgGray&  img) { return iMax(img); }
ImgInt  ::Pixel Max(const ImgInt&   img) { return iMax(img); }
void Min(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out) { iMin(img1, img2, out); }
void Min(const ImgGray&  img1, const ImgGray&  img2, ImgGray*  out) { iMin(img1, img2, out); }
void Min(const ImgInt&   img1, const ImgInt&   img2, ImgInt*   out) { iMin(img1, img2, out); }
void Max(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out) { iMax(img1, img2, out); }
void Max(const ImgGray&  img1, const ImgGray&  img2, ImgGray*  out) { iMax(img1, img2, out); }
void Max(const ImgInt&   img1, const ImgInt&   img2, ImgInt*   out) { iMax(img1, img2, out); }
void MinMax(const ImgGray&  img, ImgGray ::Pixel* minn, ImgGray ::Pixel* maxx) { iMinMax(img, minn, maxx); }
void MinMax(const ImgInt&   img, ImgInt  ::Pixel* minn, ImgInt  ::Pixel* maxx) { iMinMax(img, minn, maxx); }
void MinMax(const ImgFloat& img, ImgFloat::Pixel* minn, ImgFloat::Pixel* maxx) { iMinMax(img, minn, maxx); }

//ImgGray::Pixel Min(const ImgGray& img)
//{
//  assert(img.Width() > 0 && img.Height() > 0);
//  ImgGray::Pixel minn = img(0, 0);
//  for (ImgGray::ConstIterator p = img.Begin() ; p != img.End() ; p++)  minn = blepo_ex::Min(minn, *p);
//  return minn;
//}
//
//ImgInt::Pixel Min(const ImgInt& img)
//{
//  assert(img.Width() > 0 && img.Height() > 0);
//  ImgInt::Pixel minn = img(0, 0);
//  for (ImgInt::ConstIterator p = img.Begin() ; p != img.End() ; p++)  minn = blepo_ex::Min(minn, *p);
//  return minn;
//}
//
//ImgFloat::Pixel Min(const ImgFloat& img)
//{
//  assert(img.Width() > 0 && img.Height() > 0);
//  ImgFloat::Pixel minn = img(0, 0);
//  for (ImgFloat::ConstIterator p = img.Begin() ; p != img.End() ; p++)  minn = blepo_ex::Min(minn, *p);
//  return minn;
//}


//ImgGray::Pixel Max(const ImgGray& img)
//{
//  assert(img.Width() > 0 && img.Height() > 0);
//  ImgGray::Pixel maxx = img(0, 0);
//  for (ImgGray::ConstIterator p = img.Begin() ; p != img.End() ; p++)  maxx = blepo_ex::Max(maxx, *p);
//  return maxx;
//}
//
//ImgInt::Pixel Max(const ImgInt& img)
//{
//  assert(img.Width() > 0 && img.Height() > 0);
//  ImgInt::Pixel maxx = img(0, 0);
//  for (ImgInt::ConstIterator p = img.Begin() ; p != img.End() ; p++)  maxx = blepo_ex::Max(maxx, *p);
//  return maxx;
//}
//
//ImgFloat::Pixel Max(const ImgFloat& img)
//{
//  assert(img.Width() > 0 && img.Height() > 0);
//  ImgFloat::Pixel maxx = img(0, 0);
//  for (ImgFloat::ConstIterator p = img.Begin() ; p != img.End() ; p++)  maxx = blepo_ex::Max(maxx, *p);
//  return maxx;
//}

//void Min(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out)
//{
//  assert(IsSameSize(img1, img2));
//  out->Reset(img1.Width(), img1.Height());
//  ImgFloat::ConstIterator p1 = img1.Begin();
//  ImgFloat::ConstIterator p2 = img2.Begin();
//  ImgFloat::Iterator po = out->Begin();
//  for ( ; po != out->End() ; )  *po++ = blepo_ex::Min(*p1++, *p2++);
//}
//
//void Min(const ImgGray& img1, const ImgGray& img2, ImgGray* out)
//{
//  assert(IsSameSize(img1, img2));
//  out->Reset(img1.Width(), img1.Height());
//  ImgGray::ConstIterator p1 = img1.Begin();
//  ImgGray::ConstIterator p2 = img2.Begin();
//  ImgGray::Iterator po = out->Begin();
//  for ( ; po != out->End() ; )  *po++ = blepo_ex::Min(*p1++, *p2++);
//}
//
//void Min(const ImgInt& img1, const ImgInt& img2, ImgInt* out)
//{
//  assert(IsSameSize(img1, img2));
//  out->Reset(img1.Width(), img1.Height());
//  ImgInt::ConstIterator p1 = img1.Begin();
//  ImgInt::ConstIterator p2 = img2.Begin();
//  ImgInt::Iterator po = out->Begin();
//  for ( ; po != out->End() ; )  *po++ = blepo_ex::Min(*p1++, *p2++);
//}
//
//void Max(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out)
//{
//  assert(IsSameSize(img1, img2));
//  out->Reset(img1.Width(), img1.Height());
//  ImgFloat::ConstIterator p1 = img1.Begin();
//  ImgFloat::ConstIterator p2 = img2.Begin();
//  ImgFloat::Iterator po = out->Begin();
//  for ( ; po != out->End() ; )  *po++ = blepo_ex::Max(*p1++, *p2++);
//}
//
//void Max(const ImgGray& img1, const ImgGray& img2, ImgGray* out)
//{
//  assert(IsSameSize(img1, img2));
//  out->Reset(img1.Width(), img1.Height());
//  ImgGray::ConstIterator p1 = img1.Begin();
//  ImgGray::ConstIterator p2 = img2.Begin();
//  ImgGray::Iterator po = out->Begin();
//  for ( ; po != out->End() ; )  *po++ = blepo_ex::Max(*p1++, *p2++);
//}
//
//void Max(const ImgInt& img1, const ImgInt& img2, ImgInt* out)
//{
//  assert(IsSameSize(img1, img2));
//  out->Reset(img1.Width(), img1.Height());
//  ImgInt::ConstIterator p1 = img1.Begin();
//  ImgInt::ConstIterator p2 = img2.Begin();
//  ImgInt::Iterator po = out->Begin();
//  for ( ; po != out->End() ; )  *po++ = blepo_ex::Max(*p1++, *p2++);
//}


//void MinMax(const ImgGray& img, ImgGray::Pixel* minn, ImgGray::Pixel* maxx)
//{
//  assert(img.Width() > 0 && img.Height() > 0);
//  *minn = *maxx = img(0, 0);
//  for (ImgGray::ConstIterator p = img.Begin() ; p != img.End() ; p++)
//  {
//    ImgGray::Pixel pix = *p;
//    *minn = blepo_ex::Min(*minn, pix);
//    *maxx = blepo_ex::Max(*maxx, pix);
//  }
//}
//
//void MinMax(const ImgInt& img, ImgInt::Pixel* minn, ImgInt::Pixel* maxx)
//{
//  assert(img.Width() > 0 && img.Height() > 0);
//  *minn = *maxx = img(0, 0);
//  for (ImgInt::ConstIterator p = img.Begin() ; p != img.End() ; p++)
//  {
//    ImgInt::Pixel pix = *p;
//    *minn = blepo_ex::Min(*minn, pix);
//    *maxx = blepo_ex::Max(*maxx, pix);
//  }
//}
//
//void MinMax(const ImgFloat& img, ImgFloat::Pixel* minn, ImgFloat::Pixel* maxx)
//{
//  assert(img.Width() > 0 && img.Height() > 0);
//  *minn = *maxx = img(0, 0);
//  for (ImgFloat::ConstIterator p = img.Begin() ; p != img.End() ; p++)
//  {
//    ImgFloat::Pixel pix = *p;
//    *minn = blepo_ex::Min(*minn, pix);
//    *maxx = blepo_ex::Max(*maxx, pix);
//  }
//}

void And(const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iAnd(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
}

void And(const ImgBgr& img1, const ImgBgr& img2, ImgBgr* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iAnd(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
}

void And(const ImgGray& img1, const ImgGray& img2, ImgGray* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iAnd(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
}

void And(const ImgInt& img1, const ImgInt& img2, ImgInt* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iAnd(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
}

void And(const ImgBgr& img, const ImgBinary& mask, ImgBgr* out)
{
  ImgBgr tmp;
  Convert(mask, &tmp);
  And(img, tmp, out);
//  if (!IsSameSize(img, mask))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img.Width(), img.Height());
//  ImgBgr::ConstIterator p = img.Begin();
//  ImgBinary::ConstIterator m = mask.Begin();
//  ImgBgr::Iterator q = out->Begin();
//  for ( ; p != img.End() ; p++, m++, q++)  *q = (*m) ? *p : Bgr(0,0,0);
}

void And(const ImgGray& img, const ImgBinary& mask, ImgGray* out)
{
  ImgGray tmp;
  Convert(mask, &tmp);
  And(img, tmp, out);
//  if (!IsSameSize(img, mask))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img.Width(), img.Height());
//  ImgBgr::ConstIterator p = img.Begin();
//  ImgBinary::ConstIterator m = mask.Begin();
//  ImgBgr::Iterator q = out->Begin();
//  for ( ; p != img.End() ; p++, m++, q++)  *q = (*m) ? *p : Bgr(0,0,0);
}

void And(const ImgInt& img, const ImgBinary& mask, ImgInt* out)
{
  if (!IsSameSize(img, mask))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img.Width(), img.Height());
  ImgInt::ConstIterator p = img.Begin();
  ImgBinary::ConstIterator m = mask.Begin();
  ImgInt::Iterator q = out->Begin();
  for ( ; p != img.End() ; p++, m++, q++)  *q = (*m) ? *p : 0;
}

void Or(const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iOr(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
}

void Or(const ImgBgr& img1, const ImgBgr& img2, ImgBgr* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iOr(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
}

void Or(const ImgGray& img1, const ImgGray& img2, ImgGray* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iOr(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
}

void Or(const ImgInt& img1, const ImgInt& img2, ImgInt* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iOr(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
}

void Xor(const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iXor(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
}

void Xor(const ImgBgr& img1, const ImgBgr& img2, ImgBgr* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iXor(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
}

void Xor(const ImgGray& img1, const ImgGray& img2, ImgGray* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iXor(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
}

void Xor(const ImgInt& img1, const ImgInt& img2, ImgInt* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iXor(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
}

void Not(const ImgBinary& img, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  int nbytes = img.NBytes();
  iNot(img.BytePtr(), out->BytePtr(), nbytes); 
}

void Not(const ImgBgr& img, ImgBgr* out)
{
  out->Reset(img.Width(), img.Height());
  int nbytes = img.NBytes();
  iNot(img.BytePtr(), out->BytePtr(), nbytes); 
}

void Not(const ImgGray& img, ImgGray* out)
{
  out->Reset(img.Width(), img.Height());
  int nbytes = img.NBytes();
  iNot(img.BytePtr(), out->BytePtr(), nbytes); 
}

void Not(const ImgInt& img, ImgInt* out)
{
  out->Reset(img.Width(), img.Height());
  int nbytes = img.NBytes();
  iNot(img.BytePtr(), out->BytePtr(), nbytes); 
}

void AbsDiff(const ImgGray& img1, const ImgGray& img2, ImgGray* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iAbsDiff(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes);
}

void AbsDiff(const ImgBgr& img1, const ImgBgr& img2, ImgBgr* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iAbsDiff(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes);
}

// specializations
//void And(const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out) { iiAnd(img1, img2, out); }
//void And(const ImgBgr   & img1, const ImgBgr   & img2, ImgBgr   * out) { iiAnd(img1, img2, out); }
//void And(const ImgGray  & img1, const ImgGray  & img2, ImgGray  * out) { iiAnd(img1, img2, out); }
//void And(const ImgInt   & img1, const ImgInt   & img2, ImgInt   * out) { iiAnd(img1, img2, out); }
//void Or (const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out) { iiOr (img1, img2, out); }
//void Or (const ImgBgr   & img1, const ImgBgr   & img2, ImgBgr   * out) { iiOr (img1, img2, out); }
//void Or (const ImgGray  & img1, const ImgGray  & img2, ImgGray  * out) { iiOr (img1, img2, out); }
//void Or (const ImgInt   & img1, const ImgInt   & img2, ImgInt   * out) { iiOr (img1, img2, out); }
//void Xor(const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out) { iiXor(img1, img2, out); }
//void Xor(const ImgBgr   & img1, const ImgBgr   & img2, ImgBgr   * out) { iiXor(img1, img2, out); }
//void Xor(const ImgGray  & img1, const ImgGray  & img2, ImgGray  * out) { iiXor(img1, img2, out); }
//void Xor(const ImgInt   & img1, const ImgInt   & img2, ImgInt   * out) { iiXor(img1, img2, out); }
//void And(const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out) //{ iiOp<iAndOperator>(img1, img2, out); }
////template <typename U, typename T>
////void iiOp(const Image<T>& img1, const Image<T>& img2, Image<T>* out)
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iAndOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//void And(const ImgBgr   & img1, const ImgBgr   & img2, ImgBgr   * out) //{ iiOp<iAndOperator>(img1, img2, out); }
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iAndOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//void And(const ImgGray  & img1, const ImgGray  & img2, ImgGray  * out) //{ iiOp<iAndOperator>(img1, img2, out); }
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iAndOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//void And(const ImgInt   & img1, const ImgInt   & img2, ImgInt   * out) //{ iiOp<iAndOperator>(img1, img2, out); }
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iAndOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//void Or (const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out) //{ iiOp<iOrOperator> (img1, img2, out); }
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iOrOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//void Or (const ImgBgr   & img1, const ImgBgr   & img2, ImgBgr   * out) //{ iiOp<iOrOperator> (img1, img2, out); }
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iOrOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//void Or (const ImgGray  & img1, const ImgGray  & img2, ImgGray  * out) //{ iiOp<iOrOperator> (img1, img2, out); }
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iOrOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//void Or (const ImgInt   & img1, const ImgInt   & img2, ImgInt   * out) //{ iiOp<iOrOperator> (img1, img2, out); }
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iOrOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//void Xor(const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out) //{ iiOp<iXorOperator>(img1, img2, out); }
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iXorOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//void Xor(const ImgBgr   & img1, const ImgBgr   & img2, ImgBgr   * out) //{ iiOp<iXorOperator>(img1, img2, out); }
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iXorOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//void Xor(const ImgGray  & img1, const ImgGray  & img2, ImgGray  * out) //{ iiOp<iXorOperator>(img1, img2, out); }
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iXorOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//void Xor(const ImgInt   & img1, const ImgInt   & img2, ImgInt   * out) //{ iiOp<iXorOperator>(img1, img2, out); }
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iXorOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//void Not(const ImgBinary& img, ImgBinary* out) { iiNot(img, out); }
////{
////  out->Reset(img.Width(), img.Height());
////  int nbytes = img.NBytes();
////  iOp<iNotOperator>(img.BytePtr(), out->BytePtr(), nbytes); 
////}
//
//void Not(const ImgBgr   & img, ImgBgr   * out) { iiNot(img, out); }
////{
////  out->Reset(img.Width(), img.Height());
////  int nbytes = img.NBytes();
////  iOp<iNotOperator>(img.BytePtr(), out->BytePtr(), nbytes); 
////}
////
//void Not(const ImgGray  & img, ImgGray  * out) { iiNot(img, out); }
////{
////  out->Reset(img.Width(), img.Height());
////  int nbytes = img.NBytes();
////  iOp<iNotOperator>(img.BytePtr(), out->BytePtr(), nbytes); 
////}
////
//void Not(const ImgInt   & img, ImgInt   * out) { iiNot(img, out); }
////{
////  out->Reset(img.Width(), img.Height());
////  int nbytes = img.NBytes();
////  iOp<iNotOperator>(img.BytePtr(), out->BytePtr(), nbytes); 
////}
////
//void AbsDiff(const ImgBgr   & img1, const ImgBgr   & img2, ImgBgr   * out) //{ iiOp<iAbsDiffOperator>(img1, img2, out); }
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iAbsDiffOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}
//
//void AbsDiff(const ImgGray  & img1, const ImgGray  & img2, ImgGray  * out) //{ iiOp<iAbsDiffOperator>(img1, img2, out); }
//{
//  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
//  out->Reset(img1.Width(), img1.Height());
//  int nbytes = img1.NBytes();
//  iOp<iAbsDiffOperator>(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes); 
//}

void And(const ImgBinary& img, ImgBinary::Pixel val, ImgBinary* out)
{
  out->Reset( img.Width(), img.Height() );
  unsigned char v = val ? 0xFF : 0x00;
  iConstAnd( img.BytePtr(), v, out->BytePtr(), img.NBytes() ); 
}

void And(const ImgBgr& img, ImgBgr::Pixel val, ImgBgr* out)
{
  out->Reset( img.Width(), img.Height() );
  if (val.b == val.g == val.r)
  {
    iConstAnd( img.BytePtr(), val.b, out->BytePtr(), img.NBytes() ); 
  }
  else
  {
    ImgBgr::ConstIterator p = img.Begin();
    ImgBgr::Iterator q = out->Begin();
    ImgBgr::Pixel pix;
    while (p != img.End())
    {
      pix.b = p->b & val.b;
      pix.g = p->g & val.g;
      pix.r = p->r & val.r;
      *q++ = pix;
      p++;
    }
  }
}

void And(const ImgGray& img, ImgGray::Pixel val, ImgBinary* out)
{
  out->Reset( img.Width(), img.Height() );
  iConstAnd( img.BytePtr(), val, out->BytePtr(), img.NBytes() ); 
}

void And(const ImgInt& img, ImgInt::Pixel val, ImgInt* out)
{
  out->Reset( img.Width(), img.Height() );
  ImgInt::ConstIterator p = img.Begin();
  ImgInt::Iterator q = out->Begin();
  while ( p != img.End() )  *q++ = *p++ & val;
}

void Or(const ImgBinary& img, ImgBinary::Pixel val, ImgBinary* out)
{
  out->Reset( img.Width(), img.Height() );
  unsigned char v = val ? 0xFF : 0x00;
  iConstOr( img.BytePtr(), v, out->BytePtr(), img.NBytes() ); 
}

void Or(const ImgBgr& img, ImgBgr::Pixel val, ImgBgr* out)
{
  out->Reset( img.Width(), img.Height() );
  if (val.b == val.g == val.r)
  {
    iConstOr( img.BytePtr(), val.b, out->BytePtr(), img.NBytes() ); 
  }
  else
  {
    ImgBgr::ConstIterator p = img.Begin();
    ImgBgr::Iterator q = out->Begin();
    ImgBgr::Pixel pix;
    while (p != img.End())
    {
      pix.b = p->b | val.b;
      pix.g = p->g | val.g;
      pix.r = p->r | val.r;
      *q++ = pix;
      p++;
    }
  }
}

void Or(const ImgGray& img, ImgGray::Pixel val, ImgBinary* out)
{
  out->Reset( img.Width(), img.Height() );
  iConstOr( img.BytePtr(), val, out->BytePtr(), img.NBytes() ); 
}

void Or(const ImgInt& img, ImgInt::Pixel val, ImgInt* out)
{
  out->Reset( img.Width(), img.Height() );
  ImgInt::ConstIterator p = img.Begin();
  ImgInt::Iterator q = out->Begin();
  while ( p != img.End() )  *q++ = *p++ | val;
}

void Xor(const ImgBinary& img, ImgBinary::Pixel val, ImgBinary* out)
{
  out->Reset( img.Width(), img.Height() );
  unsigned char v = val ? 0xFF : 0x00;
  iConstXor( img.BytePtr(), v, out->BytePtr(), img.NBytes() ); 
}

void Xor(const ImgBgr& img, ImgBgr::Pixel val, ImgBgr* out)
{
  out->Reset( img.Width(), img.Height() );
  if (val.b == val.g == val.r)
  {
    iConstXor( img.BytePtr(), val.b, out->BytePtr(), img.NBytes() ); 
  }
  else
  {
    ImgBgr::ConstIterator p = img.Begin();
    ImgBgr::Iterator q = out->Begin();
    ImgBgr::Pixel pix;
    while (p != img.End())
    {
      pix.b = p->b ^ val.b;
      pix.g = p->g ^ val.g;
      pix.r = p->r ^ val.r;
      *q++ = pix;
      p++;
    }
  }
}

void Xor(const ImgGray& img, ImgGray::Pixel val, ImgBinary* out)
{
  out->Reset( img.Width(), img.Height() );
  iConstXor( img.BytePtr(), val, out->BytePtr(), img.NBytes() ); 
}

void Xor(const ImgInt& img, ImgInt::Pixel val, ImgInt* out)
{
  out->Reset( img.Width(), img.Height() );
  ImgInt::ConstIterator p = img.Begin();
  ImgInt::Iterator q = out->Begin();
  while ( p != img.End() )  *q++ = *p++ ^ val;
}

void AbsDiff(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  ImgFloat::ConstIterator p1 = img1.Begin();
  ImgFloat::ConstIterator p2 = img2.Begin();
  ImgFloat::Iterator q = out->Begin();
  while (p1 != img1.End())  *q++ = blepo_ex::Abs( (*p1++) - (*p2++) );
}

void AbsDiff(const ImgInt  & img1, const ImgInt  & img2, ImgInt  * out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  ImgInt::ConstIterator p1 = img1.Begin();
  ImgInt::ConstIterator p2 = img2.Begin();
  ImgInt::Iterator q = out->Begin();
  while (p1 != img1.End())  *q++ = blepo_ex::Abs( (*p1++) - (*p2++) );
}

//void Add(const ImgBgr & img1, const ImgBgr & img2, ImgBgr * out) { iiOp<iSaturatedSumOperator>(img1, img2, out); }
//void Add(const ImgGray& img1, const ImgGray& img2, ImgGray* out) { iiOp<iSaturatedSumOperator>(img1, img2, out); }

void Add(const ImgBgr& img1, const ImgBgr& img2, ImgBgr* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iSaturatedSum(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes);
}

void Add(const ImgGray& img1, const ImgGray& img2, ImgGray* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iSaturatedSum(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes);
}

void Add(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  ImgFloat::ConstIterator p1 = img1.Begin();
  ImgFloat::ConstIterator p2 = img2.Begin();
  ImgFloat::Iterator q = out->Begin();
  while (p1 != img1.End())  *q++ = (*p1++) + (*p2++);
}

void Add(const ImgInt& img1, const ImgInt& img2, ImgInt* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  ImgInt::ConstIterator p1 = img1.Begin();
  ImgInt::ConstIterator p2 = img2.Begin();
  ImgInt::Iterator q = out->Begin();
  while (p1 != img1.End())  *q++ = (*p1++) + (*p2++);
}

//void Subtract(const ImgBgr & img1, const ImgBgr & img2, ImgBgr * out) { iiOp<iSaturatedSubtractOperator>(img1, img2, out); }
//void Subtract(const ImgGray& img1, const ImgGray& img2, ImgGray* out) { iiOp<iSaturatedSubtractOperator>(img1, img2, out); }

void Subtract(const ImgBgr& img1, const ImgBgr& img2, ImgBgr* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iSaturatedSubtract(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes);
}

void Subtract(const ImgGray& img1, const ImgGray& img2, ImgGray* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  int nbytes = img1.NBytes();
  iSaturatedSubtract(img1.BytePtr(), img2.BytePtr(), out->BytePtr(), nbytes);
}

void Subtract(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  ImgFloat::ConstIterator p1 = img1.Begin();
  ImgFloat::ConstIterator p2 = img2.Begin();
  ImgFloat::Iterator q = out->Begin();
  while (p1 != img1.End())  *q++ = (*p1++) - (*p2++);
}

void Subtract(const ImgInt& img1, const ImgInt& img2, ImgInt* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  ImgInt::ConstIterator p1 = img1.Begin();
  ImgInt::ConstIterator p2 = img2.Begin();
  ImgInt::Iterator q = out->Begin();
  while (p1 != img1.End())  *q++ = (*p1++) - (*p2++);
}

void Invert(const ImgBgr& img, ImgBgr* out)
{
  out->Reset(img.Width(), img.Height());
  int nbytes = img.NBytes();
  ImgBgr::ConstIterator p = img.Begin();
  ImgBgr::Iterator q = out->Begin();
  while (p != img.End())
  {
	  q->b = 255 - p->b;
	  q->g = 255 - p->g;
	  q->r = 255 - p->r;
	  p++;
	  q++;
  }
}

void Invert(const ImgFloat& img, ImgFloat* out)
{
  out->Reset(img.Width(), img.Height());
  int nbytes = img.NBytes();
  ImgFloat::ConstIterator p = img.Begin();
  ImgFloat::Iterator q = out->Begin();
  while (p != img.End())  *q++ = blepo_ex::Clamp(1.0f - *p++, 0.0f, 1.0f);
}

void Invert(const ImgGray& img, ImgGray* out)
{
  out->Reset(img.Width(), img.Height());
  int nbytes = img.NBytes();
  ImgGray::ConstIterator p = img.Begin();
  ImgGray::Iterator q = out->Begin();
  while (p != img.End())  *q++ = 255 - *p++;
}

void Multiply(const ImgFloat& img1, const ImgFloat& img2, ImgFloat* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  ImgFloat::ConstIterator p1 = img1.Begin();
  ImgFloat::ConstIterator p2 = img2.Begin();
  ImgFloat::Iterator q = out->Begin();
  while (p1 != img1.End())  *q++ = (*p1++) * (*p2++);
}

void Multiply(const ImgInt& img1, const ImgInt& img2, ImgInt* out)
{
  if (!IsSameSize(img1, img2))  BLEPO_ERROR("Images must be of the same size");
  out->Reset(img1.Width(), img1.Height());
  ImgInt::ConstIterator p1 = img1.Begin();
  ImgInt::ConstIterator p2 = img2.Begin();
  ImgInt::Iterator q = out->Begin();
  while (p1 != img1.End())  *q++ = (*p1++) * (*p2++);
}

void Add(const ImgGray& img, ImgGray::Pixel val, ImgGray* out)
{
  out->Reset(img.Width(), img.Height());
  ImgGray::ConstIterator p = img.Begin();
  ImgGray::Iterator q = out->Begin();
  while (p != img.End())  *q++ = blepo_ex::Clamp( ((*p++) + val) , 0, 255);
}

void Add(const ImgFloat& img, ImgFloat::Pixel val, ImgFloat* out)
{
  out->Reset(img.Width(), img.Height());
  ImgFloat::ConstIterator p = img.Begin();
  ImgFloat::Iterator q = out->Begin();
  while (p != img.End())  *q++ = (*p++) + val;
}

void Add(const ImgInt& img, ImgInt::Pixel val, ImgInt* out)
{
  out->Reset(img.Width(), img.Height());
  ImgInt::ConstIterator p = img.Begin();
  ImgInt::Iterator q = out->Begin();
  while (p != img.End())  *q++ = (*p++) + val;
}

void Subtract(const ImgGray& img, ImgGray::Pixel val, ImgGray* out)
{
  out->Reset(img.Width(), img.Height());
  ImgGray::ConstIterator p = img.Begin();
  ImgGray::Iterator q = out->Begin();
  while (p != img.End())  *q++ = blepo_ex::Clamp( ((*p++) - val) , 0, 255);
}

void Subtract(const ImgFloat& img, ImgFloat::Pixel val, ImgFloat* out)
{
  out->Reset(img.Width(), img.Height());
  ImgFloat::ConstIterator p = img.Begin();
  ImgFloat::Iterator q = out->Begin();
  while (p != img.End())  *q++ = (*p++) - val;
}

void Subtract(const ImgInt& img, ImgInt::Pixel val, ImgInt* out)
{
  out->Reset(img.Width(), img.Height());
  ImgInt::ConstIterator p = img.Begin();
  ImgInt::Iterator q = out->Begin();
  while (p != img.End())  *q++ = (*p++) - val;
}

void Multiply(const ImgGray& img, ImgGray::Pixel val, ImgGray* out)
{
  out->Reset(img.Width(), img.Height());
  ImgGray::ConstIterator p = img.Begin();
  ImgGray::Iterator q = out->Begin();
  while (p != img.End())  *q++ = blepo_ex::Clamp( ((*p++) * val) , 0, 255);
}

void Multiply(const ImgFloat& img, ImgFloat::Pixel val, ImgFloat* out)
{
  out->Reset(img.Width(), img.Height());
  ImgFloat::ConstIterator p = img.Begin();
  ImgFloat::Iterator q = out->Begin();
  while (p != img.End())  *q++ = (*p++) * val;
}

void Multiply(const ImgInt& img, ImgInt::Pixel val, ImgInt* out)
{
  out->Reset(img.Width(), img.Height());
  ImgInt::ConstIterator p = img.Begin();
  ImgInt::Iterator q = out->Begin();
  while (p != img.End())  *q++ = (*p++) * val;
}

void Divide(const ImgFloat& img, ImgFloat::Pixel val, ImgFloat* out)
{
  out->Reset(img.Width(), img.Height());
  ImgFloat::ConstIterator p = img.Begin();
  ImgFloat::Iterator q = out->Begin();
  while (p != img.End())  *q++ = (*p++) / val;
}

void Divide(const ImgInt& img, ImgInt::Pixel val, ImgInt* out)
{
  out->Reset(img.Width(), img.Height());
  ImgInt::ConstIterator p = img.Begin();
  ImgInt::Iterator q = out->Begin();
  while (p != img.End())  *q++ = (*p++) / val;
}

void Log10(const ImgFloat& img, ImgFloat* out)
{
  out->Reset(img.Width(), img.Height());
  ImgFloat::ConstIterator p = img.Begin();
  ImgFloat::Iterator q = out->Begin();
  while (p != img.End())  *q++ = log(*p++);
}

void LinearlyScale(const ImgGray& img, ImgGray::Pixel minval, ImgGray::Pixel maxval, ImgGray* out)
{
  out->Reset(img.Width(), img.Height());
  if (img.Width()>0 && img.Height()>0)
  {
    ImgGray::Pixel minn, maxx;
    MinMax(img, &minn, &maxx);
    if (minn == maxx)  return;
    float scale = ((float) maxval - minval) / (maxx - minn);
    ImgGray::ConstIterator p = img.Begin();
    ImgGray::Iterator q = out->Begin();
    int v;
    while (p != img.End())
    { 
      v = blepo_ex::Round( (*p++ - minn) * scale + minval );
      *q++ = blepo_ex::Clamp(v, 0, 255);
    }
  }
}

void LinearlyScale(const ImgFloat& img, float minval, float maxval, ImgFloat* out)
{
  out->Reset(img.Width(), img.Height());
  if (img.Width()>0 && img.Height()>0)
  {
    float minn, maxx;
    MinMax(img, &minn, &maxx);
    if (minn == maxx)  return;
    float scale = (maxval - minval) / (maxx - minn);
    ImgFloat::ConstIterator p = img.Begin();
    ImgFloat::Iterator q = out->Begin();
    while (p != img.End())  *q++ = (*p++ - minn) * scale + minval;
  }
}

void LinearlyScale(const ImgInt& img, int minval, int maxval, ImgInt* out)
{
  out->Reset(img.Width(), img.Height());
  if (img.Width()>0 && img.Height()>0)
  {
    int minn, maxx;
    MinMax(img, &minn, &maxx);
    if (minn == maxx)  return;
    float scale = static_cast<float>(maxval - minval) / (maxx - minn);
    ImgInt::ConstIterator p = img.Begin();
    ImgInt::Iterator q = out->Begin();
	  while (p != img.End())  *q++ = blepo_ex::Round( (*p++ - minn) * scale + minval );
  }
}

void Clamp(const ImgGray& img, ImgGray::Pixel minval, ImgGray::Pixel maxval, ImgGray* out)
{
  out->Reset(img.Width(), img.Height());
  ImgGray::ConstIterator p = img.Begin();
  ImgGray::Iterator q = out->Begin();
  while (p != img.End())  *q++ = blepo_ex::Clamp(*p++, minval, maxval);
}

void Clamp(const ImgFloat& img, float minval, float maxval, ImgFloat* out)
{
  out->Reset(img.Width(), img.Height());
  ImgFloat::ConstIterator p = img.Begin();
  ImgFloat::Iterator q = out->Begin();
  while (p != img.End())  *q++ = blepo_ex::Clamp(*p++, minval, maxval);
}

void Clamp(const ImgInt& img, int minval, int maxval, ImgInt* out)
{
  out->Reset(img.Width(), img.Height());
  ImgInt::ConstIterator p = img.Begin();
  ImgInt::Iterator q = out->Begin();
  while (p != img.End())  *q++ = blepo_ex::Clamp(*p++, minval, maxval);
}

//---------------------------------------------------//
void Erode3x3(const ImgGray& img, ImgGray* out)
{
  InPlaceSwapper<ImgGray> inplace(img, &out);
  out->Reset(img.Width(), img.Height());
  if ( blepo::CanDoMmx() &&  (img.NBytes() >=8)  )
  {
    Array<unsigned char> tmplate(12);
    tmplate[0] = 1;	tmplate[1] = 1;	tmplate[2] = 1; tmplate[3] = 0;
    tmplate[4] = 1;	tmplate[5] = 1;	tmplate[6] = 1; tmplate[7] = 0;
    tmplate[8] = 1;	tmplate[9] = 1;	tmplate[10] = 1; tmplate[11] = 0;
    mmx_erode(img.Begin(), tmplate.Begin(), out->Begin(), img.Width(), img.Height());
  }
  {
    iErode3x3(img, out);
  }
}

void Erode3x3Cross(const ImgGray& img, ImgGray* out)
{
  InPlaceSwapper<ImgGray> inplace(img, &out);
  out->Reset(img.Width(), img.Height());
  if ( blepo::CanDoMmx() &&  (img.NBytes() >=8)  )
  {
    Array<unsigned char> tmplate(12);
    tmplate[0] = 0;	tmplate[1] = 1;	tmplate[2] = 0; tmplate[3] = 0;
    tmplate[4] = 1;	tmplate[5] = 1;	tmplate[6] = 1; tmplate[7] = 0;
    tmplate[8] = 0;	tmplate[9] = 1;	tmplate[10] = 0; tmplate[11] = 0;
    mmx_erode(img.Begin(), tmplate.Begin(), out->Begin(), img.Width(), img.Height());
  }
  else
  {
    iErode3x3Cross(img, out);
  }
}

void Dilate3x3(const ImgGray& img, ImgGray* out)
{
  InPlaceSwapper<ImgGray> inplace(img, &out);
  out->Reset(img.Width(), img.Height());
  if ( blepo::CanDoMmx() &&  (img.NBytes() >=8)  )
  {
    Array<unsigned char> tmplate(12);
    tmplate[0] = 1;	tmplate[1] = 1;	tmplate[2] = 1; tmplate[3] = 0;
    tmplate[4] = 1;	tmplate[5] = 1;	tmplate[6] = 1; tmplate[7] = 0;
    tmplate[8] = 1;	tmplate[9] = 1;	tmplate[10] = 1; tmplate[11] = 0;
    mmx_dilate(img.Begin(), tmplate.Begin(), out->Begin(), img.Width(), img.Height());
  }
  else
  {
    iDilate3x3(img, out);
  }
}

void Dilate3x3Cross(const ImgGray& img, ImgGray* out)
{
  InPlaceSwapper<ImgGray> inplace(img, &out);
  out->Reset(img.Width(), img.Height());
  if ( blepo::CanDoMmx() &&  (img.NBytes() >=8)  )
  {
    Array<unsigned char> tmplate(12);
    tmplate[0] = 0;	tmplate[1] = 1;	tmplate[2] = 0; tmplate[3] = 0;
    tmplate[4] = 1;	tmplate[5] = 1;	tmplate[6] = 1; tmplate[7] = 0;
    tmplate[8] = 0;	tmplate[9] = 1;	tmplate[10] = 0; tmplate[11] = 0;
    mmx_dilate(img.Begin(), tmplate.Begin(), out->Begin(), img.Width(), img.Height());
  }
  else
  {
    iDilate3x3Cross(img, out);
  }
}

void Erode3x3(const ImgBinary& img, ImgBinary* out)
{
  if ( blepo::CanDoMmx() )
  { 
    ImgGray img1, img2;
    Convert(img, &img1);
    Erode3x3(img1, &img2);
    Convert(img2, out);
  }
  else
  {
    iErode3x3(img, out);
  }
}

void Erode3x3Cross(const ImgBinary& img, ImgBinary* out)
{
  if ( blepo::CanDoMmx() )
  { 
    ImgGray img1, img2;
    Convert(img, &img1);
    Erode3x3Cross(img1, &img2);
    Convert(img2, out);
  }
  else
  {
    iErode3x3Cross(img, out);
  }
}

void Dilate3x3(const ImgBinary& img, ImgBinary* out)
{
  if ( blepo::CanDoMmx() )
  { 
    ImgGray img1, img2;
    Convert(img, &img1);
    Dilate3x3(img1, &img2);
    Convert(img2, out);
  }
  else
  {
    iDilate3x3(img, out);
  }
}

void Dilate3x3Cross(const ImgBinary& img, ImgBinary* out)
{
  if ( blepo::CanDoMmx() )
  { 
    ImgGray img1, img2;
    Convert(img, &img1);
    Dilate3x3Cross(img1, &img2);
    Convert(img2, out);
  }
  else
  {
    iDilate3x3Cross(img, out);
  }
}

// grayscale erosion
void GrayscaleErode3x3(const ImgGray& img, int offset, ImgGray* out)
{
  InPlaceSwapper< ImgGray > inplace(img, &out);
  out->Reset(img.Width(), img.Height());
  *out = img;  // to copy border (inefficient but works)
  const int w = 1;  // halfwidth of 3x3 kernel
  for (int y=w ; y<img.Height()-w ; y++)
  {
    for (int x=w ; x<img.Width()-w ; x++)
    {
      int minn = 9999999;
      for (int yy=-w ; yy<=w ; yy++)
      {
        for (int xx=-w ; xx<=w ; xx++)
        {
          int val = blepo_ex::Clamp( img(x+xx, y+yy) - offset, 0, 255);
          if (val < minn)  minn = val;
        }
      }
      assert(minn>=0 && minn<=255);
      (*out)(x, y) = minn;
    }
  }
}

// grayscale dilation
void GrayscaleDilate3x3(const ImgGray& img, int offset, ImgGray* out)
{
  InPlaceSwapper< ImgGray > inplace(img, &out);
  out->Reset(img.Width(), img.Height());
  *out = img;  // to copy border (inefficient but works)
  const int w = 1;  // halfwidth of 3x3 kernel
  for (int y=w ; y<img.Height()-w ; y++)
  {
    for (int x=w ; x<img.Width()-w ; x++)
    {
      int maxx = -1;
      for (int yy=-w ; yy<=w ; yy++)
      {
        for (int xx=-w ; xx<=w ; xx++)
        {
          int val = blepo_ex::Clamp( img(x+xx, y+yy) + offset, 0, 255);
          if (val > maxx)  maxx = val;
        }
      }
      assert(maxx>=0 && maxx<=255);
      (*out)(x, y) = maxx;
    }
  }
}

void Convert(const ImgGray& img, ImgBgr* out)
{
  out->Reset(img.Width(), img.Height());
  ImgGray::ConstIterator p;
  ImgBgr::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++) 
  {
    const ImgGray::Pixel& pixi = *p;
    ImgBgr::Pixel& pixo = *q;
    pixo.b = pixi;
    pixo.g = pixi;
    pixo.r = pixi;
  }
}

void Convert(const ImgBgr& img, ImgGray* out)
{
  out->Reset(img.Width(), img.Height());
  ImgBgr::ConstIterator p;
  ImgGray::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    const ImgBgr::Pixel& pixi = *p;
    int pixo = (pixi.b + 6*pixi.g + 3*pixi.r) / 10;
    *q = blepo_ex::Min(blepo_ex::Max(pixo, static_cast<int>(ImgGray::MIN_VAL)), static_cast<int>(ImgGray::MAX_VAL));
  }
}

void Convert(const ImgGray& img, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  ImgGray::ConstIterator p;
  ImgBinary::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = (*p != 0);
  }
}

void Convert(const ImgBinary& img, ImgGray* out, ImgGray::Pixel val0, ImgGray::Pixel val1)
{
  out->Reset(img.Width(), img.Height());
  ImgBinary::ConstIterator p;
  ImgGray::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = (*p) ? val1 : val0;
  }
}

void Convert(const ImgGray& img, ImgInt* out)
{
  out->Reset(img.Width(), img.Height());
  ImgGray::ConstIterator p;
  ImgInt::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = static_cast<ImgInt::Pixel>( *p );
  }
}

void Convert(const ImgInt& img, ImgGray* out)
{
  out->Reset(img.Width(), img.Height());
  ImgInt::ConstIterator p;
  ImgGray::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = static_cast<ImgGray::Pixel>( blepo_ex::Max( blepo_ex::Min( 255, *p ), 0 ) );
  }
}

void Convert(const ImgGray& img, ImgFloat* out)
{
  out->Reset(img.Width(), img.Height());
  ImgGray::ConstIterator p;
  ImgFloat::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = static_cast<ImgFloat::Pixel>( *p );
  }
}

void Convert(const ImgFloat& img, ImgGray* out, bool linearly_scale)
{
  out->Reset(img.Width(), img.Height());
  ImgFloat::ConstIterator p = img.Begin();
  ImgFloat::ConstIterator end = img.End();
  ImgGray::Iterator q = out->Begin();
  ImgFloat foo;
  if (linearly_scale)
  {
    LinearlyScale(img, 0, 255, &foo);
    p = foo.Begin();
    end = foo.End();
  }
  while (p != end)
  {
    *q++ = blepo_ex::Clamp( blepo_ex::Round( *p++ ), 0, 255 );
  }
}

void Convert(const ImgInt& img, ImgFloat* out)
{
  out->Reset(img.Width(), img.Height());
  ImgInt::ConstIterator p;
  ImgFloat::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = static_cast<ImgFloat::Pixel>(*p);
  }
}

void Convert(const ImgFloat& img, ImgInt* out, bool linearly_scale)
{
  out->Reset(img.Width(), img.Height());
  ImgFloat::ConstIterator p = img.Begin();
  ImgFloat::ConstIterator end = img.End();
  ImgInt::Iterator q = out->Begin();
  ImgFloat foo;
  if (linearly_scale)
  {
    LinearlyScale(img, (float) ImgInt::MIN_VAL, (float) ImgInt::MAX_VAL, &foo);
    p = foo.Begin();
    end = foo.End();
  }
  while (p != end)
  {
    *q++ = blepo_ex::Round( *p++ );
  }
}

void Convert(const ImgBinary& img, ImgBgr* out, const ImgBgr::Pixel& val0, const ImgBgr::Pixel& val1)
{
  out->Reset(img.Width(), img.Height());
  ImgBinary::ConstIterator p;
  ImgBgr::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = (*p) ? val1 : val0;
  }
}

void Convert(const ImgBgr& img, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  ImgBgr::ConstIterator p;
  ImgBinary::Iterator q;
  ImgBgr::Pixel zero(0,0,0);
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = (*p != zero);
  }
}

void Convert(const ImgBgr& img, ImgFloat* out)
{
  // could be made more efficient
  ImgGray tmp;
  Convert(img, &tmp);
  Convert(tmp, out);
}

void Convert(const ImgFloat& img, ImgBgr* out, bool linearly_scale)
{
  // could be made more efficient
  ImgGray tmp;
  Convert(img, &tmp, linearly_scale);
  Convert(tmp, out);
}

void Convert(const ImgInt& img, ImgBgr* out, Bgr::IntType format)
{
  out->Reset(img.Width(), img.Height());
  ImgInt::ConstIterator p;
  ImgBgr::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    q->FromInt( static_cast<unsigned int>(*p), format );
  }
}

void Convert(const ImgBgr& img, ImgInt* out, Bgr::IntType format)
{
  out->Reset(img.Width(), img.Height());
  ImgBgr::ConstIterator p;
  ImgInt::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q =  static_cast<ImgInt::Pixel>( p->ToInt( format ) );
  }
}

void Convert(const ImgBinary& img, ImgInt* out, ImgInt::Pixel val0, ImgInt::Pixel val1)
{
  out->Reset(img.Width(), img.Height());
  ImgBinary::ConstIterator p;
  ImgInt::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = (*p) ? val1 : val0;
  }
}

void Convert(const ImgInt& img, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  ImgInt::ConstIterator p;
  ImgBinary::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = (*p != 0);
  }
}

void Convert(const ImgBinary& img, ImgFloat* out, ImgFloat::Pixel val0, ImgFloat::Pixel val1)
{
  out->Reset(img.Width(), img.Height());
  ImgBinary::ConstIterator p;
  ImgFloat::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = (*p) ? val1 : val0;
  }
}

void Convert(const ImgFloat& img, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  ImgFloat::ConstIterator p;
  ImgBinary::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = (*p != 0);
  }
}





//void Convert(const ImgBgr& img, ImgInt* out)
//{
//  out->Reset(img.Width(), img.Height());
//  ImgBgr::ConstIterator p;
//  ImgInt::Iterator q;
//  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
//  {
//    const ImgBgr::Pixel& pixi = *p;
//    int pixo = (pixi.b + 6*pixi.g + 3*pixi.r) / 10;
//    *q = blepo_ex::Min(blepo_ex::Max(pixo, static_cast<int>(ImgInt::MIN_VAL)), static_cast<int>(ImgInt::MAX_VAL));
//  }
//}
//
//void Convert(const ImgBgr& img, ImgFloat* out)
//{
//  out->Reset(img.Width(), img.Height());
//  ImgBgr::ConstIterator p;
//  ImgFloat::Iterator q;
//  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
//  {
//    const ImgBgr::Pixel& pixi = *p;
//    ImgFloat::Pixel pixo = (pixi.b + 6*pixi.g + 3*pixi.r) / 10.0f;
//    *q = blepo_ex::Min(blepo_ex::Max(pixo, ImgFloat::MIN_VAL), ImgFloat::MAX_VAL) / 255;
//  }
//}

//void Convert(const ImgGray& img, ImgInt* out)
//{
//  out->Reset(img.Width(), img.Height());
//  ImgGray::ConstIterator p;
//  ImgInt::Iterator q;
//  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
//  {
//    *q = *p;
//  }
//}
//
//void Convert(const ImgInt& img, ImgGray* out)
//{
//  out->Reset(img.Width(), img.Height());
//  ImgInt::ConstIterator p;
//  ImgGray::Iterator q;
//  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
//  {
//    ImgInt::Pixel pix = *p;
//    if (pix > ImgGray::MAX_VAL)  pix = ImgGray::MAX_VAL;
//    else if (pix < ImgGray::MIN_VAL)  pix = ImgGray::MIN_VAL;
//    *q = static_cast<unsigned char>(pix);
//  }
//}

//void Convert(const ImgBinary& img, ImgGray* out)
//{
//  out->Reset(img.Width(), img.Height());
//  ImgBinary::ConstIterator p;
//  ImgGray::Iterator q;
//  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
//  {
//    *q = static_cast<ImgGray::Pixel>(*p);
//  }
//}

//void Convert(const ImgBinary& img, ImgFloat* out)
//{
//  out->Reset(img.Width(), img.Height());
//  ImgBinary::ConstIterator p;
//  ImgFloat::Iterator q;
//  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
//  {
//    *q = static_cast<ImgFloat::Pixel>(*p);
//  }
//}


void Threshold(const ImgBgr& img, int threshold, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  ImgBgr::ConstIterator p;
  ImgBinary::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = (p->b + p->g + p->r >= threshold);
  }
}

void Threshold(const ImgGray& img, unsigned char threshold, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  ImgGray::ConstIterator p;
  ImgBinary::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = (*p >= threshold);
  }
}

void Threshold(const ImgInt& img, int threshold, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  ImgInt::ConstIterator p;
  ImgBinary::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = (*p >= threshold);
  }
}

void Threshold(const ImgFloat& img, float threshold, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  ImgFloat::ConstIterator p;
  ImgBinary::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    *q = (*p >= threshold);
  }
}

// This iterative algorithm operates on the graylevel histogram of the image.
// See Rider-Calvard 1978.  Also see Gonzalez and Woods, 2nd ed., p. 599
double ComputeThreshold(const ImgGray& img)
{
  // compute histogram
  int hist[256];
  memset(hist, 0, 256*sizeof(int));
  const unsigned char* p = img.Begin();
  while (p != img.End())  hist[ *p++ ]++;
  const int max_niter = 100;  // just in case
  int iter = 0;
  int i;

  double th, th_new = 127;
  do
  {
    th = th_new;

    // compute mu1 and mu2
    double mu1, mu2;
    {
      int n1 = 0, x1 = 0, n2 = 0, x2 = 0;
      for (i=0 ; i<256 ; i++)
      {
        if (i < blepo_ex::Round(th))
        {
          n1 += hist[i];
          x1 += i * hist[i];
        }
        else
        {
          n2 += hist[i];
          x2 += i * hist[i];
        }
      }
      mu1 = ((double) x1) / n1;
      mu2 = ((double) x2) / n2;
    }

    // adjust threshold
    th_new = (mu1 + mu2) / 2.0;

    iter++;
    if (iter > max_niter)  break;
  } 
  while ( fabs(th_new - th) > 1.0 );

  return th_new;
}

// set all pixels

void Set(ImgBgr* out, ImgBgr::Pixel val)
{
  for (ImgBgr::Iterator p = out->Begin() ; p != out->End() ; p++)  *p = val;
}

void Set(ImgBinary* out, ImgBinary::Pixel val)
{ // could make this faster by operating on bytes instead
  for (ImgBinary::Iterator p = out->Begin() ; p != out->End() ; p++)  *p = val;
}

void Set(ImgFloat* out, ImgFloat::Pixel val)
{
  for (ImgFloat::Iterator p = out->Begin() ; p != out->End() ; p++)  *p = val;
}

void Set(ImgGray* out, ImgGray::Pixel val)
{
  for (ImgGray::Iterator p = out->Begin() ; p != out->End() ; p++)  *p = val;
}

void Set(ImgInt* out, ImgInt::Pixel val)
{
  for (ImgInt::Iterator p = out->Begin() ; p != out->End() ; p++)  *p = val;
}

// set contiguous list of pixels to constant value

void Set(ImgBgr* out, ImgBgr::Pixel val, int x, int y, int n)
{
  ImgBgr::Iterator p = out->Begin(x, y);
  for (int i=0 ; i<n ; i++)  *p++ = val;
}

void Set(ImgBinary* out, ImgBinary::Pixel val, int x, int y, int n)
{ // could make this faster by operating on bytes instead
  ImgBinary::Iterator p = out->Begin(x, y);
  for (int i=0 ; i<n ; i++)  *p++ = val;
}

void Set(ImgFloat* out, ImgFloat::Pixel val, int x, int y, int n)
{
  ImgFloat::Iterator p = out->Begin(x, y);
  for (int i=0 ; i<n ; i++)  *p++ = val;
}

void Set(ImgGray* out, ImgGray::Pixel val, int x, int y, int n)
{
  ImgGray::Iterator p = out->Begin(x, y);
  for (int i=0 ; i<n ; i++)  *p++ = val;
}

void Set(ImgInt* out, ImgInt::Pixel val, int x, int y, int n)
{
  ImgInt::Iterator p = out->Begin(x, y);
  for (int i=0 ; i<n ; i++)  *p++ = val;
}

// set all pixels inside 'rect'

void Set(ImgBgr* out, const Rect& rect, ImgBgr::Pixel val)
{
  assert(rect.left<=rect.right && rect.top<=rect.bottom && rect.left>=0 && rect.top>=0 && rect.right<=out->Width() && rect.bottom<=out->Height());
  ImgBgr::Iterator p = out->Begin(rect.left, rect.top);
  int skip = out->Width() - (rect.right - rect.left);
  for (int y=rect.top ; y<rect.bottom ; y++, p += skip)
  {
    for (int x=rect.left ; x<rect.right ; x++)  *p++ = val;
  }
}

void Set(ImgBinary* out, const Rect& rect, ImgBinary::Pixel val)
{
  assert(rect.left<=rect.right && rect.top<=rect.bottom && rect.left>=0 && rect.top>=0 && rect.right<=out->Width() && rect.bottom<=out->Height());
  ImgBinary::Iterator p = out->Begin(rect.left, rect.top);
  int skip = out->Width() - (rect.right - rect.left);
  for (int y=rect.top ; y<rect.bottom ; y++, p += skip)
  {
    for (int x=rect.left ; x<rect.right ; x++)  *p++ = val;
  }
}

void Set(ImgFloat* out, const Rect& rect, ImgFloat::Pixel val)
{
  assert(rect.left<=rect.right && rect.top<=rect.bottom && rect.left>=0 && rect.top>=0 && rect.right<=out->Width() && rect.bottom<=out->Height());
  ImgFloat::Iterator p = out->Begin(rect.left, rect.top);
  int skip = out->Width() - (rect.right - rect.left);
  for (int y=rect.top ; y<rect.bottom ; y++, p += skip)
  {
    for (int x=rect.left ; x<rect.right ; x++)  *p++ = val;
  }
}

void Set(ImgGray* out, const Rect& rect, ImgGray::Pixel val)
{
  assert(rect.left<=rect.right && rect.top<=rect.bottom && rect.left>=0 && rect.top>=0 && rect.right<=out->Width() && rect.bottom<=out->Height());
  ImgGray::Iterator p = out->Begin(rect.left, rect.top);
  int skip = out->Width() - (rect.right - rect.left);
  for (int y=rect.top ; y<rect.bottom ; y++, p += skip)
  {
    for (int x=rect.left ; x<rect.right ; x++)  *p++ = val;
  }
}

void Set(ImgInt* out, const Rect& rect, ImgInt::Pixel val)
{
  assert(rect.left<=rect.right && rect.top<=rect.bottom && rect.left>=0 && rect.top>=0 && rect.right<=out->Width() && rect.bottom<=out->Height());
  ImgInt::Iterator p = out->Begin(rect.left, rect.top);
  int skip = out->Width() - (rect.right - rect.left);
  for (int y=rect.top ; y<rect.bottom ; y++, p += skip)
  {
    for (int x=rect.left ; x<rect.right ; x++)  *p++ = val;
  }
}

// set all pixels specified by mask to constant value

void Set(ImgBgr* out, const ImgBinary& mask, ImgBgr::Pixel val)
{
  assert( IsSameSize(*out, mask) );
  ImgBinary::ConstIterator p = mask.Begin();
  ImgBgr::Iterator q = out->Begin();
  while (q != out->End())  { if (*p++)  *q = val;  q++; }
}

void Set(ImgBinary* out, const ImgBinary& mask, ImgBinary::Pixel val)
{
  assert( IsSameSize(*out, mask) );
  ImgBinary::ConstIterator p = mask.Begin();
  ImgBinary::Iterator q = out->Begin();
  while (q != out->End())  { if (*p++)  *q = val;  q++; }
}

void Set(ImgFloat* out, const ImgBinary& mask, ImgFloat::Pixel val)
{
  assert( IsSameSize(*out, mask) );
  ImgBinary::ConstIterator p = mask.Begin();
  ImgFloat::Iterator q = out->Begin();
  while (q != out->End())  { if (*p++)  *q = val;  q++; }
}

void Set(ImgGray* out, const ImgBinary& mask, ImgGray::Pixel val)
{
  assert( IsSameSize(*out, mask) );
  ImgBinary::ConstIterator p = mask.Begin();
  ImgGray::Iterator q = out->Begin();
  while (q != out->End())  { if (*p++)  *q = val;  q++; }
}

void Set(ImgInt* out, const ImgBinary& mask, ImgInt::Pixel val)
{
  assert( IsSameSize(*out, mask) );
  ImgBinary::ConstIterator p = mask.Begin();
  ImgInt::Iterator q = out->Begin();
  while (q != out->End())  { if (*p++)  *q = val;  q++; }
}




// specializations
void SetOutside(ImgBinary* out, const Rect& rect, ImgBinary::Pixel val) { iSetOutside(out, rect, val); }
void SetOutside(ImgBgr*    out, const Rect& rect, ImgBgr   ::Pixel val) { iSetOutside(out, rect, val); }
void SetOutside(ImgFloat*  out, const Rect& rect, ImgFloat ::Pixel val) { iSetOutside(out, rect, val); }
void SetOutside(ImgGray*   out, const Rect& rect, ImgGray  ::Pixel val) { iSetOutside(out, rect, val); }
void SetOutside(ImgInt*    out, const Rect& rect, ImgInt   ::Pixel val) { iSetOutside(out, rect, val); }

// set using another image:  copies entire 'img' to 'out' at location 'pt'

void Set(ImgBgr* out, const CPoint& pt, const ImgBgr& img)
{
  assert(pt.x>=0 && pt.y>=0 && pt.x+img.Width()<=out->Width() && pt.y+img.Height()<=out->Height());
  ImgBgr::ConstIterator p = img.Begin();
  ImgBgr::Iterator q = out->Begin(pt.x, pt.y);
  int skip = out->Width() - img.Width();
  for (int y=0 ; y<img.Height() ; y++, q += skip)
  {
    for (int x=0 ; x<img.Width() ; x++)  *q++ = *p++;
  }
}

void Set(ImgBinary* out, const CPoint& pt, const ImgBinary& img)
{
  assert(pt.x>=0 && pt.y>=0 && pt.x+img.Width()<=out->Width() && pt.y+img.Height()<=out->Height());
  ImgBinary::ConstIterator p = img.Begin();
  ImgBinary::Iterator q = out->Begin(pt.x, pt.y);
  int skip = out->Width() - img.Width();
  for (int y=0 ; y<img.Height() ; y++, q += skip)
  {
    for (int x=0 ; x<img.Width() ; x++)  *q++ = *p++;
  }
}

void Set(ImgFloat* out, const CPoint& pt, const ImgFloat& img)
{
  assert(pt.x>=0 && pt.y>=0 && pt.x+img.Width()<=out->Width() && pt.y+img.Height()<=out->Height());
  ImgFloat::ConstIterator p = img.Begin();
  ImgFloat::Iterator q = out->Begin(pt.x, pt.y);
  int skip = out->Width() - img.Width();
  for (int y=0 ; y<img.Height() ; y++, q += skip)
  {
    for (int x=0 ; x<img.Width() ; x++)  *q++ = *p++;
  }
}

void Set(ImgGray* out, const CPoint& pt, const ImgGray& img)
{
  assert(pt.x>=0 && pt.y>=0 && pt.x+img.Width()<=out->Width() && pt.y+img.Height()<=out->Height());
  ImgGray::ConstIterator p = img.Begin();
  ImgGray::Iterator q = out->Begin(pt.x, pt.y);
  int skip = out->Width() - img.Width();
  for (int y=0 ; y<img.Height() ; y++, q += skip)
  {
    for (int x=0 ; x<img.Width() ; x++)  *q++ = *p++;
  }
}

void Set(ImgInt* out, const CPoint& pt, const ImgInt& img)
{
  assert(pt.x>=0 && pt.y>=0 && pt.x+img.Width()<=out->Width() && pt.y+img.Height()<=out->Height());
  ImgInt::ConstIterator p = img.Begin();
  ImgInt::Iterator q = out->Begin(pt.x, pt.y);
  int skip = out->Width() - img.Width();
  for (int y=0 ; y<img.Height() ; y++, q += skip)
  {
    for (int x=0 ; x<img.Width() ; x++)  *q++ = *p++;
  }
}

// set using another image:  copies 'rect' of 'img' to 'out' at location 'pt'

void Set(ImgBgr* out, const CPoint& pt, const ImgBgr& img, const Rect& rect)
{
  assert(rect.left>=0 && rect.right>=rect.left);
  assert(rect.top>=0 && rect.bottom>=rect.top);
  assert(rect.right<=img.Width() && rect.bottom<=img.Height());
  assert(pt.x+rect.Width()<=out->Width() && pt.y+rect.Height()<=out->Height());

  ImgBgr::ConstIterator p = img.Begin(rect.left, rect.top);
  ImgBgr::Iterator q = out->Begin(pt.x, pt.y);
  int pskip = img.Width() - rect.Width();
  int qskip = out->Width() - rect.Width();
  for (int y=0 ; y<rect.Height() ; y++, p+=pskip, q+=qskip)
  {
    for (int x=0 ; x<rect.Width() ; x++)  *q++ = *p++;
  }
}

void Set(ImgBinary* out, const CPoint& pt, const ImgBinary& img, const Rect& rect)
{
  assert(rect.left>=0 && rect.right>=rect.left);
  assert(rect.top>=0 && rect.bottom>=rect.top);
  assert(rect.right<=img.Width() && rect.bottom<=img.Height());
  assert(pt.x+rect.Width()<=out->Width() && pt.y+rect.Height()<=out->Height());

  ImgBinary::ConstIterator p = img.Begin(rect.left, rect.top);
  ImgBinary::Iterator q = out->Begin(pt.x, pt.y);
  int pskip = img.Width() - rect.Width();
  int qskip = out->Width() - rect.Width();
  for (int y=0 ; y<rect.Height() ; y++, p+=pskip, q+=qskip)
  {
    for (int x=0 ; x<rect.Width() ; x++)  *q++ = *p++;
  }
}

void Set(ImgFloat* out, const CPoint& pt, const ImgFloat& img, const Rect& rect)
{
  assert(rect.left>=0 && rect.right>=rect.left);
  assert(rect.top>=0 && rect.bottom>=rect.top);
  assert(rect.right<=img.Width() && rect.bottom<=img.Height());
  assert(pt.x+rect.Width()<=out->Width() && pt.y+rect.Height()<=out->Height());

  ImgFloat::ConstIterator p = img.Begin(rect.left, rect.top);
  ImgFloat::Iterator q = out->Begin(pt.x, pt.y);
  int pskip = img.Width() - rect.Width();
  int qskip = out->Width() - rect.Width();
  for (int y=0 ; y<rect.Height() ; y++, p+=pskip, q+=qskip)
  {
    for (int x=0 ; x<rect.Width() ; x++)  *q++ = *p++;
  }
}

void Set(ImgGray* out, const CPoint& pt, const ImgGray& img, const Rect& rect)
{
  assert(rect.left>=0 && rect.right>=rect.left);
  assert(rect.top>=0 && rect.bottom>=rect.top);
  assert(rect.right<=img.Width() && rect.bottom<=img.Height());
  assert(pt.x+rect.Width()<=out->Width() && pt.y+rect.Height()<=out->Height());

  ImgGray::ConstIterator p = img.Begin(rect.left, rect.top);
  ImgGray::Iterator q = out->Begin(pt.x, pt.y);
  int pskip = img.Width() - rect.Width();
  int qskip = out->Width() - rect.Width();
  for (int y=0 ; y<rect.Height() ; y++, p+=pskip, q+=qskip)
  {
    for (int x=0 ; x<rect.Width() ; x++)  *q++ = *p++;
  }
}

void Set(ImgInt* out, const CPoint& pt, const ImgInt& img, const Rect& rect)
{
  assert(rect.left>=0 && rect.right>=rect.left);
  assert(rect.top>=0 && rect.bottom>=rect.top);
  assert(rect.right<=img.Width() && rect.bottom<=img.Height());
  assert(pt.x+rect.Width()<=out->Width() && pt.y+rect.Height()<=out->Height());

  ImgInt::ConstIterator p = img.Begin(rect.left, rect.top);
  ImgInt::Iterator q = out->Begin(pt.x, pt.y);
  int pskip = img.Width() - rect.Width();
  int qskip = out->Width() - rect.Width();
  for (int y=0 ; y<rect.Height() ; y++, p+=pskip, q+=qskip)
  {
    for (int x=0 ; x<rect.Width() ; x++)  *q++ = *p++;
  }
}


void Set(ImgBgr   * out, const ImgBgr::Pixel old_value, const ImgBgr::Pixel new_value)
{
  ImgBgr::Iterator p = out->Begin();
  while(p != out->End())
  {
    if(*p == old_value)
      *p = new_value;
    
    p++;
  }
}
void Set(ImgBinary* out, const ImgBinary::Pixel old_value, const ImgBinary::Pixel new_value)
{
  ImgBinary::Iterator p = out->Begin();
  while(p != out->End())
  {
    if(*p == old_value)
      *p = new_value;

    p++;
  }
}

void Set(ImgFloat * out, const ImgFloat::Pixel old_value, const ImgFloat::Pixel new_value)
{
  ImgFloat::Iterator p = out->Begin();
  while(p != out->End())
  {
    if(*p == old_value)
      *p = new_value;

    p++;
  }
}

void Set(ImgGray  * out, const ImgGray::Pixel old_value, const ImgGray::Pixel new_value)
{
  ImgGray::Iterator p = out->Begin();
  while(p != out->End())
  {
    if(*p == old_value)
      *p = new_value;

    p++;
  }
}

void Set(ImgInt   * out, const ImgInt::Pixel old_value, const ImgInt::Pixel new_value)
{
  ImgInt::Iterator p = out->Begin();
  while(p != out->End())
  {
    if(*p == old_value)
      *p = new_value;

    p++;
  }
}


// extract 'rect' of pixels from 'img' to 'out'

template <typename T>
inline void iExtract(const Image<T>& img, const Rect& rect, Image<T>* out)
{
  assert(rect.left>=0 && rect.right>=rect.left);
  assert(rect.top>=0 && rect.bottom>=rect.top);
  assert(rect.right<=img.Width() && rect.bottom<=img.Height());
  InPlaceSwapper< Image<T> > inplace(img, &out);
  out->Reset(rect.Width(), rect.Height());
  Image<T>::ConstIterator p = img.Begin(rect.left, rect.top);
  Image<T>::Iterator q = out->Begin();
  int skip = img.Width() - rect.Width();
  for (int y=0 ; y<rect.Height() ; y++, p+=skip)
  {
    for (int x=0 ; x<rect.Width() ; x++)  *q++ = *p++;
//    memcpy(q, p, rect.Width()*sizeof(Image<T>::Pixel));  // could be faster, but be careful with binary
  }
}

void Extract(const ImgBgr   & img, const Rect& rect, ImgBgr   * out) { iExtract(img, rect, out); } 
void Extract(const ImgBinary& img, const Rect& rect, ImgBinary* out) { iExtract(img, rect, out); }
void Extract(const ImgFloat & img, const Rect& rect, ImgFloat * out) { iExtract(img, rect, out); }
void Extract(const ImgGray  & img, const Rect& rect, ImgGray  * out) { iExtract(img, rect, out); }
void Extract(const ImgInt   & img, const Rect& rect, ImgInt   * out) { iExtract(img, rect, out); }

template <typename T>
inline void iExtract(const Image<T>& img, int xc, int yc, int hw, int hh, Image<T>* out)
{
  int left = xc-hw;
  int top = yc-hh;
  int width = 2*hw+1;
  int height = 2*hh+1;
  assert(left >= 0 && top >= 0);
  assert(left+width < img.Width() && top+height < img.Height());
//  assert(rect.left>=0 && rect.right>=rect.left);
//  assert(rect.top>=0 && rect.bottom>=rect.top);
//  assert(rect.right<=img.Width() && rect.bottom<=img.Height());
  InPlaceSwapper< Image<T> > inplace(img, &out);
  out->Reset(width, height);
  Image<T>::ConstIterator p = img.Begin(left, top);
  Image<T>::Iterator q = out->Begin();
  int skip = img.Width() - width;
  for (int y=0 ; y<height ; y++, p+=skip)
  {
    for (int x=0 ; x<width ; x++)  *q++ = *p++;
//    memcpy(q, p, rect.Width()*sizeof(Image<T>::Pixel));  // could be faster, but be careful with binary
  }
}

void Extract(const ImgBgr   & img, int xc, int yc, int hw, int hh, ImgBgr   * out) { iExtract(img, xc, yc, hw, hh, out); } 
void Extract(const ImgBinary& img, int xc, int yc, int hw, int hh, ImgBinary* out) { iExtract(img, xc, yc, hw, hh, out); } 
void Extract(const ImgFloat & img, int xc, int yc, int hw, int hh, ImgFloat * out) { iExtract(img, xc, yc, hw, hh, out); } 
void Extract(const ImgGray  & img, int xc, int yc, int hw, int hh, ImgGray  * out) { iExtract(img, xc, yc, hw, hh, out); } 
void Extract(const ImgInt   & img, int xc, int yc, int hw, int hh, ImgInt   * out) { iExtract(img, xc, yc, hw, hh, out); } 

template <typename T>
inline void iExtract(const Image<T>& img, int xc, int yc, 
                     int hwl, int hwr, int hht, int hhb, 
                     double scale, Image<T>* out)
{
  int left = xc-blepo_ex::Round(hwl*scale);
  int top  = yc-blepo_ex::Round(hht*scale);
  assert(hwr >= -hwl && hhb >= -hht);
  int width  = hwl+hwr+1;
  int height = hht+hhb+1;
//  assert(rect.left>=0 && rect.right>=rect.left);
//  assert(rect.top>=0 && rect.bottom>=rect.top);
//  assert(rect.right<=img.Width() && rect.bottom<=img.Height());
  InPlaceSwapper< Image<T> > inplace(img, &out);
  out->Reset(width, height);
  Image<T>::ConstIterator p = img.Begin(left, top);
  Image<T>::Iterator q = out->Begin();
  double yy = 0;
  int m = 0;
  for (int y=0 ; y<height ; y++)
  {
    double xx = 0;
    int n = 0;
    for (int x=0 ; x<width ; x++)  
    {
      *q++ = *p;
      xx += scale;
      int bxx = (int) xx;
      p += bxx;
      xx -= bxx;
      n += bxx;
      assert(left + n < img.Width());
    }
    yy += scale;
    int byy = (int) yy;
    int skip = img.Width() - n;
    p += skip + img.Width() * (byy-1);
    yy -= byy;
    m += byy;
    assert(top + m < img.Height());
  }
}

void Extract(const ImgBgr   & img, int xc, int yc, int hwl, int hwr, int hht, int hhb, double scale, ImgBgr   * out) { iExtract(img, xc, yc, hwl, hwr, hht, hhb, scale, out); } 
void Extract(const ImgBinary& img, int xc, int yc, int hwl, int hwr, int hht, int hhb, double scale, ImgBinary* out) { iExtract(img, xc, yc, hwl, hwr, hht, hhb, scale, out); } 
void Extract(const ImgFloat & img, int xc, int yc, int hwl, int hwr, int hht, int hhb, double scale, ImgFloat * out) { iExtract(img, xc, yc, hwl, hwr, hht, hhb, scale, out); } 
void Extract(const ImgGray  & img, int xc, int yc, int hwl, int hwr, int hht, int hhb, double scale, ImgGray  * out) { iExtract(img, xc, yc, hwl, hwr, hht, hhb, scale, out); } 
void Extract(const ImgInt   & img, int xc, int yc, int hwl, int hwr, int hht, int hhb, double scale, ImgInt   * out) { iExtract(img, xc, yc, hwl, hwr, hht, hhb, scale, out); } 

//void Extract(const ImgBgr& img, const Rect& rect, ImgBgr* out)
//{
//  assert(rect.left>=0 && rect.right>=rect.left);
//  assert(rect.top>=0 && rect.bottom>=rect.top);
//  assert(rect.right<=img.Width() && rect.bottom<=img.Height());
//
//  out->Reset(rect.Width(), rect.Height());
//  ImgBgr::ConstIterator p = img.Begin(rect.left, rect.top);
//  ImgBgr::Iterator q = out->Begin();
//  int skip = img.Width() - rect.Width();
//  for (int y=rect.top ; y<rect.bottom ; y++, p+=skip)
//  {
//    for (int x=0 ; x<rect.Width() ; x++)  *q++ = *p++;
////    memcpy(q, p, rect.Width()*sizeof(ImgBgr::Pixel));  // could be faster, but be careful with binary
//  }
//}
//
//void Extract(const ImgBinary& img, const Rect& rect, ImgBinary* out)
//{
//  assert(rect.left>=0 && rect.right>=rect.left);
//  assert(rect.top>=0 && rect.bottom>=rect.top);
//  assert(rect.right<=img.Width() && rect.bottom<=img.Height());
//
//  out->Reset(rect.Width(), rect.Height());
//  ImgBinary::ConstIterator p = img.Begin(rect.left, rect.top);
//  ImgBinary::Iterator q = out->Begin();
//  int skip = img.Width() - rect.Width();
//  for (int y=rect.top ; y<rect.bottom ; y++, p+=skip)
//  {
//    for (int x=0 ; x<rect.Width() ; x++)  *q++ = *p++;
//  }
//}
//
//void Extract(const ImgFloat& img, const Rect& rect, ImgFloat* out)
//{
//  assert(rect.left>=0 && rect.right>=rect.left);
//  assert(rect.top>=0 && rect.bottom>=rect.top);
//  assert(rect.right<=img.Width() && rect.bottom<=img.Height());
//
//  out->Reset(rect.Width(), rect.Height());
//  ImgFloat::ConstIterator p = img.Begin(rect.left, rect.top);
//  ImgFloat::Iterator q = out->Begin();
//  int skip = img.Width() - rect.Width();
//  for (int y=rect.top ; y<rect.bottom ; y++, p+=skip)
//  {
//    for (int x=0 ; x<rect.Width() ; x++)  *q++ = *p++;
////    memcpy(q, p, rect.Width()*sizeof(ImgFloat::Pixel));  // could be faster, but be careful with binary
//  }
//}
//
//void Extract(const ImgGray& img, const Rect& rect, ImgGray* out)
//{
//  assert(rect.left>=0 && rect.right>=rect.left);
//  assert(rect.top>=0 && rect.bottom>=rect.top);
//  assert(rect.right<=img.Width() && rect.bottom<=img.Height());
//
//  out->Reset(rect.Width(), rect.Height());
//  ImgGray::ConstIterator p = img.Begin(rect.left, rect.top);
//  ImgGray::Iterator q = out->Begin();
//  int skip = img.Width() - rect.Width();
//  for (int y=rect.top ; y<rect.bottom ; y++, p+=skip)
//  {
//    for (int x=0 ; x<rect.Width() ; x++)  *q++ = *p++;
////    memcpy(q, p, rect.Width()*sizeof(ImgGray::Pixel));  // could be faster, but be careful with binary
//  }
//}
//
//void Extract(const ImgInt& img, const Rect& rect, ImgInt* out)
//{
//  assert(rect.left>=0 && rect.right>=rect.left);
//  assert(rect.top>=0 && rect.bottom>=rect.top);
//  assert(rect.right<=img.Width() && rect.bottom<=img.Height());
//
//  out->Reset(rect.Width(), rect.Height());
//  ImgInt::ConstIterator p = img.Begin(rect.left, rect.top);
//  ImgInt::Iterator q = out->Begin();
//  int skip = img.Width() - rect.Width();
//  for (int y=rect.top ; y<rect.bottom ; y++, p+=skip)
//  {
//    for (int x=0 ; x<rect.Width() ; x++)  *q++ = *p++;
////    memcpy(q, p, rect.Width()*sizeof(ImgInt::Pixel));  // could be faster, but be careful with binary
//  }
//}


// Note:  It would be good to have a fast copy function, that copies a contiguous set of
// pixels from one image to another.  If the image is binary, it uses bytes or words wherever
// it can to make it fast.  For other image types, it uses memcpy.  This would be a low-level function
// used by Set and Extract above.

//bool IsSameSize(const ImgBinary& img1, const ImgBinary& img2)
//{
//  return (img1.Width()==img2.Width() && img1.Height()==img2.Height());
//}
//
//bool IsSameSize(const ImgBgr& img1, const ImgBgr& img2)
//{
//  return (img1.Width()==img2.Width() && img1.Height()==img2.Height());
//}
//
//bool IsSameSize(const ImgGray& img1, const ImgGray& img2)
//{
//  return (img1.Width()==img2.Width() && img1.Height()==img2.Height());
//}
//
//bool IsSameSize(const ImgInt& img1, const ImgInt& img2)
//{
//  return (img1.Width()==img2.Width() && img1.Height()==img2.Height());
//}
//
//bool IsSameSize(const ImgFloat& img1, const ImgFloat& img2)
//{
//  return (img1.Width()==img2.Width() && img1.Height()==img2.Height());
//}

bool IsIdentical(const ImgBinary& img1, const ImgBinary& img2)
{
  if (!IsSameSize(img1, img2))  return false;
  if (img1.Width() * img1.Height() % 8 == 0)
  {
    return memcmp(img1.BytePtr(), img2.BytePtr(), img1.NBytes()) == 0;
  }
  else
  {
    if (memcmp(img1.BytePtr(), img2.BytePtr(), img1.NBytes()-1) != 0)  return false;
    unsigned char last_byte1 = *(img1.BytePtr() + img1.NBytes() - 1);
    unsigned char last_byte2 = *(img2.BytePtr() + img2.NBytes() - 1);
    unsigned char diff = last_byte1 ^ last_byte2;
    int mod = img1.Width() * img1.Height() % 8;
    signed char mask = -128;
    mask >>= mod;
    return (diff & static_cast<unsigned char>(mask)) == 0;
  }

//
//  if (m_data.Len() > 0)
//  {
//    if ((memcmp(m_data.Begin(), other.m_data.Begin(), m_data.Len()-1) != 0) return false;
//    unsigned char byte1 = *(m_data.End()-1);
//    unsigned char byte2 = *(other.m_data.End()-1);
//    byte1 &= 
//  }
}

bool IsIdentical(const ImgBgr& img1, const ImgBgr& img2)
{
  if (!IsSameSize(img1, img2))  return false;
  return memcmp(img1.BytePtr(), img2.BytePtr(), img1.NBytes()) == 0;
}

bool IsIdentical(const ImgGray& img1, const ImgGray& img2)
{
  if (!IsSameSize(img1, img2))  return false;
  return memcmp(img1.BytePtr(), img2.BytePtr(), img1.NBytes()) == 0;
}

bool IsIdentical(const ImgInt& img1, const ImgInt& img2)
{
  if (!IsSameSize(img1, img2))  return false;
  return memcmp(img1.BytePtr(), img2.BytePtr(), img1.NBytes()) == 0;
}

bool IsIdentical(const ImgFloat& img1, const ImgFloat& img2)
{
  if (!IsSameSize(img1, img2))  return false;
  return memcmp(img1.BytePtr(), img2.BytePtr(), img1.NBytes()) == 0;
}

void Equal(const ImgBgr&    img1, const ImgBgr&    img2, ImgBinary* out) { iEqual(img1, img2, out); }
void Equal(const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out) { iEqual(img1, img2, out); }
void Equal(const ImgFloat&  img1, const ImgFloat&  img2, ImgBinary* out) { iEqual(img1, img2, out); }
void Equal(const ImgGray&   img1, const ImgGray&   img2, ImgBinary* out) { iEqual(img1, img2, out); }
void Equal(const ImgInt&    img1, const ImgInt&    img2, ImgBinary* out) { iEqual(img1, img2, out); }
void NotEqual(const ImgBgr&    img1, const ImgBgr&    img2, ImgBinary* out) { iNotEqual(img1, img2, out); }
void NotEqual(const ImgBinary& img1, const ImgBinary& img2, ImgBinary* out) { iNotEqual(img1, img2, out); }
void NotEqual(const ImgFloat&  img1, const ImgFloat&  img2, ImgBinary* out) { iNotEqual(img1, img2, out); }
void NotEqual(const ImgGray&   img1, const ImgGray&   img2, ImgBinary* out) { iNotEqual(img1, img2, out); }
void NotEqual(const ImgInt&    img1, const ImgInt&    img2, ImgBinary* out) { iNotEqual(img1, img2, out); }
void LessThan(const ImgFloat& img1, const ImgFloat& img2, ImgBinary* out) { iLessThan(img1, img2, out); }
void LessThan(const ImgGray&  img1, const ImgGray&  img2, ImgBinary* out) { iLessThan(img1, img2, out); }
void LessThan(const ImgInt&   img1, const ImgInt&   img2, ImgBinary* out) { iLessThan(img1, img2, out); }
void GreaterThan(const ImgFloat& img1, const ImgFloat& img2, ImgBinary* out) { iGreaterThan(img1, img2, out); }
void GreaterThan(const ImgGray&  img1, const ImgGray&  img2, ImgBinary* out) { iGreaterThan(img1, img2, out); }
void GreaterThan(const ImgInt&   img1, const ImgInt&   img2, ImgBinary* out) { iGreaterThan(img1, img2, out); }
void LessThanOrEqual(const ImgFloat& img1, const ImgFloat& img2, ImgBinary* out) { iLessThanOrEqual(img1, img2, out); }
void LessThanOrEqual(const ImgGray&  img1, const ImgGray&  img2, ImgBinary* out) { iLessThanOrEqual(img1, img2, out); }
void LessThanOrEqual(const ImgInt&   img1, const ImgInt&   img2, ImgBinary* out) { iLessThanOrEqual(img1, img2, out); }
void GreaterThanOrEqual(const ImgFloat& img1, const ImgFloat& img2, ImgBinary* out) { iGreaterThanOrEqual(img1, img2, out); }
void GreaterThanOrEqual(const ImgGray&  img1, const ImgGray&  img2, ImgBinary* out) { iGreaterThanOrEqual(img1, img2, out); }
void GreaterThanOrEqual(const ImgInt&   img1, const ImgInt&   img2, ImgBinary* out) { iGreaterThanOrEqual(img1, img2, out); }

void Equal(const ImgBinary& img, const ImgBinary::Pixel& pix, ImgBinary* out) { iEqual(img, pix, out); }
void Equal(const ImgBgr&    img, const ImgBgr   ::Pixel& pix, ImgBinary* out) { iEqual(img, pix, out); }
void Equal(const ImgFloat&  img, const ImgFloat ::Pixel& pix, ImgBinary* out) { iEqual(img, pix, out); }
void Equal(const ImgGray&   img, const ImgGray  ::Pixel& pix, ImgBinary* out) { iEqual(img, pix, out); }
void Equal(const ImgInt&    img, const ImgInt   ::Pixel& pix, ImgBinary* out) { iEqual(img, pix, out); }
void NotEqual(const ImgBinary& img, const ImgBinary::Pixel& pix, ImgBinary* out) { iNotEqual(img, pix, out); }
void NotEqual(const ImgBgr&    img, const ImgBgr   ::Pixel& pix, ImgBinary* out) { iNotEqual(img, pix, out); }
void NotEqual(const ImgFloat&  img, const ImgFloat ::Pixel& pix, ImgBinary* out) { iNotEqual(img, pix, out); }
void NotEqual(const ImgGray&   img, const ImgGray  ::Pixel& pix, ImgBinary* out) { iNotEqual(img, pix, out); }
void NotEqual(const ImgInt&    img, const ImgInt   ::Pixel& pix, ImgBinary* out) { iNotEqual(img, pix, out); }
void LessThan(const ImgFloat&  img, const ImgFloat ::Pixel& pix, ImgBinary* out) { iLessThan(img, pix, out); }
void LessThan(const ImgGray&   img, const ImgGray  ::Pixel& pix, ImgBinary* out) { iLessThan(img, pix, out); }
void LessThan(const ImgInt&    img, const ImgInt   ::Pixel& pix, ImgBinary* out) { iLessThan(img, pix, out); }
void GreaterThan(const ImgFloat&  img, const ImgFloat ::Pixel& pix, ImgBinary* out) { iGreaterThan(img, pix, out); }
void GreaterThan(const ImgGray&   img, const ImgGray  ::Pixel& pix, ImgBinary* out) { iGreaterThan(img, pix, out); }
void GreaterThan(const ImgInt&    img, const ImgInt   ::Pixel& pix, ImgBinary* out) { iGreaterThan(img, pix, out); }
void LessThanOrEqual(const ImgFloat&  img, const ImgFloat ::Pixel& pix, ImgBinary* out) { iLessThanOrEqual(img, pix, out); }
void LessThanOrEqual(const ImgGray&   img, const ImgGray  ::Pixel& pix, ImgBinary* out) { iLessThanOrEqual(img, pix, out); }
void LessThanOrEqual(const ImgInt&    img, const ImgInt   ::Pixel& pix, ImgBinary* out) { iLessThanOrEqual(img, pix, out); }
void GreaterThanOrEqual(const ImgFloat&  img, const ImgFloat ::Pixel& pix, ImgBinary* out) { iGreaterThanOrEqual(img, pix, out); }
void GreaterThanOrEqual(const ImgGray&   img, const ImgGray  ::Pixel& pix, ImgBinary* out) { iGreaterThanOrEqual(img, pix, out); }
void GreaterThanOrEqual(const ImgInt&    img, const ImgInt   ::Pixel& pix, ImgBinary* out) { iGreaterThanOrEqual(img, pix, out); }

void Resample(const ImgBinary& img, int new_width, int new_height, ImgBinary* out) { iResample(img, new_width, new_height, out); }
void Resample(const ImgBgr&    img, int new_width, int new_height, ImgBgr*    out) { iResample(img, new_width, new_height, out); }
void Resample(const ImgFloat&  img, int new_width, int new_height, ImgFloat*  out) { iResample(img, new_width, new_height, out); }
void Resample(const ImgGray&   img, int new_width, int new_height, ImgGray*   out) { iResample(img, new_width, new_height, out); }
void Resample(const ImgInt&    img, int new_width, int new_height, ImgInt*    out) { iResample(img, new_width, new_height, out); }
void Upsample(const ImgBinary& img, int factor_x, int factor_y, ImgBinary* out) { iUpsample(img, factor_x, factor_y, out); }
void Upsample(const ImgBgr&    img, int factor_x, int factor_y, ImgBgr*    out) { iUpsample(img, factor_x, factor_y, out); }
void Upsample(const ImgFloat&  img, int factor_x, int factor_y, ImgFloat*  out) { iUpsample(img, factor_x, factor_y, out); }
void Upsample(const ImgGray&   img, int factor_x, int factor_y, ImgGray*   out) { iUpsample(img, factor_x, factor_y, out); }
void Upsample(const ImgInt&    img, int factor_x, int factor_y, ImgInt*    out) { iUpsample(img, factor_x, factor_y, out); }

void Downsample(const ImgBinary& img, int factor_x, int factor_y, ImgBinary* out) { iDownsample(img, factor_x, factor_y, out); }
void Downsample(const ImgBgr&    img, int factor_x, int factor_y, ImgBgr*    out) { iDownsample(img, factor_x, factor_y, out); }
void Downsample(const ImgFloat&  img, int factor_x, int factor_y, ImgFloat*  out) { iDownsample(img, factor_x, factor_y, out); }
void Downsample(const ImgGray&   img, int factor_x, int factor_y, ImgGray*   out) { iDownsample(img, factor_x, factor_y, out); }
void Downsample(const ImgInt&    img, int factor_x, int factor_y, ImgInt*    out) { iDownsample(img, factor_x, factor_y, out); }
void Downsample2x2(const ImgBinary& img, ImgBinary* out) { iDownsample2x2(img, out); }
void Downsample2x2(const ImgBgr&    img, ImgBgr*    out) { iDownsample2x2(img, out); }
void Downsample2x2(const ImgFloat&  img, ImgFloat*  out) { iDownsample2x2(img, out); }
void Downsample2x2(const ImgGray&   img, ImgGray*   out) { iDownsample2x2(img, out); }
void Downsample2x2(const ImgInt&    img, ImgInt*    out) { iDownsample2x2(img, out); }

// Note:  cast to int (in Round), then to float, then to int should be simplified
ImgBinary::Pixel Interp(const ImgBinary& img, float x, float y)
{
  assert( img.Width() > 1 && img.Height() > 1 );
  if (x<0)  x = 0;
  else if (x >= img.Width()-1)  x = img.Width() - g_interp_extra;
  else x = static_cast<float>( blepo_ex::Round(x) );
  if (y<0)  y = 0;
  else if (y >= img.Height()-1)  y = img.Height() - g_interp_extra;
  else y = static_cast<float>( blepo_ex::Round(y) );
  assert( x >= 0 && y >= 0 && x <= img.Width()-1 && y <= img.Height()-1 );
  return img(static_cast<int>(x), static_cast<int>(y));
}

ImgBgr::Pixel Interp(const ImgBgr& img, float x, float y)
{
  assert( img.Width() > 1 && img.Height() > 1 );
  if (x<0)  x = 0;
  else if (x >= img.Width()-1)  x = img.Width() - g_interp_extra;
  if (y<0)  y = 0;
  else if (y >= img.Height()-1)  y = img.Height() - g_interp_extra;
  int xx = static_cast<int>(x);
  int yy = static_cast<int>(y);
  float ax = x - xx;
  float ay = y - yy;
  assert( xx >= 0 && yy >= 0 && xx < img.Width()-1 && yy < img.Height()-1 );
  const ImgBgr::Pixel& v00 = img(xx, yy);
  const ImgBgr::Pixel& v10 = img(xx+1, yy);
  const ImgBgr::Pixel& v01 = img(xx, yy+1);
  const ImgBgr::Pixel& v11 = img(xx+1, yy+1);

  float val_b = (1-ax) * (1-ay) * v00.b + ax * (1-ay) * v10.b
              + (1-ax) *   ay   * v01.b + ax *   ay   * v11.b;
  float val_g = (1-ax) * (1-ay) * v00.g + ax * (1-ay) * v10.g
              + (1-ax) *   ay   * v01.g + ax *   ay   * v11.g;
  float val_r = (1-ax) * (1-ay) * v00.r + ax * (1-ay) * v10.r
              + (1-ax) *   ay   * v01.r + ax *   ay   * v11.r;
  return ImgBgr::Pixel(blepo_ex::Round(val_b), blepo_ex::Round(val_g), blepo_ex::Round(val_r));
}

ImgFloat::Pixel Interp(const ImgFloat& img, float x, float y)
{
  assert( img.Width() > 1 && img.Height() > 1 );
  if (x<0)  x = 0;
  else if (x >= img.Width()-1)  x = img.Width() - 1.0001f;
  if (y<0)  y = 0;
  else if (y >= img.Height()-1)  y = img.Height() - 1.0001f;
  int xx = static_cast<int>(x);
  int yy = static_cast<int>(y);
  float ax = x - xx;
  float ay = y - yy;
  assert( xx >= 0 && yy >= 0 && xx < img.Width()-1 && yy < img.Height()-1 );
  float val = (1-ax) * (1-ay) * img(xx, yy  ) + ax * (1-ay) * img(xx+1, yy  )
            + (1-ax) *   ay   * img(xx, yy+1) + ax *   ay   * img(xx+1, yy+1);
  return val;
}

ImgGray::Pixel Interp(const ImgGray& img, float x, float y)
{
  assert( img.Width() > 1 && img.Height() > 1 );
  if (x<0)  x = 0;
  else if (x >= img.Width()-1)  x = img.Width() - 1.0001f;
  if (y<0)  y = 0;
  else if (y >= img.Height()-1)  y = img.Height() - 1.0001f;
  int xx = static_cast<int>(x);
  int yy = static_cast<int>(y);
  float ax = x - xx;
  float ay = y - yy;
  assert( xx >= 0 && yy >= 0 && xx < img.Width()-1 && yy < img.Height()-1 );
  float val = (1-ax) * (1-ay) * img(xx, yy  ) + ax * (1-ay) * img(xx+1, yy  )
            + (1-ax) *   ay   * img(xx, yy+1) + ax *   ay   * img(xx+1, yy+1);
  return blepo_ex::Round(val);
}

ImgInt::Pixel Interp(const ImgInt& img, float x, float y)
{
  assert( img.Width() > 1 && img.Height() > 1 );
  if (x<0)  x = 0;
  else if (x >= img.Width()-1)  x = img.Width() - 1.0001f;
  if (y<0)  y = 0;
  else if (y >= img.Height()-1)  y = img.Height() - 1.0001f;
  int xx = static_cast<int>(x);
  int yy = static_cast<int>(y);
  float ax = x - xx;
  float ay = y - yy;
  float val = (1-ax) * (1-ay) * img(xx, yy  ) + ax * (1-ay) * img(xx+1, yy  )
            + (1-ax) *   ay   * img(xx, yy+1) + ax *   ay   * img(xx+1, yy+1);
  return blepo_ex::Round(val);
}

void InterpRectCenter(const ImgBgr& img, float xc, float yc, int hw, int hh, ImgBgr* out)
{
  assert(hw >= 0 && hh >= 0);
  float left = xc-hw;
  float top = yc-hh;
  int width = 2*hw+1;
  int height = 2*hh+1;
  InterpRect(img, left, top, width, height, out);
}

void InterpRectCenter(const ImgFloat& img, float xc, float yc, int hw, int hh, ImgFloat* out)
{
  assert(hw >= 0 && hh >= 0);
  float left = xc-hw;
  float top = yc-hh;
  int width = 2*hw+1;
  int height = 2*hh+1;
  InterpRect(img, left, top, width, height, out);
//  
//  out->Reset(2*hw+1, 2*hh+1);
//  if (x-hw >= 0 && x+hw < img.Width()-1 && y-hh >= 0 && y+hh < img.Height()-1)
//  {
//    int xx = static_cast<int>(x);
//    int yy = static_cast<int>(y);
//    float ax = x - xx;
//    float ay = y - yy;
//    float mxmy = (1-ax) * (1-ay);
//    float pxmy =   ax   * (1-ay);
//    float mxpy = (1-ax) *   ay  ;
//    float pxpy =   ax   *   ay  ;
//    const int w = img.Width();
//    const int skip = img.Width() - out->Width();
//
//    const float* p = img.Begin(xx-hw, yy-hh);
//    float* q = out->Begin();
//
//    int dx, dy;
//    for (dy=-hh ; dy<=hh ; dy++)
//    {
//      for (dx=-hw ; dx<=hw ; dx++)
//      {
//        assert(p == img.Begin(xx+dx, yy+dy));
//        assert(q == out->Begin(dx+hw, dy+hh));
//        float val = mxmy * p[0] + pxmy * p[1] + mxpy * p[w] + pxpy * p[w+1];
//#ifndef NDEBUG
//        float ival = Interp(img, x+dx, y+dy);
//#endif
//        assert( fabs(val - Interp(img, x+dx, y+dy)) <= 0.01 );
//        *q = val;
//        p++;  q++;
//      }
//      p += skip;
//    }
//  }
//  else  // slow way -- could be improved if necessary
//  {
//    float* q = out->Begin();
//    int dx, dy;
//    for (dy=-hh ; dy<=hh ; dy++)
//    {
//      for (dx=-hw ; dx<=hw ; dx++)
//      {
//        float val = Interp(img, x+dx, y+dy);
//        *q++ = val;
//      }
//    }
//  }
}

void InterpRectCenter(const ImgGray& img, float xc, float yc, int hw, int hh, ImgGray* out)
{
  assert(hw >= 0 && hh >= 0);
  float left = xc-hw;
  float top = yc-hh;
  int width = 2*hw+1;
  int height = 2*hh+1;
  InterpRect(img, left, top, width, height, out);
}

void InterpRectCenter(const ImgInt& img, float xc, float yc, int hw, int hh, ImgInt* out)
{
  assert(hw >= 0 && hh >= 0);
  float left = xc-hw;
  float top = yc-hh;
  int width = 2*hw+1;
  int height = 2*hh+1;
  InterpRect(img, left, top, width, height, out);
}

void InterpRectCenterAsymmetric(const ImgBgr& img, float xc, float yc, int hwl, int hwr, int hht, int hhb, ImgBgr* out)
{
//  assert(hwl >= 0 && hwr >= 0 && hht >= 0 && hhb >= 0);
  assert(hwr >= -hwl && hhb >= -hht);
  float left = xc-hwl;
  float top = yc-hht;
  int width = hwl+hwr+1;
  int height = hht+hhb+1;
  InterpRect(img, left, top, width, height, out);
}

void InterpRectCenterAsymmetric(const ImgFloat& img, float xc, float yc, int hwl, int hwr, int hht, int hhb, ImgFloat* out)
{
//  assert(hwl >= 0 && hwr >= 0 && hht >= 0 && hhb >= 0);
  assert(hwr >= -hwl && hhb >= -hht);
  float left = xc-hwl;
  float top = yc-hht;
  int width = hwl+hwr+1;
  int height = hht+hhb+1;
  InterpRect(img, left, top, width, height, out);
//
//
//  out->Reset(hwl+hwr+1, hht+hhb+1);
//  if (x-hwl >= 0 && x+hwr < img.Width()-1 && y-hht >= 0 && y+hhb < img.Height()-1)
//  {
//    int xx = static_cast<int>(x);
//    int yy = static_cast<int>(y);
//    float ax = x - xx;
//    float ay = y - yy;
//    float mxmy = (1-ax) * (1-ay);
//    float pxmy =   ax   * (1-ay);
//    float mxpy = (1-ax) *   ay  ;
//    float pxpy =   ax   *   ay  ;
//    const int w = img.Width();
//    const int skip = img.Width() - out->Width();
//
//    const float* p = img.Begin(xx-hwl, yy-hht);
//    float* q = out->Begin();
//
//    int dx, dy;
//    for (dy=-hht ; dy<=hhb ; dy++)
//    {
//      for (dx=-hwl ; dx<=hwr ; dx++)
//      {
//        assert(p == img.Begin(xx+dx, yy+dy));
//        assert(q == out->Begin(dx+hwl, dy+hht));
//        float val = mxmy * p[0] + pxmy * p[1] + mxpy * p[w] + pxpy * p[w+1];
//#ifndef NDEBUG
//        float ival = Interp(img, x+dx, y+dy);
//#endif
//        assert( fabs(val - Interp(img, x+dx, y+dy)) <= 0.01 );
//        *q = val;
//        p++;  q++;
//      }
//      p += skip;
//    }
//  }
//  else  // slow way -- could be improved if necessary
//  {
//    float* q = out->Begin();
//    int dx, dy;
//    for (dy=-hht ; dy<=hhb ; dy++)
//    {
//      for (dx=-hwl ; dx<=hwr ; dx++)
//      {
//        float val = Interp(img, x+dx, y+dy);
//        *q++ = val;
//      }
//    }
//  }
}

void InterpRectCenterAsymmetric(const ImgGray& img, float xc, float yc, int hwl, int hwr, int hht, int hhb, ImgGray* out)
{
//  assert(hwl >= 0 && hwr >= 0 && hht >= 0 && hhb >= 0);
  assert(hwr >= -hwl && hhb >= -hht);
  float left = xc-hwl;
  float top = yc-hht;
  int width = hwl+hwr+1;
  int height = hht+hhb+1;
  InterpRect(img, left, top, width, height, out);
}

void InterpRectCenterAsymmetric(const ImgInt& img, float xc, float yc, int hwl, int hwr, int hht, int hhb, ImgInt* out)
{
//  assert(hwl >= 0 && hwr >= 0 && hht >= 0 && hhb >= 0);
  assert(hwr >= -hwl && hhb >= -hht);
  float left = xc-hwl;
  float top = yc-hht;
  int width = hwl+hwr+1;
  int height = hht+hhb+1;
  InterpRect(img, left, top, width, height, out);
}

void InterpRect(const ImgBgr& img, float x, float y, int width, int height, ImgBgr* out)
{
  assert(width > 0 && height > 0);
  out->Reset(width, height);
  if (x >= 0 && x+width < img.Width()-1 && y >= 0 && y+height < img.Height()-1)
  {
    int xx = static_cast<int>(x);
    int yy = static_cast<int>(y);
    float ax = x - xx;
    float ay = y - yy;
    float mxmy = (1-ax) * (1-ay);
    float pxmy =   ax   * (1-ay);
    float mxpy = (1-ax) *   ay  ;
    float pxpy =   ax   *   ay  ;
    const int w = img.Width();
    const int skip = img.Width() - out->Width();

    ImgBgr::ConstIterator p = img.Begin(xx, yy);
    ImgBgr::Iterator q = out->Begin();

    int dx, dy;
    for (dy=0 ; dy<height ; dy++)
    {
      for (dx=0 ; dx<width ; dx++)
      {
        assert(p == img.Begin(xx+dx, yy+dy));
        assert(q == out->Begin(dx, dy));

        float val_b = mxmy * p[0].b + pxmy * p[1].b + mxpy * p[w].b + pxpy * p[w+1].b;
        float val_g = mxmy * p[0].g + pxmy * p[1].g + mxpy * p[w].g + pxpy * p[w+1].g;
        float val_r = mxmy * p[0].r + pxmy * p[1].r + mxpy * p[w].r + pxpy * p[w+1].r;
        ImgBgr::Pixel val = ImgBgr::Pixel(blepo_ex::Round(val_b), blepo_ex::Round(val_g), blepo_ex::Round(val_r));
#ifndef NDEBUG
        ImgBgr::Pixel ival = Interp(img, x+dx, y+dy);
        assert( val == ival );
#endif
        *q = val;
        p++;  q++;
      }
      p += skip;
    }
  }
  else  // slow way -- could be improved if necessary
  {
    ImgBgr::Iterator q = out->Begin();
    int dx, dy;
    for (dy=0 ; dy<height ; dy++)
    {
      for (dx=0 ; dx<width ; dx++)
      {
        ImgBgr::Pixel val = Interp(img, x+dx, y+dy);
        *q++ = val;
      }
    }
  }
}

void InterpRect(const ImgFloat& img, float x, float y, int width, int height, ImgFloat* out)
{
  assert(width > 0 && height > 0);
  out->Reset(width, height);
  if (x >= 0 && x+width < img.Width()-1 && y >= 0 && y+height < img.Height()-1)
  {
    int xx = static_cast<int>(x);
    int yy = static_cast<int>(y);
    float ax = x - xx;
    float ay = y - yy;
    float mxmy = (1-ax) * (1-ay);
    float pxmy =   ax   * (1-ay);
    float mxpy = (1-ax) *   ay  ;
    float pxpy =   ax   *   ay  ;
    const int w = img.Width();
    const int skip = img.Width() - out->Width();

    ImgFloat::ConstIterator p = img.Begin(xx, yy);
    ImgFloat::Iterator q = out->Begin();

    int dx, dy;
    for (dy=0 ; dy<height ; dy++)
    {
      for (dx=0 ; dx<width ; dx++)
      {
        assert(p == img.Begin(xx+dx, yy+dy));
        assert(q == out->Begin(dx, dy));
        ImgFloat::Pixel val = mxmy * p[0] + pxmy * p[1] + mxpy * p[w] + pxpy * p[w+1];
#ifndef NDEBUG
        ImgFloat::Pixel ival = Interp(img, x+dx, y+dy);
        assert( fabs(val - ival) <= 0.01 );
#endif
        *q = val;
        p++;  q++;
      }
      p += skip;
    }
  }
  else  // slow way -- could be improved if necessary
  {
    ImgFloat::Iterator q = out->Begin();
    int dx, dy;
    for (dy=0 ; dy<height ; dy++)
    {
      for (dx=0 ; dx<width ; dx++)
      {
        ImgFloat::Pixel val = Interp(img, x+dx, y+dy);
        *q++ = val;
      }
    }
  }
}

void InterpRect(const ImgGray& img, float x, float y, int width, int height, ImgGray* out)
{
  assert(width > 0 && height > 0);
  out->Reset(width, height);
  if (x >= 0 && x+width < img.Width()-1 && y >= 0 && y+height < img.Height()-1)
  {
    int xx = static_cast<int>(x);
    int yy = static_cast<int>(y);
    float ax = x - xx;
    float ay = y - yy;
    float mxmy = (1-ax) * (1-ay);
    float pxmy =   ax   * (1-ay);
    float mxpy = (1-ax) *   ay  ;
    float pxpy =   ax   *   ay  ;
    const int w = img.Width();
    const int skip = img.Width() - out->Width();

    ImgGray::ConstIterator p = img.Begin(xx, yy);
    ImgGray::Iterator q = out->Begin();

    int dx, dy;
    for (dy=0 ; dy<height ; dy++)
    {
      for (dx=0 ; dx<width ; dx++)
      {
        assert(p == img.Begin(xx+dx, yy+dy));
        assert(q == out->Begin(dx, dy));
        ImgGray::Pixel val = blepo_ex::Round( mxmy * p[0] + pxmy * p[1] + mxpy * p[w] + pxpy * p[w+1] );
#ifndef NDEBUG
        ImgGray::Pixel ival = Interp(img, x+dx, y+dy);
        assert( val == ival );
#endif
        *q = val;
        p++;  q++;
      }
      p += skip;
    }
  }
  else  // slow way -- could be improved if necessary
  {
    ImgGray::Iterator q = out->Begin();
    int dx, dy;
    for (dy=0 ; dy<height ; dy++)
    {
      for (dx=0 ; dx<width ; dx++)
      {
        ImgGray::Pixel val = Interp(img, x+dx, y+dy);
        *q++ = val;
      }
    }
  }
}

void InterpRect(const ImgInt& img, float x, float y, int width, int height, ImgInt* out)
{
  assert(width > 0 && height > 0);
  out->Reset(width, height);
  if (x >= 0 && x+width < img.Width()-1 && y >= 0 && y+height < img.Height()-1)
  {
    int xx = static_cast<int>(x);
    int yy = static_cast<int>(y);
    float ax = x - xx;
    float ay = y - yy;
    float mxmy = (1-ax) * (1-ay);
    float pxmy =   ax   * (1-ay);
    float mxpy = (1-ax) *   ay  ;
    float pxpy =   ax   *   ay  ;
    const int w = img.Width();
    const int skip = img.Width() - out->Width();

    ImgInt::ConstIterator p = img.Begin(xx, yy);
    ImgInt::Iterator q = out->Begin();

    int dx, dy;
    for (dy=0 ; dy<height ; dy++)
    {
      for (dx=0 ; dx<width ; dx++)
      {
        assert(p == img.Begin(xx+dx, yy+dy));
        assert(q == out->Begin(dx, dy));
        ImgInt::Pixel val = blepo_ex::Round( mxmy * p[0] + pxmy * p[1] + mxpy * p[w] + pxpy * p[w+1] );
#ifndef NDEBUG
        ImgInt::Pixel ival = Interp(img, x+dx, y+dy);
        assert( val == ival );
#endif
        *q = val;
        p++;  q++;
      }
      p += skip;
    }
  }
  else  // slow way -- could be improved if necessary
  {
    ImgInt::Iterator q = out->Begin();
    int dx, dy;
    for (dy=0 ; dy<height ; dy++)
    {
      for (dx=0 ; dx<width ; dx++)
      {
        ImgInt::Pixel val = Interp(img, x+dx, y+dy);
        *q++ = val;
      }
    }
  }
}



// old:
//
//void Copy(ImgInt* out, const CPoint& pt, const ImgInt& img)
//{
//  ImgInt::ConstIterator p = img.Begin();
//  ImgInt::Iterator q = out->Begin(pt.x, pt.y);
//  int skip = out->Width() - img.Width();
//  for (int y=0 ; y<img.Height() ; y++, q += skip)
//  {
//    for (int x=0 ; x<img.Width() ; x++)  *q++ = *p++;
//  }
//}
///
//void Set(ImgInt* out, ImgInt::Pixel val, const Rect& rect)
//{
//  ImgInt::Iterator p;
//  int skip = out->Width() - (rect.right - rect.left);
//  p = out->Begin(rect.left, rect.top);
//  for (int y=rect.top ; y<rect.bottom ; y++)
//  {
//    for (int x=rect.left ; x<rect.right ; x++)
//    {
//      *p++ = val;
//    }
//    p += skip;
//  }
//}
//void ExtractRect(const ImgBgr& img, int l, int t, int r, int b, ImgBgr* out)
//{
//  assert(l>=0 && l<r);
//  assert(t>=0 && t<b);
//  assert(r<img.Width());
//  assert(b<img.Height());
//
//  out->Reset(r-l, b-t);
//  for (int y=t ; y<b ; y++)
//  {
//    ImgBgr::ConstIterator pi = img.Begin(l, y);
//    ImgBgr::Iterator po = out->Begin(0, y-t);
//    memcpy(po, pi, (r-l)*sizeof(ImgBgr::Pixel));
////
////    for (int x=l ; x<r ; x++)
////    {
////      *po++ = *pi++;
////    }
//  }
//}


void Rand(int width, int height, ImgBgr* out)
{
  out->Reset(width, height);
  ImgBgr::Iterator p;
  for (p = out->Begin() ; p != out->End() ; p++)
  {
    p->b = blepo_ex::GetRand(0, 256);
    p->g = blepo_ex::GetRand(0, 256);
    p->r = blepo_ex::GetRand(0, 256);
    p++;
  }
}

void Rand(int width, int height, ImgBinary* out)
{
  out->Reset(width, height);
  ImgBinary::Iterator p = out->Begin();
  while (p != out->End())  *p++ = blepo_ex::GetRand(0, 2) != 0;
}

void Rand(int width, int height, ImgGray* out)
{
  out->Reset(width, height);
  ImgGray::Iterator p = out->Begin();
  while (p != out->End())  *p++ = blepo_ex::GetRand(0, 256);
}

void Rand(int width, int height, ImgInt* out)
{
  out->Reset(width, height);
  ImgInt::Iterator p = out->Begin();
  while (p != out->End())  *p++ = blepo_ex::GetRand(0, ImgInt::MAX_VAL);
}

void RampX(int width, int height, ImgFloat* out)
{
  out->Reset(width, height);
  int x, y;
  for (y=0 ; y<height ; y++)
  {
    for (x=0 ; x<width ; x++)
    {
      (*out)(x,y) = (float) x;
    }
  }
}

void RampY(int width, int height, ImgFloat* out)
{
  out->Reset(width, height);
  int x, y;
  for (y=0 ; y<height ; y++)
  {
    for (x=0 ; x<width ; x++)
    {
      (*out)(x,y) = (float) y;
    }
  }
}

Bgr GetRandColor()
{
  Bgr color;
  color.b = (unsigned char) blepo_ex::GetRand(0, 256);
  color.g = (unsigned char) blepo_ex::GetRand(0, 256);
  color.r = (unsigned char) blepo_ex::GetRand(0, 256);
  return color;
}

Bgr GetRandSaturatedColor()
{
//  double h = blepo_ex::GetRandDbl();
//  double s = 1;
//  double v = 1;
//  double b, g, r;
//  iHsvToBgr(h, s, v, &b, &g, &r);
//  return Bgr(b, g, r);
//
  const int n = 6;
  Bgr colors[n] = 
  { 
    Bgr(255, 0, 0),    // blue
    Bgr(0, 255, 0),    // green
    Bgr(0, 0, 255),    // red
    Bgr(255, 255, 0),  // cyan
    Bgr(255, 0, 255),  // magenta
    Bgr(0, 255, 255),  // yellow 
  };
  int i = blepo_ex::GetRand(0, n);
  return colors[i];
}

void PseudoColor(const ImgInt& img, ImgBgr* out)
{
  // convert to pseudo-random colors
  out->Reset(img.Width(), img.Height());
  ImgInt::ConstIterator p;
  ImgBgr::Iterator q;
  for (p = img.Begin(), q = out->Begin() ; p != img.End() ; p++, q++)
  {
    // There is no significance to the integer here
    *q = ImgBgr::Pixel((*p * 12342223) % 0xFFFFFF, Bgr::BLEPO_BGR_XBGR);
  }
}

bool IsGrayscale(const ImgBgr& img)
{
  ImgBgr::ConstIterator p = img.Begin();
  while (p != img.End())
  {
    if ((p->b != p->g) || (p->b != p->r) || (p->g != p->r))  return false;
    p++;
  }
  return true;
}

void Draw(const ImgBgr& img, HDC hdc, const Rect& src, const Rect& dst)
{
  iImgBitmapinfo ibmi(img);

  int mode_orig = ::SetStretchBltMode(hdc, STRETCH_DELETESCANS);
  assert(mode_orig != 0);  // value of 0 indicates an error occurred

  int ret = ::StretchDIBits(hdc, 
                            dst.left, dst.top, dst.Width(), dst.Height(), // destination
                            src.left, src.top, src.Width(), src.Height(), // source
                            ibmi.GetDataPtr(),
                            ibmi.GetBitmapInfo(), DIB_RGB_COLORS, SRCCOPY);
  assert(ret != GDI_ERROR);

  ::SetStretchBltMode(hdc, mode_orig);

/* old way that works fine; replaced with iImgBitmapinfo class above
  int width = img.Width();
  int height = img.Height();
  int nbits_per_pixel = ImgBgr::NBITS_PER_PIXEL;
  int nbytes = iComputeAlignment32(width, height, nbits_per_pixel);

  BITMAPINFO bmi;
  BITMAPINFOHEADER* bh = &(bmi.bmiHeader);
  bh->biSize = sizeof(BITMAPINFOHEADER);  
  bh->biWidth = width;
  bh->biHeight = -height;  // negative sign indicates top-down (right-side-up)
  bh->biPlanes = 1;
  bh->biBitCount = (WORD) nbits_per_pixel;
  bh->biCompression = BI_RGB;
  bh->biSizeImage = nbytes;
  bh->biXPelsPerMeter = 0;
  bh->biYPelsPerMeter = 0;
  bh->biClrUsed = 0;
  bh->biClrImportant = 0;	

  const unsigned char* data_ptr;
  Array<unsigned char> data;

  if (nbytes == width * height * 3)
  {
    data_ptr = reinterpret_cast<const unsigned char*>(img.Begin());
  }
  else
  {
    // align data to match expectations of StretchDIBits
    data.Reset(nbytes);
    ImgBgr::ConstIterator pi = img.Begin();
    for (int y=0 ; y<img.Height() ; y++)
    {
      unsigned char* po = data.Begin() + y*(nbytes / height);
      for (int x=0 ; x<img.Width() ; x++)
      {
        const ImgBgr::Pixel& pix = *pi++;
        *po++ = pix.b;
        *po++ = pix.g;
        *po++ = pix.r;
      }
    }
    data_ptr = data.Begin();
  }

  int mode_orig = ::SetStretchBltMode(hdc, STRETCH_DELETESCANS);
  assert(mode_orig != 0);  // value of 0 indicates an error occurred

  int ret = ::StretchDIBits(hdc, 
                            dst.left, dst.top, dst.Width(), dst.Height(), // destination
                            src.left, src.top, src.Width(), src.Height(), // source
                            data_ptr,
                            &bmi, DIB_RGB_COLORS, SRCCOPY);
  assert(ret != GDI_ERROR);

  ::SetStretchBltMode(hdc, mode_orig);
*/
}

void Draw(const ImgBgr& img, HDC hdc, int x, int y)
{
  Draw(img, hdc, Rect(0, 0, img.Width(), img.Height()), Rect(x, y, x+img.Width(), y+img.Height()));
}

void Draw(const ImgBgr& img, CDC& dc, int x, int y)
{
  Draw(img, dc.m_hDC, x, y);
}

void Draw(const ImgBinary& img, HDC hdc, int x, int y)
{
  Draw(img, hdc, Rect(0, 0, img.Width(), img.Height()), Rect(x, y, x+img.Width(), y+img.Height()));
}

void Draw(const ImgBinary& img, CDC& dc, int x, int y)
{
  Draw(img, dc.m_hDC, Rect(0, 0, img.Width(), img.Height()), Rect(x, y, x+img.Width(), y+img.Height()));
}

void Draw(const ImgFloat& img, HDC hdc, int x, int y)
{
  Draw(img, hdc, Rect(0, 0, img.Width(), img.Height()), Rect(x, y, x+img.Width(), y+img.Height()));
}

void Draw(const ImgFloat& img, CDC& dc, int x, int y)
{
  Draw(img, dc.m_hDC, Rect(0, 0, img.Width(), img.Height()), Rect(x, y, x+img.Width(), y+img.Height()));
}

void Draw(const ImgGray& img, CDC& dc, int x, int y)
{
  Draw(img, dc.m_hDC, Rect(0, 0, img.Width(), img.Height()), Rect(x, y, x+img.Width(), y+img.Height()));
}

void Draw(const ImgGray& img, HDC hdc, int x, int y)
{
  Draw(img, hdc, Rect(0, 0, img.Width(), img.Height()), Rect(x, y, x+img.Width(), y+img.Height()));
}

void Draw(const ImgInt& img, HDC hdc, int x, int y)
{
  Draw(img, hdc, Rect(0, 0, img.Width(), img.Height()), Rect(x, y, x+img.Width(), y+img.Height()));
}

void Draw(const ImgInt& img, CDC& dc, int x, int y)
{
  Draw(img, dc, Rect(0, 0, img.Width(), img.Height()), Rect(x, y, x+img.Width(), y+img.Height()));
}

void Draw(const ImgBgr& img, CDC& dc, const Rect& src, const Rect& dst)
{
  Draw(img, dc.m_hDC, src, dst);
}

void Draw(const ImgBinary& img, HDC hdc, const Rect& src, const Rect& dst)
{
  ImgGray gray;
  Convert(img, &gray, 0, 255);
  Draw(gray, hdc, src, dst);
}

void Draw(const ImgBinary& img, CDC& dc, const Rect& src, const Rect& dst)
{
  Draw(img, dc.m_hDC, src, dst);
}

void Draw(const ImgFloat& img, HDC hdc, const Rect& src, const Rect& dst)
{
  ImgFloat img2;
  ImgGray gray;
  LinearlyScale(img, 0, 255, &img2);
  Convert(img2, &gray);
  Draw(gray, hdc, src, dst);
}

void Draw(const ImgFloat& img, CDC& dc, const Rect& src, const Rect& dst)
{
  Draw(img, dc.m_hDC, src, dst);
}

void Draw(const ImgGray& img, HDC hdc, const Rect& src, const Rect& dst)
{
  ImgBgr bgr;
  Convert(img, &bgr);
  Draw(bgr, hdc, src, dst);
}

void Draw(const ImgGray& img, CDC& dc, const Rect& src, const Rect& dst)
{
  Draw(img, dc.m_hDC, src, dst);
}

void Draw(const ImgInt& img, HDC hdc, const Rect& src, const Rect& dst)
{
  ImgInt img2;
  ImgGray gray;
  LinearlyScale(img, 0, 255, &img2);
  Convert(img2, &gray);
  Draw(gray, hdc, src, dst);
}

void Draw(const ImgInt& img, CDC& dc, const Rect& src, const Rect& dst)
{
  Draw(img, dc.m_hDC, src, dst);
}

// old:
//void Draw(const ImgGray& img, HDC hdc, const Rect& src, const Rect& dst)
//{
//  ImgBgr bgr;
//  Convert(img, &bgr);
//  Draw(bgr, hdc, src, dst);
//}
//
//void Draw(const ImgGray& img, CDC& dc, const Rect& src, const Rect& dst)
//{
//  ImgBgr bgr;
//  Convert(img, &bgr);
//  Draw(bgr, dc.m_hDC, src, dst);
//}

bool RectInsideImage(const Rect& rect, int width, int height)
{
	return (		rect.left < rect.right && rect.top < rect.bottom 
					&&	rect.left >= 0 && rect.top >= 0 
					&&	rect.right < width && rect.bottom < height
				  );
}

bool PointInsideImage(const blepo::Point& pt, int width, int height)
{
	return (	pt.x >=0 && pt.y>=0 &&	pt.x < width && pt.y < height );
}

void DrawDot(const Point& pt, ImgBgr* out, const Bgr& color, int size, bool inside_image_check)
{
  int b0 = size / 2;
  int b1 = size - b0;
  int x0 = blepo_ex::Max((int) pt.x-b0, 0);
  int x1 = blepo_ex::Min((int) pt.x+b1, out->Width());
  int y0 = blepo_ex::Max((int) pt.y-b0, 0);
  int y1 = blepo_ex::Min((int) pt.y+b1, out->Height());

	if(inside_image_check)
	{
		Rect rect(x0, y0, x1, y1);
		if(RectInsideImage(rect, out->Width(), out->Height()))
		{
			Set(out, rect, color);
		}
	}
	else
	{
			Set(out, Rect(x0, y0, x1, y1), color);
	}
}

void DrawDot(const Point& pt, ImgFloat* out, float color, int size, bool inside_image_check)
{
  int b0 = size / 2;
  int b1 = size - b0;
  int x0 = blepo_ex::Max((int) pt.x-b0, 0);
  int x1 = blepo_ex::Min((int) pt.x+b1, out->Width());
  int y0 = blepo_ex::Max((int) pt.y-b0, 0);
  int y1 = blepo_ex::Min((int) pt.y+b1, out->Height());

	if(inside_image_check)
	{
		Rect rect(x0, y0, x1, y1);
		if(RectInsideImage(rect, out->Width(), out->Height()))
		{
			Set(out, rect, color);
		}
	}
	else
	{
			Set(out, Rect(x0, y0, x1, y1), color);
	}
}

void DrawDot(const Point& pt, ImgGray* out, unsigned char color, int size, bool inside_image_check)
{
  int b0 = size / 2;
  int b1 = size - b0;
  int x0 = blepo_ex::Max((int) pt.x-b0, 0);
  int x1 = blepo_ex::Min((int) pt.x+b1, out->Width());
  int y0 = blepo_ex::Max((int) pt.y-b0, 0);
  int y1 = blepo_ex::Min((int) pt.y+b1, out->Height());

	Rect rect(x0, y0, x1, y1);
	if(inside_image_check)
	{
		Rect rect(x0, y0, x1, y1);
		if(RectInsideImage(rect, out->Width(), out->Height()))
		{
			Set(out, rect, color);
		}
	}
	else
	{
			Set(out, Rect(x0, y0, x1, y1), color);
	}
}

void DrawDot(const Point& pt, ImgInt* out, int color, int size , bool inside_image_check)
{
  int b0 = size / 2;
  int b1 = size - b0;
  int x0 = blepo_ex::Max((int) pt.x-b0, 0);
  int x1 = blepo_ex::Min((int) pt.x+b1, out->Width());
  int y0 = blepo_ex::Max((int) pt.y-b0, 0);
  int y1 = blepo_ex::Min((int) pt.y+b1, out->Height());

	Rect rect(x0, y0, x1, y1);
	if(inside_image_check)
	{
		Rect rect(x0, y0, x1, y1);
		if(RectInsideImage(rect, out->Width(), out->Height()))
		{
			Set(out, rect, color);
		}
	}
	else
	{
			Set(out, Rect(x0, y0, x1, y1), color);
	}
}

void DrawLine(const Point& pt1, const Point& pt2, ImgBgr* out, const Bgr& color, int thickness)
{
  CvPoint p1 = cvPoint(pt1.x,pt1.y), p2 = cvPoint(pt2.x,pt2.y);
  ImgIplImage foo(*out);
  cvLine(foo, p1, p2, cvScalar(color.b, color.g, color.r), thickness);
  foo.CastToBgr(out);
}

void DrawLine(const Point& pt1, const Point& pt2, ImgFloat* out, float color, int thickness)
{
  CvPoint p1 = cvPoint(pt1.x,pt1.y), p2 = cvPoint(pt2.x,pt2.y);
  ImgIplImage foo(*out);
  cvLine(foo, p1, p2, cvScalar(color), thickness);
  foo.CastToFloat(out);
}

void DrawLine(const Point& pt1, const Point& pt2, ImgGray* out, unsigned char color, int thickness)
{
  CvPoint p1 = cvPoint(pt1.x,pt1.y), p2 = cvPoint(pt2.x,pt2.y);
  ImgIplImage foo(*out);
  cvLine(foo, p1, p2, cvScalar(color, color, color), thickness);
  foo.CastToGray(out);
}

void DrawLine(const Point& pt1, const Point& pt2, ImgInt* out, int color, int thickness)
{
  CvPoint p1 = cvPoint(pt1.x,pt1.y), p2 = cvPoint(pt2.x,pt2.y);
  ImgIplImage foo(*out);
  cvLine(foo, p1, p2, cvScalar(color), thickness);
  foo.CastToInt(out);
}
void DrawLine(const Point& pt1, const Point& pt2, ImgBinary * out, int color, int thickness)
{
	ImgInt tmp;
	Convert(*out, &tmp);
	DrawLine(pt1, pt2, &tmp, color, thickness);
	Threshold(tmp, 1, out);
}

void DrawRect(const Rect& rect, ImgBgr* out, const Bgr& color, int thickness)
{
  CvPoint pt1 = cvPoint(rect.left,rect.top), pt2 = cvPoint(rect.right,rect.bottom);
  ImgIplImage foo(*out);
  cvRectangle(foo, pt1, pt2, cvScalar(color.b, color.g, color.r), thickness);
  foo.CastToBgr(out);
}

void DrawRect(const Rect& rect, ImgFloat* out, float color, int thickness)
{
  CvPoint pt1 = cvPoint(rect.left,rect.top), pt2 = cvPoint(rect.right,rect.bottom);
  ImgIplImage foo(*out);
  cvRectangle(foo, pt1, pt2, cvScalar(color), thickness);
  foo.CastToFloat(out);
}

void DrawRect(const Rect& rect, ImgGray* out, unsigned char color, int thickness)
{
  CvPoint pt1 = cvPoint(rect.left,rect.top), pt2 = cvPoint(rect.right,rect.bottom);
  ImgIplImage foo(*out);
  cvRectangle(foo, pt1, pt2, cvScalar(color, color, color), thickness);
  foo.CastToGray(out);
}

void DrawRect(const Rect& rect, ImgInt* out, int color, int thickness)
{
  CvPoint pt1 = cvPoint(rect.left,rect.top), pt2 = cvPoint(rect.right,rect.bottom);
  ImgIplImage foo(*out);
  cvRectangle(foo, pt1, pt2, cvScalar(color), thickness);
  foo.CastToInt(out);
}

void DrawCircle(const Point& center, int radius, ImgBgr* out, const Bgr& color, int thickness)
{
  CvPoint cen = cvPoint(center.x, center.y);
  ImgIplImage foo(*out);
  cvCircle(foo, cen, radius, cvScalar(color.b, color.g, color.r), thickness);
  foo.CastToBgr(out);
}

void DrawCircle(const Point& center, int radius, ImgFloat* out, float color, int thickness)
{
  CvPoint cen = cvPoint(center.x, center.y);
  ImgIplImage foo(*out);
  cvCircle(foo, cen, radius, cvScalar(color), thickness);
  foo.CastToFloat(out);
}

void DrawCircle(const Point& center, int radius, ImgGray* out, unsigned char color, int thickness)
{
  CvPoint cen = cvPoint(center.x, center.y);
  ImgIplImage foo(*out);
  cvCircle(foo, cen, radius, cvScalar(color), thickness);
  foo.CastToGray(out);
}

void DrawCircle(const Point& center, int radius, ImgInt* out, int color, int thickness)
{
  CvPoint cen = cvPoint(center.x, center.y);
  ImgIplImage foo(*out);
  cvCircle(foo, cen, radius, cvScalar(color), thickness);
  foo.CastToInt(out);
}

void DrawEllipse(const Point& center, int major_axis, int minor_axis, double angle, ImgBgr* out, const Bgr& color, int thickness)
{
  DrawEllipticArc(center, major_axis, minor_axis, angle, 0, 360, out, color, thickness);
}

void DrawEllipse(const Point& center, int major_axis, int minor_axis, double angle, ImgFloat* out, float color, int thickness)
{
  DrawEllipticArc(center, major_axis, minor_axis, angle, 0, 360, out, color, thickness);
}

void DrawEllipse(const Point& center, int major_axis, int minor_axis, double angle, ImgGray* out, unsigned char color, int thickness)
{
  DrawEllipticArc(center, major_axis, minor_axis, angle, 0, 360, out, color, thickness);
}

void DrawEllipse(const Point& center, int major_axis, int minor_axis, double angle, ImgInt* out, int color, int thickness)
{
  DrawEllipticArc(center, major_axis, minor_axis, angle, 0, 360, out, color, thickness);
}

void DrawEllipticArc(const Point& center, int major_axis, int minor_axis, double angle, double start_angle, double end_angle, ImgBgr* out, const Bgr& color, int thickness)
{
  CvPoint cen = cvPoint(center.x, center.y);
  CvSize axes = cvSize(major_axis, minor_axis);
  ImgIplImage foo(*out);
  // multiply angles by -1, because cvEllipse uses plot axes rather than image axes
  cvEllipse(foo, cen, axes, -angle, -start_angle, -end_angle, cvScalar(color.b, color.g, color.r), thickness);
  
  foo.CastToBgr(out);
}

void DrawEllipticArc(const Point& center, int major_axis, int minor_axis, double angle, double start_angle, double end_angle, ImgFloat* out, float color, int thickness)
{
  CvPoint cen = cvPoint(center.x, center.y);
  CvSize axes = cvSize(major_axis, minor_axis);
  ImgIplImage foo(*out);
  cvEllipse(foo, cen, axes, angle, start_angle, end_angle, cvScalar(color), thickness);
  foo.CastToFloat(out);
}

void DrawEllipticArc(const Point& center, int major_axis, int minor_axis, double angle, double start_angle, double end_angle, ImgGray* out, unsigned char color, int thickness)
{
  CvPoint cen = cvPoint(center.x, center.y);
  CvSize axes = cvSize(major_axis, minor_axis);
  ImgIplImage foo(*out);
  cvEllipse(foo, cen, axes, angle, start_angle, end_angle, cvScalar(color, color, color), thickness);
  foo.CastToGray(out);
}

void DrawEllipticArc(const Point& center, int major_axis, int minor_axis, double angle, double start_angle, double end_angle, ImgInt* out, int color, int thickness)
{
  CvPoint cen = cvPoint(center.x, center.y);
  CvSize axes = cvSize(major_axis, minor_axis);
  ImgIplImage foo(*out);
  cvEllipse(foo, cen, axes, angle, start_angle, end_angle, cvScalar(color), thickness);
  foo.CastToInt(out);
}

// This macro makes it easier to access 'm_font', which is void* in the header file to prevent header leak.
#define GGET_FONT (static_cast<CvFont*>( m_font ))  

/// Constructor initializes font
/// 'height':  Height of font; approximately height of lower case letter 'o' in pixels
/// 'thickness':  Thickness of stroke
TextDrawer::TextDrawer(int height, int thickness)
{
  m_font = (void*) new CvFont;
  double scale = (double) height / 20;
  cvInitFont( GGET_FONT, CV_FONT_VECTOR0, scale, scale, 0, thickness);
}

TextDrawer::~TextDrawer()
{
  delete GGET_FONT;
}

/// Returns the size, in pixels, of the bounding box enclosing the text
Size TextDrawer::GetTextSize(const char* text)
{
  CvSize text_size;
  int baseline;
  cvGetTextSize(text, GGET_FONT, &text_size, &baseline);

  Size sz;
  sz.cx = text_size.width;
  sz.cy = text_size.height + baseline + 1;
  return sz;
}

/// Returns the bounding rectangle, in pixels, enclosing the text
/// 'pt':  top-left corner of the rectangle
Rect TextDrawer::GetTextBoundingRect(const char* text, const Point& pt)
{
  Size sz = GetTextSize(text);
  return Rect(pt.x, pt.y, pt.x + sz.cx, pt.y + sz.cy);
}

/// Draws text on an image, replacing the pixels
/// 'img':  both input and output
/// 'pt':  top-left corner of region in which to draw text
/// 'color':  color of text
void TextDrawer::DrawText(ImgBgr* img, const char* text, const Point& pt, const Bgr& color)
{
  CvSize text_size;
  int baseline;
  cvGetTextSize(text, GGET_FONT, &text_size, &baseline);

  ImgIplImage ipl(*img);
  cvPutText( ipl, text, cvPoint(pt.x, pt.y + text_size.height), GGET_FONT, CV_RGB(color.r, color.g, color.b) );
  ipl.CastToBgr(img);
}

/// Same as previous function, but first clears the bounding box of the text
/// with a solid color.
/// 'background_color':  The color with which to fill the background before drawing text.
void TextDrawer::DrawText(ImgBgr* img, const char* text, const Point& pt, const Bgr& color, const Bgr& background_color)
{
  Rect rect;
  rect.IntersectRect(GetTextBoundingRect(text, pt), Rect(0, 0, img->Width(), img->Height()));
  Set(img, rect, background_color);
  DrawText(img, text, pt, color);
}

template <typename U, typename T>
inline U iSum(const Image<T>& img, const Rect& rect)
{
  Image<T>::ConstIterator p;
  int skip = img.Width() - (rect.right - rect.left);
  U total = 0;
  p = img.Begin(rect.left, rect.top);
  for (int y=rect.top ; y<rect.bottom ; y++)
  {
    for (int x=rect.left ; x<rect.right ; x++)
    {
      total += *p++;
    }
    p += skip;
  }
  return total;
}

template <typename U, typename T>
inline U iSum(const Image<T>& img, const ImgBinary& mask)
{
  Image<T>::ConstIterator p = img.Begin();
  ImgBinary::ConstIterator q = mask.Begin();
  U total = 0;
  for ( ; p != img.End() ; p++)  if (*q++)  total += *p;
  return total;
}
int   Sum(const ImgBinary& img, const Rect& rect)      { return iSum<int>  (img, rect); }
int   Sum(const ImgGray&   img, const Rect& rect)      { return iSum<int>  (img, rect); }
float Sum(const ImgFloat&  img, const Rect& rect)      { return iSum<float>(img, rect); }
int   Sum(const ImgInt&    img, const Rect& rect)      { return iSum<int>  (img, rect); }
int   Sum(const ImgBinary& img, const ImgBinary& mask) { return iSum<int>  (img, mask); }
int   Sum(const ImgGray&   img, const ImgBinary& mask) { return iSum<int>  (img, mask); }
float Sum(const ImgFloat&  img, const ImgBinary& mask) { return iSum<float>(img, mask); }
int   Sum(const ImgInt&    img, const ImgBinary& mask) { return iSum<int>  (img, mask); }
int   Sum(const ImgBinary& img)                        { return Sum(img, Rect(0, 0, img.Width(), img.Height())); }
int   Sum(const ImgGray&   img)                        { return Sum(img, Rect(0, 0, img.Width(), img.Height())); }
float Sum(const ImgFloat&  img)                        { return Sum(img, Rect(0, 0, img.Width(), img.Height())); }
int   Sum(const ImgInt&    img)                        { return Sum(img, Rect(0, 0, img.Width(), img.Height())); }

void  Sum(const ImgBgr& img, const Rect& rect, float* bsum, float* gsum, float* rsum)
{
  ImgBgr::ConstIterator p;
  int skip = img.Width() - (rect.right - rect.left);
  p = img.Begin(rect.left, rect.top);
  *bsum = *gsum = *rsum = 0;
  for (int y=rect.top ; y<rect.bottom ; y++)
  {
    for (int x=rect.left ; x<rect.right ; x++)
    {
      const Bgr& pix = *p;
      *bsum += pix.b;
      *gsum += pix.g;
      *rsum += pix.r;
      p++;
    }
    p += skip;
  }
}

void  Sum(const ImgBgr& img, const ImgBinary& mask, float* bsum, float* gsum, float* rsum)
{
  ImgBgr::ConstIterator p = img.Begin();
  ImgBinary::ConstIterator q = mask.Begin();
  *bsum = *gsum = *rsum = 0;
  for ( ; p != img.End() ; p++)  
  {
    if (*q++)  
    {
      const Bgr& pix = *p;
      *bsum += pix.b;
      *gsum += pix.g;
      *rsum += pix.r;
    }
  }
}

void  Sum(const ImgBgr& img, float* bsum, float* gsum, float* rsum)
{
  Sum(img, Rect(0, 0, img.Width(), img.Height()), bsum, gsum, rsum);
}


int SumSquared(const ImgGray& img, const Rect& rect)
{
  ImgGray::ConstIterator p;
  int skip = img.Width() - (rect.right - rect.left);
  int total = 0;
  p = img.Begin(rect.left, rect.top);
  for (int y=rect.top ; y<rect.bottom ; y++)
  {
    for (int x=rect.left ; x<rect.right ; x++)
    {
      int val = *p++;
      total += val * val;
    }
    p += skip;
  }
  return total;
}

double SumSquared(const ImgFloat& img, const Rect& rect)
{
  ImgFloat::ConstIterator p;
  int skip = img.Width() - (rect.right - rect.left);
  double total = 0;
  p = img.Begin(rect.left, rect.top);
  for (int y=rect.top ; y<rect.bottom ; y++)
  {
    for (int x=rect.left ; x<rect.right ; x++)
    {
      float val = *p++;
      total += val * val;
    }
    p += skip;
  }
  return total;
}

float Mean(const ImgGray& img, const ImgBinary& mask)
{
  int area = Sum(mask);
  assert(area > 0);
  if (area == 0)  BLEPO_ERROR("Cannot compute the mean of an empty set");
  return static_cast<float>(Sum(img, mask)) / area;
}

float Mean(const ImgGray& img, const Rect& rect)
{
  int area = (rect.right-rect.left) * (rect.bottom-rect.top);
  assert(area > 0);
  if (area == 0)  BLEPO_ERROR("Cannot compute the mean of an empty set");
  return static_cast<float>(Sum(img, rect)) / area;
}

float Mean(const ImgFloat& img, const ImgBinary& mask)
{
  int area = Sum(mask);
  assert(area > 0);
  if (area == 0)  BLEPO_ERROR("Cannot compute the mean of an empty set");
  return static_cast<float>(Sum(img, mask)) / area;
}

float Mean(const ImgFloat& img, const Rect& rect)
{
  int area = (rect.right-rect.left) * (rect.bottom-rect.top);
  assert(area > 0);
  if (area == 0)  BLEPO_ERROR("Cannot compute the mean of an empty set");
  return static_cast<float>(Sum(img, rect)) / area;
}

Bgr Mean(const ImgBgr& img, const Rect& rect)
{
  int area = (rect.right-rect.left) * (rect.bottom-rect.top);
  assert(area > 0);
  if (area == 0)  BLEPO_ERROR("Cannot compute the mean of an empty set");
  float bsum, gsum, rsum;
  Sum(img, rect, &bsum, &gsum, &rsum);
  return Bgr( static_cast<unsigned char>( blepo_ex::Clamp( bsum / area, 0.0f, 255.0f) ),
              static_cast<unsigned char>( blepo_ex::Clamp( gsum / area, 0.0f, 255.0f) ),
              static_cast<unsigned char>( blepo_ex::Clamp( rsum / area, 0.0f, 255.0f) )
    );
}

Bgr Mean(const ImgBgr& img, const ImgBinary& mask)
{
  int area = Sum(mask);
  assert(area > 0);
  if (area == 0)  BLEPO_ERROR("Cannot compute the mean of an empty set");
  float bsum, gsum, rsum;
  Sum(img, mask, &bsum, &gsum, &rsum);
  return Bgr( static_cast<unsigned char>( blepo_ex::Clamp( bsum / area, 0.0f, 255.0f) ),
              static_cast<unsigned char>( blepo_ex::Clamp( gsum / area, 0.0f, 255.0f) ),
              static_cast<unsigned char>( blepo_ex::Clamp( rsum / area, 0.0f, 255.0f) )
    );
}

float Variance(const ImgGray& img, const Rect& rect)
{
  ImgGray::ConstIterator p;
  int skip = img.Width() - (rect.right - rect.left);
  float total = 0;
  float mu = Mean(img, rect);
  float diff;
  p = img.Begin(rect.left, rect.top);
  for (int y=rect.top ; y<rect.bottom ; y++)
  {
    for (int x=rect.left ; x<rect.right ; x++)
    {
      diff = (*p++ - mu);
      total += diff * diff;
    }
    p += skip;
  }
  int area = (rect.right-rect.left) * (rect.bottom-rect.top);
  assert(area > 0);
  return total / area;
}

float Variance(const ImgGray& img, const ImgBinary& mask)
{
  ImgGray::ConstIterator p = img.Begin();
  ImgBinary::ConstIterator q = mask.Begin();
  float mu = Mean(img, mask);
  float total = 0;
  float diff;
  for (int y=0 ; y<img.Height() ; y++)
  {
    for (int x=0 ; x<img.Width() ; x++)
    {
      if (*q++)
      {
        diff = (*p - mu);
        total += diff * diff;
      }
      p++;
    }
  }
  int area = Sum(mask);
  if (area == 0)  BLEPO_ERROR("Cannot compute the variance of an empty set");
  return total / area;
}

double Variance(const ImgFloat& img, const Rect& rect)
{
  ImgFloat::ConstIterator p;
  int skip = img.Width() - (rect.right - rect.left);
  double total = 0;
  double mu = Mean(img, rect);
  double diff;
  p = img.Begin(rect.left, rect.top);
  for (int y=rect.top ; y<rect.bottom ; y++)
  {
    for (int x=rect.left ; x<rect.right ; x++)
    {
      diff = (*p++ - mu);
      total += diff * diff;
    }
    p += skip;
  }
  int area = (rect.right-rect.left) * (rect.bottom-rect.top);
  assert(area > 0);
  return total / area;

}

double Variance(const ImgFloat& img, const ImgBinary& mask)
{
  ImgFloat::ConstIterator p = img.Begin();
  ImgBinary::ConstIterator q = mask.Begin();
  float mu = Mean(img, mask);
  double total = 0;
  double diff;
  for (int y=0 ; y<img.Height() ; y++)
  {
    for (int x=0 ; x<img.Width() ; x++)
    {
      if (*q++)
      {
        diff = (*p - mu);
        total += diff * diff;
      }
      p++;
    }
  }
  int area = Sum(mask);
  if (area == 0)  BLEPO_ERROR("Cannot compute the variance of an empty set");
  return total / area;
}

float StandardDeviation(const ImgGray& img, const Rect& rect)
{
  return sqrt(Variance(img, rect));
}

float StandardDeviation(const ImgGray& img, const ImgBinary& mask)
{
  return sqrt(Variance(img, mask));
}

float StandardDeviation(const ImgFloat& img, const Rect& rect)
{
  return static_cast<float>( sqrt(Variance(img, rect)) );
}

float StandardDeviation(const ImgFloat& img, const ImgBinary& mask)
{
  return static_cast<float>( sqrt(Variance(img, mask)) );
}

void FlipVertical(const ImgBgr& img, ImgBgr* out)
{
  iFlipVertical(img, out);
}

void FlipVertical(const ImgFloat& img, ImgFloat* out)
{
  iFlipVertical(img, out);
}

void FlipVertical(const ImgGray& img, ImgGray* out)
{
  iFlipVertical(img, out);
}

void FlipVertical(const ImgInt& img, ImgInt* out)
{
  iFlipVertical(img, out);
}

void FlipHorizontal(const ImgBgr& img, ImgBgr* out)
{
  iFlipHorizontal(img, out);
}

void FlipHorizontal(const ImgFloat& img, ImgFloat* out)
{
  iFlipHorizontal(img, out);
}

void FlipHorizontal(const ImgGray& img, ImgGray* out)
{
  iFlipHorizontal(img, out);
}

void FlipHorizontal(const ImgInt& img, ImgInt* out)
{
  iFlipHorizontal(img, out);
}

void Transpose(const ImgFloat& img, ImgFloat* out) { iTranspose(img, out); }

/*
  Convolution with Hardcode Gaussian Kernel to improve the speed.
  @author Yi Zhou  yiz@clemson.edu
  May 2007
*/

// Convolution with Gauss Kernel 3x1 = 1/4{1, 2, 1}
template <typename T>
void iSmoothGaussHoriz3(const Image<T>& img, Image<T>* img_smoothed)
{
  InPlaceSwapper< Image<T> > inplace(img, &img_smoothed);

//--------------------------------------------------------------------------//
// DECLARATION
//--------------------------------------------------------------------------//
  int i, j;
  int margin;
  int image_width, image_height;
  Image<T>::ConstIterator p_in = img.Begin();
  Image<T>::Iterator p_out;
//--------------------------------------------------------------------------//
// INITIALIZATION
//--------------------------------------------------------------------------//
  image_width = img.Width();
  image_height = img.Height();

  (*img_smoothed).Reset(image_width, image_height);
  Set(img_smoothed, 0); 

  margin = 1;

  p_in += margin * image_width - margin;
  p_out = (*img_smoothed).Begin();
  p_out += margin * image_width - margin;
//--------------------------------------------------------------------------//
// FUNCTION CORE
//--------------------------------------------------------------------------//
  for(i = margin; i < image_height - margin; i++)
  {
    p_in += (margin * 2);
  	p_out += (margin * 2);
  
	for(j = margin; j < image_width - margin; j++)
	{
// (p_in[-1] + p_in[0] * 2 + p_in[1]) / 4;
	  *p_out = (p_in[-1] + 2 * p_in[0] + p_in[1]) / 4;
	  p_in++;
	  p_out++;
	}
  }
//--------------------------------------------------------------------------//
// END
//--------------------------------------------------------------------------//	
}

// Convolution with Gauss Kernel 5x1 = 1/16{1, 4, 6, 4, 1}
template <typename T>
void iSmoothGaussHoriz5(const Image<T>& img, Image<T>* img_smoothed)
{
  InPlaceSwapper< Image<T> > inplace(img, &img_smoothed);
//--------------------------------------------------------------------------//
// DECLARATION
//--------------------------------------------------------------------------//
  int i, j;
  int margin;
  int image_width, image_height;
  Image<T>::ConstIterator p_in = img.Begin();
  Image<T>::Iterator p_out;
//--------------------------------------------------------------------------//
// INITIALIZATION
//--------------------------------------------------------------------------//
  image_width = img.Width();
  image_height = img.Height();

  (*img_smoothed).Reset(image_width, image_height);
  Set(img_smoothed, 0); 

  margin = 2;

  p_in += margin * image_width - margin;
  p_out = (*img_smoothed).Begin();
  p_out += margin * image_width - margin;
//--------------------------------------------------------------------------//
// FUNCTION CORE
//--------------------------------------------------------------------------//
  for(i = margin; i < image_height - margin; i++)
  {
	p_in += (margin * 2);
    p_out += (margin * 2);
  
	for(j = margin; j < image_width - margin; j++)
	{
	  *p_out = (p_in[-2] + 4 * p_in[-1] + 6 * p_in[0] + 4 * p_in[1] + p_in[2]) / 16;
	  p_in++;
	  p_out++;
	}
  }
//--------------------------------------------------------------------------//
// END
//--------------------------------------------------------------------------// 	
}

// Convolution with Gauss Kernel 3x1 = 1/4{1, 2, 1}'
template <typename T>
void iSmoothGaussVert3(const Image<T>& img, Image<T>* img_smoothed)
{
  InPlaceSwapper< Image<T> > inplace(img, &img_smoothed);
//--------------------------------------------------------------------------//
// DECLARATION
//--------------------------------------------------------------------------//
  int i, j;
  int margin;
  int image_width, image_height;
  Image<T>::ConstIterator p_in = img.Begin();
  Image<T>::Iterator p_out;
//--------------------------------------------------------------------------//
// INITIALIZATION
//--------------------------------------------------------------------------//
  image_width = img.Width();
  image_height = img.Height();

  (*img_smoothed).Reset(image_width, image_height);
  Set(img_smoothed, 0); 

  margin = 1;

  p_in += margin * image_width - margin;
  p_out = (*img_smoothed).Begin();
  p_out += margin * image_width - margin;
//--------------------------------------------------------------------------//
// FUNCTION CORE
//--------------------------------------------------------------------------//
  for(i = margin; i < image_height - margin; i++)
  {
	p_in += (margin * 2);
    p_out += (margin * 2);
  
	for(j = margin; j < image_width - margin; j++)
	{
	  *p_out = (p_in[-image_width] + 2 * p_in[0] + p_in[image_width]) / 4;
	  p_in++;
	  p_out++;
	}
  }
//--------------------------------------------------------------------------//
// END
//--------------------------------------------------------------------------//
}


// Convolution with Gauss Kernel 5x1 = 1/16{1, 4, 6, 4, 1}'
template <typename T>
void iSmoothGaussVert5(const Image<T>& img, Image<T>* img_smoothed)
{
  InPlaceSwapper< Image<T> > inplace(img, &img_smoothed);
//--------------------------------------------------------------------------//
// DECLARATION
//--------------------------------------------------------------------------//
  int i, j;
  int margin;
  int image_width, image_height;
  Image<T>::ConstIterator p_in = img.Begin();
  Image<T>::Iterator p_out;
//--------------------------------------------------------------------------//
// INITIALIZATION
//--------------------------------------------------------------------------//
  image_width = img.Width();
  image_height = img.Height();

  (*img_smoothed).Reset(image_width, image_height);
  Set(img_smoothed, 0); 

  margin = 2;

  p_in += margin * image_width - margin;
  p_out = (*img_smoothed).Begin();
  p_out += margin * image_width - margin;
//--------------------------------------------------------------------------//
// FUNCTION CORE
//--------------------------------------------------------------------------//
  for(i = margin; i < image_height - margin; i++)
  {
	p_in += (margin * 2);
    p_out += (margin * 2);
  
	for(j = margin; j < image_width - margin; j++)
	{
// (p_in[-image_with * 2] + p_in[-image_width] * 4 + p_in[0] * 6 + p_in[image_width] * 4 + p_in[image_width * 2]) / 16;
	  *p_out = (p_in[-(image_width * 2)] + 4 * p_in[-image_width] + 6 * p_in[0] + 4 * p_in[image_width] + p_in[image_width * 2]) / 16;
	  p_in++;
	  p_out++;
	}
  }
//--------------------------------------------------------------------------//
// END
//--------------------------------------------------------------------------//	
}

// Convolution with Gauss Kernel 3x1 and 1x3
template <typename T>
void iSmoothGauss3x3(const Image<T>& img, Image<T>* img_smoothed)
{
  InPlaceSwapper< Image<T> > inplace(img, &img_smoothed);
//--------------------------------------------------------------------------//
// DECLARATION
//--------------------------------------------------------------------------//
  Image<T> img_temp;
//--------------------------------------------------------------------------//
// INITIALIZATION
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
// FUNCTION CORE
//--------------------------------------------------------------------------//
  SmoothGaussHoriz3(img, &img_temp);
  SmoothGaussVert3(img_temp, img_smoothed);
//--------------------------------------------------------------------------//
// END
//--------------------------------------------------------------------------// 	
}


// Convolution with Gauss Kernel 5x1 and 1x5
template <typename T>
void iSmoothGauss5x5(const Image<T>& img, Image<T>* img_smoothed)
{
  InPlaceSwapper< Image<T> > inplace(img, &img_smoothed);
//--------------------------------------------------------------------------//
// DECLARATION
//--------------------------------------------------------------------------//
  Image<T> img_temp;
//--------------------------------------------------------------------------//
// INITIALIZATION
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
// FUNCTION CORE
//--------------------------------------------------------------------------//
  SmoothGaussHoriz5(img, &img_temp);
  SmoothGaussVert5(img_temp, img_smoothed);
//--------------------------------------------------------------------------//
// END
//--------------------------------------------------------------------------// 
}

void SmoothGaussHoriz3(const ImgGray& img, ImgGray* img_smoothed)
{
  iSmoothGaussHoriz3(img, img_smoothed);
}

void SmoothGaussHoriz5(const ImgGray& img, ImgGray* img_smoothed)
{
  iSmoothGaussHoriz5(img, img_smoothed);
}

void SmoothGaussVert3(const ImgGray& img, ImgGray* img_smoothed)
{
  iSmoothGaussVert3(img, img_smoothed);
}

void SmoothGaussVert5(const ImgGray& img, ImgGray* img_smoothed)
{
  iSmoothGaussVert5(img, img_smoothed);
}

void SmoothGauss3x3(const ImgGray& img, ImgGray* img_smoothed)
{
  iSmoothGauss3x3(img, img_smoothed);
}

//void SmoothGauss5x5(const ImgGray& img, ImgGray* img_smoothed)
//{
//  iSmoothGauss5x5(img, img_smoothed);
//}

void SmoothGaussHoriz3(const ImgInt& img, ImgInt* img_smoothed)
{
  iSmoothGaussHoriz3(img, img_smoothed);
}

void SmoothGaussHoriz5(const ImgInt& img, ImgInt* img_smoothed)
{
  iSmoothGaussHoriz5(img, img_smoothed);
}

void SmoothGaussVert3(const ImgInt& img, ImgInt* img_smoothed)
{
  iSmoothGaussVert3(img, img_smoothed);
}

void SmoothGaussVert5(const ImgInt& img, ImgInt* img_smoothed)
{
  iSmoothGaussVert5(img, img_smoothed);
}

void SmoothGauss3x3(const ImgInt& img, ImgInt* img_smoothed)
{
  iSmoothGauss3x3(img, img_smoothed);
}

void SmoothGauss5x5(const ImgInt& img, ImgInt* img_smoothed)
{
  iSmoothGauss5x5(img, img_smoothed);
}

void SmoothGaussHoriz3(const ImgFloat& img, ImgFloat* img_smoothed)
{
  iSmoothGaussHoriz3(img, img_smoothed);
}

void SmoothGaussHoriz5(const ImgFloat& img, ImgFloat* img_smoothed)
{
  iSmoothGaussHoriz5(img, img_smoothed);
}

void SmoothGaussVert3(const ImgFloat& img, ImgFloat* img_smoothed)
{
  iSmoothGaussVert3(img, img_smoothed);
}

void SmoothGaussVert5(const ImgFloat& img, ImgFloat* img_smoothed)
{
  iSmoothGaussVert5(img, img_smoothed);
}

void SmoothGauss3x3(const ImgFloat& img, ImgFloat* img_smoothed)
{
  iSmoothGauss3x3(img, img_smoothed);
}

void SmoothGauss5x5(const ImgFloat& img, ImgFloat* img_smoothed)
{
  iSmoothGauss5x5(img, img_smoothed);
}

void SmoothGauss5x1WithBorders(const ImgFloat& img, ImgFloat* out)
{
  assert( out != &img );
  assert( img.Width() >= 5 );

  out->Reset( img.Width(), img.Height() );
  int x, y;
  const int wm4 = img.Width() - 4;
  const float* p = img.Begin();
  float* q = out->Begin();
  for (y=0 ; y<img.Height() ; y++)
  {
    assert(p == img.Begin(0,y));
    assert(q == out->Begin(0,y));

    // first column  [1 1]
    *q = 0.5f * (p[0] + p[1]);   
    p++;  q++;

    // second column  [1 2 1]
    *q = 0.25f * (p[-1] + p[1]) + 0.5f * p[0];  
    p++;  q++;

    // middle columns  [1 4 6 4 1]
    for (x=0 ; x<wm4 ; x++)
    {
      *q = 0.0625f * (p[-2] + p[2]) + 0.25f * (p[-1] + p[1]) + 0.3750f * p[0];
      p++;  q++;
    }

    // penultimate column [1 2 1]
    *q = 0.25f * (p[-1] + p[1]) + 0.5f * p[0];  
    p++;  q++;

    // last column [1 1]
    *q = 0.5f * (p[-1] + p[0]);  
    p++;  q++;
  }
}

void SmoothGauss1x5WithBorders(const ImgFloat& img, ImgFloat* out)
{
  assert( out != &img );
  assert( img.Height() >= 5 );

  out->Reset( img.Width(), img.Height() );
  const int w = img.Width();
  const int wt2 = w*2;
  const int hm4 = img.Height() - 4;
  int x, y;
  const float* p = img.Begin();
  float* q = out->Begin();

  // first row [1 1]
  assert(p == img.Begin(0,0));
  assert(q == out->Begin(0,0));
  for (x=0 ; x<w ; x++)
  {
    *q = 0.5f * (p[0] + p[w]);
    p++;  q++;
  }

  // second row [1 2 1]
  assert(p == img.Begin(0,1));
  assert(q == out->Begin(0,1));
  for (x=0 ; x<w ; x++)
  {
    *q = 0.25f * (p[-w] + p[w]) + 0.5f * p[0];  
    p++;  q++;
  }

  // middle rows [1 4 6 4 1]
  for (y=0 ; y<hm4 ; y++)
  {
    assert(p == img.Begin(0,y+2));
    assert(q == out->Begin(0,y+2));
    for (x=0 ; x<w ; x++)
    {
      *q = 0.0625f * (p[-wt2] + p[wt2]) + 0.25f * (p[-w] + p[w]) + 0.3750f * p[0];
      p++;  q++;
    }
  }

  // penultimate row [1 2 1]
  assert(p == img.Begin(0,hm4+2));
  assert(q == out->Begin(0,hm4+2));
  for (x=0 ; x<w ; x++)
  {
    *q = 0.25f * (p[-w] + p[w]) + 0.5f * p[0];
    p++;  q++;
  }

  // last row [1 1]
  assert(p == img.Begin(0,hm4+3));
  assert(q == out->Begin(0,hm4+3));
  for (x=0 ; x<w ; x++)
  {
    *q = 0.5f * (p[-w] + p[0]);  
    p++;  q++;
  }
}

void SmoothGauss3x1WithBorders(const ImgFloat& img, ImgFloat* out)
{
  assert(out != &img);

  out->Reset( img.Width(), img.Height() );
  int x, y;
  const int wm2 = img.Width() - 2;
  const float* p = img.Begin();
  float* q = out->Begin();
  for (y=0 ; y<img.Height() ; y++)
  {
    assert(p == img.Begin(0,y));
    assert(q == out->Begin(0,y));

    // first column  [1 1]
    *q = 0.5f * (p[0] + p[1]);  
    p++;  q++;

    // middle columns  [1 2 1]
    for (x=0 ; x<wm2 ; x++)
    {
      *q = 0.25f * (p[-1] + p[1]) + 0.5f * p[0];
      p++;  q++;
    }

    // last column [1 1]
    *q = 0.5f * (p[-1] + p[0]);  
    p++;  q++;
  }
}

void SmoothGauss1x3WithBorders(const ImgFloat& img, ImgFloat* out)
{
  assert(out != &img);

  out->Reset( img.Width(), img.Height() );
  const int w = img.Width();
  const int hm2 = img.Height() - 2;
  int x, y;
  const float* p = img.Begin();
  float* q = out->Begin();

  // first row [1 1]
  assert(p == img.Begin(0,0));
  assert(q == out->Begin(0,0));
  for (x=0 ; x<w ; x++)
  {
    *q = 0.5f * (p[0] + p[w]);  
    p++;  q++;
  }

  // middle rows [1 2 1]
  for (y=0 ; y<hm2 ; y++)
  {
    assert(p == img.Begin(0,y+1));
    assert(q == out->Begin(0,y+1));
    for (x=0 ; x<w ; x++)
    {
      *q = 0.25f * (p[-w] + p[w]) + 0.5f * p[0];  
      p++;  q++;
    }
  }

  // last row [1 1]
  assert(p == img.Begin(0,hm2+1));
  assert(q == out->Begin(0,hm2+1));
  for (x=0 ; x<w ; x++)
  {
    *q = 0.5f * (p[-w] + p[0]);  
    p++;  q++;
  }
}

// convolve with a 5x5 box filter of all ones 
// (Normalizes by 16 -- for efficiency, because it's a power of two)
// 'inplace' okay and efficient
void ConvolveBox5x5(const ImgGray& img, ImgGray* out)
{
  assert(out == &img);  // should not be necessary, but need to check on this b/c I was having problems!
  const int ww = 5;              // box width and height
  const int border = (ww-1)/2;   // values are invalid in this region
  int normalization = 2;         // power of two ( 2^2 = 4 ) normalization in each direction; should be ww ideally
  int w = img.Width();
  int h = img.Height();
  int x, y;
  int val;

  // convolve with horizontal [1 1 1 1 1] kernel
  const unsigned char* pi = img.Begin();
  const unsigned char* ps = img.Begin();
  unsigned char* po = out->Begin();
  unsigned char* pe = out->Begin( w - 2*border );
  int summ[2] = {0, 0};
  int sum;
  for (y = 0 ; y < h ; y++)
  {
    sum = 0;
    sum += *pi++;
    sum += *pi++;
    sum += *pi++;
    sum += *pi++;
    while (po != pe)
    {
      sum += *pi++;
      val = *ps++;
      *po++ = summ[0];
      summ[0] = summ[1];
      summ[1] = blepo_ex::Clamp(sum >> normalization, 0, 255);
      sum -= val;
    }

    *po++ = summ[0];
    *po++ = summ[1];
    ps += 2*border;
    po += border;
    pe += w;
  }

  // convolve with vertical [1 1 1 1 1]^T kernel
  pe = out->Begin(0, h - 2*border );
  for (x = 0 ; x < w ; x++)
  {
    pi = img.Begin(x, 0);
    ps = img.Begin(x, 0);
    po = out->Begin(x, 0);

    sum = 0;
    sum += *pi;  pi += w;
    sum += *pi;  pi += w;
    sum += *pi;  pi += w;
    sum += *pi;  pi += w;
    while (po != pe)
    {
      sum += *pi;     pi += w;
      val = *ps;      ps += w;
      *po = summ[0];  po += w;
      summ[0] = summ[1];
      summ[1] = blepo_ex::Clamp(sum >> normalization, 0, 255);
      sum -= val;
    }

    *po = summ[0];  po += w;
    *po = summ[1];
    pe++;
  }
}

// convolve with a NxN box filter of all ones 
// (Normalizes by NxN)
void ConvolveBoxNxN(const ImgGray& img, ImgGray* out, int winsize)
{
  assert(out == &img);  // should not be necessary, but need to check on this b/c I was having problems!
  const int ww = winsize;        // box width and height
  const int border = (ww-1)/2;   // values are invalid in this region
  int w = img.Width();
  int h = img.Height();
  int x, y;
  int val;
  ImgGray temp;
  temp.Reset(img.Width(),img.Height());

  // convolve with horizontal [1 1 1 1 1] kernel
  const unsigned char* pi = img.Begin();
  const unsigned char* ps = img.Begin();
  unsigned char* po = temp.Begin();
  unsigned char* pe = temp.Begin( w - 2*border );
  int sum;
  for (y = 0 ; y < h ; y++)
  {
    sum = 0;
    for(x=0; x<(ww-1); x++)  sum += *pi++;
    for(x=0; x<border; x++)  *po++ = 0;
    while (po != pe)
    {
      sum += *pi++;
      val = *ps++;
      *po++ = sum/ww;
      sum -= val;
    }
    for(x=0; x<border; x++)  *po++ = 0;
    ps += 2*border;
    pe += w;
  }
  
  // convolve with vertical [1 1 1 1 1]^T kernel
  pe = out->Begin(0, h - 2*border );
  for (x = 0 ; x < w ; x++)
  {
    pi = temp.Begin(x, 0);
    ps = temp.Begin(x, 0);
    po = out->Begin(x, 0);
    
    sum = 0;
    for(y=0; y<(ww-1); y++)
    {	
      sum += *pi; pi += w;
    }
    for(y=0; y<border; y++)
    {	
      *po = 0; po += w;
    }
    while (po != pe)
    {
      sum += *pi;     pi += w;
      val = *ps;      ps += w;
      *po = sum/ww;  po += w;
      sum -= val;
    }
    for(y=0; y<border; y++)
    {	
      *po = 0; po += w;
    }
    pe++;
  }
}

//void SmoothBoxHoriz3(const ImgFloat& img, ImgFloat* out)
//{
//  assert(out != &img);
//  out->Reset(img.Width(), img.Height());
//  const float* p = img.Begin(1, 0);
//  float* q = out->Begin();
//  const float* end = img.Begin( img.Width()-1, img.Height()-1 );
//  *q++ = 0;
//  while (p != end)
//  {
//    *q = 0.333333 * (p[-1] + p[0] + p[1]);
//    p++;
//    q++;
//  }
//  *q++ = 0;
//}
//
//void SmoothBoxVert3(const ImgFloat& img, ImgFloat* out)
//{
//  assert(out != &img);
//  const int w = img.Width();
//  out->Reset(img.Width(), img.Height());
//  Set(out, 0);
//  const float* p = img.Begin(0, 1);
//  float* q = out->Begin(0, 1);
//  const float* end = img.Begin( img.Width()-1, img.Height()-2 );
//  while (p < end)
//  {
//    *q = 0.333333 * (p[-w] + p[0] + p[w]);
//    p++;
//    q++;
//  }
//}
//
//void SmoothBox3x3(const ImgFloat& img, ImgFloat* out)
//{
//  ImgFloat tmp;
//  SmoothBoxHoriz3(img, &tmp);
//  SmoothBoxVert3(tmp, out);
//}

void SmoothBox3x1WithBorders(const ImgFloat& img, ImgFloat* out)
{
  assert(out != &img);
  out->Reset(img.Width(), img.Height());
  const float* p = img.Begin();
  float* q = out->Begin();
  const int wm2 = img.Width() - 2;
  int x, y;

  for (y=0 ; y<img.Height() ; y++)
  {
    assert(p == img.Begin(0,y));
    assert(q == out->Begin(0,y));

    // first column [1 1]
    *q = 0.5f * (p[0] + p[1]);
    p++;  q++;

    // middle columns [1 1 1]
    for (x=0 ; x<wm2 ; x++)
    {
      *q = 0.333333f * (p[-1] + p[0] + p[1]);
      p++;  q++;
    }

    // last column [1 1]
    *q = 0.5f * (p[-1] + p[0]);
    p++;  q++;
  }
}

void SmoothBox1x3WithBorders(const ImgFloat& img, ImgFloat* out)
{
  assert(out != &img);
  out->Reset(img.Width(), img.Height());
  const float* p = img.Begin();
  float* q = out->Begin();
  const int hm2 = img.Height() - 2;
  const int w = img.Width();
  int x, y;

  // first row [1 1]
  assert(p == img.Begin());
  assert(q == out->Begin());
  for (x=0 ; x<img.Width() ; x++)
  {
    *q = 0.5f * (p[0] + p[w]);
    p++;  q++;
  }

  // middle rows [1 1 1]
  for (y=0 ; y<hm2 ; y++)
  {
    assert(p == img.Begin(0,y+1));
    assert(q == out->Begin(0,y+1));
    for (x=0 ; x<img.Width() ; x++)
    {
      *q = 0.333333f * (p[-w] + p[0] + p[w]);
      p++;  q++;
    }
  }

  // last row [1 1]
  assert(p == img.Begin(0,hm2+1));
  assert(q == out->Begin(0,hm2+1));
  for (x=0 ; x<img.Width() ; x++)
  {
    *q = 0.5f * (p[-w] + p[0]);
    p++;  q++;
  }
}

void SmoothBox3x3WithBorders(const ImgFloat& img, ImgFloat* out, ImgFloat* work)
{
  ImgFloat tmp;
  if (!work)  work = &tmp;
  SmoothBox3x1WithBorders(img, &tmp);
  SmoothBox1x3WithBorders(tmp, out);
}

void SumBox3x1WithBorders(const ImgFloat& img, ImgFloat* out)
{
  assert(out != &img);
  out->Reset(img.Width(), img.Height());
  const float* p = img.Begin();
  float* q = out->Begin();
  const int wm2 = img.Width() - 2;
  int x, y;

  for (y=0 ; y<img.Height() ; y++)
  {
    assert(p == img.Begin(0,y));
    assert(q == out->Begin(0,y));

    // first column [1 1]
    *q = p[0] + p[0] + p[1];
    p++;  q++;

    // middle columns [1 1 1]
    for (x=0 ; x<wm2 ; x++)
    {
      *q = p[-1] + p[0] + p[1];
      p++;  q++;
    }

    // last column [1 1]
    *q = p[-1] + p[0] + p[0];
    p++;  q++;
  }
}

void SumBox1x3WithBorders(const ImgFloat& img, ImgFloat* out)
{
  assert(out != &img);
  out->Reset(img.Width(), img.Height());
  const float* p = img.Begin();
  float* q = out->Begin();
  const int hm2 = img.Height() - 2;
  const int w = img.Width();
  int x, y;

  // first row [2 1]
  assert(p == img.Begin());
  assert(q == out->Begin());
  for (x=0 ; x<img.Width() ; x++)
  {
    *q = p[0] + p[0] + p[w];
    p++;  q++;
  }

  // middle rows [1 1 1]
  for (y=0 ; y<hm2 ; y++)
  {
    assert(p == img.Begin(0,y+1));
    assert(q == out->Begin(0,y+1));
    for (x=0 ; x<img.Width() ; x++)
    {
      *q = p[-w] + p[0] + p[w];
      p++;  q++;
    }
  }

  // last row [1 2]
  assert(p == img.Begin(0,hm2+1));
  assert(q == out->Begin(0,hm2+1));
  for (x=0 ; x<img.Width() ; x++)
  {
    *q = p[-w] + p[0] + p[0];
    p++;  q++;
  }
}

void SumBox3x3WithBorders(const ImgFloat& img, ImgFloat* out, ImgFloat* work)
{
  ImgFloat tmp;
  if (!work)  work = &tmp;
  SumBox3x1WithBorders(img, &tmp);
  SumBox1x3WithBorders(tmp, out);
}


//**************************************************************************************//

void SmoothGauss5x5(const ImgGray& img, ImgGray* out)
{
  const int border = 2;  // values are invalid in this region
  int w = img.Width();
  int h = img.Height();
  int y;

  // This extra image reduces efficiency.  If speed is important, we may
  // want to reimplement this function.
  ImgGray tmp(w, h);

  out->Reset(w, h);

  // This is an easy way to set all the border pixels equal to the original
  // image, which is necessary b/c the vertical convolution uses these
  // values; if speed is important we could rewrite this line to copy only
  // the border pixels.
  *out = img;

  // convolve with horizontal [1 4 6 4 1] kernel
  for (y=border ; y<h-border ; y++)
  {
    ImgGray::ConstIterator pi = img.Begin(border, y);
    ImgGray::Iterator po = tmp.Begin(border, y);
    ImgGray::Iterator pe = tmp.Begin(w-border, y);
    while (po != pe)
    {
      *po++ = (pi[-2] + 4 * pi[-1] + 6 * pi[0] + 4 * pi[1] + pi[2] ) / 16;
      pi++;
    }
  }

  // convolve with vertical [1 4 6 4 1]^T kernel,
  // reusing values from previous convolution
  for (y=border ; y<h-border ; y++)
  {
    ImgGray::ConstIterator pi = tmp.Begin(border, y);
    ImgGray::Iterator po = out->Begin(border, y);
    ImgGray::Iterator pe = out->Begin(w-border, y);
    while (po != pe)
    {
      *po++ = (pi[-2*w] + 4 * pi[-w] + 6 * pi[0] + 4 * pi[w] + pi[2*w] ) / 16;
      pi++;
    }
  }
}

void FindPixels(const ImgBinary& img, ImgBinary::Pixel value, std::vector<Point>* loc) { iFindPixels(img, value, loc); }
void FindPixels(const ImgInt&    img, ImgInt   ::Pixel value, std::vector<Point>* loc) { iFindPixels(img, value, loc); }
void FindPixels(const ImgFloat&  img, ImgFloat ::Pixel value, std::vector<Point>* loc) { iFindPixels(img, value, loc); }
void FindPixels(const ImgGray&   img, ImgGray  ::Pixel value, std::vector<Point>* loc) { iFindPixels(img, value, loc); }

void Smooth(
  const ImgFloat& img, 
  float sigma, 
  ImgFloat* img_smoothed)
{
  ImgFloat gauss_x, gauss_y;
  Gauss(sigma, &gauss_x, &gauss_y);
  
  ImgFloat tmp;
  Convolve(img, gauss_x, &tmp);
  Convolve(tmp, gauss_y, img_smoothed);
}

//void Smooth(
//  const ImgFloat& img, 
//  float sigma, 
//  ImgFloat* img_smoothed, 
//  float tail_cutoff)
//{
//  ImgFloat gauss_x, gauss_y;
//  Gauss(sigma, &gauss_x, &gauss_y);
//  
//  ImgFloat tmp;
//  Convolve(img, gauss_x, &tmp);
//  Convolve(tmp, gauss_y, img_smoothed);
//}

// returns length of kernel to capture approximately 
//   +/- 2.5 sigma of the Gaussian (98.76%)
// guaranteed to return an odd number
// common numbers:
//    sigma   |  length
//  ----------+----------
//      0.6   |     3
//      1.0   |     5
//      1.4   |     7
//      1.8   |     9
int GetKernelLength(float sigma)
{
  // width = 2 * hw + 1 = 5 * sigma
  int hw = blepo_ex::Round(2.5f * sigma - 0.5f);
  if (hw < 1)  hw = 1;
  return 2 * hw + 1;
}

void GaussVert(
  float sigma, 
  ImgFloat* out)
{
  assert(sigma>0); // sigma must be positive

  int kernel_length = GetKernelLength(sigma);
  out->Reset(1,kernel_length);
  
  int hw = kernel_length/2; // kernel half width
  ImgFloat::Iterator p;
  p = out->Begin();
  float kernel_sum = 0.0f;
  for(int i=-hw; i<=hw; i++)
  {
    *p = (float) exp((double)(-i*i/(2*sigma*sigma)));
    kernel_sum += (*p);
    p++;
  }

  // normalizing
  for (p = out->Begin(); p != out->End(); p++)
  {
    *p /= kernel_sum;
  }
}

//void GaussVert(
//  float sigma, 
//  ImgFloat* out, 
//  float tail_cutoff)
//{
//  assert(sigma>0.05f); // minimum value of sigma
//  assert(tail_cutoff>0.0f && tail_cutoff<=1.0f); // range of tail_cutoff 
//
//  float max_gauss = 1.0f; // maximum value of kernel function
//  float gauss_val;
//  int hw = 0; // kernel half width
//  while(1)
//  {
//    gauss_val = (float) exp((double)(-hw*hw/(2*sigma*sigma)));
//    if((gauss_val/max_gauss) < tail_cutoff)
//    {
//      hw--;
//      break;
//    }
//    else
//      hw++;
//  }
//
//  int kernel_length = 2*hw+1;
//  GaussVert(sigma,kernel_length,out);
//}

void GaussHoriz(
  float sigma, 
  ImgFloat* out)
{

  ImgFloat tmp;
  GaussVert(sigma, &tmp);
  const int n = tmp.Height();
  out->Reset(n, 1);
  memcpy( out->Begin(), tmp.Begin(), n * sizeof( ImgFloat::Pixel ) );
//  ImgFloat::Iterator d = out->Begin();
//  ImgFloat::ConstIterator s;
//  for(s=out_tmp.Begin(); s!=out_tmp.End(); s++)
//  {
//    *d = *s;
//    d++;
//  }
}

//void GaussHoriz(
//  float sigma, 
//  ImgFloat* out, 
//  float tail_cutoff)
//{
//  ImgFloat out_tmp;
//  GaussVert(sigma,&out_tmp,tail_cutoff);
//
//  out->Reset(out_tmp.Height(),1);
//  ImgFloat::Iterator d = out->Begin();
//  ImgFloat::ConstIterator s;
//  for(s=out_tmp.Begin(); s!=out_tmp.End(); s++)
//  {
//    *d = *s;
//    d++;
//  }
//}

void Gauss(
  float sigma, 
  ImgFloat* outx, ImgFloat* outy)
{
  GaussHoriz(sigma, outx);
  GaussVert(sigma, outy);
}

//void Gauss(
//  float sigma, 
//  ImgFloat* outx, ImgFloat* outy, 
//  float tail_cutoff)
//{
//  GaussHoriz(sigma,outx,tail_cutoff);
//  GaussVert(sigma,outy,tail_cutoff);
//}

void GaussDerivVert(
  float sigma, 
  ImgFloat* out)
{
  assert(sigma>0); // sigma must be positive

  int kernel_length = GetKernelLength(sigma);
  out->Reset(1, kernel_length);
  
  int hw = kernel_length/2; // kernel half width
  ImgFloat::Iterator p;
  p = out->Begin();
  float denom = 0.0f;
  float gauss_deriv_val;
  for(int i=-hw; i<=hw; i++)
  {
    gauss_deriv_val = (float) (i*exp((double)(-i*i/(2*sigma*sigma))));
    denom += i * gauss_deriv_val;
    *p = gauss_deriv_val;
    p++;
  }

  // normalizing and sign adjusting
  for(p = out->Begin(); p != out->End(); p++)
    *p = -(*p) / denom;
}

//void GaussDerivVert(
//  float sigma, 
//  ImgFloat* out, 
//  float tail_cutoff)
//{
//  assert(sigma>0.05f); // minimum value of sigma
//
//  float max_gauss_deriv = (float) (sigma*exp((double)(-0.5f)));
//  assert(tail_cutoff>0.0f && tail_cutoff<=max_gauss_deriv); // range of tail_cutoff
//
//  float gauss_deriv_val;
//  int hw = (int) ceil((double)sigma); // kernel half width
//  while(1)
//  {
//    gauss_deriv_val = (float) (hw*exp((double)(-hw*hw/(2*sigma*sigma))));
//    if((gauss_deriv_val/max_gauss_deriv) < tail_cutoff)
//    {
//      hw--;
//      break;
//    }
//    else
//      hw++;
//  }
//
//  assert(hw>0); // kernel_length must be atleast 3
//
//  int kernel_length = 2*hw+1;
//  GaussDerivVert(sigma,kernel_length,out);
//}

void GaussDerivHoriz(
  float sigma, 
  ImgFloat* out)
{
  ImgFloat tmp;
  GaussDerivVert(sigma, &tmp);

  const int n = tmp.Height();
  out->Reset(n, 1);
  memcpy( out->Begin(), tmp.Begin(), n * sizeof( ImgFloat::Pixel ) );
//  ImgFloat::Iterator d = out->Begin();
//  ImgFloat::ConstIterator s;
//  for(s=out_tmp.Begin(); s!=out_tmp.End(); s++)
//  {
//    *d = *s;
//    d++;
//  }
}

//void GaussDerivHoriz(
//  float sigma, 
//  ImgFloat* out, 
//  float tail_cutoff)
//{
//  ImgFloat out_tmp;
//  GaussDerivVert(sigma,&out_tmp,tail_cutoff);
//
//  out->Reset(out_tmp.Height(),1);
//  ImgFloat::Iterator d = out->Begin();
//  ImgFloat::ConstIterator s;
//  for(s=out_tmp.Begin(); s!=out_tmp.End(); s++)
//  {
//    *d = *s;
//    d++;
//  }
//}

void GaussDeriv(
  float sigma, 
  ImgFloat* outx, ImgFloat* outy)
{
  GaussDerivHoriz(sigma, outx);
  GaussDerivVert(sigma, outy);
}

//void GaussDeriv(
//  float sigma, 
//  ImgFloat* outx, ImgFloat* outy, 
//  float tail_cutoff)
//{
//  GaussDerivHoriz(sigma,outx,tail_cutoff);
//  GaussDerivVert(sigma,outy,tail_cutoff);
//}

void Gradient(
  const ImgFloat& img,
  float sigma,
  ImgFloat* gradx, ImgFloat* grady)
{
  ImgFloat gauss_x, gauss_y;
  Gauss(sigma, &gauss_x, &gauss_y);

  ImgFloat gauss_deriv_x, gauss_deriv_y;
  GaussDeriv(sigma, &gauss_deriv_x, &gauss_deriv_y);

  ImgFloat tmp;
  Convolve(img, gauss_deriv_x, &tmp);
  Convolve(tmp, gauss_y, gradx);
  Convolve(img, gauss_deriv_y, &tmp);
  Convolve(tmp, gauss_x, grady);
}

//void Gradient(
//  const ImgFloat& img,
//  float sigma,
//  ImgFloat* gradx, ImgFloat* grady,
//  float tail_cutoff)
//{
//  ImgFloat gauss_x, gauss_y;
//  Gauss(sigma,&gauss_x,&gauss_y,tail_cutoff);
//
//  ImgFloat gauss_deriv_x, gauss_deriv_y;
//  GaussDeriv(sigma,&gauss_deriv_x,&gauss_deriv_y,tail_cutoff);
//
//  ImgFloat tmp;
//  Convolve(img, gauss_deriv_x, &tmp);
//  Convolve(tmp, gauss_y, gradx);
//  Convolve(img, gauss_deriv_y, &tmp);
//  Convolve(tmp, gauss_x, grady);
//}

void GradMag(
  const ImgFloat& img, 
  float sigma, 
  ImgFloat* gradmag,
  ImgFloat* phase)
{
  assert(gradmag != NULL);
  // could be written more efficiently
  ImgFloat gx, gy, tmp_phase;
  if (phase == NULL)  phase = &tmp_phase;
  Gradient(img, sigma, &gx, &gy);
  RectToMagPhase(gx, gy, gradmag, phase);
}

// convolves with [1 0 0 0 -1] and its transpose.
// (You probably want to smooth the image before calling this function.)
// (no normalization, so result will be four times gradient)
// used by FastSmoothAndGradientApprox.
void FastGradientSkip1Times4
(
  const ImgFloat& smoothed,
  ImgFloat* gradx,
  ImgFloat* grady
)
{
  int x, y;
  const int w = smoothed.Width();
  const int h = smoothed.Height();
  const int w2 = w * 2;
  const int wm2 = smoothed.Width() - 2;
  const int hm2 = smoothed.Height() - 2;

  gradx->Reset( w, h );
  grady->Reset( w, h );

  // compute gradx
  const float* p = smoothed.Begin();
  float* q = gradx->Begin();
  for (y=0 ; y<h ; y++)
  {
    // first column
    *q = p[1] - p[0];  p++;  q++;

    // second column
    *q = p[1] - p[-1];  p++;  q++;

    // middle columns
    for (x=2 ; x<wm2 ; x++)
    {
      *q = p[2] - p[-2];  p++;  q++;
    }

    // penultimate column
    *q = p[1] - p[-1];  p++;  q++;

    // last column
    *q = p[0] - p[-1];  p++;  q++;
  }

  // compute grady
  p = smoothed.Begin();
  q = grady->Begin();

  // first row
  for (x=0 ; x<w ; x++)
  {
    *q = p[w] - p[0];  p++;  q++;
  }

  // second row
  for (x=0 ; x<w ; x++)
  {
    *q = p[w] - p[-w];  p++;  q++;
  }

  // middle rows
  for (y=2 ; y<hm2 ; y++)
  {
    for (x=0 ; x<w ; x++)
    {
      *q = p[w2] - p[-w2];  p++;  q++;
    }
  }

  // penultimate row
  for (x=0 ; x<w ; x++)
  {
    *q = p[w] - p[-w];  p++;  q++;
  }

  // last row
  for (x=0 ; x<w ; x++)
  {
    *q = p[0] - p[-w];  p++;  q++;
  }
}

// could easily fix this to compute actual gradient (i.e., not approx) without any additional computation:
//    allow kernel values to be passed into smoothgauss
//    multiply those values by weighting to overcome the fact that the fastgradient
//    is not weighting the pixels
void FastSmoothAndGradientApprox
(
  const ImgFloat& img, 
  ImgFloat* smoothed,
  ImgFloat* gradx,
  ImgFloat* grady,
  ImgFloat* tmp
)
{
  // convolve with Gaussian
  SmoothGauss5x1WithBorders(img, tmp);
  SmoothGauss1x5WithBorders(*tmp, smoothed);

  // compute gradient in x
  FastGradientSkip1Times4(*smoothed, gradx, grady);
}

void SmoothAndGradient
(
  const ImgFloat& img, 
  float sigma,
  ImgFloat* smoothed,
  ImgFloat* gradx,
  ImgFloat* grady,
  ImgFloat* tmp
)
{
  ImgFloat gauss_x, gauss_y;
  ImgFloat deriv_x, deriv_y;
  Gauss(sigma, &gauss_x, &gauss_y);
  GaussDeriv(sigma, &deriv_x, &deriv_y);

  Convolve(img , gauss_y, tmp);
  Convolve(*tmp, deriv_x, gradx);
  Convolve(img , gauss_x, tmp);
  Convolve(*tmp, deriv_y, grady);
  Convolve(*tmp, gauss_y, smoothed);
}

// Convolves image with 3x3 Gaussian directional derivatives,
//   using kernel [k1 k0 k1] * [-1 0 1]^T
// Uses separability and symmetry for speed.
// If 'smooth' is not NULL, then also computes the smoothed image
//   by convolving with 3x3 Gaussian.
// Bug:
//    Supposed to be fast, but is actually slower than FastSmoothAndGradientApprox.
//    Could be because pointers are not used, or could be because checking for 'smooth'
//    version in the middle of the loop.
void Gradient3x3
(
  const ImgFloat& img, 
  float k0,             // center weight in Gaussian kernel (times 0.5 to include the Gauss deriv normalization)
  float k1,             // off-center weight in Gaussian kernel (times 0.5 to include the Gauss deriv normalization)
  ImgFloat* gradx,
  ImgFloat* grady,
  ImgFloat* smooth
)
{
  assert(k0 >= k1);

  const int w = img.Width();
  const int h = img.Height();
  const int wm1 = w - 1;
  const int hm1 = h - 1;
  int x, y;

//  ImgFloat tmp(w,2);   // temporary data structure
  
  gradx->Reset(w, h);
  grady->Reset(w, h);
  if (smooth)  smooth->Reset(w, h);

  const float* p = img.Begin(0, 1);
  const float* pu = p - w;
  const float* pd = p + w;

  float* qx = gradx->Begin(0, 1);
  float* qy = grady->Begin(0, 1);
  float* qs = smooth ? smooth->Begin(0, 1) : 0;

  float *t0 = gradx->Begin();  // temporary data structures; use top rows of output, since
  float *t1 = grady->Begin();  //   they are not filled in until the very end
//  float *t0 = tmp.Begin();
//  float *t1 = tmp.Begin() + w;

  // compute middle rows of image
  for (y=1 ; y<hm1 ; y++, p+=w, pu+=w, pd+=w, qx+=w, qy+=w)
  {
    assert(p  == img.Begin(0,y) );
    assert(pu == img.Begin(0,y-1) );
    assert(pd == img.Begin(0,y+1) );
    assert(qx == gradx->Begin(0,y) );
    assert(qy == grady->Begin(0,y) );

    // convolve vertical
    for (x=0 ; x<w ; x++)
    {
      t0[x] = pd[x] - pu[x];                          // differentiate
      t1[x] = k1 * (pu[x] + pd[x]) + k0 * p[x];       // smooth
    }

    if (qs)
    {
      const float s0 = 4 * k0;  // need to remove Gaussian derivative normalization from smoothing
      const float s1 = 4 * k1;

      x = 0;
      qx[x] = t1[x+1] - t1[x];                      // first column
      qy[x] = k1 * (t0[x] + t0[x+1]) + k0 * t0[x];
      qs[x] = k1 * (t1[x] + t1[x+1]) + k0 * t1[x];
      // convolve horizontal
      for (x=1 ; x<wm1 ; x++)
      {
        qx[x] = t1[x+1] - t1[x-1];                      // differentiate
        qy[x] = k1 * (t0[x-1] + t0[x+1]) + k0 * t0[x];  // smooth
        qs[x] = s1 * (t1[x-1] + t1[x+1]) + s0 * t1[x];  // smooth
      }
      qx[x] = t1[x] - t1[x-1];                      // last column
      qy[x] = k1 * (t0[x-1] + t0[x]) + k0 * t0[x];  
      qs[x] = k1 * (t1[x-1] + t1[x]) + k0 * t1[x];
      qs += w;
    }
    else
    {
      x = 0;
      qx[x] = t1[x+1] - t1[x];                      // first column
      qy[x] = k1 * (t0[x] + t0[x+1]) + k0 * t0[x];
      // convolve horizontal
      for (x=1 ; x<wm1 ; x++)
      {
        qx[x] = t1[x+1] - t1[x-1];                      // differentiate
        qy[x] = k1 * (t0[x-1] + t0[x+1]) + k0 * t0[x];  // smooth
      }
      qx[x] = t1[x] - t1[x-1];                      // last column
      qy[x] = k1 * (t0[x-1] + t0[x]) + k0 * t0[x];  
    }
  }

  // copy values into top and bottom row 
  // (Could do this more accurately but would be a lot of work)
  const int nbytes = w * sizeof(*p);
  memcpy( qx, qx-w, nbytes );                         // bottom border
  memcpy( qy, qy-w, nbytes );
  qx = gradx->Begin();  memcpy( qx, qx+w, nbytes );   // top border
  qy = grady->Begin();  memcpy( qy, qy+w, nbytes );
  
  if (qs)  
  { 
    memcpy( qs, qs-w, nbytes );  // top border
    qs = smooth->Begin();  
    memcpy( qs, qs+w, nbytes );  // bottom border
  }
}

// compute gradient using Sobel kernel (1/8) * [1 2 1] * [-1 0 1]^T (sigma^2 = 0.5)
void GradientSobel(const ImgFloat& img, ImgFloat* gradx, ImgFloat* grady, ImgFloat* smooth)
{
  Gradient3x3(img, 0.25f, 0.125f, gradx, grady, smooth);
}

// compute gradient using Sharr kernel (1/32) * [3 10 3] * [-1 0 1]^T (sigma^2 = 1.1)
void GradientSharr(const ImgFloat& img, ImgFloat* gradx, ImgFloat* grady, ImgFloat* smooth)
{
  Gradient3x3(img, 0.3125f, 0.09375f, gradx, grady, smooth);
}


//// convolves image with 3x3 derivative of Gaussian 
//// Kernels are [1 2 1]/4 and [-1 0 1]/2 (i.e., sigma = 0.6)
//// uses separability and symmetry for speed
//void Gradient3x3WithBorders
//(
//  const ImgFloat& img, 
//  ImgFloat* gradx,
//  ImgFloat* grady
//)
//{
//  const int w = img.Width();
//  const int h = img.Height();
//  const int wm2 = w - 2;
//  const int hm2 = h - 2;
//  int x, y;
//
//  ImgFloat tmp0(w,1), tmp1(w,1);
//  float *t0, *t1;
//  const float* p = img.Begin();
//
//  gradx->Reset(w, h);
//  grady->Reset(w, h);
//
//  float* qx = gradx->Begin();
//  float* qy = grady->Begin();
//
//  // first row
//  t0 = tmp0.Begin();
//  t1 = tmp1.Begin();
//
//  // convolve vertical
//  for (x=0 ; x<w ; x++)
//  {
//    assert(p == img.Begin(x, 0));
//    t0 = 0.5f * (p[w] - p[0]);
//    *t1 = 0.5f * (p[w] + p[0]);
//    p++;  qx++;  qy++;
//  }
//
//  // convolve horizontal
//
//  // first column
//  *qx = 0.5f * (t0[1] - t0[0]);
//  *qy = 0.5f * (t1[1] + t1[0]);
//  p++;  qx++;  qy++;
//
//  // middle columns
//  for (x=1 ; x<wm2 ; x++)
//  {
//    assert(p == img.Begin(x, 0));
//    *qx = 0.5f * (t0[1] - t0[-1]);
//    *qy = 0.25f * (t1[1] + t1[-1]) + 0.5f * t0[0];
//    p++;  qx++;  qy++;
//  }
//
//  // last column
//  *qx = 0.5f * (t0[0] - t0[-1]);
//  *qy = 0.5f * (t1[0] + t1[-1]);
//  p++;  qx++;  qy++;
//
//  // middle rows
//  for (y=1 ; y<hm2 ; y++)
//  {
//    t0 = tmp0.Begin();
//    t1 = tmp1.Begin();
//
//    // convolve vertical
//    for (x=0 ; x<w ; x++)
//    {
//      *t0 = 0.5f * (p[w] - p[-w]);
//      *t1 = 0.25f * (p[-w] + p[w]) + 0.5f * p[0];
//      p++;  qx++;  qy++;
//    }
//
//    // convolve horizontal
//
//    // first column
//    *qx = 0.5f * (t0[1] - t0[0]);
//    *qy = 0.5f * (t1[1] + t1[0]);
//    p++;  qx++;  qy++;
//
//    // middle columns
//    for (x=1 ; x<wm2 ; x++)
//    {
//      *qx = 0.5f * (t0[1] - t0[-1]);
//      *qy = 0.25f * (t1[-1] + t1[1]) + 0.5f * t1[0];
//      p++;  qx++;  qy++;
//    }
//
//    // last column
//    *qx = 0.5f * (t0[0] - t0[-1]);
//    *qy = 0.5f * (t1[0] + t1[-1]);
//    p++;  qx++;  qy++;
//  }
//
//  // last row
//  t0 = tmp0.Begin();
//  t1 = tmp1.Begin();
//
//  // convolve vertical
//  for (x=0 ; x<w ; x++)
//  {
//    *t0 = 0.5f * (p[0] - p[-w]);
//    *t1 = 0.5f * (p[0] + p[-w]);
//    p++;  qx++;  qy++;
//  }
//
//  // convolve horizontal
//
//  // first column
//  *qx = 0.5f * (t0[1] - t0[0]);
//  *qy = 0.5f * (t1[1] + t1[0]);
//  p++;  qx++;  qy++;
//
//  // middle columns
//  for (x=1 ; x<wm2 ; x++)
//  {
//    *qx = 0.5f * (t0[1] - t0[-1]);
//    *qy = 0.25f * (t1[1] + t1[-1]) + 0.5f * t0[0];
//    p++;  qx++;  qy++;
//  }
//
//  // last column
//  *qx = 0.5f * (t0[0] - t0[-1]);
//  *qy = 0.5f * (t1[0] + t1[-1]);
//  p++;  qx++;  qy++;
//
////
////
////    // first column
////    *qx = 0.5f * (p[1] - p[0]);
////    *qy = 0.5f * (p[1] + p[0]);
////    p++;  qx++;  qy++;
////
////    for (x=1 ; x<wm2 ; x++)
////    {
////      *qx = 0.5f * (p[1] - p[-1]);
////      *qy = 0.25f * (p[-1] + p[1]) + 0.5f * p[0];
////      p++;  qx++;  qy++;
////    }
////
////    // last column
////    *qx = 0.5f * (p[0] - p[-1]);
////    *qy = 0.5f * (p[0] + p[-1]);
////    p++;  qx++;  qy++;
////  }
//
//}


void RectToMagPhase(const ImgFloat& gradx, const ImgFloat& grady,
                   ImgFloat* mag, ImgFloat* phase)
{
  assert(IsSameSize(gradx, grady));
  mag->Reset(gradx.Width(), gradx.Height());
  phase->Reset(gradx.Width(), gradx.Height());
  ImgFloat::ConstIterator px = gradx.Begin();
  ImgFloat::ConstIterator py = grady.Begin();
  ImgFloat::Iterator pm = mag->Begin();
  ImgFloat::Iterator pp = phase->Begin();
  while (px != gradx.End())
  {
    *pm = sqrt( (*px) * (*px) + (*py) * (*py) );
    *pp = atan2( *py, *px );
    px++;  py++;  pm++;  pp++;
  }
}

void MagPhaseToRect(const ImgFloat& mag, const ImgFloat& phase,
                   ImgFloat* gradx, ImgFloat* grady)
{
  assert(IsSameSize(mag, phase));
  gradx->Reset(mag.Width(), mag.Height());
  grady->Reset(mag.Width(), mag.Height());
  ImgFloat::ConstIterator pm = mag.Begin();
  ImgFloat::ConstIterator pp = phase.Begin();
  ImgFloat::Iterator px = gradx->Begin();
  ImgFloat::Iterator py = grady->Begin();
  while (pm != mag.End())
  {
    *px = (*pm) * cos( *pp );
    *py = (*pm) * sin( *pp );
    px++;  py++;  pm++;  pp++;
  }
}

void GradMagPrewitt(const ImgGray& img, ImgGray* out)
{
  InPlaceSwapper< ImgGray > inplace(img, &out);
  const int border = 1;  // values are invalid in this region
  int w = img.Width();
  int h = img.Height();
  int y;

  out->Reset(w, h);

  // This is an easy way to set all the border pixels to zero;
  // if speed is important we could rewrite this line to copy only
  // the border pixels.
  Set(out, 0);

  // convolve with horizontal [-1 0 1] and vertical [-1 0 1]^T kernels;
  // approximate magnitude with sum of absolute values
  for (y=border ; y<h-border ; y++)
  {
    ImgGray::ConstIterator pi = img.Begin(border, y);
    ImgGray::Iterator po = out->Begin(border, y);
    ImgGray::Iterator pe = out->Begin(w-border, y);
    while (po != pe)
    {
      *po++ = blepo_ex::Abs(pi[1] - pi[-1]) + blepo_ex::Abs(pi[w] - pi[-w]);
      pi++;
    }
  }

}

void GradMagSobel(const ImgGray& img, ImgGray* out)
{
  InPlaceSwapper< ImgGray > inplace(img, &out);
  const int border = 1;  // values are invalid in this region
  int w = img.Width();
  int h = img.Height();
  int y;

  out->Reset(w, h);

  // This is an easy way to set all the border pixels to zero;
  // if speed is important we could rewrite this line to copy only
  // the border pixels.
  Set(out, 0);

  // approximate magnitude with sum of absolute values
  for (y=border ; y<h-border ; y++)
  {
    ImgGray::ConstIterator pi = img.Begin(border, y);
    ImgGray::Iterator po = out->Begin(border, y);
    ImgGray::Iterator pe = out->Begin(w-border, y);
    while (po != pe)
    {
      int val1 = pi[w-1] + 2*pi[w] + pi[w+1] -pi[-w-1] -2*pi[-w] -pi[-w+1];
      val1 = (val1 < 0) ? -val1 : val1;
      int val2 = pi[-w+1] + 2*pi[1] + pi[w+1] -pi[-w-1] -2*pi[-1] -pi[w-1]; 
      val2 = (val2 < 0) ? -val2 : val2;
      int val= (val1 > val2) ? val1 : val2;                    
      val = (val > 255) ? 255 : val;
      *po = (blepo::ImgGray::Pixel) val;
      pi++;
      po++;
    }
  }

}

void GradPrewittX(const ImgGray& img, ImgFloat* out)
{
  const int border = 1;  // values are invalid in this region
  int w = img.Width();
  int h = img.Height();
  int y;

  out->Reset(w, h);

  // This is an easy way to set all the border pixels to zero;
  // if speed is important we could rewrite this line to copy only
  // the border pixels.
  Set(out, 0);

  // convolve with horizontal [-1 0 1] kernel
  for (y=0 ; y<h ; y++)
  {
    ImgGray::ConstIterator pi = img.Begin(border, y);
    ImgFloat::Iterator po = out->Begin(border, y);
    ImgFloat::Iterator pe = out->Begin(w-border, y);
    while (po != pe)
    {
      *po++ = static_cast<float>( (pi[1] - pi[-1]) );
      pi++;
    }
  }
}

void GradPrewittY(const ImgGray& img, ImgFloat* out)
{
  const int border = 1;  // values are invalid in this region
  int w = img.Width();
  int h = img.Height();
  int y;

  out->Reset(w, h);

  // This is an easy way to set all the border pixels to zero;
  // if speed is important we could rewrite this line to copy only
  // the border pixels.
  Set(out, 0);

  // convolve with vertical [-1 0 1]^T kernel
  for (y=border ; y<h-border ; y++)
  {
    ImgGray::ConstIterator pi = img.Begin(0, y);
    ImgFloat::Iterator po = out->Begin(0, y);
    ImgFloat::Iterator pe = po + w;
    while (po != pe)
    {
      *po++ = static_cast<float>( (pi[w] - pi[-w]) );
      pi++;
    }
  }
}

void GradPrewitt(const ImgGray& img, ImgFloat* gradx, ImgFloat* grady)
{
  GradPrewittX(img, gradx);
  GradPrewittY(img, grady);
}

void FindTransitionPixels(const ImgBinary& in, ImgBinary* out) { iFindTransitionPixels(in, out); }
void FindTransitionPixels(const ImgBgr   & in, ImgBinary* out) { iFindTransitionPixels(in, out); }
void FindTransitionPixels(const ImgFloat & in, ImgBinary* out) { iFindTransitionPixels(in, out); }
void FindTransitionPixels(const ImgGray  & in, ImgBinary* out) { iFindTransitionPixels(in, out); }
void FindTransitionPixels(const ImgInt   & in, ImgBinary* out) { iFindTransitionPixels(in, out); }

/**
Correlates an integer/float image with a kernel.
The centre of the kernel is computed as [(KernelWidth-1)/2,(KernelHeight-1)/2]. 
@author Prashant Oswal
*/

void Correlate(const ImgInt& img,const ImgInt& kernel,ImgInt* out)
{
  assert( !img.IsNull() && !kernel.IsNull() );
  out->Reset(img.Width(),img.Height());
  if(img.Width() < kernel.Width() || img.Height() < kernel.Height())
  {
    Set(out,0);
    return;
  }
  const int height = img.Height();
  const int width = img.Width();
  const int centre_x = (kernel.Width()-1)/2;
  const int centre_y = (kernel.Height()-1)/2;
  const int vert_margin_up = centre_y;
  const int horiz_margin_lft = centre_x;
  const int vert_margin_dn = kernel.Height()-centre_y - 1;
  const int horiz_margin_rgt = kernel.Width()-centre_x-1;
  int i,j;
  
  //Setting the border pixels to zero.
  int border_value = 0;
  for(i=0;i<vert_margin_up;i++)
  {
    ImgInt::Iterator po_up=out->Begin(0,i);
    for(j=0;j<width;j++)
      *po_up++ = border_value;
  }
  for(i=height-vert_margin_dn;i<height;i++)
  {
    ImgInt::Iterator po_dn=out->Begin(0,i);
    for(j=0;j<width;j++)
      *po_dn++ = border_value;
  }
  for(i=vert_margin_up;i<height-vert_margin_dn;i++)
  {
    ImgInt::Iterator po_lft=out->Begin(0,i);
    ImgInt::Iterator po_rgt = out->Begin(width-1,i);
    for(j=0;j<horiz_margin_lft;j++)
      *po_lft++ = border_value;
    for(j=0;j<horiz_margin_rgt;j++)
      *po_rgt-- = border_value;
  }
  
  
  ImgInt::Pixel sum_of_products = 0;
  for (i=vert_margin_up;i<height-vert_margin_dn;i++)
  {
    ImgInt::Iterator po_image = out->Begin(horiz_margin_lft,i); 
    for (j=horiz_margin_lft;j<width-horiz_margin_rgt;j++)
    {
      for (int x=i-vert_margin_up,k=0; x<=i+vert_margin_dn; x++,k++)
      {
        ImgInt::ConstIterator po_wndw = img.Begin(j-horiz_margin_lft,x);
        ImgInt::ConstIterator po_krnl = kernel.Begin(0,k);
        for (int y=0; y<kernel.Width(); y++)
        {
          sum_of_products = sum_of_products + (*po_wndw++)*(*po_krnl++);
        }
      }
      *po_image++ = sum_of_products;
      sum_of_products = 0;
    }
  }
}

void Correlate(const ImgFloat& img,const ImgFloat& kernel,ImgFloat* out, CorrelateType type)
{
  assert( !img.IsNull() && !kernel.IsNull() );
  out->Reset(img.Width(),img.Height());
  if(img.Width() < kernel.Width() || img.Height() < kernel.Height())
  {
    Set(out,0.0);
    return;
  }
  const int height = img.Height();
  const int width = img.Width();
  const int centre_x = (kernel.Width()-1)/2;
  const int centre_y = (kernel.Height()-1)/2;
  const int vert_margin_up = centre_y;
  const int horiz_margin_lft = centre_x;
  const int vert_margin_dn = kernel.Height()-centre_y - 1;
  const int horiz_margin_rgt = kernel.Width()-centre_x-1;
  int i,j;
  
  //Setting the border pixels to zero.
  float border_value = 0.0f;
  for(i=0;i<vert_margin_up;i++)
  {
    ImgFloat::Iterator po_up=out->Begin(0,i);
    for(j=0;j<width;j++)
      *po_up++ = border_value;
  }
  for(i=height-vert_margin_dn;i<height;i++)
  {
    ImgFloat::Iterator po_dn=out->Begin(0,i);
    for(j=0;j<width;j++)
      *po_dn++ = border_value;
  }
  for(i=vert_margin_up;i<height-vert_margin_dn;i++)
  {
    ImgFloat::Iterator po_lft=out->Begin(0,i);
    ImgFloat::Iterator po_rgt = out->Begin(width-1,i);
    for(j=0;j<horiz_margin_lft;j++)
      *po_lft++ = border_value;
    for(j=0;j<horiz_margin_rgt;j++)
      *po_rgt-- = border_value;
  }
  
  double sum_of_products = 0.0;
  double suma = 0.0;
  double sumb = 0.0;
  for (i=vert_margin_up;i<height-vert_margin_dn;i++)
  {
    ImgFloat::Iterator po_image = out->Begin(horiz_margin_lft,i); 
    for (j=horiz_margin_lft;j<width-horiz_margin_rgt;j++)
    {
      for (int x=i-vert_margin_up,k=0; x<=i+vert_margin_dn; x++,k++)
      {
        ImgFloat::ConstIterator po_wndw = img.Begin(j-horiz_margin_lft,x);
        ImgFloat::ConstIterator po_krnl = kernel.Begin(0,k);
        for (int y=0; y<kernel.Width(); y++)
        {
          switch (type)
          {
          case BPO_CORR_NORMALIZED:
            suma += (*po_wndw)*(*po_wndw);
            sumb += (*po_krnl)*(*po_krnl);
            sum_of_products += (*po_wndw++)*(*po_krnl++);
            break;
          case BPO_CORR_SSD:
            {
              double diff = (*po_wndw++) - (*po_krnl++);
              sum_of_products += diff * diff;
              break;
            }
          case BPO_CORR_STANDARD:
          default:
            sum_of_products += (*po_wndw++)*(*po_krnl++);
            break;
          }
        }
      }
      if (type==BPO_CORR_NORMALIZED)
      {
        *po_image++ = static_cast<float>(sum_of_products / sqrt(suma * sumb));
      }
      else
      {
        *po_image++ = static_cast<float>(sum_of_products);
      }
      suma = 0.0;
      sumb = 0.0;
      sum_of_products = 0.0;
    }
  }
}

/**
Convolves an integer/float image with a kernel.
The kernel is flipped in the vertical and horizontal direction.
Support for slow implementation of Convolve function may be removed at a later date.
@author Prashant Oswal
*/

void Convolve(const ImgInt& img,const ImgInt& kernel,ImgInt* out)
{
  ImgInt kernel_copy;
  kernel_copy = kernel;
  FlipVertical(kernel_copy, &kernel_copy);
  FlipHorizontal(kernel_copy, &kernel_copy);
  Correlate(img,kernel_copy,out);
}

void Convolve(const ImgFloat& img,const ImgFloat& kernel,ImgFloat* out)
{
  ImgFloat kernel_copy;
  kernel_copy = kernel;
  FlipVertical(kernel_copy, &kernel_copy);
  FlipHorizontal(kernel_copy, &kernel_copy);
  Correlate(img,kernel_copy,out, BPO_CORR_STANDARD);
}

//void ConvolveSlow(const ImgInt& img,const ImgInt& kernel,ImgInt* out)
//{
//  // The kernel height and width must be odd. 
//  // This allows unambiguous placement of the kernel centre.
//  assert((kernel.Height())%2 != 0);
//  assert((kernel.Width())%2 != 0);
//  out->Reset(img.Width(),img.Height());
//  const int height = img.Height();
//  const int width = img.Width();
//  const int vert_margin = kernel.Height()/2;
//  const int horiz_margin = kernel.Width()/2;
//  int i,j;
//  
//  // Due to overflow the convolution sum of the pixels at the image boundary cannot be computed.
//  // The boundary pixels are set to zero.
//  for (i=0;i<vert_margin;i++)
//  {
//    for (j=0;j<width;j++)
//  {
//    *(out->Begin(j,i)) = 0;
//    *(out->Begin(j,height-i-1)) = 0;
//  }
//  }
//  
//  // Due to overflow the convolution sum of the pixels at the image boundary cannot be computed.
//  // The boundary pixels are set to zero.
//  for (i=0;i<horiz_margin;i++)
//  {
//    for (j=vert_margin;j<height-vert_margin;j++)
//  {
//      *(out->Begin(i,j)) = 0;
//      *(out->Begin(width-i-1,j)) = 0;
//  }
//  }
//
//  ImgInt::Pixel sum_of_products = 0;
//  for (i=vert_margin;i<height-vert_margin;i++)
//  {
//    for (j=horiz_margin;j<width-horiz_margin;j++)
//  {
//      for (int x=i-vert_margin,k=0; x<=i+vert_margin; x++,k++)
//    {
//        for (int y=j-horiz_margin,l=0; y<=j+horiz_margin; y++,l++)
//    {
//          sum_of_products = sum_of_products + (*(img.Begin(y,x)))*(*(kernel.Begin(l,k)));
//    }
//    }
//      *(out->Begin(j,i)) = sum_of_products;
//      sum_of_products = 0;
//  }
//  }
//}
//
//void ConvolveSlow(const ImgFloat& img,const ImgFloat& kernel,ImgFloat* out)
//{
//  // The kernel height and width must be odd. 
//  // This allows unambiguous placement of the kernel centre.
//  assert((kernel.Height())%2 != 0);
//  assert((kernel.Width())%2 != 0);
//  out->Reset(img.Width(),img.Height());
//  const int height = img.Height();
//  const int width = img.Width();
//  const int vert_margin = kernel.Height()/2;
//  const int horiz_margin = kernel.Width()/2;
//  int i,j;
//  
//  // Due to overflow the convolution sum of the pixels at the image boundary cannot be computed.
//  // The boundary pixels are set to zero.
//  for (i=0;i<vert_margin;i++)
//  {
//    for (j=0;j<width;j++)
//    {
//        *(out->Begin(j,i)) = 0.0;
//        *(out->Begin(j,height-i-1)) = 0.0;
//    }
//  }
//  
//  // Due to overflow the convolution sum of the pixels at the image boundary cannot be computed.
//  // The boundary pixels are set to zero.
//  for (i=0;i<horiz_margin;i++)
//  {
//    for (j=vert_margin;j<height-vert_margin;j++)
//    {
//        *(out->Begin(i,j)) = 0.0;
//        *(out->Begin(width-i-1,j)) = 0.0;
//    }
//  }
//
//  ImgFloat::Pixel sum_of_products = 0.0;
//  for (i=vert_margin;i<height-vert_margin;i++)
//  {
//    for (j=horiz_margin;j<width-horiz_margin;j++)
//  {
//      for (int x=i-vert_margin,k=0; x<=i+vert_margin; x++,k++)
//    {
//        for (int y=j-horiz_margin,l=0; y<=j+horiz_margin; y++,l++)
//    {
//          sum_of_products = sum_of_products + ((*(img.Begin(y,x)))*(*(kernel.Begin(l,k))));
//    }
//    }
//      *(out->Begin(j,i)) = sum_of_products;
//      sum_of_products = 0.0;
//  }
//  }
//}

// Extend image to enable convolution not to affect borders
void EnlargeByExtension(const ImgBgr   & img, int border, ImgBgr   * out) { iEnlargeByExtension(img, border, out); }
void EnlargeByExtension(const ImgBinary& img, int border, ImgBinary* out) { iEnlargeByExtension(img, border, out); }
void EnlargeByExtension(const ImgFloat & img, int border, ImgFloat * out) { iEnlargeByExtension(img, border, out); }
void EnlargeByExtension(const ImgGray  & img, int border, ImgGray  * out) { iEnlargeByExtension(img, border, out); }
void EnlargeByExtension(const ImgInt   & img, int border, ImgInt   * out) { iEnlargeByExtension(img, border, out); }

void EnlargeByReflection(const ImgBgr   & img, int border, ImgBgr   * out) { iEnlargeByReflection(img, border, out); }
void EnlargeByReflection(const ImgBinary& img, int border, ImgBinary* out) { iEnlargeByReflection(img, border, out); }
void EnlargeByReflection(const ImgFloat & img, int border, ImgFloat * out) { iEnlargeByReflection(img, border, out); }
void EnlargeByReflection(const ImgGray  & img, int border, ImgGray  * out) { iEnlargeByReflection(img, border, out); }
void EnlargeByReflection(const ImgInt   & img, int border, ImgInt   * out) { iEnlargeByReflection(img, border, out); }

//// This is a weird name, but it goes with the other Enlarge functions
void EnlargeByCropping(const ImgBgr   & img, int border, ImgBgr   * out) { iEnlargeByCropping(img, border, out); }
void EnlargeByCropping(const ImgBinary& img, int border, ImgBinary* out) { iEnlargeByCropping(img, border, out); }
void EnlargeByCropping(const ImgFloat & img, int border, ImgFloat * out) { iEnlargeByCropping(img, border, out); }
void EnlargeByCropping(const ImgGray  & img, int border, ImgGray  * out) { iEnlargeByCropping(img, border, out); }
void EnlargeByCropping(const ImgInt   & img, int border, ImgInt   * out) { iEnlargeByCropping(img, border, out); }


// old, non-templated, non-pointerized version:
// Extend image to enable convolution not to affect borders
//void EnlargeByExtension(const ImgFloat& img, int border, ImgFloat* out)
//{
//  InPlaceSwapper< ImgFloat > inplace(img, &out);
//  assert( border >= 0 );
//  const int w = img.Width(), h = img.Height();
//  out->Reset(w + 2*border, h + 2*border);
//  Set(out, Point(border, border), img);
//  int x, y;
//  ImgFloat::Pixel val;
//  for (y=border ; y<h+border ; y++)
//  {
//    // extend left
//    val = (*out)(border, y);
//    for (x=0 ; x<border ; x++)
//    {
//      (*out)(x, y) = val;
//    }
//    // extend right
//    val = (*out)(w+border-1, y);
//    for (x=w+border ; x<w+2*border ; x++)
//    {
//      (*out)(x, y) = val;
//    }
//  }
//  for (x=0 ; x<w+2*border ; x++)
//  {
//    // extend top
//    val = (*out)(x, border);
//    for (y=0 ; y<border ; y++)
//    {
//      (*out)(x, y) = val;
//    }
//    // extend bottom
//    val = (*out)(x, h+border-1);
//    for (y=h+border ; y<h+2*border ; y++)
//    {
//      (*out)(x, y) = val;
//    }
//  }
//}
//
//// This is a weird name, but it goes with the other Enlarge functions
//void EnlargeByCropping(const ImgFloat& img, int border, ImgFloat* out)
//{
//  assert(border > 0);
//  assert(img.Width() >= 2*border);
//  assert(img.Height() >= 2*border);
//  Extract(img, Rect(border, border, img.Width()-border, img.Height()-border), out);
//}

// Uses kernel [ 0 -1 0 ; -1 4 -1 ; 0 -1 0 ]
void LaplacianOfGaussian(const ImgGray& img, ImgGray* out)
{
  const int w = img.Width();
  const int h = img.Height();
  out->Reset(w, h);

  // set top and bottom borders to zero (side borders are automatically filled in below, albeit with garbage)
  Set(out, 0, 0, 0, w+1);
  Set(out, 0, w-1, h-2, w+1);

  const unsigned char* p  = img.Begin(1, 1);
  const unsigned char* pu = p - w;
  const unsigned char* pd = p + w;
  const unsigned char* pl = p - 1;
  const unsigned char* pr = p + 1;
  unsigned char* po = out->Begin(1, 1);
  const unsigned char* end = img.Begin(w-1, h-2);

  while (p != end)
  {
    int val = (*p << 2) - *pu - *pd - *pl - *pr;
    *po++ = blepo_ex::Clamp((val+255)/2, 0, 255);
    p++;  pu++;  pd++;  pl++;  pr++;
  }
}

// Uses kernel [ 0 -1 0 ; -1 4 -1 ; 0 -1 0 ]
void LaplacianOfGaussian(const ImgGray& img, ImgInt* out)
{
  const int w = img.Width();
  const int h = img.Height();
  out->Reset(w, h);

  // set top and bottom borders to zero (side borders are automatically filled in below, albeit with garbage)
  Set(out, 0, 0, 0, w+1);
  Set(out, 0, w-1, h-2, w+1);

  const unsigned char* p  = img.Begin(1, 1);
  const unsigned char* pu = p - w;
  const unsigned char* pd = p + w;
  const unsigned char* pl = p - 1;
  const unsigned char* pr = p + 1;
  int* po = out->Begin(1, 1);
  const unsigned char* end = img.Begin(w-1, h-2);

  while (p != end)
  {
    int val = (*p << 2) - *pu - *pd - *pl - *pr;
    *po++ = val;
    p++;  pu++;  pd++;  pl++;  pr++;
  }
}

void ZeroCrossings(const ImgGray& img, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  for (int y=1 ; y<img.Height() ; y++)
  {
    for (int x=1 ; x<img.Width() ; x++)
    {
      //if ( (img(x,y) * img(x-1,y) <= 0)
      //  || (img(x,y) * img(x,y-1) <= 0))
      if ( (img(x,y) > 0 && img(x-1,y) < 0)
        || (img(x,y) > 0 && img(x,y-1) < 0))
      {
        (*out)(x,y) = 1;
      }
      else
      {
        (*out)(x,y) = 0;
      }
    }
  }
}

void ZeroCrossings(const ImgInt& img, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  for (int y=1 ; y<img.Height() ; y++)
  {
    for (int x=1 ; x<img.Width() ; x++)
    {
      //if ( (img(x,y) * img(x-1,y) <= 0)
      //  || (img(x,y) * img(x,y-1) <= 0))
      if ( (img(x,y) > 0 && img(x-1,y) < 0)
        || (img(x,y) > 0 && img(x,y-1) < 0))
      {
        (*out)(x,y) = 1;
      }
      else
      {
        (*out)(x,y) = 0;
      }
    }
  }
}

// Uses kernel [ 0 -1 0 ; -1 4 -1 ; 0 -1 0 ]
void Slog(const ImgGray& img, ImgBinary* out, int th)
{
  const int w = img.Width();
  const int h = img.Height();
  out->Reset(w, h);

  // set top and bottom borders to zero (side borders are automatically filled in below, albeit with garbage)
  Set(out, 0, 0, 0, w+1);
  Set(out, 0, w-1, h-2, w+1);

  const unsigned char* p  = img.Begin(1, 1);
  const unsigned char* pu = p - w;
  const unsigned char* pd = p + w;
  const unsigned char* pl = p - 1;
  const unsigned char* pr = p + 1;
  ImgBinary::Iterator po = out->Begin(1, 1);

  for (int y=1 ; y<h-1 ; y++)
  {
    for (int x=1 ; x<w-1 ; x++)
    {
      int val = (*p << 2) - *pu - *pd - *pl - *pr;
      *po++ = (val > th);
      p++;  pu++;  pd++;  pl++;  pr++;
    }
  }
}

// This function was used to print 'bit_reversal_array' below.
//void PrintBitReversalArray()
//{
//  FILE* fp = fopen("foo.txt", "wt");
//  unsigned char reverse[16] = { 0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15 };
//  fprintf(fp, "static unsigned char bit_reversal_array[256] = \n{\n");
//  for (int i=0 ; i<16 ; i++)
//  {
//    fprintf(fp, "  ");
//    int low_nibble = reverse[i];
//    for (int j=0 ; j<16 ; j++)
//    {
//      int high_nibble = reverse[j];
//      unsigned char byte = (high_nibble << 4) | low_nibble;
//      fprintf(fp, "0x%02x, ", byte);
//    }
//    fprintf(fp, "\n");
//  }
//  fprintf(fp, "};\n");
//  fclose(fp);
//}

// This array contains the bytes that result from reversing the bits in a byte so 
// that the most significant bit becomes the least signficant bit, and so on.
static unsigned char bit_reversal_array[256] = 
{
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 
  0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 
  0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 
  0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 
  0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 
  0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 
  0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 
  0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 
  0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 
  0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 
  0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

// inplace is okay
void ReverseBits(const ImgBinary& img, ImgBinary* out)
{
  int foo = bit_reversal_array[0x67];
  if (out != &img)  out->Reset(img.Width(), img.Height());
  const unsigned char* p = img.BytePtr();
  unsigned char* q = out->BytePtr();
  const unsigned char* end = img.BytePtrEnd();
  while (p != end)  *q++ = bit_reversal_array[ *p++ ];
}

// inplace NOT okay
void ReverseBytes(const ImgBinary& img, ImgBinary* out)
{
  assert(out != &img);
  out->Reset(img.Width(), img.Height());
  int nbytes = img.NBytes();
  const unsigned char* p = img.BytePtr();
  const unsigned char* end = p + nbytes;
  unsigned char* q = out->BytePtr() + nbytes-1;
  while (p != end)
  {
    *q-- = *p++;
  }
}

// slow way, for debugging
void ShiftLeftSlow(const ImgBinary& img, int shift_amount, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  Set(out, 0);
  for (int y=0 ; y<img.Height() ; y++)
  {
    for (int x=0 ; x<img.Width()-shift_amount ; x++)
    {
      (*out)(x, y) = img(x + shift_amount, y);
    }
  }
}

// slow way, for debugging
void ShiftRightSlow(const ImgBinary& img, int shift_amount, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  Set(out, 0);
  for (int y=0 ; y<img.Height() ; y++)
  {
    for (int x=shift_amount ; x<img.Width() ; x++)
    {
      (*out)(x, y) = img(x - shift_amount, y);
    }
  }
}

// slow way, for debugging
void ShiftUpSlow(const ImgBinary& img, int shift_amount, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  Set(out, 0);
  for (int y=0 ; y<img.Height()-shift_amount ; y++)
  {
    for (int x=0 ; x<img.Width() ; x++)
    {
      (*out)(x, y) = img(x, y + shift_amount);
    }
  }
}

// slow way, for debugging
void ShiftDownSlow(const ImgBinary& img, int shift_amount, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  Set(out, 0);
  for (int y=img.Height()-1 ; y>=shift_amount ; y--)
  {
    for (int x=0 ; x<img.Width() ; x++)
    {
      (*out)(x, y) = img(x, y - shift_amount);
    }
  }
}

void ShiftLeft(const ImgBinary& img, int shift_amount, ImgBinary* out)
{
  if (CanDoSse2() && img.NBytes() % nbytes_per_doublequadword == 0) 
  {
    *out = img;
    // Reverse bits so that little endian loads preserves adjacency of bits
    ReverseBits(*out, out);
    const int num_doublequadwords = img.NBytes() / nbytes_per_doublequadword;
    // Because bits are reversed, must shift right instead of left
    xmm_shiftright(out->BytePtr(), out->BytePtr(), shift_amount, num_doublequadwords);
    ReverseBits(*out, out);
    {
      Figure fig("slow"), figg("diff");
      ImgBinary tmp, tmp2;
      ShiftLeftSlow(img, shift_amount, &tmp);
      Xor(tmp, *out, &tmp2);
      fig.Draw(tmp);
      figg.Draw(tmp2);
    }
  }
  else if (CanDoMmx() && img.NBytes() % nbytes_per_quadword == 0)
  {
    *out = img;
    // Reverse bits so that little endian loads preserves adjacency of bits
    ReverseBits(*out, out);
    const int num_quadwords = img.NBytes() / nbytes_per_quadword;
    // Because bits are reversed, must shift right instead of left
    mmx_shiftright(out->BytePtr(), out->BytePtr(), shift_amount, num_quadwords);
    ReverseBits(*out, out);
  }
  else
  {
    ShiftLeftSlow(img, shift_amount, out);
  }
}

void ShiftRight(const ImgBinary& img, int shift_amount, ImgBinary* out)
{
  if (CanDoSse2() && img.NBytes() % nbytes_per_doublequadword == 0)
  {
    *out = img;
    // Reverse bits so that little endian loads preserves adjacency of bits
    ReverseBits(*out, out);  
    const int num_dblquadwords = img.NBytes() / nbytes_per_doublequadword;
    // Because bits are reversed, must shift left instead of right
    xmm_shiftleft(out->BytePtr(), out->BytePtr(), shift_amount, num_dblquadwords);
    ReverseBits(*out, out);
    {
      Figure fig("slow"), figg("diff");
      ImgBinary tmp, tmp2;
//      tmp.Reset( out->Width(), out->Height() );
//      ImgBinary foo = img;
//      ReverseBits(foo, &foo);
//      mmx_shiftleft(foo.BytePtr(), tmp.BytePtr(), shift_amount, num_dblquadwords * 2);
//      ReverseBits(tmp, &tmp);
      ShiftRightSlow(img, shift_amount, &tmp);
      Xor(tmp, *out, &tmp2);
      fig.Draw(tmp);
      figg.Draw(tmp2);
    }
  }
  else if (CanDoMmx() && img.NBytes() % nbytes_per_quadword == 0)
  {
    *out = img;
    // Reverse bits so that little endian loads preserves adjacency of bits
    ReverseBits(*out, out);  
    const int num_quadwords = img.NBytes() / nbytes_per_quadword;
    // Because bits are reversed, must shift left instead of right
    mmx_shiftleft(out->BytePtr(), out->BytePtr(), shift_amount, num_quadwords);
    ReverseBits(*out, out);
  }
  else
  {
    ShiftRightSlow(img, shift_amount, out);
  }
}

void ShiftUp(const ImgBinary& img, int shift_amount, ImgBinary* out)
{
  if (img.Width() % 8 == 0)
  {
    InPlaceSwapper< ImgBinary > inplace(img, &out);
    out->Reset( img.Width(), img.Height() );
    int nbytes_per_row = (img.Width() / 8);
    int nrows_to_copy = img.Height() - shift_amount;
    const unsigned char* p = img.BytePtr() + nbytes_per_row * shift_amount;
    memcpy(out->BytePtr(), p, nbytes_per_row * nrows_to_copy);
  }
  else
  {
    ShiftUpSlow(img, shift_amount, out);
  }
}

void ShiftDown(const ImgBinary& img, int shift_amount, ImgBinary* out)
{
  if (img.Width() % 8 == 0)
  {
    InPlaceSwapper< ImgBinary > inplace(img, &out);
    out->Reset( img.Width(), img.Height() );
    int nbytes_per_row = (img.Width() / 8);
    int nrows_to_copy = img.Height() - shift_amount;
    const unsigned char* p = img.BytePtr();
    memcpy(out->BytePtr() + nbytes_per_row * shift_amount, p, nbytes_per_row * nrows_to_copy);
  }
  else
  {
    ShiftDownSlow(img, shift_amount, out);
  }
}

void Shift(const ImgBinary& img, int dx, int dy, ImgBinary* out)
{
  if (dy >= 0)  ShiftDown (img ,  dy, out);
  else          ShiftUp   (img , -dy, out);

  if (dx >= 0)  ShiftRight(*out,  dx, out);
  else          ShiftLeft (*out, -dx, out);
}

// This function was used to print 'bit_count_array' below.
//void PrintBitCountArray()
//{
//  FILE* fp = fopen("foo.txt", "wt");
//  fprintf(fp, "static unsigned char bit_count_array[256] = \n{\n");
//  for (int i=0 ; i<16 ; i++)
//  {
//    fprintf(fp, "  ");
//    int high_nibble = i;
//    for (int j=0 ; j<16 ; j++)
//    {
//      int low_nibble = j;
//      unsigned char byte = (high_nibble << 4) | low_nibble;
//      int count = 0;
//      for (int k=0 ; k<8 ; k++)
//      {
//        if (byte & 1)  count++;
//        byte >>= 1;
//      }
//      fprintf(fp, "0x%02x, ", count);
//    }
//    fprintf(fp, "\n");
//  }
//  fprintf(fp, "};\n");
//  fclose(fp);
//}

// The ith element of this array is the number of bits in the binary representation
// of the number i.
static unsigned char bit_count_array[256] = 
{
  0x00, 0x01, 0x01, 0x02, 0x01, 0x02, 0x02, 0x03, 0x01, 0x02, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04, 
  0x01, 0x02, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04, 0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 
  0x01, 0x02, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04, 0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 
  0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 
  0x01, 0x02, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04, 0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 
  0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 
  0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 
  0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 0x04, 0x05, 0x05, 0x06, 0x05, 0x06, 0x06, 0x07, 
  0x01, 0x02, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04, 0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 
  0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 
  0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 
  0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 0x04, 0x05, 0x05, 0x06, 0x05, 0x06, 0x06, 0x07, 
  0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 
  0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 0x04, 0x05, 0x05, 0x06, 0x05, 0x06, 0x06, 0x07, 
  0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 0x04, 0x05, 0x05, 0x06, 0x05, 0x06, 0x06, 0x07, 
  0x04, 0x05, 0x05, 0x06, 0x05, 0x06, 0x06, 0x07, 0x05, 0x06, 0x06, 0x07, 0x06, 0x07, 0x07, 0x08 
};

void CountSlogDiffBits8x8Slow(const ImgBinary& slogdiff, ImgGray* out)
{
  if (slogdiff.Width() % 8 == 0 && slogdiff.Height() % 8 == 0)
  {
    const int w = slogdiff.Width() / 8;
    const int h = slogdiff.Height() / 8;
    out->Reset( w, h );
    for (int y=0 ; y<h ; y++)
    {
      for (int x=0 ; x<w ; x++)
      {
        int sum = 0;
        for (int dy=0 ; dy<8 ; dy++)
        {
          for (int dx=0 ; dx<8 ; dx++)
          {
            sum += bit_count_array[ slogdiff(x*8+dx, y*8+dy) ];
          }
        }
        (*out)(x, y) = sum;
      }
    }
  }
  else
  {
    assert(0);  // not implemented
  }
}

void CountSlogDiffBits8x8(const ImgBinary& slogdiff, ImgGray* out)
{
  if (slogdiff.Width() % 8 == 0 && slogdiff.Height() % 8 == 0)
  {
    const int w = slogdiff.Width() / 8;
    const int h = slogdiff.Height() / 8;
    out->Reset( w, h );
    const unsigned char* p = slogdiff.BytePtr();
    unsigned char* q = out->BytePtr();
    for (int y=0 ; y<h ; y++)
    {
      for (int x=0 ; x<w ; x++)
      {
        int sum = 0;
        for (int dy=0 ; dy<8 ; dy++)
        {
          sum += bit_count_array[ p[ dy*w ] ];
        }
        *q++ = sum;
        p++;
      }
      p += w*7;
    }

    {
      ImgGray tmp;
      CountSlogDiffBits8x8Slow(slogdiff, &tmp);
      assert(IsIdentical(*out, tmp));
    }
  }
  else
  {
    assert(0);  // not implemented
  }
}

void SlogCorrelate(const ImgGray& img1, const ImgGray& img2, 
                   ImgGray* dxx, ImgGray* dyy, ImgGray* confidence)
{
//  PrintBitCountArray();
  Figure fig1, fig2, fig3, fig4, fig5, fig6, fig7;
  assert(img1.Width() == img2.Width() && img1.Height() == img2.Height());
  ImgBinary slog1, slog2, slogdiff;
  slogdiff.Reset( img1.Width(), img1.Height() );
  Slog(img1, &slog1);
  Slog(img2, &slog2);
  ImgGray minval;
  ImgInt doxxg, doyyg;
  ImgInt* doxx = &doxxg, *doyy = &doyyg;
  {
    assert( img1.Width() % 8 == 0 && img1.Height() % 8 == 0 );
    doxx->Reset( img1.Width() / 8, img1.Height() / 8 );
    doyy->Reset( img1.Width() / 8, img1.Height() / 8 );
    minval.Reset( img1.Width() / 8, img1.Height() / 8 );
    Set(&minval, 255);
  }

  unsigned char* p1 = slog1.BytePtr();
  unsigned char* p2 = slog2.BytePtr();
  unsigned char* po = slogdiff.BytePtr();

  assert(slog1.NBytes() == slog2.NBytes());
  assert(slog1.NBytes() % nbytes_per_doublequadword == 0);  // if this is not true, then we'll have to deal separately with extra pixels at the end
  int num_doublequadwords = slog1.NBytes() / nbytes_per_doublequadword;

  fig1.Draw(slog1);
  fig2.Draw(slog2);
  ImgBinary tmp;
  ImgGray bitcount;
  const int sr = 4;
//  for (int dy=1 ; dy<=1 ; dy++)
//  {
//    for (int dx=-2 ; dx<=1 ; dx++)
  for (int dy=-sr ; dy<=sr ; dy++)
  {
    for (int dx=-sr ; dx<=sr ; dx++)
    {
      Shift(slog2, dx, dy, &tmp);
      fig4.Draw(tmp);
      unsigned char* p = tmp.BytePtr();
      xmm_xor(p1, p, po, num_doublequadwords);
      CountSlogDiffBits8x8(slogdiff, &bitcount);
      {
        for (int j=0 ; j<bitcount.Height() ; j++)
        {
          for (int i=0 ; i<bitcount.Width() ; i++)
          {
            if (bitcount(i, j) < minval(i, j))
            {
              (*doxx)(i, j) = dx;
              (*doyy)(i, j) = dy;
              minval(i, j) = bitcount(i, j);
            }
          }
        }
      }
      fig3.Draw(slogdiff);
      Multiply(bitcount, 4, &bitcount);
      fig5.Draw(bitcount);
//      fig3.GrabMouseClick();
    }
  }
  fig6.Draw(*doxx);
  fig7.Draw(*doyy);
}


/**
Decompress jpeg file from memory buffer.
- Needs jmemsrc.c file with orher jpeglib files
- Needs following declaration in jpeglib.h:
EXTERN(void) jpeg_memory_src JPP((j_decompress_ptr cinfo, const JOCTET* input, size_t bufsize)); 

  added by Neeraj Kanhere 11/3/09
*/
void LoadJpegFromMemory(const BYTE* jpeg_data, size_t jpeg_data_length, ImgBgr* out)
{
  //unsigned int width, height;
  //BYTE* rgb = JpegMemory::JpegMemoryToRGB(cap.pData, capStat->jpgsize, &width, &height);
  unsigned int width, height;
  //BYTE* data_ptr = JpegFile::JpegMemoryToRGB(jpeg_data, jpeg_data_length, &width, &height);
  BYTE* data_ptr = JpegFile::JpegMemoryToRGB(jpeg_data, jpeg_data_length, &width, &height);
  
  //Memory for data_ptr is created in function JpegFileToRGB. We must clean up memory later!!
  if (!data_ptr)  BLEPO_ERROR(StringEx("Unable to load JPEG data from memry"));
  BYTE* data_ptr_temp = data_ptr;
  out->Reset(width, height);
  ImgBgr::Iterator imgbgr_ptr;
  //Converting from RGB to BGR format
  for (imgbgr_ptr = out->Begin() ; imgbgr_ptr != out->End() ; imgbgr_ptr++)
  {
    imgbgr_ptr->r = *data_ptr++;
    imgbgr_ptr->g = *data_ptr++;
    imgbgr_ptr->b = *data_ptr++;
  }
  delete[] data_ptr_temp; 
}



size_t SaveJpegToMemory(const ImgBgr& img, BYTE* write_buffer, bool save_as_bgr, int quality)
{
  BYTE* data_ptr;
  data_ptr = (BYTE *)new BYTE[img.Width() * 3 * img.Height()];
  assert(data_ptr);
  BYTE* data_ptr_temp = data_ptr;
  ImgBgr::ConstIterator imgbgr_ptr;
  //Converting from BGR to RGB format.
  for (imgbgr_ptr = img.Begin() ; imgbgr_ptr != img.End() ; imgbgr_ptr++)
  {
    *data_ptr++ = imgbgr_ptr->r;
    *data_ptr++ = imgbgr_ptr->g;
    *data_ptr++ = imgbgr_ptr->b;
  }
  
  //size_t nbytes =JpegFile::RGBToJpegMemory(data_ptr_temp,write_buffer, img.Width(),img.Height(),save_as_bgr);
  size_t nbytes = JpegFile::RGBToJpegMemory(data_ptr_temp,write_buffer, img.Width(),img.Height(),save_as_bgr, quality);
  assert(nbytes);
  
  delete[] data_ptr_temp;

  return nbytes;
}


// Loads an image (various filetypes) using OpenCV
// Not used currently
void LoadOpenCV(const char* fname, ImgBgr* out)
{
  // Load image into local variable, then copy it and destroy the local variable
  // (I know this is silly, but OpenCV and IPL are conflicting when I free the memory
  // if I try to use them interchangeably -- STB)
  IplImage* img = cvLoadImage(fname,1);
  assert(img);
  assert(img->nChannels == 3);
  assert(img->depth == IPL_DEPTH_8U);
  assert(strncmp(img->colorModel, "RGB", 4)==0);
  assert(strncmp(img->channelSeq, "BGR", 4)==0);
  assert(img->dataOrder == IPL_DATA_ORDER_PIXEL);
  assert(img->origin == IPL_ORIGIN_TL);
  assert(img->align == IPL_ALIGN_QWORD || img->width*3==img->widthStep);
  assert(img->imageDataOrigin == img->imageData);

  if(     (img != NULL) 
      &&  (img->nChannels == 3) 
      &&  (img->depth == IPL_DEPTH_8U)
      &&  (strncmp(img->colorModel, "RGB", 4)==0)
      &&  (strncmp(img->channelSeq, "BGR", 4)==0)
      &&  (img->dataOrder == IPL_DATA_ORDER_PIXEL)
      &&  (img->origin == IPL_ORIGIN_TL)
      &&  (img->align == IPL_ALIGN_QWORD || img->width*3==img->widthStep)
      &&  (img->imageDataOrigin == img->imageData)
    )
  {
    // Copy header and data row-by-row, since OpenCV's struct may
    // have extra alignment bytes at the end of the rows
    out->Reset(img->width, img->height);
    for (int y=0 ; y<img->height ; y++)
    {
      void* pi = img->imageData + y * img->widthStep;
      ImgBgr::Iterator po = out->Begin(0, y);
      memcpy(po, pi, out->Width()*sizeof(ImgBgr::Pixel));
    }

    // Release temporary IplImage
    cvReleaseImage(&img);
  }
  else
  {
    out->Reset(0,0);
  }
}

// Saves an image (various filetypes) using OpenCV.
// Allows for additional image types, such as PNG and GIF
// Not used currently
void SaveOpenCV(const ImgBgr& in, const char* fname)
{
  CvSize sz;
  sz.width = in.Width();
  sz.height = in.Height();
  IplImage* img = cvCreateImage(sz, IPL_DEPTH_8U, 3);
  assert(strncmp(img->colorModel, "RGB", 4)==0);
  assert(strncmp(img->channelSeq, "BGR", 4)==0);

  // Copy header and data row-by-row, since OpenCV's struct may
  // have extra alignment bytes at the end of the rows
  for (int y=0 ; y<in.Height() ; y++)
  {
    ImgBgr::ConstIterator pi = in.Begin(0, y);
    void* po = img->imageData + y * img->widthStep;
    memcpy(po, pi, in.Width()*sizeof(ImgBgr::Pixel));
  }
  cvSaveImage(fname, img);

  // Release temporary IplImage
  cvReleaseImage(&img);
}

/**
  Load/Save functions.
  @author Prashant Oswal
*/

void Load(const CString& fname, ImgBgr* out, bool use_opencv)
{
  if(use_opencv)
  {
    CStringA fnamea;
    fnamea = fname;
    LoadOpenCV(fnamea, out);
  }
  else
  {
    const char char_bmp[] = "BM";
    const char char_pgm[] = "P5";
    const char char_ppm[] = "P6";
    const char char_pgm_ascii[] = "P2";
    const char char_ppm_ascii[] = "P3";
    // Some Jpeg Files start with hex FF D8 FF E0.
    // Some Jpeg Files start with hex FF D8 FF E1.
    // According to http://en.wikipedia.org/wiki/Magic_number_(programming),
    //    there are just three bytes in the JPEG magic number.
    const char char_jpg1[5] = {'\xFF','\xD8','\xFF','\xE0','\0'};
    const char char_jpg2[5] = {'\xFF','\xD8','\xFF','\xE1','\0'};
    char data[5];

    {  // Determine file type
      FILE* fp;
      long result;
      fp = _wfopen(fname, L"rb");
      if (fp==NULL)  BLEPO_ERROR(StringEx("Unable to open file '%s'", fname));
      result=fread(data,1,4,fp); //read magic characters
      if (result==0)  BLEPO_ERROR(StringEx("Error reading file '%s'", fname));
      data[4]='\0';
      fclose(fp);
    }
	  if (_strnicmp(data,char_pgm,strlen(char_pgm))==0)
    {
		  ImgGray out_gray;
		  iLoadPgm(fname,&out_gray);
		  Convert(out_gray,out);
    }
	  else if (_strnicmp(data,char_ppm,strlen(char_ppm))==0)
    {
		  iLoadPpm(fname,out);
    }
	  else if (_strnicmp(data,char_bmp,strlen(char_bmp))==0)
    {
		  iLoadBmp(fname,out);
    }
    else if (_strnicmp(data,char_jpg1,strlen(char_jpg1))==0
          || _strnicmp(data,char_jpg2,strlen(char_jpg2))==0)
    {
		  iLoadJpeg(fname,out);
    }
	  else if (_strnicmp(data,char_pgm_ascii,strlen(char_pgm_ascii))==0)
    {
		  BLEPO_ERROR(StringEx("Sorry, only binary PGM files are supported.  File '%s' is ASCII.", fname));
    }
	  else if (_strnicmp(data,char_ppm_ascii,strlen(char_ppm_ascii))==0)
    {
		  BLEPO_ERROR(StringEx("Sorry, only binary PPM files are supported.  File '%s' is ASCII.", fname));
    }
	  else
    {
		  BLEPO_ERROR(StringEx("File '%s' is an unsupported image type", fname));
    }
  }
}



void Save(const ImgBgr& img, const CString& fname, const char* filetype)
{
  CStringA fnamea;
  fnamea = fname;
  if ((filetype==NULL) || strlen(filetype)==0)
  {
    const char* p = strrchr(fnamea, '.');
    if (p == NULL)
    {
      BLEPO_ERROR("Filename has no extension");
    }
    filetype = p+1;
  }

  if(_stricmp("pgm",filetype)==0)
  {
    ImgGray img_gray;
    Convert(img, &img_gray);
    iSavePgm(img_gray, fname);
  }
  else if(_stricmp("ppm",filetype)==0)
  {
    iSavePpm(img,fname);
  }
  else if(_stricmp("bmp",filetype)==0)
  {
    iSaveBmpRgb(img,fname);
  }
  else if((_stricmp("jpg",filetype)==0) || (strcmp("jpeg",filetype)==0))
  {
    iSaveJpeg(img,fname,true);
  }
  else if(_stricmp("eps",filetype)==0)
  {
    iSaveEpsBgr(img, fname);
  }
  else
  {
    BLEPO_ERROR("Filetype not supported");
  }
}

void SaveAsText(const ImgFloat& src, const CString& fname, const char* fmt)
{
  const char* eol = "\r\n";
  FILE* fp = _wfopen(fname, L"wb");
  fprintf(fp, "%d %d%s", src.Width(), src.Height(), eol); 
  ImgFloat::ConstIterator p = src.Begin();
  for (int y=0 ; y<src.Height() ; y++)
  {
    for (int x=0 ; x<src.Width() ; x++)
    {
      float f = *p++;
      fprintf(fp, fmt, f);
    }
    fprintf(fp, "%s", eol);
  }
  fclose(fp);
}

void Load(const CString& fname, ImgGray* out)
{
  const char char_bmp[] = "BM";
  const char char_pgm[] = "P5";
  const char char_ppm[] = "P6";
  // Some Jpeg Files start with hex FF D8 FF E0
  const char char_jpg1[5] = {'\xFF','\xD8','\xFF','\xE0','\0'};
  // Some Jpeg Files start with hex FF D8 FF E1
  const char char_jpg2[5] = {'\xFF','\xD8','\xFF','\xE1','\0'};
  char data[5];

  {  // Determine file type
    FILE* fp;
    long result;
    fp = _wfopen(fname, L"rb");
    if (fp==NULL)  BLEPO_ERROR(StringEx("Unable to open file '%s'", fname));
    result=fread(data,1,4,fp); //read magic characters
    if (result==0)  BLEPO_ERROR("Error reading file");
    data[4]='\0';
    fclose(fp);
  }
  if (_strnicmp(data,char_pgm,strlen(char_pgm))==0)
  {
	  iLoadPgm(fname,out);
  }
  else if (_strnicmp(data,char_ppm,strlen(char_ppm))==0)
  {
	  ImgBgr out_bgr;	
	  iLoadPpm(fname,&out_bgr);
	  Convert(out_bgr,out);
  }
  else if (_strnicmp(data,char_bmp,strlen(char_bmp))==0)
  {
	  ImgBgr out_bgr;
    iLoadBmp(fname,&out_bgr);
    Convert(out_bgr,out);
  }
  else if (_strnicmp(data,char_jpg1,strlen(char_jpg1))==0
        || _strnicmp(data,char_jpg2,strlen(char_jpg2))==0)
  {
    ImgBgr out_bgr;
	  iLoadJpeg(fname,&out_bgr);
	  Convert(out_bgr,out);
  }
  else
  {
  	BLEPO_ERROR("Filetype not supported");
  }
}

void LoadImgInt(const CString& fname, ImgInt* out, bool binary)
{
	FILE *fp;
	if(binary)
		fp = _wfopen(fname, L"rb");
	else
		fp = _wfopen(fname, L"r");
	int width = 640, height = 480; //If messed up, just assume
	if(binary) {
		fread(&width,sizeof(int),1,fp);
		fread(&height,sizeof(int),1,fp);
		out->Reset(width,height);
		ImgInt::Iterator p = out->Begin();
		fread(p,sizeof(int),width*height,fp);
	} else {
		//fscanf(fp,"%i,%i,",&width,&height);
		out->Reset(width,height);
		ImgInt::Iterator p = out->Begin();
		while(p != out->End()) {
			fscanf(fp,"%i",p);
			p++;
		}
	}
	fclose(fp);
}

void Save(const ImgGray& img, const CString& fname, const char* filetype)
{
  if ((filetype==NULL) || strlen(filetype)==0)
  {
    CStringA fnamea;
    fnamea = fname;
    const char* p = strrchr(fnamea, '.');
    if (p == NULL)
    {
      BLEPO_ERROR("Filename has no extension");
    }
    filetype = p+1;
  }

  if(_stricmp("pgm",filetype)==0)
  {
    iSavePgm(img,fname);
  }
  else if(_stricmp("ppm",filetype)==0)
  {
    ImgBgr img_bgr;
    Convert(img,&img_bgr);
    iSavePpm(img_bgr,fname);
  }
  else if(_stricmp("bmp",filetype)==0)
  {
    iSaveBmpGray(img,fname);
  }
  else if((_stricmp("jpg",filetype)==0) || (strcmp("jpeg",filetype)==0))
  {
    ImgBgr img_bgr;
    Convert(img,&img_bgr);
    iSaveJpeg(img_bgr,fname,false);
  }
  else if(_stricmp("eps",filetype)==0)
  {
    iSaveEpsGray(img,fname);
  }
  else
  {
    BLEPO_ERROR("Filetype not supported");
  }
}

void Save(const ImgBinary& img, const CString& fname, const char* filetype)
{
  ImgGray gimg;
  Convert(img, &gimg, 0, 255);
  Save(gimg, fname, filetype);
}

//Save as .dep file for data
void SaveImgInt(const ImgInt& img, const CString& fname, bool binary)
{
	FILE *fp;
	if(binary)
		fp = _wfopen(fname, L"wb");
	else
		fp = _wfopen(fname, L"w");
	ImgInt::ConstIterator p = img.Begin();
	int width = img.Width(), height = img.Height();
	if(binary) {
		fwrite(&width,sizeof(int),1,fp);
		fwrite(&height,sizeof(int),1,fp);
		fwrite(p,sizeof(int),img.Width() * img.Height(),fp);
	} else {
		fprintf(fp,"%i,%i,",width,height);
		while(p != img.End())
			fprintf(fp,"%i,",*p++);
	}
	fclose(fp);
}


void AffineFit(const Array<Point2d>& pt1, const Array<Point2d>& pt2, MatDbl* out)
{
  assert(pt1.Len() == pt2.Len());
  assert(pt1.Len() >= 3);
  const int n = pt1.Len();
  out->Reset(3, 2);

  // Set up matrix and vector
  MatDbl mat(6, 2*n);
  MatDbl vec(1, 2*n);
  const Point2d* p1 = pt1.Begin();
  const Point2d* p2 = pt2.Begin();
  double* p = mat.Begin();
  double* pv = vec.Begin();
  for ( ; p1 != pt1.End() ; p1++, p2++)
  {
    // even row of 'mat'
    *p++ = p1->x;
    *p++ = p1->y;
    *p++ = 1;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    // odd row of 'mat'
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = p1->x;
    *p++ = p1->y;
    *p++ = 1;
    // even and odd rows of 'vec'
    *pv++ = p2->x;
    *pv++ = p2->y;
  }

  // Solve equation
  SolveLinear(mat, vec, out);
  out->Reshape(3, 2);
}

void WeightedAffineFit(
  const Array<Point2d>& pt1, 
  const Array<Point2d>& pt2, 
  const Array<double>& weights,
  MatDbl* out)
{
  assert(pt1.Len() == pt2.Len());
  assert(pt1.Len() == weights.Len());
  assert(pt1.Len() >= 3);
  const int n = pt1.Len();
  out->Reset(3, 2);

  // Set up matrix and vector
  MatDbl mat(6, 2*n);
  MatDbl vec(1, 2*n);
  const Point2d* p1 = pt1.Begin();
  const Point2d* p2 = pt2.Begin();
  double* p  = mat.Begin();
  double* pv = vec.Begin();
  const double* w  = weights.Begin();
  for ( ; p1 != pt1.End() ; p1++, p2++, w++)
  {
    // even row of 'mat'
    *p++ = (p1->x) * (*w);
    *p++ = (p1->y) * (*w);
    *p++ = (1) * (*w);
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    // odd row of 'mat'
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = (p1->x) * (*w);
    *p++ = (p1->y) * (*w);
    *p++ = (1) * (*w);
    // even and odd rows of 'vec'
    *pv++ = (p2->x) * (*w);
    *pv++ = (p2->y) * (*w);
  }

  // Solve equation
  SolveLinear(mat, vec, out);
  out->Reshape(3, 2);
}

void TransScaleFit(const std::vector<Point2d>& pt1, const std::vector<Point2d>& pt2, 
                   double* zoom, double* dx, double* dy)
{
  assert(pt1.size() == pt2.size());
  assert(pt1.size() >= 2);
  const int n = pt1.size();

  // Set up matrix and vector
  MatDbl mat(3, 2*n);
  MatDbl vec(1, 2*n);
  MatDbl out;
  std::vector<Point2d>::const_iterator p1 = pt1.begin();
  std::vector<Point2d>::const_iterator p2 = pt2.begin();
  double* p = mat.Begin();
  double* pv = vec.Begin();
  for ( ; p1 != pt1.end() ; p1++, p2++)
  {
    // even row of 'mat'
    *p++ = p1->x;
    *p++ = 1;
    *p++ = 0;
    // odd row of 'mat'
    *p++ = p1->y;
    *p++ = 0;
    *p++ = 1;
    // even and odd rows of 'vec'
    *pv++ = p2->x;
    *pv++ = p2->y;
  }

  // Solve equation
  SolveLinear(mat, vec, &out);
  assert( out.Width() == 1 && out.Height() == 3 );
  *zoom = out(0);
  *dx = out(1);
  *dy = out(2);
}

void AffineFit(const std::vector<Point2d>& pt1, const std::vector<Point2d>& pt2, MatDbl* out)
{
  assert(pt1.size() == pt2.size());
  assert(pt1.size() >= 3);
  const int n = pt1.size();
  out->Reset(3, 2);

  // Set up matrix and vector
  MatDbl mat(6, 2*n);
  MatDbl vec(1, 2*n);
//  const Point2d* p1 = pt1.begin();
//  const Point2d* p2 = pt2.begin();
  std::vector<Point2d>::const_iterator p1 = pt1.begin();
  std::vector<Point2d>::const_iterator p2 = pt2.begin();
  double* p = mat.Begin();
  double* pv = vec.Begin();
  for ( ; p1 != pt1.end() ; p1++, p2++)
  {
    // even row of 'mat'
    *p++ = p1->x;
    *p++ = p1->y;
    *p++ = 1;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    // odd row of 'mat'
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = p1->x;
    *p++ = p1->y;
    *p++ = 1;
    // even and odd rows of 'vec'
    *pv++ = p2->x;
    *pv++ = p2->y;
  }

  // Solve equation
  SolveLinear(mat, vec, out);
  out->Reshape(3, 2);
}

void WeightedAffineFit(
  const std::vector<Point2d>& pt1, 
  const std::vector<Point2d>& pt2, 
  const std::vector<double>& weights,
  MatDbl* out)
{
  assert(pt1.size() == pt2.size());
  assert(pt1.size() == weights.size());
  assert(pt1.size() >= 3);
  const int n = pt1.size();
  out->Reset(3, 2);

  // Set up matrix and vector
  MatDbl mat(6, 2*n);
  MatDbl vec(1, 2*n);
//  const Point2d* p1 = pt1.begin();
//  const Point2d* p2 = pt2.begin();
  std::vector<Point2d>::const_iterator p1 = pt1.begin();
  std::vector<Point2d>::const_iterator p2 = pt2.begin();
  double* p  = mat.Begin();
  double* pv = vec.Begin();
//  const double* w  = weights.begin();
  std::vector<double>::const_iterator w = weights.begin();
  for ( ; p1 != pt1.end() ; p1++, p2++, w++)
  {
    // even row of 'mat'
    *p++ = (p1->x) * (*w);
    *p++ = (p1->y) * (*w);
    *p++ = (1) * (*w);
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    // odd row of 'mat'
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = (p1->x) * (*w);
    *p++ = (p1->y) * (*w);
    *p++ = (1) * (*w);
    // even and odd rows of 'vec'
    *pv++ = (p2->x) * (*w);
    *pv++ = (p2->y) * (*w);
  }

  // Solve equation
  SolveLinear(mat, vec, out);
  out->Reshape(3, 2);
}

void HomographyFit(const std::vector<Point2d>& pt1, const std::vector<Point2d>& pt2, MatDbl* out)
{
  assert(pt1.size() == pt2.size());
  assert(pt1.size() >= 3);
  const int n = pt1.size();
  out->Reset(3, 3);

  // Set up matrices
  CvMat* src = cvCreateMat(n,2,CV_64F);
  CvMat* dst = cvCreateMat(n,2,CV_64F);
  CvMat* end = cvCreateMat(3,3,CV_64F);
//  const Point2d* p1 = pt1.begin();
//  const Point2d* p2 = pt2.begin();
  std::vector<Point2d>::const_iterator p1 = pt1.begin();
  std::vector<Point2d>::const_iterator p2 = pt2.begin();
  int row=0;

  // Convert points to CvMat matrices
  for ( ; p1 != pt1.end() ; p1++, p2++)
  {
    cvmSet(src,row,0,p1->x);
    cvmSet(src,row,1,p1->y);
    cvmSet(dst,row,0,p2->x);
    cvmSet(dst,row,1,p2->y);
    row++;
  }

  // Solve equation
  cvFindHomography(src, dst, end);

  // Convert homography result to MatDbl matrix
  double* p = out->Begin();
  for (row=0 ; row<3 ; row++)
  {
    // copy value from "end" to "mat"
    *p++ = cvmGet(end,row,0);
    *p++ = cvmGet(end,row,1);
    *p++ = cvmGet(end,row,2);
  }

  // Release CvMat matrices
  cvReleaseMat(&src);
  cvReleaseMat(&dst);
  cvReleaseMat(&end);
}

void HomographyFit(const std::vector<CPoint>& pt1, const std::vector<CPoint>& pt2, MatDbl* out)
{
	assert(pt1.size() == pt2.size());
	assert(pt1.size() >= 3);
	const int n = pt1.size();
	out->Reset(3, 3);
	
	// Set up matrices
	CvMat* src = cvCreateMat(n,2,CV_64F);
	CvMat* dst = cvCreateMat(n,2,CV_64F);
	CvMat* end = cvCreateMat(3,3,CV_64F);
	//  const Point2d* p1 = pt1.begin();
	//  const Point2d* p2 = pt2.begin();
	std::vector<CPoint>::const_iterator p1 = pt1.begin();
	std::vector<CPoint>::const_iterator p2 = pt2.begin();
	int row=0;
	
	// Convert points to CvMat matrices
	for ( ; p1 != pt1.end() ; p1++, p2++)
	{
		cvmSet(src,row,0,p1->x);
		cvmSet(src,row,1,p1->y);
		cvmSet(dst,row,0,p2->x);
		cvmSet(dst,row,1,p2->y);
		row++;
	}
	
	// Solve equation
	cvFindHomography(src, dst, end);
	
	// Convert homography result to MatDbl matrix
	double* p = out->Begin();
	for (row=0 ; row<3 ; row++)
	{
		// copy value from "end" to "mat"
		*p++ = cvmGet(end,row,0);
		*p++ = cvmGet(end,row,1);
		*p++ = cvmGet(end,row,2);
	}
	
	// Release CvMat matrices
	cvReleaseMat(&src);
	cvReleaseMat(&dst);
	cvReleaseMat(&end);
}

void ComputeAffine(double dx1, double dy1, double theta, double sx, double sy, double a, double b, double dx2, double dy2, MatDbl* out)
{
  out->Reset(3, 2);
  double* p = out->Begin();
  double st = sin(theta);
  double ct = cos(theta);
  p[0] = sx * ct + a * sy * st;
  p[1] = -sx * st + a * sy * ct;
  p[3] = b * sx * ct + sy * st;
  p[4] = -b * sx * st + sy * ct;
  p[2] = p[0]*dx1 + p[1]*dy1 + dx2;
  p[5] = p[3]*dx1 + p[4]*dy1 + dy2;
}

void AffineMultiply(const MatDbl& aff, const Array<Point2d>& pt1, Array<Point2d>* pt2)
{
  pt2->Reset( pt1.Len() );
  const double *a = aff.Begin();
  const Point2d* p1 = pt1.Begin();
  Point2d* p2 = pt2->Begin();
  for ( ; p1 != pt1.End() ; p1++, p2++)
  {
    p2->x = a[0] * p1->x + a[1] * p1->y + a[2];
    p2->y = a[3] * p1->x + a[4] * p1->y + a[5];
  }
}

void InitWarpAffine(const MatDbl& a, int width, int height, ImgFloat* fx, ImgFloat* fy)
{
  assert(a.Width()==3 && a.Height()==2);
  fx->Reset(width, height);
  fy->Reset(width, height);
  double a00 = a(0,0);
  double a10 = a(1,0);
  double a20 = a(2,0);
  double a01 = a(0,1);
  double a11 = a(1,1);
  double a21 = a(2,1);
  float* px = fx->Begin();
  float* py = fy->Begin();
  for (int y=0 ; y<height ; y++)
  {
    for (int x=0; x<width ; x++)
    {
      *px++ = static_cast<float>( a00 * x + a10 * y + a20 );
      *py++ = static_cast<float>( a01 * x + a11 * y + a21 );
    }
  }
}

void InitWarpTransScale(double zoom, double dx, double dy, int width, int height, ImgFloat* fx, ImgFloat* fy)
{
  fx->Reset(width, height);
  fy->Reset(width, height);
  float* px = fx->Begin();
  float* py = fy->Begin();
  for (int y=0 ; y<height ; y++)
  {
    for (int x=0; x<width ; x++)
    {
      *px++ = static_cast<float>( zoom * x + dx );
      *py++ = static_cast<float>( zoom * y + dy );
    }
  }
}

void InitWarpTransScaleCenter(double zoom, double xc, double yc, int width, int height, ImgFloat* fx, ImgFloat* fy)
{
  fx->Reset(width, height);
  fy->Reset(width, height);
  float* px = fx->Begin();
  float* py = fy->Begin();
  const int hw = width/2;
  const int hh = height/2;
  for (int y=0 ; y<height ; y++)
  {
    for (int x=0; x<width ; x++)
    {
      *px++ = static_cast<float>( zoom * (xc - hw + x) );
      *py++ = static_cast<float>( zoom * (yc - hh + y) );
    }
  }
}

void InitWarpTransRotCenter(double theta, double xc, double yc, double tx, double ty, int width, int height, ImgFloat* fx, ImgFloat* fy)
{
  fx->Reset(width, height);
  fy->Reset(width, height);
  float* px = fx->Begin();
  float* py = fy->Begin();

  MatDbl r(2,2), c(1,2), t(1,2), tt;
  r(0,0) = cos(theta);  r(1,0) = -sin(theta);
  r(0,1) = sin(theta);  r(1,1) = cos(theta);
  c(0) = xc;
  c(1) = yc;
  t(0) = tx;
  t(1) = ty;
  tt = -r * c + c + t;

  for (int y=0 ; y<height ; y++)
  {
    for (int x=0; x<width ; x++)
    {
      *px++ = static_cast<float>( r(0,0) * x + r(1,0) * y + tt(0) );
      *py++ = static_cast<float>( r(0,1) * x + r(1,1) * y + tt(1) );
    }
  }
}



void InitWarpHomography(const MatDbl& a, int width, int height, ImgFloat* fx, ImgFloat* fy)
{
  assert(a.Width()==3 && a.Height()==3);
  fx->Reset(width, height);
  fy->Reset(width, height);
  double a00 = a(0,0);
  double a10 = a(1,0);
  double a20 = a(2,0);
  double a01 = a(0,1);
  double a11 = a(1,1);
  double a21 = a(2,1);
  double a02 = a(0,2);
  double a12 = a(1,2);
  double a22 = a(2,2);
  float* px = fx->Begin();
  float* py = fy->Begin();
  double alpha;
  for (int y=0 ; y<height ; y++)
  {
    for (int x=0; x<width ; x++)
    {
      alpha = 1 / (a02 * x + a12 * y + a22);
      *px++ = static_cast<float>(( a00 * x + a10 * y + a20 ) * alpha);
      *py++ = static_cast<float>(( a01 * x + a11 * y + a21 ) * alpha);
    }
  }
}

template <typename T>
void Warp(const Image<T>& img, const ImgFloat& fx, const ImgFloat& fy, Image<T>* out)
{
  assert(IsSameSize(fx, fy));
  assert(out != &img);
// old:  InPlaceSwapper< Image<T> > inplace(img, &out);
  out->Reset(fx.Width(), fx.Height());
  ImgFloat::ConstIterator qx = fx.Begin();
  ImgFloat::ConstIterator qy = fy.Begin();
  Image<T>::Iterator q = out->Begin();
  while (q != out->End())  *q++ = Interp(img, *qx++, *qy++);
//  while (q != out->End())  *q++ = img(*qx++, *qy++); // faster for now!!!
}

template void Warp(const ImgBgr   & img, const ImgFloat& x, const ImgFloat& y, ImgBgr* out);
template void Warp(const ImgBinary& img, const ImgFloat& x, const ImgFloat& y, ImgBinary* out);
template void Warp(const ImgFloat & img, const ImgFloat& x, const ImgFloat& y, ImgFloat* out);
template void Warp(const ImgGray  & img, const ImgFloat& x, const ImgFloat& y, ImgGray* out);
template void Warp(const ImgInt   & img, const ImgFloat& x, const ImgFloat& y, ImgInt* out);

void ComputeIntegralImage(const ImgInt& img, ImgInt* out)
{
  int w = img.Width();
  int h = img.Height();
  ImgInt::ConstIterator p;
  ImgInt::Iterator q;
  int x, y;

  out->Reset(w, h);

  // first pixel (top-left corner)
  p = img.Begin();
  q = out->Begin();
  *q++ = *p++;

  // first row
  for (x=1 ; x<w ; x++)
  {
    *q = *p + q[-1];
    p++;
    q++;
  }

  // first column
  for (y=1 ; y<h ; y++)
  {
    *q = *p + q[-w];
    p += w;
    q += w;
  }

  // rest of image
  p = img.Begin(1, 1);
  q = out->Begin(1, 1);
  for (y=1 ; y<h ; y++)
  {
    for (x=1 ; x<w ; x++)
    {
      // equation:  S(x,y) = I(x,y) + S(x-1,y) + S(x,y-1) - S(x-1,y-1)
      *q = *p + q[-1] + q[-w] - q[-1-w];
      p++;
      q++;
    }
    p++;  // skip first column
    q++;
  }
}



void ComputeIntegralImage(const ImgBinary& img, ImgInt* out)
{
  int w = img.Width();
  int h = img.Height();
  ImgBinary::ConstIterator p;
  ImgInt::Iterator q;
  int x, y;

  out->Reset(w, h);

  // first pixel (top-left corner)
  p = img.Begin();
  q = out->Begin();
  *q++ = *p++;

  // first row
  for (x=1 ; x<w ; x++)
  {
    *q = *p + q[-1];
    p++;
    q++;
  }

  // first column
  for (y=1 ; y<h ; y++)
  {
    *q = *p + q[-w];
    p += w;
    q += w;
  }

  // rest of image
  p = img.Begin(1, 1);
  q = out->Begin(1, 1);
  for (y=1 ; y<h ; y++)
  {
    for (x=1 ; x<w ; x++)
    {
      // equation:  S(x,y) = I(x,y) + S(x-1,y) + S(x,y-1) - S(x-1,y-1)
      *q = *p + q[-1] + q[-w] - q[-1-w];
      p++;
      q++;
    }
    p++;  // skip first column
    q++;
  }
}

void ComputeIntegralImage(const ImgGray& img, ImgInt* out)
{
  int w = img.Width();
  int h = img.Height();
  ImgGray::ConstIterator p;
  ImgInt::Iterator q;
  int x, y;

  out->Reset(w, h);

  // first pixel (top-left corner)
  p = img.Begin();
  q = out->Begin();
  *q++ = *p++;

  // first row
  for (x=1 ; x<w ; x++)
  {
    *q = *p + q[-1];
    p++;
    q++;
  }

  // first column
  for (y=1 ; y<h ; y++)
  {
    *q = *p + q[-w];
    p += w;
    q += w;
  }

  // rest of image
  p = img.Begin(1, 1);
  q = out->Begin(1, 1);
  for (y=1 ; y<h ; y++)
  {
    for (x=1 ; x<w ; x++)
    {
      // equation:  S(x,y) = I(x,y) + S(x-1,y) + S(x,y-1) - S(x-1,y-1)
      *q = *p + q[-1] + q[-w] - q[-1-w];
      p++;
      q++;
    }
    p++;  // skip first column
    q++;
  }
}

int UseIntegralImage(const ImgInt& ii, const Rect& rect)
{
  assert(rect.left >= 0 && rect.top >= 0 && rect.right <= ii.Width() && rect.bottom <= ii.Height());
  const int f = rect.left   - 1;
  const int r = rect.right  - 1;
  const int t = rect.top    - 1;
  const int b = rect.bottom - 1;
  int val = ii( r, b );
  if (f >= 0)  val -= ii( f, b );
  if (t >= 0)  val -= ii( r, t );
  if (f >= 0 && t >= 0)  val += ii( f, t );
  return val; 
}

void LocalMaxima(const ImgInt& img, ImgBinary* out)
{
  out->Reset(img.Width(), img.Height());
  Set(out, 0);
  for (int y=1 ; y<img.Height()-1 ; y++)
  {
    for (int x=1 ; x<img.Width()-1 ; x++)
    {
      int pix = img(x, y);
      if ( pix >= img(x-1, y)
        && pix >= img(x+1, y) 
        && pix >= img(x, y-1)
        && pix >= img(x, y+1)) 
        (*out)(x, y) = 1;
    }
  }
}

/**
  FFT functions.
  uses FFTW (www.fftw.org)
  @author Prashant Oswal
*/

class FftEngine
{
public:
  FftEngine(int width, int height): m_free(1)
  {
    Reset(width, height);
  }
  
  ~FftEngine()
  {
    if(m_free == 0)
    {
      fftwf_destroy_plan(plan_fwd);
      fftwf_destroy_plan(plan_bwd);
      fftwf_free(in);
      fftwf_free(out);
    }
  }
  
  void Reset(int width, int height)
  {
    if (m_free == 0)
    {
      fftwf_destroy_plan(plan_fwd);
      fftwf_destroy_plan(plan_bwd);
      fftwf_free(in);
      fftwf_free(out);
    }
    m_width = width;
    m_height = height;
    in = (fftwf_complex *) fftwf_malloc(sizeof(fftwf_complex)*height*width);
    out = (fftwf_complex *) fftwf_malloc(sizeof(fftwf_complex)*height*width);
    plan_fwd = fftwf_plan_dft_2d(height,width, in, out, FFTW_FORWARD,FFTW_ESTIMATE);
    plan_bwd = fftwf_plan_dft_2d(height,width, in, out, FFTW_BACKWARD,FFTW_ESTIMATE);
    m_free = 0;
  }

  void ForwardFft(const ImgFloat& in_img, ImgFloat* out_img_real, ImgFloat* out_img_imag)
  {
    ImgFloat::ConstIterator in_ptr;
    ImgFloat::Iterator out_real_ptr;
    ImgFloat::Iterator out_imag_ptr;
    int width = in_img.Width();
    int height = in_img.Height();
    int i;
    
    if (width != m_width || height != m_height)
    {
      Reset(width,height);
    }
    
    out_img_real->Reset(width,height);
    out_img_imag->Reset(width,height);

    in_ptr=in_img.Begin();
    for(i=0; in_ptr!= in_img.End(); i++)
    {
      in[i][0] = *in_ptr++;
      in[i][1] = 0.0;
    }  

    fftwf_execute(plan_fwd);

    out_real_ptr = out_img_real->Begin();
    out_imag_ptr = out_img_imag->Begin();

    for(i=0; out_real_ptr != out_img_real->End(); i++)
    {
      *out_real_ptr++ = out[i][0];
      *out_imag_ptr++ = out[i][1];
    }

  }
  
  void ForwardFft(const ImgFloat& in_img_real,const ImgFloat& in_img_imag, ImgFloat* out_img_real, ImgFloat* out_img_imag)
  {
    if((in_img_real.Width() != in_img_imag.Width()) || (in_img_real.Height() != in_img_imag.Height()))
    {
      BLEPO_ERROR ("The real and imaginary parts of input must have same dimensions");
    }
    ImgFloat::ConstIterator in_real_ptr;
    ImgFloat::ConstIterator in_imag_ptr;
    ImgFloat::Iterator out_real_ptr;
    ImgFloat::Iterator out_imag_ptr;
    int width = in_img_real.Width();
    int height = in_img_real.Height();
    int i;
    
    if (width != m_width || height != m_height)
    {
      Reset(width,height);
    }
    
    out_img_real->Reset(width,height);
    out_img_imag->Reset(width,height);
    
    in_real_ptr=in_img_real.Begin();
    in_imag_ptr=in_img_imag.Begin();
    for(i=0; in_real_ptr!= in_img_real.End(); i++)
    {
      in[i][0] = *in_real_ptr++;
      in[i][1] = *in_imag_ptr++;;
    }  

    fftwf_execute(plan_fwd);

    out_real_ptr = out_img_real->Begin();
    out_imag_ptr = out_img_imag->Begin();

    for(i=0; out_real_ptr != out_img_real->End(); i++)
    {
      *out_real_ptr++ = out[i][0];
      *out_imag_ptr++ = out[i][1];
    }
  }
  
  void InverseFFT(const ImgFloat& in_img_real,const ImgFloat& in_img_imag, ImgFloat* out_img_real, ImgFloat* out_img_imag)
  {
    if((in_img_real.Width() != in_img_imag.Width()) || (in_img_real.Height() != in_img_imag.Height()))
    {
      BLEPO_ERROR ("The real and imaginary parts of input must have same dimensions");
    }
    ImgFloat::ConstIterator in_real_ptr;
    ImgFloat::ConstIterator in_imag_ptr;
    ImgFloat::Iterator out_real_ptr;
    ImgFloat::Iterator out_imag_ptr;
    int width = in_img_real.Width();
    int height = in_img_real.Height();
    int i;
    
    if (width != m_width || height != m_height)
    {
      Reset(width,height);
    }
    
    out_img_real->Reset(width,height);
    out_img_imag->Reset(width,height);
    
    in_real_ptr=in_img_real.Begin();
    in_imag_ptr=in_img_imag.Begin();
    for(i=0; in_real_ptr!= in_img_real.End(); i++)
    {
      in[i][0] = *in_real_ptr++;
      in[i][1] = *in_imag_ptr++;;
    }  

    fftwf_execute(plan_bwd);

    out_real_ptr = out_img_real->Begin();
    out_imag_ptr = out_img_imag->Begin();

    for(i=0; out_real_ptr != out_img_real->End(); i++)
    {
      *out_real_ptr++ = out[i][0];
      *out_imag_ptr++ = out[i][1];
    }
  }

private:
  fftwf_plan plan_fwd;
  fftwf_plan plan_bwd;
  fftwf_complex *in;
  fftwf_complex *out;
  int m_free;
  int m_width;
  int m_height;
};

void ComputeFFT(const ImgFloat& in_img, ImgFloat* out_img_real, ImgFloat* out_img_imag)
{
  FftEngine fft_engine(in_img.Width(),in_img.Height());
  fft_engine.ForwardFft(in_img,out_img_real,out_img_imag);
}

void ComputeFFT(const ImgFloat& in_img_real,const ImgFloat& in_img_imag, ImgFloat* out_img_real, ImgFloat* out_img_imag)
{
  FftEngine fft_engine(in_img_real.Width(),in_img_real.Height());
  fft_engine.ForwardFft(in_img_real,in_img_imag,out_img_real,out_img_imag);
}

void ComputeInverseFFT(const ImgFloat& in_img_real,const ImgFloat& in_img_imag, ImgFloat* out_img_real, ImgFloat* out_img_imag)
{
  FftEngine fft_engine(in_img_real.Width(),in_img_real.Height());
  fft_engine.InverseFFT(in_img_real,in_img_imag,out_img_real,out_img_imag);
}

void BgrToRgb(const ImgBgr& img, ImgBgr* out)
{
  if (out == &img)
  { // in place
    ImgBgr::Iterator q = out->Begin();
    assert(q == img.Begin());
    for (; q != out->End() ; q++)
    {
      unsigned char v = q->r;
      q->r = q->b;
      q->b = v;
    }
  }
  else
  { // not in place
    out->Reset(img.Width(), img.Height());
    ImgBgr::ConstIterator p = img.Begin();
    ImgBgr::Iterator q = out->Begin();
    for (; p != img.End() ; p++, q++)
    {
      q->b = p->r;
      q->g = p->g;
      q->r = p->b;
    }
  }
}

void BgrToHsv(const ImgBgr& img, ImgFloat* h, ImgFloat* s, ImgFloat* v)
{
  int width = img.Width();
  int height = img.Height();
  h->Reset(width, height);
  s->Reset(width, height);
  v->Reset(width, height);

  ImgBgr::ConstIterator p = img.Begin();
  ImgFloat::Iterator qh = h->Begin();
  ImgFloat::Iterator qs = s->Begin();
  ImgFloat::Iterator qv = v->Begin();
  for ( ; p != img.End() ; p++)
  {
    double h, s, v;
    iBgrToHsv(p->b / 255.0, p->g / 255.0, p->r / 255.0, &h, &s, &v);
    *qh++ = (float) h;
    *qs++ = (float) s;
    *qv++ = (float) v;
  }
}

void HsvToBgr(const ImgFloat& h, const ImgFloat& s, const ImgFloat& v, ImgBgr* out)
{
  assert( h.Width() == s.Width() && h.Height() == s.Height() );
  assert( h.Width() == v.Width() && h.Height() == v.Height() );
  out->Reset( h.Width(), h.Height() );

  ImgFloat::ConstIterator qh = h.Begin();
  ImgFloat::ConstIterator qs = s.Begin();
  ImgFloat::ConstIterator qv = v.Begin();
  ImgBgr::Iterator p = out->Begin();
  for ( ; p != out->End() ; p++)
  {
    double b, g, r;
    iHsvToBgr(*qh++, *qs++, *qv++, &b, &g, &r);
    p->b = static_cast<unsigned char>( b );
    p->g = static_cast<unsigned char>( g );
    p->r = static_cast<unsigned char>( r );
  }
}


///////////////////////////////////////////////////////////////////////////////
void Thin3x3(ImgBinary* edges)
{  
  int i, x, y;
  int w = edges->Width();
  int h = edges->Height();
  bool erase;
  ImgBinary bimg;
  bimg=(*edges);
// Set(&bimg,0);

  ImgBinary::ConstIterator iter[9];
  
  int se_1[9] = {0,0,0,9,1,9,1,1,1};
  int se_2[9] = {9,0,0,1,1,0,9,1,9};
  int se_3[9] = {0,9,1,0,1,1,0,9,1};
  int se_4[9] = {0,0,9,0,1,1,9,1,9};
  int se_5[9] = {1,1,1,9,1,9,0,0,0};
  int se_6[9] = {9,1,9,0,1,1,0,0,9};
  int se_7[9] = {1,9,0,1,1,0,1,9,0};
  int se_8[9] = {9,1,9,1,1,0,9,0,0};

  int pattern[9];
  
  for(y=1; y < h-1; y++)
  {
    i = 0;
    // first three columns
    for(int yy=y-1; yy < y+2; yy++)
    {
      for(int xx=0; xx<3; xx++)
      {
        iter[i] = edges->Begin(xx,yy);
        i++;
      }
      
    }
    
    // central columns
    for(x=1; x < w-1; x++)
    {
      if(*iter[4] != 0)
      {
        for(i=0; i < 9; i++)
        {
          pattern[i] = (int)(*(iter[i]));
        }
        erase=true;
        for(i=0; i < 9; i++)
        {
          if(se_1[i] > 1)
            continue;
          
          if(se_1[i] != pattern[i])
          {
            erase = false;
            break;
          }
        }
        if(erase == true)
        {
          //          (*edges)(x,y) = 0;         
          bimg(x,y)=0;
        }
      }
      for(i=0; i < 9; i++)
      {
        iter[i]++;
      }
    }
  }
  (*edges)=bimg;

  for(y=1; y < h-1; y++)
  {
    i = 0;
    for(int yy=y-1; yy < y+2; yy++)
    {
      for(int xx=0; xx<3; xx++)
      {
        iter[i] = edges->Begin(xx,yy);
        i++;
      }
      
    }
    for(x=1; x < w-1; x++)
    {
      if(*iter[4] != 0)
      {
        for(i=0; i < 9; i++)
        {
          pattern[i] = (int)(*(iter[i]));
        }
        erase=true;
        for(i=0; i < 9; i++)
        {
          if(se_2[i] > 1)
            continue;
          
          if(se_2[i] != pattern[i])
          {
            erase = false;
            break;
          }
        }
        if(erase == true)
        {
          //          (*edges)(x,y) = 0;         
          bimg(x,y)=0;
        }
      }
      for(i=0; i < 9; i++)
      {
        iter[i]++;
      }
    }
  }
  (*edges)=bimg;
  
  for(y=1; y < h-1; y++)
  {
    i = 0;
    for(int yy=y-1; yy < y+2; yy++)
    {
      for(int xx=0; xx<3; xx++)
      {
        iter[i] = edges->Begin(xx,yy);
        i++;
      }
      
    }
    for(x=1; x < w-1; x++)
    {
      if(*iter[4] != 0)
      {
        for(i=0; i < 9; i++)
        {
          pattern[i] = (int)(*(iter[i]));
        }
        erase=true;
        for(i=0; i < 9; i++)
        {
          if(se_3[i] > 1)
            continue;
          
          if(se_3[i] != pattern[i])
          {
            erase = false;
            break;
          }
        }
        if(erase == true)
        {
          //          (*edges)(x,y) = 0;         
          bimg(x,y)=0;
        }
      }
      for(i=0; i < 9; i++)
      {
        iter[i]++;
      }
    }
  }
  (*edges)=bimg;
  
  for(y=1; y < h-1; y++)
  {
    i = 0;
    for(int yy=y-1; yy < y+2; yy++)
    {
      for(int xx=0; xx<3; xx++)
      {
        iter[i] = edges->Begin(xx,yy);
        i++;
      }
      
    }
    for(x=1; x < w-1; x++)
    {
      if(*iter[4] != 0)
      {
        for(i=0; i < 9; i++)
        {
          pattern[i] = (int)(*(iter[i]));
        }
        erase=true;
        for(i=0; i < 9; i++)
        {
          if(se_4[i] > 1)
            continue;
          
          if(se_4[i] != pattern[i])
          {
            erase = false;
            break;
          }
        }
        if(erase == true)
        {
          //          (*edges)(x,y) = 0;         
          bimg(x,y)=0;
        }
      }
      for(i=0; i < 9; i++)
      {
        iter[i]++;
      }
    }
  }
  (*edges)=bimg;
  
  for(y=1; y < h-1; y++)
  {
    i = 0;
    for(int yy=y-1; yy < y+2; yy++)
    {
      for(int xx=0; xx<3; xx++)
      {
        iter[i] = edges->Begin(xx,yy);
        i++;
      }
      
    }
    for(x=1; x < w-1; x++)
    {
      if(*iter[4] != 0)
      {
        for(i=0; i < 9; i++)
        {
          pattern[i] = (int)(*(iter[i]));
        }
        erase=true;
        for(i=0; i < 9; i++)
        {
          if(se_5[i] > 1)
            continue;
          
          if(se_5[i] != pattern[i])
          {
            erase = false;
            break;
          }
        }
        if(erase == true)
        {
          //          (*edges)(x,y) = 0;         
          bimg(x,y)=0;
        }
      }
      for(i=0; i < 9; i++)
      {
        iter[i]++;
      }
    }
  }
  (*edges)=bimg;
  
  for(y=1; y < h-1; y++)
  {
    i = 0;
    for(int yy=y-1; yy < y+2; yy++)
    {
      for(int xx=0; xx<3; xx++)
      {
        iter[i] = edges->Begin(xx,yy);
        i++;
      }
      
    }
    for(x=1; x < w-1; x++)
    {
      if(*iter[4] != 0)
      {
        for(i=0; i < 9; i++)
        {
          pattern[i] = (int)(*(iter[i]));
        }
        erase=true;
        for(i=0; i < 9; i++)
        {
          if(se_6[i] > 1)
            continue;
          
          if(se_6[i] != pattern[i])
          {
            erase = false;
            break;
          }
        }
        if(erase == true)
        {
          //          (*edges)(x,y) = 0;         
          bimg(x,y)=0;
        }
      }
      for(i=0; i < 9; i++)
      {
        iter[i]++;
      }
    }
  }
  (*edges)=bimg;
  
  for(y=1; y < h-1; y++)
  {
    i = 0;
    for(int yy=y-1; yy < y+2; yy++)
    {
      for(int xx=0; xx<3; xx++)
      {
        iter[i] = edges->Begin(xx,yy);
        i++;
      }
      
    }
    for(x=1; x < w-1; x++)
    {
      if(*iter[4] != 0)
      {
        for(i=0; i < 9; i++)
        {
          pattern[i] = (int)(*(iter[i]));
        }
        erase=true;
        for(i=0; i < 9; i++)
        {
          if(se_7[i] > 1)
            continue;
          
          if(se_7[i] != pattern[i])
          {
            erase = false;
            break;
          }
        }
        if(erase == true)
        {
          //          (*edges)(x,y) = 0;         
          bimg(x,y)=0;
        }
      }
      for(i=0; i < 9; i++)
      {
        iter[i]++;
      }
    }
  }
  (*edges)=bimg;
  
  for(y=1; y < h-1; y++)
  {
    i = 0;
    for(int yy=y-1; yy < y+2; yy++)
    {
      for(int xx=0; xx<3; xx++)
      {
        iter[i] = edges->Begin(xx,yy);
        i++;
      }
      
    }
    for(x=1; x < w-1; x++)
    {
      if(*iter[4] != 0)
      {
        for(i=0; i < 9; i++)
        {
          pattern[i] = (int)(*(iter[i]));
        }
        erase=true;
        for(i=0; i < 9; i++)
        {
          if(se_8[i] > 1)
            continue;
          
          if(se_8[i] != pattern[i])
          {
            erase = false;
            break;
          }
        }
        if(erase == true)
        {
          //          (*edges)(x,y) = 0;         
          bimg(x,y)=0;
        }
      }
      for(i=0; i < 9; i++)
      {
        iter[i]++;
      }
    }
  }

/*
        if(erase == false)
        {
          erase = true;
          
          for(i=0; i < 9; i++)
          {
            if(se_2[i] > 1)
              continue;
            
            if(se_2[i] != pattern[i])
            {
              erase = false;
              break;
            }
          }
        }
        
        if(erase == false)
        {
          erase = true;
          
          for(i=0; i < 9; i++)
          {
            if(se_3[i] > 1)
              continue;
            
            if(se_3[i] != pattern[i])
            {
              erase = false;
              break;
            }
          }
        }
        
        if(erase == false)
        {
          erase = true;
          
          for(i=0; i < 9; i++)
          {
            if(se_4[i] > 1)
              continue;
            
            if(se_4[i] != pattern[i])
            {
              erase = false;
              break;
            }
          }
        }
        
        
        if(erase == false)
        {
          erase = true;
          
          for(i=0; i < 9; i++)
          {
            if(se_5[i] > 1)
              continue;
            
            if(se_5[i] != pattern[i])
            {
              erase = false;
              break;
            }
          }
        }
        
        if(erase == false)
        {
          erase = true;
          
          for(i=0; i < 9; i++)
          {
            if(se_6[i] > 1)
              continue;
            
            if(se_6[i] != pattern[i])
            {
              erase = false;
              break;
            }
          }
        }
        
        if(erase == false)
        {
          erase = true;
          
          for(i=0; i < 9; i++)
          {
            if(se_7[i] > 1)
              continue;
            
            if(se_7[i] != pattern[i])
            {
              erase = false;
              break;
            }
          }
        }
        
        if(erase == false)
        {
          erase = true;
          
          for(i=0; i < 9; i++)
          {
            if(se_8[i] > 1)
              continue;
            
            if(se_8[i] != pattern[i])
            {
              erase = false;
              break;
            }
          }
        }
        
        
        if(erase == true)
        {
//          (*edges)(x,y) = 0;         
			bimg(x,y)=0;
        }
	  }

	  for(i=0; i < 9; i++)
	  {
	    iter[i]++;
	  }
    }
  }
*/
  (*edges)=bimg;
}

void Skeleton(const ImgBinary& bimg, ImgBinary* out)
{
//  out->Reset(bimg.Width(), bimg.Height());	
//
////	bimg2=bimg;
//
////	Figure fig;
//
//  while (1)
//  {
//    Thin3x3(&bimg2);
//    if (IsIdentical(bimg, bimg2))  break;
//    bimg = bimg2;
//
////		fig.Draw(bimg);
//	}
//	return(bimg);
}

//////////////////////////////////////////////////////////////////////////////

void FindJunctions(const ImgBinary& edges, Array<Point>* pts)
{
  
  int i, x, y;
  int w = edges.Width();
  int h = edges.Height();
  
  pts->Reset();
  
  int b1[8] = {0, 3, 6, 7, 8, 5, 2, 1};
  int b2[8] = {3, 6, 7, 8, 5, 2, 1, 0};
  
  ImgBinary::ConstIterator iter[9];
  
  
  for(y=1; y < h-1; y++)
  {
    
    i = 0;
    for(int yy=y-1; yy < y+2; yy++)
    {
      for(int xx=0; xx<3; xx++)
      {
        iter[i] = edges.Begin(xx,yy);
        i++;
      }
    }
    
    for(x=1; x < w-1; x++)
    {
      
      
      if(*(iter[4]) != 0)
      {
        int crossings = 0;
        for(i=0; i < 8; i++)
        {
          int p1 = b1[i];
          int p2 = b2[i];
          crossings += abs( (int)(*(iter[p1])) - (int)(*(iter[p2])) );
        }
        
        if(crossings >= 6)
        {
          pts->Push(Point(x,y));
        }
      }
      
      for(i=0; i < 9; i++)
      {
        iter[i]++;
      }
    }
  }

}

void NonmaxSuppress4(const ImgFloat& img, ImgFloat* out, ImgFloat::Pixel nonmax_val)
{
//  InPlaceSwapper< Image<T> > inplace(img, &out);
  InPlaceSwapper< ImgFloat > inplace(img, &out);

  int w = img.Width();
  int h = img.Height();
  int x, y;

  out->Reset(w, h);
  Set(out, nonmax_val);

  for(y=1; y < h-1 ; y++)
  {
    for(x=1; x < w-1 ; x++)
    {
      if ( img(x,y) >= img(x-1,y)
        && img(x,y) >= img(x+1,y)
        && img(x,y) >= img(x,y-1)
        && img(x,y) >= img(x,y+1))
      {
        (*out)(x,y) = img(x,y);
      }
    }
  }
}

void NonmaxSuppress8(const ImgFloat& img, ImgFloat* out, ImgFloat::Pixel nonmax_val)
{
//  InPlaceSwapper< Image<T> > inplace(img, &out);
  InPlaceSwapper< ImgFloat > inplace(img, &out);

  int w = img.Width();
  int h = img.Height();
  int x, y;

  out->Reset(w, h);
  Set(out, nonmax_val);

  for(y=1; y < h-1 ; y++)
  {
    for(x=1; x < w-1 ; x++)
    {
      if ( img(x,y) >= img(x-1,y)
        && img(x,y) >= img(x+1,y)
        && img(x,y) >= img(x,y-1)
        && img(x,y) >= img(x,y+1)
        && img(x,y) >= img(x-1,y-1)
        && img(x,y) >= img(x-1,y+1)
        && img(x,y) >= img(x+1,y-1)
        && img(x,y) >= img(x+1,y+1))
      {
        (*out)(x,y) = img(x,y);
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////
void QuantizeImage(unsigned short bit_shifts, ImgGray* img)
{
	ImgGray::Iterator iter = img->Begin();

	while(iter != img->End())
	{
		*iter = ((*iter) >> bit_shifts) << bit_shifts;
		iter++;	
	}
}

void QuantizeImage(unsigned short bit_shifts, ImgBgr* img)
{
	ImgBgr::Iterator iter = img->Begin();

	while(iter != img->End())
	{
		(*iter).r = (((*iter).r) >> bit_shifts) << bit_shifts;
		(*iter).g = (((*iter).g) >> bit_shifts) << bit_shifts;
		(*iter).b = (((*iter).b) >> bit_shifts) << bit_shifts;
		iter++;	
	}
}
///////////////////////////////////////////////////////////////////////
// Median Filter for Grayscale images based on bin-sort.
// Neeraj Kanhere (nkanher@clemson.edu)
///////////////////////////////////////////////////////////////////////
void RankFilter(const ImgBgr& img, int rx, int ry, ImgBgr* out, int rank)
{
	ImgGray b,fb, g,fg, r,fr;
	ExtractBgr(img, &b, &g, &r);
	RankFilter(b, rx, ry, &fb);
	RankFilter(g, rx, ry, &fg);
	RankFilter(r, rx, ry, &fr);
	CombineBgr(fb, fg, fr, out);
}
void RankFilter(const ImgGray& img, int rx, int ry, ImgGray* out, int rank)
{
  assert(rx > 0);
  assert(ry > 0);
  
  int rank_index;
  int N = (2*rx + 1) * (2*ry + 1); // total number of pixels in a window
  if((rank < 1) |(rank > N))
  {
    rank_index = N/2;
  }
  else
  {
    rank_index = rank-1;
  }

  int w = img.Width();
  int h = img.Height();
  
  int x, y, i, j;

  out->Reset(w,h);
  Set(out, 0);

  unsigned char hist[256];
  
  ImgGray::ConstIterator top_left;
  ImgGray::ConstIterator top_right;
  ImgGray::Iterator out_iter;

  Array<ImgGray::ConstIterator> left_column(2*ry+1);
  Array<ImgGray::ConstIterator> right_column(2*ry+1);

  for(y=ry; y < h-ry; y++)
  {

    for(i=0; i < 256; i++)
    {
      hist[i] = 0;
    }

    unsigned char bin;
    ImgGray::ConstRectIterator init_iter = img.BeginRect(Rect(0,y-ry,2*rx+1,y+ry+1));
    assert(*init_iter == img(0, y-ry));

    while(!init_iter.AtEnd())
    {
      bin = *init_iter;      
      hist[bin] += 1;
      init_iter++;
    }

    // count the number of elements in bin starting from 0-intensity, and stop when
    // rank_index is reached.
    int acc = 0;
    int output_value = -1;
    while((acc < rank_index) | (acc < 1))
    {
      output_value++;
      acc += hist[output_value];      
    }    
    out_iter = out->Begin(rx, y);
    *out_iter = (unsigned char) output_value;    
    
    // done computing value of the first pixel (x=0) for row y.

    // set up for looping over x:
    top_left = img.Begin(1, y-ry);  
    top_right = top_left + 2*rx;            
    for(j=0; j<= 2*ry; j++)
    {
      left_column[j] = top_left;
      right_column[j] = top_right;
      top_left += w;
      top_right += w;
    }

    // loop over x:
    for(x=rx+1; x < w-rx; x++)
    {
      out_iter++;
      
      for(int j=0; j <= 2*ry; j++)
      {
        // remove entries of left-column (of window) from hitogram 
        bin = *(left_column[j]);
        hist[bin] -= 1; 
        left_column[j] +=1;

        //add new entries of right-column .
        bin = *(right_column[j]);
        hist[bin] += 1;
        right_column[j] +=1;
        
      }

      acc = 0;
      output_value = -1;
      
      while((acc < rank_index) | (acc < 1))
      {
        output_value++;
        acc += hist[output_value];      
      }
      *out_iter = (unsigned char) output_value;    
      
    }
  }
}


void ExtractBgr(const ImgBgr& img, ImgGray* b, ImgGray* g, ImgGray* r)
{
  b->Reset(img.Width(), img.Height());
  g->Reset(img.Width(), img.Height());
  r->Reset(img.Width(), img.Height());

  ImgBgr::ConstIterator iter = img.Begin();
  ImgGray::Iterator iter_b = b->Begin();
  ImgGray::Iterator iter_g = g->Begin();
  ImgGray::Iterator iter_r = r->Begin();

  while(iter != img.End())
  {
    *iter_b = iter->b;
    *iter_g = iter->g;
    *iter_r = iter->r;

    iter++;
    iter_b++;
    iter_g++;
    iter_r++;
  }

}

void CombineBgr(const ImgGray& b, const ImgGray& g, const ImgGray& r, ImgBgr* img)
{
  assert((b.Width() == g.Width()) & (b.Height() == g.Height()));
  assert((r.Width() == g.Width()) & (r.Height() == g.Height()));

  img->Reset(b.Width(), b.Height());

  ImgGray::ConstIterator iter_b = b.Begin();
  ImgGray::ConstIterator iter_g = g.Begin();
  ImgGray::ConstIterator iter_r = r.Begin();
  ImgBgr::Iterator iter = img->Begin();

  while(iter != img->End())
  {
    iter->b = *iter_b;
    iter->g = *iter_g;
    iter->r = *iter_r;

    iter++;
    iter_b++;
    iter_g++;
    iter_r++;
  }
}

class iDelaunayOpencv
{
public:

  iDelaunayOpencv(const Rect& rect) 
  {
    CvRect r = { rect.left, rect.top, rect.Width(), rect.Height() };
    m_rect = rect;
    m_storage = cvCreateMemStorage(0);
    m_subdiv = cvCreateSubdiv2D( 
                               CV_SEQ_KIND_SUBDIV2D, 
                               sizeof(*m_subdiv),
                               sizeof(CvSubdiv2DPoint),
                               sizeof(CvQuadEdge2D),
                               m_storage );
    cvInitSubdivDelaunay2D( m_subdiv, r );
  }
  ~iDelaunayOpencv()
  {
    cvReleaseMemStorage( &m_storage );
//    cvReleaseSubdiv2D( &m_subdiv );  // There must be a way to release this data structure
  }
  void InsertPoint(const Point2d& pt)
  {
    assert(pt.x >= m_rect.left && pt.y >= m_rect.top && pt.x < m_rect.right && pt.y < m_rect.bottom);
    CvPoint2D32f fp = cvPoint2D32f( (float) pt.x, (float) pt.y );
    cvSubdivDelaunay2DInsert( m_subdiv, fp );
  }
  const std::vector<DelaunayPointPair>* GetEdges()
  {
    UpdateEdges();
    return &m_edges;
  }
protected:
  void UpdateEdges()
  {
    m_edges.clear();
    CvSeqReader reader;
    int total = m_subdiv->edges->total;
    int elem_size = m_subdiv->edges->elem_size;
    int i;

    cvStartReadSeq( (CvSeq*)(m_subdiv->edges), &reader, 0 );

    for( i = 0; i < total; i++ )
    {
      CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);
    
      if( CV_IS_SET_ELEM( edge ))
      {
        CvSubdiv2DPoint* org_pt;
        CvSubdiv2DPoint* dst_pt;
        CvPoint2D32f org;
        CvPoint2D32f dst;
//        CvPoint iorg, idst;
        org_pt = cvSubdiv2DEdgeOrg((CvSubdiv2DEdge) edge);
        dst_pt = cvSubdiv2DEdgeDst((CvSubdiv2DEdge) edge);
        if( org_pt && dst_pt )
        {
          int index = org_pt->first;
          org = org_pt->pt;
          dst = dst_pt->pt;

          DelaunayPointPair p;
          p.first = Point2d( org.x, org.y );
          p.second = Point2d( dst.x, dst.y );
          m_edges.push_back( p );
//          iorg = cvPoint( cvRound( org.x ), cvRound( org.y ));
//          idst = cvPoint( cvRound( dst.x ), cvRound( dst.y ));
        
//          DrawLine(Point(iorg.x, iorg.y), Point(idst.x, idst.y), &img, Bgr(0,255,0) );
//          TRACE("%d\n", index);
//          fig.Draw(img);
          //Sleep(1);
          //          out->push_back();
        }
      }
    
      CV_NEXT_SEQ_ELEM( elem_size, reader );
    }
  }
private:
  CvMemStorage* m_storage;
  CvSubdiv2D* m_subdiv;
  Rect m_rect;
  std::vector<DelaunayPointPair> m_edges;
};

void ComputeDelaunayOpencv(const std::vector<Point2d>& points, const Rect& rect, std::vector<DelaunayPointPair>* out)
{
  iDelaunayOpencv del(rect);
  for ( int i = 0; i < 200; i++ )  del.InsertPoint( points[i] );

  *out = *del.GetEdges();
}

// Bug:  Current implementation does not normalize coordinates first before using them
void ComputeFundamentalMatrix(const std::vector<Point>& pt1, const std::vector<Point>& pt2, MatDbl* out)
{
  const int n = pt1.size();
  assert(n == pt1.size());

  // construct measurement matrix A
  MatDbl a(9, n);
  for (int i=0 ; i<n ; i++)
  {
    const Point& p1 = pt1[i];
    const Point& p2 = pt2[i];
    double x1 = p1.x;
    double y1 = p1.y;
    double x2 = p2.x;
    double y2 = p2.y;
    a(0, i) = x1 * x2;
    a(1, i) = x1 * y2;
    a(2, i) = x1;
    a(3, i) = x2 * y1;
    a(4, i) = y1 * y2;
    a(5, i) = y1;
    a(6, i) = x2;
    a(7, i) = y2;
    a(8, i) = 1;
  }

  // compute svd of A
  MatDbl u, s, v;
  Svd(a, &u, &s, &v);
  assert(v.Width() == 9 && v.Height() == 9);

  // grab smallest singular vector to get Fundamental matrix
  MatDbl fm(3, 3);
  double* p = fm.Begin();
  for (int j=0 ; j<9 ; j++)
  {
    *p++ = v(8, j);
  }

  // now compute SVD of Fundamental matrix and enforce rank(F) = 2
  Svd(fm, &u, &s, &v);
  assert(u.Width() == 3 && u.Height() == 3);
  u(2,2) = 0;

  // compute final Fundamental matrix
  *out = u * Diag(s) * Transpose(v);
}

void ComputeEpipolarLine(double x1, double y1, const MatDbl& fundamental_matrix, double* a, double* b, double* c)
{
  assert(fundamental_matrix.Width() == 3 && fundamental_matrix.Height() == 3);
  const double* p = fundamental_matrix.Begin();
  *a = x1 * p[0] + y1 * p[3] + p[6];
  *b = x1 * p[1] + y1 * p[4] + p[7];
  *c = x1 * p[2] + y1 * p[5] + p[8];
}

int iHexAtoi(char c)
{
  if (c >= '0' && c <= '9')  return c - '0';
  else if (c >= 'a' && c <= 'f')  return c - 'a' + 10;
  else if (c >= 'A' && c <= 'F')  return c - 'A' + 10;
  else 
  {
    assert(0);
    return 0;
  }
}

// If you copy and paste the image data from a color EPS (encapsulated postscript file)
// into a string, this function will convert it to a Bgr image.  With a little extra
// work for file parsing, this could be used to convert an EPS file to a PPM/BMP file.
void ExtractImageFromEpsEncoding(const char* str, int width, int height, ImgBgr* out)
{
  int npix = width * height;
  int nbytes = npix * 3;
  int nchar = strlen(str);
  {
    out->Reset(width, height);
    ImgBgr::Iterator p = out->Begin();
    const char* c = str;
    int i;
    for (i=0 ; i<npix ; i++)
    {
      if ((c[0] == '\x0D') && (c[1] == '\x0A'))  c += 2;  // assuming Windows newline character
      assert( c + 6 - str <= nchar );
      p->r =  iHexAtoi( c[0] ) * 16 + iHexAtoi( c[1] );
      p->g =  iHexAtoi( c[2] ) * 16 + iHexAtoi( c[3] );
      p->b =  iHexAtoi( c[4] ) * 16 + iHexAtoi( c[5] );
      c += 6;
      p++;
    }
    assert( p == out->End() );
    assert( (c - str) == nchar );
//    if ( (c - str) != nchar )
//    {
//      BLEPO_ERROR(StringEx("Wrong string length.  strlen=%d, but expected=%d\n", nchar, nbytes*2));
//    }
  }
}

void ExtractImageFromEpsEncodedRawFile(const CString& fname, int width, int height, ImgBgr* out)
{
  FILE* fp = _wfopen(fname, L"rb");
  if (fp)
  {
    assert(sizeof(char) == 1);
    int nchar = width * height * 3 * 2 * 2;  // 3 color channels, 2 ASCII chars per pixel byte, but last 2 is a hack to make sure we have enough space for 0D0A newline characters
    char* str = (char*) malloc(nchar * sizeof(char));
    int ret = fread(str, 1, nchar, fp);
    str[ ret ] = '\0';
    ExtractImageFromEpsEncoding(str, width, height, out);
    free(str);
  }
}

// inplace ok
void CorruptImageSaltNoise(const ImgGray& img, ImgGray* out, float p)
{
  assert(p >= 0.0 && p <= 1.0);
  ImgGray::ConstIterator a = img.Begin();
  ImgGray::Iterator b = out->Begin();
  while (a != img.End()) 
  {
    double c = blepo_ex::GetRandDbl();
    if (c <= p)  *b = 255;
    a++;  b++;
  }
}

//  Rect rect(0, 0, 400, 400);
//  Figure fig;
//  ImgBgr img(rect.Width(), rect.Height());
//  DelaunayOpencv del(rect);
//  int i;
//
//  for( i = 0; i < 200; i++ )
//  {
//    Point2d pt( rand()%(rect.Width()-10)+5, rand()%(rect.Height()-10)+5 );
//    del.InsertPoint( pt );
//    DrawDot(Point( blepo_ex::Round(pt.x), blepo_ex::Round(pt.y)), &img, Bgr(0,0,255) );
//    fig.Draw(img);
//  }
//
//  const std::vector< DelaunayPointPair >* ed = del.GetEdges();
//
//  for (i=0 ; i<ed->size() ; i++)
//  {
//    const DelaunayOpencv::PointPair& pp = (*ed)[i];
//    Point p1( blepo_ex::Round(pp.first.x), blepo_ex::Round(pp.first.y));
//    Point p2( blepo_ex::Round(pp.second.x), blepo_ex::Round(pp.second.y));
//    DrawLine(p1, p2, &img, Bgr(0,255,0) );
////    TRACE("%d\n", index);
//  }
//  fig.Draw(img);
//}
//
//void ComputeDelaunay(std::vector<Point2d>& points, std::vector<DelaunayEdge>* out)
//{
//  Rect rect(0, 0, 400, 400);
//  Figure fig;
//  ImgBgr img(rect.Width(), rect.Height());
//  DelaunayOpencv del(rect);
//  int i;
//
//  for( i = 0; i < 200; i++ )
//  {
//    Point2d pt( rand()%(rect.Width()-10)+5, rand()%(rect.Height()-10)+5 );
//    del.InsertPoint( pt );
//    DrawDot(Point( blepo_ex::Round(pt.x), blepo_ex::Round(pt.y)), &img, Bgr(0,0,255) );
//    fig.Draw(img);
//  }
//
//  const std::vector< DelaunayOpencv::PointPair >* ed = del.GetEdges();
//
//  for (i=0 ; i<ed->size() ; i++)
//  {
//    const DelaunayOpencv::PointPair& pp = (*ed)[i];
//    Point p1( blepo_ex::Round(pp.first.x), blepo_ex::Round(pp.first.y));
//    Point p2( blepo_ex::Round(pp.second.x), blepo_ex::Round(pp.second.y));
//    DrawLine(p1, p2, &img, Bgr(0,255,0) );
////    TRACE("%d\n", index);
//  }
//  fig.Draw(img);
//}

//  int i;
//
//  CvMemStorage* storage = cvCreateMemStorage(0);
//  CvSubdiv2D* subdiv = cvCreateSubdiv2D( 
//                             CV_SEQ_KIND_SUBDIV2D, 
//                             sizeof(*subdiv),
//                             sizeof(CvSubdiv2DPoint),
//                             sizeof(CvQuadEdge2D),
//                             storage );
//
//  cvInitSubdivDelaunay2D( subdiv, r );
//  for( i = 0; i < 200; i++ )
//  {
//      CvPoint2D32f fp = cvPoint2D32f( (float)(rand()%(r.width-10)+5),
//                                      (float)(rand()%(r.height-10)+5));
//      cvSubdivDelaunay2DInsert( subdiv, fp );
//
//      DrawDot(Point(fp.x, fp.y), &img, Bgr(0,0,255) );
//      fig.Draw(img);
//      //Sleep(1);
//  }
//  CvSeqReader  reader;
//  cvStartReadSeq( (CvSeq*)(subdiv->edges), &reader, 0 );
//
//  int total = subdiv->edges->total;
//  int elem_size = subdiv->edges->elem_size;
//  for( i = 0; i < total; i++ )
//  {
//    CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);
//    
//    if( CV_IS_SET_ELEM( edge ))
//    {
//      CvSubdiv2DPoint* org_pt;
//      CvSubdiv2DPoint* dst_pt;
//      CvPoint2D32f org;
//      CvPoint2D32f dst;
//      CvPoint iorg, idst;
//      org_pt = cvSubdiv2DEdgeOrg((CvSubdiv2DEdge) edge);
//      dst_pt = cvSubdiv2DEdgeDst((CvSubdiv2DEdge) edge);
//      if( org_pt && dst_pt )
//      {
//        int index = org_pt->first;
//        org = org_pt->pt;
//        dst = dst_pt->pt;
//
//        std::pair<float, float> p;
//        iorg = cvPoint( cvRound( org.x ), cvRound( org.y ));
//        idst = cvPoint( cvRound( dst.x ), cvRound( dst.y ));
//        
//        DrawLine(Point(iorg.x, iorg.y), Point(idst.x, idst.y), &img, Bgr(0,255,0) );
//        TRACE("%d\n", index);
//        fig.Draw(img);
//        //Sleep(1);
//        //          out->push_back();
//      }
//    }
//    
//    CV_NEXT_SEQ_ELEM( elem_size, reader );
//  }
//  cvReleaseMemStorage( &storage );
//}



// in-place NOT okay
//void Convolve5x1(const ImgFloat& img, float* kernel, ImgFloat* out)
//{
//
//}

//void jDetectFeatures(const ImgGray& img, std::vector<jFeature>* features)
//{
//  Figure fig1, fig2, fig3, fig4, fig5, fig6, fig7;
//  ImgFloat fimg, gradx, grady, ggxx, ggyy, ggxy, tmp, fimg_smoothed, mineig, gradx2, grady2;
//  int x, y;
//
//  Convert(img, &fimg);
//  // compute gradient
//#if 0
//  // baseline system
//  double sigma = 1.5;
//  int kernel_length = 7;
//  Convert(img, &fimg);
//  Gradient(fimg, sigma, kernel_length, &gradx, &grady);
//#else
//  // my fancy way of avoiding too many convolutions
//
//  // build kernel
//  ImgFloat kernel(5,1);
//  {
//    float k[] = { 0.0293, 0.0575, 0.0762, 0.0575, 0.0293 };
//    memcpy(kernel.Begin(), k, 5 * sizeof(k[0]));
//  }
//
//  // smooth image
//  Convolve(fimg, kernel, &tmp);
//  Transpose(kernel, &kernel);
//  Convolve(tmp, kernel, &fimg_smoothed);
//
//  fig1.Draw(fimg);
//  fig2.Draw(tmp);
//  fig3.Draw(fimg_smoothed);
//  {
//    float k[] = { 1, 0, 0, 0, -1 };
//    memcpy(kernel.Begin(), k, 5 * sizeof(k[0]));
//  }
//  Convolve(fimg_smoothed, kernel, &gradx);
//  Transpose(kernel, &kernel);
//  Convolve(fimg_smoothed, kernel, &grady);
//#endif
//
//  fig4.Draw(gradx);
//  fig5.Draw(grady);
//
//  Multiply(gradx, gradx, &ggxx);
//  Multiply(grady, grady, &ggyy);
//  Multiply(gradx, grady, &ggxy);
//  fig4.Draw(ggxx);
//  SmoothBox3x3(ggxx, &ggxx);
//  SmoothBox3x3(ggyy, &ggyy);
//  SmoothBox3x3(ggxy, &ggxy);
//
//  fig1.Draw(fimg);
//  fig2.Draw(gradx);
//  fig3.Draw(grady);
//  fig5.Draw(ggxx);
////  fig5.Draw(ggyy);
//  fig6.Draw(ggxy);
//
//  mineig.Reset( img.Width(), img.Height() );
//  float* pxx = ggxx.Begin();
//  float* pyy = ggyy.Begin();
//  float* pxy = ggxy.Begin();
//  float* q = mineig.Begin();
//  for (y=0 ; y<img.Height() ; y++)
//  {
//    for (x=0 ; x<img.Width() ; x++)
//    {
//      float gxx = *pxx;
//      float gyy = *pyy;
//      float gxy = *pxy;
//      *q++ = ((gxx + gyy - sqrt((gxx - gyy)*(gxx - gyy) + 4*gxy*gxy))/2.0f);
//      pxx++;  pyy++;  pxy++;
//    }
//  }
//  //Clamp(mineig, 0, 1, &mineig);
//  fig7.Draw(mineig);
//
//  std::vector<fPoint> points;
//  float* r = mineig.Begin(1, 1);
//  for (y=1 ; y<img.Height()-1 ; y++)
//  {
//    for (x=1 ; x<img.Width()-1 ; x++)
//    {
//      if ( r[0] >= r[-1]
//        && r[0] >= r[1]
//        && r[0] >= r[-w-1]
//        && r[0] >= r[-w]
//        && r[0] >= r[-w+1]
//        && r[0] >= r[w-1]
//        && r[0] >= r[w]
//        && r[0] >= r[w+1])
//      {
//
//      }
//      float gxx = *pxx;
//      float gyy = *pyy;
//      float gxy = *pxy;
//      *q++ = ((gxx + gyy - sqrt((gxx - gyy)*(gxx - gyy) + 4*gxy*gxy))/2.0f);
//      pxx++;  pyy++;  pxy++;
//    }
//  }
//
//  //
////  gradx.Reset( img.Width(), img.Height() );
////  grady.Reset( img.Width(), img.Height() );
////  Set(&gradx, 0);
////  Set(&grady, 0);
////  mineig.Reset( img.Width(), img.Height() );
////  int x, y;
////  float* gx = gradx.Begin();
////  float* gy = grady.Begin();
////  float* ggxy = gradx2.Begin();
////  float* q = mineig.Begin();
////  for (y = 0 ; y < img.Height() ; y++)
////  {
////    for (x = 0 ; x < img.Width() ; x++)
////    {
////      float gxx = (*gx) * (*gx);
////      float gyy = (*gy) * (*gy);
////      float gxy = (*ggxy);
////      *q++ = ((gxx + gyy - sqrt((gxx - gyy)*(gxx - gyy) + 4*gxy*gxy))/2.0f);
////      gx++;  gy++;  ggxy++;
////    }
////  }
////  Clamp(mineig, 0, 30, &mineig);
////  fig7.Draw(mineig);
//
////
////
////  double sigma = 1.5;
////  int kernel_length = 7;
////  Convert(img, &fimg);
////  Convolve(fimg, kernel, &tmp);
////
////  Gradient(fimg, sigma, kernel_length, &gradx, &grady);
////  Multiply(gradx, gradx, &gxx);
////  Multiply(grady, grady, &gyy);
////  Multiply(gradx, grady, &gxy);
////
////  for (int y=0 ; y<img.Height() ; y++)
////  {
////    for (int x=0 ; x<img.Width() ; x++)
////    {
////      double lambda = 
////
////    }
////  }
////
////  Convolve(fimg, kernel, &);
////
////
//}
//
//void jDetectFeaturesBaseline(const ImgGray& img, std::vector<jFeature>* features)
//{
//  Figure fig1, fig2, fig3, fig4, fig5, fig6, fig7;
//  ImgFloat fimg, gradx, grady, ggxx, ggyy, ggxy, tmp, fimg_smoothed, mineig, gradx2, grady2;
//
//  double sigma = 1.5;
//  int kernel_length = 7;
//  Convert(img, &fimg);
//  Gradient(fimg, sigma, kernel_length, &gradx, &grady);
//  Multiply(gradx, gradx, &ggxx);
//  Multiply(grady, grady, &ggyy);
//  Multiply(gradx, grady, &ggxy);
//  fig4.Draw(ggxx);
//  SmoothBox3x3(ggxx, &ggxx);
//  SmoothBox3x3(ggyy, &ggyy);
//  SmoothBox3x3(ggxy, &ggxy);
//
//  fig1.Draw(fimg);
//  fig2.Draw(gradx);
//  fig3.Draw(grady);
//  fig5.Draw(ggxx);
////  fig5.Draw(ggyy);
//  fig6.Draw(ggxy);
//
//  mineig.Reset( img.Width(), img.Height() );
//  float* pxx = ggxx.Begin();
//  float* pyy = ggyy.Begin();
//  float* pxy = ggxy.Begin();
//  float* q = mineig.Begin();
//  for (int y=0 ; y<img.Height() ; y++)
//  {
//    for (int x=0 ; x<img.Width() ; x++)
//    {
//      float gxx = *pxx;
//      float gyy = *pyy;
//      float gxy = *pxy;
//      *q++ = ((gxx + gyy - sqrt((gxx - gyy)*(gxx - gyy) + 4*gxy*gxy))/2.0f);
//      pxx++;  pyy++;  pxy++;
//    }
//  }
//  Clamp(mineig, 0, 100, &mineig);
//  fig7.Draw(mineig);
//
////  Convolve(fimg, kernel, &);
//
//
//
////  // build kernel
////  ImgFloat kernel(5,1);
////  {
////    float k[] = { 0.0293, 0.0575, 0.0762, 0.0575, 0.0293 };
////    memcpy(kernel.Begin(), k, 5 * sizeof(k[0]));
////  }
////
////  // smooth image
////  Convert(img, &fimg);
////  Convolve(fimg, kernel, &tmp);
////  Transpose(kernel, &kernel);
////  Convolve(tmp, kernel, &fimg_smoothed);
////
////  fig1.Draw(fimg);
////  fig2.Draw(tmp);
////  fig3.Draw(fimg_smoothed);
////
////  // compute gradient
////  {
////    float k[] = { -1, 0, 0, 0, 1 };
////    memcpy(kernel.Begin(), k, 5 * sizeof(k[0]));
////  }
////  Convolve(fimg_smoothed, kernel, &gradx);
////  Transpose(kernel, &kernel);
////  Convolve(fimg_smoothed, kernel, &grady);
////
////  fig4.Draw(gradx);
////  fig5.Draw(grady);
////
////  // compute multiplied gradient
////  {
////    float k[] = {0, -1, 0, 1, 0};
////    memcpy(kernel.Begin(), k, 5 * sizeof(k[0]));
////  }
////  Convolve(fimg_smoothed, kernel, &gradx2);
////  Transpose(kernel, &kernel);
////  Convolve(fimg_smoothed, kernel, &grady2);
////  Multiply(gradx2, grady2, &gradx2);
////  {
////    float k[] = {.0625, .25, .375, .25, .0625};
////    memcpy(kernel.Begin(), k, 5 * sizeof(k[0]));
////  }
////  Convolve(gradx2, kernel, &grady2);
////  Transpose(kernel, &kernel);
////  Convolve(grady2, kernel, &gradx2);
////
////  fig6.Draw(gradx2);
////
//////
//////  gradx.Reset( img.Width(), img.Height() );
//////  grady.Reset( img.Width(), img.Height() );
//////  Set(&gradx, 0);
//////  Set(&grady, 0);
////  mineig.Reset( img.Width(), img.Height() );
////  int x, y;
////  float* gx = gradx.Begin();
////  float* gy = grady.Begin();
////  float* ggxy = gradx2.Begin();
////  float* q = mineig.Begin();
////  for (y = 0 ; y < img.Height() ; y++)
////  {
////    for (x = 0 ; x < img.Width() ; x++)
////    {
////      float gxx = (*gx) * (*gx);
////      float gyy = (*gy) * (*gy);
////      float gxy = (*ggxy);
////      *q++ = ((gxx + gyy - sqrt((gxx - gyy)*(gxx - gyy) + 4*gxy*gxy))/2.0f);
////      gx++;  gy++;  ggxy++;
////    }
////  }
////  Clamp(mineig, 0, 30, &mineig);
////  fig7.Draw(mineig);
////
//////
//////
//}

//////////////////////////////////////////////////////////////////////////////
};  // end namespace blepo

