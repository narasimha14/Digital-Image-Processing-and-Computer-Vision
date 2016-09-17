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

#include "Matrix.h"

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

//template <typename T>
//void Matrix<T>::TransposeInPlace() 
//{
//  if (IsVector()) 
//  {  // vector, so no need to change the data
//    blepo_ex::Exchange(m_height, m_width);
//
//  } else if (m_height==m_width)
//  {
//    // matrix is square, so just exchange elements
//    for (int y=0 ; y<m_height ; y++) 
//    {
//      for (int x=y+1 ; x<m_width ; x++) 
//      {
//        blepo_ex::Exchange((*this)(x,y), (*this)(y,x));
//      }
//    }
//  } else 
//  {
//    // matrix is non-square, so do it the slow way
//    Matrix<T> tmp;
//    Transpose(*this, &tmp);
//    *this = tmp;
//  }
//}

};  // namespace blepo
