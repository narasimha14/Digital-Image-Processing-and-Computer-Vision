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

#ifndef __BLEPO_DIRECTSHOWFORDUMMIES_H__
#define __BLEPO_DIRECTSHOWFORDUMMIES_H__

/////////////////////////////////////////////////////////
// This file contains helper routines to make DirectShow easier to use, at least 
// for grabbing video and audio.  Because this header file includes many of the 
// DirectShow headers, it should not be included by any header file that makes its
// way to the end user.  Rather, it should be included only by cpp files.
//
// These functions throw an exception if anything goes wrong.

#include <assert.h>
#include <afx.h>  // CString
#include <vector>
#include <afxwin.h>  // CRect
#include <afx.h>  // CString
#include <dshow.h>
#include <atlbase.h>  // CComPtr
#include <mtype.h>
#include <qedit.h>  // CLSID_SampleGrabber
#include <vector>
#include "Utilities/Exception.h"  // BLEPO_ERROR, Exception

namespace DirectShowForDummies
{

void InitializeDirectShow();
void UninitializeDirectShow();

/// This is a singleton class that ensures that DirectShow is initialized before
/// anyone needs it, and is uninitialized after everyone is done with it.
/// Uses reference counting.  Call AddRef() if you want to use DirectShow and RemoveRef() if you are done.
/// Static functions should call EnsureInitialized().
class DirectShowInitializer
{
public:
  static void AddRef() { if (m_nrefs==0)  InitializeDirectShow();  m_nrefs++; }
  static void RemoveRef() { m_nrefs--;  if (m_nrefs==0)  UninitializeDirectShow(); }
  static void EnsureInitialized() { if (m_nrefs==0)  InitializeDirectShow(); }
private:
  static int m_nrefs;
};

/** This is a slight modification to Microsoft's CComPtr class (found in ATLBASE.H).  
    I wrote this class because CComPtr::operator=(T* lp) calls AtlComPtrAssign, which
    has been causing the program to crash (usually upon trying to connect two pins) whenever
    I tried to rebuild a graph that had previously been destroyed.  I am not sure what
    the problem is, but this solution seems to work.  I recommend using this class 
    instead of CComPtr in all cases, until we learn more.  -- STB 6/16/05  
*/
template<class T>
class SimpleComPtr : public CComPtr<T>
{
public:
  SimpleComPtr() : CComPtr<T>() {}
  SimpleComPtr(T* lp) : CComPtr<T>(lp) {}
  T* operator=(T* lp) { p = lp;  return p; }  ///< just copy the pointer
};

void GetFilterFriendlyNames(const CLSID& categoryCLSID, std::vector<CString>* friendly_names);

IGraphBuilder* StartBuildingGraph();

ICaptureGraphBuilder2* StartBuildingCaptureGraph(IGraphBuilder* graph_builder);

IMediaSeeking* GetMediaSeekingInterface(IGraphBuilder* graph_builder);

IMediaControl* GetMediaControlInterface(IGraphBuilder* graph_builder);

/// returns NULL if filter cannot be created; 
/// probably indicates that camera is not plugged in
IBaseFilter* AddVideoCaptureFilterToGraph(IGraphBuilder* graph_builder, int camera_index);

/// returns NULL if filter cannot be created; 
/// probably indicates that mic is not plugged in
IBaseFilter* AddAudioCaptureFilterToGraph(IGraphBuilder* graph_builder, int microphone_index);

/// returns NULL if filter cannot be created; 
/// probably indicates that camera or mic is not plugged in
IBaseFilter* CreateFilterWithIndex(const CLSID& categoryCLSID, int index);

void SetupVideoMediaType(int width, int height, CMediaType* media_type);

void SetupAudioMediaType(int nchannels, int sampling_rate, int bits_per_sample, CMediaType* media_type);

//void ConfigureVideoCaptureFilter(IBaseFilter* video_capture_filter, CMediaType& media_type);

DWORD ConfigureVideoCaptureFilter(IBaseFilter* video_capture_filter, int width, int height);

void ConfigureAudioCaptureFilter(IBaseFilter* audio_capture_filter, CMediaType& media_type);

DWORD GetVideoCaptureFilterConfiguration(IBaseFilter* video_capture_filter, int* width, int* height);

IBaseFilter* AddVideoGrabberFilterToGraph(IGraphBuilder* graph_builder);

IBaseFilter* AddAudioGrabberFilterToGraph(IGraphBuilder* graph_builder);

IBaseFilter* AddVideoRenderFilterToGraph(IGraphBuilder* graph_builder);

IBaseFilter* AddAudioRenderFilterToGraph(IGraphBuilder* graph_builder);

IBaseFilter* AddAviFileReaderFilterToGraph(const char* fname, IGraphBuilder* graph_builder);

IPin* GetPin(IBaseFilter* filter, PIN_DIRECTION direction, int index);

void ConnectPins(IBaseFilter* filt1, int index1, IBaseFilter* filt2, int index2, IGraphBuilder* graph_builder);

void DisconnectPin(IBaseFilter* filt, int index, IGraphBuilder* graph_builder);

DWORD AddGraphToRot(IUnknown *pUnkGraph);

void RemoveGraphFromRot(DWORD rot_id);

void StartGraph(IMediaControl* media_control);

void StopGraph(IMediaControl* media_control);

void PauseGraph(IMediaControl* media_control);

class SampleGrabberCallback : public ISampleGrabberCB
{
public:
  // You need to override this abstract method
  // Return S_OK or S_FALSE
	virtual STDMETHODIMP SampleCB(double sampleTime, IMediaSample *pSample) = 0;

  // Fake out any COM ref counting
  virtual STDMETHODIMP_(ULONG) AddRef() { return 2; }
  virtual STDMETHODIMP_(ULONG) Release() { return 1; }

  // Fake out any COM QI'ing
  virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
  {
    if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown ) 
    {
      *ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
      return NOERROR;
    }
    return E_NOINTERFACE;
  }

	virtual STDMETHODIMP BufferCB(double sampleTime, BYTE *pBuffer, long bufferLen)
  {
    return E_NOTIMPL;  // not implemented
  }
};

void ConfigureSampleGrabberFilter(IBaseFilter* video_grabber_filter, ISampleGrabberCB* callback);

void ConfigureVideoGrabberFilter(IBaseFilter* video_grabber_filter, SampleGrabberCallback* callback);

void ConfigureAudioGrabberFilter(IBaseFilter* audio_grabber_filter, SampleGrabberCallback* callback);

void ConfigureVideoGrabberFilterInputPin(IBaseFilter* video_grabber_filter);

}; // end namespace DirectShowForDummies

#endif // __BLEPO_DIRECTSHOWFORDUMMIES_H__
