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

#include "ImageAlgorithms.h"
#include "ImageOperations.h"  // Interp
#include "Matrix/MatrixOperations.h"
#include "Matrix/LinearAlgebra.h"  // SolveLinear
#include "Utilities/Math.h"  // SolveLinear
#include "math.h"

// ================> begin local functions (available only to this translation unit)
namespace
{

// status indices

const int _tracked = 0;
const int _not_found = 1;
const int _small_det = 2;
const int _max_iterations = 3;
const int _oob = 4;
const int _large_residue = 5;
const int _small_region = 6;
const int _border_or_nonexistent_region = 7;

using namespace blepo;

struct PairFloat
{
  float x;
  float y;
};

float iSumAbsFloatVect(const std::vector<float>& in)
{
  std::vector<float>::const_iterator i;
  float sum = 0.0f;
  for(i=in.begin(); i!=in.end(); i++)
  {
    sum += fabsf(*i);
  }

  return sum;
}

void iConvertToPairFloat(
  const std::vector<Point>& in,
  std::vector<PairFloat>* out)
{
  PairFloat tmp;
  std::vector<Point>::const_iterator i;
  for(i=in.begin(); i!=in.end(); i++)
  {
    tmp.x = (float) (i->x);
    tmp.y = (float) (i->y);
    
    out->push_back(tmp);
  }
}

PairFloat iFindCentroid(const std::vector<PairFloat>& in)
{
  std::vector<PairFloat>::const_iterator i;
  float length = (float) in.size();
  assert(length > 0.0f);

  float sumx = 0.0f;
  float sumy = 0.0f;
  for(i=in.begin(); i!=in.end(); i++)
  {
    sumx += (i->x);
    sumy += (i->y);
  }

  PairFloat tmp;
  tmp.x = sumx/length;
  tmp.y = sumy/length;
  
  return tmp;
}

void iShiftOriginToCentroid(
  const std::vector<PairFloat>& in,
  std::vector<PairFloat>* out)
{
  PairFloat in_centroid = iFindCentroid(in);
  
  PairFloat tmp;
  std::vector<PairFloat>::const_iterator i;
  for(i=in.begin(); i!=in.end(); i++)
  {
    tmp.x = (i->x) - in_centroid.x;
    tmp.y = (i->y) - in_centroid.y;

    out->push_back(tmp);
  }
}

void iShiftOrigin(
  const std::vector<PairFloat>& in,
  const PairFloat offset,
  std::vector<PairFloat>* out)
{
  PairFloat tmp;
  std::vector<PairFloat>::const_iterator i;
  for(i=in.begin(); i!=in.end(); i++)
  {
    tmp.x = (i->x) + offset.x;
    tmp.y = (i->y) + offset.y;

    out->push_back(tmp);
  }
}

void iShrinkImage(
  const ImgInt& in, 
  const int shrink_width, const int shrink_height,
  const bool label_increment,
  ImgInt* out)
{
  assert(shrink_width%2 == 0); // must be even
  assert(shrink_height%2 == 0); // must be even
  assert(in.Width() > shrink_width);
  assert(in.Height() > shrink_height);

  out->Reset(in.Width(), in.Height());
  Set(out, 0);
  Rect box((shrink_width/2), (shrink_height/2),
           (in.Width() - (shrink_width/2)), (in.Height() - (shrink_height/2)));

  int add = 0;
  if(label_increment) add++;

  ImgInt::ConstRectIterator in_p = in.BeginRect(box);
  ImgInt::RectIterator out_p=out->BeginRect(box);
  while(!in_p.AtEnd())
  {
    *out_p = *in_p + add;
    out_p++;
    in_p++;
  }
}
    
void iGetAffineTransformLocations(
  const std::vector<PairFloat>& in, 
  const MatFlt& A, const MatFlt& d,
  std::vector<PairFloat>* out)
{
  MatFlt tmplocin,tmplocout;
  PairFloat tmp;

  std::vector<PairFloat>::const_iterator i;
  for(i=in.begin(); i!=in.end(); i++)
  {
    tmplocin.Reset(1,2);
    tmplocin(0,0) = i->x;
    tmplocin(0,1) = i->y;

    tmplocout.Reset(1,2);
    tmplocout = (A * tmplocin) + d;
    tmp.x = tmplocout(0,0);
    tmp.y = tmplocout(0,1);
    
    out->push_back(tmp);
  }
}

void iGetRatioOOB(
  const std::vector<PairFloat>& in,
  const float x_min, const float y_min,
  const float x_max, const float y_max,
  float* ratio)
{
  float countOOB = 0;
  std::vector<PairFloat>::const_iterator i;
  for(i=in.begin(); i!=in.end(); i++)
    if( ((i->x) < x_min) || ((i->x) > (x_max-1.0f)) ||
        ((i->y) < y_min) || ((i->y) > (y_max-1.0f)) )
      countOOB++;
	
		*ratio = (countOOB/((float) in.size()));
}

void iGetIntensityDifference(
  const ImgFloat& img1,
  const ImgFloat& img2,
  const std::vector<PairFloat>& loc1,
  const std::vector<PairFloat>& loc2,
  std::vector<float>* intdiff)
{
  assert(loc1.size() == loc2.size());
  
  const float xlim = (float) img1.Width();
  const float ylim = (float) img1.Height();

  float I1, I2;
  std::vector<PairFloat>::const_iterator i,j;
  j = loc2.begin();
  for(i=loc1.begin(); i!=loc1.end(); i++)
  {
    if( ((i->x) >= 0.0f) && ((i->x) <= (xlim-1.0f)) &&
        ((i->y) >= 0.0f) && ((i->y) <= (ylim-1.0f)) )
      I1 = Interp(img1, i->x, i->y);
    else
      I1 = 0.0f;

    if( ((j->x) >= 0.0f) && ((j->x) <= (xlim-1.0f)) &&
        ((j->y) >= 0.0f) && ((j->y) <= (ylim-1.0f)) )
      I2 = Interp(img2, j->x, j->y);
    else
      I2 = 0.0f; 

    intdiff->push_back(I1-I2);
    j++;
  }
}

void iGetGradientVectors(
  const ImgFloat& gradx,
  const ImgFloat& grady,
  const std::vector<PairFloat>& loc,
  std::vector<PairFloat>* grad)
{
  const float xlim = (float) gradx.Width();
  const float ylim = (float) gradx.Height();
  
  PairFloat g;
  std::vector<PairFloat>::const_iterator i;
  for(i=loc.begin(); i!=loc.end(); i++)
  {
    if( ((i->x) >= 0.0f) && ((i->x) <= (xlim-1.0f)) &&
        ((i->y) >= 0.0f) && ((i->y) <= (ylim-1.0f)) )
    {
      g.x = Interp(gradx, i->x, i->y);
      g.y = Interp(grady, i->x, i->y);
    }
    else
    {
      g.x = 0.0f;
      g.y = 0.0f;
    }

    grad->push_back(g);
  }
}

void iCompute6by6GradientMatrix(
  const std::vector<PairFloat>& loc,
  const std::vector<PairFloat>& grad,
  MatFlt* T)
{
  assert(loc.size() == grad.size());

  T->Reset(6,6);
  int i,j;
  for(j=0; j<6; j++)
    for(i=0; i<6; i++)
      (*T->Begin(i,j)) = 0.0f;

  float x,y,x2,y2,gx,gy,gx2,gy2;
  std::vector<PairFloat>::const_iterator k, r;
  r = grad.begin();
  for(k=loc.begin(); k!=loc.end(); k++)
  {
    x = k->x; y = k->y; 
    x2 = x*x; y2 = y*y;
    gx = r->x; gy = r->y;
    gx2 = gx*gx; gy2 = gy*gy;

    (*T->Begin(0,0)) += x2*gx2;
    (*T->Begin(1,0)) += x*y*gx2; 
    (*T->Begin(2,0)) += x*gx2;
    (*T->Begin(3,0)) += x2*gx*gy;
    (*T->Begin(4,0)) += x*y*gx*gy;
    (*T->Begin(5,0)) += x*gx*gy;

    (*T->Begin(1,1)) += y2*gx2;
    (*T->Begin(2,1)) += y*gx2;
    (*T->Begin(3,1)) += x*y*gx*gy;
    (*T->Begin(4,1)) += y2*gx*gy;
    (*T->Begin(5,1)) += y*gx*gy;

    (*T->Begin(2,2)) += gx2;
    (*T->Begin(3,2)) += x*gx*gy;
    (*T->Begin(4,2)) += y*gx*gy;
    (*T->Begin(5,2)) += gx*gy;

    (*T->Begin(3,3)) += x2*gy2;
    (*T->Begin(4,3)) += x*y*gy2;
    (*T->Begin(5,3)) += x*gy2;
    
    (*T->Begin(4,4)) += y2*gy2;
    (*T->Begin(5,4)) += y*gy2;

    (*T->Begin(5,5)) += gy2;

    r++;
  }
  
  for (j = 1 ; j < 6 ; j++)
    for (i = 0 ; i < j ; i++)
      (*T->Begin(i,j)) = (*T->Begin(j,i));
}

void iCompute6by1ErrorVector(
  const std::vector<PairFloat>& loc,
  const std::vector<PairFloat>& grad,
  const std::vector<float>& intdiff,
  MatFlt* e)
{
  assert(loc.size() == grad.size());
  assert(loc.size() == intdiff.size());

  e->Reset(1,6);
  for(int j=0; j<6; j++)
    (*e->Begin(0,j)) = 0.0f;

  std::vector<PairFloat>::const_iterator k, r;
  std::vector<float>::const_iterator t;
  r = grad.begin();
  t = intdiff.begin();
  for(k=loc.begin(); k!=loc.end(); k++)
  {
    (*e->Begin(0,0)) += (k->x) * (*t) * (r->x);
    (*e->Begin(0,1)) += (k->y) * (*t) * (r->x);
    (*e->Begin(0,2)) += (*t) * (r->x);

    (*e->Begin(0,3)) += (k->x) * (*t) * (r->y);
    (*e->Begin(0,4)) += (k->y) * (*t) * (r->y);
    (*e->Begin(0,5)) += (*t) * (r->y);

    r++;
    t++;
  }

}

void iSolveAffineEquation(
  const MatFlt& T,
  const MatFlt& e,
  MatFlt* delta)
{
  MatDbl TD, eD, deltaD;

  Convert(T,&TD);
  Convert(e,&eD);

  SolveLinear(TD,eD,&deltaD);
  Convert(deltaD,delta);
}

float iDeterminant(const MatFlt& mat)
{
  MatDbl tmp;
  Convert(mat,&tmp);
  return (float) Determinant(tmp);
} 

};
// ================< end local functions

namespace blepo
{

void LucasKanadeAffineRegions(
  const ImgFloat& img1,
  const ImgFloat& img2,
  const ImgInt& img_label,
  const LucasKanadeAffineParameters& params,
  const std::vector<MatFlt>& A_init_regions,
  const std::vector<MatFlt>& d_init_regions,
  std::vector<LucasKanadeAffineResult>* result_regions)
{
  assert( (img1.Width()==img2.Width()) && (img1.Height()==img2.Height()) );
  assert( (img1.Width()==img_label.Width()) && (img1.Height() == img_label.Height()) );

  float width = (float) img1.Width();
  float height = (float) img1.Height();

  ImgFloat img1_smoothed, img2_smoothed;
  Smooth(img1,params.smooth_sigma,&img1_smoothed);
  Smooth(img2,params.smooth_sigma,&img2_smoothed);

  ImgFloat gradx, grady;
  Gradient(img2_smoothed, params.gradient_sigma, &gradx, &grady);

  int min_label = Min(img_label);
  int max_label = Max(img_label);

  assert(min_label == 0);
  assert(max_label > 0);

  bool label_increment;
  if(params.skip_label_zero)
  {
    label_increment = false;
    min_label = 1;
  }
  else
  {
    label_increment = true; // labels are shiftep up by 1 index
    min_label = 1;
    max_label = max_label + 1;
  }

	int shrink_width = blepo_ex::Round( width*(params.shrink_width_ratio) );
  if(shrink_width%2 != 0) shrink_width++; // making it even
  int shrink_height = blepo_ex::Round( height*(params.shrink_height_ratio) );
  if(shrink_height%2 != 0) shrink_height++; // making it even

  ImgInt img_label_shrunk;
  iShrinkImage(img_label, shrink_width, shrink_height, label_increment, &img_label_shrunk);
  
  assert(A_init_regions.size() == (max_label - min_label + 1)); 
  assert(d_init_regions.size() == (max_label - min_label + 1)); 

  std::vector<MatFlt>::const_iterator ai_p = A_init_regions.begin();
  std::vector<MatFlt>::const_iterator di_p = d_init_regions.begin();

  std::vector<Point> loc_int; // coordinate locations in integral values
  std::vector<PairFloat> loc; // coordinate locations in float values
  std::vector<PairFloat> loc_region; // coordinate locations with region's centroid as origin
  MatFlt A_zeros(2,2);
  MatFlt d_zeros(1,2);
  Set(&A_zeros, 0.0f);
  Set(&d_zeros, 0.0f);
  MatFlt A,d;
  LucasKanadeAffineResult result;
  for(int i=min_label; i<=max_label; i++) // start label loop
  {
    loc_int.resize(0);
    FindPixels(img_label_shrunk,i,&loc_int);

    // if region does not exist skip affine calculations
    if(loc_int.size() == 0)
    {
      result.status = _border_or_nonexistent_region;
      result.A = A_zeros;
      result.d = d_zeros;
      result.residue = 0.0f;
      result.centroid_x = 0.0f;
      result.centroid_y = 0.0f;
      result_regions->push_back(result);
      continue;
    }

    loc.resize(0);
    iConvertToPairFloat(loc_int, &loc);
    loc_region.resize(0); 
    iShiftOriginToCentroid(loc,&loc_region);

    PairFloat loc_centroid = iFindCentroid(loc);
    result.centroid_x = loc_centroid.x;
    result.centroid_y = loc_centroid.y;

    MatFlt loc_centroid_MatFlt(1,2);
    loc_centroid_MatFlt(0,0) = loc_centroid.x;
    loc_centroid_MatFlt(0,1) = loc_centroid.y;

    // if region is small skip affine calculations
    if((int)loc_region.size() < params.min_num_pixels)
    {
      result.status = _small_region;
      result.A = A_zeros;
      result.d = d_zeros;
      result.residue = 0.0f;
      result_regions->push_back(result);
      continue;
    }
    
    // Initializing affine parameter variables A and d
    A = *ai_p;
    d = *di_p;
    ai_p++;
    di_p++;

    assert(A.Width() == 2 && A.Height() == 2);
    assert(d.Width() == 1 && d.Height() == 2);

    float ratio_oob;
    float residue_per_pixel;
    bool oob_t = false, smalldet_t = false, notfound_t = false, tracked_t = false;
    MatFlt T,e,delta,delta_p;
    delta_p.Reset(1,6);
    Set(&delta_p,0.0f);
    float T_det;
    std::vector<float> intdiff;
    std::vector<PairFloat> grad;
    for(int iter=0; iter<params.num_iterations; iter++) // start iteration
    {
      std::vector<PairFloat> loc_aff_region;
      loc_aff_region.resize(0);
      iGetAffineTransformLocations(loc_region,A,d,&loc_aff_region);
      std::vector<PairFloat> loc_aff;
      loc_aff.resize(0);
      iShiftOrigin(loc_aff_region,loc_centroid,&loc_aff);

      ratio_oob = 0.0f;
      iGetRatioOOB(loc_aff,0.0f,0.0f,width,height,&ratio_oob);
      if(ratio_oob > params.oob_threshold)
      {
        oob_t = true;
        break;
      }

      intdiff.resize(0);
      iGetIntensityDifference(img1_smoothed,img2_smoothed,loc,loc_aff,&intdiff);

      grad.resize(0);
      iGetGradientVectors(gradx,grady,loc_aff,&grad);

      T.Reset(6,6);
      iCompute6by6GradientMatrix(loc_region,grad,&T);

      T_det = iDeterminant(T);
      if(T_det < params.det_threshold)
      {
        smalldet_t = true;
        break;
      }
        
      e.Reset(1,6);
      iCompute6by1ErrorVector(loc_region,grad,intdiff,&e);

      delta.Reset(1,6);
      iSolveAffineEquation(T,e,&delta);

      residue_per_pixel = iSumAbsFloatVect(intdiff)/((float) intdiff.size());
      TRACE("residue is %f\n",residue_per_pixel);

      if( (delta(0,0)>params.max_aff_threshold) || (delta(0,1)>params.max_aff_threshold) || 
          (delta(0,3)>params.max_aff_threshold) || (delta(0,4)>params.max_aff_threshold) )
      {
        notfound_t = true;
        break;
      }
      if( (delta(0,2)>params.max_disp_threshold) || (delta(0,5)>params.max_disp_threshold) )
      {
        notfound_t =true;
        break;
      }
      
      if( (fabsf(delta(0,0)-delta_p(0,0)) < params.min_aff_threshold) &&
          (fabsf(delta(0,1)-delta_p(0,1)) < params.min_aff_threshold) &&
          (fabsf(delta(0,3)-delta_p(0,3)) < params.min_aff_threshold) &&
          (fabsf(delta(0,4)-delta_p(0,4)) < params.min_aff_threshold) &&
          (fabsf(delta(0,2)-delta_p(0,2)) < params.min_disp_threshold) &&
          (fabsf(delta(0,5)-delta_p(0,5)) < params.min_disp_threshold) )
      {
        if(residue_per_pixel < params.residue_threshold)
          tracked_t = true;
        break;
      }

      for(int j=0; j<6; j++)
        delta_p(0,j) = delta(0,j); 

      A(0,0) += delta(0,0);
      A(1,0) += delta(0,1);
      A(0,1) += delta(0,3);
      A(1,1) += delta(0,4);

      d(0,0) += delta(0,2);
      d(0,1) += delta(0,5);
 
      //Display(A);
      //Display(d);

    } // end iteration loop

    // converting back to image coordinate system
    if(params.origin_centroid == false)
      d = d + loc_centroid_MatFlt - (A*loc_centroid_MatFlt);

    if(oob_t == true)
    {
      result.status = _oob;
      result.A = A_zeros;
      result.d = d_zeros;
      result.residue = 0.0f;
      result_regions->push_back(result);
      oob_t = false;
    }
    else if(smalldet_t == true)
    {
      result.status = _small_det;
      result.A = A_zeros;
      result.d = d_zeros;
      result.residue = 0.0f;
      result_regions->push_back(result);
      smalldet_t = false;
    }
    else if(notfound_t == true)
    {
      result.status = _not_found;
      result.A = A_zeros;
      result.d = d_zeros;
      result.residue = 0.0f;
      result_regions->push_back(result);
      notfound_t = false;
    }      
    else if(tracked_t == true)
    {
      result.status = _tracked;
      result.A = A;
      result.d = d;
      result.residue = residue_per_pixel;
      result_regions->push_back(result);
      tracked_t = false;
    }
    else if(residue_per_pixel < params.residue_threshold) // residue checking
    {
      result.status = _max_iterations;
      result.A = A;
      result.d = d;
      result.residue = residue_per_pixel;
      result_regions->push_back(result);
    }
    else
    {
      result.status = _large_residue;
      result.A = A;
      result.d = d;
      result.residue = residue_per_pixel;
      result_regions->push_back(result);
    }

  } // end label loop

}

void LucasKanadeAffine(
  const ImgFloat& img1,
  const ImgFloat& img2,
  const ImgBinary& mask,
  const LucasKanadeAffineParameters& params,
  const MatFlt& A_init,
  const MatFlt& d_init,
  LucasKanadeAffineResult* result)
{
  ImgInt mask_int;
  Convert(mask,&mask_int);
  assert(params.skip_label_zero==true);

  std::vector<MatFlt> A_init_regions;
  std::vector<MatFlt> d_init_regions;
  A_init_regions.push_back(A_init);
  d_init_regions.push_back(d_init);

  std::vector<LucasKanadeAffineResult> result_regions;
  LucasKanadeAffineRegions(img1, img2, mask_int, params, A_init_regions, d_init_regions, &result_regions); 

  result->status = (result_regions[0]).status;
  result->A = (result_regions[0]).A;
  result->d = (result_regions[0]).d;
  result->centroid_x = (result_regions[0]).centroid_x;
  result->centroid_y = (result_regions[0]).centroid_y;
  result->residue = (result_regions[0]).residue;
}

void LucasKanadeAffine(
  const ImgFloat& img1,
  const ImgFloat& img2,
  const Rect& rect,
  const LucasKanadeAffineParameters& params,
  const MatFlt& A_init,
  const MatFlt& d_init,
  LucasKanadeAffineResult* result)
{
  ImgInt mask_int(img1.Width(), img1.Height());
  Set(&mask_int,0);
  ImgInt::RectIterator mask_true_p = mask_int.BeginRect(rect);
  while(!mask_true_p.AtEnd())
  {
    *mask_true_p = 1;
    mask_true_p++;
  }

  ImgBinary mask;
  Convert(mask_int,&mask);
  LucasKanadeAffine(img1, img2, mask, params, A_init, d_init, result);
}

};
