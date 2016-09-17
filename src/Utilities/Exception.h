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

#ifndef __BLEPO_EXCEPTION_H__
#define __BLEPO_EXCEPTION_H__

#include <assert.h>
#include <afxwin.h>  // CString

/**
@struct Exception

Exception struct, used primarily for errors.  To use, call BLEPO_ERROR(msg)
when an error is encountered, where 'msg' is a human-readable string describing the 
error.  BLEPO_ERROR is a macro which calls throw Exception with the filname and
line number from which the exception is thrown.  Catch the exception and display it
to the user using
@code
catch (const Exception& e)
{
  e.Display();
}
@endcode

@author Stan Birchfield (STB)
*/

namespace blepo
{

struct Exception
{
public:
  int m_line_number;  ///< line number from which exception thrown
  CString m_filename;  ///< name of file from which exception thrown
  CString m_message;  ///< human-readable description of error encountered

public:
  Exception();
  Exception(int line, const CString& filename, const CString& message);
  virtual ~Exception() {}

  void Display() const;

};

// Creates a CString using a variable number of arguments, in printf style
CString StringEx(const char* format, ...);

};  // end namespace blepo

/// Call this macro when an error occurs that is not the result of blepo_ex
/// inconsistency (use assert() in that case).  This macro throws an Exception.
/// Caller should catch with catch (const Exception& e).  
/// The assert() helps pinpoint the reason for the error, using just-in-time debugging.
// #define BLEPO_ERROR(msg)  { assert("BLEPO_ERROR" && 0);  throw Exception(__LINE__, __FILE__, msg); }
#define BLEPO_ERROR(msg)  { throw Exception(__LINE__, __FILE__, msg); }

#endif //__BLEPO_EXCEPTION_H__

