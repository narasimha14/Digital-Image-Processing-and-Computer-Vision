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

#ifndef __BLEPO_CAPTUREIEEE1394_H__
#define __BLEPO_CAPTUREIEEE1394_H__

#include "Image/Image.h"
#include "Utilities/Mutex.h"

/**
@class CaptureIEEE1394

  This class enables you to capture from an IEEE1394 (Firewire) camera.  It is built around
  Carnegie Mellon's code ( http://www.cs.cmu.edu/~iwan/1394/ ), and should therefore work with any OHCI-compliant camera.  It will
  probably NOT work with DV cameras.

  Before using this class, first set the driver to the CMU driver:
    1.  plug in the IEEE1394 camera
    2.  go to Control Panel -> System -> Hardware -> Device Manager -> Imaging Devices
    3.  right click on 1394 camera and choose Update driver...
    4.  manually select blepo/external/inf/IEEE1394 directory (do not let Windows search for it)
        On WindowsXP, the sequence of mouseclicks to do this is as follows:
          - Install from a list or specific location (Advanced)
          - Don't search.  I will choose the driver to install.
          - Have disk...
          - Browse to blepo/external/IEEE1394 directory
    5.  Select the driver (ignore the fact that it is not digitally signed)
  
  If you have any trouble, see the help in blepo/external/doc/IEEE1394
  After installing the driver, you may test your camera using Blepo code or the CMU test
  application in blepo/external/bin/1394CameraDemo.exe

@author Stan Birchfield (STB)
*/

namespace blepo
{

class CaptureIEEE1394
{
public:
  // 'Fps' is number of frames per second
  typedef enum { FPS_2, FPS_4, FPS_7, FPS_15, FPS_30, FPS_60 } Fps;

  // 'Format' contains the image size and color packing.  Note that the 
  // color packing only affects the bytes transferred from the camera to 
  // the computer; once at the computer they are converted to BGR.  Probably
  // only developers need to concern themselves with the color packing.
  typedef enum {  FMT_160x120_YUV444, 
                  FMT_320x240_YUV422, 
                  FMT_640x480_YUV411, 
                  FMT_640x480_YUV422,
                  FMT_640x480_RGB,
                  FMT_640x480_MONO,
                  FMT_800x600_YUV422,
                  FMT_800x600_RGB,
                  FMT_800x600_MONO,
                  FMT_1024x768_YUV422,
                  FMT_1024x768_RGB,
                  FMT_1024x768_MONO,
                  FMT_1280x960_YUV422,
                  FMT_1280x960_RGB,
                  FMT_1280x960_MONO,
                  FMT_1600x1200_YUV422,
                  FMT_1600x1200_RGB,
                  FMT_1600x1200_MONO } Format;
public:
  CaptureIEEE1394();
  virtual ~CaptureIEEE1394();

  static int GetNumCameras();

  // Select and initialize camera (index is zero-based).
  // Must call this before calling any other function.
  void SelectCamera(int index);

  // Returns whether the camera supports a given format
  bool IsFormatSupported(Format fmt, Fps fps);

  // Sets the format for the camera.
  // 'autostuff':  if true, then sets white balance, brightness, exposure, etc. to automatic mode
  bool SetFormat(Format fmt, Fps fps, bool autostuff);  //Returns 1 for success and 0 for fail

  void Start();
  void Stop();
  void Pause();

  /**
  Once the camera is running, frames will be grabbed continuously at the
  frame rate, automatically.  Call this function to get access to the latest 
  frame that has been grabbed.
  @param 'timeout'  in milliseconds (-1 for infinite, 0 for immediate return)
  @returns true if a new frame is available since the last time this function
  was called, false otherwise
  */
  bool GetLatestImage(ImgBgr* out, int timeout = 0);

protected:
  void ConvertFormat(Format fmt, Fps fps, int* format, int* mode, int* rate);
  void SetAutoParams();
  void InitCamera();
  void GrabFrame();
  
private:
  class CamThread : public Thread
  {
  public:
    CamThread(CaptureIEEE1394* camera) : m_camera(camera) {}
    virtual unsigned long MainRoutine();
  private:
    CaptureIEEE1394* m_camera;
  };
  friend CamThread;

private:
  ImgBgr m_img;
  Mutex m_mutex;
  void* m_camera;
  CamThread m_thread;
  Semaphore m_semaphore;
};


};  // end namespace blepo

#endif //__BLEPO_CAPTUREIEEE1394_H__
