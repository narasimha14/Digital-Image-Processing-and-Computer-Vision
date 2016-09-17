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

//#include "ActiveContour.h"
//#include <math.h>  // sqrtf
#include "ImageAlgorithms.h"
#include "ImageOperations.h"
//#include "Figure/Figure.h"


// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------


namespace blepo
{

float ExternalEnergy(const ImgGray& gradmag, const Point& pt)
{
  return (float) -gradmag(pt.x, pt.y);
}

float InternalEnergy(const Point& pt1, const Point& pt2, float alpha)
{
  float diffx = (float) (pt1.x-pt2.x);
  float diffy = (float) (pt1.y-pt2.y);
  return alpha * (diffx*diffx + diffy*diffy);
}

Point OffsetFromRow(int row)
{
  int dx = (row / 3) - 1;
  int dy = (row % 3) - 1;
  return Point(dx, dy);
}

bool iSnakeIterationBare(const ImgGray& img, float alpha, Snake* points_ptr, bool fix_first_point)
{
  typedef ImgFloat Array2D;

  ImgGray gradmag;
  GradMagPrewitt(img, &gradmag);
//  static bool firstt = true;
//  if (firstt)
//  {
//    Figure fig;
//    fig.Draw(gradmag);
//    firstt = false;
//  }
  Snake& snake = *points_ptr;
  const int m = 9;  // no. of search locations for each point, 9 for +/- 1 in x and y
  const int n = snake.size();
  Array2D table_energy(n, m), table_parent(n, m);
  int c, r;
  bool snake_moved = false;

  // Fill up table
  for (c=0 ; c<n ; c++)
  {
    for (r=0 ; r<m ; r++)
    {
      int parent = 0;
      const Point& offset = OffsetFromRow(r);
      Point point2 = snake[c] + offset;
      float energy = (c==0 && fix_first_point && (offset.x!=0 || offset.y!=0)) ? 9999999 :  ExternalEnergy(gradmag, point2);
if (c<=2)
{
  TRACE("(%2d,%2d): %7.1f\n", c, r, energy);
}
      if (c > 0)
      {
        // Add internal energy
        float vmin = 0;
        for (int rp=0 ; rp<m ; rp++)
        {
          Point point1 = snake[c-1] + OffsetFromRow(rp);
          float v = table_energy(c-1, rp) + InternalEnergy(point1, point2, alpha);
if (c<=2)
{
  TRACE("           %7.1f\n", v);
}
          if (c == n-1)
          {
            // Add internal energy between first and last point
            // (This code is more general than it needs to be, because all paths
            //  will trace back to the same cell in the first column; i.e., 'row'
            //  (and hence 'point1') will be the same for all 'r' (and hence 'point2'))
            int row = rp;
            for (int k=c-1 ; k>0 ; k--)
            {
              row = (int) table_parent(k, row);
            }
            point1 = snake[0] + OffsetFromRow(row);
            v += InternalEnergy(point1, point2, alpha);
          }
if (c<=2)
{
  TRACE("       %2d: (%2d,%2d) %7.1f\n", rp, OffsetFromRow(rp).x, OffsetFromRow(rp).y, v);
}
          if (rp==0 || (v < vmin))
          {
            vmin = v;
            parent = rp;
          }
        }
        energy += vmin;
      }
      table_energy(c,r) = (float) energy;
      table_parent(c,r) = (float) parent;
if (c<=2)
{
  TRACE("(%2d,%2d) = %7.1f, %3d\n", c, r, energy, parent);
}
    }
  }

  // Traverse table backwards to get best path, yielding the new snake
  float vmin = 0;
  int row = 0;
  for (r=0 ; r<m ; r++)
  {
    float v = table_energy(c-1, r);
    if (r==0 || v < vmin)
    {
      vmin = v;
      row = r;
    }
  }
  for (c=n-1 ; c>=0 ; c--)
  {
    const Point& offset = OffsetFromRow(row);
    snake[c] += offset;
    snake_moved = snake_moved || offset.x!= 0 || offset.y != 0;
    row = (int) table_parent(c, row);
  }

  return snake_moved;
}

bool SnakeIteration(const ImgGray& img, float alpha, Snake* points_ptr)
{
  Snake& snake = *points_ptr;
  const int n = snake.size();
  int n2 = n / 2;
  int i;

  // run snake algorithm
  Snake snake2 = snake;
  iSnakeIterationBare(img, alpha, &snake2, false);

  // select the answer for the middle point, and change the point order so that
  // the middle point becomes the first point
  Point foo = snake2[n2];
  snake2.clear();
  snake2.push_back(foo);
  for (i=n2+1 ; i<n ; i++)  snake2.push_back(snake[i]);
  for (i=0 ; i<n2 ; i++)  snake2.push_back(snake[i]);

  // run snake algorithm again
  bool snake_moved = iSnakeIterationBare(img, alpha, &snake2, true);

  // Change the point order back to the original order
  for (i=n2 ; i<n ; i++)  snake[i] = snake2[i-n2];
  for (i=0 ; i<n2 ; i++)  snake[i] = snake2[i+n-n2];

  return snake_moved;
}


//////////////////////////////////////////////////////////////////
// beta below



float InternalEnergy(const Point& pt0, const Point& pt1, const Point& pt2, float beta)
{
  float diffx = (float) (pt0.x - 2*pt1.x + pt2.x);
  float diffy = (float) (pt0.y - 2*pt1.y + pt2.y);
  return beta * (diffx*diffx + diffy*diffy);
}

Point Offset0FromRow(int row)
{
  int r = row / 9;
  int dx = (r / 3) - 1;
  int dy = (r % 3) - 1;
  return Point(dx, dy);
}

Point Offset1FromRow(int row)
{
  int r = row % 9;
  int dx = (r / 3) - 1;
  int dy = (r % 3) - 1;
  return Point(dx, dy);
}

bool iSnakeIterationBare(const ImgGray& img, float alpha, float beta, Snake* points_ptr, bool fix_first_point)
{
  typedef ImgFloat Array2D;

  ImgGray gradmag;
  GradMagPrewitt(img, &gradmag);
  Snake& snake = *points_ptr;
  const int m = 9*9;  // no. of search locations for each point, 9 for +/- 1 in x and y
  const int n = snake.size();
  Array2D table_energy(n, m), table_parent(n, m);
  int c, r;
  bool snake_moved = false;

  // Fill up table
  for (c=0 ; c<n ; c++)
  {
    for (r=0 ; r<m ; r++)
    {
      int parent = 0;
      const Point& offset = Offset1FromRow(r);
      Point point2 = snake[c] + offset;
      float energy = (c==0 && fix_first_point && (offset.x!=0 || offset.y!=0)) ? 9999999 :  ExternalEnergy(gradmag, point2);
//if (c<=2)
//{
//  TRACE("(%2d,%2d): %7.1f\n", c, r, energy);
//}
      if (c > 0)
      {
        // Add internal energy
        float vmin = -9999999;
        for (int rp=0 ; rp<m ; rp++)
        {
//          Point oo0 = Offset0FromRow(rp);
//          Point oo1 = Offset1FromRow(rp);
//          TRACE("oo %2d:  (%2d,%2d)  (%2d,%2d)\n", rp, oo0.x, oo0.y, oo1.x, oo1.y);

          if (Offset0FromRow(r) != Offset1FromRow(rp))  continue;

          Point point1 = snake[c-1] + Offset1FromRow(rp);
          float v = table_energy(c-1, rp) + InternalEnergy(point1, point2, alpha);
//if (c<=2)
//{
//  TRACE("           %7.1f\n", v);
//}
          if (c > 1)
          {
            Point point0 = snake[c-2] + Offset0FromRow(rp);
            v += InternalEnergy(point0, point1, point2, beta);
          }
          if (c == n-1)
          {
            // Add internal energy between first and last points
            // (This code is more general than it needs to be, because all paths
            //  will trace back to the same cell in the first column; i.e., 'row'
            //  (and hence 'point1') will be the same for all 'r' (and hence 'point2'))
            int row = rp;
            for (int k=c-1 ; k>1 ; k--)
            {
              row = (int) table_parent(k, row);
            }
            Point point4 = snake[1] + Offset1FromRow(row);
            Point point3 = snake[0] + Offset0FromRow(row);
            {
              int foorow = (int) table_parent(1, row);
              assert(Offset1FromRow(foorow) == Offset0FromRow(row));
            }
            v += InternalEnergy(point2, point3, alpha);
            v += InternalEnergy(point1, point2, point3, beta);
            v += InternalEnergy(point2, point3, point4, beta);
          }
          if (c == n-1)
          {
            // Add internal energy between first and last point
            // (This code is more general than it needs to be, because all paths
            //  will trace back to the same cell in the first column; i.e., 'row'
            //  (and hence 'point1') will be the same for all 'r' (and hence 'point2'))
            int row = rp;
            for (int k=c-1 ; k>0 ; k--)
            {
              row = (int) table_parent(k, row);
            }
            point1 = snake[0] + OffsetFromRow(row);
            v += InternalEnergy(point1, point2, alpha);
          }
//if (c<=2)
//{
//  TRACE("       %2d: (%2d,%2d) %7.1f\n", rp, Offset1FromRow(rp).x, Offset1FromRow(rp).y, v);
//}
          if (vmin==-9999999 || (v < vmin))
          {
            vmin = v;
            parent = rp;
          }
        }
        energy += vmin;
      }
      table_energy(c,r) = (float) energy;
      table_parent(c,r) = (float) parent;
//if (c<=2)
//{
//  TRACE("(%2d,%2d) = %7.1f, %3d\n", c, r, energy, parent);
//}
    }
  }
//  Figure fig;
//  fig.Draw(table_energy);

  // Traverse table backwards to get best path, yielding the new snake
  float vmin = 0;
  int row = 0;
  for (r=0 ; r<m ; r++)
  {
    float v = table_energy(c-1, r);
    if (r==0 || v < vmin)
    {
      vmin = v;
      row = r;
    }
  }
  for (c=n-1 ; c>=0 ; c--)
  {
    const Point& offset = Offset1FromRow(row);
    snake[c] += offset;
    snake_moved = snake_moved || offset.x!= 0 || offset.y != 0;
    row = (int) table_parent(c, row);
  }

  return snake_moved;
}

/**
  SnakeIteration:  One iteration of the snake minimization algorithm by 
  Amini et al.
  @author Stan Birchfield (STB)
*/

bool SnakeIteration(const ImgGray& img, float alpha, float beta, Snake* points_ptr)
{
  Snake& snake = *points_ptr;
  const int n = snake.size();
  int n2 = n / 2;
  int i;

  // run snake algorithm
  Snake snake2 = snake;
  iSnakeIterationBare(img, alpha, beta, &snake2, false);
//return 0;  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!



  // select the answer for the middle point, and change the point order so that
  // the middle point becomes the first point
  Point foo = snake2[n2];
  snake2.clear();
  snake2.push_back(foo);
  for (i=n2+1 ; i<n ; i++)  snake2.push_back(snake[i]);
  for (i=0 ; i<n2 ; i++)  snake2.push_back(snake[i]);

  // run snake algorithm again
  bool snake_moved = iSnakeIterationBare(img, alpha, beta, &snake2, true);

  // Change the point order back to the original order
  for (i=n2 ; i<n ; i++)  snake[i] = snake2[i-n2];
  for (i=0 ; i<n2 ; i++)  snake[i] = snake2[i+n-n2];

  return snake_moved;
}

};  // end namespace blepo

