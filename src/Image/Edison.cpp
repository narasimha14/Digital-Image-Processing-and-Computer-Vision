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

#include "edison/segm/msImageProcessor.h"
#include "ImageOperations.h"  // Bgr2Rgb
#include "ImageAlgorithms.h"
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

// Mean shift segmentation, as implemented by Chris M. Christoudias and Bogdan Georgescu
// at Rutgers University.  
// http://www.caip.rutgers.edu/riul/research/code/EDISON/index.html
int MeanShiftSegmentation(const ImgBgr& img, ImgInt* out_labels, ImgBgr* out_meancolors, const MeanShiftSegmentationParams& params)
{
  CWaitCursor wait;
  msImageProcessor ip;
  BgrToRgb(img, out_meancolors);
  ip.DefineImage(out_meancolors->BytePtr(), COLOR, img.Height(), img.Width());
  SpeedUpLevel speedup = (params.speedup==0) ? NO_SPEEDUP : ((params.speedup==1) ? MED_SPEEDUP : HIGH_SPEEDUP);
  ip.Segment(params.sigma_spatial, params.sigma_color, params.minregion, speedup);

  
  // Get results as mean color of region for each pixel
  ip.GetResults(out_meancolors->BytePtr());
  BgrToRgb(*out_meancolors, out_meancolors);

  // Get results as region number for each pixel
  int* labels;
  float* modes;
  int* modePointCounts;
  int nregions = ip.GetRegions(&labels, &modes, &modePointCounts);

  out_labels->Reset( img.Width(), img.Height() );
  int* p = labels;
  ImgInt::Iterator q = out_labels->Begin();
  while (q != out_labels->End())
  {
    *q++ = *p++;
  }

  delete[] labels;
  delete[] modes;
  delete[] modePointCounts;

  return nregions;
}

};  // end namespace blepo

