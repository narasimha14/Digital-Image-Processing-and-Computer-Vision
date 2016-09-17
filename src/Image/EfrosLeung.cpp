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
//#include "BasicImageOperations.h"
//#include "Utilities/Array.h"
#include "Utilities/Math.h"
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

//Struct for storing number of neighbors
struct PixelInfo
{
	int xc,yc,totalnbr;
};

std::vector<PixelInfo> g_pixel; 

// Struct for storing SSD and Pixel value
struct PixSSD
{
	int pixval;
	float ssd;
};

//Function to return pixel value of best match.
int BestMatch(const ImgGray& img,ImgGray* p,int window_width,int window_height,int xcord,int ycord,ImgBinary &img_bin)
{
  int height=img.Height();
  int width=img.Width();
  int window_width_f=static_cast<int>(blepo_ex::Floor(window_width/2.0f));
  int window_width_c=static_cast<int>(blepo_ex::Ceil(window_width/2.0f));
  int window_height_f=static_cast<int>(blepo_ex::Floor(window_height/2.0f));
  int window_height_c=static_cast<int>(blepo_ex::Ceil(window_height/2.0f));
  static float sum;
  double minsum=1000000.0;
  int num;
  std::vector<PixSSD> p_ssd,p_ssd_sel;


  // Compute SSD
  int i;
  for(i=window_width_f;i<(width-window_width_c);i++)
  {
    for(int j=window_height_f;j<(height-window_height_c);j++)
    {
      sum=0;
      for(int x=-window_width_f;x<window_width_c;x++)
      {
        for(int y=-window_height_f;y<window_height_c;y++)
        {
          if(img_bin(xcord+x,ycord+y)==true)
          {
            sum += (float)(pow( static_cast<float>( img(i+x,j+y)-(*p)(x + window_width_f,y+window_height_f) ), 2.0f));
          }
        }
      }
      if((int)sum <= (int)minsum)
      {
        minsum=sum;
      }
      struct PixSSD tmp;
      tmp.ssd=sum;
      tmp.pixval=img(i,j);
      p_ssd.push_back(tmp);
    }
  }

  // Consider all matches with SSD value less than 1.1 and randomly choose one
  for(i=0 ; i<(int)p_ssd.size() ; i++)
  {
    if (p_ssd[i].ssd<=(1.1)*minsum)
    p_ssd_sel.push_back(p_ssd[i]);
  }

  if(p_ssd_sel.size()==0)
	return 0;
  else
  num=blepo_ex::GetRand(0,p_ssd_sel.size());
  return p_ssd_sel[num].pixval;
}

// Function to sort the list of neighborhood pixels in descending order//
int Compare(const void* pixel1,const void* pixel2)
{
  struct PixelInfo* p1=(struct PixelInfo*)pixel1;
  struct PixelInfo* p2=(struct PixelInfo*)pixel2;
  return ((p1->totalnbr)-(p2->totalnbr));	
}

};
// ================< end local functions

namespace blepo
{

//Function to compute texture from sample 
void SynthesizeTextureEfrosLeung(const ImgGray& texture, const ImgBinary &mask, int window_width, int window_height,int out_width, int out_height, ImgGray* out)
{
  int height=texture.Height();
  int width=texture.Width();
  bool img_filled=false;
  int win_wid_c=static_cast<int>(blepo_ex::Ceil(window_width/2.0f));
  int win_wid_f=static_cast<int>(blepo_ex::Floor(window_width/2.0f));
  int win_ht_c=static_cast<int>(blepo_ex::Ceil(window_height/2.0f));
  int win_ht_f=static_cast<int>(blepo_ex::Floor(window_height/2.0f));
  int i,j,m,n,k;
  static bool hasnbr=false;
  static int nbr=0;
  ImgGray templates;
  templates.Reset(window_width,window_height);
  ImgBinary img_bin;
  img_bin = mask;

  //Find neighbors
  while(1)
  {
    for(i=win_wid_c;i<(out_width-win_wid_c);i++)
    {
      for(j=win_ht_c;j<(out_height-win_ht_c);j++)
      {
        hasnbr=false;
        nbr=0;
        if(img_bin(i,j)==false)
        {
          for (m=-win_wid_f;m<win_wid_c;m++)
          {
            for (n=-win_ht_f;n<win_ht_c;n++)
            {
              if(img_bin(i+m,j+n)==true)
              {
                nbr=nbr+1;
                hasnbr=true;
              }
            }
          }
        }
        if(hasnbr==true)
        {
          struct PixelInfo pix;
          pix.xc=i;
          pix.yc=j;
          pix.totalnbr=nbr;
          g_pixel.push_back(pix);
        }
      }
    }

    if(g_pixel.size()==0)
    {
      break;
    }
    //Sort list of neighbors
    qsort(&g_pixel[0],g_pixel.size(),sizeof(g_pixel[0]),Compare);


    //Compute best match for window around given pixel
    while(g_pixel.size()!=0)
    {
      struct PixelInfo temp;
      temp=g_pixel.back();  
      g_pixel.pop_back();
      for (i=0;i<window_width;i++)
        for(j=0;j<window_height;j++)
          templates(i,j)=(*out)(temp.xc+i-win_wid_f,temp.yc+j-win_ht_f);
      k=BestMatch(texture,&templates,window_width,window_height,temp.xc,temp.yc,img_bin);
      (*out)(temp.xc,temp.yc)=k;
      img_bin(temp.xc,temp.yc)=true;
    }
  }
}

};  // end namespace blepo
