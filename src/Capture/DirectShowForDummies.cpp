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

#include "DirectShowForDummies.h"
#include <assert.h>
#include <afx.h>  // CString
//#include <vector>
#include <afxwin.h>  // CRect
#include <afx.h>  // CString
#include "Utilities/Array.h"
using namespace blepo;  // for BLEPO_ERROR

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

// ================> begin local functions (available only to this translation unit)
namespace{

// Returns the number of bytes needed to hold a double-word-aligned image with the given
// width, height, and bit depth.   (32 bits in a double word)
int iComputeAlignment32(int width, int height, int nbits_per_pixel)
{ 
  assert(width>=0 && height>=0 && nbits_per_pixel>=0);
  const int ndwords_per_row = ((width*nbits_per_pixel)+31) >> 5;
  return height * (ndwords_per_row << 2);
}

};
// ================< end local functions

/// This is the type of error that should (hopefully) only affect developers.
/// Errors that might affect users should have a better description.
#define FATAL_DSHOW_ERROR  { BLEPO_ERROR("Fatal DirectShow error; did you forget to call InitializeDirectShow()?"); }

namespace DirectShowForDummies
{
int DirectShowInitializer::m_nrefs = 0;

// Initialize DirectShow.  Call this before you call anything else.
void InitializeDirectShow()
{
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
}

// Uninitialize DirectShow.  Call this when you are done.
void UninitializeDirectShow()
{
  CoUninitialize();
}

// Get the English-readable friendly names (e.g., "Logitech Quickcam Pro 4000")
// of all the devices of a particular type.
//
// Example:  
//    {
//      std::vector<CString> friendly_names;
//      GetFilterFriendlyNames(CLSID_VideoInputDeviceCategory, &friendly_names);
//    }
void GetFilterFriendlyNames(const CLSID& categoryCLSID, std::vector<CString>* friendly_names)
{
  assert(S_OK == NOERROR);  // If this is not true, then this code may not match the example code from which it was formed.
  USES_CONVERSION;

  friendly_names->clear();

  HRESULT hr;

  // create an enumerator
  SimpleComPtr< ICreateDevEnum > pCreateDevEnum;
  hr = pCreateDevEnum.CoCreateInstance( CLSID_SystemDeviceEnum );
  if (hr != S_OK || !pCreateDevEnum)  FATAL_DSHOW_ERROR;
  ASSERT(hr == S_OK && pCreateDevEnum);  // perhaps you forgot to call CoInitialize

  // enumerate video capture devices
  SimpleComPtr< IEnumMoniker > pEm;
  hr = pCreateDevEnum->CreateClassEnumerator(categoryCLSID, &pEm, 0 );
  if(hr != S_OK || !pEm )  return;  // no camera; perhaps you forgot to plug in the camera!
  pEm->Reset();

  // find all devices
  while( 1 )
  {
    ULONG ulFetched = 0;
    SimpleComPtr< IMoniker > pM;

    hr = pEm->Next( 1, &pM, &ulFetched );
    if( hr != S_OK )  return;

    // get the property bag interface from the moniker
    {
      SimpleComPtr< IPropertyBag > pBag;
      hr = pM->BindToStorage( 0, 0, IID_IPropertyBag, (void**) &pBag );
      if( hr == S_OK )
      {
        // ask for the english-readable name
//        VARIANT var;
        CComVariant var;
        var.vt = VT_BSTR;
        hr = pBag->Read( L"FriendlyName", &var, NULL );
        if( hr == S_OK )
        {
          friendly_names->push_back( W2T( var.bstrVal ) );
          SysFreeString(var.bstrVal);
        }
      }
    }
  }

  return;
}

IGraphBuilder* StartBuildingGraph()
{
  // Create the filter graph.
  IGraphBuilder* graph_builder = NULL;
  HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
                        IID_IGraphBuilder, (void **)&graph_builder);
  if (hr != S_OK || !graph_builder)  FATAL_DSHOW_ERROR;
  return graph_builder;
}

ICaptureGraphBuilder2* StartBuildingCaptureGraph(IGraphBuilder* graph_builder)
{
  assert(graph_builder);
  ICaptureGraphBuilder2* capture_builder = NULL;
  HRESULT hr;
  // Create the capture graph builder.
  hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, 
                        IID_ICaptureGraphBuilder2, (void **)&capture_builder);
  if (hr != S_OK || !capture_builder)  FATAL_DSHOW_ERROR;
  // Associate the filter graph with the capture graph builder
  hr = capture_builder->SetFiltergraph(graph_builder);    
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  return capture_builder;
}

IMediaSeeking* GetMediaSeekingInterface(IGraphBuilder* graph_builder)
{
  IMediaSeeking* media_seeking = NULL;
	HRESULT hr = graph_builder->QueryInterface(IID_IMediaSeeking, (void **)&media_seeking);
  if (hr != S_OK || !media_seeking)  FATAL_DSHOW_ERROR;
  return media_seeking;
}

IMediaControl* GetMediaControlInterface(IGraphBuilder* graph_builder)
{
  IMediaControl* media_control = NULL;
	HRESULT hr = graph_builder->QueryInterface(IID_IMediaControl, (void **)&media_control);
  if (hr != S_OK || !media_control)  FATAL_DSHOW_ERROR;
  return media_control;
}

IBaseFilter* AddVideoCaptureFilterToGraph(IGraphBuilder* graph_builder, int camera_index)
{
  IBaseFilter* video_filter = CreateFilterWithIndex(CLSID_VideoInputDeviceCategory, camera_index);
  if (video_filter == NULL)  return NULL;  // is camera plugged in?
  HRESULT hr = graph_builder->AddFilter(video_filter, L"Video Capture");
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  return video_filter;
}

IBaseFilter* AddAudioCaptureFilterToGraph(IGraphBuilder* graph_builder, int microphone_index)
{
  IBaseFilter* audio_filter = CreateFilterWithIndex(CLSID_AudioInputDeviceCategory, microphone_index);
  HRESULT hr = graph_builder->AddFilter(audio_filter, L"Audio Capture");
  if (hr != S_OK && hr != VFW_S_DUPLICATE_NAME ) return NULL;  // is mic plugged in?
  return audio_filter;
}

IBaseFilter* CreateFilterWithIndex(const CLSID& categoryCLSID, int index)
{
  SimpleComPtr< ICreateDevEnum > pCreateDevEnum;
  SimpleComPtr< IEnumMoniker > pEm;
  SimpleComPtr< IMoniker > pM;
  SimpleComPtr< IPropertyBag > pBag;
  IBaseFilter* pFilter;
  HRESULT hr;

  // create an enumerator
  pCreateDevEnum.CoCreateInstance( CLSID_SystemDeviceEnum );
  if (!pCreateDevEnum)  FATAL_DSHOW_ERROR;

  // enumerate devices of the desired type
  pCreateDevEnum->CreateClassEnumerator( categoryCLSID, &pEm, 0 );
  if (!pEm)  return NULL;  // is camera or mic plugged in?
  pEm->Reset( );
 
  // skip to 'index'
  if (index>0)
  {
    hr = pEm->Skip(index);
    if (hr != S_OK)  FATAL_DSHOW_ERROR;
  }

  // grab the next device of the desired type
  ULONG ulFetched = 0;
  hr = pEm->Next( 1, &pM, &ulFetched );
  if (hr != S_OK)  FATAL_DSHOW_ERROR;

  // ask for the actual filter
  hr = pM->BindToObject( 0, 0, IID_IBaseFilter, (void**) &pFilter );
  if (hr != S_OK)  FATAL_DSHOW_ERROR;

  return pFilter;
}

IPin* GetPin(IBaseFilter* filter, PIN_DIRECTION direction, int index)
{
  SimpleComPtr< IEnumPins > enum_pins = NULL;
  IPin* pin = NULL;
  ULONG ulFound;
  HRESULT hr;
  
  hr = filter->EnumPins(&enum_pins);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;

  while (1)
  {
    if (index<0)  FATAL_DSHOW_ERROR;
    hr = enum_pins->Next(1, &pin, &ulFound);
    if (hr != S_OK)  FATAL_DSHOW_ERROR;
    PIN_DIRECTION pindir = (PIN_DIRECTION) 3;
    hr = pin->QueryDirection(&pindir);
    if (hr != S_OK)  FATAL_DSHOW_ERROR;
    if (pindir == direction)
    {
      if (index == 0)  break;  // found it!!
      else  index--;
    }
    pin->Release();
  }
  return pin;
}

void SetupVideoMediaType(int width, int height, CMediaType* media_type)
{
  assert(0);  // Do not call this function!  See BUG #mediatype below.

  // setup the format
  VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*) (media_type->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
  BITMAPINFOHEADER* bmih = &(vih->bmiHeader);
  bmih->biBitCount = 24;
  bmih->biClrImportant = 0;
  bmih->biClrUsed = 0;
  bmih->biCompression = BI_RGB;
  bmih->biWidth = width;
  bmih->biHeight = height;
  bmih->biPlanes = 1;
  bmih->biSize = sizeof(BITMAPINFOHEADER);
  bmih->biSizeImage = iComputeAlignment32(bmih->biWidth, abs(bmih->biHeight), bmih->biBitCount);
  bmih->biXPelsPerMeter = 0;
  bmih->biYPelsPerMeter = 0;

  media_type->majortype            = MEDIATYPE_Video;
  media_type->subtype              = MEDIASUBTYPE_RGB24;
  media_type->bFixedSizeSamples    = TRUE;
  media_type->bTemporalCompression = FALSE;
  media_type->lSampleSize          = bmih->biSizeImage;
  media_type->formattype           = FORMAT_VideoInfo;
  media_type->pUnk                 = NULL;
  // BUG #mediatype:
  // I have no idea how to assign these last two members; as a result, this function
  // gives unpredictable results. -- STB
//  media_type->cbFormat             = 0;
//  media_type->pbFormat             = reinterpret_cast<unsigned char*>(vih);
}

void SetupAudioMediaType(int nchannels, int sampling_rate, int bits_per_sample, CMediaType* media_type)
{
  WAVEFORMATEX * wfx = (WAVEFORMATEX *) (media_type->AllocFormatBuffer(sizeof(WAVEFORMATEX)));

  wfx->cbSize = sizeof(WAVEFORMATEX);
  wfx->nChannels = (unsigned short) nchannels;
  wfx->nSamplesPerSec = sampling_rate;
  wfx->wBitsPerSample = (unsigned short) bits_per_sample;
  wfx->wFormatTag = 1; // PCM
  wfx->nBlockAlign = static_cast<WORD>((wfx->nChannels*wfx->wBitsPerSample)/8);
  if (wfx->nBlockAlign*8 < wfx->nChannels*wfx->wBitsPerSample)  wfx->nBlockAlign++;
  wfx->nAvgBytesPerSec = wfx->nSamplesPerSec*wfx->nBlockAlign;

  media_type->majortype            = MEDIATYPE_Audio;
  media_type->subtype              = MEDIASUBTYPE_PCM;
  media_type->formattype           = FORMAT_WaveFormatEx;
  media_type->bFixedSizeSamples    = TRUE;
  media_type->bTemporalCompression = FALSE;
  media_type->pUnk                 = NULL;
}

void SetOutputPinMedia(IBaseFilter* filter, CMediaType& media_type)
{
  SimpleComPtr< IAMStreamConfig > stream_config = NULL;
  SimpleComPtr< IPin > pPin = NULL;
  HRESULT hr;

  // set the format of the output pin of the capture filter
  pPin = GetPin(filter, PINDIR_OUTPUT, 0);
  hr = pPin->QueryInterface(IID_IAMStreamConfig, (void**) &stream_config);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  hr = stream_config->SetFormat(&media_type);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
}

//void SetInputPinMedia(IBaseFilter* filter, CMediaType& media_type)
//{
//  SimpleComPtr< IAMStreamConfig > stream_config = NULL;
//  SimpleComPtr< IPin > pPin = NULL;
//  HRESULT hr;
//
//  // set the format of the output pin of the capture filter
//  pPin = GetPin(filter, PINDIR_INPUT, 0);
//  hr = pPin->QueryInterface(IID_IAMStreamConfig, (void**) &stream_config);
//  if (hr != S_OK)  FATAL_DSHOW_ERROR;
//  hr = stream_config->SetFormat(&media_type);
//  if (hr != S_OK)  FATAL_DSHOW_ERROR;
//}

void ConfigureVideoCaptureFilter(IBaseFilter* video_capture_filter, CMediaType& media_type)
{
  assert(0);  // Do not call this function!  See BUG #mediatype above.

  SetOutputPinMedia(video_capture_filter, media_type);
}

void ConfigureAudioCaptureFilter(IBaseFilter* audio_capture_filter, CMediaType& media_type)
{
  SetOutputPinMedia(audio_capture_filter, media_type);
}

// returns the pixel compression format
DWORD ConfigureVideoCaptureFilter(IBaseFilter* video_capture_filter, int width, int height)
{
  SimpleComPtr< IAMStreamConfig > stream_config = NULL;
  SimpleComPtr< IPin > pPin = NULL;
  AM_MEDIA_TYPE* media_type = NULL;  // Will not compile as a SimpleComPtr for some reason
  HRESULT hr;

  // get the format of the output pin of the capture filter
  pPin = GetPin(video_capture_filter, PINDIR_OUTPUT, 0);
  hr = pPin->QueryInterface(IID_IAMStreamConfig, (void**) &stream_config);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  hr = stream_config->GetFormat(&media_type);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;

  // set the format
  VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*) (media_type->pbFormat);
  BITMAPINFOHEADER* bmih = &(vih->bmiHeader);
  bmih->biWidth = width;
  bmih->biHeight = height;
  bmih->biSizeImage = iComputeAlignment32(bmih->biWidth, abs(bmih->biHeight), bmih->biBitCount);
  hr = stream_config->SetFormat(media_type);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;

  DWORD pixel_format;
  { // get and check format 
    hr = stream_config->GetFormat(&media_type);
    if (hr != S_OK)  FATAL_DSHOW_ERROR;
    VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) (media_type->pbFormat);
    BITMAPINFOHEADER * bmih = &(vih->bmiHeader);
    assert(bmih->biWidth == width);
    assert(bmih->biHeight == height);
    pixel_format = bmih->biCompression;
    DeleteMediaType(media_type);
  }
  return pixel_format;
}

// returns the pixel compression format
DWORD GetVideoCaptureFilterConfiguration(IBaseFilter* video_capture_filter, int* width, int* height)
{
  SimpleComPtr< IAMStreamConfig > stream_config = NULL;
  SimpleComPtr< IPin > pPin = NULL;
  AM_MEDIA_TYPE* media_type = NULL;  // Will not compile as a SimpleComPtr for some reason
  HRESULT hr;

  // get the format of the output pin of the capture filter
  pPin = GetPin(video_capture_filter, PINDIR_OUTPUT, 0);
  hr = pPin->QueryInterface(IID_IAMStreamConfig, (void**) &stream_config);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  hr = stream_config->GetFormat(&media_type);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;

  VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) (media_type->pbFormat);
  BITMAPINFOHEADER * bmih = &(vih->bmiHeader);
  *width = bmih->biWidth;
  *height = bmih->biHeight; 
  DWORD pixel_format = bmih->biCompression;
  DeleteMediaType(media_type);

  return pixel_format;
}

IBaseFilter* AddVideoGrabberFilterToGraph(IGraphBuilder* graph_builder)
{
  // create sample grabber filter
  IBaseFilter* sample_grabber = NULL;
  HRESULT hr;
  hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **) &sample_grabber);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  // add sample grabber filter to graph
  hr = graph_builder->AddFilter(sample_grabber, L"Video Sample Grabber");
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  return sample_grabber;
}

IBaseFilter* AddAudioGrabberFilterToGraph(IGraphBuilder* graph_builder)
{
  // create sample grabber filter
  IBaseFilter* sample_grabber = NULL;
  HRESULT hr;
  hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **) &sample_grabber);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  // add sample grabber filter to graph
  hr = graph_builder->AddFilter(sample_grabber, L"Audio Sample Grabber");
  if (hr != S_OK && hr != VFW_S_DUPLICATE_NAME ) return NULL;
  return sample_grabber;
}

void ConfigureSampleGrabberFilter(IBaseFilter* video_grabber_filter, ISampleGrabberCB* callback)
{
  SimpleComPtr< ISampleGrabber > sgi;
  HRESULT hr;
  hr = video_grabber_filter->QueryInterface(IID_ISampleGrabber, (void**) &sgi);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  hr = sgi->SetOneShot(FALSE);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  hr = sgi->SetBufferSamples(FALSE);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  hr = sgi->SetCallback(callback, 0);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
}

void ConfigureVideoGrabberFilter(IBaseFilter* video_grabber_filter, SampleGrabberCallback* callback)
{
  ConfigureSampleGrabberFilter(video_grabber_filter, callback);
}

void ConfigureAudioGrabberFilter(IBaseFilter* audio_grabber_filter, SampleGrabberCallback* callback)
{
  ConfigureSampleGrabberFilter(audio_grabber_filter, callback);
}

void ConfigureVideoGrabberFilterInputPin(IBaseFilter* video_grabber_filter)
{
  HRESULT hr;
//  CMediaType GrabType;
//  GrabType.SetType( &MEDIATYPE_Video );
//  GrabType.SetSubtype( &MEDIASUBTYPE_RGB24 );
//  hr = pGrabber->SetMediaType( &GrabType );
//  return;

  IBaseFilter* filter = video_grabber_filter;
  SimpleComPtr< IAMStreamConfig > stream_config = NULL;
  SimpleComPtr< IPin > pPin = NULL;


  // set the format of the output pin of the capture filter
  pPin = GetPin(filter, PINDIR_INPUT, 0);
  hr = pPin->QueryInterface(IID_IAMStreamConfig, (void**) &stream_config);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
//  hr = stream_config->SetFormat(&media_type);
//  if (hr != S_OK)  FATAL_DSHOW_ERROR;

  AM_MEDIA_TYPE* media_type = NULL;  // Will not compile as a SimpleComPtr for some reason
//  HRESULT hr;

  // get the format of the output pin of the capture filter
//  pPin = GetPin(video_capture_filter, PINDIR_OUTPUT, 0);
//  hr = pPin->QueryInterface(IID_IAMStreamConfig, (void**) &stream_config);
//  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  hr = stream_config->GetFormat(&media_type);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;

  // set the format
  VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*) (media_type->pbFormat);
  BITMAPINFOHEADER* bmih = &(vih->bmiHeader);
  hr = stream_config->SetFormat(media_type);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;

  { // check, just for debugging
    hr = stream_config->GetFormat(&media_type);
    if (hr != S_OK)  FATAL_DSHOW_ERROR;
    VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) (media_type->pbFormat);
    BITMAPINFOHEADER * bmih = &(vih->bmiHeader);
//    assert(bmih->biWidth == width);
//    assert(bmih->biHeight == height);
  }
  DeleteMediaType(media_type);
}

IBaseFilter* AddVideoRenderFilterToGraph(IGraphBuilder* graph_builder)
{
  // create video renderer filter
  SimpleComPtr< IBaseFilter > video_renderer = NULL;
  HRESULT hr;
  hr = CoCreateInstance(CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **) &video_renderer);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  // add video renderer filter to graph
  hr = graph_builder->AddFilter(video_renderer, L"Video Renderer");
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  return video_renderer;
}

IBaseFilter* AddAudioRenderFilterToGraph(IGraphBuilder* graph_builder)
{
  // create audio renderer filter
  SimpleComPtr< IBaseFilter > audio_renderer = NULL;
  HRESULT hr;
  hr = CoCreateInstance(CLSID_AudioRender, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **) &audio_renderer);
  if (hr != S_OK || audio_renderer == NULL)  FATAL_DSHOW_ERROR;
  // add audio renderer filter to graph
  hr = graph_builder->AddFilter(audio_renderer, L"Audio Renderer");
  if (hr != S_OK && hr != VFW_S_DUPLICATE_NAME ) return NULL;
  return audio_renderer;
}

IBaseFilter* AddAviFileReaderFilterToGraph(const char* fname, IGraphBuilder* graph_builder)
{
  SimpleComPtr< IBaseFilter > avi_file_reader = NULL;
  SimpleComPtr< IFileSourceFilter >  file_source_filter = NULL;
  Array<WCHAR> ws;
	HRESULT hr;
  // create AVI file reader filter
  hr = CoCreateInstance(CLSID_AVIDoc, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **) &avi_file_reader);
  if (hr != S_OK || avi_file_reader == NULL)  FATAL_DSHOW_ERROR;
  // add filter to graph
  hr = graph_builder->AddFilter(avi_file_reader, L"AVI file reader");
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  // open file named 'fname'
	hr = avi_file_reader->QueryInterface(IID_IFileSourceFilter, (void **)&file_source_filter);
  if (hr != S_OK || file_source_filter == NULL)  FATAL_DSHOW_ERROR;
	ws.Reset(strlen(fname) + 1);
	MultiByteToWideChar(CP_ACP, 0, fname, -1, ws.Begin(), ws.Len() );
	hr = file_source_filter->Load(ws.Begin(), NULL);
  if (FAILED(hr))  FATAL_DSHOW_ERROR;
  return avi_file_reader;
}

void ConnectPins(IBaseFilter* filt1, int index1, IBaseFilter* filt2, int index2, IGraphBuilder* graph_builder)
{
  SimpleComPtr< IPin > pin_src = GetPin(filt1, PINDIR_OUTPUT, index1);
  SimpleComPtr< IPin > pin_dst = GetPin(filt2, PINDIR_INPUT, index2);
  HRESULT hr = graph_builder->Connect(pin_src, pin_dst);
	if (hr != S_OK)  FATAL_DSHOW_ERROR;
}

void DisconnectPin(IBaseFilter* filt, int index, IGraphBuilder* graph_builder)
{
  SimpleComPtr< IPin > pin_dst = GetPin(filt, PINDIR_INPUT, index);
  HRESULT hr = graph_builder->Disconnect(pin_dst);
	if (hr != S_OK)  FATAL_DSHOW_ERROR;
}

// Adds a DirectShow filter graph to the Running Object Table,
// allowing GraphEdit to "spy" on a remote filter graph.
DWORD AddGraphToRot(IUnknown *pUnkGraph) 
{
  assert(pUnkGraph);
  SimpleComPtr< IMoniker > pMoniker;
  SimpleComPtr< IRunningObjectTable > pROT;
  DWORD rot_id;
  WCHAR wsz[128];
  HRESULT hr;

  hr = GetRunningObjectTable(0, &pROT);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;

  wsprintfW(wsz, L"FilterGraph %08x pid %08x\0", (DWORD_PTR)pUnkGraph, 
            GetCurrentProcessId());

  hr = CreateItemMoniker(L"!", wsz, &pMoniker);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  {
      // Use the ROTFLAGS_REGISTRATIONKEEPSALIVE to ensure a strong reference
      // to the object.  Using this flag will cause the object to remain
      // registered until it is explicitly revoked with the Revoke() method.
      //
      // Not using this flag means that if GraphEdit remotely connects
      // to this graph and then GraphEdit exits, this object registration 
      // will be deleted, causing future attempts by GraphEdit to fail until
      // this application is restarted or until the graph is registered again.
      hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph, 
                          pMoniker, &rot_id);
      if (hr != S_OK)  FATAL_DSHOW_ERROR;
  }
  return rot_id;
}

// Removes a filter graph from the Running Object Table
void RemoveGraphFromRot(DWORD rot_id)
{
  SimpleComPtr< IRunningObjectTable > pROT;
  HRESULT hr = GetRunningObjectTable(0, &pROT);
  if (hr != S_OK)  FATAL_DSHOW_ERROR;
  pROT->Revoke(rot_id);
}

// starts graph if it's stopped or paused
void StartGraph(IMediaControl* media_control)
{
  // Ignoring return value for now.  If no video renderer, then result 
  // is not S_FALSE but the graph seems to play just fine.  So we should
  // check on what return values to expect.
  HRESULT hr = media_control->Run();
//  if (hr != S_FALSE)  FATAL_DSHOW_ERROR;
}

// stops graph if it's running or paused
void StopGraph(IMediaControl* media_control)
{
  HRESULT hr = media_control->Stop();
  if (hr != S_OK && hr != S_FALSE)  FATAL_DSHOW_ERROR;
}

// pauses graph if it's running (does not unpause if it's paused)
void PauseGraph(IMediaControl* media_control)
{
  // No need to check for return value, because it does no good anyway.
  // I.e., S_FALSE just means that the filters have not yet completed
  // their pausing, but they will do so in due time, and there is nothing
  // that can be done about it (as far as I know).
  media_control->Pause();
}

}; // end namespace DirectShowForDummies
