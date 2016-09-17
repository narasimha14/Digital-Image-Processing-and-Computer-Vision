/* 
 * Copyright (c) 2006 Clemson University.
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

#include "Image.h"
#include <vector>

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

namespace blepo {

void WallFollow(const ImgInt& img, int label, std::vector<Point> *chain)
{  
  enum direction_t {UP, LEFT, DOWN, RIGHT};
  direction_t direction = LEFT;
  bool found = false;
  int start_x, start_y, x, y;

  for (x=0; x < img.Width(); x++)
  {
    if (found)  break;
    for (y=0; y < img.Height(); y++)
    {
      if (img(x,y) == label)
      {
        start_x = x;
        start_y = y;
        found = true;
        break;
      }
    }
  }

  if (!found)  return;

  chain->clear();
  x = start_x;
  y = start_y;
  bool valid = true;

  do
  {        
    chain->push_back(Point(x,y));
    if (direction == LEFT)
    {
      if (img(x, y-1) == label)
      {
        y--;
        direction = UP;
      }
      else if (img(x-1, y) == label)
      {
        x--;
        direction = LEFT;
      }
      else if (img(x, y+1) == label)
      {
        y++;
        direction = DOWN;
      }
      else if (img(x+1, y) == label)
      {
        x++;
        direction = RIGHT;
      }
    } 
    else if (direction == DOWN)
    {
      if (img(x-1, y) == label)
      {
        x--;
        direction = LEFT;
      }
      else if (img(x, y+1) == label)
      {
        y++;
        direction = DOWN;
      }
      else if (img(x+1, y) == label)
      {
        x++;
        direction = RIGHT;
      }
      else if (img(x, y-1) == label)
      {
        y--;
        direction = UP;
      }
    } 
    else if (direction == RIGHT)
    {
      if (img(x, y+1) == label)
      {
        y++;
        direction = DOWN;
      }
      else if (img(x+1, y) == label)
      {
        x++;
        direction = RIGHT;
      }
      else if (img(x, y-1) == label)
      {
        y--;
        direction = UP;
      }
      else if (img(x-1, y) == label)
      {
        x--;
        direction = LEFT;
      }
    } 
    else if (direction == UP)
    {
      if (img(x+1, y) == label)
      {
        x++;
        direction = RIGHT;
      }
      else if (img(x, y-1) == label)
      {
        y--;
        direction = UP;
      }
      else if (img(x-1, y) == label)
      {
        x--;
        direction = LEFT;
      }
      else if (img(x, y+1) == label)
      {
        y++;
        direction = DOWN;
      }
    }
  
  } while (x != start_x || y != start_y);
}

};  // end namespace blepo

