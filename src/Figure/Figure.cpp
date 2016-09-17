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

// Status bar (CStatusBarCtrl) code comes from http://www.codeguru.com/cpp/w-d/dislog/toolbarsandstatusbars/article.php/c1955/

#include "Figure.h"
#include "Image/ImageOperations.h"
#include "Utilities/Array.h"
#include "Utilities/Math.h"
#include <conio.h>  // kbhit
#include <algorithm>  // std::find
#include <afxdlgs.h>  // CFileDialog

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

#pragma warning(disable: 4355)  // this used in base member initializer list

// These numbers are somewhat arbitrary, as long as they are different
// from each other
#define ID_FIGURE_FILE_OPEN    5670
//#define ID_FIGURE_FILE_SAVE    5671
#define ID_FIGURE_FILE_SAVEAS  5672
#define ID_FIGURE_EDIT_COPY    5673
#define ID_FIGURE_EDIT_ORIGSIZE    5674
#define IDC_FIGURE_STATUSBAR   5675

// ================> begin local functions (available only to this translation unit)
namespace
{
using namespace blepo;

// Draws a line on a CDC
void iDrawLine(CDC& dc, const Point& pt1, const Point& pt2, const Bgr& color, int thickness)
{
  CPen pen(PS_SOLID, thickness, color.ToInt(Bgr::BLEPO_BGR_XBGR));
  CPen* old = dc.SelectObject(&pen);
  dc.MoveTo(pt1);
  dc.LineTo(pt2);
  dc.SelectObject(old);
}

// Draws a dot on a CDC
void iDrawDot(CDC& dc, const Point& pt, const Bgr& color, int size)
{
  int r = size / 2;
  dc.FillSolidRect(pt.x-r, pt.y-r, size, size, color.ToInt(Bgr::BLEPO_BGR_XBGR));
}

void iDrawCrosshairs(CDC& dc, const CPoint& pt, int w, int h)
{
  iDrawLine(dc, Point(0, pt.y), Point(w, pt.y), Bgr(255,255,255), 1);
  iDrawLine(dc, Point(pt.x, 0), Point(pt.x, h), Bgr(255,255,255), 1);
}

// Converts a window coordinate to an image coordinate; useful for getting
// the coordinates of the image pixel to which the mouse points when the image
// display is scaled.  The function also works for the y direction by substituting
// height for width.  The algorithm was discovered empirically by examining the 
// pixel values on the screen using CDC::GetPixel.  With a 5x5 test image, it
// was discovered that the first window x-coordinate for each pixel could be
// computed in Matlab using round((wcw/5))*(0:4), where 'wcw' is the window 
// client width.
static int iWnd2Img(int win_x, int win_width, int img_width)
{
  for (int i=img_width-1 ; i>=0 ; i--)
  {
    int thresh = blepo_ex::Round((double) i * win_width / img_width);
    if (win_x >= thresh)  return i;
  }
  return 0;
}

//typedef bool(*ExitTest)();
Array<HWND>  g_hwnd_list;

// Tell Windows to handle the next message in the queue for this thread.
// Returns false if the message is WM_QUIT, WM_DESTROY, or WM_CLOSE, or if
// a key has been hit in the console window.  Returns true otherwise.
bool iProcessWindowsMessage()
{
  MSG msg;

  // test if there is a message in queue, if so get it
  if (PeekMessage(&msg,NULL,0,0,PM_NOREMOVE))
  { 
//    printf("%x\n", msg.message);
//    {
//      TRACE("qee %x: ", msg.hwnd);
//      for (int i=0 ; i<g_hwnd_list.Len() ; i++)
//      {
//        TRACE("%x ", g_hwnd_list[i]);
//      }
//      TRACE("\n");
//    }

    // The purpose of this commented out code was to 
    //   only process messages corresponding to FigureWnd windows or their children.
    //   (Should probably be a recursive test (i.e., grandchildren, etc.), but this
    //   is not necessary right now or any time in the forseeable future.)
    // However, we found that in some scenarios there would be a message on the queue
    //   that did not pass this 'one_of_us' test below, and therefore it would not
    //   be handled.  Because it was not removed from the queue, it would prevent any
    //   other message from being handled, too.  One such scenario was real-time 
    //   capture from a DShow camera with the face detector running (in Demo).
    // One day, perhaps, we should look into it.  For now, I'm just reverting the 
    //   code back to processing all windows messages.  The only slight danger is
    //   that the user can quit the application by hitting a button on the main dlg,
    //   even when the figure is stuck waiting for input.
//    bool one_of_us = false;
//    {
//      HWND* pp = g_hwnd_list.Begin();
//      while (pp != g_hwnd_list.End())
//      {
//        if (::IsChild(*pp++, msg.hwnd))  one_of_us = true;
//      }
//      if (std::find(g_hwnd_list.Begin(), g_hwnd_list.End(), msg.hwnd) != g_hwnd_list.End())  one_of_us = true;
//    }

//    {
//      HWND a = msg.hwnd;
//      HWND b = *g_hwnd_list.Begin();
//      int n = g_hwnd_list.Len();
//      char str[100];
//      GetWindowText(a, str, 100);
//      TRACE("a %d '%s' oou:%d [%d]: b %d\n", a, str, one_of_us, n, b);
//    }

    if (1)    // old:  if (one_of_us)
    {
      // remove from the queue
      PeekMessage(&msg,NULL,0,0,PM_REMOVE);

      // test if this is a quit
      if (msg.message == WM_QUIT)
        return false;
      if (msg.message == WM_DESTROY)
        return false;
      if (msg.message == WM_CLOSE)
        return false;

      // translate any accelerator keys
      TranslateMessage(&msg);

      // send the message to the window proc
      DispatchMessage(&msg);
    }
  } // end if

  // exit loop if key is hit in console window -- why would we want this behavior?
//  if (_kbhit())  
//  {
//    _getch();  // throw away character
//    return false;
//  }
  // main processing goes here
  // My_Main(); // or whatever your loop is called
  return true;
}

};
// ================< end local functions

namespace blepo
{

void EventLoop()
{
  while (iProcessWindowsMessage());
}

//void EventLoop()  { EventLoop(NULL); }


//////////////////////////////////////////////////////////////////
// FigureWnd
//
// A simple internal class for displaying images.

class FigureWnd : public CWnd
{
public:
  FigureWnd(const CString& title = L"Figure", int x=-1, int y=-1);
  ~FigureWnd();

  void Draw(const ImgBgr& img);
  void Draw(const ImgBinary& img);
  void Draw(const ImgFloat& img);
  void Draw(const ImgGray& img);
  void Draw(const ImgInt& img);

  // draws rect on current image
  void DrawRect(const Rect& rect, const Bgr& color);

  void GrabMouseClicks(int n_mouse_clicks, Array<Figure::MouseClick>* points, COLORREF point_color, COLORREF line_color);
  void GrabLeftMouseClicks(Array<Figure::MouseClick>* points, COLORREF point_color, COLORREF line_color);
  CRect GrabRect();

  void Link(FigureWnd* other, bool symmetric);

  // returns which button was pressed (0 if none); if 'pt' is not NULL,
  // and if button was pressed, then fills it in with the image coordinates of the mouse click.
  // Unlike GrabMouseClick, this function returns immediately.
  Figure::WhichButton TestMouseClick(CPoint* pt = NULL);

  // returns the pixels being displayed in the figure
  void GetDisplay(ImgBgr* out) { *out = m_img_bgr0; }

  bool IsMouseInsideWindow() { return m_mouse_is_inside_window; }

  bool IsClosed() const { return m_hWnd == NULL; }

  CPoint ImageCoordFromClient(const CPoint& pt);

  // Generated message map functions
	//{{AFX_MSG(FigureWnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnFileOpen();
//	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnEditCopy();
	afx_msg void OnEditOrigSize();
//	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
  void ResizeClientArea(int width, int height);
  void Refresh(CDC& dc);
  void Refresh(bool paint_dc = false);
  void RefreshStatusPane();
  CSize GetCurrentImageSize();
  void MyGetClientRect(CRect* r);
  BOOL CopyRect2Clipboard();

private:
  static void InitializeGlobal();
  static CString g_class_name;
  static int g_left, g_top;
  static int g_num;
  static CFont g_font;
  static int g_statusbar_height;
  CStatusBarCtrl m_statusbar;
  int m_which;  ///< which image is valid, or -1 if none
  /// Note:  We have to store a local copy of the actual image so that we can
  /// display the actual pixel values in response to mouse move events.  But
  /// to make the display fast, we first convert the image to BGR (if it is
  /// not already); then we can just display the BGR image whenever a refresh
  /// is needed.  The loss in speed that comes from not doing this is noticeable
  /// on mouse move events.
  ImgBgr m_img_bgr0;  ///< always stores the image being displayed
  ImgBinary m_img_binary1;
  ImgFloat m_img_float2;
  ImgGray m_img_gray3;
  ImgInt m_img_int4;
  CString m_title;
  bool m_equal_mode;
  bool m_display_coords_on_bottom;
  bool m_mouse_is_inside_window;
  Array<FigureWnd*> m_linked_figs;  // other figures linked to this one (mouse moves affect all simultaneously)

  class MouseGrabber
  {
  public:
    enum Mode { MG_NONE, MG_GRAB_NUM_CLICKS, MG_GRAB_RECT, MG_GRAB_LEFT_CLICKS };

    MouseGrabber(FigureWnd* fw) 
      : m_figwnd(fw), m_mode(MG_NONE), m_done(false), 
        m_n_mouse_clicks_to_grab(0), m_mouse_clicks(), m_last_mouse_pt(0,0), m_last_mouse_pt_image_coords(0,0),
        m_point_color(-1), m_line_color(-1) 
    {}

    const Array<Figure::MouseClick>* GetMouseClickList() const { return &m_mouse_clicks; }

    CPoint GetLastMousePoint(bool image_coords) const { return image_coords ? m_last_mouse_pt_image_coords : m_last_mouse_pt; }

    // returns true if operation has completed, false otherwise
    bool IsDone() const { return m_done; }

    COLORREF GetPointColor() const { return m_point_color; }
    COLORREF GetLineColor() const { return m_line_color; }

    void EnterGrabNumClicksMode(int nclicks, COLORREF point_color, COLORREF line_color)
    {
      m_mode = MG_GRAB_NUM_CLICKS;
      m_done = false;
      m_mouse_clicks.Reset();
      m_n_mouse_clicks_to_grab = nclicks;
      m_point_color = point_color;
      m_line_color = line_color;
    }

    void EnterGrabRectMode()
    {
      m_mode = MG_GRAB_RECT;
      m_done = false;
      m_mouse_clicks.Reset();
      m_point_color = m_line_color = -1;
    }

    void EnterGrabLeftClicksMode(COLORREF point_color, COLORREF line_color)
    {
      m_mode = MG_GRAB_LEFT_CLICKS;
      m_done = false;
      m_mouse_clicks.Reset();
      m_point_color = point_color;
      m_line_color = line_color;
    }

    void ResetMode()
    {
      m_mode = MG_NONE;
      m_done = false;
      m_mouse_clicks.Reset();
      m_point_color = m_line_color = -1;
    }

    Mode  GetMode() { return m_mode; }
//    bool NeedToRedraw() { return m_mode != MG_NONE; }

    void LButtonDown(const CPoint& point)
    {
      Figure::MouseClick mc( Figure::MC_LEFT, m_figwnd->ImageCoordFromClient(point) );
      switch (m_mode)
      {
      case MG_GRAB_NUM_CLICKS:
        if (m_n_mouse_clicks_to_grab > 0)  
        {
          m_mouse_clicks.Push( mc );
          m_n_mouse_clicks_to_grab--;
          if (m_n_mouse_clicks_to_grab == 0)  m_done = true;
        }
        break;
      case MG_GRAB_RECT:
        if (m_mouse_clicks.Len() < 1)  m_mouse_clicks.Push( mc );
        break;
      case MG_GRAB_LEFT_CLICKS:
        m_mouse_clicks.Push( mc );
        break;
      case MG_NONE:
        if (m_mouse_clicks.Len() == 0)  m_mouse_clicks.Push( mc );  // store click in case TestMouseClick() is called
        break;
      }
    }

    void LButtonUp(const CPoint& point)
    {
      Figure::MouseClick mc( Figure::MC_LEFT, m_figwnd->ImageCoordFromClient(point) );
      switch (m_mode)
      {
      case MG_GRAB_NUM_CLICKS:
        break;
      case MG_GRAB_RECT:
        if (m_mouse_clicks.Len()>0)
        {
          m_mouse_clicks.Push( mc );
          m_done = true;
        }
        break;
      case MG_GRAB_LEFT_CLICKS:
        break;
      }
    }

    void RButtonDown(const CPoint& point)
    {
      Figure::MouseClick mc( Figure::MC_RIGHT, m_figwnd->ImageCoordFromClient(point) );
      switch (m_mode)
      {
      case MG_GRAB_NUM_CLICKS:
        if (m_n_mouse_clicks_to_grab > 0)  
        {
          m_mouse_clicks.Push( mc );
          m_n_mouse_clicks_to_grab--;
          if (m_n_mouse_clicks_to_grab == 0)  m_done = true;
        }
        break;
      case MG_GRAB_RECT:
        break;
      case MG_GRAB_LEFT_CLICKS:
        m_done = true;
        break;
      case MG_NONE:
        if (m_mouse_clicks.Len() == 0)  m_mouse_clicks.Push( mc );  // store click in case TestMouseClick() is called
        break;
      }
    }

    void MouseMove(const CPoint& point)
    {
      m_last_mouse_pt = point;
      m_last_mouse_pt_image_coords = m_figwnd->ImageCoordFromClient(point);  // convert from client to image coordinates
    }

  private:
    Mode m_mode;
    int m_n_mouse_clicks_to_grab;
    Array<Figure::MouseClick> m_mouse_clicks;
    CPoint m_last_mouse_pt, m_last_mouse_pt_image_coords;
    FigureWnd* m_figwnd;
    bool m_done;
    COLORREF m_point_color, m_line_color;  // -1 means do not draw
  };

  MouseGrabber m_mouse_grabber;
};


//////////////////////////////////////////////////////////////////
// FigureWnd message map

BEGIN_MESSAGE_MAP(FigureWnd, CWnd)
	//{{AFX_MSG_MAP(FigureWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSELEAVE,OnMouseLeave)
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_COMMAND(ID_FIGURE_FILE_OPEN, OnFileOpen)
//	ON_COMMAND(ID_FIGURE_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FIGURE_FILE_SAVEAS, OnFileSaveAs)
	ON_COMMAND(ID_FIGURE_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_FIGURE_EDIT_ORIGSIZE, OnEditOrigSize)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////
// FigureWnd

CString FigureWnd::g_class_name;
int FigureWnd::g_left = 0;
int FigureWnd::g_top = 0;
int FigureWnd::g_num = 1;
int FigureWnd::g_statusbar_height = 20;
CFont FigureWnd::g_font;
//const int FigureWnd::m_mouse_clicks_queue_size = 20;


//////////////////////////////////////////////////////////////////
// FigureWnd methods

void FigureWnd::InitializeGlobal()
{
  if (g_class_name.GetLength()==0)
  {
    g_class_name = AfxRegisterWndClass(
            CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
            ::LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)),
            (HBRUSH) GetStockObject(BLACK_BRUSH), 
            0);
    g_font.CreatePointFont(100, L"Courier New");
  }
}

int FigureWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	

  { // Add main menu
    CMenu menu, file_popup, edit_popup;
    BOOL ret;

    // create main menu
    ret = menu.CreateMenu();  assert(ret);
    
    // create and add file menu
    ret = file_popup.CreatePopupMenu();  assert(ret);
    file_popup.AppendMenu(MF_STRING, ID_FIGURE_FILE_OPEN, L"&Open...");
    file_popup.AppendMenu(MF_STRING, ID_FIGURE_FILE_SAVEAS, L"&Save As...");
    menu.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR) file_popup.m_hMenu, L"&File");

    // create and add edit menu
    ret = edit_popup.CreatePopupMenu();  assert(ret);
    edit_popup.AppendMenu(MF_STRING, ID_FIGURE_EDIT_COPY, L"&Copy");
    edit_popup.AppendMenu(MF_STRING, ID_FIGURE_EDIT_ORIGSIZE, L"&Original Size");
    menu.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR) edit_popup.m_hMenu, L"&Edit");

    ret = SetMenu(&menu);  assert(ret);

    {
      HMENU retm;
      retm = menu.Detach();  assert(retm); // so the CMenu destructor won't destroy the menu
      retm = file_popup.Detach();  assert(retm);
      retm = edit_popup.Detach();  assert(retm);
    }
  }
  
  // status bar control
  int m_Widths[4];
	int rets = m_statusbar.Create(WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP, 
                CRect(0,0,0,0), 
                this,
        				IDC_FIGURE_STATUSBAR);
  assert(rets);
  
  // status bar panes
	m_Widths[0] = 70; // (x,y) coordinates
 	m_Widths[1] = 150; // pixel value 
 	m_Widths[2] = 190; // image type
	m_statusbar.SetParts( 3, m_Widths);
  {
    // calculate height of status bar ctrl
    CRect r;
    m_statusbar.GetWindowRect(&r);
    g_statusbar_height = r.Height();
  }

	return 0;
}

void FigureWnd::MyGetClientRect(CRect* r)
{
  GetClientRect(r);
  r->bottom -= g_statusbar_height;
}

void FigureWnd::OnFileOpen()
{
  try 
  {
    CFileDialog dlg(TRUE, // open file dialog
                    NULL, // default extension
                    NULL,  // default filename
                    OFN_HIDEREADONLY,  // flags
                    L"All image files|*.pgm;*ppm;*.bmp;*.jpg;*.jpeg|PGM/PPM files (*.pgm)|*.pgm;*.ppm|BMP files (*.bmp)|*.bmp|JPEG files (*.jpg,.jpeg)|*.jpg;*.jpeg|All files (*.*)|*.*||",  // filter
                    NULL);
//    char dir[1000];
//    _getcwd(dir, 1000);
//    CString foo = CString(dir) + "\\..\\images";
//	  dlg.m_ofn.lpstrInitialDir = (const char*) foo;
	  dlg.m_ofn.lpstrTitle = L"Load image";
    if (dlg.DoModal() == IDOK)
    {
      CString fname = dlg.GetPathName();
      ImgBgr img;
      Load(fname, &img);
      m_which = 0;
      ResizeClientArea(img.Width(), img.Height());
      Draw(img);
    }

  } catch (const Exception& e)
  {
    // image failed to load, so notify user
    e.Display();
  }
}

// from "Capture Screen to Clipboard including dropdown menu" by Y. Huang, http://www.codeproject.com/KB/clipboard/hscr2clp.aspx
BOOL FigureWnd::CopyRect2Clipboard()
{
  // Get entire client rect
  CRect rect;
  GetClientRect(rect);
  rect.bottom -= g_statusbar_height;  // subtract status bar at bottom
  rect.NormalizeRect();
  if (rect.IsRectEmpty() || rect.IsRectNull())
    return FALSE;
  
  // Copy bits to a CBitmap
  CClientDC dcScrn(this);
  CDC memDc;
  if (!memDc.CreateCompatibleDC(&dcScrn))
    return FALSE;
  CBitmap bitmap;
  if (!bitmap.CreateCompatibleBitmap(&dcScrn,rect.Width(), rect.Height()))
    return FALSE;
  CBitmap* pOldBitmap = memDc.SelectObject(&bitmap);
  memDc.BitBlt(0, 0, rect.Width(), rect.Height(), &dcScrn,rect.left, rect.top, SRCCOPY);
  
  // Put bitmap on clipboard
  if (OpenClipboard())
  {
    EmptyClipboard();
    SetClipboardData(CF_BITMAP,bitmap. GetSafeHandle());
    CloseClipboard();
  }
  memDc.SelectObject(pOldBitmap);
  return TRUE;
}

void FigureWnd::OnFileSaveAs()
{
  try
  {
    CFileDialog dlg(FALSE, // save file dialog
                    L"eps", // default extension
                    L"blepo_image",  // default filename
                    OFN_HIDEREADONLY,  // flags
                    L"EPS file (*.eps)|*.eps|PPM file (*.ppm)|*.ppm|PGM file (*.pgm)|*.pgm|BMP files (*.bmp)|*.bmp|JPEG files (*.jpg,.jpeg)|*.jpg;*.jpeg|All files (*.*)|*.*||",  // filter
                    NULL);
    dlg.m_ofn.lpstrTitle = L"Save figure";
    if (dlg.DoModal() == IDOK)
    {
      CString fname = dlg.GetPathName();
      // check whether file already exists
      CFileStatus stat;
      if (CFile::GetStatus(fname, stat) == TRUE)
      {
        CString str;
        str.Format(L"File '%s' already exists.  Overwrite?", fname);
        int ret = AfxMessageBox(str, MB_YESNO);
        if (ret == IDNO)  return;
      }
      // get image from display and save to file
      ImgBgr img;
      GetDisplay(&img);
      Save(img, fname);
    }
  } catch (Exception& e)
  {
    e.Display();
  }
}


LRESULT FigureWnd::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
  m_mouse_is_inside_window = false;
  Refresh();
  
	return 0;
}

void FigureWnd::OnEditCopy()
{
  CopyRect2Clipboard();
}

void FigureWnd::OnEditOrigSize()
{
  const ImgBgr& img = m_img_bgr0;
  if (img.IsNull())  return;
  ResizeClientArea(img.Width(), img.Height());
  Refresh();
}

void FigureWnd::OnClose()
{
  CWnd::OnClose();
}



//int FigureWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
//{
//	if (CWnd::OnCreate(lpCreateStruct) == -1)
//		return -1;
//	
//	// TODO: Add your specialized creation code here
//  SetFont(&g_font, FALSE);
//	
//	return 0;
//}

void FigureWnd::Refresh(bool paint_dc)
{
  if (m_hWnd == NULL)  return;
  if (paint_dc)
  {
    CPaintDC dc(this);
    Refresh(dc);
  }
  else
  {
    CClientDC dc(this);
    Refresh(dc);
  }
}

void FigureWnd::Refresh(CDC& dc)
{
  CRect wrect;
  MyGetClientRect(&wrect);
  int w = wrect.Width();
  int h = wrect.Height();
  if (m_which >= 0)
  {
    if (m_equal_mode)
    {
      ImgBinary tmp;
      CPoint pt = m_mouse_grabber.GetLastMousePoint(true);  // m_last_mouse_pt_image_coords;  // old: ImageCoordFromClient(m_last_mouse_pt);
      switch(m_which)
      {
      case 0:  Equal(m_img_bgr0,    m_img_bgr0   (pt.x, pt.y), &tmp);  break;
      case 1:  Equal(m_img_binary1, m_img_binary1(pt.x, pt.y), &tmp);  break;
      case 2:  Equal(m_img_float2,  m_img_float2 (pt.x, pt.y), &tmp);  break;
      case 3:  Equal(m_img_gray3,   m_img_gray3  (pt.x, pt.y), &tmp);  break;
      case 4:  Equal(m_img_int4,    m_img_int4   (pt.x, pt.y), &tmp);  break;
      default:  assert(0);
      }
      blepo::Draw(tmp, dc, Rect(0, 0, m_img_bgr0.Width(), m_img_bgr0.Height()), Rect(0, 0, w, h));
    }
    else
    {
      blepo::Draw(m_img_bgr0, dc, Rect(0, 0, m_img_bgr0.Width(), m_img_bgr0.Height()), Rect(0, 0, w, h));
    }
  }  

  switch (m_mouse_grabber.GetMode())
  {
  case MouseGrabber::MG_GRAB_NUM_CLICKS:
  case MouseGrabber::MG_GRAB_LEFT_CLICKS:
//  if (m_n_mouse_clicks_to_grab > 0)
    {
      // draw crosshairs
      const CPoint& pt = m_mouse_grabber.GetLastMousePoint(false);
      iDrawCrosshairs(dc, pt, w, h);
//      iDrawLine(dc, Point(0, pt.y), Point(w, pt.y), Bgr(255,255,255), 1);
//      iDrawLine(dc, Point(pt.x, 0), Point(pt.x, h), Bgr(255,255,255), 1);

      // draw points and polylines
      COLORREF pcol = m_mouse_grabber.GetPointColor();
      COLORREF lcol = m_mouse_grabber.GetLineColor();
      if (lcol <= 0x00FFFFFF)
      {
        const Array<Figure::MouseClick>* mcl = m_mouse_grabber.GetMouseClickList();
        const int n = mcl->Len();
        int i;
        for (i=0 ; i<n-1 ; i++)
        {
          const Point& p1 = (*mcl)[i].pt;
          const Point& p2 = (*mcl)[i+1].pt;
          iDrawLine(dc, p1, p2, Bgr(lcol, Bgr::BLEPO_BGR_XBGR), 1);
        }
        if (i<n)  iDrawLine(dc, (*mcl)[i].pt, pt, Bgr(lcol, Bgr::BLEPO_BGR_XBGR), 1);
      }
      if (pcol <= 0x00FFFFFF)
      {
        const Array<Figure::MouseClick>* mcl = m_mouse_grabber.GetMouseClickList();
        const int n = mcl->Len();
        for (int i=0 ; i<n ; i++)
        {
          const Point& p = (*mcl)[i].pt;
          iDrawDot(dc, p, Bgr(pcol, Bgr::BLEPO_BGR_XBGR), 3);
        }
      }
    }
    break;
  case MouseGrabber::MG_GRAB_RECT:
    {
      const Array<Figure::MouseClick>* mcl = m_mouse_grabber.GetMouseClickList();
      Point lmp = m_mouse_grabber.GetLastMousePoint(false);
      if (mcl->Len()>0)
      {
        const Point& pt = (*mcl)[0].pt;
        Rect rect(pt.x, pt.y, lmp.x, lmp.y);
    //    TRACE(" %d %d %d %d\n", rect.left, rect.top, rect.right, rect.bottom);
        CBrush brush;
        brush.CreateSolidBrush(RGB(255,0,0));
        dc.FrameRect(rect, &brush);
      }
      else 
      {
        // draw crosshairs
        iDrawCrosshairs(dc, lmp, w, h);
//        iDrawLine(dc, Point(0, lmp.y), Point(w, lmp.y), Bgr(255,255,255), 1);
//        iDrawLine(dc, Point(lmp.x, 0), Point(lmp.x, h), Bgr(255,255,255), 1);
      }
    }
    break;
  }

//  if (!m_mouse_is_inside_window)
//  {
//    // clear first two panes
//    CString msg;
//    m_statusbar.SetText(msg, 0, 0);
//    m_statusbar.SetText(msg, 1, 0);
//  }

  RefreshStatusPane();

  // process any Windows messages associated with figures that happen to be in the 
  // queue.  If the user is displaying a sequence of images in a loop, then this
  // line of code enables the window's children (i.e., status bar) to redraw themselves
  // without having to wait till the end.
  iProcessWindowsMessage();
}

void FigureWnd::RefreshStatusPane()
{
  CString msg0, msg1, msg2;
  CPoint pt = m_mouse_grabber.GetLastMousePoint(true); //m_last_mouse_pt_image_coords;  // old:ImageCoordFromClient(m_last_mouse_pt);
  int x = pt.x, y = pt.y;

  // display mouse coords only if 
  //   - mouse is inside window, or
  //   - mouse is inside one of the windows linked to this one
  bool showme = m_mouse_is_inside_window;
  for (int i=0 ; i<m_linked_figs.Len(); i++)
  {
    showme = showme || m_linked_figs[i]->IsMouseInsideWindow();
  }

  // display text in status window
  if (showme)  msg0.Format(L"(%4d,%4d)", x, y);
  showme = showme && x>=0 && y>=0 
                       && x<m_img_bgr0.Width() && y<m_img_bgr0.Height(); //ok b/c bgr0 is always used, no matter m_which
  switch (m_which)
  {
  case 0:
    {
      if (showme)
      {
        ImgBgr::Pixel pix = m_img_bgr0(x, y);
        msg1.Format(L"[%3d,%3d,%3d]", pix.b, pix.g, pix.r);
      }
      msg2 = "Bgr";
    }
    break;
  case 1:
    if (showme)  msg1.Format(L"%3d", (bool) m_img_binary1(x, y));
    msg2 = "Binary";
    break;
  case 2:
    if (showme)  msg1.Format(L"%10f", m_img_float2(x, y));
    msg2 = "Float";
    break;
  case 3:
    if (showme)  msg1.Format(L"%3d", m_img_gray3(x, y));
    msg2 = "Gray";
    break;
  case 4:
    if (showme)  msg1.Format(L"%5d", m_img_int4(x, y));
    msg2 = "Int";
    break;
  default:
    break;
  }
  m_statusbar.SetText(msg0, 0, 0);
  m_statusbar.SetText(msg1, 1, 0);
  m_statusbar.SetText(msg2, 2, 0);
}

void FigureWnd::OnPaint() 
{
	if (IsIconic())
	{
  }
  else
  {
    Refresh(true);
  }
}

/// Given client coordinates (x,y), returns the image coordinates.
/// Takes the current zoom factor into account.
CPoint FigureWnd::ImageCoordFromClient(const CPoint& pt)
{
  CRect wrect;
  MyGetClientRect(&wrect);
  CSize image_sz = GetCurrentImageSize();
  int image_x = iWnd2Img(pt.x, wrect.Width(), image_sz.cx);
  int image_y = iWnd2Img(pt.y, wrect.Height(), image_sz.cy);
  return CPoint(image_x, image_y);
}

#define ARTIFICIAL_MOUSE_MOVE_SENT_FROM_OTHER_WINDOW 0xF8F8  // number doesn't matter, so long as it does not conflict with genuine flags

void FigureWnd::OnMouseMove(UINT nFlags, CPoint point) 
{
  if (nFlags == ARTIFICIAL_MOUSE_MOVE_SENT_FROM_OTHER_WINDOW)
  {
    nFlags = 0;
  }
  else 
  {
    for (int i=0 ; i<m_linked_figs.Len() ; i++)
    {
      m_linked_figs[i]->SendMessage(WM_MOUSEMOVE, ARTIFICIAL_MOUSE_MOVE_SENT_FROM_OTHER_WINDOW, (point.y << 16) | point.x);
    }
  }

  if (!m_mouse_is_inside_window)
  {
    // This block of code causes the OnMouseLeave() function to 
    // be called when the mouse leaves the window.
    m_mouse_is_inside_window = true;
  	TRACKMOUSEEVENT track;     // Declares structure
    track.cbSize = sizeof(track);
    track.dwFlags = TME_LEAVE; // Notify us when the mouse leaves
    track.hwndTrack = m_hWnd;  // Assigns this window's hwnd
    TrackMouseEvent(&track);   // Tracks the events like WM_MOUSELEAVE
  }

  // avoid invalid pixel accessing of an empty image
  if (GetCurrentImageSize() == CSize(0,0))  return;

  m_mouse_grabber.MouseMove(point);
//  m_last_mouse_pt = point;
//  m_last_mouse_pt_image_coords = ImageCoordFromClient(point);  // convert from client to image coordinates
//  CRect wrect;
//  MyGetClientRect(&wrect);

  {
    MouseGrabber::Mode m = m_mouse_grabber.GetMode();
//    if (m == MouseGrabber::MG_GRAB_RECT || m == MouseGrabber::MG_GRAB_NUM_CLICKS)
    if (m != MouseGrabber::MG_NONE)
    {
      // temporarily display crosshair cursor; will automatically revert back to arrow
      SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_CROSS)));
      Refresh();
    }
    else if (m_equal_mode)
    {
      Refresh();
    }
  }

  RefreshStatusPane();
  
  CWnd::OnMouseMove(nFlags, point);  // necessary?
}

void FigureWnd::OnLButtonDown(UINT nFlags, CPoint point) 
{
  m_mouse_grabber.LButtonDown( point );
//  m_mouse_clicks.Push( Figure::MouseClick( Figure::MC_LEFT, ImageCoordFromClient(point) ));
//  if (m_n_mouse_clicks_to_grab > 0)
//  {
//    m_n_mouse_clicks_to_grab--;
//  }
//  else if (m_mouse_clicks.Len() > m_mouse_clicks_queue_size)
//  {
////    m_mouse_clicks.erase( m_mouse_clicks.begin(), m_mouse_clicks.begin()+1 );
//    m_mouse_clicks.Delete( 0 );
//    assert(m_mouse_clicks.Len() == m_mouse_clicks_queue_size);
//  }

	CWnd::OnLButtonDown(nFlags, point);
}

void FigureWnd::OnRButtonDown(UINT nFlags, CPoint point) 
{
  m_mouse_grabber.RButtonDown( point );
//  m_mouse_clicks.Push( Figure::MouseClick( Figure::MC_RIGHT, ImageCoordFromClient(point) ));
//  if (m_n_mouse_clicks_to_grab > 0)
//  {
//    m_n_mouse_clicks_to_grab--;
//  }
//  else if (m_mouse_clicks.Len() > m_mouse_clicks_queue_size)
//  {
////    m_mouse_clicks.erase( m_mouse_clicks.begin(), m_mouse_clicks.begin()+1 );
//    m_mouse_clicks.Delete( 0 );
//    assert(m_mouse_clicks.Len() == m_mouse_clicks_queue_size);
//  }

	CWnd::OnRButtonDown(nFlags, point);
}

void FigureWnd::OnLButtonUp(UINT nFlags, CPoint point) 
{
  m_mouse_grabber.LButtonUp( point );
//  if (m_grab_rect && m_mouse_clicks.Len()>0)
//  {
//    m_mouse_clicks.Push( Figure::MouseClick( Figure::MC_LEFT, ImageCoordFromClient(point) ));
//    m_grab_rect = false;
//  }
	
	CWnd::OnLButtonUp(nFlags, point);
}

void FigureWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  if (nChar == 'E')  
  {
    m_equal_mode = !m_equal_mode;	
    Refresh();
  }
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void FigureWnd::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}

void FigureWnd::OnSize(UINT nType, int cx, int cy) 
{
  if (m_statusbar.m_hWnd)
  {
    // resize status bar
    m_statusbar.SendMessage(WM_SIZE, cx, cy);
    {
      // recalculate height, just in case
      CRect r;
      m_statusbar.GetWindowRect(&r);
      g_statusbar_height = r.Height();
    }
  }

	CWnd::OnSize(nType, cx, cy);
}

FigureWnd::FigureWnd(const CString& title, int x, int y) 
  : CWnd(), m_which(-1), m_title(title), //m_n_mouse_clicks_to_grab(0), m_grab_rect(false),
    m_equal_mode(false), m_display_coords_on_bottom(false),
    m_mouse_is_inside_window(false),
    m_mouse_grabber(this)
{
  CString title2 = title;
  CString title_tmp;
  if (title.GetLength() == 0)
  {
    title_tmp.Format(L"Figure %d", g_num++);
    title2 = title_tmp;
  }
  InitializeGlobal();
  if (x<0)  x = g_left;
  if (y<0)  y = g_top;
  CreateEx(0, g_class_name, title2, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CRect(x, y, x+200, y+200), NULL, 0);
  g_left = x + 20;
  g_top = y + 20;
  g_hwnd_list.Push(m_hWnd);
}

FigureWnd::~FigureWnd()
{
  HWND* p = std::find(g_hwnd_list.Begin(), g_hwnd_list.End(), m_hWnd);
  if (p != g_hwnd_list.End())
  {
    g_hwnd_list.Delete( p - g_hwnd_list.Begin() );  // old:  erase(p)
  }
}

CSize FigureWnd::GetCurrentImageSize()
{
  return (m_which>=0) ? CSize(m_img_bgr0.Width(), m_img_bgr0.Height()) : CSize(0,0);
}

void FigureWnd::Draw(const ImgBgr& img)
{
  if (img.IsNull())  return;
  CSize sz = GetCurrentImageSize();
  if (img.Width() != sz.cx || img.Height() != sz.cy)  ResizeClientArea(img.Width(), img.Height());
  m_img_bgr0 = img;
  m_which = 0;
//  m_statusbar.SetText("Bgr", 2, 0);
  Refresh();
}

void FigureWnd::Draw(const ImgBinary& img)
{
  if (img.IsNull())  return;
  CSize sz = GetCurrentImageSize();
  if (img.Width() != sz.cx || img.Height() != sz.cy)  ResizeClientArea(img.Width(), img.Height());
  m_img_binary1 = img;
  m_which = 1;
  {
    ImgGray gray;
    Convert(img, &gray, 0, 255);
    Convert(gray, &m_img_bgr0);
  }
//  m_statusbar.SetText("Binary", 2, 0);
  Refresh();
}

void FigureWnd::Draw(const ImgFloat& img)
{
  if (img.IsNull())  return;
  CSize sz = GetCurrentImageSize();
  if (img.Width() != sz.cx || img.Height() != sz.cy)  ResizeClientArea(img.Width(), img.Height());
  m_img_float2 = img;
  m_which = 2;
  { // could be made faster by reducing this 4-pass version to 2-pass
    ImgFloat img2;
    ImgGray gray;
    LinearlyScale(img, 0, 255, &img2);
    Convert(img2, &gray);  
    Convert(gray, &m_img_bgr0);
  }
  m_statusbar.SetText(L"Float", 2, 0);
  Refresh();
}

void FigureWnd::Draw(const ImgGray& img)
{
  if (img.IsNull())  return;
  CSize sz = GetCurrentImageSize();
  if (img.Width() != sz.cx || img.Height() != sz.cy)  ResizeClientArea(img.Width(), img.Height());
  m_img_gray3 = img;
  m_which = 3;
  Convert(img, &m_img_bgr0);
//  m_statusbar.SetText("Gray", 2, 0);
  Refresh();
}

void FigureWnd::Draw(const ImgInt& img)
{
  if (img.IsNull())  return;
  CSize sz = GetCurrentImageSize();
  if (img.Width() != sz.cx || img.Height() != sz.cy)  ResizeClientArea(img.Width(), img.Height());
  m_img_int4 = img;
  m_which = 4;
  { // could be made faster by reducing this 4-pass version to 2-pass
    ImgInt img2;
    ImgGray gray;
    LinearlyScale(img, 0, 255, &img2);
    Convert(img2, &gray);
    Convert(gray, &m_img_bgr0);
  }
//  m_statusbar.SetText("Int", 2, 0);
  Refresh();
}

void FigureWnd::DrawRect(const Rect& rect, const Bgr& color)
{
  blepo::DrawRect(rect, &m_img_bgr0, color);
  Refresh();
}

void FigureWnd::ResizeClientArea(int width, int height)
{
  // resize window to fit image exactly
  CRect crect, wrect;
  GetWindowRect(&wrect);
  MyGetClientRect(&crect);
  int new_width = width + wrect.Width() - crect.Width();
  int new_height = height + wrect.Height() - crect.Height();
  if (new_width != wrect.Width() || new_height != wrect.Height())
  {
    SetWindowPos(NULL, 0, 0, new_width, new_height, SWP_NOMOVE | SWP_NOZORDER);
  }
}

void FigureWnd::GrabMouseClicks(int n_mouse_clicks, Array<Figure::MouseClick>* points, COLORREF point_color, COLORREF line_color)
{
  m_mouse_grabber.EnterGrabNumClicksMode( n_mouse_clicks, point_color, line_color );
//  m_n_mouse_clicks_to_grab = n_mouse_clicks;
//  m_mouse_clicks.Reset();
//  points->clear();
//  while (m_n_mouse_clicks_to_grab > 0 && ProcessWindowsMessage());
  while (!m_mouse_grabber.IsDone() && iProcessWindowsMessage());
//  *points = m_mouse_clicks;
  *points = *m_mouse_grabber.GetMouseClickList();
  m_mouse_grabber.ResetMode();
//  m_mouse_clicks.Reset();
  Refresh();
}

void FigureWnd::GrabLeftMouseClicks(Array<Figure::MouseClick>* points, COLORREF point_color, COLORREF line_color)
{
  m_mouse_grabber.EnterGrabLeftClicksMode(point_color, line_color);
//  m_n_mouse_clicks_to_grab = n_mouse_clicks;
//  m_mouse_clicks.Reset();
//  points->clear();
//  while (m_n_mouse_clicks_to_grab > 0 && ProcessWindowsMessage());
  while (!m_mouse_grabber.IsDone() && iProcessWindowsMessage());
//  *points = m_mouse_clicks;
  *points = *m_mouse_grabber.GetMouseClickList();
  m_mouse_grabber.ResetMode();
//  m_mouse_clicks.Reset();
  Refresh();
}

Rect FigureWnd::GrabRect()
{
  m_mouse_grabber.EnterGrabRectMode();
//  m_grab_rect = true;
//  m_mouse_clicks.Reset();
//  m_n_mouse_clicks_to_grab = n_mouse_clicks;
//  points->clear();
//  while (m_grab_rect && ProcessWindowsMessage());
  while (!m_mouse_grabber.IsDone() && iProcessWindowsMessage());
  const Array<Figure::MouseClick>& mcl = *m_mouse_grabber.GetMouseClickList();
//  assert(m_mouse_clicks.Len() == 2);
  if (mcl.Len() == 2)
  {
    assert(mcl.Len() == 2);
  //  *points = m_mouse_clicks;
    Refresh();
    const Point& p1 = mcl[0].pt;
    const Point& p2 = mcl[1].pt;
    int left   = p1.x < p2.x ? p1.x : p2.x;
    int top    = p1.y < p2.y ? p1.y : p2.y;
    int right  = p1.x > p2.x ? p1.x : p2.x;
    int bottom = p1.y > p2.y ? p1.y : p2.y;
  //  m_mouse_clicks.Reset();
    m_mouse_grabber.ResetMode();
    return Rect(left, top, right, bottom);
  }
  else
  {
    return CRect(0,0,0,0);  // only gets called if user aborts iProcessWindowsMessage() by pressing key
  }
}

Figure::WhichButton FigureWnd::TestMouseClick(CPoint* pt)
{
//  m_mouse_grabber.EnterTestMouseClickMode();
  iProcessWindowsMessage();
  const Array<Figure::MouseClick>& mcl = *m_mouse_grabber.GetMouseClickList();
  if (mcl.Len() > 0)
  {
    const Figure::MouseClick& m = mcl[0];
    if (pt)  *pt = m.pt;
    Figure::WhichButton button = m.button;
//    m_mouse_clicks.erase( m_mouse_clicks.begin(), m_mouse_clicks.begin()+1 );
    m_mouse_grabber.ResetMode();
//    m_mouse_clicks.Delete( 0 );
    return button;
  }
  else
  {
    CPoint p = m_mouse_grabber.GetLastMousePoint(true);
    if (pt)  *pt = p;
    return Figure::MC_NONE;
  }
}

void FigureWnd::Link(FigureWnd* other, bool symmetric)
{
  m_linked_figs.Push(other);
  if (symmetric)  other->Link(this, false);
}


//////////////////////////////////////////////////////////////////
// Figure
//
// This class is for the end user.  Although it would make sense to derive
// it from FigureWnd, this is not possible because of the 'permanent' option.
// 'Permanent' means that the window is not deleted when the destructor of this
// class is called, and there is no way to prevent the destructor of a base class
// from being called from within the destructor of the derived class. 

Figure::Figure(const CString& title, int x, int y, bool permanent,bool disable_close_button)
  : m_permanent(permanent), m_wnd(NULL)
{
  m_wnd = new FigureWnd(title, x, y);
	DisableCloseButton(disable_close_button);
}

Figure::~Figure()
{
  if (!m_permanent)  { delete m_wnd;  m_wnd = NULL; }
}

void Figure::Draw(const ImgBgr&    img) { m_wnd->Draw(img); }
void Figure::Draw(const ImgBinary& img) { m_wnd->Draw(img); }
void Figure::Draw(const ImgFloat&  img) { m_wnd->Draw(img); }
void Figure::Draw(const ImgGray&   img) { m_wnd->Draw(img); }
void Figure::Draw(const ImgInt&    img) { m_wnd->Draw(img); }

// draws rect on current image
void Figure::DrawRect(const Rect& rect, const Bgr& color) { m_wnd->DrawRect(rect, color); }

void Figure::GrabMouseClicks(int n_mouse_clicks, Array<Figure::MouseClick>* points, COLORREF point_color, COLORREF line_color)
{
  m_wnd->GrabMouseClicks(n_mouse_clicks, points, point_color, line_color);
}

void Figure::GrabLeftMouseClicks(Array<Figure::MouseClick>* points, COLORREF point_color, COLORREF line_color)
{
  m_wnd->GrabLeftMouseClicks(points, point_color, line_color);
}

CPoint Figure::GrabMouseClick(WhichButton* button)
{
  Array<MouseClick> points;
  m_wnd->GrabMouseClicks(1, &points, -1, -1);
  if (points.Len() > 0)
  {
    if (button)  *button = points[0].button;
    return points[0].pt;
  }
  else
  {
    return CPoint(0,0);  // only gets called if user aborts iProcessWindowsMessage() by pressing key
  }
}

CRect Figure::GrabRect()
{
  return m_wnd->GrabRect();
}

Figure::WhichButton Figure::TestMouseClick(CPoint* pt)
{
  return m_wnd->TestMouseClick(pt);
}

void Figure::GetDisplay(ImgBgr* out) { m_wnd->GetDisplay(out); }

void Figure::SetWindowPosition(int x, int y)
{
  m_wnd->SetWindowPos(NULL, x, y, 0, 0, SWP_NOSIZE);
}

void Figure::SetWindowSize(int width, int height)
{
  m_wnd->SetWindowPos(NULL, 0, 0, width, height, SWP_NOMOVE);
}

void Figure::SetWindowRect(const CRect& rect)
{
  m_wnd->SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(), 0);
}

CPoint Figure::GetWindowPosition() const
{
  CRect rect;
  m_wnd->GetWindowRect(rect);
  return CPoint(rect.left, rect.top);
}

CSize Figure::GetWindowSize() const
{
  CRect rect;
  m_wnd->GetWindowRect(rect);
  return CSize(rect.Width(), rect.Height());
}

CRect Figure::GetWindowRect() const
{
  CRect rect;
  m_wnd->GetWindowRect(rect);
  return rect;
}

void Figure::PlaceToTheRightOf(const Figure& other)
{
  CRect rect1 = this->GetWindowRect();
  CRect rect2 = other.GetWindowRect();
  SetWindowRect(CRect(rect2.right, rect2.top, rect2.right + rect1.Width(), rect2.top + rect1.Height()));
}

void Figure::SetTitle(const CString& str)
{
  m_wnd->SetWindowText(str);
}

void Figure::SetVisible(bool visible)
{
  m_wnd->ShowWindow(visible);
}

bool Figure::IsClosed() const { return m_wnd->IsClosed(); }

void Figure::Link(Figure* other, bool symmetric)
{
  m_wnd->Link(other->m_wnd, symmetric);
}


/*
	Enable/Disable the close button on figure window.
	From 
	http://www.codeguru.com/forum/showthread.php?t=284001

	-nkk
*/
void Figure::DisableCloseButton(bool disable)
{
	UINT menuf = disable? (MF_BYCOMMAND | MF_GRAYED | MF_DISABLED) : (MF_BYCOMMAND);
	CMenu* pSM = m_wnd->GetSystemMenu(FALSE);
	if(pSM)
	{
		pSM->EnableMenuItem(SC_CLOSE, menuf);
	}
}

void Figure::ProcessWindowsMessage()
{
	iProcessWindowsMessage();
}


};  // namespace blepo
