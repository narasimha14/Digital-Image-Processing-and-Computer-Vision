/* 
 * Copyright (c) 2004 Clemson University.
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

#ifndef __BLEPO_ARRAY_H__
#define __BLEPO_ARRAY_H__

#include <assert.h>
#include <stdlib.h>  // realloc
#include <string.h>  // memcpy
#include <algorithm>  // sort
#include "Reallocator.h"

namespace blepo 
{

///////////////////////////////////////////////////////////////
// Array typedefs

template <class T>  class Array;  // forward declaration
typedef Array<int   > ArrInt;
typedef Array<float > ArrFlt;
typedef Array<double> ArrDbl;
//typedef Array<CpxFlt> ArrFx;
//typedef Array<CpxDbl> ArrDx;

typedef ArrInt SigInt;  // temporary, will have to rethink and/or remove this later -- STB
typedef ArrFlt SigFlt;  // temporary, will have to rethink and/or remove this later -- STB
typedef ArrDbl SigDbl;  // temporary, will have to rethink and/or remove this later -- STB
//typedef ArrFx  SigFx;  // temporary, will have to rethink and/or remove this later -- STB
//typedef ArrDx  SigDx;  // temporary, will have to rethink and/or remove this later -- STB

/**
@class Array

  A templated class containing a contiguous one-dimensional (1D) array of elements.  
  This class is versatile, allowing you to use it as a stack, too.  It handles all
  memory allocation/deallocation, and it automatically grows and shrinks the allocated
  memory (if desired), but in a smart way so as to achieve a reasonable tradeoff between speed and size.

  Why would you want to use this class instead of std::vector?
    - simpler, easier syntax
    - lightweight (the constructor, copy constructor, destructor, etc. of its elements are not called)
    - Begin() and End() are guaranteed to return pointers, making it easier to interface with
      legacy C code.
  Note:  Currently this class should only be used to store native types (int, float, etc.) or 
  lightweight structs whose members do not have to be initialized.  This may be changed
  in the future.

  Bug:  Need to rethink growing and shrinking.  Default behavior should be to allocate 
  exactly the right number of bytes .  Auto-growing and shrinking should have to be 
  turned on explicitly, or the result of pushing and popping, etc.  

@author Stan Birchfield (STB)
*/
template <class T>
class Array
{
public:
  typedef T* Iterator;
  typedef const T* ConstIterator;

public:
  Array() : m_autoshrink(false), m_n(0), m_data() {}
  Array(int n) : m_autoshrink(false), m_n(0), m_data() { Reset(n); }
  Array(int n, const T& value) : m_autoshrink(false), m_n(0), m_data() { Reset(n);  SetValues(value); }
  Array(const Array& other) : m_autoshrink(false), m_n(0), m_data() { *this = other; }
  virtual ~Array() {}

  /// Clears all data
  void Reset() { Reset(0); }

  /// Resizes the array; if the size is increased, then all newly allocated elements
  /// are uninitialized.  All values in [ 0,min(n1,n) ) are unchanged,
  /// where n1 is the length before calling this function -- same behavior as realloc().
  void Reset(int n) { if (n!=m_n) { iAllocate(n);  m_n = n; } }

  /// Assignment operator
  Array& operator=(const Array& other)
  {
    m_autoshrink = other.m_autoshrink;
    m_n = other.m_n;
    m_data = other.m_data;
    return *this;
  }

  /// Pushes a value onto the end of the array
  void Push(const T& value)
  {
    Reset(Len()+1);
    (*this)[Len()-1] = value;
  }

  /// Pops a value from the end of the array
  T Pop() 
  {
    assert(Len()>0);
    T value = (*this)[Len()-1];
    Reset(Len()-1);
    return value;
  }

  /// Inserts an element, shifting all following values down toward the end.
  void Insert(const T& value, int indx)
  {
    const int n = Len();
    assert(indx >= 0 && indx <= n);
    Reset(n+1);
    for (int i = n ; i > indx ; i--) {
      (*this)[i] = (*this)[i-1];
    }
    (*this)[indx] = value;
  }

  /// Deletes the element at 'indx', shifting all following values up toward the front.
  void Delete(int indx)
  {
    assert(indx>=0 && indx<Len());
    for (int i=indx ; i<Len()-1 ; i++) {
      (*this)[i] = (*this)[i+1];
    }
    Reset(Len()-1);
  }

  /// Deletes the elements in [start, end), i.e., from start to end, where start is
  /// inclusive and end is exclusive.  If end is negative, then deletes 'till the end of the array.
  void Delete(int start, int end)
  {
    if (start >= end)  return;
    if (end < 0)  end = Len();
    assert(start>=0 && start<=end && end<=Len() );
    int skip = end - start;
    for (int i=start ; i<Len()-skip ; i++) {
      (*this)[i] = (*this)[i+skip];
    }
    Reset(Len()-skip);
  }

  /// Returns the length of the array.
  int Len() const { return m_n; }

  /// Sets values in [indx_start, indx_end); indx_end<0 is the same as indx_end = Len(). 
  void SetValues(const T& value, int indx_start=0, int indx_end=-1)
  {
    if (indx_end<0)  indx_end = m_n;
    assert(indx_start <= indx_end);
    for (int i=indx_start ; i<indx_end ; i++) {
      (*this)[i] = value;
    }
  }

  /// Returns the value at 'indx'
  T& operator[](int indx) { assert(indx>=0 && indx<Len());  return m_data[indx]; }

  /// Returns the value at 'indx'
  const T& operator[](int indx) const { assert(indx>=0 && indx<Len());  return m_data[indx]; }

  /// Returns the value at 'indx'
  T& operator()(int indx) { return (*this)[indx]; }

  /// Returns the value at 'indx'
  const T& operator()(int indx) const { return (*this)[indx]; }

  /// Sets *this to a subarray of 'other'.  indx_end<0 is the same as indx_end=Len().
  /// With default arguments, the function is the same as the assignment operator.
  void Extract(const Array& other, int indx_start=0, int indx_end=-1)
  {
    if (indx_end<0)  indx_end = other.m_n;
    assert(indx_start <= indx_end);
    Reset(indx_end - indx_start);
    for (int i=indx_start, j=0 ; i<indx_end ; i++, j++) {
      (*this)[j] = other[i];
    }
  }

  /// Iterator functions
  Iterator Begin() { return m_data.Begin(); }
  ConstIterator Begin() const { return m_data.Begin(); }
  Iterator End() { return m_data.End(); }
  ConstIterator End() const { return m_data.End(); }

  void Sort(bool ascending) 
  {
	  if (ascending) 
		  std::sort( Begin(), End() );
	  else 
		  std::sort( Begin(), End(), std::greater<T>() );
  }

  /// Turns autoshrinking on (or off, which is the default).  Autoshrinking
  /// enables the array to deallocate memory that is no longer needed, but in 
  /// a smart way so as to avoid frequent deallocation.
  void AutoShrink(bool a = true) { m_autoshrink = a; }

  void operator*=(const Array& other);

  bool operator==(const Array& other)
  {
    if (m_n != other.m_n)  return false;
    if (memcmp(m_data.Begin(), other.m_data.Begin(), m_n*sizeof(T)) != 0)  return false;
    return true;
  }

  bool operator!=(const Array& other)
  {
    return !(*this == other);
  }

private:
  /// Reallocates memory, if necessary, to accommodate an array of length 'n'.
  /// (allocates exactly n)
  void iAllocate(int n)
  {
    if (m_data.GetN() != n)
    {
      m_data.Reset(n);
    }
  }
  /// Reallocates memory, if necessary, to accommodate an array of length 'n'.
  /// (allocates at least n)
  void iEnsureAllocated(int n)
  {
    int nallocated = m_data.GetN();
    int nallocated_goal = iRecommendAllocation(n, nallocated, m_autoshrink);
    if (nallocated != nallocated_goal)  iAllocate(nallocated_goal);
  }
  /// Returns the recommended number of elements to allocate in order to hold an
  /// array of length 'n', given the 'nalloc_curr' elements are currently allocated.
  /// Performs a tradeoff between memory and execution time by doubling the allocated
  /// space when the vector is growing, and halving it when the vector is shrinking
  /// (if autoshrink is true), with some hysteresis to prevent frequent reallocation.
  static int iRecommendAllocation(int n, int nalloc_curr, bool auto_shrink)
  {
    /// Compute the smallest number in {10, 20, 40, 80, 160, ...} that is >= n.
    /// If n is greater than this number, then we need to allocate more memory.
    /// Else if it is less than this number / 4, then we need to free some memory.
    /// Else we don't need to do anything.
    int upper_bound = 10;
    while (upper_bound < n)  upper_bound <<= 1;
    if (nalloc_curr < upper_bound)  return upper_bound;
    else if (auto_shrink) 
    {
      int lower_bound = (upper_bound>>2);
      if (nalloc_curr < lower_bound)  return lower_bound;
    }
    return nalloc_curr;
  }
private:
  Reallocator<T> m_data;
  int m_n;
  bool m_autoshrink;
};

///////////////////////////////////////////////////////////////
// Array definitions

template <class T>
inline void Array<T>::operator*=(const Array& other)
{
  assert(Len() == other.Len());
  for (int i=0 ; i<Len() ; i++)  (*this)[i] *= other[i];
}

///////////////////////////////////////////////////////////////
// Array functions

// Convert from one type of array to another
template <typename T, typename U>
inline void Convert(const Array<T>& src, Array<U>* dst)
{
  dst->Reset(src.Len());
  for (int i=0 ; i<dst->Len() ; i++) {
    (*dst)[i] = static_cast<const U&>( src[i] );
  }
}

template <typename T>
void Save(const Array<T>& src, const CString& fname);

inline void iPrintArray(double d, CString* out)  { out->Format(L"%10.4f", d); }
inline void iPrintArray(float d, CString* out)  { out->Format(L"%6.2f", d); }
inline void iPrintArray(int d, CString* out)  { out->Format(L"%5d", d); }

template <typename T>
inline void Print(const Array<T>& arr, CString* out)
{
  *out = "";
  const CString eol = "\r\n";
  CString tmp;
  for (int i=0 ; i<arr.Len() ; i++)
  {
    CString tmp;
    iPrintArray(arr[i], &tmp);
    *out += tmp;
  }
  *out += eol;
}

template <typename T>
inline void Display(const Array<T>& arr, const char* name = NULL)
{
  CString str;
  Print(arr, &str);
  if (name)
  {
    CString n;
    n.Format("%s =\r\n%s", name, str);
    str = n;
  }
  AfxMessageBox(str, MB_ICONINFORMATION);
}

};  // namespace blepo

#endif //__BLEPO_ARRAY_H__
