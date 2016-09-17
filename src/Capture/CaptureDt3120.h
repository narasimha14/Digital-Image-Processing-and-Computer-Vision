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

#ifndef __BLEPO_CAPTUREDT3120_H__
#define __BLEPO_CAPTUREDT3120_H__

#include "../Image/Image.h"

/**
@class CaptureDt3120
A class to grab video frames from Data Translation's 3120 PCI frame grabber card.

@author Stan Birchfield (STB)
*/

namespace blepo
{

class CaptureDt3120
{
public:
  /** Connect to the framegrabber, and set up for capturing images.
      @param image_size:  Indicates the size of the image.
                          possible values:
                                1 = full size (640 x 480)
                                2 = half size (320 x 240)
                                4 = quarter size (160 x 120)
      @param color:  True for grabbing BGR images, false for grabbing Gray
  */
  CaptureDt3120(int image_size, bool color);
  virtual ~CaptureDt3120();

  /// Read one image frame from incoming video.  Call this function if in color mode.
  void ReadOneFrame(ImgBgr* out);
  /// Read one image frame from incoming video.  Call this function if in grayscale mode.
  void ReadOneFrame(ImgGray* out);

private:
  /// Read one frame into a buffer that has already been allocated.
  void iReadOneFrame(unsigned char* p);

private:
  int m_width;
  int m_height;
  int m_depth;  ///< pixel depth:  1=mono, 3=24-bit color
  void* m_stuff;  ///< this is void* to avoid header leak
};


};  // end namespace blepo

#endif //__BLEPO_CAPTUREDT3120_H__
