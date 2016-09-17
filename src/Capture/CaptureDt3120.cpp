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

#include "CaptureDt3120.h"
//#include <windows.h>
//#include "../../external/include/DT3120/olwintyp.h"
//#include "../../external/include/DT3120/olfgapi.h"
//#include "../../external/include/DT3120/olimgapi.h"
#include "../../external/DataTranslation/DT3120/DtColorSdk.h"         // you may have to changes the "" for braces
#include "Utilities/Exception.h"

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

// This is a macro that dumps on the screen
// the error message if an error occurs.
#define DT_TRY(a) {\
char Message[1000];\
DWORD Error;\
Error = a;\
OlImgGetStatusMessage(Error,Message,1000);\
if (Error)\
{\
  BLEPO_ERROR(Message);\
}\
}

// ================> begin local functions (available only to this translation unit)
namespace {

struct Stuff
{
  OLT_IMG_DEV_ID dev_id;
  OLT_FG_FRAME_ID frame_id;
  OLT_IMAGE_MODE storage;
  ULNG timeout;
};

};
// ================< end local functions

// This macro makes it easier to access 'm_stuff', which is void* in the header file to prevent header leak.
#define SSTUFF (static_cast<Stuff*>( m_stuff ))

namespace blepo
{

CaptureDt3120::CaptureDt3120(int image_size, bool color)
  : m_stuff(NULL)
{
  m_stuff = static_cast<void*>( new Stuff );

  OLT_SCALE_PARAM scale;
  int scaling;

  // setup image dimensions, depth, and scaling
  // (Note:  scaling appears to be a percentage, with 100 means full size)
  switch (image_size)
  {
    case 1:  m_width = 640;  m_height = 480;  scaling = 100;  break;
    case 2:  m_width = 320;  m_height = 240;  scaling =  50;  break;
    case 4:  m_width = 160;  m_height = 120;  scaling =  25;  break;
    default:  BLEPO_ERROR("Illegal image size");
  }
  m_depth = color ? 3 : 1;


  // Open the device -- the name has to be the same that the one you put
  // in the control panel.
  DT_TRY(OlImgOpenDevice("DT3120K-1",&(SSTUFF->dev_id)));	// DT3120K-1, change device to match your installed board

  DT_TRY(OlImgSetTimeoutPeriod(SSTUFF->dev_id,10,&(SSTUFF->timeout)));

  // Required to capture a composite (monochrome) video signal
  // signal = OLC_MONO_SIGNAL;
  // DT_TRY(DtColorSignalType(DevId, 0, OLC_WRITE_CONTROL, &signal));	//may need to change channel #, however use channel 2 to capture (1st) S-Video signal with a DT313x

  // Required to set Storage Mode to monochrome
  SSTUFF->storage = (m_depth==1) ? OLC_IMAGE_MONO : OLC_IMAGE_RGB_24;
  DT_TRY(DtColorStorageMode(SSTUFF->dev_id, 0, OLC_WRITE_CONTROL, &(SSTUFF->storage)));	

  // Allocate one frame
  DT_TRY(OlFgAllocateBuiltInFrame(SSTUFF->dev_id, OLC_FG_DEV_MEM_VOLATILE, OLC_FG_NEXT_FRAME,&(SSTUFF->frame_id)))

  scale.hscale = scaling;
  scale.vscale = scaling;
  DT_TRY(DtColorHardwareScaling(SSTUFF->dev_id,0,OLC_WRITE_CONTROL,&scale))		//may need to change channel #
}

CaptureDt3120::~CaptureDt3120()
{
  DT_TRY(OlImgCloseDevice(SSTUFF->dev_id));
  delete SSTUFF;
}

void CaptureDt3120::ReadOneFrame(ImgBgr* out)
{
  if (m_depth != 3)  BLEPO_ERROR("Cannot get a BGR image when capturing grayscale");
  out->Reset(m_width, m_height);
  iReadOneFrame((unsigned char*)out->Begin());
}

void CaptureDt3120::ReadOneFrame(ImgGray* out)
{
  if (m_depth != 1)  BLEPO_ERROR("Cannot get a BGR image when capturing grayscale");
  out->Reset(m_width, m_height);
  iReadOneFrame(out->Begin());
}
void CaptureDt3120::iReadOneFrame(unsigned char* p)
{
  // Acquire one frame
  DT_TRY(OlFgAcquireFrameToDevice(SSTUFF->dev_id, SSTUFF->frame_id));
  DT_TRY(OlFgReadFrameRect(SSTUFF->dev_id, SSTUFF->frame_id, 0, 0, m_width, m_height, p, m_width*m_height*m_depth));
}


};  // end namespace blepo

