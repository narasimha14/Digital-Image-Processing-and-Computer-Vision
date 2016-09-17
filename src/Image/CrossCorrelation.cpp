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

/*
Update @Yi Zhou  yiz@clemson.edu
Spring 2007
*/

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$//

#include "Image.h"
#include "ImageOperations.h"
#include <afx.h>  // CString
#include "Utilities/PointSizeRect.h"
//#include "Utilities/Utilities.h"
#include <math.h>


namespace blepo {
  //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$//
  /*
  Return disparity map between left and right image which have been rectified
  */
  //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$//
  
  void StereoCrossCorr(const ImgInt& img_left, const ImgInt& img_right, const int& window_size, const int& max_disp, const CString& method, ImgGray* out)
  {
    //--------------------------------------------------------------------------//
    // CHECK VALIDITY OF INPUTS
    //--------------------------------------------------------------------------//
    assert(img_left.Width() == img_right.Width());
    assert(img_left.Height() == img_right.Height());
    assert((window_size % 2) != 0);
    assert(method == "basic" || method == "normalized");
    //--------------------------------------------------------------------------//
    // DECLARATION
    //--------------------------------------------------------------------------//
    int i, j, d, m, n;
    int image_width, image_height;
    int image_margin_min, image_margin_max_width, image_margin_max_height, image_margin_max_window;
    float ssd, ssd_min, energy;
    int difference;
    int disparity;
    int left_width, left_height, right_width;
    //--------------------------------------------------------------------------//
    // INITIALIZATION
    //--------------------------------------------------------------------------//
    image_width = img_left.Width();
    image_height = img_left.Height();
    image_margin_min = window_size / 2;
    image_margin_max_width = image_width - image_margin_min;
    image_margin_max_height = image_height - image_margin_min;
    image_margin_max_window = -image_margin_min + window_size;
    
    ssd = ssd_min = 0;
    disparity = 0;
    
    (*out).Reset(image_width, image_height);
    Set(out, 0);
    //--------------------------------------------------------------------------//
    // FUNCTION CORE
    //--------------------------------------------------------------------------//
    // Cross-correlation search
    for(i = image_margin_min; i < image_margin_max_width; i++)
      for(j = image_margin_min; j < image_margin_max_height; j++)
      {
        for(d = 0; d <= max_disp; d++)
        {
          if(i - image_margin_min - d >= 0)
          {
            ssd = 0;
            energy = 0;
            
            for(m = -image_margin_min; m < image_margin_max_window; m++)
            {
              left_width = i + m;
              right_width = left_width - d;
              for(n = -image_margin_min; n < image_margin_max_window; n++)
              {
                if(left_width - d >= 0 && left_width < image_width)
                {
                  left_height = j + n;
                  difference = img_left(left_width, left_height) - img_right(right_width, left_height);
                  ssd += difference * difference;
                  // Normalization
                  if(method == "normalized")
                    energy += img_right(right_width, left_height) * img_right(right_width, left_height) ;
                }
              }
            }
            
            if(method == "normalized")
              ssd /= sqrt(energy);
            
            if(d == 0 || ssd < ssd_min)
            {
              ssd_min = ssd;
              disparity = d;
            }	  
            // Output
            (*out)(i, j) = disparity;
          }
        }
      }
      //
      //--------------------------------------------------------------------------//
      // END
      //--------------------------------------------------------------------------//
  }
  
  
  void StereoCrossCorr(const ImgGray& img_left, const ImgGray& img_right, const int& window_size, const int& max_disp, const CString& method, ImgGray* out)
  {
    //--------------------------------------------------------------------------//
    // CHECK VALIDITY OF INPUTS
    //--------------------------------------------------------------------------//
    assert(img_left.Width() == img_right.Width());
    assert(img_left.Height() == img_right.Height());
    assert((window_size % 2) != 0);
    assert(method == "basic" || method == "normalized");
    //--------------------------------------------------------------------------//
    // DECLARATION
    //--------------------------------------------------------------------------//
    int i, j, d, m, n;
    int image_width, image_height;
    int image_margin_min, image_margin_max_width, image_margin_max_height, image_margin_max_window;
    float ssd, ssd_min, energy;
    int disparity;
    int difference;
    int left_width, left_height, right_width;
    //--------------------------------------------------------------------------//
    // INITIALIZATION
    //--------------------------------------------------------------------------//
    image_width = img_left.Width();
    image_height = img_left.Height();
    image_margin_min = window_size / 2;
    image_margin_max_width = image_width - image_margin_min;
    image_margin_max_height = image_height - image_margin_min;
    image_margin_max_window = -image_margin_min + window_size;
    
    ssd = ssd_min = 0;
    disparity = 0;
    
    (*out).Reset(image_width, image_height);
    Set(out, 0);
    //--------------------------------------------------------------------------//
    // FUNCTION CORE
    //--------------------------------------------------------------------------//
    // Cross-correlation search
    for(i = image_margin_min; i < image_margin_max_width; i++)
      for(j = image_margin_min; j < image_margin_max_height; j++)
      {
        for(d = 0; d <= max_disp; d++)
        {
          if(i - image_margin_min - d >= 0)
          {
            ssd = 0;
            energy = 0;
            
            for(m = -image_margin_min; m < image_margin_max_window; m++)
            {
              left_width = i + m;
              right_width = left_width - d;
              for(n = -image_margin_min; n < image_margin_max_window; n++)
              {
                if(left_width - d >= 0 && left_width < image_width)
                {
                  left_height = j + n;
                  difference = img_left(left_width, left_height) - img_right(right_width, left_height);
                  ssd += difference * difference;
                  // Normalization
                  if(method == "normalized")
                    energy += img_right(right_width, left_height) * img_right(right_width, left_height) ;
                }
              }
            }
            
            if(method == "normalized")
              ssd /= sqrt(energy);
            
            if(d == 0 || ssd < ssd_min)
            {
              ssd_min = ssd;
              disparity = d;
            }	  
            // Output
            (*out)(i, j) = disparity;
          }
        }
      }
      //--------------------------------------------------------------------------//
      // END
      //--------------------------------------------------------------------------//
  }
  
  void StereoCrossCorr(const ImgFloat& img_left, const ImgFloat& img_right, const int& window_size, const int& max_disp, const CString& method, ImgGray* out)
  {
    //--------------------------------------------------------------------------//
    // CHECK VALIDITY OF INPUTS
    //--------------------------------------------------------------------------//
    assert(img_left.Width() == img_right.Width());
    assert(img_left.Height() == img_right.Height());
    assert((window_size % 2) != 0);
    assert(method == "basic" || method == "normalized");
    //--------------------------------------------------------------------------//
    // DECLARATION
    //--------------------------------------------------------------------------//
    int i, j, d, m, n;
    int image_width, image_height;
    int image_margin_min, image_margin_max_width, image_margin_max_height, image_margin_max_window;
    float ssd, ssd_min, energy;
    float difference;
    int disparity;
    int left_width, left_height, right_width;
    //--------------------------------------------------------------------------//
    // INITIALIZATION
    //--------------------------------------------------------------------------//
    image_width = img_left.Width();
    image_height = img_left.Height();
    image_margin_min = window_size / 2;
    image_margin_max_width = image_width - image_margin_min;
    image_margin_max_height = image_height - image_margin_min;
    image_margin_max_window = -image_margin_min + window_size;
    
    ssd = ssd_min = 0;
    disparity = 0;
    
    (*out).Reset(image_width, image_height);
    Set(out, 0);
    //--------------------------------------------------------------------------//
    // FUNCTION CORE
    //--------------------------------------------------------------------------//
    // Cross-correlation search
    for(i = image_margin_min; i < image_margin_max_width; i++)
      for(j = image_margin_min; j < image_margin_max_height; j++)
      {
        for(d = 0; d <= max_disp; d++)
        {
          if(i - image_margin_min - d >= 0)
          {
            ssd = 0;
            energy = 0;
            
            for(m = -image_margin_min; m < image_margin_max_window; m++)
            {
              left_width = i + m;
              right_width = left_width - d;
              for(n = -image_margin_min; n < image_margin_max_window; n++)
              {
                if(left_width - d >= 0 && left_width < image_width)
                {
                  left_height = j + n;
                  difference = img_left(left_width, left_height) - img_right(right_width, left_height);
                  ssd += difference * difference;
                  // Normalization
                  if(method == "normalized")
                    energy += img_right(right_width, left_height) * img_right(right_width, left_height) ;
                }
              }
            }
            
            if(method == "normalized")
              ssd /= sqrt(energy);
            
            if(d == 0 || ssd < ssd_min)
            {
              ssd_min = ssd;
              disparity = d;
            }	  
            // Output
            (*out)(i, j) = disparity;
          }
        }
      }
      //
      //--------------------------------------------------------------------------//
      // END
      //--------------------------------------------------------------------------//
  }
  
  //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$//
  /*
  Return the left-top location of the image which matches best with the given template.
  "search_range defines" the space within which the search window moves.
  */
  //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$//
  
  void TemplateCrossCorr(const ImgInt& template_img, const ImgInt& img, const CRect& search_range, const CString& method, CPoint* out)
  {
    //--------------------------------------------------------------------------//
    // CHECK VALIDITY OF INPUTS
    //--------------------------------------------------------------------------//
    assert(search_range.left >= 0 && search_range.right < img.Width());
    assert(search_range.top >= 0 && search_range.bottom < img.Height());
    assert(search_range.right - search_range.left >= template_img.Width());
    assert(search_range.bottom - search_range.top >= template_img.Height());
    assert( method == "basic" || method == "normalized");
    //--------------------------------------------------------------------------//
    // DECLARATION
    //--------------------------------------------------------------------------//
    int i, j, m, n;
    int image_width, image_height, template_width, template_height;
    int match_width, match_height;
    int difference;
    float ssd, ssd_min, energy;
    //--------------------------------------------------------------------------//
    // INITIALIZATION
    //--------------------------------------------------------------------------//
    image_width = img.Width();
    image_height = img.Height();
    template_width = template_img.Width();
    template_height = template_img.Height();
    
    ssd = ssd_min = 0;
    //--------------------------------------------------------------------------//
    // FUNCTION CORE
    //--------------------------------------------------------------------------//
    // Template cross-correlation search
    for(i = search_range.left; i < search_range.right - template_width; i++)
      for(j = search_range.top; j < search_range.bottom - template_height; j++)
      {
        if(i >= 0 && i < image_width && j >= 0 && j < image_height)
        {
          ssd = 0;
          energy = 0;
          
          for(m = 0; m < template_width; m++)
            for(n = 0; n < template_height; n++)
            {
              if(i + m >= 0 && i + m < image_width && j + n >= 0 && j + n < image_height)
              {
                difference = template_img(m, n) - img(i + m, j + n);
                ssd += difference * difference;
                
                // Normalization
                if(method == "normalized")
                  energy += img(i + m, j + n) * img(i + m, j + n);
              }
            }
            
            // Normalization
            if(method == "normalized")
              ssd /= (float)sqrt(energy);
            
            if((i == search_range.left && j == search_range.top) || ssd < ssd_min)
            {
              ssd_min = ssd;
              match_width = i;
              match_height = j;
            }
        }
      }
      // Output
      (*out).x = match_width;
      (*out).y = match_height;
      //--------------------------------------------------------------------------//
      // END
      //--------------------------------------------------------------------------// 	
  }
  
  
  void TemplateCrossCorr(const ImgGray& template_img, const ImgGray& img, const CRect& search_range, const CString& method, CPoint* out)
  {
    //--------------------------------------------------------------------------//
    // CHECK VALIDITY OF INPUTS
    //--------------------------------------------------------------------------//
    assert(search_range.left >= 0 && search_range.right < img.Width());
    assert(search_range.top >= 0 && search_range.bottom < img.Height());
    assert(search_range.right - search_range.left >= template_img.Width());
    assert(search_range.bottom - search_range.top >= template_img.Height());
    assert( method == "basic" || method == "normalized");
    //--------------------------------------------------------------------------//
    // DECLARATION
    //--------------------------------------------------------------------------//
    int i, j, m, n;
    int image_width, image_height, template_width, template_height;
    int match_width, match_height;
    int difference;
    float ssd, ssd_min, energy;
    //--------------------------------------------------------------------------//
    // INITIALIZATION
    //--------------------------------------------------------------------------//
    image_width = img.Width();
    image_height = img.Height();
    template_width = template_img.Width();
    template_height = template_img.Height();
    
    ssd = ssd_min = 0;
    //--------------------------------------------------------------------------//
    // FUNCTION CORE
    //--------------------------------------------------------------------------//
    // Template cross-correlation search
    for(i = search_range.left; i < search_range.right - template_width; i++)
      for(j = search_range.top; j < search_range.bottom - template_height; j++)
      {
        if(i >= 0 && i < image_width && j >= 0 && j < image_height)
        {
          ssd = 0;
          energy = 0;
          
          for(m = 0; m < template_width; m++)
            for(n = 0; n < template_height; n++)
            {
              if(i + m >= 0 && i + m < image_width && j + n >= 0 && j + n < image_height)
              {
                difference = template_img(m, n) - img(i + m, j + n);
                ssd += difference * difference;
                
                // Normalization
                if(method == "normalized")
                  energy += img(i + m, j + n) * img(i + m, j + n);
              }
            }
            
            // Normalization
            if(method == "normalized")
              ssd /= (float)sqrt(energy);
            
            if((i == search_range.left && j == search_range.top) || ssd < ssd_min)
            {
              ssd_min = ssd;
              match_width = i;
              match_height = j;
            }
        }
      }
      // Output
      (*out).x = match_width;
      (*out).y = match_height;
      //--------------------------------------------------------------------------//
      // END
      //--------------------------------------------------------------------------//  
  }
  
  
  void TemplateCrossCorr(const ImgFloat& template_img, const ImgFloat& img, const CRect& search_range, const CString& method, CPoint* out)
  {
    //--------------------------------------------------------------------------//
    // CHECK VALIDITY OF INPUTS
    //--------------------------------------------------------------------------//
    assert(search_range.left >= 0 && search_range.right < img.Width());
    assert(search_range.top >= 0 && search_range.bottom < img.Height());
    assert(search_range.right - search_range.left >= template_img.Width());
    assert(search_range.bottom - search_range.top >= template_img.Height());
    assert( method == "basic" || method == "normalized");
    //--------------------------------------------------------------------------//
    // DECLARATION
    //--------------------------------------------------------------------------//
    int i, j, m, n;
    int image_width, image_height, template_width, template_height;
    int match_width, match_height;
    float difference;
    float ssd, ssd_min, energy;
    //--------------------------------------------------------------------------//
    // INITIALIZATION
    //--------------------------------------------------------------------------//
    image_width = img.Width();
    image_height = img.Height();
    template_width = template_img.Width();
    template_height = template_img.Height();
    
    ssd = ssd_min = 0;
    //--------------------------------------------------------------------------//
    // FUNCTION CORE
    //--------------------------------------------------------------------------//
    // Template cross-correlation search
    for(i = search_range.left; i < search_range.right - template_width; i++)
      for(j = search_range.top; j < search_range.bottom - template_height; j++)
      {
        if(i >= 0 && i < image_width && j >= 0 && j < image_height)
        {
          ssd = 0;
          energy = 0;
          
          for(m = 0; m < template_width; m++)
            for(n = 0; n < template_height; n++)
            {
              if(i + m >= 0 && i + m < image_width && j + n >= 0 && j + n < image_height)
              {
                difference = template_img(m, n) - img(i + m, j + n);
                ssd += difference * difference;
                
                // Normalization
                if(method == "normalized")
                  energy += img(i + m, j + n) * img(i + m, j + n);
              }
            }
            
            // Normalization
            if(method == "normalized")
              ssd /= (float)sqrt(energy);
            
            if((i == search_range.left && j == search_range.top) || ssd < ssd_min)
            {
              ssd_min = ssd;
              match_width = i;
              match_height = j;
            }
        }
      }
      // Output
      (*out).x = match_width;
      (*out).y = match_height;
      //--------------------------------------------------------------------------//
      // END
      //--------------------------------------------------------------------------//   
  }
  
};  // end namespace blepo
