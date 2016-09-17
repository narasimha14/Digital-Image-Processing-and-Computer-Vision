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

#include "CaptureDirectShow.h"
#include "DirectShowForDummies.h"
#include "Utilities/Math.h"
#include "Image/ImageOperations.h"  // FlipVertical

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

using namespace blepo;
using namespace DirectShowForDummies;

namespace blepo
{

class bpoVideoCallback : public SampleGrabberCallback
{
public:
  bpoVideoCallback() {}
  void Setup(CaptureDirectShow* graph, int width, int height)
  {
    m_dsgraph = graph;
    m_width = width;
    m_height = height;
  }
  virtual STDMETHODIMP SampleCB(double sampleTime, IMediaSample *pSample)
  {
    HRESULT hr;
    REFERENCE_TIME start, end;  // in 100ns units
    BYTE* data;

    int nbytes = pSample->GetSize();
    DWORD pixel_format = m_dsgraph->GetPixelFormat();

#ifndef NDEBUG
    {
      /// Check to be sure that 'nbytes' is correct
      /// Note:  If this assertion fires, it means that the video is not being grabbed correctly.
      /// I have had this problem with a Logitech QuickCam Express on a Dell Inspiron 700m laptop.
      /// I am not sure what the problem is, perhaps something to do with the driver?
      int nbytes_per_pixel = 3;
      {
        switch (pixel_format)
        {
        case '2YUY':  nbytes_per_pixel = 2;  break;
        case BI_RGB:   nbytes_per_pixel = 3;  break;
        default:
          assert(0);  // unrecognized format
        }
      }
      assert(nbytes == nbytes_per_pixel * m_width * m_height);
    }
#endif

    // get time
    CaptureDirectShow::Time timestamp;
    hr = pSample->GetTime(&start, &end);
    if (hr == VFW_E_SAMPLE_TIME_NOT_SET)
    {
      // We do not know what the time is.  I have only seen this happen after the graph has been stopped.
      timestamp.seconds = 0;
      timestamp.nanoseconds = 0;
    }
    else
    {
      if (hr != S_OK && hr!=VFW_S_NO_STOP_TIME) { assert(0);  return S_FALSE; }
      timestamp.seconds = (unsigned int) (start / 10000000);
      timestamp.nanoseconds = (unsigned int) ((start - (timestamp.seconds * 10000000)) * 100);
    }

    // get data
    hr = pSample->GetPointer(&data);
    if (hr != S_OK) { assert(0);  return S_FALSE; }
    
    // call callback
    m_dsgraph->OnNewVideoFrame(static_cast<const unsigned char*>(data), m_width, m_height, nbytes, pixel_format, timestamp);
    return S_OK;
  }
private:
  CaptureDirectShow* m_dsgraph;
  int m_width, m_height;
};

};


using namespace blepo;
using namespace DirectShowForDummies;

struct CaptureDirectShow::Stuff
{
  Stuff() 
    : graph_builder(), capture_builder(), 
      media_seeking(), media_control(), 
      video_capture_filter(), video_grabber_filter(), video_render_filter(),
      is_running(false), is_paused(false),
      rot_id(0), video_callback()
  {}
  SimpleComPtr< IGraphBuilder > graph_builder;
  SimpleComPtr< ICaptureGraphBuilder2 > capture_builder;
  SimpleComPtr< IMediaSeeking > media_seeking;
  SimpleComPtr< IMediaControl > media_control;
  SimpleComPtr< IBaseFilter > video_capture_filter;
  SimpleComPtr< IBaseFilter > video_grabber_filter;
  SimpleComPtr< IBaseFilter > video_render_filter;
  SimpleComPtr< IAMCrossbar > video_crossbar;
  bpoVideoCallback video_callback;
  bool is_running;
  bool is_paused;
  unsigned long rot_id;
};

namespace blepo
{
using namespace DirectShowForDummies;

void CaptureDirectShow::GetVideoInputDevices(std::vector<CString>* friendly_names)
{
  DirectShowInitializer::EnsureInitialized();
  GetFilterFriendlyNames(CLSID_VideoInputDeviceCategory, friendly_names);
}

void CaptureDirectShow::GetAudioInputDevices(std::vector<CString>* friendly_names)
{
  DirectShowInitializer::EnsureInitialized();
  GetFilterFriendlyNames(CLSID_AudioInputDeviceCategory, friendly_names);
}

CaptureDirectShow::CaptureDirectShow()
  : m_graph(NULL)
{
  DirectShowInitializer::AddRef();
}

CaptureDirectShow::~CaptureDirectShow()
{
  TearDownGraph();
  DirectShowInitializer::RemoveRef();
}

void CaptureDirectShow::BuildGraph(int camera_index, const BuildParams& params)
{
  BuildGraph(-1, -1, camera_index, params);
}

void CaptureDirectShow::BuildGraph(int width, int height, int camera_index, const BuildParams& params)
{
  TearDownGraph();
  m_graph = new Stuff;

//  const bool display = false;  // causes live video to be displayed in popup window; good for debugging
//  const bool add_to_rot = false;  // causes graph to be added to ROT so it can be viewed by graphedt.exe; good for debugging

  // start building graph
  m_graph->graph_builder = StartBuildingGraph();
  m_graph->capture_builder = StartBuildingCaptureGraph(m_graph->graph_builder);
  // get interfaces
  m_graph->media_control = GetMediaControlInterface(m_graph->graph_builder);
  m_graph->media_seeking = GetMediaSeekingInterface(m_graph->graph_builder);
  // add video capture filter and configure it
  m_graph->video_capture_filter = AddVideoCaptureFilterToGraph(m_graph->graph_builder, camera_index);
  if (m_graph->video_capture_filter == NULL)  
  {
    delete m_graph;
    m_graph = NULL;
    BLEPO_ERROR("Unable to build DirectShow graph.  Is the camera plugged in?");
  }
  {  // add video crossbar for analog video (tested with GrabBeeX-lite USB frame grabber)
    HRESULT res;
    res = m_graph->capture_builder->FindInterface(
      &LOOK_UPSTREAM_ONLY, 
      NULL, 
      m_graph->video_capture_filter,
      IID_IAMCrossbar,
      (void**)&m_graph->video_crossbar);
    if (res != S_OK)  {} // okay, this may not be analog video { assert(0); }
    // Note:  Sometimes GrabBeeX-lite driver, when installed, defaults to PAL instead of NTSC.
    // We have solved this problem by running GrabBeeX application-level software to change the
    // format.  There may be a way to do this programmatically, but we haven't yet figured it out.
    // Perhaps we should change the output pin of the Analog Video Crossbar to
    // AnalogVideo_NTSC_M (see AnalogVideoStandard Enumeration).
    // See also http://doc.51windows.net/Directx9_SDK/?url=/Directx9_SDK/dir_8.htm
    // -- STB 9/18/08
  }
  if (width>0 && height>0)
  {
    m_pixel_format = ConfigureVideoCaptureFilter(m_graph->video_capture_filter, width, height);
  }
  else
  {
    m_pixel_format = GetVideoCaptureFilterConfiguration(m_graph->video_capture_filter, &width, &height);
  }
  // add video grabber filter, configure it, and connect pins
  m_graph->video_grabber_filter = AddVideoGrabberFilterToGraph(m_graph->graph_builder);
  m_graph->video_callback.Setup(this, width, height);
  ConfigureVideoGrabberFilter(m_graph->video_grabber_filter, &m_graph->video_callback);
  ConnectPins(m_graph->video_capture_filter, 0, m_graph->video_grabber_filter, 0, m_graph->graph_builder);
  // add video render filter and connect pins
  if (params.display)
  {
    m_graph->video_render_filter = AddVideoRenderFilterToGraph(m_graph->graph_builder);
    ConnectPins(m_graph->video_grabber_filter, 0, m_graph->video_render_filter, 0, m_graph->graph_builder);
  }
  if (params.add_to_rot)
  {
    // add graph to ROT (enables you to connect to graph using Microsoft's GraphEdit, graphedt.exe,
    // which is in the DirectX SDK under Bin/DXUtils.  Select File -> Connect to Remote Graph)
    m_graph->rot_id = AddGraphToRot(m_graph->graph_builder);
  }
}

void CaptureDirectShow::TearDownGraph()
{
  if (m_graph)
  {
    if (m_graph->rot_id != 0) RemoveGraphFromRot(m_graph->rot_id);
    m_graph->rot_id = 0;
    delete m_graph;
    m_graph = NULL;
  }
}

void CaptureDirectShow::Start()
{
  if (!m_graph)  return;
  StartGraph(m_graph->media_control);
  m_graph->is_running = true;
  m_graph->is_paused = false;
}

void CaptureDirectShow::Stop()
{
  if (!m_graph)  return;
  StopGraph(m_graph->media_control);
  m_graph->is_running = false;
  m_graph->is_paused = false;
}

void CaptureDirectShow::Pause()
{
  if (m_graph->is_running) 
  {
    if (m_graph->is_paused)
    {
      StartGraph(m_graph->media_control);  // unpause
      m_graph->is_paused = false;
    }
    else
    {
      PauseGraph(m_graph->media_control);  // pause
      m_graph->is_paused = true;
    }
  }
}

bool CaptureDirectShow::IsRunning() const
{
  return m_graph->is_running;
}

bool CaptureDirectShow::IsPaused() const
{
  return m_graph->is_paused;
}

void CaptureDirectShow::RawImageData::StoreCopy(int width, int height, int nbytes, DWORD format, const unsigned char* data)
{
  m_width = width;
  m_height = height;
  m_format = format;
  m_data.Reset(nbytes);
  memcpy(m_data.Begin(), data, nbytes);
}

static inline void CaptureYuvToBgr(unsigned char  y, unsigned char  u, unsigned char  v,
                                   unsigned char* b, unsigned char* g, unsigned char* r)
{
  int blue  = ( 298 * (y-16) + 516 * (u-128)                 + 128) >> 8;
  int green = ( 298 * (y-16) - 100 * (u-128) - 208 * (v-128) + 128) >> 8;
  int red   = ( 298 * (y-16)                 + 409 * (v-128) + 128) >> 8;
  *b = blepo_ex::Clamp(blue , 0, 255);
  *g = blepo_ex::Clamp(green, 0, 255);
  *r = blepo_ex::Clamp(red  , 0, 255);
}

void CaptureDirectShow::RawImageData::ToBgr(ImgBgr* out, bool flip_vertical) const
{
  switch (m_format)
  {
  case BI_RGB:
    {
      const int nbytes_per_row = m_width * (ImgBgr::NBITS_PER_PIXEL/8);
      const unsigned char* data = m_data.Begin();

      out->Reset(m_width, m_height);
      if (flip_vertical)
      { // flip data vertically (because image is upside down) -- fast implementation
        ImgBgr::Iterator p = out->Begin(0, m_height-1);
        for (int y=0 ; y<m_height ; y++)
        {
          memcpy((void*) p, data, nbytes_per_row);
          data += nbytes_per_row;
          p -= m_width;
        }
      }
      else
      { // just copy data
        ImgBgr::Iterator p = out->Begin();
        memcpy((void*) p, data, nbytes_per_row * m_height);
      }
      break;
    }
  case '2YUY':
    {
      // format of YUY2 means that 2 adjacent pixels are packed into 4 consecutive bytes,
      // represeting [ Y U Y V ], i.e., the two pixels share chrominance information
      out->Reset(m_width, m_height);
      const unsigned char* p = m_data.Begin();
      {
        ImgBgr::Iterator q = out->Begin();
        {  // YUV to RGB conversion
          assert(m_data.Len() % 4 == 0);  // must be an even number of pixels, b/c adjacent pixels share U and V
          assert(m_data.Len() == m_width * m_height * 2);
          while (p < m_data.End())
          {
            // first pixel of pair
            CaptureYuvToBgr(p[0], p[1], p[3], &q->b, &q->g, &q->r);
            q++;
            // second pixel of pair
            CaptureYuvToBgr(p[2], p[1], p[3], &q->b, &q->g, &q->r);
            q++;
            p += 4;
          }
        }
      }
      if (flip_vertical)
      {  // flip data vertically (because image is upside down)
        FlipVertical(*out, out);  // slow way
      }
      break;
    }
  default:
    assert(0);  // unrecognized (unimplemented) pixel compression format
  }
}

void CaptureDirectShow::RawImageData::ToGray(ImgGray* out, bool flip_vertical) const
{
  switch (m_format)
  {
  case BI_RGB:
    {
      const int nbytes_per_row = m_width * (ImgBgr::NBITS_PER_PIXEL/8);
      const unsigned char* p = m_data.Begin();

      out->Reset(m_width, m_height);
      {  // copy the data
        ImgGray::Iterator q = out->Begin();
        while ( p < m_data.End() )
        {
          unsigned char b = p[0];
          unsigned char g = p[1];
          unsigned char r = p[2];
          *q++ = (b + g*2 + r) >> 2;  // quick approximation to BGR->Y conversion
          p += 3;
        }
      }
      if (flip_vertical)
      {  // flip data vertically (because image is upside down)
        FlipVertical(*out, out);  // slow way
      }
      break;
    }
  case '2YUY':
    {
      // format of YUY2 means that 2 adjacent pixels are packed into 4 consecutive bytes,
      // represeting [ Y U Y V ], i.e., the two pixels share chrominance information
      out->Reset(m_width, m_height);
      const unsigned char* p = m_data.Begin();
      {
        ImgGray::Iterator q = out->Begin();
        {  // grayscale conversion
          while (p != m_data.End())
          {
            *q++ = *p++;
            p++;  // skip U and V chrominance components
          }
        }
      }
      if (flip_vertical)
      {  // flip data vertically (because image is upside down)
        FlipVertical(*out, out);  // slow way
      }
      break;
    }
  default:
    assert(0);  // unrecognized (unimplemented) pixel compression format
  }
}

/// 'data' comes in BGR format, upside down
void CaptureDirectShow::OnNewVideoFrame(const unsigned char* data, int width, int height, int nbytes, DWORD format, Time time)
{
  m_semaphore.Wait(0);  // clear semaphore
  m_mutex.Lock();
  // store a copy of the data as quickly as we can, because this callback is called at frame rate
  m_upsidedown_image.StoreCopy(width, height, nbytes, format, data);
  m_mutex.Unlock();
  m_semaphore.Signal();  // signal semaphore
//    TRACE("%d %d\n", time.seconds, time.nanoseconds);
}

bool CaptureDirectShow::GetLatestImage(ImgBgr* out, bool flip_vertical, int timeout)
{
  bool signaled = m_semaphore.Wait(timeout);
  if (signaled)
  {
    m_mutex.Lock();
    // only when the user has requested a frame do we need to convert the data
    m_upsidedown_image.ToBgr(out, flip_vertical);
    m_mutex.Unlock();
    return true;
  }
  else
  {
    return false;
  }
}

bool CaptureDirectShow::GetLatestImage(ImgGray* out, bool flip_vertical, int timeout)
{
  bool signaled = m_semaphore.Wait(timeout);
  if (signaled)
  {
    m_mutex.Lock();
    // only when the user has requested a frame do we need to convert the data
    m_upsidedown_image.ToGray(out, flip_vertical);
    m_mutex.Unlock();
    return true;
  }
  else
  {
    return false;
  }
}

};  // end namespace blepo





//////////////////////////////////////
// For reference, here is how to call OpenCV's capture code

/*
#include "cv.h" // CvCapture
#include "highgui.h"
void OpenCvCapture()
{
  IplImage *image = 0, *grey = 0, *prev_grey = 0;
  CvCapture* capture = 0;

  capture = cvCaptureFromCAM( 0 );
  cvNamedWindow( "LkDemo", 0 );
//    cvSetMouseCallback( "LkDemo", on_mouse, 0 );
  for(;;)
  {
      IplImage* frame = 0;
      int c;

      frame = cvQueryFrame( capture );
      if( !frame )
          break;

      if( !image )
      {
          // allocate all the buffers
          image = cvCreateImage( cvGetSize(frame), 8, 3 );
          image->origin = frame->origin;
          grey = cvCreateImage( cvGetSize(frame), 8, 1 );
          prev_grey = cvCreateImage( cvGetSize(frame), 8, 1 );
      }

      cvCopy( frame, image, 0 );
      cvCvtColor( image, grey, CV_BGR2GRAY );

      cvShowImage( "LkDemo", image );

      c = cvWaitKey(10);
      if( (char)c == 27 )
          break;
  }

  cvReleaseCapture( &capture );
  cvDestroyWindow("LkDemo");
  return;
}
*/