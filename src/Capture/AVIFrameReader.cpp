// AVIFrameReader.cpp: implementation of the AVIFrameReader class.
//
//////////////////////////////////////////////////////////////////////

#include "AVIFrameReader.h"
#include "blepo_opencv.h"  // CvCapture -- OpenCV
//#include "highgui.h"  // CvCapture -- OpenCV
#include "../Image/ImgIplImage.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace blepo
{

AVIFrameReader::AVIFrameReader()
{
    m_ok = false;
    m_FPS = -1;
    m_width = -1;
    m_height = -1;
}

int AVIFrameReader::Initialize(const char* filename, int start_frame, int skip_frame)
{    
  if(m_ok)
  {
    CloseStream();
    m_ok = false;
    m_FPS = -1;
    m_width = -1;
    m_height = -1;
  }

    AVIFileInit();

    
    res=AVIFileOpen(&avi, filename, OF_READ, NULL);

    if (res!=AVIERR_OK)
    {
        //an error occures
        if (avi!=NULL)
            AVIFileRelease(avi);
        
        return(-1);
    }

    
    AVIFileInfo(avi, &avi_info, sizeof(AVIFILEINFO));

//    CString szFileInfo;
//    szFileInfo.Format("Dimention: %dx%d\n"
//                      "Length: %d frames\n"
//                      "Max bytes per second: %d\n"
//                      "Samples per second: %d\n"
//                      "Streams: %d\n"
//                      "File Type: %d", avi_info.dwWidth,
//                            avi_info.dwHeight,
//                            avi_info.dwLength,
//                            avi_info.dwMaxBytesPerSec,
//                            (DWORD) (avi_info.dwRate / avi_info.dwScale),
//                            avi_info.dwStreams,
//                            avi_info.szFileType);
//
//    AfxMessageBox(szFileInfo, MB_ICONINFORMATION | MB_OK);

    
    m_width = avi_info.dwWidth;
    m_height = avi_info.dwHeight;
    m_FPS = (DWORD) (avi_info.dwRate / avi_info.dwScale);

    res=AVIFileGetStream(avi, &pStream, streamtypeVIDEO /*video stream*/, 
                                               0 /*first stream*/);

    if (res!=AVIERR_OK)
    {
        if (pStream!=NULL)
            AVIStreamRelease(pStream);

        AVIFileExit();
        return(-1);
    }

    //do some task with the stream
    int iNumFrames;
    int iFirstFrame;

    iFirstFrame=AVIStreamStart(pStream);

    if (iFirstFrame==-1)
    {
        //Error getteing the frame inside the stream

        if (pStream!=NULL)
            AVIStreamRelease(pStream);

        AVIFileExit();
        return(-1);
    }

    iNumFrames=AVIStreamLength(pStream);
    
    
    if (iNumFrames==-1)
    {
        //Error getteing the number of frames inside the stream
        
        if (pStream!=NULL)
            AVIStreamRelease(pStream);
        
        AVIFileExit();
        return(-1);
    }

    //getting bitmap from frame
    

    ZeroMemory(&bih, sizeof(BITMAPINFOHEADER));

    bih.biBitCount=24;    //24 bit per pixel
    bih.biClrImportant=0;
    bih.biClrUsed = 0;
    bih.biCompression = BI_RGB;
    //bih.biCompression = 'DIVX';
    bih.biPlanes = 1;
    bih.biSize = 40;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    //calculate total size of RGBQUAD scanlines (DWORD aligned)
    bih.biSizeImage = (((bih.biWidth * 3) + 3) & 0xFFFC) * bih.biHeight ;

		//pFrame=AVIStreamGetFrameOpen(pStream, (BITMAPINFOHEADER*) AVIGETFRAMEF_BESTDISPLAYFMT);
		pFrame=AVIStreamGetFrameOpen(pStream, (BITMAPINFOHEADER*) &bih);
    //pFrame=AVIStreamGetFrameOpen(pStream, NULL);

    //pFrame=AVIStreamGetFrameOpen(pStream, &bih);
    
    //pFrame=AVIStreamGetFrameOpen(pStream, 
    //       NULL/*(BITMAPINFOHEADER*) AVIGETFRAMEF_BESTDISPLAYFMT*/ /*&bih*/);


    //Get the first frame

    //AVIFileInfo(avi, &avi_info, sizeof(AVIFILEINFO));

    //m_first = iFirstFrame;
    m_first = start_frame;
    m_skip = skip_frame;
    m_index = m_first;
    m_last = iNumFrames;

    m_ok = true;
    return(m_last);

}

long AVIFrameReader::ReadFrame(long frame, ImgBgr* bgr)
{
    bgr->Reset(avi_info.dwWidth, avi_info.dwHeight);

        if(frame >= m_last) 
          return -1;
					
				if(frame < 0)
				{
					return -1;
				}

        //m_index = frame;
        BYTE* pDIB = (BYTE*) AVIStreamGetFrame(pFrame, frame);
        BITMAPINFOHEADER bh;
        memcpy(&bh, pDIB,sizeof(BITMAPINFOHEADER));
        memcpy(bgr->BytePtr(), pDIB + sizeof(BITMAPINFOHEADER), avi_info.dwWidth * avi_info.dwHeight * 3);
        FlipVertical(*bgr, bgr);
        //m_index = m_index + m_skip;
        
        return(frame);
}

long AVIFrameReader::ReadNextFrame(ImgBgr* bgr)
{
    bgr->Reset(avi_info.dwWidth, avi_info.dwHeight);

        if(m_index >= m_last) 
          return -1;

				if(m_index < 0)
				{
					return -1;
				}

        BYTE* pDIB = (BYTE*) AVIStreamGetFrame(pFrame, m_index);
        memcpy(bgr->BytePtr(), pDIB + sizeof(BITMAPINFOHEADER), avi_info.dwWidth * avi_info.dwHeight * 3);
        FlipVertical(*bgr, bgr);
        m_index = m_index + m_skip;
        
        return(m_index);
}



void AVIFrameReader::CloseStream()
{
  if(m_ok)
  {
    AVIStreamGetFrameClose(pFrame);

    //close the stream after finishing the task
    if (pStream!=NULL)
		{
        AVIStreamRelease(pStream);
		}
		AVIFileRelease(avi);
    AVIFileExit();

    m_ok = false;
  }

}

AVIFrameReader::~AVIFrameReader()
{
  CloseStream();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////

struct AVIReaderOpenCv::Data
{
  Data() : m_cap(NULL) {}
  CvCapture* m_cap;
};


AVIReaderOpenCv::AVIReaderOpenCv()
  : m_data(NULL)
{
  m_data = new Data();

}

AVIReaderOpenCv::~AVIReaderOpenCv()
{
  if (m_data->m_cap) cvReleaseCapture( &m_data->m_cap );
  delete m_data;
}

bool AVIReaderOpenCv::OpenFile(const char* filename)
{
  m_data->m_cap = cvCreateFileCapture( filename );
  return m_data->m_cap != NULL;
}

bool AVIReaderOpenCv::GrabNextFrame(ImgBgr* out)
{
  int ret = cvGrabFrame( m_data->m_cap );
  if (ret == 0)  return false;
  IplImage* img = cvRetrieveFrame( m_data->m_cap );
  if (img == NULL)  return false;
  ImgIplImage::CastIplImgToBgr( img, out );
  return true;
}



};