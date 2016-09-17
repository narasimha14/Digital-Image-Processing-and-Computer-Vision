/* 
 * Copyright (c) 2007 Clemson University.
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

// This file contains an implementation of the Douglas-Peucker algorithm.
// Ported from P. Kovesi's MATLAB and Octave Functions for Computer Vision and Image Processing
// at http://www.csse.uwa.edu.au/~pk/research/matlabfns

#include <afxwin.h>  // CPoint
#include "Image.h"
#include "ImageAlgorithms.h"
#include <algorithm>

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

using namespace std;

// ================> begin local functions (available only to this translation unit)
namespace
{
using namespace blepo;

typedef std::vector<CPoint> Edge;
enum STATUS{NOPOINT, THEREISAPOINT, LASTPOINT};

bool IsJunctions(const CPoint& pt, const Edge& junctions)
{
  int i; 
  bool exist = false;
  for (i = 0 ; i < (int) junctions.size() ; i++)
  {
    if(junctions[i] == pt)
    {
      exist = true;
      break;
    }
    
  }
  return exist;
}

//bool isJunctions(const ImgBinary& edgeim, const CPoint& pt)
//{
//  bool A[8], B[8];
//  int x = pt.x;
//  int y = pt.y;
//  bool junc;
//  if(x >= 0 && y>=0 && x < edgeim.Width() && y < edgeim.Height())
//  {
//    if(edgeim(x,y))
//    {
//      A[0] = B[7] = edgeim(x-1,y-1);
//      A[1] = B[0] = edgeim(x-1,y);
//      A[2] = B[1] = edgeim(x-1,y+1);
//      A[3] = B[2] = edgeim(x,y+1);
//      A[4] = B[3] = edgeim(x+1,y+1);
//      A[5] = B[4] = edgeim(x+1,y);
//      A[6] = B[5] = edgeim(x+1,y-1);
//      A[7] = B[6] = edgeim(x,y-1);
//      int sum = 0;
//      for(int k = 0; k < 8; k++)
//        sum += abs(A[k] - B[k]);
//      if(sum >= 6)
//        junc = true;
//      else
//        junc = false;
//    }
//    else
//      junc = false;
//  }
//  else
//    junc = false;
//
//  return junc;
//}

void neighboors(int width, int height, const CPoint& pt, std::vector<CPoint> *ns)
{
  int r[8] = {pt.x-1, pt.x,  pt.x+1,  pt.x+1, pt.x+1, pt.x, pt.x-1, pt.x-1};
  int c[8] = {pt.y-1, pt.y-1, pt.y-1, pt.y, pt.y+1, pt.y+1,  pt.y+1, pt.y};
  ns->clear();
  int i;
  for(i = 0; i < 8; i++)
  {
    if(r[i]>=0 && c[i]>=0 && r[i]<width && c[i]<height)
    {
      CPoint pt(r[i], c[i]);
      ns->push_back(pt);
    }
  }
  
}

int nextpoint(ImgInt *EDGEIM, CPoint pt, const Edge& junctions, int edgeNo, CPoint *nextpt)
{
  int status = 0; 
  int i,j;
  int height = EDGEIM->Height();
  int width = EDGEIM->Width();
  // row and column offsets for the eight neighbours of a point
  int c[8] = {pt.y-1, pt.y,  pt.y+1,  pt.y, pt.y-1, pt.y-1, pt.y+1, pt.y+1};
  int r[8] = {pt.x, pt.x+1, pt.x, pt.x-1, pt.x-1, pt.x+1,  pt.x+1, pt.x-1};
  int num = 0;
  
  
  for(i = 0; i < 8; i++)
  {
    if(r[i]>=0 && c[i]>=0 && r[i]<width && c[i]<height)
    {
      CPoint pt(r[i],c[i]);
      int val = (*EDGEIM)(r[i],c[i]);
      // Search through neighbours and see if one is a junction point
      
      if(val != -edgeNo &&  IsJunctions(pt, junctions))
      {
        nextpt->x = r[i];
        nextpt->y = c[i];
        status = LASTPOINT;
        return status; //Break out
      }
    }
  }
  
  // If we get here there were no junction points.  Search through neighbours
  // and return first connected edge point that itself has less than two
  // neighbours connected back to our current edge.  This prevents occasional
  // erroneous doubling back onto the wrong segment
  CPoint remember;
  bool checkflag = false;
  for(i = 0; i < 8; i++)
  {
    if(r[i]>=0 && c[i]>=0 && r[i]<width && c[i]<height && (*EDGEIM)(r[i],c[i])==1)
    {
      
      CPoint pt(r[i],c[i]);
      std::vector<CPoint> ns;
      neighboors(width, height, pt, &ns);
      int num = 0;
      for(j = 0; j < (int) ns.size() ; j++)
      {
        if((*EDGEIM)(ns[j].x, ns[j].y) == -edgeNo)
          num++;
      }
      if(num < 2)
      {
        nextpt->x = r[i];
        nextpt->y = c[i];
        status = THEREISAPOINT;
        return status;          //Break out
      }
      else
      {
        checkflag = true;      //have to use it
        remember.x = r[i];
        remember.y = c[i];
      }
    }
  }
  // If we get here (and 'checkFlag' is true) there was no connected edge point
  // that had less than two connections to our current edge, but there was one
  // with more.  Use the point we remembered above.
  if(checkflag)
  {
    nextpt->x =  remember.x;
    nextpt->y =  remember.y;
    status = THEREISAPOINT;
    return status;       //Break out
  }
  
  //if we get here there was no connecting next point at all.
  nextpt->x = 0;
  nextpt->y = 0;
  status = NOPOINT;
  return status;
}

int nextpoint(const ImgBinary& edgeim, ImgInt *EDGEIM, CPoint pt, const Edge& junctions, int edgeNo, CPoint *nextpt)
{
  int status = 0; 
  int i,j;
  int height = EDGEIM->Height();
  int width = EDGEIM->Width();
  // row and column offsets for the eight neighbours of a point
  int c[8] = {pt.y-1, pt.y,  pt.y+1,  pt.y, pt.y-1, pt.y-1, pt.y+1, pt.y+1};
  int r[8] = {pt.x, pt.x+1, pt.x, pt.x-1, pt.x-1, pt.x+1,  pt.x+1, pt.x-1};
  int num=0;
  for(i = 0; i < 8; i++)
  {
    if(edgeim(r[i],c[i]) == 1)
      num++;

  }
  if(num>2)
  {
  for(i = 0; i < 8; i++)
  {
    if(r[i]>=0 && c[i]>=0 && r[i]<width && c[i]<height)
    {
      CPoint pt(r[i],c[i]);
      int val = (*EDGEIM)(r[i],c[i]);
      // Search through neighbours and see if one is a junction point
      if(val != -edgeNo &&  IsJunctions(pt, junctions))//isJunctions(edgeim, pt))//
      {
        nextpt->x = r[i];
        nextpt->y = c[i];
        status = LASTPOINT;
        return status; //Break out
      }
    }
  }
  }
  // If we get here there were no junction points.  Search through neighbours
  // and return first connected edge point that itself has less than two
  // neighbours connected back to our current edge.  This prevents occasional
  // erroneous doubling back onto the wrong segment
  CPoint remember;
  bool checkflag = false;
  for(i = 0; i < 8; i++)
  {
    if(r[i]>=0 && c[i]>=0 && r[i]<width && c[i]<height && (*EDGEIM)(r[i],c[i])==1)
    {
      
      CPoint pt(r[i],c[i]);
      std::vector<CPoint> ns;
      neighboors(width, height, pt, &ns);
      int num = 0;
      for(j = 0; j < (int) ns.size(); j++)
      {
        if((*EDGEIM)(ns[j].x, ns[j].y) == -edgeNo)
          num++;
      }
      if(num < 2)
      {
        nextpt->x = r[i];
        nextpt->y = c[i];
        status = THEREISAPOINT;
        return status;          //Break out
      }
      else
      {
        checkflag = true;      //have to use it
        remember.x = r[i];
        remember.y = c[i];
      }
    }
  }
  // If we get here (and 'checkFlag' is true) there was no connected edge point
  // that had less than two connections to our current edge, but there was one
  // with more.  Use the point we remembered above.
  if(checkflag)
  {
    nextpt->x =  remember.x;
    nextpt->y =  remember.y;
    status = THEREISAPOINT;
    return status;       //Break out
  }
  
  //if we get here there was no connecting next point at all.
  nextpt->x = 0;
  nextpt->y = 0;
  status = NOPOINT;
  return status;
}

void trackedge(const ImgBinary& edgeim, ImgInt *EDGEIM, const CPoint& startPt, const Edge& junctions, int edgeNo, Edge* edgePoints)
{
  int x = startPt.x;
  int y = startPt.y;
  (*EDGEIM)(x,y) = -edgeNo; // Start a new list for this edge.
  edgePoints->push_back(startPt); // Edge points in the image are encoded by -ve of their edgeNo.
  
  CPoint nextpt;
  //Find next connected  edge point
  int status = nextpoint(edgeim, EDGEIM, startPt, junctions, edgeNo, &nextpt); 
  
  while(status != NOPOINT)
  {
    x = nextpt.x;
    y = nextpt.y;
    edgePoints->push_back(nextpt); //Add point to point list.
    (*EDGEIM)(x,y) = -edgeNo;      //Update edge image.
    if(status == LASTPOINT)        // We have hit a junction point.
      status = NOPOINT;            //make sure we stop tracking here.
    else                           //Otherwise keep going.
    {
      status = nextpoint(edgeim, EDGEIM, nextpt, junctions, edgeNo, &nextpt); 
    }
    
  }
  //Now track from original point in the opposite direction - but only if
  //the starting point was not a junction point.
  if(!IsJunctions(startPt, junctions))//(!isJunctions(edgeim, startPt))//
  {
    std::reverse(edgePoints->begin( ), edgePoints->end( ) ); //First reverse order of existing points in the edge list
    status = nextpoint(EDGEIM, startPt, junctions, edgeNo, &nextpt); 
    while(status != NOPOINT)
    {
      x = nextpt.x;
      y = nextpt.y;
      edgePoints->push_back(nextpt); //Add point to point list.
      
      (*EDGEIM)(x,y) = -edgeNo;      //Update edge image.
      if(status == LASTPOINT)        // We have hit a junction point.
        status = NOPOINT;            //make sure we stop tracking here.
      else                           //Otherwise keep going.
      {
        status = nextpoint(edgeim, EDGEIM, nextpt, junctions, edgeNo, &nextpt); 
      }
    }
  }
  // Final check to see if this edgelist should have start and end points
  // matched to form a loop.  If the number of points in the list is four or
  // more (the minimum number that could form a loop), and the endpoints are
  // within a pixel of each other, append a copy if the first point to the
  // end to complete the loop
//    if(edgePoints->size() >= 4)
//    {
//      int xdiff = abs(edgePoints->front().x - edgePoints->back().x);
//      int ydiff = abs(edgePoints->front().y - edgePoints->back().y);
//      if(xdiff <= 1 && ydiff <= 1)
//        edgePoints->push_back(edgePoints->front());
//      
//    }
}

void findendsjunctions(const ImgBinary& edgeim, Edge *junctions, Edge *ends)
{
  bool A[8], B[8];
  junctions->clear();
  ends->clear();
  int i,j,k;
  //FILE *fp=fopen("test6.txt","w");
  for(i = 1; i < edgeim.Width()-1; i++)
  {  
    for(j = 1; j < edgeim.Height()-1; j++)
      
    {
      if(edgeim(i,j))
      {
        A[0] = B[7] = edgeim(i-1,j-1);
        A[1] = B[0] = edgeim(i-1,j);
        A[2] = B[1] = edgeim(i-1,j+1);
        A[3] = B[2] = edgeim(i,j+1);
        A[4] = B[3] = edgeim(i+1,j+1);
        A[5] = B[4] = edgeim(i+1,j);
        A[6] = B[5] = edgeim(i+1,j-1);
        A[7] = B[6] = edgeim(i,j-1);
        int sum = 0;
        for(k = 0; k < 8; k++)
          sum += abs(A[k] - B[k]);
        if(sum >= 6)
        {
          CPoint pt(i,j);
          junctions->push_back(pt);
          //fprintf(fp, "x =%d, y =%d\n", pt.x, pt.y);
        }
        else if(sum==2)
        {
          CPoint pt(i,j);
          ends->push_back(pt);
        }
      }
      
    }
  }
  //fclose(fp);
}


void edgelink(const ImgBinary& edgeim, int min_len, vector<Edge> *edgelist, ImgInt *labels)
{
  std::vector<CPoint> junctions, ends;
  findendsjunctions(edgeim, &junctions, &ends);
  
  ImgInt EDGEIM;
  Convert(edgeim,&EDGEIM);
  int i, j;
  int edgeNo = 0; 
  for(j = 0; j < EDGEIM.Height(); j++)
  { 
    for(i = 0; i < EDGEIM.Width(); i++)
    {
      if(EDGEIM(i,j) == 1)
      {
        CPoint pt(i,j);
        Edge edgePoints;
        //trackedge(&EDGEIM, pt, junctions, edgeNo, &edgePoints);
        trackedge(edgeim, &EDGEIM, pt, junctions, edgeNo, &edgePoints);
        if(edgePoints.size()!=0)
        {
          edgeNo++;
          edgelist->push_back(edgePoints);
        }
      }
    }
  }
}

// Eqn of line joining end pts (x1 y1) and (x2 y2) can be parameterised by
//
//    x*(y1-y2) + y*(x2-x1) + y2*x1 - y1*x2 = 0
//
// (See Jain, Rangachar and Schunck, "Machine Vision", McGraw-Hill
// 1996. pp 194-196)
void maxlinedev(const vector<CPoint> edge, int first, int last, int *index, float *maxdev)
{
  
  float xdiff = (float)(edge[last].x-edge[first].x);
  float ydiff = (float)(edge[first].y-edge[last].y);
  
  float D = sqrt(xdiff * xdiff + ydiff * ydiff);
  vector<float> d;
  int i;
  if(D > 0)
  {
    float C = (float)(edge[last].y * edge[first].x - edge[first].y * edge[last].x);
    for(i = first; i<=last; i++)
    {
      float temp = fabs(edge[i].x * ydiff + edge[i].y * xdiff + C)/D;
      d.push_back(temp);
    }
  }
  else
  {
    for(i = first; i<=last; i++)
    {
      float temp = static_cast<float>( (edge[i].x - edge[first].x) * (edge[i].x - edge[first].x) +
        (edge[i].y - edge[first].y) * (edge[i].y - edge[first].y) );
      temp = sqrt(temp);
      d.push_back(temp);
    }
  }
  *maxdev = 0;
  *index = first;
  for(i = 0; i < (int) d.size(); i++)
  {
    if(*maxdev < d[i])
    {
      *maxdev = d[i];
      *index = i;
    }
    
  }
}

void linklines(vector<Edge>* seglist, float tol)
{
  int num_segs = seglist->size();
  int i, j;
  
  for(i = 0; i <  num_segs; i++)
  {
    if((*seglist)[i].size()>1)
    {
      CPoint pt = (*seglist)[i].front();
      CPoint pt_end = (*seglist)[i].back();
      int x1 = (*seglist)[i][1].x;
      int y1 = (*seglist)[i][1].y;
      if(abs(pt.x-pt_end.x)<=10 && abs(pt.x-pt_end.x)>=3)
      {
      
        for(j = 0; j <  num_segs; j++)
        {
          if(i != j && (*seglist)[j].size()>1 )
          {
            CPoint pt1 = (*seglist)[j].front();
            CPoint pt2 = (*seglist)[j].back();
            //          float diff1 =  pow(pt.x-pt1.x,2) + pow(pt.y-pt1.y,2) ;
            //          float diff2 =  pow(pt.x-pt2.x,2) + pow(pt.y-pt2.y,2) ;
            
            float slope = fabs(atan((float)(pt2.y-pt1.y)/(float)(pt2.x-pt1.x)));
            
            
            int size = (*seglist)[j].size();
            if(pt.x >= 0 && pt1.x >= 0 && abs(pt.x-pt1.x) <4 && abs(pt.y-pt1.y)<4 && slope < 1.1  
              && abs(pt2.x-pt1.x)<=10 && abs(pt2.x-pt1.x)>=3) //check head
            { 
              int x2 = (*seglist)[j][1].x;
              int y2 = (*seglist)[j][1].y;
              float xdiff = static_cast<float>( x2 - x1 );
              float ydiff = static_cast<float>( y1 - y2 );
              float D = sqrt( xdiff * xdiff + ydiff * ydiff);
              float C = (float)(y2 * x1 - y1 * x2);
              float deviate1 = fabs(pt.x * ydiff + pt.y * xdiff + C)/D;
              float deviate2 = fabs(pt1.x * ydiff + pt1.y * xdiff + C)/D;
              if(deviate1< tol && deviate2 < tol)
              {
                (*seglist)[i][0].x = -1;//remove this line
                (*seglist)[j][0].x = x1;
                (*seglist)[j][0].y = y1;
                
              }
              
            }
            else if(pt.x >= 0 && pt2.x >= 0 &&  abs(pt.x-pt2.x) <4 && abs(pt.y-pt2.y)<4 && (*seglist)[j][size-2].x >=0 && slope < 1.1 && abs(pt2.x-pt1.x)<=10) //check tail
            { 
              
              int x2 = (*seglist)[j][size-2].x;
              int y2 = (*seglist)[j][size-2].y;
              float xdiff = static_cast<float>( x2- x1 );
              float ydiff = static_cast<float>( y1 - y2 );
              float D = sqrt( xdiff * xdiff + ydiff * ydiff);
              float C = (float)(y2 * x1 - y1 * x2);
              float deviate1 = fabs(pt.x * ydiff + pt.y * xdiff + C)/D;
              float deviate2 = fabs(pt2.x * ydiff + pt2.y * xdiff + C)/D;
              if(deviate1< tol && deviate2 < tol)
              {
                (*seglist)[i][0].x = -1; //remove this line
                (*seglist)[j][size-1].x = x1;
                (*seglist)[j][size-1].y = y1;
                
              }
            }
          }
        }
      }
    }
  }  
}

void lineseg(const vector<Edge>& edgelist, float tol, vector<Edge>* seglist )
{
  int num_edge = edgelist.size();
  int i;
  seglist->clear();
  
  for(i = 0; i <  num_edge; i++)
  {
    vector<CPoint> edge = edgelist[i];
    int first = 0;                // Indices of first and last points in edge
    int last = edge.size()-1;         // segment being considered.
    int  Npts = 1;
    vector<CPoint> seg;
    seg.push_back(edge[first]);
    int  index;
    float maxdev;
    while( first< last)
    {
      maxlinedev(edge, first, last, &index, &maxdev);
      int min_seg = min(index, last - first - index);
      float tol_adjust = static_cast<float>( -tol*( 1 - exp(0.1*min_seg) )/ ( 1 + exp(0.1*min_seg) ) );
      while(maxdev>tol_adjust && min_seg > 6)      // While deviation is > tol
        // Shorten line to point of max deviation by adjusting lst
      {
        last = index + first;
        maxlinedev(edge, first, last, &index, &maxdev);
        min_seg = min(index, last - first - index);
        tol_adjust = static_cast<float>( -tol*( 1 - exp(0.1*min_seg) )/ ( 1 + exp(0.1*min_seg) ) );
      }
      seg.push_back(edge[last]);
      first = last;            // reset fst and lst for next iteration
      last = edge.size()-1;
    }
    
    seglist->push_back(seg);
    
  }
  
}

void bd(CvPoint* points, int count, int *max_x, int *min_x, int *max_y, int *min_y)
{
	*max_x = 0, *min_x = 999999999, *max_y = 0, *min_y = 999999999;
	for(int i = 0; i < count; i++)
	{
		int x = points[i].x;
		int y = points[i].y;
		*max_x = x > *max_x ? x : *max_x;
		*max_y = y > *max_y ? y : *max_y;
		*min_x = x < *min_x ? x : *min_x;
		*min_y = y < *min_y ? y : *min_y;
		
	}
	
}

void endpts(CvPoint* points, int count, float* line, CvPoint *pt1, CvPoint*pt2)
{
	float d = (float) sqrt((double)line[0]*line[0] + (double)line[1]*line[1]);
	line[0] /= d;
	line[1] /= d;
	int max_x, min_x, max_y, min_y;
	bd(points, count, &max_x, &min_x, &max_y, &min_y);
	float t;
	CvPoint pts_tmp[4];
	t = (max_x - line[2])/line[0];
	int x = cvRound(line[2] + line[0]*t);
	int y = cvRound(line[3] + line[1]*t);
	pts_tmp[0].x = x;
	pts_tmp[0].y = y;
	t = (max_y - line[3])/line[1];
	x = cvRound(line[2] + line[0]*t);
	y = cvRound(line[3] + line[1]*t);
	pts_tmp[1].x = x;
	pts_tmp[1].y = y;
	
	t = (line[2] - min_x)/line[0];
	x = cvRound(line[2] - line[0]*t);
	y = cvRound(line[3] - line[1]*t);
	pts_tmp[2].x = x;
	pts_tmp[2].y = y;
	t = (line[3] - min_y)/line[1];
	x = cvRound(line[2] - line[0]*t);
	y = cvRound(line[3] - line[1]*t);
	pts_tmp[3].x = x;
	pts_tmp[3].y = y;
	pt1->x = -1;
	pt1->y = -1;
	float max_dist = 0;
	for(int i = 0; i < 4; i++)
	{
		if(pts_tmp[i].x <= max_x && 
			pts_tmp[i].x >= min_x &&
			pts_tmp[i].y <= max_y &&
			pts_tmp[i].y >= min_y)
			
		{
			for(int j = i + 1; j < 4; j++)
			{
				if(pts_tmp[j].x <= max_x && 
					pts_tmp[j].x >= min_x &&
					pts_tmp[j].y <= max_y &&
					pts_tmp[j].y >= min_y)
				{
					float dist = (float) sqrtf((float) pow(double(pts_tmp[i].x-pts_tmp[j].x),double(2))) + 
						(float) pow(double(pts_tmp[i].y-pts_tmp[j].y),double(2));
					if(dist>max_dist)
					{
						max_dist = dist;
						pt1->x = pts_tmp[i].x;
						pt1->y = pts_tmp[i].y;
						pt2->x = pts_tmp[j].x;
						pt2->y = pts_tmp[j].y;
						
					}
				}
			}
		}
		
	}
}

};
// ================< end local functions

namespace blepo
{

// Suggestion:  There is no need for this to be a class.  It has no constructor
// or destructor, nor does it have any member variables.  I think a function would
// be more appropriate. -- STB
LineFitting::LineFitting()
{
  
}

LineFitting::~LineFitting()
{

}

// Suggestion:  This should probably call the binary version of DetectLines, to
// avoid unnecessary code duplication -- STB
void LineFitting::DetectLines(const ImgBgr& img, int min_len, vector<Line> *lines)
{
  ImgBinary edgeim;
  ImgGray gimg;
  Convert(img, &gimg);
  const float sigma = 1.0f;
  Canny(gimg, &edgeim, sigma);
  std::vector<Edge> edgelist, seglist;
  ImgInt labels;
  
  edgelink(edgeim, 0, &edgelist, &labels); //seting min_len = 0 will speed up much more. 
  lineseg(edgelist, 1.5, &seglist);
  linklines(&seglist, 1.5);
  ImgBgr im_display = img;
  
  int i, j;
  for(i = 0; i < (int) seglist.size(); i++)
  {
    Edge seg = seglist[i];
    for(j = 0; j < (int) seg.size()-1; j++)
    {
      float dist = sqrt(pow((float) (seg[j+1].x-seg[j].x), 2) + pow((float) (seg[j+1].y-seg[j].y), 2));
      if(dist> min_len && seg[j].x >=0 )
      {
        Line line;
        line.p1.x = seg[j].x; 
        line.p1.y = seg[j].y; 
        line.p2.x = seg[j+1].x; 
        line.p2.y = seg[j+1].y;
        double y_d = (double)(line.p2.y - line.p1.y);
        double x_d = (double)(line.p2.x - line.p1.x);
        double angle = atan(x_d/y_d);
        line.theta = static_cast<float>( (x_d==0) ? 0.0f : angle*180/3.14159f );
        if(angle < 0)
          line.rho = static_cast<float>( (double)line.p1.x * cos(-angle) + (double)line.p1.y * sin(-angle) );
        else
          line.rho = static_cast<float>( (double)line.p1.y * sin(angle) - (double)line.p1.x * cos(angle) );
        
        lines->push_back(line);
      }
    }
  }

}
void LineFitting::DetectLines(const ImgBinary& edgeim, int min_len, vector<Line> *lines)
{

  std::vector<Edge> edgelist, seglist;
  ImgInt labels;
  int i, j;
  edgelink(edgeim, 0, &edgelist, &labels); //seting min_len = 0 will speed up much more. 
  lineseg(edgelist, 2 , &seglist);

  linklines(&seglist,2);// 1.6 for door detection
  
  
  
  for(i = 0; i < (int) seglist.size(); i++) 
  {
    Edge seg = seglist[i];
    for(j = 0; j < (int) seg.size()-1; j++)
    {
      float dist = static_cast<float>( sqrt(pow((double) (seg[j+1].x-seg[j].x), 2) + pow((double) (seg[j+1].y-seg[j].y), 2)) );
      if(dist> min_len && seg[j].x >=0 )
      {
        Line line;
        if(seg[j].y < seg[j+1].y)
        {
          line.p1.x = seg[j].x; 
          line.p1.y = seg[j].y; 
          line.p2.x = seg[j+1].x; 
          line.p2.y = seg[j+1].y;
        }
        else
        {
          line.p2.x = seg[j].x; 
          line.p2.y = seg[j].y; 
          line.p1.x = seg[j+1].x; 
          line.p1.y = seg[j+1].y;
        }


        double y_d = (double)(line.p2.y - line.p1.y);
        double x_d = (double)(line.p2.x - line.p1.x);
        double angle = atan(x_d/y_d);
        line.theta = static_cast<float>( (x_d==0) ? 0.0f : angle*180/3.14159f );
        if(angle < 0)
          line.rho = static_cast<float>( (double)line.p1.x * cos(-angle) + (double)line.p1.y * sin(-angle) );
        else
          line.rho = static_cast<float>( (double)line.p1.y * sin(angle) - (double)line.p1.x * cos(angle) );
        
        lines->push_back(line);
      }
    }
  }
}

void LineFitting::FitLine2D(const std::vector<CPoint>& pts, LineFitting::Line *line)
{
    CvPoint pt1, pt2;
    int count = pts.size();
    if (count< 2)
    {
		line->p1.x = -1;
		line->p1.y = -1;
		line->p2.x = -1;
		line->p2.y = -1;
		return;
    }
	
    //convert to cv Points
    CvPoint* points = (CvPoint*)malloc( count * sizeof(points[0]));
    CvMat pointMat = cvMat( 1, count, CV_32SC2, points );
    for(int i = 0 ; i < count; i++ ) {
		points[i].x = pts[i].x;
		points[i].y = pts[i].y;
    }
    float linePara[4];
    cvFitLine( &pointMat, CV_DIST_L1, 1, 0.001, 0.001, linePara);
    endpts(points, count, linePara, &pt1, &pt2);
    if(pt1.y < pt2.y)
    {
		line->p1.x = pt1.x;
		line->p1.y = pt1.y;
		line->p2.x = pt2.x;
		line->p2.y = pt2.y;
    }
    else
    {
		line->p1.x = pt2.x;
		line->p1.y = pt2.y;
		line->p2.x = pt1.x;
		line->p2.y = pt1.y;
    }
    double y_d = (double)(pt2.y - pt1.y);
    double x_d = (double)(pt2.x - pt1.x);
    double angle = atan(x_d/y_d);
    line->theta = (float) ( x_d==0 ? 0 : angle*180/3.1415 );
    if(angle < 0)
		line->rho = (float) ( (double)pt1.x * cos(-angle) + (double)pt1.y * sin(-angle) );
    else
		line->rho = (float) ( (double)pt1.y * sin(angle) - (double)pt1.x * cos(angle) );
	
}

};  // end namespace blepo
