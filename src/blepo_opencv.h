/* 
 * Copyright (c) 2004-2011 Clemson University.
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

#ifndef __BLEPO_OPENCV_H__
#define __BLEPO_OPENCV_H__

// include OpenCV headers
#ifdef BLEPO_I_AM_USING_VISUAL_CPP_60__
#include "../external/OpenCV-1/cv.h"  // OpenCV 
#include "../external/OpenCV-1/highgui.h"
#else
#include "../external/OpenCV-2/cv.h"
#include "../external/OpenCV-2/highgui.h"
#endif // BLEPO_I_AM_USING_VISUAL_CPP_60__

#endif //__BLEPO_OPENCV_H__
