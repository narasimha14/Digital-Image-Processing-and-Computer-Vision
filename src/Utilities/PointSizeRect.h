/* 
 * Copyright (c) 2005 Clemson University.
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

#ifndef __BLEPO_POINTSIZERECT_H__
#define __BLEPO_POINTSIZERECT_H__

#include <afxwin.h>  // CPoint, CRect, CSize
#include "Math.h"  // Round()

/**

@author Stan Birchfield (STB)
*/

namespace blepo
{

  typedef CPoint Point;
  typedef CRect Rect;
  typedef CSize Size;

  inline int Area(const Rect& rect) { return rect.Width() * rect.Height(); }

  struct Point2d
  {
    Point2d() {}
    Point2d(double xx, double yy) : x(xx), y(yy) {}
    double x, y;

    Point GetPoint() const { return Point( blepo_ex::Round(x), blepo_ex::Round(y) ); }
  };

  struct Point2f 
  { 
    Point2f() {}
    Point2f(float xx, float yy) : x(xx), y(yy) {}
    float x, y; 

    Point GetPoint() const { return Point( blepo_ex::Round(x), blepo_ex::Round(y) ); }
  };

//class Point
//{
//public:
//  int x, y;
//
//  Point() {}
//  Point(int xx, int yy) : x(xx), y(yy) {}
//  Point operator+(const Point& other) { return Point(x+other.x, y+other.y); }
//  void operator+=(const Point& other) { x+=other.x;  y+=other.y; }
//  bool operator==(const Point& other) { return x==other.x && y==other.y; }
//  bool operator!=(const Point& other) { return x!=other.x || y!=other.y; }
//};
//
//class Size
//{
//public:
//  int cx, cy;
//
//  Size() {}
//  Size(int xx, int yy) : cx(xx), cy(yy) {}
//};
//
//class Rect
//{
//public:
//  int left, top, right, bottom;
//
//  Rect() {}
//  Rect(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}
//};
//
//typedef Point PtInt;
//typedef Rect RectInt;

};  // end namespace blepo

#endif //__BLEPO_POINTSIZERECT_H__
