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

#ifndef __BLEPO_MATRIXOPERATIONS_H__
#define __BLEPO_MATRIXOPERATIONS_H__

#include "Matrix.h"
#include "Utilities/PointSizeRect.h"

///////////////////////////////////////////////////////////////
// This file contains basic operations on a matrix.

namespace blepo 
{

// Set all elements to constant value
void Set(MatDbl* out, double val);
void Set(MatFlt* out, float val);
// set all elements inside 'rect' to constant value
//void Set(MatDbl* out, const Rect& rect, MatDbl::Pixel val);
// set all elements outside 'rect' to constant value
//template <typename T> void SetOutside(Image<T>* out, const Rect& rect, double val);
// set using another matrix:  copies entire 'src' to 'src_dst' at location 'pt'
void Set(MatDbl* src_dst, const Point& pt, const MatDbl& src);
// set using another image:  copies 'rect' of 'img' to 'out' at location 'pt'
void Set(MatDbl* src_dst, const Point& pt, const MatDbl& src, const Rect& rect);
// extract 'rect' of elements from 'img' to 'out'
//void Extract(const MatDbl& img, const Rect& rect, MatDbl* out);

// Create identity matrix
void Eye(int dim, MatDbl* mat);
void Eye(int dim, MatFlt* mat);

// Creates a diagonal matrix if 'mat' is a vector;
// Else extracts the diagonal from the matrix.
void Diag(const MatDbl& mat, MatDbl* out);

// Create random matrix with uniform distribution in [0,1)
void Rand(int width, int height, MatDbl* out);

// arithmetic operations ('inplace' allowed)
void Add(const MatDbl& src1, const MatDbl& src2, MatDbl* dst);
void Add(const MatFlt& src1, const MatFlt& src2, MatFlt* dst);
void Subtract(const MatFlt& src1, const MatFlt& src2, MatFlt* dst);
void Subtract(const MatDbl& src1, const MatDbl& src2, MatDbl* dst);
// arithmetic operations with constant value ('inplace' allowed)
void Add(const MatDbl& src, double val, MatDbl* dst);
void Subtract(const MatDbl& src, double val, MatDbl* dst);
void Multiply(const MatDbl& src, double val, MatDbl* dst);
// multiply by -1
void Negate(const MatDbl& src, MatDbl* dst);
void Negate(const MatFlt& src, MatFlt* dst);

bool Similar(const MatDbl& src1, const MatDbl& src2, double tolerance);

// returns the Euclidean norm of a vector
double Norm(const MatDbl& src);

// returns the dot product, or scalar product, of two vectors.
// Both inputs must be vectors
double DotProd(const MatDbl& src1, const MatDbl& src2);


template <class T>
inline bool IsSameSize(const Matrix<T>& src1, const Matrix<T>& src2)  
{ return src1.Width()==src2.Width() && src1.Height()==src2.Height(); }

// multiply
//void Multiply(const MatDbl& src1, const MatDbl& src2, MatDbl* dst);

//void operator+=(MatDbl& io, const MatDbl& src2);
//void operator-=(MatDbl& io, const MatDbl& src2);

// 'inplace' is NOT allowed
template <class T>
void MatrixMultiply(const Matrix<T>& a, const Matrix<T>& b, Matrix<T>* out)
{
  if (&a == out || &b == out)  BLEPO_ERROR("Matrix multiply cannot be done inplace");
  if (a.Width() != b.Height())  BLEPO_ERROR("Matrix dimensions inconsistent");
  int height = a.Height();
  int width = b.Width();
  int nk = a.Width();
  out->Reset(b.Width(), a.Height());
  for (int y=0 ; y<height ; y++) {
    for (int x=0 ; x<width ; x++) {
      T val = 0;
      for (int k=0 ; k<nk ; k++)  val += a(k,y) * b(x,k);
      (*out)(x,y) = val;
    }
  }
}

template <class T>
void MultiplyElements(const Matrix<T>& src1, const Matrix<T>& src2, Matrix<T>* dst)
{
  if (!IsSameSize(src1, src2))  BLEPO_ERROR("Matrices must be same size");
  dst->Reset(src1.Width(), src1.Height());
  MatDbl::ConstIterator p1 = src1.Begin();
  MatDbl::ConstIterator p2 = src2.Begin();
  MatDbl::Iterator q = dst->Begin();
  while (p1 != src1.End())  *q++ = (*p1++) * (*p2++);
}

template <class T>
void Transpose(const Matrix<T>& src, Matrix<T>* dst)
{
  dst->Reset(src.Height(), src.Width());
  Matrix<T>::ConstIterator pi = src.Begin();
  for (int y=0 ; y<src.Height() ; y++)
  {
    Matrix<T>::Iterator po = dst->Begin(y, 0);
    for (int x=0 ; x<src.Width() ; x++)
    {
      *po = *pi++;
      po += dst->Width();
    }
  }
}

/// Fast in-place transpose for vectors and square matrices.
/// If this is a non-square matrix, then the computation will be no faster
/// than Transpose().
/// Bug:  Need to remove this interface.  Just overload the previous function;
/// check whether input and output are the same; if so, then do it inplace.
template <class T>
inline void Transpose(Matrix<T>* mat)
{
  if (mat->IsVector()) 
  {  
    // vector, so no need to change the data
    mat->FlipVector();
  } 
  else if (mat->Height() == mat->Width())
  {
    // matrix is square, so just exchange elements
    for (int y=1 ; y<mat->Height() ; y++) 
    {
      Matrix<T>::Iterator p1 = mat->Begin(y, y-1);
      Matrix<T>::Iterator p2 = mat->Begin(y-1, y);
      for (int x=y-1 ; x>=0 ; x--)
      {
        T tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1 -= mat->Height();
        p2--;
      }
    }
  } else 
  {
    // matrix is non-square, so do it the slow way
    Matrix<T> tmp = *mat;
    Transpose(tmp, mat);
  }
}

template <class T>
T Sum(const Matrix<T>& src)
{
  T sum = 0;
  for (const T* p = src.Begin() ; p != src.End() ; p++)  sum += *p;
  return sum;
}

//@name Simple syntax functions.
// These functions enable matrix operations to be combined with a more natural,
// Matlab-like syntax, e.g., b = a * c + Transpose(e);
// Keep in mind, though, that they are particularly inefficient due to their use
// of temporary variables and return by value. 
//@{
MatDbl Diag(const MatDbl& mat); 
MatDbl Transpose(const MatDbl& mat); 
MatDbl operator+(const MatDbl& src1, const MatDbl& src2); 
MatFlt operator+(const MatFlt& src1, const MatFlt& src2);
MatDbl operator*(const MatDbl& src1, const MatDbl& src2); 
MatFlt operator*(const MatFlt& src1, const MatFlt& src2);
MatDbl operator-(const MatDbl& src1, const MatDbl& src2); 
MatFlt operator-(const MatFlt& src1, const MatFlt& src2);
MatDbl operator-(const MatDbl& src);
MatFlt operator-(const MatFlt& src); 
MatDbl Eye(int dim);
//@}

template <typename T, typename U>
void Convert(const Matrix<T>& src, Matrix<U>* dst)
{
  dst->Reset(src.Width(), src.Height());
  Matrix<T>::ConstIterator pi = src.Begin();
  Matrix<U>::Iterator po = dst->Begin();
  for (int i=src.Width()*src.Height() ; i>0 ; i--)
  {
    *po++ = static_cast<const U&>( *pi++ );
  }
}

template <typename T>
void Print(const Matrix<T>& mat, CString* out, const CString& fmt = L"%10.4f ")
{
  *out = "";
  const CString eol = "\r\n";
  CString tmp;
  for (int y=0 ; y<mat.Height() ; y++)
  {
    for (int x=0 ; x<mat.Width() ; x++)
    {
      tmp.Format(fmt, mat(x,y));
      *out += tmp;
    }
    *out += eol;
  }
}

template <typename T>
void Display(const Matrix<T>& mat, const char* name = NULL)
{
  CString str;
  Print(mat, &str);
  if (name)
  {
    CString n;
    n.Format(L"%s =\r\n%s", name, str);
    str = n;
  }
  AfxMessageBox(str, MB_ICONINFORMATION);
}

template <typename T>
void Save(const Matrix<T>& src, const char* fname, const char* fmt = "%10.4f ", bool add_comment_char = false);

template <typename T>
void Load(const char* fname, Matrix<T>* dst, const char* fmt = "%f ", bool has_comment_char = false);

};  // namespace blepo


#endif // __BLEPO_MATRIXOPERATIONS_H__
