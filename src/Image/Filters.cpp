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

#include "Image.h"

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

// Median filters written by Prashanth Y. Govindaraju and Ramakrishnan Ravindran, 2005

// Median Filter for gray scale images
void MedianFilter(const ImgGray& img,  const int kernel_width,const int kernel_height, ImgGray* out)
{
	int count=0;
	out->Reset(img.Width(), img.Height());            //Reset output image to same size as input
	const int height=img.Height();
	const int width=img.Width();
	ImgInt median_array;
	median_array.Reset(kernel_width*kernel_height,1); //Temprary array used for sorting		
	int min,median_value,temp;
	int x_cor;
	int y_cor;
	int k;
	for(int i=0;i<width-1;i++)
		for(int j=0;j<height-1;j++)
		{
			
			count=0;
			for(int x=-(kernel_width)/2;x<=(kernel_width)/2;x++)	//Collecting values within the kernel
				for(int y=-(kernel_height)/2;y<=(kernel_height)/2;y++)
				{
					
					x_cor=i+x;
					y_cor=j+y;
					if((x_cor)<0) x_cor=0;
					if((y_cor)<0) y_cor=0;
					if(x_cor>width-1) x_cor=width-2;
					if(y_cor>height-1) y_cor=height-2;
					median_array(count,0)=img(x_cor,y_cor);
					count++;
				}
				min=median_array(0,0);
				k=0;
				while(k<count-1)	//Sorting the kernel pixel values
				{
					if(median_array(k,0)> median_array(k+1,0))
					{
						temp=median_array(k,0);
						median_array(k,0)=median_array(k+1,0);
						median_array(k+1,0)=temp;
						k=-1;
					}
					k++;
					
				}
				median_value=median_array((count+1)/2,0);	//Finding the Median
				*(out->Begin(i,j))=median_value;
		}
}

// Median filter for integer images

void MedianFilter(const ImgInt& img,  const int kernel_width,const int kernel_height, ImgInt* out)
{
	int count=0;
	out->Reset(img.Width(), img.Height());              //Reset output image to same size as input
	const int height=img.Height();
	const int width=img.Width();
	ImgInt median_array;
    median_array.Reset(kernel_width*kernel_height,1);  //Temprary array used for sorting
	int min,median_value,temp;
	int x_cor;
	int y_cor;
	int k;
	for(int i=0;i<width-1;i++)
		for(int j=0;j<height-1;j++)
		{
			
			count=0;
			for(int x=-kernel_width/2;x<=kernel_width/2;x++)	//Collecting values within the kernel
				for(int y=-kernel_height/2;y<=kernel_height/2;y++)
				{
					
					x_cor=i+x;
					y_cor=j+y;
					if((x_cor)<0) x_cor=0;
					if((y_cor)<0) y_cor=0;
					if(x_cor>width-1) x_cor=width-2;
					if(y_cor>height-1) y_cor=height-2;
					median_array(count,0)=img(x_cor,y_cor);
					count++;
				}
				min=median_array(0,0);
				k=0;
				while(k<count-1)	//Sorting the kernel pixel values
				{
					if(median_array(k,0)> median_array(k+1,0))
					{
						temp=median_array(k,0);
						median_array(k,0)=median_array(k+1,0);
						median_array(k+1,0)=temp;
						k=-1;
					}
					k++;
					
				}
				median_value=median_array((count+1)/2,0);	//Finding the Median
				*(out->Begin(i,j))=median_value;
		}
}

};  // end namespace blepo

