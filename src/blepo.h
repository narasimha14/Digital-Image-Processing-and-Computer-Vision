/* 
 * Copyright (c) 2004,2005,2006 Clemson University.
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

/*
      Blepo Version 0.6.9   (18 Aug 2013)
*/

#ifndef __BLEPO_H__
#define __BLEPO_H__

// This file includes all the top-level header files for the library.
// Someone using the library should only have to include this header file.
// For developers, whenever a new top-level header file is written please 
// be sure to add it here.  By top-level I mean a header file containing 
// classes, structures, functions, etc., intended for a user of the library.

// If you are using Visual C++ 6.0, then uncomment this line (or insert the constant into Project -> Settings -> C/C++ -> Preprocessor definitions)
// #define BLEPO_I_AM_USING_VISUAL_CPP_60__

// If you want to use the Kinect sensor, then uncomment this line.  Must be using Windows 7 or later, and VS2010 or later
// #define BLEPO_I_WANT_TO_USE_KINECT

// include OpenCV headers
#ifdef BLEPO_I_AM_USING_VISUAL_CPP_60__
#include "../external/OpenCV-1/cv.h"  // OpenCV 
#include "../external/OpenCV-1/highgui.h"
#else
#include "../external/OpenCV-2/cv.h"
#include "../external/OpenCV-2/highgui.h"
#endif // BLEPO_I_AM_USING_VISUAL_CPP_60__

// blepo includes
#include "Figure/Figure.h"
#include "Figure/FigureGlut.h"
#include "Image/Image.h"
#include "Image/ImageAlgorithms.h"
#include "Image/ImageOperations.h"
//#include "Image/ImgIplImage.h"
#include "Utilities/Array.h"
#include "Utilities/Exception.h"
#include "Utilities/Math.h"
#include "Utilities/PointSizeRect.h"
#include "Utilities/Utilities.h"
//#include "Utilities/PerformanceTimer.h"
#include "Matrix/Matrix.h"
#include "Matrix/MatrixOperations.h"
#include "Matrix/LinearAlgebra.h"

#include "Image/Surf.h"
#endif //__BLEPO_H__
