#ifndef __FIGURE_GLUT_H__
#define __FIGURE_GLUT_H__

//Include OpenGL header files, so that we can use OpenGL
#include <afxwin.h>  // TCHAR
#include "../Matrix/Matrix.h"  
#include "../Image/Image.h"
#include "../Image/PointCloud.h"
#include "../../external/OpenGL/freeglut.h"  // (order might matter here to avoid 'redefinition compile error; http://www.lighthouse3d.com/opengl/glut/)
#include <vector>
#include <float.h>  // FLT_MAXs



/**
This class makes it easy to create a window that uses OpenGL.  
Only one window can be created at a time with this implementation, due to the fact that GLUT
does not allow arbitrary information to be passed to callbacks.  Could be fixed with some work.
*/

// whether to draw points or quads (or both)
#define BLEPO_FIGUREGLUT_DRAW_POINTS 0
#define BLEPO_FIGUREGLUT_DRAW_QUADS 1
#define BLEPO_FIGUREGLUT_DRAW_TRIANGLES 2
#define BLEPO_FIGUREGLUT_DRAW_SPHERE 3

namespace blepo
{

//Basic Glut Figure Class
class FigureGlut
{
public:
  FigureGlut(const CStringA& title, int win_width, int win_height, int method=BLEPO_FIGUREGLUT_DRAW_POINTS);
  FigureGlut(const CStringA& title, int win_width, int win_height, int* argc, char* argv[]);
  
  ~FigureGlut() {}
  
  void EnterMainLoop()
  {
    glutMainLoop(); //Start the main loop.  glutMainLoop can return if you define GLUT_ACTION_ON_WINDOW_CLOSE option and invoke the glutLeaveMainLoop.
  }

  // Redraws the window but does not yield control to glut
  void Redraw()
  {
    glutMainLoopEvent();
    glutPostRedisplay();
  }
  
protected:
  int width,height;
  int Draw_Method;
  virtual void OnRedraw() {}
  virtual void OnKeypress(unsigned char key, int x, int y) {}
  virtual void OnResize(int w, int h) {}
  virtual void OnMouseMove(int x, int y) {}
  virtual void OnMouseMoveWhileButtonDown(int x, int y) {}
  virtual void OnMouseClickUpOrDown(int button, int state, int x, int y) {}
  
private:
  void iInitialize(const CStringA& title, int win_width, int win_height, int* argc, char* argv[]);
  
  static void iRedraw()
  {
    g_which_one->OnRedraw();
  }
  static void iHandleKeypress(unsigned char key, int x, int y)  //The key that was pressed	 & The current mouse coordinates
  {
    g_which_one->OnKeypress(key, x, y);
  }
  static void iHandleResize(int w, int h)
  {
    g_which_one->OnResize(w, h);
  }
  static void iHandleMouseMove(int x, int y)
  {
    g_which_one->OnMouseMove(x, y);
  }
  static void iHandleMouseMoveWhileButtonDown(int x, int y)
  {
    g_which_one->OnMouseMoveWhileButtonDown(x, y);
  }
  static void iHandleMouseClickUpOrDown(int button, int state, int x, int y)
  {
    g_which_one->OnMouseClickUpOrDown(button, state, x, y);
  }
private:
  static FigureGlut* g_which_one;  // need to change this architecture to allow for multiple windows
};


/**
This class creates a window that uses OpenGL and implements basic routines like mouse callbacks for 
navigating (translating, zooming, and rotation) the space, and for drawing points.
*/

class MyFigureGlut : public FigureGlut
{
public:

  struct Cuboid
  {
    float min_x, max_x, min_y, max_y, min_z, max_z;
  };

public:
  MyFigureGlut(const CStringA& title, int win_width, int win_height, int* argc, char* argv[]);
  MyFigureGlut(const CStringA& title, int win_width, int win_height, int method=BLEPO_FIGUREGLUT_DRAW_POINTS);
  ~MyFigureGlut() {}
  
  static void GetBoundingBoxOfPoints(const PointCloud& pts, Cuboid* out);

  void SetTranslation(float x, float y, float z);
  void GetTranslation(float* x, float* y, float* z) const;
  void SetRotation(float x, float y, float z);
  void GetRotation(float* x, float* y, float *z) const;
  void InitParameters();
  
  void SetPoints(PointCloud& pts);

  void SetMesh(PointCloud& pts);

  void DrawCloud(blepo::PointCloud &cloud) {
    SetPoints(cloud);
  }
  void DrawMesh(blepo::PointCloud &mesh) {
	SetMesh(mesh);
  };
  void DrawSphere(double radius, int slices, int stacks);

  //same as SetPoints() function except that the x,y values 
  void SetDepthPoints(PointCloud& pts, int win_width, int win_height);

  void DrawDepthCloud(blepo::PointCloud &cloud, int win_width, int win_height) {
    SetDepthPoints(cloud,win_width,win_height);
  }

  void GetImage(ImgBgr *out);

protected:
  virtual void OnDrawPoints();
  virtual void OnDrawQuads();
  virtual void OnDrawMesh();

  virtual void OnDrawScene()
  {
    if(Draw_Method == BLEPO_FIGUREGLUT_DRAW_POINTS)
      OnDrawPoints();
    else if(Draw_Method == BLEPO_FIGUREGLUT_DRAW_QUADS)
      OnDrawQuads();
	else if(Draw_Method == BLEPO_FIGUREGLUT_DRAW_TRIANGLES)
	{
		OnDrawPoints();
		OnDrawMesh();
	}
	else {
		DrawSphere(2.0f,20,20);
	}
  }

  virtual void OnRedraw();
  virtual void OnKeypress(unsigned char key, int x, int y);  
  virtual void OnResize(int w, int h);
  virtual void OnMouseMoveWhileButtonDown(int x, int y);  
  virtual void OnMouseClickUpOrDown(int button, int state, int x, int y);
  
private:
public:
  float m_zoom;                       // Zooming (or Scaling) factor
  float m_pos_x, m_pos_y, m_pos_z;    // X, Y and Z translation
  float m_rot_x, m_rot_y, m_rot_z;             // X, Y rotation
  CPoint m_last_mouse_location;       // Last location of the mouse
  bool m_control, m_shift;            // Whether keyboard buttons down
  bool auto_adjust;

  // data to display
  PointCloud m_pts, m_mesh;
};

};  // end namespace blepo

#endif // __FIGURE_GLUT_H__
