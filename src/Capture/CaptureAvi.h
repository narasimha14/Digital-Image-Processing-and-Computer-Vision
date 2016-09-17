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

#ifndef __BLEPO_CAPTUREAVI_H__
#define __BLEPO_CAPTUREAVI_H__

#include <afx.h>  // CString
// #include <vector>
#include "Image/Image.h"
#include "Utilities/Array.h"
#include "Utilities/Mutex.h"
#include "Utilities/Exception.h"

/**
@class CaptureAvi
This class enables you to open an AVI file and extract frames from it.
The class uses DirectShow.

@bug  This class is in really bad shape.  You might be able to open an AVI file
and extract some frames from it, but no guarantees.  I never finished writing the
class, because I found a freeware program, VirtualDub, that will do this.

@author Stan Birchfield (STB)
*/

namespace blepo
{

class CaptureAvi
{
public:
  CaptureAvi();
  virtual ~CaptureAvi();

  bool BuildGraph(const char* fname);
  void TearDownGraph();

protected:

  struct Time
  {
    unsigned int seconds, nanoseconds;
    // returns time in milliseconds
    unsigned int GetMS() const { return (seconds*1000) + (nanoseconds / 1000000); };
  };
  friend class CaptureAviVideoCallback;  ///< so it can call OnNewVideoFrame
  virtual void OnNewVideoFrame(const unsigned char* data, int width, int height, Time timestamp);

private:
  void* m_graph;  ///< this is void* to avoid header leak
  ImgBgr m_img;
  Mutex m_mutex;
  Semaphore m_semaphore;

  class UpsideDownBgrImage
  {
  public:
    void StoreCopy(int width, int height, const unsigned char* data);
    void ToBgr(ImgBgr* out) const;
  private:
    int m_width, m_height;
    Array<unsigned char> m_data;
  };
  UpsideDownBgrImage m_upsidedown_image;
};


};  // end namespace blepo

#endif //__BLEPO_CAPTUREAVI_H__
