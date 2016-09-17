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

#ifndef __BLEPO_TRIPLEBUFFER_H__
#define __BLEPO_TRIPLEBUFFER_H__

#include "Utilities/System/Mutex.h"

/**
@class TripleBuffer

  Class to hold a triple buffer, for capturing data.  The normal way
  of using this class is to have two threads, one for reading and one
  for writing, using it simultaneously.  The writing thread calls 
  BeginWrite() to start writing data to an unused buffer, and it calls
  EndWrite() to indicate that it has successfully finished writing to 
  the buffer.  The reading thread calls ReadLatest() whenever it's ready
  to read the latest available buffer that has been successfully written
  to.  If there is no such buffer, then the function returns NULL.

  The class uses a mutex to keep everything thread-safe.  If you call
  BeginWrite() multiple times in a row, without also calling EndWrite(),
  then you will just keep writing to the same buffer.  Reading is a little
  different, however.  If you call ReadLatest() and the latest available
  data is in the buffer you're already reading, then it returns NULL.

  This is a templated class.  Instantiate it with the type of data to
  be stored in the buffer, along with the length of the buffers
  (all buffers must be the same size).

@author Stan Birchfield (STB)
*/

namespace blepo
{

typedef <class T>
class TripleBuffer {
public:
  /// Constructs object and allocates memory for buffers
  /// @param nelements Number of elements in each buffer
  TripleBuffer(int nelements)
    : m_indx_write(NONE), m_indx_read(NONE), m_indx_latest(NONE)
  {
    for (int i=0 ; i<m_nbuffers ; i++)  m_buff_ptr[i] = new T[nelements];
    m_allocated_buffers_ourselves = true;
  }
  
  /// Constructs object using external buffers
  TripleBuffer(T* p0, T* p1, T* p2)
    : m_indx_write(NONE), m_indx_read(NONE), m_indx_latest(NONE)
  {
    m_buff_ptr[0] = p0;
    m_buff_ptr[1] = p1;
    m_buff_ptr[2] = p2;
    m_allocated_buffers_ourselves = false;
  }
  
  ~TripleBuffer()
  {
    if (m_allocated_buffers_ourselves)
    {
      for (int i=0 ; i<m_nbuffers ; i++)  delete[] m_buff_ptr[i];
    }
  }

  /// Returns a pointer to an unused buffer so you can begin writing to it
  T* StartWriting()
  {
    if (m_indx_write == NONE)
    {
      // set write to an index that's not currently being used by read
      // or latest
      m_indx_write = (m_indx_read != 0 && m_indx_latest != 0)
                      ? 0
                      : ((m_indx_read != 1 && m_indx_latest != 1)
                          ? 1 : 2;
    }
    return m_buff_ptr[m_indx_write];
  }        

  /// Marks the buffer to which you have been writing as successfully
  /// finished, so that it's ready for reading.
  void EndWriting()
  {
    if (m_indx_write != NONE)
    {
      m_indx_latest = m_indx_write;
      m_indx_write = NONE;
    }
  }

  /// Returns a pointer to the latest buffer containing valid data,
  /// or NULL if there is no such buffer (or if you already have 
  /// the pointer to the latest buffer, through a previous call
  /// to ReadLatest).
  const T* ReadLatest()
  {
    if ((m_indx_latest == NONE) || (m_indx_latest == m_indx_read))
    {
      return NULL;
    }
    else
    {
      m_indx_read = m_indx_latest;
      return m_buff_ptr[m_indx_read];
    }
  }

private:
  static const int NONE = -1;  ///< indicates an invalid index
  static const int m_nbuffers = 3;  ///< this is a triple buffer class, so there are three buffers (don't change this number!)
  bool m_allocated_buffers_ourselves;  ///< whether buffers were allocated blepo_exly
  T* m_buff_ptr[m_nbuffers];  ///< pointers to actual buffers
  int m_indx_write;  ///< index of the current buffer being written to
  int m_indx_read;  ///< index of the current buffer being read from
  int m_indx_latest;    ///< index of the buffer that was written to last (if there is one)
  Mutex m_mutex;
};

};  // end namespace blepo

#endif //__BLEPO_TRIPLEBUFFER_H__
