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

#ifndef __BLEPO_FIGURE_H__
#define __BLEPO_FIGURE_H__

#include <afxwin.h>
#include <afxcmn.h>  // CStatusBarCtrl
#include "Image/Image.h"
#include "Utilities/Mutex.h"  // Thread
#include "Utilities/Array.h"
//#include <vector>

/**
@class Figure
Class for displaying image and other data.

@author Stan Birchfield (STB)
*/

namespace blepo
{

class Figure
{
public:
  enum WhichButton { MC_NONE = 0, MC_LEFT = 1, MC_RIGHT = 2 };
  struct MouseClick 
  { 
    MouseClick( WhichButton b, const CPoint& p) : button(b), pt(p) {}
    WhichButton button;
    CPoint pt;
  };

public:
  Figure(const CString& title = L"", int x=-1, int y=-1, bool permanent = true, bool disable_close_button = false);
  virtual ~Figure();
	void DisableCloseButton(bool disable = true);
  // resizes the window only if image size is different from before
  void Draw(const ImgBgr&    img);
  void Draw(const ImgBinary& img);
  void Draw(const ImgFloat&  img);
  void Draw(const ImgGray&   img);
  void Draw(const ImgInt&    img);

  // draws rect on current image
  void DrawRect(const Rect& rect, const Bgr& color);

  // grab a certain number of mouse clicks (any button)
  // if 'point_color' is valid (i,e., <= 0xFFFFFF), then 3x3 colored dots are displayed at the click locations
  // if 'line_color' is valid (i,e., <= 0xFFFFFF), then a 1-pixel-thick polyline is drawn
  void GrabMouseClicks(int n_mouse_clicks, Array<MouseClick>* points, COLORREF point_color = -1, COLORREF line_color = -1);

  // grab a consecutive number of left mouse clicks.
  // Terminate by clicking the right mouse button
  // if 'point_color' is valid (i,e., <= 0xFFFFFF), then 3x3 colored dots are displayed at the click locations
  // if 'line_color' is valid (i,e., <= 0xFFFFFF), then a 1-pixel-thick polyline is drawn
  void GrabLeftMouseClicks(Array<MouseClick>* points, COLORREF point_color = -1, COLORREF line_color = -1);

  // if button is not NULL, then sets it to the button that was pressed
  CPoint GrabMouseClick(WhichButton* button = NULL);

  CRect GrabRect();

  // Test whether mouse was clicked without waiting.  
  // Returns the button pressed (or MC_NONE if no button pressed) 
  // Sets 'pt' to the coordinates of the click (or the current mouse coordinates).
  WhichButton TestMouseClick(CPoint* pt = NULL);

  void GetDisplay(ImgBgr* out);

  // Sets window position in screen coordinates
  void SetWindowPosition(int x, int y);

  // Sets window size in screen coordinates
  void SetWindowSize(int width, int height);

  // Sets window position and size in screen coordinates
  void SetWindowRect(const CRect& rect);

  // Gets window position in screen coordinates
  CPoint GetWindowPosition() const;

  // Gets window size in screen coordinates
  CSize GetWindowSize() const;

  // Gets window position and size in screen coordinates
  CRect GetWindowRect() const;

  // Place this figure immediately to the right of an existing figure.
  void PlaceToTheRightOf(const Figure& other);

  void SetTitle(const CString& str);

  void SetVisible(bool visible);

  bool IsClosed() const;

  // Link two figures together.  Mousemove in one figure causes mousemove in other figure.
  // Bug:  Only works correctly if both figures are the same size and both images are the
  // same size.
  void Link(Figure* other, bool symmetric = true);

  // Cause window to respond to messages in the Windows queue
  void ProcessWindowsMessage();

private:
  class FigureWnd* m_wnd;
  bool m_permanent;
};

// Enters a Windows event loop.
// Returns only after a key is typed in the console window.  
void EventLoop();

};  // end namespace blepo

#endif //__BLEPO_FIGURE_H__
