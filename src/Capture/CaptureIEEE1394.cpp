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

#include "CaptureIEEE1394.h"
#include "IEEE1394Camera/stdafx.h"
#include "IEEE1394Camera/1394Camera.h"
#include "Image/ImageOperations.h"  // FlipVertical

//#include "IEEE1394Camera/1394camapi.h"
//#include "IEEE1394Camera/1394common.h"

#pragma warning( disable: 4355 )  // 'this' : used in base member initializer list

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

//  typedef C1394Camera Camera;
//class Camera : public C1394Camera
//{
//};
//
};
// ================< end local functions

#define CCAMERA (static_cast<C1394Camera*>( m_camera ))

namespace blepo
{

CaptureIEEE1394::CaptureIEEE1394()
  : m_thread(this)
{
  m_camera = new C1394Camera;
}

CaptureIEEE1394::~CaptureIEEE1394()
{
  delete CCAMERA;
}

void CaptureIEEE1394::InitCamera()
{
  CCAMERA->InitCamera();
	// checking feature presence
	CCAMERA->InquireControlRegisters();
	// checking feature status
	CCAMERA->StatusControlRegisters();
}

bool CaptureIEEE1394::IsFormatSupported(Format fmt, Fps fps)
{
  int format, mode, rate;
  ConvertFormat(fmt, fps, &format, &mode, &rate);
//	for (int format=0; format<3; format++)
//		for (int mode=0; mode<8; mode++)
//			for (int rate=0; rate<6; rate++)
//        if (CCAMERA->m_videoFlags[format][mode][rate])
//        {
//          int a = 5;
//        }
//        return true;
  return CCAMERA->m_videoFlags[format][mode][rate];
}

int CaptureIEEE1394::GetNumCameras()
{
  C1394Camera tmp;
	if (tmp.CheckLink())
  {
//		AfxMessageBox("No Cameras Found");
    return 0;
  }
	else
	{
    return tmp.GetNumberCameras();
//    int nCameras;
//    CString buf;
//    nCameras = cam.GetNumberCameras();
//		buf.Format("Connection OK, found %d camera(s)",nCameras);
//		AfxMessageBox(buf);
//    return nCameras;
	}
}

void CaptureIEEE1394::ConvertFormat(Format fmt, Fps fps, int* format, int* mode, int* rate)
{
  // defaults
  *format = 0;
  *mode = 4;
  *rate = 4;

  switch (fmt)
  {
  case FMT_160x120_YUV444:    *format = 0; *mode = 0; break;
  case FMT_320x240_YUV422:    *format = 0; *mode = 1; break;
  case FMT_640x480_YUV411:    *format = 0; *mode = 2; break;
  case FMT_640x480_YUV422:    *format = 0; *mode = 3; break;
  case FMT_640x480_RGB:       *format = 0; *mode = 4; break;
  case FMT_640x480_MONO:      *format = 0; *mode = 5; break;
  case FMT_800x600_YUV422:    *format = 1; *mode = 0; break;
  case FMT_800x600_RGB:       *format = 1; *mode = 1; break;
  case FMT_800x600_MONO:      *format = 1; *mode = 2; break;
  case FMT_1024x768_YUV422:   *format = 1; *mode = 3; break;
  case FMT_1024x768_RGB:      *format = 1; *mode = 4; break;
  case FMT_1024x768_MONO:     *format = 1; *mode = 5; break;
  case FMT_1280x960_YUV422:   *format = 2; *mode = 0; break;
  case FMT_1280x960_RGB:      *format = 2; *mode = 1; break;
  case FMT_1280x960_MONO:     *format = 2; *mode = 2; break;
  case FMT_1600x1200_YUV422:  *format = 2; *mode = 3; break;
  case FMT_1600x1200_RGB:     *format = 2; *mode = 4; break;
  case FMT_1600x1200_MONO:    *format = 2; *mode = 5; break;
  default:  assert(0);
  };

  switch (fps)
  {
  case FPS_2:   *rate = 0;  break;
  case FPS_4:   *rate = 1;  break;
  case FPS_7:   *rate = 2;  break;
  case FPS_15:  *rate = 3;  break;
  case FPS_30:  *rate = 4;  break;
  case FPS_60:  *rate = 5;  break;
  default:  assert(0);
  }
}

void CaptureIEEE1394::SelectCamera(int index)
{
  // might need to call CheckLink() and/or GetNumberCameras() here to let underlying
  // C1394Camera object know that we're ready to use this camera. -- STB
  CCAMERA->SelectCamera(index);
  CCAMERA->m_cameraInitialized = false;
  InitCamera();
}

bool CaptureIEEE1394::SetFormat(Format fmt, Fps fps, bool autostuff)
{
  if (!IsFormatSupported(fmt, fps))
  {
    printf("ERROR: Format not supported by this camera\n");
    return 0;
    BLEPO_ERROR("Format not supported by this camera");
  }
  int format, mode, rate;
  ConvertFormat(fmt, fps, &format, &mode, &rate);
  CCAMERA->SetVideoFormat(format);
  CCAMERA->SetVideoMode(mode);
  CCAMERA->SetVideoFrameRate(rate);
  if (autostuff)  SetAutoParams();
  return 1;
}

//Sets camera parameters to automatic
void CaptureIEEE1394::SetAutoParams()
{
  CCAMERA->m_controlAutoExposure.SetAutoMode(true);
  CCAMERA->m_controlBrightness.SetAutoMode(true);
  CCAMERA->m_controlWhiteBalance.SetAutoMode(true);
}

unsigned long CaptureIEEE1394::CamThread::MainRoutine()
{
  while (1)
  {
    m_camera->GrabFrame();
    Sleep(10);
  }
  return 0;
}

void CaptureIEEE1394::GrabFrame()
{
  m_semaphore.Wait(0);  // clear semaphore
  m_mutex.Lock();
 	if (CCAMERA->AcquireImage())
		AfxMessageBox("Problem Acquiring Image");
	CCAMERA->getDIB(m_img.BytePtr());
  m_mutex.Unlock();
  m_semaphore.Signal();  // signal semaphore
}

void CaptureIEEE1394::Start()
{
  m_img.Reset(CCAMERA->m_width, CCAMERA->m_height);
	if (CCAMERA->StartImageAcquisition())
		AfxMessageBox("Problem Starting Image Acquisition");
  m_thread.Start();
}

void CaptureIEEE1394::Stop()
{
  m_thread.Suspend();
	if (CCAMERA->StopImageAcquisition())
		AfxMessageBox("Problem Stopping Image Acquisition");
}

void CaptureIEEE1394::Pause()
{
}

//void CaptureIEEE1394::GrabLatestFrame(ImgBgr* out)
//{
//  m_mutex.Lock();
//  *out = m_img;
//  FlipVertical(out);
//  m_mutex.Unlock();
//}

bool CaptureIEEE1394::GetLatestImage(ImgBgr* out, int timeout)
{
  bool signaled = m_semaphore.Wait(timeout);
  if (signaled)
  {
    m_mutex.Lock();
    *out = m_img;
    m_mutex.Unlock();
    FlipVertical(*out, out);
    return true;
  }
  else
  {
    return false;
  }
}

};  // end namespace blepo

