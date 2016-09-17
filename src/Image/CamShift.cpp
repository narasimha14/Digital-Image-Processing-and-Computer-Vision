/* 
 * Copyright (c) 2008 Clemson University.
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

#include "Image.h"
#include "ImgIplImage.h"
#include "ImageAlgorithms.h"

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

namespace blepo {


CamShift::CamShift(const ImgBgr& img)
{
  
  hist = NULL;
  hue = NULL;
  hdims = 16;
  hsv = NULL;
  backproject = NULL;
  histimg = NULL;
  mask = NULL;
  
  float hranges_arr[] = {0,180};
  float* hranges = hranges_arr;
  ImgIplImage frame(img);
  image = cvCreateImage( cvGetSize(frame), 8, 3 );
  image->origin = ((IplImage*) frame)->origin;
  hsv = cvCreateImage( cvGetSize(frame), 8, 3 );
  hue = cvCreateImage( cvGetSize(frame), 8, 1 );
  mask = cvCreateImage( cvGetSize(frame), 8, 1 );
  backproject = cvCreateImage( cvGetSize(frame), 8, 1 );
  hist = cvCreateHist( 1, &hdims, CV_HIST_ARRAY, &hranges, 1 );
  histimg = cvCreateImage( cvSize(320,200), 8, 3 );
  cvZero( histimg );
  //cvReleaseImage( &frame );
  vmin = 10; vmax = 256; smin = 30;

}

CamShift::~CamShift()
{
   cvReleaseImage(&image);
   cvReleaseImage(&hsv);
   cvReleaseImage(&hue);
   cvReleaseImage(&mask);
   cvReleaseImage(&backproject);
   cvReleaseImage(&histimg);
}

CamShift::Box CamShift::Track(const ImgBgr& img)
{
	cvCopy(ImgIplImage(img), image, 0 );
  cvCvtColor( image, hsv, CV_BGR2HSV );
  cvFlip(hsv,hsv,0);
  int _vmin = vmin, _vmax = vmax;
  
  cvInRangeS( hsv, cvScalar(0,smin,MIN(_vmin,_vmax),0),
  cvScalar(180,256,MAX(_vmin,_vmax),0), mask );
  cvSplit( hsv, hue, 0, 0, 0 );
  cvCalcBackProject( &hue, backproject, hist );
  //cvSaveImage("backproject.bmp", backproject);
  cvAnd( backproject, mask, backproject, 0 );
  //cvSaveImage("backproject.bmp", backproject);
  cvCamShift( backproject, track_window,
    cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ),
    &track_comp, &track_box );
  track_window = track_comp.rect;

  Box result;
  result.angle= track_box.angle;
  result.center.x= static_cast<LONG>( track_box.center.x );
  result.center.y= static_cast<LONG>( img.Height()-track_box.center.y-1 );
  result.size.cy = static_cast<LONG>( track_box.size.width );
  result.size.cx = static_cast<LONG>( track_box.size.height );
  return result;
}

void CamShift::CalcHistogram(const ImgBgr& img, const CRect& sel)
{
  selection.x = sel.left;
  selection.y = img.Height()-sel.bottom-1;
  selection.width = sel.Width();
  selection.height = sel.Height();

  cvCopy(ImgIplImage(img), image, 0 );
  cvCvtColor( image, hsv, CV_BGR2HSV );
  cvFlip(hsv,hsv,0);
  //cvSaveImage("hsv.bmp", hsv);
  //cvSaveImage("img.bmp", image);
  int _vmin = vmin, _vmax = vmax;
  cvInRangeS( hsv, cvScalar(0,smin,MIN(_vmin,_vmax),0),
  cvScalar(180,256,MAX(_vmin,_vmax),0), mask );
  cvSplit( hsv, hue, 0, 0, 0 );
  float max_val = 0.f;
  cvSetImageROI(hue, selection );
  cvSetImageROI( mask, selection );
  cvCalcHist( &hue, hist, 0, mask );
  cvGetMinMaxHistValue( hist, 0, &max_val, 0, 0 );
  cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );
  cvResetImageROI( hue );
  cvResetImageROI( mask );
  track_window = selection;
//  cvZero( histimg );
//  int bin_w = histimg->width / hdims;
//  for(int i = 0; i < hdims; i++ )
//  {
//    int a = cvGetReal1D(hist->bins,i);
//    int val = cvRound( cvGetReal1D(hist->bins,i)*histimg->height/255 );
//    CvScalar color = hsv2rgb(i*180.f/hdims);
//    cvRectangle( histimg, cvPoint(i*bin_w,histimg->height),
//      cvPoint((i+1)*bin_w,histimg->height - val),
//      color, -1, 8, 0 );
//  }
//  cvNamedWindow( "Histogram", 1 );
//  
//  cvShowImage( "Histogram", histimg );
}

};  // end namespace blepo

