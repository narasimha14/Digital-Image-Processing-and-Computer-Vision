/* 
 * Copyright (c) 2005 Stan Birchfield.
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

#ifndef __BLEPO_ELLIPTICALHEADTRACKER_H__
#define __BLEPO_ELLIPTICALHEADTRACKER_H__

#pragma warning( disable: 4786 )

#include "Image.h"
#include "Utilities/PointSizeRect.h"
#include <vector>

/**
@class EllipticalHeadTracker

Implements the algorithm in "Elliptical Head Tracking by Intensity Gradients and Color Histograms", CVPR 1998

@author Stan Birchfield
*/

namespace blepo
{

class EllipticalHeadTracker
{
public:
  // the state of the ellipse
  struct EllipseState
  {
    EllipseState() {}
    EllipseState(int xx, int yy, int size) : x(xx), y(yy), sz(size) {}
    int x;         // x location
    int y;         // y location
    int sz;        // size (the width of the ellipse is 2*sz+1; the height is this number times the fixed aspect ratio)
  };

  EllipticalHeadTracker();
  virtual ~EllipticalHeadTracker();

  void SetState(const EllipseState& state) 
  { 
    m_state = state; 
    m_xvel = 0;
    m_yvel = 0;
  }
  void BuildModel(const ImgBgr& img, const EllipseState& state);
  void SaveModel(const char* filename);
  void LoadModel(const char* filename);

  // You should call Init (either version) at least once before calling Track().
  // 'filename':  name of file containing model histogram
  void Init(const ImgBgr& img, const EllipseState& initial_state);
  void Init(const EllipseState& initial_state, const char* filename);

  // Tracks the target from the last known position, using constant velocity prediction.
  // 'img' should be the same size each time you call this
  // 'use_gradient':  0 means do not consider gradient at all
  //                  1 means use gradient dot product (default)
  //                  2 means use gradient magnitude
  // 'use_color':     0 means do not use color histogram at all
  //                  1 means use color histogram (default)
  // Of course, do not set both of these to 0, or you'll get garbage.
  EllipseState Track(const ImgBgr& img, int use_gradient = 1, int use_color_histogram = 1);

  void OverlayEllipse(const EllipseState& state, ImgBgr* img);

  int GetMinSize() const;
  int GetMaxSize() const;
  // returns half the search range (i.e., 4 means +/- 4, 1 means +/- 1, etc.)
  void GetSearchRange(int* x, int* y, int* size);

//  // returns the width and height necessary for the image
//  // (these are determined at compile time; sorry, but this is old code)
//  int GetWidth();
//  int GetHeight();

private:
  int m_xvel, m_yvel;  // velocities (for prediction)
  EllipseState m_state;
//  typedef std::vector<float> Histogram;
//  typedef std::vector<Point> Perimeter;
//  State m_last_state;
//  Histogram m_histogram_model;
//  std::vector<ImgBinary> m_masks;
//  std::vector<Perimeter> m_perimeters;
};


};  // end namespace blepo

#endif //__BLEPO_ELLIPTICALHEADTRACKER_H__
