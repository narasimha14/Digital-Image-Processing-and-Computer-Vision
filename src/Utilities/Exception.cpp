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

#include "Exception.h"
#include <assert.h>

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

	Exception::Exception()
		: m_line_number(-1), m_filename(), m_message()
	{
	}

	Exception::Exception(int line, const CString& filename, const CString& message)
		: m_line_number(line), m_filename(filename), m_message(message)
	{
	}

	void Exception::Display() const
	{
		CString str;
		str.Format(L"Error:  %s\r\n\r\n(Line %d of %s)",
			m_message,
			m_line_number,
			m_filename);
		AfxMessageBox(str, MB_ICONEXCLAMATION);
	}

	CString StringEx(const char* format, ...)
	{
		va_list arglist;
		va_start(arglist, format);
		const int n = 1000;
		char buff[n];
		int ret = _vsnprintf(buff, n, format, arglist);
		if (ret == -1 || ret == n)
		{
			// Note:  we could easily remove this error by repeatedly allocating more memory
			// for buff until the string was not too long.
			BLEPO_ERROR("string too long");
		}
		va_end(arglist);
		CString s;
		s = buff;
		return s;
		//return CString(buff);
	}


	/*  The code in this comment block is working code.  It can be used to get around
	the syntax restrictions of C macros, so that BLEPO_ERROR accepts a variable number
	of arguments.  We have commented it out for now, because the ratio of benefit to
	added complication is really small.  We do not want to intimidate unnecessarily
	people who are trying to learn their way around the library.
	Once the library is more well-established, we can revisit whether it is worth the
	tradeoff of ugly code here to enable cleaner syntax by those using BLEPO_ERROR.  -- STB 9/24/04

	  Header file ===============================>

	#include "Mutex.h"

	// This ugly class enables us to get around syntax limitations of C macros.
	// It is used by BLEPO_ERROR below, and should be used by no one else.
	struct ExceptionBlepoError : public Exception
	{
	public:
	  /// @param message_format  Format for message, with printf syntax and
	  ///        a variable number of arguments following
	  ExceptionBlepoError(const char* message_format, ...);

	  static void SetLineAndFile(int line, const char* filename);

	private:
	  // static members
	  static int ms_line_number;  ///< line number from which exception thrown
	  static CString ms_filename;  ///< name of file from which exception thrown
	  static Mutex ms_mutex;  ///< necessary to make this class thread-safe
	};

	/// Call this macro when an error occurs that is not the result of blepo_ex
	/// inconsistency (use assert() in that case).  This macro throws an Exception.
	/// Caller should catch with catch (const Exception& e).  Ignore the fact that
	/// it is an ExceptionBlepoError.  This is only to enable variable arguments
	/// being passed to the BLEPO_ERROR macro.
	#define BLEPO_ERROR  ExceptionBlepoError::SetLineAndFile(__LINE__, __FILE__);  throw ExceptionBlepoError


	  cpp file ======================================>

	int ExceptionBlepoError::ms_line_number;
	CString ExceptionBlepoError::ms_filename;
	Mutex ExceptionBlepoError::ms_mutex;

	void ExceptionBlepoError::SetLineAndFile(int line, const char* filename)
	{
	  ms_mutex.Lock();
	  ms_line_number = line;
	  ms_filename = filename;
	}

	ExceptionBlepoError::ExceptionBlepoError(const char* message_format, ...)
	  : Exception()
	{
	  m_line_number = ms_line_number;
	  m_filename = ms_filename;
	  ms_mutex.Unlock();

	  // parse variable arguments
		va_list arglist;
		va_start(arglist, message_format);
	  const int n = 1000;
	  char buff[n];
	  int ret = _vsnprintf(buff, n, message_format, arglist);
	  if (ret == -1 || ret == n)
	  {
		// Note:  we could easily remove this error by repeatedly allocating more memory
		// for buff until the string was not too long.
		BLEPO_ERROR("string too long");
	  }
		va_end(arglist);
	  m_message = buff;
	}
	*/

};  // end namespace blepo

