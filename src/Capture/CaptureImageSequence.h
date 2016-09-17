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

#ifndef __BLEPO_CAPTUREIMAGESEQUENCE_H__
#define __BLEPO_CAPTUREIMAGESEQUENCE_H__

#include <afx.h>  // CString
//#include <vector>
#include "Image/Image.h"
//#include "Utilities/Array.h"
//#include "Utilities/Mutex.h"
//#include "Utilities/Exception.h"

/**
@class CaptureImageSequence

  Class to capture video from a sequence of images.

  @author Stan Birchfield (STB)
*/

namespace blepo
{

class CaptureImageSequence
{
public:
  /// Constructor
  /// filename_format:  Specifies format of the filename, in printf style for substituting the image frame number as an integer
  /// start_frame:  index of first image frame
  /// end_frame:  index of last image frame (or -1 to read until file does not exist)
  ///
  /// Example:  CaptureImageSequence("C:/mydir/myfile%03d.jpg", 0, 100);
  ///
  CaptureImageSequence(const CString& filename_format, int start_frame=0, int end_frame=-1);

  /// Destructor
  virtual ~CaptureImageSequence();

  bool GetNextImage(ImgBgr* out);
  bool GetNextImage(ImgGray* out);

  int GetFrameNumber() const { return m_frame; }

private:
  CString m_filename_format;
  int m_start_frame, m_end_frame;
  int m_frame;
};

};  // end namespace blepo

#endif //__BLEPO_CAPTUREIMAGESEQUENCE_H__
