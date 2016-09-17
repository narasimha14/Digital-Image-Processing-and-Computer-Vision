/* 
 * Copyright (c) 2007 Clemson University.
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


#if !defined(AFX_AVIFRAMEREADER_H__61681B38_A4A8_42A8_A78D_A3115416CEF8__INCLUDED_)
#define AFX_AVIFRAMEREADER_H__61681B38_A4A8_42A8_A78D_A3115416CEF8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../Image/Image.h"
#include "../Image/ImageOperations.h"
#include <vfw.h>


/**
@class AVIFrameReader
This class enables you to open an AVI file and extract frames from it.
The class uses older Video for Windows (VFW) API. I wrote this class without noticing the 
CaptureAvi class that is already in Blepo under src/Capture. Since this class uses
an older VFW API, the implementation is simpler, but might run into problems for future codecs and
also might lack extensibility compared to DirectShow.

@bug  Fails for video having odd-number dimensions.

@author Neeraj Kanhere (nkanher) Jan 2007
Based on original code by  A. Riazi, Shafiee 
(http://www.codeproject.com/audio/ExtractAVIFrames.asp)


@usage
vfw32.lib needs to be included in additional libraries under project settings.
AVIFrameReader reader;
reader.InitReader(avi_filename);
reader.ReadFrame(frame, &bgr_img);

Tested with 
640x480, 320x240, 500x500 RGB-24 uncompressed 
640x480, 320x240, 500x500 Color Indeo video 5.10 codec (Default AVI format for Logitech Orbit camera)
*/

namespace blepo
{


class AVIFrameReader  
{
public:
	AVIFrameReader();
	virtual ~AVIFrameReader();
  
  int Initialize(const char* filename, int start_frame=0, int skip_frame=1);
  long ReadNextFrame(ImgBgr* bgr);
  long ReadFrame(long frame, ImgBgr* bgr);
  void CloseStream();
  
  double GetFPS()     {return m_FPS;}
  int GetHeight()     {return m_height;}
  int GetWidth()      {return m_width;}
  long GetFrameCount() {return m_last;}
  bool Initialized()  {return m_ok;}

protected:
  bool m_ok;
  int  m_skip;
  long m_first;
  long m_last;
  long m_index;
  int  res;
  
  double  m_FPS;
  int  m_width;
  int  m_height;

  BITMAPINFOHEADER  bih;
  PAVIFILE          avi;
  AVIFILEINFO       avi_info;
  PAVISTREAM        pStream; 
  PGETFRAME         pFrame;
  

};

/** This class wraps OpenCV's AVI frame reader.  It is generally more robust than the class above.
Note:  OpenCV also has AVI frame writing capability, but we have not yet wrapped it.
*/

class AVIReaderOpenCv
{
public:
  AVIReaderOpenCv();
  ~AVIReaderOpenCv();

  // returns true for success, false otherwise
  bool OpenFile(const char* filename);

  // returns true for success, false otherwise
  bool GrabNextFrame(ImgBgr* out);

private:
  struct Data;
  Data* m_data;
};

};

#endif // !defined(AFX_AVIFRAMEREADER_H__61681B38_A4A8_42A8_A78D_A3115416CEF8__INCLUDED_)
