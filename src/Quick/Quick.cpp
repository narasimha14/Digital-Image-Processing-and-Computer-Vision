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

#include "Quick.h"


// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

// ================> begin local functions (available only to this translation unit)
namespace{
bool i_can_do_mmx;
bool i_can_do_sse;
bool i_can_do_sse2;
bool i_can_do_sse3;
//extern "C" int InstructionSet();
extern "C" int mmx_supported();
inline void iInit()
{
  static bool initialized = false;
  if (!initialized) 
  {
//    int cpu_info = InstructionSet();
    int cpu_info = mmx_supported();
    i_can_do_mmx = cpu_info >= 1;
    i_can_do_sse = cpu_info >= 3;
    i_can_do_sse2 = cpu_info >= 4;
    i_can_do_sse3 = cpu_info >= 5;
    initialized = true;
  }
}
};
// ================< end local functions

namespace blepo
{

bool CanDoMmx() { iInit();  return i_can_do_mmx; }
bool CanDoSse() { iInit();  return i_can_do_sse; }
bool CanDoSse2() { iInit();  return i_can_do_sse2; }
bool CanDoSse3() { iInit();  return i_can_do_sse3; }

void TurnOffAllMmxSse()
{
  iInit();
  i_can_do_mmx = i_can_do_sse = i_can_do_sse2 = i_can_do_sse3 = false;
}

};  // end namespace blepo

