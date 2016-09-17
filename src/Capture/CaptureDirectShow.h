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

#ifndef __BLEPO_CAPTUREDIRECTSHOW_H__
#define __BLEPO_CAPTUREDIRECTSHOW_H__

#include <afx.h>  // CString
#include <vector>
#include "Image/Image.h"
#include "Utilities/Array.h"
#include "Utilities/Mutex.h"
#include "Utilities/Exception.h"

/**
@class CaptureDirectShow

  Class to capture video from Microsoft's DirectShow framework.
  Should work with most USB webcams, but has been tested only
  with Logitech Quickcam Pro 4000.

  Design note:  Even though you cannot do anything with this class until you
  build the graph, it still makes sense to ask the user to first instantiate the
  object and then call BuildGraph.  This allows the user to delay building the graph
  until the parameter information (width, height, index) is available, and it enables
  the class to clean itself up automatically in the destructor.  If, instead,
  we had put these parameters into the constructor, then the user would have to either 
  instantiate the object on the stack (unlikely) or to call new once the parameters
  are available; but this would place the burden on the user not to forget to call
  delete.

  @author Stan Birchfield (STB)
*/

namespace blepo
{

class CaptureDirectShow
{
public:
  /** @name Query functions
      Call these functions to determine the number of video and/or audio
      devices currently available.  The vector is filled with human-readable
      names of the devices.  Use the index of the vector for selecting an
      input camera in the constructor of CaptureDirectShow.
      Note:  Audio capture is not yet supported. 
  */  
  //@{
  static void GetVideoInputDevices(std::vector<CString>* friendly_names);
  static void GetAudioInputDevices(std::vector<CString>* friendly_names);
  //@}

public:
  /// Constructor:  initializes DirectShow
  CaptureDirectShow();
  /// Destructor:  automatically destroys graph
  virtual ~CaptureDirectShow();

  struct BuildParams
  {
    BuildParams() : display(false), add_to_rot(false) {}
    bool display;  // causes live video to be displayed in popup window; good for debugging
    bool add_to_rot;  // causes graph to be added to ROT; good for debugging
  };

  /** @name BuildGraph
      Builds a DirectShow graph that connects to the camera.
      @param 'width', 'height':  The desired image size.
        Use only standard numbers, such as 640 x 480, 320 x 240, 352 x 288, etc.
        Some cameras may only be capable of capturing at lower resolution,
          e.g., while Logitech Quickcam Pro 4000 can capture at 640 x 480,
          the less expensive Logitech Quickcam express cannot.
        To use the camera's default image size, leave these parameters alone.
      @param 'camera_index'  Zero-based index of the camera, corresponding to
        to the index of the vector filled in by GetVideoInputDevices().
      Notes:
      - You must call this function before calling Start().
      - If a graph is already built, this function first tears down the existing graph.
      - Throws an exception if anything goes wrong.  One common failure is that a 
        camera with the requested index has not been found, e.g., the camera is not
        plugged in.
  */
  //@{
  void BuildGraph(int width, int height, int camera_index = 0, const BuildParams& params = BuildParams());
  void BuildGraph(int camera_index = 0, const BuildParams& params = BuildParams());
  //@}
  void TearDownGraph();

  // if the graph has not already been built, these functions do nothing
  void Start();  ///< Start capturing images at the default frame rate
  void Stop();  ///< Stop capturing
  void Pause();  ///< Pause capturing

  bool IsRunning() const;  ///< returns true if capturing images
  bool IsPaused() const;  ///< returns true if paused

  /**
    Once the graph is running, frames will be grabbed continuously at the
    frame rate, automatically.  Call this function to get access to the latest 
    frame that has been grabbed. 
    @param 'timeout'  in milliseconds (-1 for infinite, 0 for immediate return)
    @return  Returns true if a new frame is available since the last time this function
      was called, false otherwise
  */
  bool GetLatestImage(ImgBgr* out, bool flip_vertical = false, int timeout = 0);
  bool GetLatestImage(ImgGray* out, bool flip_vertical = false, int timeout = 0);

protected:

  struct Time
  {
    unsigned int seconds, nanoseconds;
    // returns time in milliseconds
    unsigned int GetMS() const { return (seconds*1000) + (nanoseconds / 1000000); };
  };
  friend class bpoVideoCallback;  ///< so it can call OnNewVideoFrame
  virtual void OnNewVideoFrame(const unsigned char* data, int width, int height, int nbytes, DWORD format, Time timestamp);

  DWORD GetPixelFormat() const { return m_pixel_format; }

private:
  struct Stuff;
  Stuff* m_graph;  ///< this is a d-pointer (opaque pointer) to avoid header leak
  ImgBgr m_img;
  Mutex m_mutex;
  Semaphore m_semaphore;
  DWORD m_pixel_format;  // copied from BITMAPINFOHEADER::biCompression

  class RawImageData
  {
  public:
    void StoreCopy(int width, int height, int nbytes, DWORD format, const unsigned char* data);
    void ToBgr(ImgBgr* out, bool flip_vertical = false) const;
    void ToGray(ImgGray* out, bool flip_vertical = false) const;
  private:
    int m_width, m_height;
    DWORD m_format;
    Array<unsigned char> m_data;
  };
  RawImageData m_upsidedown_image;
};

};  // end namespace blepo

#endif //__BLEPO_CAPTUREDIRECTSHOW_H__
