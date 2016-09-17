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

#include "Utilities.h"
#include <afxwin.h>  // CString
#include <BaseTsd.h>

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

};
// ================< end local functions

// System clock is accurate to about 10 ms.
// For accuracy on the order of tens of ns, see
//    QueryPerformanceFrequency, QueryPerformanceCounter functions in MSDN.

namespace blepo
{

  Stopwatch::Stopwatch()
{
  Restart();
}

Stopwatch::~Stopwatch() {}

void Stopwatch::Restart()
{
  m_start = clock();
}

long Stopwatch::GetElapsedMilliseconds(bool restart)
{
  clock_t now = clock();
  //TRACE("start %d now %d\n", m_start, now);
  long elapsed = (1000 * (now - m_start)) / CLOCKS_PER_SEC;
  if (restart)  Restart();
  return elapsed;
}

void Stopwatch::ShowElapsedMilliseconds(bool restart)
{
  long a = GetElapsedMilliseconds(false);
  CString str;
  str.Format(L"Elapsed time:  %d ms", a);
  AfxMessageBox(str);
  if (restart)  Restart();
}


};  // end namespace blepo

