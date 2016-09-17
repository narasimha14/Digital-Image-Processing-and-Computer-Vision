/* 
 * Copyright (c) 2011 Clemson University.
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

#include "CaptureImageSequence.h"
//#include "Utilities/Math.h"
#include "Image/ImageOperations.h"  // Load

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

using namespace blepo;

namespace blepo
{

CaptureImageSequence::CaptureImageSequence(const CString& filename_format, int start_frame, int end_frame)
: m_filename_format(filename_format),
  m_start_frame(start_frame),
  m_end_frame(end_frame),
  m_frame(start_frame)
{
  assert( end_frame == -1 || start_frame <= end_frame );
}

CaptureImageSequence::~CaptureImageSequence()
{
}

bool CaptureImageSequence::GetNextImage(ImgBgr* out)
{
  if (m_end_frame >=0 && m_frame > m_end_frame)  return false;
  CString str;
  str.Format(m_filename_format, m_frame++);
  try
  {
    Load(str, out);
    return true;
  } 
  catch (Exception& )
  {
    return false;
  }
}

bool CaptureImageSequence::GetNextImage(ImgGray* out)
{
  if (m_end_frame >=0 && m_frame > m_end_frame)  return false;
  CString str;
  str.Format(m_filename_format, m_frame++);
  try
  {
    Load(str, out);
    return true;
  } 
  catch (Exception& )
  {
    return false;
  }
}

};