/* 
 * Copyright (c) 2005 Clemson University.
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

#ifndef __BLEPO_UTILITIES_H__
#define __BLEPO_UTILITIES_H__

#include "time.h"     // clock_t
#include <vector>     // std::vector
#include <algorithm>  // std::sort()
#include <functional> // std::greater()

/**
@author Stan Birchfield (STB)
*/

namespace blepo
{

class Stopwatch
{
public:
  // creates and starts the stopwatch
  Stopwatch();
  virtual ~Stopwatch();

  // restarts the stopwatch
  void Restart();

  // returns the number of milliseconds elapsed since the stopwatch was last started
  // @param reset  If true, then the stopwatch is restarted
  long GetElapsedMilliseconds(bool restart);

  // display elapsed milliseconds in popup message box
  void ShowElapsedMilliseconds(bool restart);
private:
  clock_t m_start;  
};

  // Sort all the elements in a vector in ascending order
  template <typename T>
    void SortAscending(std::vector<T>* v)
  {
    std::sort(v->begin(), v->end());
  }

  // Sort all the elements in a vector in descending order
  template <typename T>
    void SortDescending(std::vector<T>* v)
  {
    std::sort(v->begin(), v->end(), std::greater<T>());
  }

  template <typename T>
  class iMyCompAscend
  {
    std::vector<T>& m_x;
  public:
    iMyCompAscend(std::vector<T>& x) : m_x(x) {}
    bool operator() (int j, int k) const { return m_x[j] < m_x[k]; }
  };

  template <typename T>
  class iMyCompDescend
  {
    std::vector<T>& m_x;
  public:
    iMyCompDescend(std::vector<T>& x) : m_x(x) {}
    bool operator() (int j, int k) const { return m_x[j] > m_x[k]; }
  };

  // Sort all the elements in a vector in ascending order
  // Also computes the original indices of the sorted elements
  template <typename T>
    void SortAscending(std::vector<T>* v, std::vector<int>* indices)
  {
    indices->resize( v->size() );
    for (int i=0 ; i < (int) v->size() ; i++) { (*indices)[i] = i; }
    std::sort(indices->begin(), indices->end(), iMyCompAscend<T>( *v ));
    std::sort(v->begin(), v->end());
  }

  // Sort all the elements in a vector in descending order
  // Also computes the original indices of the sorted elements
  template <typename T>
    void SortDescending(std::vector<T>* v, std::vector<int>* indices)
  {
    indices->resize( v->size() );
    for (int i=0 ; i<v->size() ; i++) { (*indices)[i] = i; }
    std::sort(indices->begin(), indices->end(), iMyCompDescend<T>( *v ));
    std::sort(v->begin(), v->end(), std::greater<T>());
  }

};  // end namespace blepo

#endif //__BLEPO_UTILITIES_H__
