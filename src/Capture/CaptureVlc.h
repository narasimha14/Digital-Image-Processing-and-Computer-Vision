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

#ifndef __BLEPO_CAPTUREVLC_H__
#define __BLEPO_CAPTUREVLC_H__

#include <afx.h>  // CString
#include <vector>
#include "Image/Image.h"
#include "Utilities/Array.h"
#include "Utilities/Mutex.h"
#include "Utilities/Exception.h"

/**
@class CaptureImageSequence

  Class to capture video from a sequence of images.

  Bugs:
    Temp image filename is fixed, so multiple instances of class would collide with each other.
    Some checks not in place
    If VLC has not been installed or is in another location, may not properly notify you
    Interface not user friendly if you want to do something other than grab frames.
    VLC application displays on screen, along with VLC video window (may be able to disable using --vout)

  @author Stan Birchfield (STB)
*/

namespace blepo
{

class CaptureVlc
{
public:
  /// Constructor
  CaptureVlc();

  /// Destructor
  virtual ~CaptureVlc();

  // Set the name of the video source (e.g., filename)
  void SetInputSource(const char* source);

  // Get / Set VLC executable filename (with full path)
  CString GetVlcExecutableName() const;
  void SetExecutableVlcName(const char* vlc_path);

  // Get / Set command line parameters (for fine-tuned control over program, not necessary for average user)
  CString GetVlcCmdLineParams() const;
  void SetVlcCmdLineParams(const char* vlc_params);

  // 'use_input_source':  If true, then the input source (set by SetInputSource) is appended to the command line parameters
  void Start(bool use_input_source = true);
  void Stop();

  bool IsRunning() const;

  bool GetLatestImage(ImgBgr* out);
  bool GetLatestImage(ImgGray* out);

private:
  CString m_vlc_path, m_vlc_params;
  CString m_input_source;
  bool m_is_running;
  struct ProcessStuff;
  ProcessStuff* m_stuff;  ///< this is a d-pointer (opaque pointer) to avoid header leak
};

};  // end namespace blepo

#endif  // __BLEPO_CAPTUREVLC_H__