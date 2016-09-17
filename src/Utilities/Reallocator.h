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

#ifndef __BLEPO_REALLOCATOR_H__
#define __BLEPO_REALLOCATOR_H__

//#include <assert.h>
#include "Exception.h"
#include <stdlib.h>  // realloc()

/**
@class Reallocator
Reallocates an array of basic types or structs, 
without calling the constructor on those types.

@author Stan Birchfield (STB)
*/

namespace blepo
{

template <class T>
class Reallocator
{
public:
  typedef T Type;
  Reallocator() : m_nalloc(0), m_first(0), m_last(0) {}
  Reallocator(int n) : m_nalloc(0), m_first(0), m_last(0) { Reset(n); }
  Reallocator(const Reallocator& other) : m_nalloc(0), m_first(0), m_last(0) { *this = other; }
  virtual ~Reallocator() { free(m_first); }
  void Reset() { Reset(0); }
  void Reset(int n)
  {
    if (m_nalloc == 0 && n == 0)  return;  // because realloc(0, 0) does not return 0; could be confusing to have non-NULL pointer when array is empty
    m_first = static_cast<T*>( realloc(m_first, n*sizeof(T)) );
    m_last = m_first + n;
    m_nalloc = n;
    // If we tried to allocate memory, make sure we did
    if (m_nalloc>0 && m_first==0)  BLEPO_ERROR("Out of memory");  
  }

  Reallocator& operator=(const Reallocator& other)
  {
    Reset(other.m_nalloc);
    memcpy(m_first, other.m_first, m_nalloc*sizeof(T));
    return *this;
  }

  int GetN() const { return m_nalloc; }

  T& operator[](int indx) { assert(indx>=0 && indx<GetN());  return m_first[indx]; }
  const T& operator[](int indx) const { assert(indx>=0 && indx<GetN());  return m_first[indx]; }

  Type* Begin() { return m_first; }
  Type* End() { return m_last; }
  const Type* Begin() const { return m_first; }
  const Type* End() const { return m_last; }

private:
  int m_nalloc;      //< number of elements
  T* m_first;  //< points to first element
  T* m_last;   //< points just past last element
};


};  // end namespace blepo

#endif //__BLEPO_REALLOCATOR_H__
