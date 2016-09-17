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
#include <vector>
#include "Image/ImageAlgorithms.h"  // ColorHistogramx

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

using namespace std;

namespace blepo
{

void HistogramGray(const ImgGray& img,const int bin, vector<int>* out)
{
	
	(*out).resize(bin);  
	const int img_height=img.Height();
	const int img_width=img.Width();
	int temp;
	int bin_width;
	bin_width=(int)256/bin;
	
	for(int i=0;i<img_width;i++)
	{
		for(int j=0;j<img_height;j++)
		{
			temp=(int)img(i,j)/bin_width;
			if((img(i,j)%bin_width)!=0 && img(i,j)>bin_width) 
			{
				temp++;
			}
			if(temp>bin-1) 
			{
				temp=bin-1;
			}
			(*out).at(temp)=(*out).at(temp)+1;
			
		}
	}
		
}


			
void HistogramBinary(const ImgGray& img,int* white, int* black)
{
	const int img_height=img.Height();
	const int img_width=img.Width();
	(*white)=0;(*black)=0;
	for(int i=0;i<img_width;i++)
	{
		for(int j=0;j<img_height;j++)
		{
	
			if(img(i,j)==255)
			{
				(*white)++;
			}
			else if (img(i,j)==0)
			{
				(*black)++;
			}
		}
	}
		
}
			


void ConservativeSmoothing(const ImgGray& img,  const int win_width,const int win_height, ImgGray* out)
{
	out->Reset(img.Width(), img.Height());  
	const int img_height=img.Height();
	const int img_width=img.Width();
		
	int x_len,y_len;
	int min,max;
	int i,j;
	for (i=0;i<img_width;i++)
	{
		for (j=0;j<img_height;j++)
		{
			*(out->Begin(i,j)) = *(img.Begin(i,j));
			
		}
	}
	
	
	for(i=0;i<img_width-1;i++)
	{
		for(j=0;j<img_height-1;j++)
		{
		min=999;
		max=0;
		for(int x=-win_width/2;x<=win_width/2;x++)	
		{
			for(int y=-win_height/2;y<=win_height/2;y++)
				{
					
					x_len=i+x;y_len=j+y;
					if((x_len)<0)
					{
						x_len=0;
					}
					if((y_len)<0)
					{
						y_len=0;
					}
					if(x_len>img_width-1)
					{
						x_len=img_width-2;
					}
					if(y_len>img_height-1)
					{
						y_len=img_height-2;
					}
					//Find the minimum and the maximum value in the window
					if(img(x_len,y_len)<min && x_len!=i && y_len!=j)
					{
						min=img(x_len,y_len);
					}
					if(img(x_len,y_len)>max && x_len!=i && y_len!=j)
					{
						max=img(x_len,y_len);
					}

					
					}
				     if(img(i,j)<min)//If the centre pixel is less than minimum in the window,set it to min.
					 {
						 *(out->Begin(i,j))=min;
					 }
					if(img(i,j)>max)//If the centre pixel is greater than maximum in the window, set it to max.
					{
						*(out->Begin(i,j))=max;
					}
		}
		}
	}
}

void ConservativeSmoothing(const ImgFloat& img,  const int win_width,const int win_height, ImgFloat* out)
{
	out->Reset(img.Width(), img.Height());  
	const int img_height=img.Height();
	const int img_width=img.Width();
		
	int x_len,y_len;
	int min,max;
	int i,j;
	for (i=0;i<img_width;i++)
	{
		for (j=0;j<img_height;j++)
		{
			*(out->Begin(i,j)) = *(img.Begin(i,j));
			
		}
	}
	
	
	for(i=0;i<img_width-1;i++)
	{
		for(j=0;j<img_height-1;j++)
		{
		min=999;
		max=0;
		for(int x=-win_width/2;x<=win_width/2;x++)	
		{
			for(int y=-win_height/2;y<=win_height/2;y++)
				{
					
					x_len=i+x;y_len=j+y;
					if((x_len)<0)
					{
						x_len=0;
					}
					if((y_len)<0)
					{
						y_len=0;
					}
					if(x_len>img_width-1)
					{
						x_len=img_width-2;
					}
					if(y_len>img_height-1)
					{
						y_len=img_height-2;
					}
					//Find the minimum and the maximum value in the window
					if(img(x_len,y_len)<min && x_len!=i && y_len!=j)
					{
						min=(int)img(x_len,y_len);
					}
					if(img(x_len,y_len)>max && x_len!=i && y_len!=j)
					{
						max=(int)img(x_len,y_len);
					}

					
					}
				     if(img(i,j)<min)//If the centre pixel is less than minimum in the window,set it to min.
					 {
						 *(out->Begin(i,j))=(float)min;
					 }
					if(img(i,j)>max)//If the centre pixel is greater than maximum in the window, set it to max.
					{
						*(out->Begin(i,j))=(float)max;
					}
		}
		}
	}
}

void ConservativeSmoothing(const ImgInt& img,  const int win_width,const int win_height, ImgInt* out)
{
	out->Reset(img.Width(), img.Height());  
	const int img_height=img.Height();
	const int img_width=img.Width();
		
	int x_len,y_len;
	int min,max;
	int i,j;
	for (i=0;i<img_width;i++)
	{
		for (j=0;j<img_height;j++)
		{
			*(out->Begin(i,j)) = *(img.Begin(i,j));
			
		}
	}
	
	
	for(i=0;i<img_width-1;i++)
	{
		for(j=0;j<img_height-1;j++)
		{
		min=999;
		max=0;
		for(int x=-win_width/2;x<=win_width/2;x++)	
		{
			for(int y=-win_height/2;y<=win_height/2;y++)
				{
					
					x_len=i+x;y_len=j+y;
					if((x_len)<0)
					{
						x_len=0;
					}
					if((y_len)<0)
					{
						y_len=0;
					}
					if(x_len>img_width-1)
					{
						x_len=img_width-2;
					}
					if(y_len>img_height-1)
					{
						y_len=img_height-2;
					}
					//Find the minimum and the maximum valu in the window
					if(img(x_len,y_len)<min && x_len!=i && y_len!=j)
					{
						min=img(x_len,y_len);
					}
					if(img(x_len,y_len)>max && x_len!=i && y_len!=j)
					{
						max=img(x_len,y_len);
					}

					
					}
				     if(img(i,j)<min)//If the centre pixel is less than minimum in the window,set it to min.
					 {
						 *(out->Begin(i,j))=min;
					 }
					if(img(i,j)>max)//If the centre pixel is greater than maximum in the window, set it to max.
					{
						*(out->Begin(i,j))=max;
					}
		}
		}
	}
}

// should make these member variables of ColorHistogramx class
#define COLORHISTOGRAM_HEADER_LENGTH 3
char colorhistogram_header[COLORHISTOGRAM_HEADER_LENGTH+1] = "CH1";
#define CH_MODEL_SZ		0  // unused

ColorHistogramx::ColorHistogramx()
{
	int i;
	n_bins_color[0]=8;
	n_bins_color[1]=8;
	n_bins_color[2]=8;
	n_bins_tot=1;
	for(i=0; i<3; i++)
	{
		n_bins_tot*=n_bins_color[i];
		binwidth_color[i]=256/n_bins_color[i];
	}
	val.resize(n_bins_tot);
	for(i=0; i<n_bins_tot; i++)
		val[i]=0;
}

ColorHistogramx::~ColorHistogramx()
{
}

int ColorHistogramx::GetIndex(unsigned char color1, unsigned char color2, unsigned char color3)
{
	int col1, col2, col3, indx;
	col1 = color1/binwidth_color[0];
	col2 = color2/binwidth_color[1];
	col3 = color3/binwidth_color[2];
	indx = col3*n_bins_color[0]*n_bins_color[1] + col2*n_bins_color[0] + col1;
	assert(indx >= 0 && indx < n_bins_tot);
	return indx;
}

void ColorHistogramx::GetColor(int index, unsigned char* color1, unsigned char* color2, unsigned char* color3)
{
	assert(index >= 0 && index < n_bins_tot);
	unsigned char col1, col2, col3;

	col3 = index / (n_bins_color[0]*n_bins_color[1]);
	col2 = (index - (col3*n_bins_color[0]*n_bins_color[1])) / n_bins_color[0];
	col1 = index - (col3*n_bins_color[0]*n_bins_color[1] + col2*n_bins_color[0]);

	(*color1) = col1*binwidth_color[0];
	(*color2) = col2*binwidth_color[1];
	(*color3) = col3*binwidth_color[2];
}

void ColorHistogramx::ComputeHistogram(const ImgGray& color1,
									  const ImgGray& color2,
									  const ImgGray& color3)
{
	const int ISIZEX = color1.Width(), ISIZEY = color1.Height();
	const unsigned char *ptr1, *ptr2, *ptr3;
	int indx;
	int i;

	assert(256%n_bins_color[0]==0 && 256%n_bins_color[1]==0 && 256%n_bins_color[2]==0);
	memset((int*) &val[0], 0, n_bins_tot * sizeof(int)); 
	ptr1 = color1.Begin();
	ptr2 = color2.Begin();
	ptr3 = color3.Begin();
	for (i=0 ; i<ISIZEX*ISIZEY ; i++) 
	{
		indx = GetIndex(*ptr1, *ptr2, *ptr3);
		assert(indx >= 0 && indx < n_bins_tot);
		(val[indx])++;
		ptr1++;
		ptr2++;
		ptr3++;
	}
}

void ColorHistogramx::ComputeHistogram(const ImgGray& color1,
									  const ImgGray& color2,
									  const ImgGray& color3,
									  const ImgBinary& mask)
{
	const int ISIZEX = color1.Width(), ISIZEY = color1.Height();
	const unsigned char *ptr1, *ptr2, *ptr3;
	ImgBinary::ConstIterator q;
	int indx;
	int i;

	assert(256%n_bins_color[0]==0 && 256%n_bins_color[1]==0 && 256%n_bins_color[2]==0);
	memset((int*) &val[0], 0, n_bins_tot * sizeof(int)); 
	ptr1 = color1.Begin();
	ptr2 = color2.Begin();
	ptr3 = color3.Begin();
	q = mask.Begin();
	for (i=0 ; i<ISIZEX*ISIZEY ; i++) 
	{
		if (*q++)
		{
			indx = GetIndex(*ptr1, *ptr2, *ptr3);
			assert(indx >= 0 && indx < n_bins_tot);
			(val[indx])++;
		}
		ptr1++;
		ptr2++;
		ptr3++;
	}
}

double ColorHistogramx::CompareHistograms(const ColorHistogramx& ch1, const ColorHistogramx& ch2)
{
	std::vector<int>::const_iterator ptr1, ptr2;
	double sum=0;

	ptr1 = ch1.val.begin();
	ptr2 = ch2.val.begin();
	for(unsigned int i=0; i<ch1.val.size(); i++) 
	{
		sum += min(*ptr1, *ptr2);
		ptr1++;  ptr2++;
	}

	return sum;
}

double ColorHistogramx::CompareHistogramsBhattacharyya(const ColorHistogramx& ch1, const ColorHistogramx& ch2)
{
	std::vector<int>::const_iterator ptr1, ptr2;
	double sum1=0, sum2=0, corr=0;

	ptr1 = ch1.val.begin();
	ptr2 = ch2.val.begin();
	for(unsigned int i=0; i<ch1.val.size(); i++) 
	{
		double val1 = *ptr1;
		double val2 = *ptr2;
		sum1 += val1;
		sum2 += val2;
		corr += sqrt( val1 * val2 );
		ptr1++;  ptr2++;
	}
	corr /= sqrt( sum1 * sum2 );
	return corr;
}

void ColorHistogramx::WriteToFile(const char *fname) const
{
	FILE *fp;
	int tmp;

	// Open file for writing
	fp = fopen(fname, "wb");
	if (fp == NULL) {
		AfxMessageBox(L"Error: could not open file for writing color histogram!!", MB_OK|MB_ICONSTOP, 0);
		return;
	}

	// Write header
	fwrite(colorhistogram_header, sizeof(char), COLORHISTOGRAM_HEADER_LENGTH, fp);
	tmp = n_bins_color[0];  fwrite(&tmp, sizeof(int), 1, fp);
	tmp = n_bins_color[1];  fwrite(&tmp, sizeof(int), 1, fp);
	tmp = n_bins_color[2];  fwrite(&tmp, sizeof(int), 1, fp);
	tmp = CH_MODEL_SZ;    fwrite(&tmp, sizeof(int), 1, fp);

	// Write color histogram model
	if (fwrite(&val[0], sizeof(int), n_bins_tot, fp) != n_bins_tot) 
		AfxMessageBox(L"Error writing color histogram!!", MB_OK|MB_ICONSTOP, 0);
	fclose(fp);
}

void ColorHistogramx::WriteColorToFile(const char *fname) const
{
	FILE *fp;

	// Open file for writing
	fp = fopen(fname, "w");
	if (fp == NULL) {
		AfxMessageBox(L"Error: could not open file for writing color histogram!!", MB_OK|MB_ICONSTOP, 0);
		return;
	}

	// Write color histogram model
	for(int i=0; i<n_bins_tot; i++)
		fprintf(fp,"%d ",val[i]);
	fclose(fp);
}

void ColorHistogramx::ReadFromFile(const char *fname)
{
	char strtmp[COLORHISTOGRAM_HEADER_LENGTH+1];
	int tmp;
	FILE *fp;

	// Open file for reading
	fp = fopen(fname, "rb");
	if (fp == NULL) {
		AfxMessageBox(L"Error: could not open file for reading color histogram!!", MB_OK|MB_ICONSTOP, 0);
		return;
	}

	// Read header
	fread(strtmp, sizeof(char), COLORHISTOGRAM_HEADER_LENGTH, fp);
	strtmp[COLORHISTOGRAM_HEADER_LENGTH] = '\0';
	if (strcmp(strtmp, colorhistogram_header) != 0)
		AfxMessageBox(L"Error: color histogram file has invalid header!!", MB_OK|MB_ICONSTOP, 0);
	fread(&tmp, sizeof(int), 1, fp);
	if (tmp != n_bins_color[0])
		AfxMessageBox(L"Error: color histogram file contains wrong # of color1 bins!!", MB_OK|MB_ICONSTOP, 0);
	fread(&tmp, sizeof(int), 1, fp);
	if (tmp != n_bins_color[1])
		AfxMessageBox(L"Error: color histogram file contains wrong # of color2 bins!!", MB_OK|MB_ICONSTOP, 0);
	fread(&tmp, sizeof(int), 1, fp);
	if (tmp != n_bins_color[2])
		AfxMessageBox(L"Error: color histogram file contains wrong # of color3 bins!!", MB_OK|MB_ICONSTOP, 0);
	fread(&tmp, sizeof(int), 1, fp);
	if (tmp != CH_MODEL_SZ)
		AfxMessageBox(L"Error: color histogram file contains wrong ellipse size!!", MB_OK|MB_ICONSTOP, 0);

	// Read color histogram model
	if (fread(&val[0], sizeof(int), n_bins_tot, fp) != n_bins_tot) 
		AfxMessageBox(L"Error reading color histogram!!", MB_OK|MB_ICONSTOP, 0);
	fclose(fp);
}

std::vector<double> ColorHistogramx::Normalize(int totalUnits)
{
	assert(totalUnits > 0);
	std::vector<double> pdf;

	for(unsigned int i=0; i<val.size(); i++)
		pdf.push_back((double)val.at(i)/(double)totalUnits);
	return(pdf);
}

};  // end namespace blepo

