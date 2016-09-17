/* 
 * Copyright (c) 2004,2005 Clemson University.
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
 *
 * Portions of this code were adapted from sample code in the OpenCV library.
 */

#include "Image.h"
#include "ImgIplImage.h"
#include "ImageAlgorithms.h"
#include "blepo_opencv.h" // OpenCV

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

// ================> begin local functions (available only to this translation unit)
namespace
{

CStringA iGetExecutableDirectory()
{
  CStringA help_path = AfxGetApp()->m_pszHelpFilePath;
  char* s = const_cast<char*>( strrchr(help_path, '\\') );
  *(s+1) = '\0';
  return CStringA(help_path);
}

};
// ================< end local functions


namespace blepo {

void PatternDetector::Init(const char* xml_filename)
{
  /// 'also_check_in_default_directory' If true, and if xml_filename is
  ///        not found, then the xml_filename is looked for in the default directory
  ///        (OpenCV/haarcascades of the Blepo installation used in
  ///         compiling the program.)
  /// 'also_check_in_executable_directory' If true, and if xml_filename is
  ///        not found, then the xml_filename is looked for in the directory of the
  ///        current executable.
  bool also_check_in_default_directory = true;
  bool also_check_in_executable_directory = true;

  if (m_cascade)
  {
    BLEPO_ERROR(StringEx("Pattern detector already initialized"));
  }

  CStringA cascade_name = xml_filename;

  // Loads a trained cascade classifier from file or the classifier database embedded in OpenCV
  // First try then current directory, then try Blepo OpenCV directory
  m_cascade = (CvHaarClassifierCascade*) cvLoad( cascade_name, 0, 0, 0 );
  if (!m_cascade && also_check_in_default_directory)
  {
    CStringA dir = __FILE__;
    dir = dir.Left( dir.ReverseFind('\\') );
    cascade_name = dir + "/../../external/OpenCV-2/haarcascades/" + cascade_name;
    m_cascade = (CvHaarClassifierCascade*) cvLoad( cascade_name, 0, 0, 0 );
  }
  if (!m_cascade && also_check_in_executable_directory)
  {
    cascade_name = xml_filename;
    cascade_name = iGetExecutableDirectory() + cascade_name;
    m_cascade = (CvHaarClassifierCascade*) cvLoad( (const char*) cascade_name, 0, 0, 0 );
  }

  if (!m_cascade)  
  {
    BLEPO_ERROR(StringEx("Unable to load Haar classifier cascade '%s'", xml_filename));
  }

	m_storage = cvCreateMemStorage(0);
}

PatternDetector::PatternDetector()
  : m_storage(NULL), m_cascade(NULL)
{
}

PatternDetector::PatternDetector(const char* xml_filename)
  : m_storage(NULL), m_cascade(NULL)
{
  Init(xml_filename);
}

PatternDetector::~PatternDetector()
{
//	cvReleaseHidHaarClassifierCascade( &m_hid_cascade ); //added by DKK; commented by STB
	cvReleaseMemStorage( &m_storage ); //DKK
}

void PatternDetector::DetectPatterns(const ImgBgr& img, std::vector<Rect>* rectlist)
{
  int scale = 1;
	rectlist->clear();

  cvClearMemStorage( m_storage );
  
	if (m_cascade)
	{
		// do the detection
    ImgIplImage iplimg(img);
		CvSeq* patterns = cvHaarDetectObjects(iplimg, m_cascade, m_storage, 1.2, 2, CV_HAAR_DO_CANNY_PRUNING, cvSize(40, 40) );

		// convert to std::vector<CRect>
	  CvPoint pt1, pt2;
    for (int i = 0; i < (patterns ? patterns->total : 0); i++ )
		{
			CvRect* r = (CvRect*) cvGetSeqElem( patterns, i );
			pt1.x = r->x*scale;
			pt2.x = (r->x+r->width)*scale;
			pt1.y = r->y*scale;
			pt2.y = (r->y+r->height)*scale;
			rectlist->push_back( CRect(pt1.x, pt1.y, pt2.x, pt2.y) );
		}

		//Seems like patterns should be "released" but I can't find a CV function that would do that -- DKK
	}
}

void PatternDetector::DetectPatterns(const ImgBgr& img, std::vector<Rect>* rectlist, float scale_incr_ratio, int grouping_param, bool do_canny_prunning, int min_size)
{
  int scale = 1;
	rectlist->clear();

  cvClearMemStorage( m_storage );
  
	if (m_cascade)
	{
		// do the detection
    ImgIplImage iplimg(img);
    CvSeq* patterns;
    if(do_canny_prunning)
    {
		  patterns = cvHaarDetectObjects(iplimg, m_cascade, m_storage, scale_incr_ratio, grouping_param, CV_HAAR_DO_CANNY_PRUNING, cvSize(min_size, min_size) );
    }
    else
    {
      patterns = cvHaarDetectObjects(iplimg, m_cascade, m_storage, scale_incr_ratio, grouping_param, NULL, cvSize(min_size, min_size) );
    }

		// convert to std::vector<CRect>
	  CvPoint pt1, pt2;
    for (int i = 0; i < (patterns ? patterns->total : 0); i++ )
		{
			CvRect* r = (CvRect*) cvGetSeqElem( patterns, i );
			pt1.x = r->x*scale;
			pt2.x = (r->x+r->width)*scale;
			pt1.y = r->y*scale;
			pt2.y = (r->y+r->height)*scale;
			rectlist->push_back( CRect(pt1.x, pt1.y, pt2.x, pt2.y) );
		}

		//Seems like patterns should be "released" but I can't find a CV function that would do that -- DKK
	}
}


void PatternDetector::DetectPatterns(const ImgBgr& img, std::vector<Rect>* rectlist, float scale_incr_ratio, int grouping_param, bool do_canny_prunning, Point min_size)
{
  int scale = 1;
	rectlist->clear();

  cvClearMemStorage( m_storage );
  
	if (m_cascade)
	{
		// do the detection
    ImgIplImage iplimg(img);
    CvSeq* patterns;
    if(do_canny_prunning)
    {
		  patterns = cvHaarDetectObjects(iplimg, m_cascade, m_storage, scale_incr_ratio, grouping_param, CV_HAAR_DO_CANNY_PRUNING, cvSize(min_size.x, min_size.y) );
    }
    else
    {
      patterns = cvHaarDetectObjects(iplimg, m_cascade, m_storage, scale_incr_ratio, grouping_param, NULL, cvSize(min_size.x, min_size.y) );
    }

		// convert to std::vector<CRect>
	  CvPoint pt1, pt2;
    for (int i = 0; i < (patterns ? patterns->total : 0); i++ )
		{
			CvRect* r = (CvRect*) cvGetSeqElem( patterns, i );
			pt1.x = r->x*scale;
			pt2.x = (r->x+r->width)*scale;
			pt1.y = r->y*scale;
			pt2.y = (r->y+r->height)*scale;
			rectlist->push_back( CRect(pt1.x, pt1.y, pt2.x, pt2.y) );
		}

		//Seems like patterns should be "released" but I can't find a CV function that would do that -- DKK
	}
}

FaceDetector::FaceDetector()
{
//  CString cascade_name = "haarcascade_frontalface_alt.xml";
//  CString cascade_name = "haarcascade_profileface.xml";
//  CString cascade_name = "haarcascade_upperbody.xml";
//  CString cascade_name = "haarcascade_fullbody.xml";
  m_frontal_detector.Init("haarcascade_frontalface_alt.xml");
  m_profile_detector.Init("haarcascade_profileface.xml");
}

FaceDetector::~FaceDetector()
{
}

void FaceDetector::DetectAllFaces(const ImgBgr& img, 
                                  std::vector<Rect>* rectlist_frontal,
                                  std::vector<Rect>* rectlist_left,
                                  std::vector<Rect>* rectlist_right)
{
  DetectFrontalFaces(img, rectlist_frontal);
  DetectLeftProfileFaces(img, rectlist_left);
  DetectRightProfileFaces(img, rectlist_right);
}

void FaceDetector::DetectFrontalFaces(const ImgBgr& img, std::vector<Rect>* rectlist)
{
  m_frontal_detector.DetectPatterns(img, rectlist);
}

void FaceDetector::DetectLeftProfileFaces(const ImgBgr& img, std::vector<Rect>* rectlist)
{
  FlipHorizontal(img, &m_img_tmp);
  m_profile_detector.DetectPatterns(m_img_tmp, rectlist);

  // Since image is flipped, need to flip rectangles
  int w = img.Width() - 1;
  for (int i = 0 ; i < (int) rectlist->size() ; i++)
  {
    CRect& r = (*rectlist)[i];
    r.left  = w - r.left;
    r.right = w - r.right;
  }
}

void FaceDetector::DetectRightProfileFaces(const ImgBgr& img, std::vector<Rect>* rectlist)
{
  m_profile_detector.DetectPatterns(img, rectlist);
}


};  // end namespace blepo

