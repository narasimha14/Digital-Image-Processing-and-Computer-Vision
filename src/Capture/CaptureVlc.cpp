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

#include "CaptureVlc.h"
//#include "Utilities/Math.h"
#include "Image/ImageOperations.h"  // Load

#define DUMMY_PARAMETER_NEEDED_FOR_CREATE_PROCESS  "dummyparam"
#define TEMP_IMG_FILE_PATH  "C:\\"
#define TEMP_IMG_FILE_NAME  "temp_vlc_img"
#define TEMP_IMG_FILE_TYPE  "bmp"
#define DEFAULT_VLC_EXECUTABLE_PATH  "C:\\Program Files\\VideoLAN\\VLC\\vlc.exe"
#define DEFAULT_VLC_PARAMS  DUMMY_PARAMETER_NEEDED_FOR_CREATE_PROCESS \
                            " --video-filter=scene" \
                            " --scene-prefix=" TEMP_IMG_FILE_NAME \
                            " --scene-format=" TEMP_IMG_FILE_TYPE \
                            " --scene-ratio=1" \
                            " --scene-path=" TEMP_IMG_FILE_PATH \
                            " --scene-replace "
#define TEMP_IMG_FILE_FULLNAME  TEMP_IMG_FILE_PATH TEMP_IMG_FILE_NAME "." TEMP_IMG_FILE_TYPE

//#define DEFAULT_VLC_PARAMS  DUMMY_PARAMETER_NEEDED_FOR_CREATE_PROCESS "--video-filter=scene --scene-prefix=temp_vlc_img --scene-format=bmp --scene-ratio=1 --scene-path=c:\\ --scene-replace "

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

using namespace blepo;

//  // Sample code:
//  STARTUPINFO siStartupInfo;
//  PROCESS_INFORMATION piProcessInfo;
//  memset(&siStartupInfo, 0, sizeof(siStartupInfo));
//  memset(&piProcessInfo, 0, sizeof(piProcessInfo));
//  siStartupInfo.cb = sizeof(siStartupInfo);
//
//  CString vlc_exe = "C:\\Program Files\\VideoLAN\\VLC\\vlc.exe";
//  CString vlc_params = "xyz --video-filter=scene --scene-prefix=img --scene-format=bmp --scene-ratio=1 --scene-path=c:\\all_people\\stb\\data\\currimg --scene-replace C:\\test.mpg";
//  char tmp_params[1000];
//  strncpy(tmp_params, vlc_params, 1000);
//  CreateProcessA(vlc_exe, tmp_params, 0, 0, FALSE, 0, 0, 0, &siStartupInfo, &piProcessInfo);


namespace blepo
{

struct CaptureVlc::ProcessStuff
{
  ProcessStuff() { ClearStuff(); } 
  ~ProcessStuff() {}

  void ClearStuff()
  {
    memset(&siStartupInfo, 0, sizeof(siStartupInfo));
    memset(&piProcessInfo, 0, sizeof(piProcessInfo));
    siStartupInfo.cb = sizeof(siStartupInfo);
  }

  STARTUPINFO siStartupInfo;
  PROCESS_INFORMATION piProcessInfo;
};

CaptureVlc::CaptureVlc()
  : m_vlc_path(DEFAULT_VLC_EXECUTABLE_PATH), m_vlc_params(DEFAULT_VLC_PARAMS), m_input_source(),
    m_is_running(false),
    m_stuff(NULL)
{
  m_stuff = new ProcessStuff();
}

CaptureVlc::~CaptureVlc()
{
  delete m_stuff;
}

void CaptureVlc::SetInputSource(const char* source)
{
  m_input_source = source;
}

// Get / Set VLC executable filename (with full path)
CString CaptureVlc::GetVlcExecutableName() const
{
  return m_vlc_path;
}

void CaptureVlc::SetExecutableVlcName(const char* vlc_path)
{
  m_vlc_path = vlc_path;
}

// Get / Set command line parameters (for fine-tuned control over program, not necessary for average user)
CString CaptureVlc::GetVlcCmdLineParams() const
{
  return m_vlc_params;
}

void CaptureVlc::SetVlcCmdLineParams(const char* vlc_params)
{
  m_vlc_params = vlc_params;
}

void CaptureVlc::Start(bool use_input_source)
{
  assert(!m_is_running);
  CString params = m_vlc_params;
  if (use_input_source)
  {
    params += " ";
    params += m_input_source;
  }

  {
    m_stuff->ClearStuff();
//    STARTUPINFO siStartupInfo;
//    PROCESS_INFORMATION piProcessInfo;
//    memset(&m_stuff->siStartupInfo, 0, sizeof(m_stuff->siStartupInfo));
//    memset(&m_stuff->piProcessInfo, 0, sizeof(m_stuff->piProcessInfo));
//    m_stuff->siStartupInfo.cb = sizeof(m_stuff->siStartupInfo);

  //  CString vlc_params = "--video-filter=scene --scene-prefix=img --scene-format=bmp --scene-ratio=1 --scene-path=c:/all_people/stb/data/currimg \"C:/Program Files/VideoLAN/test.mpg\"";
//    CString vlc_params = "xyz --video-filter=scene --scene-prefix=img --scene-format=bmp --scene-ratio=1 --scene-path=c:\\all_people\\stb\\data\\currimg --scene-replace C:\\test.mpg";
    char tmp_params[1000];
    strncpy(tmp_params, params, 1000);
    BOOL ret = CreateProcessA(m_vlc_path, tmp_params, 0, 0, FALSE, 0, 0, 0, &m_stuff->siStartupInfo, &m_stuff->piProcessInfo);
    assert( ret != 0 );
  }

  m_is_running = true;


//   dwExitCode = WaitForSingleObject(piProcessInfo.hProcess, (SecondsToWait * 1000)); 
//   CloseHandle(piProcessInfo.hProcess);
//   CloseHandle(piProcessInfo.hThread); 
}

void CaptureVlc::Stop()
{
  TerminateProcess(m_stuff->piProcessInfo.hProcess, 0);
  CloseHandle(m_stuff->piProcessInfo.hProcess);
  CloseHandle(m_stuff->piProcessInfo.hThread); 
  m_is_running = false;
}

bool CaptureVlc::IsRunning() const
{
  return m_is_running;
}

bool CaptureVlc::GetLatestImage(ImgBgr* out)
{
  CString str = TEMP_IMG_FILE_FULLNAME;
  try
  {
    Load(str, out);
    return true;
  } 
  catch (Exception& )
  {
    return false;
  }
}

bool CaptureVlc::GetLatestImage(ImgGray* out)
{
  CString str = TEMP_IMG_FILE_FULLNAME;
  try
  {
    Load(str, out);
    return true;
  } 
  catch (Exception& )
  {
    return false;
  }
}

};