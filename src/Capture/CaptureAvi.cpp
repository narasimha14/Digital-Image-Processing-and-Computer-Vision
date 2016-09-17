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

#include "CaptureAvi.h"
#include "DirectShowForDummies.h"
//#include <windows.h>
//#include <streams.h>
//#include <stdio.h>
//#include <atlbase.h>
//#include <qedit.h>
//#include <streams.h>  // SetMediaType

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

class CaptureAviVideoCallback : public SampleGrabberCallback
{
public:
  CaptureAviVideoCallback() {}
  void Setup(CaptureAvi* graph, int width, int height)
  {
    m_graph = graph;
    m_width = width;
    m_height = height;
  }
  virtual STDMETHODIMP SampleCB(double sampleTime, IMediaSample *pSample)
  {
    HRESULT hr;
    REFERENCE_TIME start, end;  // in 100ns units
    BYTE* data;

    AM_MEDIA_TYPE* media = NULL;
    hr = pSample->GetMediaType(&media);
    if (hr != S_OK)  assert(0);
  	VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) (media);
  	BITMAPINFOHEADER * bmih = &(vih->bmiHeader);


    /// Note:  If this assertion fires, it means that the video is not being grabbed correctly.
    /// I have had this problem with a Logitech QuickCam Express on a Dell Inspiron 700m laptop.
    /// I am not sure what the problem is, perhaps something to do with the driver?
    assert(pSample->GetSize() == 3*m_width*m_height);

    // get time
    hr = pSample->GetTime(&start, &end);
    if (hr != S_OK && hr!=VFW_S_NO_STOP_TIME) { assert(0);  return S_FALSE; }
    CaptureAvi::Time timestamp;
    timestamp.seconds = (unsigned int) (start / 10000000);
    timestamp.nanoseconds = (unsigned int) ((start - (timestamp.seconds * 10000000)) * 100);

    // get data
    hr = pSample->GetPointer(&data);
    if (hr != S_OK) { assert(0);  return S_FALSE; }
    
    // call callback
    m_graph->OnNewVideoFrame(static_cast<const unsigned char*>(data), m_width, m_height, timestamp);
    return S_OK;
  }
private:
  CaptureAvi* m_graph;
  int m_width, m_height;
};

};


// ================> begin local functions (available only to this translation unit)
namespace{
using namespace blepo;
using namespace DirectShowForDummies;

struct Stuff
{
  Stuff() : is_running(false), is_paused(false), rot_id(0) {}
  SimpleComPtr< IGraphBuilder > graph_builder;
  SimpleComPtr< ICaptureGraphBuilder2 > capture_builder;
  SimpleComPtr< IMediaSeeking > media_seeking;
  SimpleComPtr< IMediaControl > media_control;
  SimpleComPtr< IBaseFilter > avi_reader_filter;
  SimpleComPtr< IFileSourceFilter > file_source_filter;
  SimpleComPtr< IBaseFilter > video_grabber_filter;
  SimpleComPtr< IBaseFilter > video_render_filter;
  CaptureAviVideoCallback video_callback;
  bool is_running;
  bool is_paused;
  unsigned long rot_id;
};

};
// ================< end local functions

// This macro makes it easier to access 'm_graph', which is void* in the header file to prevent header leak.
#define GGRAPH (static_cast<Stuff*>( m_graph ))


namespace blepo
{
using namespace DirectShowForDummies;
 
CaptureAvi::CaptureAvi()
  : m_graph(NULL)
{
  DirectShowInitializer::AddRef();
}

CaptureAvi::~CaptureAvi()
{
  TearDownGraph();
  DirectShowInitializer::RemoveRef();
}

bool CaptureAvi::BuildGraph(const char* fname)
{
  TearDownGraph();
  m_graph = static_cast<void*>( new Stuff );
  const bool display = true;//false;  // causes live video to be displayed in popup window; good for debugging
  const bool add_to_rot = true;//false;  // causes graph to be added to ROT; good for debugging
  int rot_id;

  // start building graph
  GGRAPH->graph_builder = StartBuildingGraph();
  // get interfaces
  GGRAPH->media_control = GetMediaControlInterface(GGRAPH->graph_builder);
  GGRAPH->media_seeking = GetMediaSeekingInterface(GGRAPH->graph_builder);
  // add AVI file reader filter
  GGRAPH->avi_reader_filter = AddAviFileReaderFilterToGraph(fname, GGRAPH->graph_builder);
  // add video grabber filter, configure it, and connect pins
  GGRAPH->video_grabber_filter = AddVideoGrabberFilterToGraph(GGRAPH->graph_builder);
//  GGRAPH->video_callback.Setup(this, width, height);
  ConfigureVideoGrabberFilter(GGRAPH->video_grabber_filter, &GGRAPH->video_callback);

  CMediaType GrabType;
  GrabType.SetType( &MEDIATYPE_Video );
  GrabType.SetSubtype( &MEDIASUBTYPE_RGB24 );
  ISampleGrabber* foo = (ISampleGrabber*) ( (IBaseFilter*) GGRAPH->video_grabber_filter );
  HRESULT hr = foo->SetMediaType(&GrabType);
//  HRESULT hr = GGRAPH->video_grabber_filter->SetMediaType( &GrabType );

  ConfigureVideoGrabberFilterInputPin(GGRAPH->video_grabber_filter);
  ConnectPins(GGRAPH->avi_reader_filter, 0, GGRAPH->video_grabber_filter, 0, GGRAPH->graph_builder);
  if (display)
  {
    GGRAPH->video_render_filter = AddVideoRenderFilterToGraph(GGRAPH->graph_builder);
    ConnectPins(GGRAPH->video_grabber_filter, 0, GGRAPH->video_render_filter, 0, GGRAPH->graph_builder);
  }
  if (add_to_rot)
  {
    // add graph to ROT (enables you to connect to graph using Microsoft's GraphEdit, graphedt.exe,
    // which is in the DirectX SDK under Bin/DXUtils.  Select File -> Connect to Remote Graph)
    rot_id = AddGraphToRot(GGRAPH->graph_builder);
  }

//  StartGraph(GGRAPH->media_control);
  return true;
}

void CaptureAvi::TearDownGraph()
{
  if (GGRAPH)
  {
    if (GGRAPH->rot_id != 0) RemoveGraphFromRot(GGRAPH->rot_id);
    GGRAPH->rot_id = 0;
    delete GGRAPH;
    m_graph = NULL;
  }
}

void CaptureAvi::UpsideDownBgrImage::StoreCopy(int width, int height, const unsigned char* data)
{
  const int nbytes = width * height * 3;
  m_width = width;
  m_height = height;
  m_data.Reset(nbytes);
  memcpy(m_data.Begin(), data, nbytes);
}

void CaptureAvi::UpsideDownBgrImage::ToBgr(ImgBgr* out) const
{
  const int nbytes_per_row = m_width*3;
  const unsigned char* data = m_data.Begin();

  // copy the data, flipping vertically (because it is upside down)
  out->Reset(m_width, m_height);
  ImgBgr::Iterator p = out->Begin(0, m_height-1);
  for (int y=0 ; y<m_height ; y++)
  {
    memcpy((void*) p, data, nbytes_per_row);
    data += nbytes_per_row;
    p -= m_width;
  }
}

/// 'data' comes in BGR format, upside down
void CaptureAvi::OnNewVideoFrame(const unsigned char* data, int width, int height, Time time)
{
  m_semaphore.Wait(0);  // clear semaphore
  m_mutex.Lock();
  // store a copy of the data as quickly as we can, because this callback is called at frame rate
  m_upsidedown_image.StoreCopy(width, height, data);
  m_mutex.Unlock();
  m_semaphore.Signal();  // signal semaphore
//    TRACE("%d %d\n", time.seconds, time.nanoseconds);
}


};  // end namespace blepo

