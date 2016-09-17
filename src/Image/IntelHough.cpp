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

OpenCvHough::OpenCvHough()
{
  storage = cvCreateMemStorage(0);
  cvlines = 0;
}

OpenCvHough::~OpenCvHough()
{
  // do we need to free memory here?    
}

void OpenCvHough::Houghlines(const ImgBgr& img,
                       float sigma,
                       int threshold,
                       int min_len,
                       int gap,
                       std::vector<LineFitting::Line> *lines)
{
  int i;
  ImgGray gimg;
  ImgBinary edges;
  Convert(img, &gimg);
  Canny(gimg, &edges, sigma);
  Convert(edges, &gimg);
  ImgIplImage src(gimg);
  //cvCanny(src, src, 20, 40, 1 );
  cvlines = cvHoughLines2(src, storage, CV_HOUGH_PROBABILISTIC, 1, CV_PI/180, threshold, min_len, gap);
  lines->clear();
  for( i = 0; i < cvlines->total; i++ )
  {
    CvPoint* cvline = (CvPoint*)cvGetSeqElem(cvlines,i);
    LineFitting::Line line;
    line.p1.x = cvline[0].x; 
    line.p1.y = cvline[0].y; 
    line.p2.x = cvline[1].x; 
    line.p2.y = cvline[1].y;
    double y_d = (double)(cvline[1].y - cvline[0].y);
    double x_d = (double)(cvline[1].x - cvline[0].x);
    double angle = atan(x_d/y_d);
    line.theta = static_cast<float>( (x_d == 0.0) ? 0.0f : angle*180/3.14159f );
    if(angle < 0)
      line.rho = static_cast<float>( (double)cvline[0].x * cos(-angle) + (double)cvline[0].y * sin(-angle) );
    else
      line.rho = static_cast<float>( (double)cvline[0].y * sin(angle) - (double)cvline[0].x * cos(angle) );

    lines->push_back(line);
  }
}

void OpenCvHough::Houghlines(const ImgBgr& img, std::vector<LineFitting::Line> *lines)
{
  int i;
   float sigma = 1.00;
  ImgGray gimg;
  ImgBinary edges;
  Convert(img, &gimg);
  Canny(gimg, &edges, sigma);
  Convert(edges, &gimg);
  ImgIplImage src(gimg);
  //cvCanny(src, src, 20, 120, 3 );
  cvlines = cvHoughLines2(src, storage, CV_HOUGH_PROBABILISTIC, 1, CV_PI/180, 40, 30, 5);
  lines->clear();
  for( i = 0; i < cvlines->total; i++ )
  {
    CvPoint* cvline = (CvPoint*)cvGetSeqElem(cvlines,i);
    LineFitting::Line line;
    line.p1.x = cvline[0].x; 
    line.p1.y = cvline[0].y; 
    line.p2.x = cvline[1].x; 
    line.p2.y = cvline[1].y;
    double y_d = (double)(cvline[1].y - cvline[0].y);
    double x_d = (double)(cvline[1].x - cvline[0].x);
    double angle = atan(x_d/y_d);
    line.theta = static_cast<float>( (x_d == 0.0) ? 0.0f : angle*180/3.14159f );
    if(angle < 0)
      line.rho = static_cast<float>( (double)cvline[0].x * cos(-angle) + (double)cvline[0].y * sin(-angle) );
    else
      line.rho = static_cast<float>( (double)cvline[0].y * sin(angle) - (double)cvline[0].x * cos(angle) );

    lines->push_back(line);
  }
}

void OpenCvHough::Houghlines(const ImgGray& img, std::vector<LineFitting::Line> *lines)
{
  int i;
  float sigma = 1.0;
  ImgGray gimg;
  ImgBinary edges;
  Canny(img, &edges, sigma);
  Convert(edges, &gimg);
  ImgIplImage src(gimg);
  cvlines = cvHoughLines2(src, storage, CV_HOUGH_PROBABILISTIC, 1, CV_PI/180, 30, 5, 20 );
  lines->clear();
  for( i = 0; i < cvlines->total; i++ )
  {
    CvPoint* cvline = (CvPoint*)cvGetSeqElem(cvlines,i);
    LineFitting::Line line;
    line.p1.x = cvline[0].x; 
    line.p1.y = cvline[0].y; 
    line.p2.x = cvline[1].x; 
    line.p2.y = cvline[1].y; 
    double y_d = (double)(cvline[1].y - cvline[0].y);
    double x_d = (double)(cvline[1].x - cvline[0].x);
    line.theta = static_cast<float>( (x_d == 0.0) ? 0.0f : atan(x_d/y_d)*180/3.14159f );
    line.rho = static_cast<float>( abs((double)cvline[0].x * cos(line.theta*3.14159f/180) + 
                                       (double)cvline[0].y * sin(line.theta*3.14159f/180)) );
    lines->push_back(line);    
  }
}


};  // end namespace blepo

