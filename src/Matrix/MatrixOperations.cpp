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

#include "MatrixOperations.h"
#include "Utilities/Math.h"  // blepo_ex::Min()
#include <stdio.h>  // FILE, fopen, ...


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

void Set(MatDbl* out, double val)
{
  for (MatDbl::Iterator p = out->Begin() ; p != out->End() ; p++)  *p = val;
}

void Set(MatFlt* out, float val)
{
  for (MatFlt::Iterator p = out->Begin() ; p != out->End() ; p++)  *p = val;
}

void Eye(int dim, MatDbl* mat)
{
  mat->Reset(dim, dim);
  Set(mat, 0);
  MatDbl::Iterator p = mat->Begin();
  for (int i=0 ; i<dim ; i++)  { *p = 1;  p += dim + 1; }
}

void Eye(int dim, MatFlt* mat)
{
  mat->Reset(dim, dim);
  Set(mat, 0);
  MatFlt::Iterator p = mat->Begin();
  for (int i=0 ; i<dim ; i++)  { *p = 1.0f;  p += dim + 1; }
}

void Diag(const MatDbl& mat, MatDbl* out)
{
  if (mat.IsVector())
  { // construct diagonal matrix
    out->Reset(mat.Length(), mat.Length());
    Set(out, 0);
    for (int i=0 ; i<mat.Length() ; i++)  (*out)(i,i) = mat(i);
  }
  else
  { // extract diagonal
    int m = blepo_ex::Min(mat.Width(), mat.Height());
    out->Reset(m);
    for (int i=0 ; i<m ; i++)  (*out)(i) = mat(i,i);
  }
}

void Rand(int width, int height, MatDbl* out)
{
  out->Reset(width, height);
  MatDbl::Iterator p = out->Begin();
  while (p != out->End())  *p++ = blepo_ex::GetRandDbl();
}

void Add(const MatDbl& src1, const MatDbl& src2, MatDbl* dst)
{
  if (!IsSameSize(src1, src2))  BLEPO_ERROR("Matrices must be same size");
  dst->Reset(src1.Width(), src1.Height());
  MatDbl::ConstIterator p1 = src1.Begin();
  MatDbl::ConstIterator p2 = src2.Begin();
  MatDbl::Iterator q = dst->Begin();
  while (p1 != src1.End())  *q++ = *p1++ + *p2++;
}

void Add(const MatFlt& src1, const MatFlt& src2, MatFlt* dst)
{
  if (!IsSameSize(src1, src2))  BLEPO_ERROR("Matrices must be same size");
  dst->Reset(src1.Width(), src1.Height());
  MatFlt::ConstIterator p1 = src1.Begin();
  MatFlt::ConstIterator p2 = src2.Begin();
  MatFlt::Iterator q = dst->Begin();
  while (p1 != src1.End())  *q++ = *p1++ + *p2++;
}

void Subtract(const MatDbl& src1, const MatDbl& src2, MatDbl* dst)
{
  if (!IsSameSize(src1, src2))  BLEPO_ERROR("Matrices must be same size");
  dst->Reset(src1.Width(), src1.Height());
  MatDbl::ConstIterator p1 = src1.Begin();
  MatDbl::ConstIterator p2 = src2.Begin();
  MatDbl::Iterator q = dst->Begin();
  while (p1 != src1.End())  *q++ = *p1++ - *p2++;
}

void Subtract(const MatFlt& src1, const MatFlt& src2, MatFlt* dst)
{
  if (!IsSameSize(src1, src2))  BLEPO_ERROR("Matrices must be same size");
  dst->Reset(src1.Width(), src1.Height());
  MatFlt::ConstIterator p1 = src1.Begin();
  MatFlt::ConstIterator p2 = src2.Begin();
  MatFlt::Iterator q = dst->Begin();
  while (p1 != src1.End())  *q++ = *p1++ - *p2++;
}


//void Multiply(const MatDbl& src1, const MatDbl& src2, MatDbl* dst)
//{
//  dst->Reset(src2.Width(), src1.Height());
//  MatDbl::ConstIterator p1 = src1.Begin();
//  MatDbl::ConstIterator p2 = src2.Begin();
//  MatDbl::Iterator q = dst->Begin();
//  while (p1 != src1.End())  *q++ = *p1++ * *p2++;
//}

void Add(const MatDbl& src, double val, MatDbl* dst)
{
  dst->Reset(src.Width(), src.Height());
  MatDbl::ConstIterator p = src.Begin();
  MatDbl::Iterator q = dst->Begin();
  while (p != src.End())  *q++ = *p++ + val;
}

void Subtract(const MatDbl& src, double val, MatDbl* dst)
{
  dst->Reset(src.Width(), src.Height());
  MatDbl::ConstIterator p = src.Begin();
  MatDbl::Iterator q = dst->Begin();
  while (p != src.End())  *q++ = *p++ - val;
}

void Multiply(const MatDbl& src, double val, MatDbl* dst)
{
  dst->Reset(src.Width(), src.Height());
  MatDbl::ConstIterator p = src.Begin();
  MatDbl::Iterator q = dst->Begin();
  while (p != src.End())  *q++ = *p++ * val;
}

void Negate(const MatDbl& src, MatDbl* dst)
{
  dst->Reset(src.Width(), src.Height());
  MatDbl::ConstIterator p = src.Begin();
  MatDbl::Iterator q = dst->Begin();
  while (p != src.End())  *q++ = -(*p++);
}

void Negate(const MatFlt& src, MatFlt* dst)
{
  dst->Reset(src.Width(), src.Height());
  MatFlt::ConstIterator p = src.Begin();
  MatFlt::Iterator q = dst->Begin();
  while (p != src.End())  *q++ = -(*p++);
}

bool Similar(const MatDbl& src1, const MatDbl& src2, double tolerance)
{
  if (src1.Width() != src2.Width())  return false;
  if (src1.Height() != src2.Height())  return false;
  MatDbl::ConstIterator p1 = src1.Begin();
  MatDbl::ConstIterator p2 = src2.Begin();
  while (p1 != src1.End())  if (!blepo_ex::Similar(*p1++, *p2++, tolerance))  return false;
  return true;
}

double Norm(const MatDbl& src)
{
  if (!src.IsVector())  BLEPO_ERROR("Cannot compute norm of a matrix");
  double sumsq = 0;  
  for (MatDbl::ConstIterator p = src.Begin() ; p != src.End() ; p++)  sumsq += (*p) * (*p);
  return sqrt(sumsq);
}

//void operator+=(MatDbl& io, const MatDbl& src2)
//{
//  Add(io, src2, &io);
//}
//
//void operator-=(MatDbl& io, const MatDbl& src2)
//{
//  Subtract(io, src2, &io);
//}

MatDbl Diag(const MatDbl& mat)
{
  MatDbl tmp;
  Diag(mat, &tmp);
  return tmp;
}

MatDbl Transpose(const MatDbl& mat)
{
  MatDbl tmp;
  Transpose(mat, &tmp);
  return tmp;
}

MatDbl operator+(const MatDbl& src1, const MatDbl& src2)
{
  MatDbl tmp;
  Add(src1, src2, &tmp);
  return tmp;
}

MatFlt operator+(const MatFlt& src1, const MatFlt& src2)
{
  MatFlt tmp;
  Add(src1, src2, &tmp);
  return tmp;
}

MatDbl operator*(const MatDbl& src1, const MatDbl& src2)
{
  MatDbl tmp;
  MatrixMultiply(src1, src2, &tmp);
  return tmp;
}

MatFlt operator*(const MatFlt& src1, const MatFlt& src2)
{
  MatFlt tmp;
  MatrixMultiply(src1, src2, &tmp);
  return tmp;
} 

MatDbl operator-(const MatDbl& src1, const MatDbl& src2)
{
  MatDbl tmp;
  Subtract(src1, src2, &tmp);
  return tmp;
}

MatFlt operator-(const MatFlt& src1, const MatFlt& src2)
{
  MatFlt tmp;
  Subtract(src1, src2, &tmp);
  return tmp;
}

MatDbl operator-(const MatDbl& src)
{
  MatDbl tmp;
  Negate(src, &tmp);
  return tmp;
}

MatFlt operator-(const MatFlt& src)
{
  MatFlt tmp;
  Negate(src, &tmp);
  return tmp;
}

MatDbl Eye(int dim)
{
  MatDbl tmp;
  Eye(dim, &tmp);
  return tmp;
}

// set using another image:  copies entire 'src' to 'out' at location 'pt'

void Set(MatDbl* src_dst, const Point& pt, const MatDbl& src)
{
  assert(pt.x>=0 && pt.y>=0 && pt.x+src.Width()<=src_dst->Width() && pt.y+src.Height()<=src_dst->Height());
  MatDbl::ConstIterator p = src.Begin();
  MatDbl::Iterator q = src_dst->Begin(pt.x, pt.y);
  int skip = src_dst->Width() - src.Width();
  for (int y=0 ; y<src.Height() ; y++, q += skip)
  {
    for (int x=0 ; x<src.Width() ; x++)  *q++ = *p++;
  }
}

// set using another image:  copies 'rect' of 'src' to 'src_dst' at location 'pt'

void Set(MatDbl* src_dst, const Point& pt, const MatDbl& src, const Rect& rect)
{
  assert(rect.left>=0 && rect.right>=rect.left);
  assert(rect.top>=0 && rect.bottom>=rect.top);
  assert(rect.right<=src.Width() && rect.bottom<=src.Height());
  assert(pt.x+rect.Width()<=src_dst->Width() && pt.y+rect.Height()<=src_dst->Height());

  MatDbl::ConstIterator p = src.Begin(rect.left, rect.top);
  MatDbl::Iterator q = src_dst->Begin(pt.x, pt.y);
  int pskip = src.Width() - rect.Width();
  int qskip = src_dst->Width() - rect.Width();
  for (int y=0 ; y<rect.Height() ; y++, p+=pskip, q+=qskip)
  {
    for (int x=0 ; x<rect.Width() ; x++)  *q++ = *p++;
  }
}

double DotProd(const MatDbl& src1, const MatDbl& src2)
{
  assert(src1.IsVector() && src2.IsVector());
  assert(src1.Length() == src2.Length());
  const double* p1 = src1.Begin();
  const double* p2 = src2.Begin();
  double val = 0;
  while (p1 != src1.End())  val += *p1++ * *p2++;
  return val;
}

template<typename T>
void Save(const Matrix<T>& src, const char* fname, const char* fmt, bool add_comment_char)
{
  const char* eol = "\r\n";
  FILE* fp = fopen(fname, "wb");
  if(add_comment_char)
  {
    fprintf(fp, "%%%d %d%s", src.Width(), src.Height(), eol); 
  }
  else
  {
    fprintf(fp, "%d %d%s", src.Width(), src.Height(), eol); 
  }

  Matrix<T>::ConstIterator p = src.Begin();
  for (int y=0 ; y<src.Height() ; y++)
  {
    for (int x=0 ; x<src.Width() ; x++)
    {
      Matrix<T>::Element f = *p++;
      fprintf(fp, fmt, f);
    }
    fprintf(fp, "%s", eol);
  }
  fclose(fp);
}

//void Save(const MatDbl& src, const char* fname, const CString& fmt)
//{
//  FILE* fp = fopen(fname, "wb");
//  for (int y=0 ; y<src.Height() ; y++)
//  {
//    for (int x=0 ; x<src.Width() ; x++)
//    {
//      double f = src(y,x);
//      fprintf(fp, fmt, f);
//    }
//    fprintf(fp, "\r\n");
//  }
//  fclose(fp);
//}

template <typename T>
void Load(const char* fname, Matrix<T>* dst, const char* fmt, bool has_comment_char)
{
  const char* eol = "\r\n";
//  const int maxline = 2000;
//  char buff[maxline];
  CString tmp;
  int width, height;
  char comment_character;

  FILE* fp = fopen(fname, "rb");
  if (fp == NULL)  BLEPO_ERROR(CString(L"hi")); // StringEx("Unable to open file '%s'", fname));
  fpos_t ff;
  fgetpos(fp, &ff);
  if(has_comment_char)
  {
    tmp.Format(L"%%c%%d %%d%s", eol);
    fwscanf(fp, tmp, &comment_character, &width, &height);
  }
  else
  {
    tmp.Format(L"%%d %%d%s", eol);
    fwscanf(fp, tmp, &width, &height);
  }
  if (width < 0 || height < 0)  BLEPO_ERROR(StringEx("Width and height must be non-negative.  Something wrong with file '%s'.", fname));
  fgetpos(fp, &ff);
  dst->Reset(width, height);

  Matrix<T>::Iterator p = dst->Begin();
  float val;  // this needs to be a float, even if matrix is double, or else fscanf won't work
  for (int y=0 ; y<dst->Height() ; y++)
  {
    for (int x=0 ; x<dst->Width() ; x++)
    {
      fscanf(fp, fmt, &val);
  fgetpos(fp, &ff);
      *p++ = val;
    }
  }
  fclose(fp);

//  while (fgets(buff, maxline, fp))
//  {
//    assert(strlen(buff) < maxline);  // make sure we read the entire line
//    char* p = buff;
//    sscanf(p, "%f ", p);
//  }
//  fclose(fp);
}

// specializations
template void Save(const MatFlt& src, const char* fname, const char* fmt, bool add_comment);
template void Save(const MatDbl& src, const char* fname, const char* fmt, bool add_comment);
template void Load(const char* fname, MatFlt* dst, const char* fmt, bool has_comment);
template void Load(const char* fname, MatDbl* dst, const char* fmt, bool has_comment);



};  // namespace blepo
