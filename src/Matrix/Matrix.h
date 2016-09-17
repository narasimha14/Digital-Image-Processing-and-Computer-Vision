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

#ifndef __BLEPO_MATRIX_H__
#define __BLEPO_MATRIX_H__

#include "assert.h"
#include "Utilities/Reallocator.h"
//#include "Utilities/Array.h"
//#include "Utilities/Math.h"  // Exchange

namespace blepo 
{

///////////////////////////////////////////////////////////////
// Matrix typedefs

template <class T>  class Matrix;  // forward declaration
//typedef Matrix<int> MatInt;
typedef Matrix<float> MatFlt;
typedef Matrix<double> MatDbl;
//typedef Matrix< CpxFlt > MatFx;
//typedef Matrix< CpxDbl > MatDx;

/**
@class Matrix

  Templated base class for a matrix (or vector), for linear algebra.  
  Data are stored contiguously in row-major format.

  Note:  In order to maintain consistency with the image class, this class is 
  constructed as (width,height) and accessed as (x,y), which is the reverse of 
  the standard linear algebra convention.  Methods beginning with 'ij' have been
  provided for those who wish to use the more traditional order.  Either way,
  accessing uses zero-based indices.

  Note:  The constructor is not called on the individual elements, so do not use a 
     class as the templated type if intialization of the class is required.

@author Stan Birchfield (STB)
*/

template <class T>
class Matrix
{
public:
  /// @name Typedefs
  //@{
  typedef T Element;
  typedef T* Iterator;
  typedef const T* ConstIterator;
  //@}

public:
  /// Constructor / destructor / copy constructor
  //@{
  explicit Matrix() : m_width(0), m_height(0), m_data(0) {}
  explicit Matrix(int width, int height) : m_width(0), m_height(0), m_data(0) { Reset(width, height); }
  explicit Matrix(int height) : m_width(0), m_height(0), m_data(0) { Reset(1, height); }
  Matrix(const Matrix& other) : m_width(0), m_height(0), m_data(0) { *this = other; }
  ~Matrix() {}
  //@}

  /// Assignment operator
  Matrix& operator=(const Matrix& other)
  {
    m_data = other.m_data;
    m_height = other.m_height;
    m_width = other.m_width;
    return *this;
  }

  /// @name Reinitialization
  /// After calling Reset(), class object will be in the exact same state as if you 
  /// were to instantiate a new object by calling the constructor with those parameters.
  /// Notice that the parameters for Reset() are identical to those for the constructor.
  //@{
  void Reset(int width, int height)
  {
    m_data.Reset(width*height);
    m_width = width;
    m_height = height;
  }
  void Reset(int height)  { Reset(1, height); }
  void Reset()            { Reset(0,0); }
  //@}

  /// Changes the dimensions of the object without changing the order of the elements.
  /// The number of elements (i.e., width*height) must be the same; otherwise this function
  /// has no effect.
  void Reshape(int width, int height)
  {
    if (m_width * m_height == width * height)
    {
      m_width = width;
      m_height = height;
    }
    else
    {
      assert(0);
    }
  }

  /// @name Matrix info
  //@{
  int Width() const   { return m_width; }  ///< Number of elements in a row
  int Height() const  { return m_height; }  ///< Number of elements in a column
  int Length() const  { return m_width * m_height; }  ///< Total number of elements (length of vector)
  int Rows() const    { return Height(); }
  int Cols() const    { return Width(); }
  bool IsVector() const       { return m_height==1 || m_width==1; }
  bool IsHorizVector() const  { return m_height==1; }
  bool IsVertVector() const   { return m_width==1; }
  //@}

  /// @name Accessing functions (inefficient but convenient)
  //@{
  const T& operator()(int i) const        { return *(Begin(i)); }  //< vector
  const T& operator()(int x, int y) const { return *(Begin(x, y)); }  //< matrix
  const T& ij(int row, int column) const  { return *(Begin(column, row)); } //< (row, column) notation
  T& operator()(int i)                    { return *(Begin(i)); }
  T& operator()(int x, int y)             { return *(Begin(x, y)); }
  T& ij(int row, int column)              { return *(Begin(column, row)); }
  //@}

  /// @name Iterator functions for fast accessing
  //@{
  Iterator Begin()                        { return m_data.Begin(); }
  Iterator Begin(int x, int y)            { assert(x>=0 && x<m_width && y>=0 && y<m_height);  return m_data.Begin()+y*m_width+x; }
  Iterator Begin(int i)                   { assert(i>=0 && i<m_width*m_height);  return m_data.Begin()+i; }
  Iterator End()                          { return m_data.End(); }
  ConstIterator Begin() const             { return m_data.Begin(); }
  ConstIterator Begin(int x, int y) const { assert(x>=0 && x<m_width && y>=0 && y<m_height);  return m_data.Begin()+y*m_width+x; }
  ConstIterator Begin(int i) const        { assert(i>=0 && i<m_width*m_height);  return m_data.Begin()+i; }
  ConstIterator End() const               { return m_data.End(); }
  Iterator ijBegin(int row, int column)             { return Begin(column, row); }
  Iterator ijBegin(int i)                           { return Begin(i); }
  ConstIterator ijBegin(int row, int column) const  { return Begin(column, row); }
  ConstIterator ijBegin(int i) const                { return Begin(i); }
  //@}

  /// Flip a horizontal vector to vertical, and vice versa
  void FlipVector()
  {
    assert(IsVector());
    int tmp = m_height;  m_height = m_width;  m_width = tmp;
  }

private:
  int m_width, m_height;
  Reallocator<T> m_data;
};

};  // namespace blepo


#endif //__BLEPO_MATRIX_H__
