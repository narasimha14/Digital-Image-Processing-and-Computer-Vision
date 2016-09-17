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

#pragma warning( disable: 4786 )

#include "Image.h"
#include "ImageOperations.h"
#include "ImageAlgorithms.h"
#include "Utilities/PointSizeRect.h"
//#include "Utilities/Array.h"
#include <vector>

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

// ================> begin local functions (available only to this translation unit)
namespace
{
using namespace blepo;

template <typename T>
inline void iExpand(const Image<T>& img, Image<T>* out, std::vector<Point>* frontier, const Point& p, typename Image<T>::Pixel color, typename Image<T>::Pixel new_color)
{
  Image<T>::Pixel pix  = img(p.x, p.y);
  Image<T>::PixelRef pix2 = (*out)(p.x, p.y);
  if (pix == color && pix2 != new_color)
  {
    frontier->push_back(p);
    pix2 = new_color;
  }
}

//  FloodFill functions.
//  @author Prashant Oswal, Stan Birchfield

template <typename T>
void iFloodFill4(const Image<T>& img, int x, int y, typename Image<T>::Pixel new_color, Image<T>* out)
{
  if (x<0 || y<0 || x>=img.Width() || y>=img.Height())  BLEPO_ERROR("Out of bounds");

  out->Reset(img.Width(), img.Height());  // okay if in place b/c it won't do anything
//  if (out != &img)  *out = img;  // not in place, so copy input to output

  std::vector<Point> frontier;
  int w = img.Width()-1;
  int h = img.Height()-1;
  Point p;

  if (new_color == (*out)(x,y))  return;
  Image<T>::Pixel color = img(x, y);
  frontier.push_back( Point(x,y) );
  (*out)(x, y) = new_color;

  while ( !frontier.empty() )
  {
    p = frontier.back();
    frontier.pop_back();
    if (p.x > 0)  iExpand(img, out, &frontier, Point(p.x-1, p.y  ), color, new_color);
    if (p.x < w)  iExpand(img, out, &frontier, Point(p.x+1, p.y  ), color, new_color);
    if (p.y > 0)  iExpand(img, out, &frontier, Point(p.x,   p.y-1), color, new_color);
    if (p.y < h)  iExpand(img, out, &frontier, Point(p.x,   p.y+1), color, new_color);
  }
}

template <typename T>
void iFloodFill8(const Image<T>& img, int x, int y, typename Image<T>::Pixel new_color, Image<T>* out)
{
  if (x<0 || y<0 || x>=img.Width() || y>=img.Height())  BLEPO_ERROR("Out of bounds");

  out->Reset(img.Width(), img.Height());  // okay if in place b/c it won't do anything
//  if (out != &img)  *out = img;  // not in place, so copy input to output

  std::vector<Point> frontier;
  int w = img.Width()-1;
  int h = img.Height()-1;
  Point p;

  if (new_color == (*out)(x,y))  return;
  Image<T>::Pixel color = img(x, y);
  frontier.push_back( Point(x,y) );
  (*out)(x, y) = new_color;

  while ( !frontier.empty() )
  {
    p = frontier.back();
    frontier.pop_back();
    if (p.x > 0)  iExpand(img, out, &frontier, Point(p.x-1, p.y  ), color, new_color);
    if (p.x < w)  iExpand(img, out, &frontier, Point(p.x+1, p.y  ), color, new_color);
    if (p.y > 0)  iExpand(img, out, &frontier, Point(p.x,   p.y-1), color, new_color);
    if (p.y < h)  iExpand(img, out, &frontier, Point(p.x,   p.y+1), color, new_color);

    if (p.x > 0 && p.y > 0)  iExpand(img, out, &frontier, Point(p.x-1, p.y-1), color, new_color);
    if (p.x > 0 && p.y < h)  iExpand(img, out, &frontier, Point(p.x-1, p.y+1), color, new_color);
    if (p.x < w && p.y > 0)  iExpand(img, out, &frontier, Point(p.x+1, p.y-1), color, new_color);
    if (p.x < w && p.y < h)  iExpand(img, out, &frontier, Point(p.x+1, p.y+1), color, new_color);
  }
}

};
// ================< end local functions

namespace blepo
{

// main floodfill functions
void FloodFill4(const ImgBgr& img,    int x, int y, ImgBgr::Pixel    new_color, ImgBgr* out)    { iFloodFill4(img, x, y, new_color, out); }
void FloodFill4(const ImgBinary& img, int x, int y, ImgBinary::Pixel new_color, ImgBinary* out) { iFloodFill4(img, x, y, new_color, out); }
void FloodFill4(const ImgFloat& img,  int x, int y, ImgFloat::Pixel  new_color, ImgFloat* out)  { iFloodFill4(img, x, y, new_color, out); }
void FloodFill4(const ImgGray& img,   int x, int y, ImgGray::Pixel   new_color, ImgGray* out)   { iFloodFill4(img, x, y, new_color, out); }
void FloodFill4(const ImgInt& img,    int x, int y, ImgInt::Pixel    new_color, ImgInt* out)    { iFloodFill4(img, x, y, new_color, out); }
void FloodFill8(const ImgBgr& img,    int x, int y, ImgBgr::Pixel    new_color, ImgBgr* out)    { iFloodFill8(img, x, y, new_color, out); }
void FloodFill8(const ImgBinary& img, int x, int y, ImgBinary::Pixel new_color, ImgBinary* out) { iFloodFill8(img, x, y, new_color, out); }
void FloodFill8(const ImgFloat& img,  int x, int y, ImgFloat::Pixel  new_color, ImgFloat* out)  { iFloodFill4(img, x, y, new_color, out); }
void FloodFill8(const ImgGray& img,   int x, int y, ImgGray::Pixel   new_color, ImgGray* out)   { iFloodFill8(img, x, y, new_color, out); }
void FloodFill8(const ImgInt& img,    int x, int y, ImgInt::Pixel    new_color, ImgInt* out)    { iFloodFill8(img, x, y, new_color, out); }




/*
Fill holes in the binary image
-nkanher
*/
void FillHoles(ImgBinary* BinIn, bool use_speedup_hack)
{
  if(use_speedup_hack)
  {
    ImgInt IntIn;
    Convert(*BinIn, &IntIn);

		int search_v = 5;
		if(IntIn.Height() < search_v)
		{
			search_v = IntIn.Height();
		}
    int bg_val = -2;
    for(int u=0; u < IntIn.Width();u++)
    {
      for(int v=0; v < search_v; v++) 
      {
        if(IntIn(u, v) == 0)
        {
          FloodFill4(IntIn, u, v, bg_val, &IntIn);
        }
      }
    }
    Threshold(IntIn, bg_val+1, BinIn);
  }
  else
  {
    ImgInt In;
    std::vector<ConnectedComponentProperties<ImgInt::Pixel> > cc;
    ImgInt IntIn;
    Convert(*BinIn, &IntIn);
    int nComponents = ConnectedComponents4(IntIn, &In, &cc);
    int bg_label = nComponents+2;
    
    bool found = false;
    
    int largest_cc_index = -1;
    int npixels = -1;
    for(unsigned int i=0; i < cc.size(); i++)
    {
      if((cc[i].npixels > npixels) & (cc[i].value == 0))
      {
        npixels = cc[i].npixels;
        largest_cc_index = i;
      }
    }
    
    Set(BinIn, 0);
    ImgInt::Iterator iter_in = In.Begin();
    ImgBinary::Iterator iter_bin = BinIn->Begin();
    while(iter_in != In.End())
    {
      if(*iter_in != largest_cc_index)
      {
        *iter_bin = ImgBinary::Pixel(1);
      }
      iter_in++;
      iter_bin++;
    }
    
  } 
}

};  // end namespace blepo























/*
// ================> begin local functions (available only to this translation unit)
namespace
{
//This structure is used in the Flood Fill implementations. 
struct StackElement
{
  int x,y;
};
};
// ================< end local functions


namespace blepo
{

//  FloodFill functions.
//  @author Prashant Oswal

void FloodFill4(const ImgBinary& img, ImgBinary::Pixel new_color, int seed_x, int seed_y, ImgBinary* out)
{
  StackElement element;
  int x_curr, y_curr, width, height, index = 0;
  ImgBinary::Iterator ptr_out;
  //A table to check which pixels have been already put on the stack.
  //This increases speed tremendously!!!
  ImgBinary img_done; 
  ImgBinary::Iterator ptr_done;

  width = img.Width();
  height = img.Height();

  //Throw exception if seed lies outside the image dimensions
  if (seed_x >= width || seed_y >= height) BLEPO_ERROR("seed out of bounds");

  const ImgBinary::Pixel old_color = img(seed_x,seed_y);

  out->Reset(width,height);
  img_done.Reset(width,height);
  *out = img;
  Set(&img_done,false);

  if (old_color==new_color) return;
  
  //The stack for storing the connected pixel co-ordinates.
  std::vector<StackElement> pixel_stack;
 
  
  element.x = seed_x;
  element.y = seed_y;
  pixel_stack.push_back(element);
  index++;
  while(index>0)
  {
    index--;
    element = pixel_stack.back();
    pixel_stack.pop_back();
    x_curr = element.x;
    y_curr = element.y;
    ptr_out=out->Begin(x_curr,y_curr);
    *ptr_out = new_color;
    ptr_done = img_done.Begin(x_curr,y_curr);
    *ptr_done = true;
    if ((x_curr+1 < width) && *(ptr_out+1) == old_color && *(ptr_done+1) == false)
    {
      element.x = x_curr + 1;
      element.y = y_curr;
      pixel_stack.push_back(element);
      *(ptr_done+1) = true;
      index++;
    }
    if((x_curr - 1 >= 0) && *(ptr_out-1) ==  old_color && *(ptr_done - 1) == false)
    {
      element.x = x_curr - 1;
      element.y = y_curr;
      pixel_stack.push_back(element);
      *(ptr_done - 1) = true;
      index++;
    }
    if((y_curr + 1 < height) && *(ptr_out+width) ==  old_color && *(ptr_done + width) == false)
		{
      element.x = x_curr;
      element.y = y_curr + 1;
      pixel_stack.push_back(element);
      *(ptr_done + width) = true;
      index++;
    }
    if((y_curr - 1 >= 0) && *(ptr_out-width) ==  old_color && *(ptr_done - width) == false)
    {
      element.x = x_curr;
      element.y = y_curr - 1;
      pixel_stack.push_back(element);
      *(ptr_done - width) = true;
      index++;
    }
  }
}

void FloodFill4(const ImgGray& img, ImgGray::Pixel new_color, int seed_x, int seed_y, ImgGray* out)
{
  StackElement element;
  int x_curr, y_curr, width, height, index = 0;
  ImgGray::Iterator ptr_out;
  //A table to check which pixels have been already put on the stack.
  //This increases speed tremendously!!!
  ImgBinary img_done; 
  ImgBinary::Iterator ptr_done;

  width = img.Width();
  height = img.Height();

  //Throw exception if seed lies outside the image dimensions
  if (seed_x >= width || seed_y >= height) BLEPO_ERROR("seed out of bounds");

  const ImgGray::Pixel old_color= img(seed_x,seed_y);

  out->Reset(width,height);
  img_done.Reset(width,height);
  *out = img;
  Set(&img_done,false);

  if (old_color==new_color) return;
  
  //The stack for storing the connected pixel co-ordinates.
  std::vector<StackElement> pixel_stack;
 
  
  element.x = seed_x;
  element.y = seed_y;
  pixel_stack.push_back(element);
  index++;
  while(index>0)
  {
    index--;
    element = pixel_stack.back();
    pixel_stack.pop_back();
    x_curr = element.x;
    y_curr = element.y;
    ptr_out=out->Begin(x_curr,y_curr);
    *ptr_out = new_color;
    ptr_done = img_done.Begin(x_curr,y_curr);
    *ptr_done = true;
    if ((x_curr+1 < width) && *(ptr_out+1) == old_color && *(ptr_done+1) == false)
    {
      element.x = x_curr + 1;
      element.y = y_curr;
      pixel_stack.push_back(element);
      *(ptr_done+1) = true;
      index++;
    }
    if((x_curr - 1 >= 0) && *(ptr_out-1) ==  old_color && *(ptr_done - 1) == false)
    {
      element.x = x_curr - 1;
      element.y = y_curr;
      pixel_stack.push_back(element);
      *(ptr_done - 1) = true;
      index++;
    }
    if((y_curr + 1 < height) && *(ptr_out+width) ==  old_color && *(ptr_done + width) == false)
		{
      element.x = x_curr;
      element.y = y_curr + 1;
      pixel_stack.push_back(element);
      *(ptr_done + width) = true;
      index++;
    }
    if((y_curr - 1 >= 0) && *(ptr_out-width) ==  old_color && *(ptr_done - width) == false)
    {
      element.x = x_curr;
      element.y = y_curr - 1;
      pixel_stack.push_back(element);
      *(ptr_done - width) = true;
      index++;
    }
  }
}

void FloodFill4(const ImgBgr& img, ImgBgr::Pixel new_color, int seed_x, int seed_y, ImgBgr* out)
{
  StackElement element;
  int x_curr, y_curr, width, height, index = 0;
  ImgBgr::Iterator ptr_out;
  //A table to check which pixels have been already put on the stack.
  //This increases speed tremendously!!!
  ImgBinary img_done; 
  ImgBinary::Iterator ptr_done;

  width = img.Width();
  height = img.Height();

  //Throw exception if seed lies outside the image dimensions
  if (seed_x >= width || seed_y >= height) BLEPO_ERROR("seed out of bounds");

  const ImgBgr::Pixel old_color= img(seed_x,seed_y);

  out->Reset(width,height);
  img_done.Reset(width,height);
  *out = img;
  Set(&img_done,false);

  if (old_color==new_color) return;
  
  //The stack for storing the connected pixel co-ordinates.
  std::vector<StackElement> pixel_stack;
 
  
  element.x = seed_x;
  element.y = seed_y;
  pixel_stack.push_back(element);
  index++;
  while(index>0)
  {
    index--;
    element = pixel_stack.back();
    pixel_stack.pop_back();
    x_curr = element.x;
    y_curr = element.y;
    ptr_out=out->Begin(x_curr,y_curr);
    *ptr_out = new_color;
    ptr_done = img_done.Begin(x_curr,y_curr);
    *ptr_done = true;
    if ((x_curr+1 < width) && *(ptr_out+1) == old_color && *(ptr_done+1) == false)
    {
      element.x = x_curr + 1;
      element.y = y_curr;
      pixel_stack.push_back(element);
      *(ptr_done+1) = true;
      index++;
    }
    if((x_curr - 1 >= 0) && *(ptr_out-1) ==  old_color && *(ptr_done - 1) == false)
    {
      element.x = x_curr - 1;
      element.y = y_curr;
      pixel_stack.push_back(element);
      *(ptr_done - 1) = true;
      index++;
    }
    if((y_curr + 1 < height) && *(ptr_out+width) ==  old_color && *(ptr_done + width) == false)
		{
      element.x = x_curr;
      element.y = y_curr + 1;
      pixel_stack.push_back(element);
      *(ptr_done + width) = true;
      index++;
    }
    if((y_curr - 1 >= 0) && *(ptr_out-width) ==  old_color && *(ptr_done - width) == false)
    {
      element.x = x_curr;
      element.y = y_curr - 1;
      pixel_stack.push_back(element);
      *(ptr_done - width) = true;
      index++;
    }
  }
}

void FloodFill8(const ImgBinary& img, ImgBinary::Pixel new_color, int seed_x, int seed_y, ImgBinary* out)
{
  StackElement element;
  int x_curr, y_curr, width, height, index = 0;
  ImgBinary::Iterator ptr_out;
  //A table to check which pixels have been already put on the stack.
  //This increases speed tremendously!!!
  ImgBinary img_done; 
  ImgBinary::Iterator ptr_done;

  width = img.Width();
  height = img.Height();

  //Throw exception if seed lies outside the image dimensions
  if (seed_x >= width || seed_y >= height) BLEPO_ERROR("seed out of bounds");

  const ImgBinary::Pixel old_color = img(seed_x,seed_y);

  out->Reset(width,height);
  img_done.Reset(width,height);
  *out = img;
  Set(&img_done,false);

  if (old_color==new_color) return;
  
  //The stack for storing the connected pixel co-ordinates.
  std::vector<StackElement> pixel_stack;
 
  
  element.x = seed_x;
  element.y = seed_y;
  pixel_stack.push_back(element);
  index++;
  while(index>0)
  {
    index--;
    element = pixel_stack.back();
    pixel_stack.pop_back();
    x_curr = element.x;
    y_curr = element.y;
    ptr_out=out->Begin(x_curr,y_curr);
    *ptr_out = new_color;
    ptr_done = img_done.Begin(x_curr,y_curr);
    *ptr_done = true;
    if ((x_curr+1 < width) && *(ptr_out+1) == old_color && *(ptr_done+1) == false)
    {
      element.x = x_curr + 1;
      element.y = y_curr;
      pixel_stack.push_back(element);
      *(ptr_done+1) = true;
      index++;
    }
    if((x_curr - 1 >= 0) && *(ptr_out-1) ==  old_color && *(ptr_done - 1) == false)
    {
      element.x = x_curr - 1;
      element.y = y_curr;
      pixel_stack.push_back(element);
      *(ptr_done - 1) = true;
      index++;
    }
    if((y_curr + 1 < height) && *(ptr_out+width) ==  old_color && *(ptr_done + width) == false)
		{
      element.x = x_curr;
      element.y = y_curr + 1;
      pixel_stack.push_back(element);
      *(ptr_done + width) = true;
      index++;
    }
    if((y_curr - 1 >= 0) && *(ptr_out-width) ==  old_color && *(ptr_done - width) == false)
    {
      element.x = x_curr;
      element.y = y_curr - 1;
      pixel_stack.push_back(element);
      *(ptr_done - width) = true;
      index++;
    }
    if((x_curr + 1 < width) && (y_curr + 1 < height) && *(ptr_out+1+width) ==  old_color && *(ptr_done + 1 + width) == false)
    {
      element.x = x_curr + 1;
      element.y = y_curr + 1;
      pixel_stack.push_back(element);
      *(ptr_done + 1 + width) = true;
      index++;
    }
    if((x_curr + 1 < width) && (y_curr - 1 >= 0) && *(ptr_out+1-width) ==  old_color && *(ptr_done + 1 - width) == false)
    {
      element.x = x_curr + 1;
      element.y = y_curr - 1;
      pixel_stack.push_back(element);
      *(ptr_done + 1 - width) = true;
      index++;
    }
    if((x_curr - 1 >= 0) && (y_curr + 1 < height) && *(ptr_out-1+width) ==  old_color && *(ptr_done - 1 + width) == false)
    {
      element.x = x_curr - 1;
      element.y = y_curr + 1;
      pixel_stack.push_back(element);
      *(ptr_done - 1 + width) = true;
      index++;
    }
    if((x_curr - 1 >= 0) && (y_curr - 1 >=0) && *(ptr_out-1-width) ==  old_color && *(ptr_done - 1 - width) == false)
    {
      element.x = x_curr - 1;
      element.y = y_curr - 1;
      pixel_stack.push_back(element);
      *(ptr_done - 1 - width) = true;
      index++;
    }
  }
}

void FloodFill8(const ImgGray& img, ImgGray::Pixel new_color, int seed_x, int seed_y, ImgGray* out)
{
  StackElement element;
  int x_curr, y_curr, width, height, index = 0;
  ImgGray::Iterator ptr_out;
  //A table to check which pixels have been already put on the stack.
  //This increases speed tremendously!!!
  ImgBinary img_done; 
  ImgBinary::Iterator ptr_done;

  width = img.Width();
  height = img.Height();

  //Throw exception if seed lies outside the image dimensions
  if (seed_x >= width || seed_y >= height) BLEPO_ERROR("seed out of bounds");

  const ImgGray::Pixel old_color = img(seed_x,seed_y);

  out->Reset(width,height);
  img_done.Reset(width,height);
  *out = img;
  Set(&img_done,false);

  if (old_color==new_color) return;
  
  //The stack for storing the connected pixel co-ordinates.
  std::vector<StackElement> pixel_stack;
 
  
  element.x = seed_x;
  element.y = seed_y;
  pixel_stack.push_back(element);
  index++;
  while(index>0)
  {
    index--;
    element = pixel_stack.back();
    pixel_stack.pop_back();
    x_curr = element.x;
    y_curr = element.y;
    ptr_out=out->Begin(x_curr,y_curr);
    *ptr_out = new_color;
    ptr_done = img_done.Begin(x_curr,y_curr);
    *ptr_done = true;
    if ((x_curr+1 < width) && *(ptr_out+1) == old_color && *(ptr_done+1) == false)
    {
      element.x = x_curr + 1;
      element.y = y_curr;
      pixel_stack.push_back(element);
      *(ptr_done+1) = true;
      index++;
    }
    if((x_curr - 1 >= 0) && *(ptr_out-1) ==  old_color && *(ptr_done - 1) == false)
    {
      element.x = x_curr - 1;
      element.y = y_curr;
      pixel_stack.push_back(element);
      *(ptr_done - 1) = true;
      index++;
    }
    if((y_curr + 1 < height) && *(ptr_out+width) ==  old_color && *(ptr_done + width) == false)
		{
      element.x = x_curr;
      element.y = y_curr + 1;
      pixel_stack.push_back(element);
      *(ptr_done + width) = true;
      index++;
    }
    if((y_curr - 1 >= 0) && *(ptr_out-width) ==  old_color && *(ptr_done - width) == false)
    {
      element.x = x_curr;
      element.y = y_curr - 1;
      pixel_stack.push_back(element);
      *(ptr_done - width) = true;
      index++;
    }
    if((x_curr + 1 < width) && (y_curr + 1 < height) && *(ptr_out+1+width) ==  old_color && *(ptr_done + 1 + width) == false)
    {
      element.x = x_curr + 1;
      element.y = y_curr + 1;
      pixel_stack.push_back(element);
      *(ptr_done + 1 + width) = true;
      index++;
    }
    if((x_curr + 1 < width) && (y_curr - 1 >= 0) && *(ptr_out+1-width) ==  old_color && *(ptr_done + 1 - width) == false)
    {
      element.x = x_curr + 1;
      element.y = y_curr - 1;
      pixel_stack.push_back(element);
      *(ptr_done + 1 - width) = true;
      index++;
    }
    if((x_curr - 1 >= 0) && (y_curr + 1 < height) && *(ptr_out-1+width) ==  old_color && *(ptr_done - 1 + width) == false)
    {
      element.x = x_curr - 1;
      element.y = y_curr + 1;
      pixel_stack.push_back(element);
      *(ptr_done - 1 + width) = true;
      index++;
    }
    if((x_curr - 1 >= 0) && (y_curr - 1 >=0) && *(ptr_out-1-width) ==  old_color && *(ptr_done - 1 - width) == false)
    {
      element.x = x_curr - 1;
      element.y = y_curr - 1;
      pixel_stack.push_back(element);
      *(ptr_done - 1 - width) = true;
      index++;
    }
  }
}

void FloodFill8(const ImgBgr& img, ImgBgr::Pixel new_color, int seed_x, int seed_y, ImgBgr* out)
{
  StackElement element;
  int x_curr, y_curr, width, height, index = 0;
  ImgBgr::Iterator ptr_out;
  //A table to check which pixels have been already put on the stack.
  //This increases speed tremendously!!!
  ImgBinary img_done; 
  ImgBinary::Iterator ptr_done;

  width = img.Width();
  height = img.Height();

  //Throw exception if seed lies outside the image dimensions
  if (seed_x >= width || seed_y >= height) BLEPO_ERROR("seed out of bounds");

  const ImgBgr::Pixel old_color = img(seed_x,seed_y);

  out->Reset(width,height);
  img_done.Reset(width,height);
  *out = img;
  Set(&img_done,false);

  if (old_color==new_color) return;
  
  //The stack for storing the connected pixel co-ordinates.
  std::vector<StackElement> pixel_stack;
 
  
  element.x = seed_x;
  element.y = seed_y;
  pixel_stack.push_back(element);
  index++;
  while(index>0)
  {
    index--;
    element = pixel_stack.back();
    pixel_stack.pop_back();
    x_curr = element.x;
    y_curr = element.y;
    ptr_out=out->Begin(x_curr,y_curr);
    *ptr_out = new_color;
    ptr_done = img_done.Begin(x_curr,y_curr);
    *ptr_done = true;
    if ((x_curr+1 < width) && *(ptr_out+1) == old_color && *(ptr_done+1) == false)
    {
      element.x = x_curr + 1;
      element.y = y_curr;
      pixel_stack.push_back(element);
      *(ptr_done+1) = true;
      index++;
    }
    if((x_curr - 1 >= 0) && *(ptr_out-1) ==  old_color && *(ptr_done - 1) == false)
    {
      element.x = x_curr - 1;
      element.y = y_curr;
      pixel_stack.push_back(element);
      *(ptr_done - 1) = true;
      index++;
    }
    if((y_curr + 1 < height) && *(ptr_out+width) ==  old_color && *(ptr_done + width) == false)
		{
      element.x = x_curr;
      element.y = y_curr + 1;
      pixel_stack.push_back(element);
      *(ptr_done + width) = true;
      index++;
    }
    if((y_curr - 1 >= 0) && *(ptr_out-width) ==  old_color && *(ptr_done - width) == false)
    {
      element.x = x_curr;
      element.y = y_curr - 1;
      pixel_stack.push_back(element);
      *(ptr_done - width) = true;
      index++;
    }
    if((x_curr + 1 < width) && (y_curr + 1 < height) && *(ptr_out+1+width) ==  old_color && *(ptr_done + 1 + width) == false)
    {
      element.x = x_curr + 1;
      element.y = y_curr + 1;
      pixel_stack.push_back(element);
      *(ptr_done + 1 + width) = true;
      index++;
    }
    if((x_curr + 1 < width) && (y_curr - 1 >= 0) && *(ptr_out+1-width) ==  old_color && *(ptr_done + 1 - width) == false)
    {
      element.x = x_curr + 1;
      element.y = y_curr - 1;
      pixel_stack.push_back(element);
      *(ptr_done + 1 - width) = true;
      index++;
    }
    if((x_curr - 1 >= 0) && (y_curr + 1 < height) && *(ptr_out-1+width) ==  old_color && *(ptr_done - 1 + width) == false)
    {
      element.x = x_curr - 1;
      element.y = y_curr + 1;
      pixel_stack.push_back(element);
      *(ptr_done - 1 + width) = true;
      index++;
    }
    if((x_curr - 1 >= 0) && (y_curr - 1 >=0) && *(ptr_out-1-width) ==  old_color && *(ptr_done - 1 - width) == false)
    {
      element.x = x_curr - 1;
      element.y = y_curr - 1;
      pixel_stack.push_back(element);
      *(ptr_done - 1 - width) = true;
      index++;
    }
  }
}

};  // end namespace blepo
*/