#include "FigureGlut.h"
#include <assert.h>

namespace blepo
{

//////////////////////////////////////////////////////////////////////////////////
// FigureGlut

FigureGlut*  FigureGlut::g_which_one = NULL;

FigureGlut::FigureGlut(const CStringA& title, int win_width, int win_height,int method)
{ 
  int argc = 1;
  char* argv[1];
  argv[0] = "dummy-exe-name";
  width = win_width;
  height = win_height;
  iInitialize(title, width, height, &argc, argv);
  Draw_Method = method;
}

FigureGlut::FigureGlut(const CStringA& title, int win_width, int win_height, int* argc, char* argv[])
{
	width = win_width;
	height = win_height;
  iInitialize(title, width, height, argc, argv);
}

void FigureGlut::iInitialize(const CStringA& title, int win_width, int win_height, int* argc, char* argv[])
{
  if (g_which_one != NULL)
  {
    assert(0);  // error!  Can only have one GLUT window due to our implementation
    throw 0;
  }

  g_which_one = this;
  width = win_width;
  height = win_height;

  if (width < 0 || height < 0)
  {
    width = height = 600;  // default window size
  }

	//Initialize GLUT
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(width, height); //Set the window size

	//Create the window
	glutCreateWindow(title);

	//adding color material
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  //Initialize rendering
  //Makes 3D drawing work when something is in front of something else
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_NORMALIZE);

	//Set handler functions for drawing, keypresses, and window resizes
	glutDisplayFunc(iRedraw);
	glutKeyboardFunc(iHandleKeypress);
	glutReshapeFunc(iHandleResize);
	glutMotionFunc(iHandleMouseMoveWhileButtonDown);
	glutPassiveMotionFunc(iHandleMouseMove);
	glutMouseFunc(iHandleMouseClickUpOrDown);
}


//////////////////////////////////////////////////////////////////////////////////
// MyFigureGlut

MyFigureGlut::MyFigureGlut(const CStringA& title, int win_width, int win_height, int* argc, char* argv[]) 
  : FigureGlut(title, win_width, win_height, argc, argv)
{
  InitParameters();
}

MyFigureGlut::MyFigureGlut(const CStringA& title, int win_width, int win_height, int method) 
  : FigureGlut(title, win_width, win_height, method)
{
  Draw_Method = method;
  InitParameters();
}

void MyFigureGlut::SetTranslation(float x, float y, float z)
{
  m_pos_x = x;
  m_pos_y = y;
  m_pos_z = z;
}

void MyFigureGlut::GetTranslation(float* x, float* y, float* z) const
{
  *x = m_pos_x;
  *y = m_pos_y;
  *z = m_pos_z;
}

void MyFigureGlut::SetRotation(float x, float y, float z)
{
  m_rot_x = x;
  m_rot_y = y;
  m_rot_z = z;
}

void MyFigureGlut::GetRotation(float* x, float* y, float *z) const
{
  *x = m_rot_x;
  *y = m_rot_y;
  *z = m_rot_z;
}


void MyFigureGlut::InitParameters()
{
  m_pos_x = m_pos_y = m_pos_z = 0.0f;
  m_rot_x = m_rot_y = 0.0f;
  m_zoom = 0.6f;
  m_last_mouse_location = CPoint(-1, -1);
  m_control = false;
  m_shift = false;
  auto_adjust = false;
  SetTranslation(0,0,-10); //my guess where to start
}

void MyFigureGlut::SetPoints(PointCloud& pts)
{
  m_pts=pts;

  // back up viewpoint so we can see points
  {
    Cuboid box;
    GetBoundingBoxOfPoints(pts, &box);
    if(auto_adjust) {
		float zmax = box.max_z;
		// if zmax is too small, the object cannot be displayed.(?????)
		if ( zmax > 0 && zmax < 1) zmax += 1;  //zmax could be zero.
		if ( zmax == 0) zmax += 1.5;  
		SetTranslation(0, 0, -zmax);
	}
  }
}

void MyFigureGlut::SetMesh(PointCloud& pts)
{
  m_mesh=pts;

  // back up viewpoint so we can see points
  {
    Cuboid box;
    GetBoundingBoxOfPoints(pts, &box);
    if(auto_adjust) {
		float zmax = box.max_z;
		// if zmax is too small, the object cannot be displayed.(?????)
		if ( zmax > 0 && zmax < 1) zmax += 1;  //zmax could be zero.
		if ( zmax == 0) zmax += 1.5;  
		SetTranslation(0, 0, -zmax);
	}
  }
}

//same as SetPoints except for x,y scaling
void MyFigureGlut::SetDepthPoints(PointCloud& pts, int win_width, int win_height)
{
  PointCloud::ConstIterator p = pts.Begin();
  ColoredPoint temp;
  int cnt=0;
  float x_full, y_full;
  x_full=pts.Max().x - pts.Min().x;
  y_full=pts.Max().y - pts.Min().y;

	while(p != pts.End())
	{
		if(p->x != 0)
		{
			temp.x=(float)(((float(cnt%win_width))*x_full)/win_width) + pts.Min().x;
			temp.y=(float)(((float(cnt/win_width))*y_full)/win_height) + pts.Min().y;
			temp.z=(*p).z;
			temp.color=(*p).color;
			m_pts.Add(temp);
		}
		p++;
		cnt++;
	}

  // back up viewpoint so we can see points
  {
    Cuboid box;
    GetBoundingBoxOfPoints(pts, &box);
    if(auto_adjust) {
		float zmax = box.max_z;
		// if zmax is too small, the object cannot be displayed.(?????)
		if ( zmax > 0 && zmax < 1) zmax += 1;  //zmax could be zero.
		if ( zmax == 0) zmax += 1.5;  
		SetTranslation(0, 0, -zmax);
	}
  }
}

void MyFigureGlut::GetImage(ImgBgr *out) {
	  out->Reset(width,height);
	  //UINT *pixels=new UINT[width * height];
	  //memset(pixels, 0, sizeof(UINT)*width*height);
	  char *pixels = new char[width * height * 3];
	  glFlush(); 
	  glFinish();
	  glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels);
	  /*
	  //for flipping the image but not needed right now, apparently loading flipped?
	  int loop = 0, safeWidth = width - 1, safeHeight = height - 1, count = safeWidth, multiplier = width << 1;
	  ImgBgr::Iterator p = out->Begin(0,safeHeight);
	  char *data;
	  data = pixels;
	  while(loop < height) {
		char r = *data++, g = *data++, b = *data++;
		*p++ = Bgr(b,g,r);
		count--;
		if (count < 0) {
			loop++;
			p -= multiplier;
			count = safeWidth;
		}
	  }*/
	  ImgBgr::Iterator p = out->Begin();
	  char *data;
	  data = pixels;
	  while(p != out->End()) {
		char r = *data++, g = *data++, b = *data++;
		*p++ = Bgr(b,g,r);
	  }
	  delete [] pixels;
}

void MyFigureGlut::GetBoundingBoxOfPoints(const PointCloud& pts, Cuboid* out)
{
  assert(out);
  out->min_x = out->min_y = out->min_z =  FLT_MAX;
  out->max_x = out->max_y = out->max_z = -FLT_MAX;

  PointCloud::ConstIterator p = pts.Begin();
  while(p != pts.End()) {
    if ( p->x < out->min_x )  out->min_x = p->x;
    if ( p->y < out->min_y )  out->min_y = p->y;
    if ( -p->z < out->min_z )  out->min_z = -p->z;
    if ( p->x > out->max_x )  out->max_x = p->x;
    if ( p->y > out->max_y )  out->max_y = p->y;
    if ( -p->z > out->max_z )  out->max_z = -p->z;
	p++;
  }
}

void MyFigureGlut::OnDrawPoints()
{
  glPointSize(2.0);
  glBegin(GL_POINTS);
  PointCloud::ConstIterator p = m_pts.Begin();
  while(p != m_pts.End()){
    glColor3f(GLfloat(p->color.r / 255.0), GLfloat(p->color.g / 255.0), GLfloat(p->color.b /255.0));
    glVertex3f(p->x, p->y, -p->z);
    p++;
  }	
  glEnd();
}

void MyFigureGlut::OnDrawQuads()
{
    glBegin(GL_QUADS);                  // Start Drawing Quads
    PointCloud::ConstIterator p = m_pts.Begin();
    while(p != m_pts.End()){
      glColor3f(GLfloat(p->color.r / 255.0), GLfloat(p->color.g / 255.0), GLfloat(p->color.b /255.0));
      glVertex3f(p->x, p->y, -p->z);
      p++;
    }	
    glEnd();
}

void MyFigureGlut::OnDrawMesh()
{
	glLineWidth(1.0);
	PointCloud::ConstIterator p = m_mesh.Begin();
	while(p != m_mesh.End())
	{
		glBegin(GL_LINE_LOOP);
		glColor3f(GLfloat(p->color.r / 255.0), GLfloat(p->color.g / 255.0), GLfloat(p->color.b /255.0));
		glVertex3f(p->x, p->y, -p->z);
		p++;
		glColor3f(GLfloat(p->color.r / 255.0), GLfloat(p->color.g / 255.0), GLfloat(p->color.b /255.0));
		glVertex3f(p->x, p->y, -p->z);
		p++;
		glColor3f(GLfloat(p->color.r / 255.0), GLfloat(p->color.g / 255.0), GLfloat(p->color.b /255.0));
		glVertex3f(p->x, p->y, -p->z);
		p++;
		glEnd();
	}
}

void MyFigureGlut::DrawSphere(double radius, int slices, int stacks) {
	//Clear information from last draw
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW); //Switch to the drawing perspective
	glLoadIdentity(); //Reset the drawing perspective
	glPushMatrix(); //Save the current state of transformations
	glTranslatef(m_pos_x , m_pos_y , m_pos_z); //- _zmax);
	glScalef(m_zoom, m_zoom, m_zoom);
	glRotatef( m_rot_y, 1.0f, 0.0f, 0.0f);
	glRotatef( m_rot_x, 0.0f, 0.0f, 1.0f);
	glRotatef( m_rot_z, 0.0f, 1.0f, 0.0f);

	const GLfloat color[4] = {1.0f,0.0f,0.0f,1.0f};
    glMaterialfv(GL_FRONT,GL_DIFFUSE,color);
    glutSolidSphere(radius,slices,stacks);
    
	glPopMatrix();
    glutSwapBuffers();
	glutPostRedisplay();
}

void MyFigureGlut::OnRedraw()
{
  //Clear information from last draw
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glMatrixMode(GL_MODELVIEW); //Switch to the drawing perspective
  glLoadIdentity(); //Reset the drawing perspective
  glPushMatrix(); //Save the current state of transformations
  glTranslatef(m_pos_x , m_pos_y , m_pos_z); //- _zmax);
  glScalef(m_zoom, m_zoom, m_zoom);
  glRotatef( m_rot_y, 1.0f, 0.0f, 0.0f);
  glRotatef( m_rot_x, 0.0f, 0.0f, 1.0f);
  glRotatef( m_rot_z, 0.0f, 1.0f, 0.0f);

  OnDrawScene();

  glPopMatrix();  //Undo
  glutSwapBuffers(); //Send the 3D scene to the screen
}

void MyFigureGlut::OnKeypress(unsigned char key, int x, int y)
{
  switch (key) 
  {
  case 27: //Escape key
	exit(0);
	break;
  }
}

void MyFigureGlut::OnResize(int w, int h)
{
  // tell OpenGL how to convert from coordinates to pixel values
  glViewport(0, 0, w, h);
  
  glMatrixMode(GL_PROJECTION); //Switch to setting the camera perspective
  
  // set the camera perspective
  glLoadIdentity(); //Reset the camera
  gluPerspective(20.0,                  //The camera angle
                 (double)w / (double)h, //The width-to-height ratio
                 1.0,                   //The near z clipping coordinate
                 200.0);                //The far z clipping coordinate
}

void MyFigureGlut::OnMouseMoveWhileButtonDown(int x, int y)
{
  if ( m_last_mouse_location != CPoint(-1, -1) )
  {
    int dx = x - m_last_mouse_location.x;
    int dy = y - m_last_mouse_location.y;
    
    if (m_shift)
    { // zoom in / out
      float new_zoom = m_zoom - (dy / 100.0f);
      if (new_zoom > 0) // if this is not done, the image is reversed
      {
        m_zoom = new_zoom;
      }
    }
    else if (m_control)
    { // translate
      m_pos_x += dx * 0.1f;
      m_pos_y -= dy * 0.1f;
    }
    else if (!m_control && !m_shift)
    { // rotate
      m_rot_x += dx;
      m_rot_y += dy;
    }
  }
  glutPostRedisplay();
  m_last_mouse_location = CPoint( x, y );
}

void MyFigureGlut::OnMouseClickUpOrDown(int button, int state, int x, int y)
{
  int mod = glutGetModifiers();
  
  if (state == GLUT_DOWN)
  {
    m_control = (mod == GLUT_ACTIVE_CTRL);
    m_shift = (mod == GLUT_ACTIVE_SHIFT);
  }
  else
  {  
    assert( state == GLUT_UP );
    m_last_mouse_location = CPoint(-1, -1);
    m_control = false;
    m_shift = false;
  }
}

};  // end namespace blepo

