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

// Note:  see ComputeDifferentialPixelLists for my attempt to speed 
// computation; should be possible but will require some thought.  -- STB

//#include "EllipticalHeadTracker.h"
#include "ImageAlgorithms.h"
#include "ImageOperations.h"
#include "Figure/Figure.h"
#include <math.h>  // sqrt


// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

// added by STB 8/13/2010
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

// ================> begin local functions (available only to this translation unit)
namespace
{
using namespace blepo;

// If you change any of these parameters, be sure to delete the pixellist files,
// to force the code to recompute them.
const char* fname_pixellist_add = "blepo_eht_pixellist_add.dat";
const char* fname_pixellist_sub = "blepo_eht_pixellist_sub.dat";
#define MIN_OUTLINE_SZ	15  // minimum ellipse size (halfwidth)
#define MAX_OUTLINE_SZ	25  // maximum ellipse size (halfwidth)
#define N_STATE_VARIABLES	3  // number of variables in state (e.g., x, y, size)
#define MAX_N_STATES_TO_CHECK	9999 // this is an upper-bound, for allocating memory
#define N_BINS_COLOR1		8		// 256 must be divisible by this number!!!!
#define N_BINS_COLOR2		8		// 256 must be divisible by this number!!!!
#define N_BINS_COLOR3		4		// 256 must be divisible by this number!!!!
double aspect_ratio = 1.2;  // aspect ratio of ellipse
int msx = 4, msy = 4;  // Search range is +/- {msx, msy} pixels
int szinc = 1;  // Skip szinc pixels to find the next smaller or larger scale

// Should not need to touch these.
#define N_BINS_TOT			(N_BINS_COLOR1*N_BINS_COLOR2*N_BINS_COLOR3)
#define BINWIDTH_COLOR1		(256/N_BINS_COLOR1)
#define BINWIDTH_COLOR2		(256/N_BINS_COLOR2)
#define BINWIDTH_COLOR3		(256/N_BINS_COLOR3)
#define CH_MODEL_SZ		MAX_OUTLINE_SZ-1
typedef signed char pixelListType;
typedef signed char searchLocType;

// PixelList
// Contains list of pixels to be added or deleted from an area (in order to 
// update histogram, for example).  List is in the form (x1,y1), (x2,y2), etc.
// All coordinates are relative to the center of the outline.
typedef struct {
	int n_pixels;
	pixelListType *pixels;
} PixelList;

// SearchStrategy
// Contains the search strategy, [x1, y1, s1, x2, y2, s2, ...].  All states
// are relative to the initial search location. 
// E.g., [-1 -2 -1 0 -2 -1 1 -2 1] means check the states 
// (x,y,s) = [(-1,-2,-1), (0,-2,-1), (1,-2,-1)].
typedef struct {
	int n_states_to_check;
	searchLocType ds[N_STATE_VARIABLES * MAX_N_STATES_TO_CHECK];
} SearchStrategy;

typedef struct {
	int perim_length;	// no. of pixels along perimeter
	short *perim;
	float *normals;		// normal vectors
	int halfsize_x;
	int halfsize_y;
	int area;			// no. of pixels in mask
	unsigned char *mask; // (2*halfsize_x+1) by (2*halfsize_y+1) region
} OUTLINE;

typedef struct {
	int val[N_BINS_TOT];
} ColorHistogram;

// images
ImgGray img_gray;
ImgGray img_gauss;
ImgGray img_grad;
ImgFloat img_gradxf;
ImgFloat img_gradyf;
ImgBgr img_outline;
ImgBgr img_outline_grad;
ImgBgr img_outline_color;
ImgGray img_outline8;
ImgGray img_outline_grad8;
ImgGray img_outline_color8;
ImgGray img_mask;
ImgGray img_color1;
ImgGray img_color2;
ImgGray img_color3;
ImgInt error_grad[3];  // one for each size in the search range
ImgInt error_color[3];

// external variables
BOOL use_gradient = true;
BOOL use_color = true;
BOOL sum_gradient_magnitude = false;
BOOL show_details = false;

// Other variables
ColorHistogram ch_model;    // for building ref histogram over multiple image frames
ColorHistogram ch_ref;      // used for comparing with current image
ColorHistogram ch_background;
ColorHistogram ch_curr;
int n_images_in_model;

// Outlines
OUTLINE outlines[MAX_OUTLINE_SZ + 1];

// Search strategy
SearchStrategy search_strategy;
SearchStrategy ss_tmp;

PixelList **add_lists = NULL;
PixelList **sub_lists = NULL;
//PixelList **add_lists_tmp = NULL;
//PixelList **sub_lists_tmp = NULL;

///////////////////////////////////////////////////////////////////
// from outline.cpp

////////////////////
// Given an outline whose perimeter and bounding box (halfsize_[x,y])
// have been determined, this function allocates and constructs the mask.
// Returns the number of pixels in the mask.
static int MakeMask(OUTLINE *outline)
{
	unsigned char *ptrmask;
	short *ptrperim;
	int *mins, *maxs;
	int *ptrmin, *ptrmax;
	int x, y;
	int i, j;
	int sizex = 2*(outline->halfsize_x)+1;
	int sizey = 2*(outline->halfsize_y)+1;
	int n_pixels = 0;

	mins = (int *) malloc(sizey * sizeof(int));
	maxs = (int *) malloc(sizey * sizeof(int));
	outline->mask = (unsigned char *) malloc(sizex * sizey * sizeof(char));
	
	// initialize mins and maxs
	ptrmin = mins;
	ptrmax = maxs;
	for (i = 0 ; i < sizey ; i++)  {
		*ptrmin++ =  999;
		*ptrmax++ = -999;
	}

	// initialize mask to zero
	memset(outline->mask, 0, sizex*sizey);

	// fill in max and min vectors
	ptrperim = outline->perim;
	x = outline->halfsize_x + *ptrperim++;
	y = outline->halfsize_y + *ptrperim++;
	ptrmax = maxs + y;
	ptrmin = mins + y;
	*ptrmax = x;
	*ptrmin = x;
	for (i = 1 ; i < outline->perim_length ; i++)  {
		x += *ptrperim++;
		ptrmax += *ptrperim;
		ptrmin += *ptrperim++;
		if (x > *ptrmax)  *ptrmax = x;
		if (x < *ptrmin)  *ptrmin = x;
	}
	
	// use max and min vectors to fill in mask
	// the interior of the object is filled in, but not the perimeter
	ptrmin = mins;
	ptrmax = maxs;
	for (i = 0 ; i < sizey ; i++)  {
		ptrmask = (outline->mask) + i * sizex;
		if (*ptrmin < *ptrmax)  {
			ptrmask += *ptrmin + 1;
			for (j = *ptrmin + 1 ; j < *ptrmax ; j++) {
				*ptrmask++ = 255;
				n_pixels++;
			}
		}
		ptrmin++;
		ptrmax++;
	}

  free(mins);
  free(maxs);

	return n_pixels;
}

////////////////////
// Creates an elliptical outline, used as a model for the head.  
// 
// INPUTS
// xrad, yrad:  length of the minor and major axes, respectively,
// 			 of the ellipse (in pixels).
// 
// NOTES
// xrad < yrad
void MakeEllipticalOutline(OUTLINE *outline, int xrad, int yrad)
{
	short *ptr;
	float *ptrn;
	int xradsq = xrad * xrad;
	int yradsq = yrad * yrad;
	double ratio = ((double) xradsq) / ((double) yradsq);
	int xradbkpt = (int) (xradsq / sqrt((double) xradsq + yradsq));
	int yradbkpt = (int) (yradsq / sqrt((double) xradsq + yradsq));
	int x, y;			// x & y are in image coordinates
	int curx, cury;

	// count # of iterations in loops below
	outline->perim_length = 4 * xradbkpt + 4 * yradbkpt + 4; 
	outline->perim = (short *)
			malloc(2 * (outline->perim_length) * sizeof(short));
	outline->normals = (float *)
			malloc(2 * (outline->perim_length) * sizeof(float));
	ptr = outline->perim;
	ptrn = outline->normals;
	*(ptr)++ = curx = 0;  // perimeter starts at top and proceeds clockwise
	*(ptr)++ = cury = - yrad;
	*ptrn++ = 0.0f;
	*ptrn++ = -1.0f;

	// from 0 to 45 degrees, measured cw from top
	for (x = 1; x <= xradbkpt ; x++)  {
		y = (int) (-yrad * sqrt(1 - ((double) x / xrad)*((double) x / xrad)));
		*(ptr)++ = x - curx;
		*(ptr)++ = y - cury;
		*ptrn++ = (float) (       x  / sqrt(x*x + ratio*y*y));
		*ptrn++ = (float) ((ratio*y) / sqrt(x*x + ratio*y*y));
		curx = x;
		cury = y;
	}
	
	// from 45 to 135 degrees (including right axis)
	for (y = - yradbkpt ; y <= yradbkpt ; y++)  {
		x = (int) (xrad * sqrt(1 - ((double) y / yrad)*((double) y / yrad)));
		*(ptr)++ = x - curx;
		*(ptr)++ = y - cury;
		*ptrn++ = (float) (       x  / sqrt(x*x + ratio*y*y));
		*ptrn++ = (float) ((ratio*y) / sqrt(x*x + ratio*y*y));
		curx = x;
		cury = y;
	}
	// from 135 to 225 degrees (including down axis)
	for (x = xradbkpt  ; x >= - xradbkpt ; x--)  {
		y = (int) (yrad * sqrt(1 - ((double) x / xrad)*((double) x / xrad)));
		*(ptr)++ = x - curx;
		*(ptr)++ = y - cury;
		*ptrn++ = (float) (       x  / sqrt(x*x + ratio*y*y));
		*ptrn++ = (float) ((ratio*y) / sqrt(x*x + ratio*y*y));
		curx = x;
		cury = y;
	}
	// from 225 to 315 degrees (including left axis)
	for (y = yradbkpt ; y >= - yradbkpt ; y--)  {
		x = (int) (-xrad * sqrt(1 - ((double) y / yrad)*((double) y / yrad)));
		*(ptr)++ = x - curx;
		*(ptr)++ = y - cury;
		*ptrn++ = (float) (       x  / sqrt(x*x + ratio*y*y));
		*ptrn++ = (float) ((ratio*y) / sqrt(x*x + ratio*y*y));
		curx = x;
		cury = y;
	}
	// from 315 to 360 degrees
	for (x = - xradbkpt ; x < 0 ; x++)  {
		y = (int) (-yrad * sqrt(1 - ((double) x / xrad)*((double) x / xrad)));
		*(ptr)++ = x - curx;
		*(ptr)++ = y - cury;
		*ptrn++ = (float) (       x  / sqrt(x*x + ratio*y*y));
		*ptrn++ = (float) ((ratio*y) / sqrt(x*x + ratio*y*y));
		curx = x;
		cury = y;
	}
	
	outline->halfsize_x = xrad;
	outline->halfsize_y = yrad;
	outline->area = MakeMask(outline);
}

///////////////////////////////////////////////////////////////////
// from gradient_sum.cpp

/////////////////////
// perimGradMagnitude
// 
// Sums the gradient magnitude along the perimeter of an outline.
static int perimGradMagnitude(OUTLINE *outline, 
							  const ImgGray& grad,
							  int x_cen, int y_cen)
{
  const int ISIZEX = grad.Width(), ISIZEY = grad.Height();
	int x, y;
	short *ptr;
	const unsigned char *ptrgrad;
	int sum;
	int i;
	int incx, incy;

	ptr = outline->perim;
	x = x_cen;
	y = y_cen;
	ptrgrad = grad.Begin() + (ISIZEX * y_cen) + x_cen;

	sum = 0;
	for (i = 0 ; i < outline->perim_length ; i++)  {
		incx = *ptr++;
		incy = *ptr++;
		ptrgrad += (ISIZEX * incy) + incx;
		assert(ptrgrad >= grad.Begin() 
			   && ptrgrad < (grad.Begin() + ISIZEX*ISIZEY*sizeof(char)));
		sum += *ptrgrad;
	}	

	return(sum);
}

int NormalizedSumOfGradientMagnitude(int x, int y, OUTLINE *outline,
									 const ImgGray& grad)
{
	int perim, score;

	perim = outline->perim_length;
	score = perimGradMagnitude(outline, grad, x, y);
	score /= perim;
	return score;
}

////////////////////
// Sums the gradient magnitude along the perimeter of an outline.
static int perimGradDotProduct(OUTLINE *outline, 
							   const ImgFloat& gradx, const ImgFloat& grady,
							   int x_cen, int y_cen)
{
  const int ISIZEX = gradx.Width(), ISIZEY = gradx.Height();
	int x, y;
	short *ptr;
	float *ptrn;
	const float *ptrgradx, *ptrgrady;
	float sum;
	float nx, ny;
	int i;
	int incx, incy;

	ptr = outline->perim;
	ptrn = outline->normals;
	x = x_cen;
	y = y_cen;
	ptrgradx = gradx.Begin() + (ISIZEX * y_cen) + x_cen;
	ptrgrady = grady.Begin() + (ISIZEX * y_cen) + x_cen;

	sum = 0.0f;
	for (i = 0 ; i < outline->perim_length ; i++)  {
		incx = *ptr++;
		incy = *ptr++;
		ptrgradx += (ISIZEX * incy) + incx;
		ptrgrady += (ISIZEX * incy) + incx;
		assert(ptrgradx >= gradx.Begin() 
			   && ptrgradx < (gradx.Begin() + ISIZEX*ISIZEY*sizeof(float)));
		assert(ptrgrady >= grady.Begin() 
			   && ptrgrady < (grady.Begin() + ISIZEX*ISIZEY*sizeof(float)));
		nx = *ptrn++;
		ny = *ptrn++;
		sum += fabs((nx * *ptrgradx) + (ny * *ptrgrady));
	}	

	return((int) (100*sum));
}

int NormalizedSumOfGradientDotProduct(int x, int y, OUTLINE *outline, //int sz, 
									  const ImgFloat& gradx, const ImgFloat& grady)
{
	int perim, score;

	perim = outline->perim_length;
	score = perimGradDotProduct(outline, gradx, grady, x, y);
	score /= perim;
	return score;
}

///////////////////////////////////////////////////////////////////
// from color_histogram.cpp

void ComputeColorHistogramOfWholeImage(const ImgGray& color1,
									   const ImgGray& color2,
									   const ImgGray& color3,
									   ColorHistogram *ch)
{
  const int ISIZEX = color1.Width(), ISIZEY = color1.Height();
	const unsigned char *ptr1, *ptr2, *ptr3;
	int col1, col2, col3;
	int indx;
	int i;

	assert(256%N_BINS_COLOR1==0 && 256%N_BINS_COLOR2==0 && 256%N_BINS_COLOR3==0);
	memset(ch->val, 0, N_BINS_TOT * sizeof(int)); 
	ptr1 = color1.Begin();
	ptr2 = color2.Begin();
	ptr3 = color3.Begin();
	for (i=0 ; i<ISIZEX*ISIZEY ; i++) {
		col1 = *ptr1/BINWIDTH_COLOR1;
		col2 = *ptr2/BINWIDTH_COLOR2;
		col3 = *ptr3/BINWIDTH_COLOR3;
		indx = col3*N_BINS_COLOR1*N_BINS_COLOR2+col2*N_BINS_COLOR1 + col1;
		assert(indx >= 0 && indx < N_BINS_TOT);
		(ch->val[indx])++;
		ptr1++;
		ptr2++;
		ptr3++;
	}
}

void ComputeColorHistogram(const ImgGray& color1,
						   const ImgGray& color2,
						   const ImgGray& color3,
						   int xcen,
						   int ycen,
						   OUTLINE *outline,
						   ColorHistogram *ch)
{
  const int ISIZEX = color1.Width(), ISIZEY = color1.Height();
	int hx=outline->halfsize_x, hy=outline->halfsize_y;
	const unsigned char *ptr1, *ptr2, *ptr3;
	unsigned char *ptro = outline->mask;
	int col1, col2, col3;
	int indx;
	int x, y;

	assert(256%N_BINS_COLOR1==0 && 256%N_BINS_COLOR2==0 && 256%N_BINS_COLOR3==0);
	memset(ch->val, 0, N_BINS_TOT * sizeof(int)); 
	ptr1 = color1.Begin() + (ycen-hy)*ISIZEX + (xcen-hx);
	ptr2 = color2.Begin() + (ycen-hy)*ISIZEX + (xcen-hx);
	ptr3 = color3.Begin() + (ycen-hy)*ISIZEX + (xcen-hx);
	for (y=ycen-hy ; y<=ycen+hy ; y++) {
		for (x=xcen-hx ; x<=xcen+hx ; x++) {
			if (*ptro++) {
				col1 = *ptr1/BINWIDTH_COLOR1;
				col2 = *ptr2/BINWIDTH_COLOR2;
				col3 = *ptr3/BINWIDTH_COLOR3;
				indx = col3*N_BINS_COLOR1*N_BINS_COLOR2+col2*N_BINS_COLOR1 + col1;
				assert(indx >= 0 && indx < N_BINS_TOT);
				(ch->val[indx])++;
			}
			ptr1++;
			ptr2++;
			ptr3++;
		}
		ptr1 += ISIZEX - (2*hx+1);
		ptr2 += ISIZEX - (2*hx+1);
		ptr3 += ISIZEX - (2*hx+1);
	}
}

void UpdateColorHistogram(const ImgGray& color1,
						  const ImgGray& color2,
						  const ImgGray& color3,
						  int xcen,
						  int ycen,
						  PixelList *addlist,
						  PixelList *sublist,
						  ColorHistogram *ch)
{
  const int ISIZEX = color1.Width(), ISIZEY = color1.Height();
	const unsigned char *ptr1, *ptr2, *ptr3;
	pixelListType *ptra, *ptrs;
	int col1, col2, col3;
	int x, y;
	int indx;
	int i;

	assert(256%N_BINS_COLOR1==0 && 256%N_BINS_COLOR2==0 && 256%N_BINS_COLOR3==0);
	ptr1 = color1.Begin();
	ptr2 = color2.Begin();
	ptr3 = color3.Begin();

	// add new pixels
	ptra = addlist->pixels;
	for (i = addlist->n_pixels ; i>0 ; i--) {
		x = *ptra++ + xcen;
		y = *ptra++ + ycen;
		assert(x>=0 && x<ISIZEX && y>=0 && y<ISIZEY);
		col1 = *(ptr1+y*ISIZEX+x)/BINWIDTH_COLOR1;
		col2 = *(ptr2+y*ISIZEX+x)/BINWIDTH_COLOR2;
		col3 = *(ptr3+y*ISIZEX+x)/BINWIDTH_COLOR3;
		indx = col3*N_BINS_COLOR1*N_BINS_COLOR2+col2*N_BINS_COLOR1 + col1;
		assert(indx >= 0 && indx < N_BINS_TOT);
		(ch->val[indx])++;
	}

	// subtract old pixels
	ptrs = sublist->pixels;
	for (i = sublist->n_pixels ; i>0 ; i--) {
		x = *ptrs++ + xcen;
		y = *ptrs++ + ycen;
		assert(x>=0 && x<ISIZEX && y>=0 && y<ISIZEY);
		col1 = *(ptr1+y*ISIZEX+x)/BINWIDTH_COLOR1;
		col2 = *(ptr2+y*ISIZEX+x)/BINWIDTH_COLOR2;
		col3 = *(ptr3+y*ISIZEX+x)/BINWIDTH_COLOR3;
		indx = col3*N_BINS_COLOR1*N_BINS_COLOR2+col2*N_BINS_COLOR1 + col1;
		assert(indx >= 0 && indx < N_BINS_TOT);
		(ch->val[indx])--;
	}
}

void UpdateColorHistogramAndIntersection(const ImgGray& color1,
										 const ImgGray& color2,
										 const ImgGray& color3,
										 int xcen,
										 int ycen,
										 PixelList *addlist,
										 PixelList *sublist,
										 ColorHistogram *ch_model,
										 ColorHistogram *ch_img,
										 int *score)
{
  const int ISIZEX = color1.Width(), ISIZEY = color1.Height();
	const unsigned char *ptr1, *ptr2, *ptr3;
	pixelListType *ptra, *ptrs;
	int col1, col2, col3;
	int x, y;
	int indx;
	int i;

	assert(256%N_BINS_COLOR1==0 && 256%N_BINS_COLOR2==0 && 256%N_BINS_COLOR3==0);
	ptr1 = color1.Begin();
	ptr2 = color2.Begin();
	ptr3 = color3.Begin();

	// add new pixels
	ptra = addlist->pixels;
	for (i = addlist->n_pixels ; i>0 ; i--) {
		x = *ptra++ + xcen;
		y = *ptra++ + ycen;
		assert(x>=0 && x<ISIZEX && y>=0 && y<ISIZEY);
		col1 = *(ptr1+y*ISIZEX+x)/BINWIDTH_COLOR1;
		col2 = *(ptr2+y*ISIZEX+x)/BINWIDTH_COLOR2;
		col3 = *(ptr3+y*ISIZEX+x)/BINWIDTH_COLOR3;
		indx = col3*N_BINS_COLOR1*N_BINS_COLOR2+col2*N_BINS_COLOR1 + col1;
		assert(indx >= 0 && indx < N_BINS_TOT);
		(ch_img->val[indx])++;
		if (ch_img->val[indx] <= ch_model->val[indx]) (*score)++;
	}

	// subtract old pixels
	ptrs = sublist->pixels;
	for (i = sublist->n_pixels ; i>0 ; i--) {
		x = *ptrs++ + xcen;
		y = *ptrs++ + ycen;
		assert(x>=0 && x<ISIZEX && y>=0 && y<ISIZEY);
		col1 = *(ptr1+y*ISIZEX+x)/BINWIDTH_COLOR1;
		col2 = *(ptr2+y*ISIZEX+x)/BINWIDTH_COLOR2;
		col3 = *(ptr3+y*ISIZEX+x)/BINWIDTH_COLOR3;
		indx = col3*N_BINS_COLOR1*N_BINS_COLOR2+col2*N_BINS_COLOR1 + col1;
		assert(indx >= 0 && indx < N_BINS_TOT);
		(ch_img->val[indx])--;
		if (ch_img->val[indx] < ch_model->val[indx]) (*score)--;
	}
}


int ColorHistogramIntersection(ColorHistogram *ch1,
							   ColorHistogram *ch2)
{
	int *ptr1, *ptr2;
	int sum=0;
	int i;

	ptr1 = ch1->val;
	ptr2 = ch2->val;
	for (i=0 ; i<N_BINS_TOT ; i++) {
		sum += min(*ptr1, *ptr2);
		ptr1++;  ptr2++;
	}

	return sum;
}

// add histograms, save the result in ch1
void AddColorHistograms(ColorHistogram *ch1, 
						ColorHistogram *ch2)
{
	int *ptr1, *ptr2;
	int i;

	ptr1 = ch1->val;
	ptr2 = ch2->val;
	for (i=0 ; i<N_BINS_TOT ; i++) {
		*ptr1 += *ptr2;
		ptr1++;  ptr2++;
	}
}

void ExtractColorSpace(const ImgBgr& img, 
					   ImgGray* img_color1, 
					   ImgGray* img_color2,
					   ImgGray* img_color3)
{
  const int ISIZEX = img.Width(), ISIZEY = img.Height();
	const unsigned char *ptri = img.BytePtr();
	unsigned char *ptr1 = img_color1->Begin();
	unsigned char *ptr2 = img_color2->Begin();
	unsigned char *ptr3 = img_color3->Begin();
	int b, g ,r;
	int i;

	for (i=0 ; i<ISIZEX*ISIZEY ; i++) {
		b = *ptri++;
		g = *ptri++;
		r = *ptri++;
		// NOTE: These next three lines must be manually maintained to remain 
		// consistent with the three lines in the function below.
		*ptr1++ = (unsigned char) max(0, min(255, (b-g)*10+128));
		*ptr2++ = (unsigned char) max(0, min(255, (g-r)*10+128));
		*ptr3++ = (unsigned char) max(0, min(255, (b+g+r)/3));
	}
}

void BGRToColorBins(unsigned char b,
					unsigned char g,
					unsigned char r,
					int *col1, 
					int *col2, 
					int *col3)
{
	// NOTE: These next three lines must be manually maintained to remain
	// consistent with the three lines in the function above.
	*col1 = (unsigned char) max(0, min(255, (b-g)*10+128));
	*col2 = (unsigned char) max(0, min(255, (g-r)*10+128));
	*col3 = (unsigned char) max(0, min(255, (b+g+r)/3));
	*col1 /= BINWIDTH_COLOR1;
	*col2 /= BINWIDTH_COLOR2;
	*col3 /= BINWIDTH_COLOR3;
}

// used only for displaying
void QuantizeColorSpace(const ImgGray& img_color_in1, 
						const ImgGray& img_color_in2,
						const ImgGray& img_color_in3,
						ImgGray* img_color_out1,
						ImgGray* img_color_out2,
						ImgGray* img_color_out3)
{
  const int ISIZEX = img_color_in1.Width(), ISIZEY = img_color_in1.Height();
	const unsigned char *ptri1 = img_color_in1.Begin();
	const unsigned char *ptri2 = img_color_in2.Begin();
	const unsigned char *ptri3 = img_color_in3.Begin();
	unsigned char *ptro1 = img_color_out1->Begin();
	unsigned char *ptro2 = img_color_out2->Begin();
	unsigned char *ptro3 = img_color_out3->Begin();
	int i;

//	assert(BINWIDTH_COLOR1 == 8);
//	assert(BINWIDTH_COLOR2 == 8);
//	assert(BINWIDTH_COLOR3 == 8);
	for (i=0 ; i<ISIZEX*ISIZEY ; i++) {
		*ptro1++ = (*ptri1++ / BINWIDTH_COLOR1) * BINWIDTH_COLOR1;
		*ptro2++ = (*ptri2++ / BINWIDTH_COLOR2) * BINWIDTH_COLOR2;
		*ptro3++ = (*ptri3++ / BINWIDTH_COLOR3) * BINWIDTH_COLOR3;
	}
}

// ch_in and ch_out are allowed to be the same
void NormalizeColorHistogram(ColorHistogram *ch_in,
							 int numerator,
							 int denominator,
							 ColorHistogram *ch_out) 
{
	int *ptri, *ptro;
	int i;

	if (denominator == 0) 
		AfxMessageBox(L"Warning: divide by zero", MB_OK|MB_ICONWARNING, 0);
	else {
		ptri = ch_in->val;
		ptro = ch_out->val;
		for (i=0 ; i<N_BINS_TOT ; i++) {
			*ptro++ = (*ptri++ * numerator) / denominator;
		}
	}
}

#define COLORHISTOGRAM_HEADER_LENGTH 3
char colorhistogram_header[COLORHISTOGRAM_HEADER_LENGTH+1] = "CH1";

void WriteColorHistogram(const char *fname,
						 ColorHistogram *ch) 
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
	tmp = N_BINS_COLOR1;  fwrite(&tmp, sizeof(int), 1, fp);
	tmp = N_BINS_COLOR2;  fwrite(&tmp, sizeof(int), 1, fp);
	tmp = N_BINS_COLOR3;  fwrite(&tmp, sizeof(int), 1, fp);
	tmp = CH_MODEL_SZ;    fwrite(&tmp, sizeof(int), 1, fp);

	// Write color histogram model
	if (fwrite(ch->val, sizeof(int), N_BINS_TOT, fp) != N_BINS_TOT) 
		AfxMessageBox(L"Error writing color histogram!!", MB_OK|MB_ICONSTOP, 0);
	fclose(fp);
}

void ReadColorHistogram(const char *fname,
						ColorHistogram *ch) 
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
	if (tmp != N_BINS_COLOR1)
		AfxMessageBox(L"Error: color histogram file contains wrong # of color1 bins!!", MB_OK|MB_ICONSTOP, 0);
	fread(&tmp, sizeof(int), 1, fp);
	if (tmp != N_BINS_COLOR2)
		AfxMessageBox(L"Error: color histogram file contains wrong # of color2 bins!!", MB_OK|MB_ICONSTOP, 0);
	fread(&tmp, sizeof(int), 1, fp);
	if (tmp != N_BINS_COLOR3)
		AfxMessageBox(L"Error: color histogram file contains wrong # of color3 bins!!", MB_OK|MB_ICONSTOP, 0);
	fread(&tmp, sizeof(int), 1, fp);
	if (tmp != CH_MODEL_SZ)
		AfxMessageBox(L"Error: color histogram file contains wrong ellipse size!!", MB_OK|MB_ICONSTOP, 0);

	// Read color histogram model
	if (fread(ch->val, sizeof(int), N_BINS_TOT, fp) != N_BINS_TOT) 
		AfxMessageBox(L"Error reading color histogram!!", MB_OK|MB_ICONSTOP, 0);
	fclose(fp);
}
#undef COLORHISTOGRAM_HEADER_LENGTH

// ch_in and ch_out are allowed to be the same
int SizeOfColorHistogram(ColorHistogram *ch)
{
	int *ptr;
	int i;
	int count = 0;

	ptr = ch->val;
	for (i=0 ; i<N_BINS_TOT ; i++) {
		count += *ptr++;
	}
	return count;
}

BOOL AreColorHistogramsEqual(ColorHistogram *ch1,
							 ColorHistogram *ch2)
{
	int *ptr1, *ptr2;
	BOOL equal = TRUE;
	int i;

	ptr1 = ch1->val;
	ptr2 = ch2->val;
	for (i=0 ; i<N_BINS_TOT ; i++) {
		if (*ptr1++ != *ptr2++) equal = FALSE;
	}
	return equal;
}

///////////////////////////////////////////////////////////////////
// from draw.cpp

void DrawMask(int xcen, int ycen, OUTLINE *outline, ImgGray* img_mask)
{
  const int ISIZEX = img_mask->Width(), ISIZEY = img_mask->Height();
	unsigned char *ptri = outline->mask;
	unsigned char *ptro = img_mask->Begin(xcen-outline->halfsize_x, ycen-outline->halfsize_y);
	int sizex = 2*(outline->halfsize_x)+1;
	int sizey = 2*(outline->halfsize_y)+1;
	int x, y;

  Set(img_mask, 0);
	for (y=0 ; y<sizey ; y++) {
		for (x=0 ; x<sizex ; x++) {
      assert(ptro>=img_mask->Begin() && ptro<=img_mask->End());
			*ptro++ = *ptri++;
		}
		ptro += ISIZEX - sizex;
	}
}

void OverlayOutlineBGR24(const ImgGray& img_in, 
						 const EllipticalHeadTracker::EllipseState& state,
						 unsigned char r,
						 unsigned char g,
						 unsigned char b,
						 ImgBgr* img_out)
{
  const int ISIZEX = img_in.Width(), ISIZEY = img_in.Height();
  const unsigned char *ptrin;
  unsigned char *ptrout;
  short *ptrs;
  int sz;
  int x, y;
  int i;
  
  // Copy image
  ptrin = img_in.Begin();
  ptrout = img_out->BytePtr();
  for (i=0 ; i<ISIZEX*ISIZEY ; i++) {
    *ptrout++ = *ptrin;
    *ptrout++ = *ptrin;
    *ptrout++ = *ptrin++;
  }
  
  // Overlay ellipse
  sz = state.sz;
  x = state.x;
  y = state.y;
  ptrs = outlines[sz].perim;
  ptrout = img_out->BytePtr() + 3*(y*ISIZEX + x);
  for (i = outlines[sz].perim_length ; i>0; i--)  {
    x = *ptrs++;
    y = *ptrs++;
    ptrout += 3*(y*ISIZEX + x);
    assert(ptrout>=img_out->BytePtr() && ptrout<img_out->BytePtr()+3*ISIZEX*ISIZEY);
    *ptrout++ = b; 
    *ptrout++ = g; 
    *ptrout++ = r; 
    ptrout -= 3;
  }
}

void OverlayOutlineBGR24(ImgBgr* img, 
						 const EllipticalHeadTracker::EllipseState& state,
             const Bgr& color)
{
  const int ISIZEX = img->Width(), ISIZEY = img->Height();
  unsigned char *ptrout;
  short *ptrs;
  int sz;
  int x, y;
  int i;
  
  // Overlay ellipse
  sz = state.sz;
  x = state.x;
  y = state.y;
  ptrs = outlines[sz].perim;
  ptrout = img->BytePtr() + 3*(y*ISIZEX + x);
  for (i = outlines[sz].perim_length ; i>0; i--)  {
    x = *ptrs++;
    y = *ptrs++;
    ptrout += 3*(y*ISIZEX + x);
    assert(ptrout>=img->BytePtr() && ptrout<img->BytePtr()+3*ISIZEX*ISIZEY);
    *ptrout++ = color.b; 
    *ptrout++ = color.g; 
    *ptrout++ = color.r; 
    ptrout -= 3;
  }
}

void OverlayOutline8(const ImgGray& img_in, 
					 EllipticalHeadTracker::EllipseState *state,
					 unsigned char val,
					 ImgGray* img_out)
{
   const int ISIZEX = img_in.Width(), ISIZEY = img_in.Height();
   const unsigned char *ptrin;
   unsigned char *ptrout;
   short *ptrs;
   int sz;
   int x, y;
   int newval;
   int i;

   // Copy image
   *img_out = img_in;
   if (val==255) newval=254;
   else if (val==0) newval=1;
   else ;
   ptrout = img_out->Begin();
   for (i=0 ; i<ISIZEX*ISIZEY ; i++) {
	   if (*ptrout==val) *ptrout=newval;
	   ptrout++;
   }

	// Overlay ellipse
   ptrin = img_in.Begin();
   ptrout = img_out->Begin();
   for (sz = state->sz ; sz <= state->sz ; sz++)  {
	   x = state->x;
	   y = state->y;
	   ptrs = outlines[sz].perim;
	   ptrout = img_out->Begin() + x + y*ISIZEX;
       for (i = outlines[sz].perim_length ; i>0; i--)  {
           x = *ptrs++;
           y = *ptrs++;
		   ptrout += x + y*ISIZEX;
		   assert(ptrout>=img_out->Begin() && ptrout<img_out->Begin()+ISIZEX*ISIZEY);
		   *ptrout = val; 
       }
   }
}

///////////////////////////////////////////////////////////////////
// from track.cpp

void MakeSearchStrategy(SearchStrategy *ss)
{
	int i = 0;
	int x, y, sz;
	BOOL left_to_right = TRUE;
	BOOL top_to_bottom = TRUE;

	assert(N_STATE_VARIABLES == 3);
	for (sz = -szinc ; sz <= szinc ; sz += szinc) {
		for (y = -msy ; y <= msy ; y++) {
			for (x = -msx ; x <= msx ; x++) {
				assert(i < N_STATE_VARIABLES * MAX_N_STATES_TO_CHECK - 2);
				assert(x>=-127 && x<=127);
				assert(y>=-127 && y<=127);
				assert(sz>=-127 && sz<=127);
				ss->ds[i++] = (searchLocType) (left_to_right ? x : -x);
				ss->ds[i++] = (searchLocType) (top_to_bottom ? y : -y);
				ss->ds[i++] = (searchLocType) sz;
//				TRACE("     check: dx dy ds %3d %3d %3d\n",
//					ss->ds[i-3], ss->ds[i-2], ss->ds[i-1]);
			}
			left_to_right = 1 - left_to_right;
		}
		top_to_bottom = 1 - top_to_bottom;
	}

	assert(i%N_STATE_VARIABLES==0);
	ss->n_states_to_check = i/N_STATE_VARIABLES;
}

// AndNotBitmap()
// Returns img1 && !(img2)
static void AndNotBitmap(const ImgGray& img1, const ImgGray& img2, ImgGray* output)
{
  const int ISIZEX = img1.Width(), ISIZEY = img1.Height();
	const unsigned char *ptr1, *ptr2;
  unsigned char *ptrout;
	int i;

	ptr1 = img1.Begin();
	ptr2 = img2.Begin();
	ptrout = output->Begin();

	for (i = 0 ; i < ISIZEX*ISIZEY ; i++) {
		*ptrout++ = 255 * (*ptr1 && !(*ptr2));
		ptr1++; ptr2++;
	}
}

void ComputePixelList(const ImgGray& img1,
					  const ImgGray& img2,
					  int xcen, int ycen,
					  PixelList *pl
					  )
{
  const int ISIZEX = img1.Width(), ISIZEY = img1.Height();
	ImgGray img_diff(ISIZEX,ISIZEY);
	const unsigned char *ptr;
	pixelListType *ptro;
	int x, y;
	int i=0;

	AndNotBitmap(img1, img2, &img_diff);

	// Count no. of nonzero pixels
	ptr = img_diff.Begin();
	for (y=0 ; y<ISIZEX*ISIZEY ; y++) {
		if (*ptr++)  i++;
	}
	pl->n_pixels = i;
	pl->pixels = (pixelListType *) malloc(pl->n_pixels * 2 * sizeof(pixelListType));
	
	// Copy nonzero pixels to list
	ptr = img_diff.Begin();
	ptro = pl->pixels;
	for (y=0 ; y<ISIZEY ; y++) {
		for (x=0 ; x<ISIZEX ; x++) {
			if (*ptr++) {
				assert(x-xcen>=-128 && x-xcen<=127);
				assert(y-ycen>=-128 && y-ycen<=127);
				*ptro++ = (pixelListType) (x - xcen);
				*ptro++ = (pixelListType) (y - ycen);
			}
		}
	}
	
}

void** createArray2D(int ncols, int nrows, int nbytes)
{
	char **tt;
	int i;

     tt = (char **) malloc(nrows * sizeof(void *) +
                           ncols * nrows * nbytes);
	 if (tt == NULL) AfxMessageBox(L"(createArray2D) out of memory!!", MB_OK|MB_ICONSTOP, 0);	

     for (i = 0 ; i < nrows ; i++)
          tt[i] = ((char *) tt) + (nrows * sizeof(void *) + 
							i * ncols * nbytes);

	return((void **) tt);
}

void PixelList2DArrayAlloc(PixelList ***ptr, int n_states_to_check)
{
	*ptr = (PixelList **) createArray2D(n_states_to_check, MAX_OUTLINE_SZ+1, sizeof(PixelList));
}

void CopyPixelList(const PixelList& pl1, PixelList* pl2, int dx, int dy)
{
  pl2->n_pixels = pl1.n_pixels;
  pl2->pixels = (pixelListType *) malloc(pl2->n_pixels * 2 * sizeof(pixelListType));
  memcpy(pl2->pixels, pl1.pixels, pl2->n_pixels * 2 * sizeof(pixelListType));
  if (dx !=0)
  {
    pixelListType* p = pl2->pixels;
    for (int i=0 ; i<pl2->n_pixels ; i++)
    {
      *p++ += dx;
      p++;
    }
  }
}

void DrawDifferentialPixelList(const PixelList& add_list, const PixelList& sub_list,
                               ImgBgr* out)
{
  const int w=128, h=96;
  const Bgr add_color(0,255,0);
  const Bgr sub_color(0,0,255);
  int i;
  pixelListType* p;

  out->Reset(w, h);
  Set(out, Bgr(0,0,0));
  const int ww = w / 2;
  const int hh = h / 2;
  // add list
  p = add_list.pixels;
  for (i=0 ; i<add_list.n_pixels ; i++)
  {
    int x = ww + *p++;
    int y = hh + *p++;
    (*out)(x,y) = add_color;
  }
  // sub list
  p = sub_list.pixels;
  for (i=0 ; i<sub_list.n_pixels ; i++)
  {
    int x = ww + *p++;
    int y = hh + *p++;
    (*out)(x,y) = sub_color;
  }
}

////////////////////////
// Given an ellipse size and a search strategy, fills two arrays
// of pixel lists.
//   add_list contains all the pixels in img2 but not in img1, while
//   sub_list contains all the pixels in img1 but not in img2.
void ComputeDifferentialPixelLists(int sz, 
								   SearchStrategy *ss,
								   PixelList add_list[],
								   PixelList sub_list[]
								   ) 
{
  const int ISIZEX = 2*(outlines[MAX_OUTLINE_SZ].halfsize_x+msx)+2;
  const int ISIZEY = 2*(outlines[MAX_OUTLINE_SZ].halfsize_y+msy)+2;
	ImgGray img1(ISIZEX,ISIZEY), img2(ISIZEX,ISIZEY);
	searchLocType *ptr = ss->ds;
	const int xcen=ISIZEX/2, ycen=ISIZEY/2;
	int x1=xcen, y1=ycen, sz1=sz, x2=xcen, y2=ycen, sz2=sz;
	int i;

  // fast indices are used to speed up computation for the most common case
  // of a change in x by +/- 1
  int fast_fwd_indx = -1, fast_bwd_indx = -1;
  int fast_fwd_x1 = -1, fast_bwd_x1 = -1;

	assert(N_STATE_VARIABLES == 3);
	for (i=0 ; i<ss->n_states_to_check ; i++) {
		x2  = xcen + *ptr++;
		y2  = ycen + *ptr++;
		sz2 = sz   + *ptr++;
//    TRACE("asfasf %d %d %d : %d %d %d\n", x1, y1, sz1, x2, y2, sz2);

		// Do not perform computation when sz is out of bounds; this 
		// incompleteness is okay because it will never be needed at
		// runtime.
		if (sz1>=MIN_OUTLINE_SZ && sz1<=MAX_OUTLINE_SZ &&
			sz2>=MIN_OUTLINE_SZ && sz2<=MAX_OUTLINE_SZ) {

// 'fast_fwd' and 'fast_bwd', etc., are my attempt to speed up this 
// computation, which is terribly redundant.  Should be easy to do,
// but needs further testing. -- STB 8/22/05
//      bool fast_fwd = (x2-x1== 1 && y2==y1 && sz2==sz1);
//      bool fast_bwd = (x2-x1==-1 && y2==y1 && sz2==sz1);


//      if (fast_fwd && fast_fwd_indx>=0)
//      {
//        CopyPixelList(add_list[fast_fwd_indx], &add_list[i], x1 - fast_fwd_x1, 0);
////        CopyPixelList(sub_list[fast_fwd_indx], &sub_list[i], 0, 0);
//			  DrawMask(x2, y2, &outlines[sz2], &img2);
//			  ComputePixelList(img1, img2, xcen, ycen, &sub_list[i]);
//      }
//      else if (fast_bwd && fast_bwd_indx>=0)
//      {
//        CopyPixelList(add_list[fast_bwd_indx], &add_list[i]);
//        CopyPixelList(sub_list[fast_bwd_indx], &sub_list[i]);
//      }
//      else
//      {

			  // Center ellipses onto image, then draw
			  DrawMask(x1, y1, &outlines[sz1], &img1);
			  DrawMask(x2, y2, &outlines[sz2], &img2);

			  // Compute two pixel lists
			  ComputePixelList(img2, img1, xcen, ycen, &add_list[i]);
			  ComputePixelList(img1, img2, xcen, ycen, &sub_list[i]);

//        // save fast indices
//        if (fast_fwd && fast_fwd_indx<0)
//        {
//          fast_fwd_indx = i;
//          fast_fwd_x1 = x1;
//        }
//        else if (fast_bwd && fast_bwd_indx<0)
//        {
//          fast_bwd_indx = i;
//          fast_bwd_x1 = x1;
//        }
//      }

		} else {
			add_list[i].n_pixels = -1;
			sub_list[i].n_pixels = -1;
			add_list[i].pixels = NULL;  // added these lines for clarity; should be okay -- STB
			sub_list[i].pixels = NULL;
		}

		x1=x2;  y1=y2;  sz1=sz2;
	}
}

#define PIXELLIST_HEADER_LENGTH 3
char pixellist_header[PIXELLIST_HEADER_LENGTH+1] = "PL2";

void WriteDifferentialPixelLists(const char *fname, PixelList **lists,
								 SearchStrategy *ss) 
{
	searchLocType *ptr = ss->ds;
	int i, sz;
	int n_pix;
	int tmp;
	FILE *fp;

	// Open file for writing
	fp = fopen(fname, "wb");
	if (fp == NULL)
		AfxMessageBox(L"Error: could not open pixel list file for writing!!", MB_OK|MB_ICONSTOP, 0);

	// Write header
	fwrite(pixellist_header, sizeof(char), PIXELLIST_HEADER_LENGTH, fp);

  // Write search range
  fwrite(&msx, sizeof(msx), 1, fp);
  fwrite(&msy, sizeof(msy), 1, fp);
  fwrite(&szinc, sizeof(szinc), 1, fp);

	// Write search strategy
	tmp = N_STATE_VARIABLES;
	fwrite(&tmp, sizeof(int), 1, fp);
	fwrite(&(ss->n_states_to_check), sizeof(int), 1, fp);
	for (i=0 ; i<ss->n_states_to_check ; i++) {
		fwrite(ptr, sizeof(searchLocType), 1, fp);  ptr++;
		fwrite(ptr, sizeof(searchLocType), 1, fp);  ptr++;
		fwrite(ptr, sizeof(searchLocType), 1, fp);  ptr++;
	}

	// Write pixel list
	for (sz=MIN_OUTLINE_SZ ; sz<=MAX_OUTLINE_SZ ; sz++) {
		for (i=0 ; i<ss->n_states_to_check ; i++) {
			n_pix = lists[sz][i].n_pixels;
			fwrite(&n_pix, sizeof(int), 1, fp);
			if (n_pix>=0) {
				tmp = fwrite(lists[sz][i].pixels, sizeof(pixelListType), 2*n_pix, fp);	
				if (tmp != 2*n_pix) 
					AfxMessageBox(L"Error writing pixel list!!", MB_OK|MB_ICONSTOP, 0);
			}
		}
	}
	fclose(fp);
}

// Reads data and allocates memory
void ReadDifferentialPixelLists(const char *fname, 
								PixelList ***lists,
								SearchStrategy *ss) 
{
	char strtmp[PIXELLIST_HEADER_LENGTH+1];
	searchLocType *ptr = ss->ds;
	int i, sz;
	int n_pix;
	int tmp;
	FILE *fp;

	if (*lists != NULL)
		AfxMessageBox(L"Warning: trying to read pixel list into non-empty array!!", MB_OK|MB_ICONEXCLAMATION, 0);

	// Open file for reading
	fp = fopen(fname, "rb");
	if (fp == NULL)
		AfxMessageBox(L"Error: could not open pixel list file for reading!!", MB_OK|MB_ICONSTOP, 0);

	// Read header
	fread(strtmp, sizeof(char), PIXELLIST_HEADER_LENGTH, fp);
	strtmp[PIXELLIST_HEADER_LENGTH] = '\0';
	if (strcmp(strtmp, pixellist_header) != 0)
		AfxMessageBox(L"Error: pixel list file has invalid header!!", MB_OK|MB_ICONSTOP, 0);

  // Read search range
  fread(&msx, sizeof(msx), 1, fp);
  fread(&msy, sizeof(msy), 1, fp);
  fread(&szinc, sizeof(szinc), 1, fp);

	// Read search strategy
	fread(&tmp, sizeof(int), 1, fp);
	if (tmp != N_STATE_VARIABLES)
		AfxMessageBox(L"Error: pixel list file contains wrong # of state variables!!", MB_OK|MB_ICONSTOP, 0);
	fread(&(ss->n_states_to_check), sizeof(int), 1, fp);
	if (ss->n_states_to_check != (2*msx+1)*(2*msy+1)*3)
		AfxMessageBox(L"Warning: pixel list file contains questionable # of states to check!!", MB_OK|MB_ICONEXCLAMATION, 0);
	for (i=0 ; i<ss->n_states_to_check ; i++) {
		fread(ptr, sizeof(searchLocType), 1, fp);  ptr++;
		fread(ptr, sizeof(searchLocType), 1, fp);  ptr++;
		fread(ptr, sizeof(searchLocType), 1, fp);  ptr++;
	}

	// Allocate memory for pixel list array
	PixelList2DArrayAlloc(lists, ss->n_states_to_check);

	// Read pixel list
	for (sz=MIN_OUTLINE_SZ ; sz<=MAX_OUTLINE_SZ ; sz++) {
		for (i=0 ; i<ss->n_states_to_check ; i++) {
			fread(&n_pix, sizeof(int), 1, fp);
			(*lists)[sz][i].n_pixels = n_pix;
			if (n_pix>=0) {
				(*lists)[sz][i].pixels = (pixelListType *) malloc(2*n_pix*sizeof(pixelListType));
				tmp = fread((*lists)[sz][i].pixels, sizeof(pixelListType), 2*n_pix, fp);	
				if (feof(fp))
					AfxMessageBox(L"END OF FILE ENCOUNTERED!!", MB_OK|MB_ICONSTOP, 0);
				if (tmp != 2*n_pix) 
					AfxMessageBox(L"Error reading pixel list!!", MB_OK|MB_ICONSTOP, 0);
			}
		}
	}
	fclose(fp);
}
#undef PIXELLIST_HEADER_LENGTH

void CompareDifferentialPixelLists(PixelList **lists1,
								   PixelList **lists2,
								   SearchStrategy *ss1,
								   SearchStrategy *ss2) 
{
	int i, j, sz;
	int n_pix1, n_pix2;
	pixelListType *ptr1, *ptr2;

	// Compare search strategies
	if (ss1->n_states_to_check != ss2->n_states_to_check)
		AfxMessageBox(L"Error: 'n_states_to_check's are different!!", MB_OK|MB_ICONSTOP, 0);
	for (i=0 ; i<N_STATE_VARIABLES * ss1->n_states_to_check ; i++)
		if (ss1->ds[i] != ss2->ds[i])
			AfxMessageBox(L"Error: Search strategies are different!!", MB_OK|MB_ICONSTOP, 0);

	// Compare pixel lists
	for (sz=MIN_OUTLINE_SZ ; sz<MAX_OUTLINE_SZ ; sz++) {
		for (i=0 ; i<ss1->n_states_to_check ; i++) {
			n_pix1 = lists1[sz][i].n_pixels;
			n_pix2 = lists2[sz][i].n_pixels;
			if (n_pix1 != n_pix2) 
				AfxMessageBox(L"Error: PixelLists are different!!", MB_OK|MB_ICONSTOP, 0);
			ptr1 = lists1[sz][i].pixels;
			ptr2 = lists2[sz][i].pixels;
			for (j=0 ; j<2*n_pix1 ; j++) {
				if (*ptr1++ != *ptr2++)
				AfxMessageBox(L"Error: PixelLists are different!!", MB_OK|MB_ICONSTOP, 0);
			}
		}
	}
}

int grad_scores[MAX_N_STATES_TO_CHECK];
int color_scores[MAX_N_STATES_TO_CHECK];

void ComputeErrorImages(ImgInt error_grad[3], ImgInt error_color[3])
{
	int i, k;
	int x, y, sz;
	int newx, newy, newsz;
	int goodi;

  for (i=0 ; i<3 ; i++)
  {
    error_grad[i].Reset(2*msx+1, 2*msy+1);
    error_color[i].Reset(2*msx+1, 2*msy+1);
  }

	assert(N_STATE_VARIABLES == 3);
	for (sz = -szinc, k=0 ; sz <= szinc ; sz += szinc, k++) {
	  for (y = -msy ; y <= msy ; y++) {
		  for (x = -msx ; x <= msx ; x++) {
			  for (i=0 ; i<search_strategy.n_states_to_check ;i++) {
				  newx  = search_strategy.ds[N_STATE_VARIABLES*i];
				  newy  = search_strategy.ds[N_STATE_VARIABLES*i+1];
				  newsz = search_strategy.ds[N_STATE_VARIABLES*i+2];
				  if (x==newx && y==newy && sz==newsz) {
					  goodi = i;
				  }
			  }
			  if (goodi>=0) {
          assert(k>=0 && k<=3);
          error_grad[k](x+msx, y+msy) = grad_scores[goodi];
          error_color[k](x+msx, y+msy) = color_scores[goodi];
			  }
		  }
    }
	}
}

///////////////////
// searchForHead
//
// INPUTS
// s_old:  old state (x,y,sz)
//
// OUTPUTS
// s_new:  new state (x,y,sz)

void SearchForHead(EllipticalHeadTracker::EllipseState *s_old, const ImgBgr& img, 
				   SearchStrategy *ss, 
				   PixelList addlist[],
				   PixelList sublist[],
				   EllipticalHeadTracker::EllipseState *sgrad,
				   EllipticalHeadTracker::EllipseState *scolor,
				   EllipticalHeadTracker::EllipseState *s_new)
{
  const int ISIZEX = img.Width(), ISIZEY = img.Height();
	int x;
	int y;
	int sz;
	int sz_curr;
	searchLocType *ptr = ss->ds;
	int best_score_grad = -1, best_score_color = -1, best_score = -1;
	int worst_score_grad = 999999, worst_score_color = 999999;
	ColorHistogram ch_ref_scaled, ch_new;
	int i;
	int curr_score;

	assert(s_old->sz>MIN_OUTLINE_SZ && s_old->sz<MAX_OUTLINE_SZ);
	assert(s_old->x >= msx + outlines[s_old->sz+1].halfsize_x);
	assert(s_old->x <= ISIZEX-1 - msx - outlines[s_old->sz+1].halfsize_x);
	assert(s_old->y >= msy + outlines[s_old->sz+1].halfsize_y);
	assert(s_old->y <= ISIZEY-1 - msy - outlines[s_old->sz+1].halfsize_y);
	assert(ss->n_states_to_check <= MAX_N_STATES_TO_CHECK);

	// Preprocessing
  Convert(img, &img_gray);
	if (use_gradient) {
    SmoothGauss5x5(img_gray, &img_gauss);
		if (sum_gradient_magnitude) {
      GradMagPrewitt(img_gauss, &img_grad);
		} else {
      GradPrewittX(img_gauss, &img_gradxf);
      GradPrewittY(img_gauss, &img_gradyf);
		}
	}
	if (use_color) {
		ExtractColorSpace(img, &img_color1, &img_color2, &img_color3);
		// QuantizeColorSpace(&img_color1, &img_color2, &img_color3, &img_color1q, &img_color2q, &img_color3q);
	}

	// Evaluate states based on normalized gradient
	if (use_gradient) {
		if (sum_gradient_magnitude) {
			for (i = 0 ; i < ss->n_states_to_check ; i++) {
				x  = s_old->x  + *ptr++;  y  = s_old->y  + *ptr++;  sz = s_old->sz + *ptr++;
				grad_scores[i]  = NormalizedSumOfGradientMagnitude(x, y, &outlines[sz], img_grad);
			}
		} else { // sum dot product
			for (i = 0 ; i < ss->n_states_to_check ; i++) {
				x  = s_old->x  + *ptr++;  y  = s_old->y  + *ptr++;  sz = s_old->sz + *ptr++;
				grad_scores[i]  = NormalizedSumOfGradientDotProduct(x, y, &outlines[sz], 
						img_gradxf, img_gradyf);
			}
		}
	}

	// Display likelihood
//	if (use_color && show_details) {
//		DrawColorLikelihoodMap(img, &ch_ref, &img_likelihood);
//		ComputeColorHistogramOfWholeImage(&img_color1, &img_color2, &img_color3, &ch_background);
//		DrawHistogramBackprojection(img, &ch_ref, &ch_background, &img_backprojection);
//	}
	
	// Evaluate states based on color histogram
	if (use_color) {
		x = s_old->x;  y = s_old->y;  sz = s_old->sz;
		sz_curr = sz+ss->ds[2];
		ComputeColorHistogram(img_color1, img_color2, img_color3, x+ss->ds[0], y+ss->ds[1], 
							&outlines[sz_curr], &ch_new);
		assert(SizeOfColorHistogram(&ch_new) == outlines[sz_curr].area);
		NormalizeColorHistogram(&ch_ref, outlines[sz_curr].area, 
								outlines[CH_MODEL_SZ].area, &ch_ref_scaled);
		curr_score = ColorHistogramIntersection(&ch_ref_scaled, &ch_new);
		color_scores[0] = (curr_score * 10000) / outlines[sz_curr].area;

		for (i = 1 ; i < ss->n_states_to_check ; i++) {
			if (sz + ss->ds[N_STATE_VARIABLES*i+2] != sz_curr) {
				sz_curr = sz + ss->ds[N_STATE_VARIABLES*i+2];
				NormalizeColorHistogram(&ch_ref, outlines[sz_curr].area, 
								outlines[CH_MODEL_SZ].area, &ch_ref_scaled);
				UpdateColorHistogram(img_color1, img_color2, img_color3, x, y, &addlist[i], &sublist[i], &ch_new);
				assert(SizeOfColorHistogram(&ch_new) == outlines[sz_curr].area);
				curr_score = ColorHistogramIntersection(&ch_ref_scaled, &ch_new);
				color_scores[i] = (curr_score * 10000) / outlines[sz_curr].area;
			} else {
				UpdateColorHistogramAndIntersection(img_color1, img_color2, img_color3, x, y, 
								&addlist[i], &sublist[i], &ch_ref_scaled, &ch_new, &curr_score);
				assert(SizeOfColorHistogram(&ch_new) == outlines[sz_curr].area);
				color_scores[i] = (curr_score * 10000) / outlines[sz_curr].area;
			}

#if 0 // useful for debugging pixel lists
      {
        ColorHistogram ch_tmp;
			  ComputeColorHistogram(img_color1, img_color2, img_color3, x+ss->ds[N_STATE_VARIABLES*i], y+ss->ds[N_STATE_VARIABLES*i+1], 
							  &outlines[sz_curr], &ch_tmp);
			  BOOL tmp = AreColorHistogramsEqual(&ch_new, &ch_tmp);
			  if (!tmp) {
          assert(0); // Color Histograms are Not Equal!!
			  }
			  tmp = ColorHistogramIntersection(&ch_ref_scaled, &ch_tmp);
			  if (tmp != curr_score) {
          assert(0);  // Current score is incorrect!!
			  }
      }
#endif
		}
	}

	// Compute best and worst scores for grad and color alone
	for (i = 0 ; i < ss->n_states_to_check ; i++) {
		if (use_gradient) {
			curr_score = grad_scores[i];
			if (curr_score > best_score_grad) {
				best_score_grad = curr_score;
				sgrad->x  = s_old->x  + ss->ds[N_STATE_VARIABLES*i];
				sgrad->y  = s_old->y  + ss->ds[N_STATE_VARIABLES*i+1];
				sgrad->sz = s_old->sz + ss->ds[N_STATE_VARIABLES*i+2];
			}
			if (curr_score < worst_score_grad) {
				worst_score_grad = curr_score;
			}
		}
		if (use_color) {
			curr_score = color_scores[i];
			if (curr_score > best_score_color) {
				best_score_color = curr_score;
				scolor->x  = s_old->x  + ss->ds[N_STATE_VARIABLES*i];
				scolor->y  = s_old->y  + ss->ds[N_STATE_VARIABLES*i+1];
				scolor->sz = s_old->sz + ss->ds[N_STATE_VARIABLES*i+2];
			}
			if (curr_score < worst_score_color) {
				worst_score_color = curr_score;
			}
		}
	}

	// Select best state
	float grad_fact  = 1000.0f/(best_score_grad  - worst_score_grad);
	float color_fact = 1000.0f/(best_score_color - worst_score_color);
	if (!use_gradient) grad_fact = 0.0f;
	if (!use_color) color_fact = 0.0f;
	for (i = 0 ; i < ss->n_states_to_check ; i++) {
		curr_score = (int) ((grad_scores[i]  - worst_score_grad)  * grad_fact +
			                  (color_scores[i] - worst_score_color) * color_fact);
		if (curr_score > best_score) {
			best_score = curr_score;
			s_new->x  = s_old->x  + ss->ds[N_STATE_VARIABLES*i];
			s_new->y  = s_old->y  + ss->ds[N_STATE_VARIABLES*i+1];
			s_new->sz = s_old->sz + ss->ds[N_STATE_VARIABLES*i+2];
		}
	}

//  TRACE(" old:(%3d,%3d,%3d)  new:(%3d,%3d,%3d)  grad:(%3d,%3d,%3d)  color:(%3d,%3d,%3d)\n",
//    s_old->x, s_old->y, s_old->sz, s_new->x, s_new->y, s_new->sz,
//    sgrad->x, sgrad->y, sgrad->sz, scolor->x, scolor->y, scolor->sz);
//	TRACE("using grad %d %s, using color %d    ",
//		use_gradient, sum_gradient_magnitude?"MAG":"DOT", use_color);
//	TRACE("grad: <%4d,%4d>, color: <%4d,%4d>       ", 
//		worst_score_grad, best_score_grad, worst_score_color, best_score_color);

  if (show_details) {
    static Figure fig_grad("Gradient middle size"), fig_color("Color middle size");
    ComputeErrorImages(error_grad, error_color);
    fig_grad.Draw(error_grad[1]);
    fig_color.Draw(error_color[1]);
    fig_grad.SetWindowSize(200, 300);
    fig_color.SetWindowSize(200, 300);
    fig_color.PlaceToTheRightOf(fig_grad);
  }
}

void KeepStateInBounds(EllipticalHeadTracker::EllipseState *s, int ISIZEX, int ISIZEY)
{
	s->sz = max(s->sz, MIN_OUTLINE_SZ+1);
	s->sz = min(s->sz, MAX_OUTLINE_SZ-1);
	s->x  = max(s->x, msx + outlines[s->sz+1].halfsize_x);
	s->x  = min(s->x, ISIZEX-1 - msx - outlines[s->sz+1].halfsize_x);
	s->y  = max(s->y, msy + outlines[s->sz+1].halfsize_y);
	s->y  = min(s->y, ISIZEY-1 - msy - outlines[s->sz+1].halfsize_y);
}

void InitTracker()
{
	static BOOL first_time = TRUE;
	int i;

	if (!first_time) 
		AfxMessageBox(L"Warning:  InitTracker called more than once!!", MB_OK|MB_ICONEXCLAMATION, 0);	
	first_time = FALSE;

	// Initialize elliptical outlines
	for (i = MIN_OUTLINE_SZ ; i <= MAX_OUTLINE_SZ ; i++)  {
		MakeEllipticalOutline(&outlines[i], i, (int) (aspect_ratio * i));
	}

  FILE *fpa, *fps;
  fpa = fopen(fname_pixellist_add, "rb");
  fps = fopen(fname_pixellist_sub, "rb");
  if (fpa && fps)
  {
    ReadDifferentialPixelLists(fname_pixellist_add, &add_lists, &search_strategy);
    ReadDifferentialPixelLists(fname_pixellist_sub, &sub_lists, &search_strategy);
  }
  else
  {
    if (fpa)  fclose(fpa);
    if (fps)  fclose(fps);
	  MakeSearchStrategy(&search_strategy);
	  PixelList2DArrayAlloc(&add_lists, search_strategy.n_states_to_check);
	  PixelList2DArrayAlloc(&sub_lists, search_strategy.n_states_to_check);
	  for (i = MIN_OUTLINE_SZ ; i <= MAX_OUTLINE_SZ ; i++)  {
		  ComputeDifferentialPixelLists(i, &search_strategy, add_lists[i], sub_lists[i]); 
	  }
	  WriteDifferentialPixelLists(fname_pixellist_add, add_lists, &search_strategy);
	  WriteDifferentialPixelLists(fname_pixellist_sub, sub_lists, &search_strategy);
  }

//#ifdef COMPUTE_PIXEL_LISTS
//	MakeSearchStrategy(&search_strategy);
//	PixelList2DArrayAlloc(&add_lists, search_strategy.n_states_to_check);
//	PixelList2DArrayAlloc(&sub_lists, search_strategy.n_states_to_check);
//	for (i = MIN_OUTLINE_SZ ; i <= MAX_OUTLINE_SZ ; i++)  {
//		ComputeDifferentialPixelLists(i, &search_strategy, add_lists[i], sub_lists[i]); 
//	}
//	WriteDifferentialPixelLists("lists_add.dat", add_lists, &search_strategy);
//	WriteDifferentialPixelLists("lists_sub.dat", sub_lists, &search_strategy);
//#else
//	if (msx==8 && msy==8) {
//		ReadDifferentialPixelLists("lists_add8x8.dat", &add_lists, &search_strategy);
//		ReadDifferentialPixelLists("lists_sub8x8.dat", &sub_lists, &search_strategy);
//	} else if (msx==4 && msy==4) {
//		ReadDifferentialPixelLists("lists_add4x4.dat", &add_lists, &search_strategy);
//		ReadDifferentialPixelLists("lists_sub4x4.dat", &sub_lists, &search_strategy);
//	} else if (msx==16 && msy==16) {
//		ReadDifferentialPixelLists("lists_add16x16.dat", &add_lists, &search_strategy);
//		ReadDifferentialPixelLists("lists_sub16x16.dat", &sub_lists, &search_strategy);
//	} else if (msx==16 && msy==4) {
//		ReadDifferentialPixelLists("lists_add16x4.dat", &add_lists, &search_strategy);
//		ReadDifferentialPixelLists("lists_sub16x4.dat", &sub_lists, &search_strategy);
//	} else if (msx==2 && msy==2) {
//		ReadDifferentialPixelLists("lists_add2x2.dat", &add_lists, &search_strategy);
//		ReadDifferentialPixelLists("lists_sub2x2.dat", &sub_lists, &search_strategy);
//	} else {
//		AfxMessageBox("msx and msy are invalid!!", MB_OK|MB_ICONSTOP, 0);	
//		// Read anyway, just so we don't get any memory faults
//		ReadDifferentialPixelLists("lists_add8.dat", &add_lists, &search_strategy);
//		ReadDifferentialPixelLists("lists_sub8.dat", &sub_lists, &search_strategy);
//	}

//#endif

//  if (0)
//  {  // visualize, for testing
//    Figure fig;
//    ImgBgr tmp;
//    for (int i = MIN_OUTLINE_SZ ; i <= MAX_OUTLINE_SZ ; i++)
//    {
//    	for (int j=0 ; j<search_strategy.n_states_to_check ; j++)
//      {
//        PixelList& ap = add_lists[i][j];
//        PixelList& sp = sub_lists[i][j];
//        if (ap.n_pixels>0 || sp.n_pixels>0)
//        {
//          DrawDifferentialPixelList(add_lists[i][j], sub_lists[i][j], &tmp);
//          fig.Draw(tmp);
//          fig.GrabMouseClick();
//        }
//      }
//    }
//  }
//	ReadDifferentialPixelLists("lists_add.dat", &add_lists_tmp, &ss_tmp);
//	ReadDifferentialPixelLists("lists_sub.dat", &sub_lists_tmp, &ss_tmp);
//	CompareDifferentialPixelLists(add_lists, add_lists_tmp, &search_strategy, &ss_tmp);
//	CompareDifferentialPixelLists(sub_lists, sub_lists_tmp, &search_strategy, &ss_tmp);
}

void UninitTracker()
{
	int i, j;

	for (i = MIN_OUTLINE_SZ ; i <= MAX_OUTLINE_SZ ; i++)  {
		free( outlines[i].mask );
		free( outlines[i].perim );
		free( outlines[i].normals );

  	for (j = 0 ; j < search_strategy.n_states_to_check ; j++) {
      PixelList aa = add_lists[i][j];
      if (aa.n_pixels > 0)
      {
	  	  free( add_lists[i][j].pixels );
	  	  free( sub_lists[i][j].pixels );
      }
    }
	}

  free( add_lists );
	free( sub_lists );
}

void TrackHead(const ImgBgr& img, EllipticalHeadTracker::EllipseState *curr_state, int* xvel, int* yvel)
{
	EllipticalHeadTracker::EllipseState grad_state, color_state, new_state;

	curr_state->x += *xvel;
	curr_state->y += *yvel;
	KeepStateInBounds(curr_state, img.Width(), img.Height());
	SearchForHead(curr_state, 
				  img, 
				  &search_strategy, 
				  add_lists[curr_state->sz],
				  sub_lists[curr_state->sz],
				  &grad_state,
				  &color_state,
				  &new_state);
	*xvel = new_state.x - curr_state->x;
	*yvel = new_state.y - curr_state->y;
	curr_state->x = new_state.x;
	curr_state->y = new_state.y;
	curr_state->sz = new_state.sz;
}

};
// ================< end local functions


namespace blepo
{

EllipticalHeadTracker::EllipticalHeadTracker()
{
  m_xvel = 0;
  m_yvel = 0;

  // precomputes variables to speed up on-line search
  InitTracker();
//  ReadColorHistogram("model_stanb.dat", &ch_ref);
//  ReadColorHistogram("model.dat", &ch_ref);
}

EllipticalHeadTracker::~EllipticalHeadTracker()
{
  UninitTracker();
}

void ResizeImages(int width, int height, bool force_resize)
{
  if (force_resize || width != img_gray.Width() || height != img_gray.Height())
  {
    // be sure to list here all the images that appear at the top of this file
    img_gray.Reset(width, height);
    img_gauss.Reset(width, height);
    img_grad.Reset(width, height);
    img_gradxf.Reset(width, height);
    img_gradyf.Reset(width, height);
    img_outline.Reset(width, height);
    img_outline_grad.Reset(width, height);
    img_outline_color.Reset(width, height);
    img_outline8.Reset(width, height);
    img_outline_grad8.Reset(width, height);
    img_outline_color8.Reset(width, height);
    img_mask.Reset(width, height);
    img_color1.Reset(width, height);
    img_color2.Reset(width, height);
    img_color3.Reset(width, height);
  }
}

void EllipticalHeadTracker::BuildModel(const ImgBgr& img, const EllipticalHeadTracker::EllipseState& s)
{
  assert(s.sz >= MIN_OUTLINE_SZ && s.sz <= MAX_OUTLINE_SZ);
  ResizeImages(img.Width(), img.Height(), false);
  // compute model color histogram
  ExtractColorSpace(img, &img_color1, &img_color2, &img_color3);
  ComputeColorHistogram(img_color1, img_color2, img_color3, s.x, s.y, &outlines[s.sz], &ch_ref);
}

void EllipticalHeadTracker::SaveModel(const char* filename)
{
  WriteColorHistogram(filename, &ch_ref); 
}

void EllipticalHeadTracker::LoadModel(const char* filename)
{
  ReadColorHistogram(filename, &ch_ref);
}

EllipticalHeadTracker::EllipseState EllipticalHeadTracker::Track(const ImgBgr& img, int use_gradient, int use_color_histogram)
{
  ResizeImages(img.Width(), img.Height(), false);
  TrackHead(img, &m_state, &m_xvel, &m_yvel);

  return m_state;
}

void EllipticalHeadTracker::Init(const ImgBgr& img, const EllipseState& initial_state)
{
  ResizeImages(img.Width(), img.Height(), false);
  SetState(initial_state);
  BuildModel(img, initial_state);
}

void EllipticalHeadTracker::Init(const EllipseState& initial_state, const char* filename)
{
  SetState(initial_state);
  LoadModel(filename);
}


void EllipticalHeadTracker::OverlayEllipse(const EllipseState& state, ImgBgr* img)
{
  OverlayOutlineBGR24(img, state, Bgr(0, 0, 255));
}

int EllipticalHeadTracker::GetMinSize() const
{
  return MIN_OUTLINE_SZ;
}

int EllipticalHeadTracker::GetMaxSize() const
{
  return MAX_OUTLINE_SZ;
}

void EllipticalHeadTracker::GetSearchRange(int* x, int* y, int* size)
{
  *x = msx;
  *y = msy;
  *size = szinc;
}

};  // end namespace blepo

