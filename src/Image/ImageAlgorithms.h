/* 
 * Copyright (c) 2004,2005,2006 Clemson University.
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

#ifndef __BLEPO_IMAGEALGORITHMS_H__
#define __BLEPO_IMAGEALGORITHMS_H__

#pragma warning(disable: 4786)  // identifier was truncated to '255' characters
#include "ImageOperations.h"
//#include "EllipticalHeadTracker.h"
#include "klt/klt.h"
#include "Utilities/Array.h"
#include "Utilities/Math.h"
#include <vector>
#include "blepo_opencv.h"  // OpenCV headers

/**
  This file contains higher-level algorithms for images.
*/

namespace blepo
{

/**
  Connected components of an image.
*/  
template <typename T>
struct ConnectedComponentProperties
{
  ConnectedComponentProperties(const Rect& r, const Point& p, int n, T v) 
    : bounding_rect(r), pixel(p), npixels(n), value(v) {}
  Rect bounding_rect;   // bounding rectangle of component
  int npixels;       // number of pixels in the component
  T value;  // value in the original image
  Point pixel;  // a pixel in the region (could be anywhere in the region)
  Array<int> adjacent_regions;  // labels of regions that are adjacent to this one
};
// Compute connected components, using 4-neighbor or 8-neighbor connectedness.
// Labels will be consecutive non-negative integers.
// Returns the number of labels (i.e., the maximum label number + 1)
// 'inplace' NOT okay
int ConnectedComponents4(const ImgBgr   & img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgBgr   ::Pixel> >* props = NULL);
int ConnectedComponents4(const ImgBinary& img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgBinary::Pixel> >* props = NULL);
int ConnectedComponents4(const ImgGray  & img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgGray  ::Pixel> >* props = NULL);
int ConnectedComponents4(const ImgInt   & img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgInt   ::Pixel> >* props = NULL);
int ConnectedComponents8(const ImgBgr   & img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgBgr   ::Pixel> >* props = NULL);
int ConnectedComponents8(const ImgBinary& img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgBinary::Pixel> >* props = NULL);
int ConnectedComponents8(const ImgGray  & img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgGray  ::Pixel> >* props = NULL);
int ConnectedComponents8(const ImgInt   & img, ImgInt* labels, std::vector<ConnectedComponentProperties<ImgInt   ::Pixel> >* props = NULL);

/**
  Floodfill (4- and 8-connectedness)
 'inplace' okay
*/
void FloodFill4(const ImgBgr   & img, int x, int y, ImgBgr   ::Pixel new_color, ImgBgr   * out);
void FloodFill4(const ImgBinary& img, int x, int y, ImgBinary::Pixel new_color, ImgBinary* out);
void FloodFill4(const ImgFloat& img,  int x, int y, ImgFloat::Pixel  new_color, ImgFloat* out);
void FloodFill4(const ImgGray  & img, int x, int y, ImgGray  ::Pixel new_color, ImgGray  * out);
void FloodFill4(const ImgInt   & img, int x, int y, ImgInt   ::Pixel new_color, ImgInt   * out);
void FloodFill8(const ImgBgr   & img, int x, int y, ImgBgr   ::Pixel new_color, ImgBgr   * out);
void FloodFill8(const ImgBinary& img, int x, int y, ImgBinary::Pixel new_color, ImgBinary* out);
void FloodFill8(const ImgFloat& img,  int x, int y, ImgFloat::Pixel  new_color, ImgFloat* out);
void FloodFill8(const ImgGray  & img, int x, int y, ImgGray  ::Pixel new_color, ImgGray  * out);
void FloodFill8(const ImgInt   & img, int x, int y, ImgInt   ::Pixel new_color, ImgInt   * out);

/*
 Fill holes in a binary image, i.e. converts all 0-regions into 1-regions except the dominant 0-region)
 When TRUE, the function assumes that the 0 corresponding to the dominant 0-region appers within top 5 rows 
 When FALSE, the function assumes that the largest connected component in the image having value of 0 is the background
 -nkanher
*/
void FillHoles(ImgBinary* BinIn, bool use_speedup_hack = true);


// Compute Chamfer distance
void Chamfer(const ImgGray& img, ImgInt* chamfer_dist);

/**
  Properties of a binary region of pixels
*/
struct RegionProperties
{
  double m00;              // zeroth-order moment
  double m10, m01, m11;    // first-order moments in x and y
  double m20, m02;         // second-order moments in x and y
  double mu00;             // central zeroth-order moment (always equal to m00) 
  double mu10, mu01, mu11; // central first-order moments in x and y (mu10 and mu01 are always zero)
  double mu20, mu02;       // central second-order moments in x and y
  double area;             // same as m00
  Rect bounding_rect;
  double xc, yc;           // centroid
  double eccentricity;     // ratio of major axis length to minor axis length
  double direction;        // angle of major axis with respect to positive x-axis
  double major_axis_x, major_axis_y;  // major and minor axes scaled by the standard deviation = sqrt(eigenvalue)
  double minor_axis_x, minor_axis_y;
};
void RegionProps(const ImgBinary& img, RegionProperties* props);

/**
  Canny edge detection
  (convolution with derivative of Gaussian, non-maximum suppression, hysteresis thresholding)
    'sigma':  if sigma == -1, then uses fast Sobel implementation (3x3 kernel, sigma^2 = 0.5)
              if sigma == -2, then uses fast anisotropic implementation (sigma^2 = 1.0 for smoothing, sigma^2 = 2.0 for differentiation)
    'perc':  percentage of non-zero gradient magnitude pixels (after non-maximum suppression)
             that are below the high threshold
    'fact':  ratio of high to low thresholds
*/
void Canny(const ImgGray& img, ImgBinary* out, float sigma, float perc = 0.9f, float ratio = 5.0f);

/**
  Horowitz-Pavlidis split-and-merge segmentation.  The standard deviation of
  gray levels is used as the homogeneity criterion.
*/
void SplitAndMergeSegmentation(const ImgGray& img, ImgInt* labels, float th_std = 20.0f);

/* 
  Vincent-Soilles watershed segmentation.
  You should pass in a gradient magnitude image to this function.
  'marker_based':  If true, then new catchment basins will only be declared in regions
                   where 'img' is 0.  If false, then new catchment basins will be declared
                   in all local minima (likely leading to oversegmentation).
                   To use marker-based, simply set 'img' to zero wherever you would like to set a marker.
*/
void WatershedSegmentation(const ImgGray& img, ImgInt* labels, bool marker_based);

/*
  Comaniciu et al. Mean shift segmentation.  'inplace' is okay
  Returns the number of components found.
*/
struct MeanShiftSegmentationParams
{
  MeanShiftSegmentationParams() : sigma_spatial(7), sigma_color(6.5f), minregion(20), speedup(2) {}
  int sigma_spatial;  ///< spatial radius of the mean shift window
  float sigma_color;  ///< range radius of the mean shift window
  int minregion;      ///< minimum density of a region; regions smaller than this will be pruned
  int speedup;        ///< speedup level (0: slowest, 1: medium, 2: fastest)
};
int MeanShiftSegmentation(const ImgBgr& img, ImgInt* out_labels, ImgBgr* out_meancolors, const MeanShiftSegmentationParams& params = MeanShiftSegmentationParams());

// Implements Felzenszwalb-Huttenlocher segmentation algorithm described in:
//   Efficient Graph-Based Image Segmentation
//   Pedro F. Felzenszwalb and Daniel P. Huttenlocher
//   International Journal of Computer Vision, 59(2), September 2004.
//
//  The parameters are: (see the paper for details)
//
//  sigma: Used to smooth the input image before segmenting it.
//  k: Value for the threshold function.
//  min: Minimum component size enforced by post-processing.
//  input: Input image.
//  output: Output image.
//
//  Typical parameters are sigma = 0.5, k = 500, min = 20.
//  Larger values for k result in larger components in the result.
//
//  Returns the number of components found.
//
int FHGraphSegmentation(const ImgBgr& img, float sigma, float k, int min_size, ImgInt *out_labels, ImgBgr *out_pseudocolors);

int FHGraphSegmentDepth(const ImgBgr& img, const ImgGray& depth, float sigma, float k, int min_size, ImgInt *out_labels, ImgBgr *out_pseudocolors);

void RemoveGapsFromLabelImage(const ImgInt& labels, ImgInt* out);

/**
  Kass et al. Snakes (Active contours)
  Perform one iteration of the dynamic programming algorithm to update the location of the
  points on the snake.  Searches +/- 1 pixel in both x and y, and uses the magnitude of the
  gradient of the image for the external energy.
  returns true if the snake moved, false if the snake is already at a local minimum
*/
typedef std::vector<Point> Snake;
bool SnakeIteration(const ImgGray& img, float alpha, Snake* points);
bool SnakeIteration(const ImgGray& img, float alpha, float beta, Snake* points);

/**
  Face detector, using OpenCV's adaptation of the Viola-Jones algorithm (CVPR 2001)
*/
class PatternDetector
{
public:
  /// Must call Init() after calling default constructor, before using the detector
  PatternDetector();

  PatternDetector(const char* xml_filename);
  virtual ~PatternDetector();

  /// initialize (if default constructor called)
  void Init(const char* xml_filename);

  /// Run the detector on an image
  void DetectPatterns(const ImgBgr& img, std::vector<Rect>* rectlist);
  void DetectPatterns(const ImgBgr& img, std::vector<Rect>* rectlist, float scale_incr_ratio, int grouping_param, bool do_canny_prunning, int min_size);
  void DetectPatterns(const ImgBgr& img, std::vector<Rect>* rectlist, float scale_incr_ratio, int grouping_param, bool do_canny_prunning, Point min_size);

private:
	struct CvHaarClassifierCascade* m_cascade;
	struct CvMemStorage* m_storage;
};

class FaceDetector
{
public:
  FaceDetector();
  virtual ~FaceDetector();

  /// Detect frontal, left-profile, and right-profile faces
  void DetectAllFaces(const ImgBgr& img, 
                      std::vector<Rect>* rectlist_frontal,
                      std::vector<Rect>* rectlist_left,
                      std::vector<Rect>* rectlist_right
                      );

  // Detect only frontal faces
  void DetectFrontalFaces(const ImgBgr& img, std::vector<Rect>* rectlist);

  // Detect only left-profile faces
  // (flips the image horizontally, then detects right-profile faces)
  void DetectLeftProfileFaces(const ImgBgr& img, std::vector<Rect>* rectlist);

  // Detect only right-profile faces
  void DetectRightProfileFaces(const ImgBgr& img, std::vector<Rect>* rectlist);

private:
  PatternDetector m_frontal_detector;
  PatternDetector m_profile_detector;
  ImgBgr m_img_tmp;
};

// The CamShift color tracking algorithm, implemented in OpenCV
class CamShift
{
public:
  struct Box
  {
    CPoint center;
    CSize  size;
    double angle;
  };

  CamShift(const ImgBgr& img);
  virtual ~CamShift();

  /// Run the detector on an image
  Box Track(const ImgBgr& img);

  // Initialize
  void CalcHistogram(const ImgBgr& img, const CRect& sel);

private:
  IplImage *image , *hsv, *hue , *mask, *backproject, *histimg ;
  CvHistogram *hist;
  
  int backproject_mode ;
//  int select_object;
//  int track_object ;
//  int show_hist = 1;
  CvPoint origin;
  CvRect selection;
  
  CvRect track_window;
  CvBox2D track_box;
  CvConnectedComp track_comp;
  int hdims;

  int vmin , vmax , smin;
  CvScalar hsv2rgb( float hue )
  {
    int rgb[3], p, sector;
    static const int sector_data[][3]=
    {{0,2,1}, {1,2,0}, {1,0,2}, {2,0,1}, {2,1,0}, {0,1,2}};
    hue *= 0.033333333333333333333333333333333f;
    sector = cvFloor(hue);
    p = cvRound(255*(hue - sector));
    p ^= sector & 1 ? 255 : 0;
    
    rgb[sector_data[sector][0]] = 255;
    rgb[sector_data[sector][1]] = 0;
    rgb[sector_data[sector][2]] = p;
    
    return cvScalar(rgb[2], rgb[1], rgb[0],0);
  }
};

void RealTimeStereo(const ImgGray& img_left, const ImgGray& img_right, ImgGray* disparity_map, int max_disp = 14, int winsize = 5);

void HistogramGray(const ImgGray& img,const int bin, std::vector<int>* out);
void HistogramBinary(const ImgGray& img,int* white, int* black);
void ConservativeSmoothing(const ImgGray& img,  const int win_width,const int win_height, ImgGray* out);
void ConservativeSmoothing(const ImgFloat& img,  const int win_width,const int win_height, ImgFloat* out);
void ConservativeSmoothing(const ImgInt& img,  const int win_width,const int win_height, ImgInt* out);

void StereoCrossCorr(const ImgInt& img_left, const ImgInt& img_right, const int& window_size, const int& max_disp, const CString& method, ImgGray* out);
void StereoCrossCorr(const ImgGray& img_left, const ImgGray& img_right, const int& window_size, const int& max_disp, const CString& method, ImgGray* out);
void StereoCrossCorr(const ImgFloat& img_left, const ImgFloat& img_right, const int& window_size, const int& max_disp, const CString& method, ImgGray* out);
void TemplateCrossCorr(const ImgInt& template_img, const ImgInt& img, const CRect& search_range, const CString& method, CPoint* out);
void TemplateCrossCorr(const ImgGray& template_img, const ImgGray& img, const CRect& search_range, const CString& method, CPoint* out);
void TemplateCrossCorr(const ImgFloat& template_img, const ImgFloat& img, const CRect& search_range, const CString& method, CPoint* out);

// Calculates optical flow for two images by block matching method.
// Uses OpenCV implementation.
void OpticalFlowBlockMatchOpencv(const ImgGray& img1, const ImgGray& img2,
                                 const CSize& block_size,
                                 const CSize& shift_size,
                                 const CSize& max_range,
                                 bool use_previous,
                                 ImgFloat* velx,
                                 ImgFloat* vely);

void SynthesizeTextureEfrosLeung(const ImgGray& texture, const ImgBinary &mask, int window_width, int window_height, int out_width, int out_height, ImgGray* out);

////// calibration

typedef Array<CvPoint2D32f> Cvptarr;

struct CalibrationPoint
{
  double x, y, z;  // world coordinates
  double u, v;  // image coordinates
};

typedef Array<CalibrationPoint> CalibrationPointArr;

struct CalibrationParams
{
  MatDbl intrinsic_matrix;
  Array<double> distortion_coeffs;
};

bool FindChessboardCorners(const ImgGray& img, const Size& grid_dims, Cvptarr* pts);
void DrawChessboardCorners(ImgBgr* img, const Size& grid_dims, bool all_found, const Cvptarr& pts);
void TransformChessboardPoints(const Cvptarr& pts, const Size& grid_dims, CalibrationPointArr* cpts);
void CalibrateCamera(const std::vector<CalibrationPointArr>& pts, const Size& img_size, CalibrationParams* out);


// Parameters for LucasKanadeAffine() and 
// LucasKanadeAffineRegions() functions
struct LucasKanadeAffineParameters
{
  int min_num_pixels; // minimum number of pixels in region for tracking
  int num_iterations; // maximum number of iterations 

  float oob_threshold; // ratio of warped area going out of the image
  float max_disp_threshold; // maximum displacement allowed
  float min_disp_threshold; // minimum displacement for stopping iterations
  float max_aff_threshold; // maximum affine warp allowed
  float min_aff_threshold; // minimum affine warp for stopping iterations

  float det_threshold; // minimum determinant of T matrix of Tz = d
  float residue_threshold; // maximum residue allowed

  float shrink_width_ratio; // horizontal border width (ratio) to be removed
  float shrink_height_ratio; // vertical border width (ratio) to be removed
  float smooth_sigma; // gaussian sigma for smoothing
  float gradient_sigma; // gaussian sigma for gradient computation
  bool skip_label_zero; // set to true for skipping label zero in mask image
                        // must be true for LucasKanadeAffine() functions
                        // can be true or false for LucasKanadeAffineRegions() function
  bool origin_centroid; // set to true if parameters for each region are to be computed
                        // with origin at that region's centroid
                        // set to false for common origin at image's (0,0)

  // defalut values
  LucasKanadeAffineParameters()
  {
    min_num_pixels = 25;
    num_iterations = 20;

    oob_threshold = 0.0f; 
    max_disp_threshold = 5.0f;
    min_disp_threshold = 0.05f;
    max_aff_threshold = 0.5f;
    min_aff_threshold = 0.005f;

    det_threshold = 1000.0f;
    residue_threshold = 5.0f;

    shrink_width_ratio = 0.1f;
    shrink_height_ratio = 0.1f;
    smooth_sigma = 0.8f;
    gradient_sigma = 0.8f;
    skip_label_zero = true;
    origin_centroid = true; 
  }
};

// Result structure for LucasKanadeAffine() and 
// LucasKanadeAffineRegions() functions
struct LucasKanadeAffineResult
{
  
  int status;

  // status 0 mean region was tracked
  // status 1 means region was not found
  // status 2 means region determinant less than threshold
  // status 3 means region calculations reached max iterations
  // status 4 means region out of bounds
  // status 5 means region residue more than threshold
  // status 6 means region area is smaller than threshold 
  // status 7 means region is either near border or nonexistent

  MatFlt A;
  MatFlt d;
  float residue;
  float centroid_x;
  float centroid_y;  

  // A,d and residue will have non zero values only for status 0, 3 and 5
};


// Function to compute affine tracking of a rectangular patch 
// from img1 to img2 using the Lucas Kanade method. The rectangular 
// region patch, in img1, to be tracked is defined by the rect argument. 
// Forwards Additive implementation of the algorithm is used.
// z has all the 6 calculated affine paramaters and the 
// validity of tracking is returned in status. By default, affine 
// parameters are with respect to the centroid of the rectangular 
// region. Coordinates of the centroid are also returned.
// @author Braga Natarajan.

void LucasKanadeAffine(
  const ImgFloat& img1,
  const ImgFloat& img2,
  const Rect& rect,
  const LucasKanadeAffineParameters& params,
  const MatFlt& A_init,
  const MatFlt& d_init,
  LucasKanadeAffineResult* result);


// Function to compute affine tracking of an arbitrarily shaped patch 
// from img1 to img2 using the Lucas Kanade method. The region patch,
// in img1, to be tracked is defined by the binary image mask. 
// Forwards Additive implementation of the algorithm is used.
// z has all the 6 calculated affine paramaters and the 
// validity of tracking is returned in status. By default, affine 
// parameters are with respect to the centroid of the arbitrarily 
// shaped region. Coordinates of the centroid are also returned.
// @author Braga Natarajan.

void LucasKanadeAffine(
  const ImgFloat& img1,
  const ImgFloat& img2,
  const ImgBinary& mask,
  const LucasKanadeAffineParameters& params,
  const MatFlt& A_init,
  const MatFlt& d_init,
  LucasKanadeAffineResult* result);


// Function to compute affine tracking of multiple regions from img1
// to img2 using the Lucas Kanade method. The different regions, in
// img1, to be tracked are defined by the integer label image img_label. 
// Forwards Additive implementation of the algorithm is used.
// z_regions has the 6 calculated affine paramaters for all the
// regions indexed by label number and validity of tracking is returned 
// in status_regions. By default, the affine parameters for each region 
// are with respect to the centroid of that particular region. The 
// coordinates of the centroids for all regions are also returned.
// @author Braga Natarajan.

void LucasKanadeAffineRegions(
  const ImgFloat& img1,
  const ImgFloat& img2,
  const ImgInt& img_label,
  const LucasKanadeAffineParameters& params,
  const std::vector<MatFlt>& A_init_regions,
  const std::vector<MatFlt>& d_init_regions,
  std::vector<LucasKanadeAffineResult>* result_regions);


/** 
  Kanade-Lucas-Tomasi (KLT) classes and functions.
  All the classes have automatic conversions to the underlying
  C structs, so you can modify them if you want.  For your convenience,
  however, the classes handle memory management
  automatically for you.
*/

inline void KltCopyFeatureXYVal(const KLT_Feature f1, KLT_Feature f2)
{
  f2->x = f1->x;
  f2->y = f1->y;
  f2->val = f1->val;
}

class KltFeatureList
{
public:
  KltFeatureList(int nfeatures = 0)           : m_fl(NULL)  { Reset( nfeatures ); }
  KltFeatureList(const KltFeatureList& other) : m_fl(NULL)  { *this = other; }
  ~KltFeatureList()                                         { Reset( 0 ); }
  KltFeatureList& operator=(const KltFeatureList& other)
  {
    Reset( other.GetNFeatures() );
    for (int i = 0 ; i < other.GetNFeatures() ; i++)  { *(*this)[i] = *other[i]; }
	return *this;
  }
  operator const    KLT_FeatureList() const { return m_fl; }
  operator          KLT_FeatureList()       { return m_fl; }
  int               GetNFeatures() const    { return m_fl ? m_fl->nFeatures : 0; }
  const KLT_Feature operator[](int i) const { return m_fl->feature[i]; }
  KLT_Feature       operator[](int i)       { return m_fl->feature[i]; }
  void Reset(int nfeatures) 
  {
    if ((m_fl == NULL && nfeatures == 0)
     || (m_fl && nfeatures == m_fl->nFeatures))  return;  // already the correct size
    if (m_fl)  { KLTFreeFeatureList( m_fl );  m_fl = NULL; }
    if (nfeatures > 0)  m_fl = KLTCreateFeatureList( nfeatures );
  }
  void WriteToPpm(const ImgGray& img, const char* filename)
  {
    KLTWriteFeatureListToPPM(*this, 
                             const_cast<unsigned char*>(img.Begin()), 
                             img.Width(), img.Height(), const_cast<char*>(filename));  
  }
  int CountValidFeatures() const
  {
    int count = 0;
    for (int i=0 ; i<GetNFeatures() ; i++)
    {
      const KLT_Feature& f = (*this)[i];
      if (f->val >= 0)  count++;
    }
    return count;
  }
  bool WriteValidFeaturesAscii(const char* fname) const
  {
    FILE* fp = fopen(fname, "wt");
    if (fp)
    {
      fprintf(fp, "%d\n", CountValidFeatures());
      for (int i=0 ; i<GetNFeatures() ; i++)
      {
        const KLT_Feature& f = (*this)[i];
        if (f->val >= 0)
        {
          fprintf(fp, "%f %f\n", f->x, f->y);
        }
      }
      fclose(fp);
      return true;
    }
    else
    {
      return false;
    }
  }
private:
  KLT_FeatureList m_fl;
};

class KltFeatureHistory
{
public:
  KltFeatureHistory(int nframes = 0)                : m_fh(NULL)  { Reset( nframes ); } 
  KltFeatureHistory(const KltFeatureHistory& other) : m_fh(NULL)  { *this = other; }
  ~KltFeatureHistory()                                            { Reset( 0 ); }
  KltFeatureHistory& operator=(const KltFeatureHistory& other)
  {
    Reset( other.GetNFrames() );
    for (int i = 0 ; i < other.GetNFrames() ; i++)  { KltCopyFeatureXYVal(other[i], (*this)[i]); }
	return *this;
  }
  operator const    KLT_FeatureHistory() const { return m_fh; }
  operator          KLT_FeatureHistory()       { return m_fh; }
  int               GetNFrames() const         { return m_fh ? m_fh->nFrames : 0; }
  const KLT_Feature operator[](int i) const    { return m_fh->feature[i]; }
  KLT_Feature       operator[](int i)          { return m_fh->feature[i]; }
  void Reset(int nframes) 
  {
    if ((m_fh == NULL && nframes == 0)
     || (m_fh && nframes == m_fh->nFrames))  return;  // already the correct size
    if (m_fh)  { KLTFreeFeatureHistory( m_fh );  m_fh = NULL; }
    if (nframes > 0)  m_fh = KLTCreateFeatureHistory( nframes );
  }
private:
  KLT_FeatureHistory m_fh;
};

class KltFeatureTable
{
public:
  KltFeatureTable()                             : m_ft(NULL)  { Reset( 0, 0 ); }
  KltFeatureTable(int nframes, int nfeatures)   : m_ft(NULL)  { Reset( nframes, nfeatures ); }
  KltFeatureTable(const KltFeatureTable& other) : m_ft(NULL)  { *this = other; }
  ~KltFeatureTable()                                          { Reset( 0, 0 ); }
  KltFeatureTable& operator=(const KltFeatureTable& other)
  {
    Reset( other.GetNFrames(), other.GetNFeatures() );
    for (int i = 0 ; i < other.GetNFrames()   ; i++)  
    for (int j = 0 ; j < other.GetNFeatures() ; j++)
      { *(*this)(i, j) = *other(i, j); }
	return *this;
  }
  operator const KLT_FeatureTable() const { return m_ft; }
  operator       KLT_FeatureTable()       { return m_ft; }
  int            GetNFeatures()     const { return m_ft->nFeatures; }
  int            GetNFrames()       const { return m_ft->nFrames; }
  void Reset(int nframes, int nfeatures) 
  {
    if ((m_ft == NULL && nframes == 0 && nfeatures == 0)
     || (m_ft && nframes == m_ft->nFrames && nfeatures == m_ft->nFeatures))  return;  // already the correct size
    if (m_ft)  { KLTFreeFeatureTable( m_ft );  m_ft = NULL; }
    if (nframes > 0 && nfeatures > 0)  m_ft = KLTCreateFeatureTable( nframes, nfeatures );
  }
  const KLT_Feature operator()(int frame, int feature) const { return m_ft->feature[feature][frame]; }
  KLT_Feature       operator()(int frame, int feature)       { return m_ft->feature[feature][frame]; }
  void SetFeatureList(const KltFeatureList& fl, int frame_index)
  {
    assert(frame_index >= 0 && frame_index < GetNFrames());
    assert(fl.GetNFeatures() == GetNFeatures());
    for (int j = 0 ; j < fl.GetNFeatures() ; j++)
    { KltCopyFeatureXYVal(fl[j], (*this)(frame_index, j)); }
  }
  void GetFeatureList(int frame_index, KltFeatureList* fl) const
  {
    assert(frame_index >= 0 && frame_index < GetNFrames());
    fl->Reset( GetNFeatures() );
    for (int j = 0 ; j < fl->GetNFeatures() ; j++)
    { KltCopyFeatureXYVal((*this)(frame_index, j), (*fl)[j]); }
  }
  void SetFeatureHistory(const KltFeatureHistory& fh, int feature_index)
  {
    assert(feature_index >= 0 && feature_index < GetNFeatures());
    assert(fh.GetNFrames() == GetNFrames());
    for (int i = 0 ; i < fh.GetNFrames() ; i++)
    { KltCopyFeatureXYVal(fh[i], (*this)(i, feature_index)); }
  }
  void GetFeatureHistory(int feature_index, KltFeatureHistory* fh) const
  {
    assert(feature_index >= 0 && feature_index < GetNFeatures());
    fh->Reset( GetNFrames() );
    for (int i = 0 ; i < fh->GetNFrames() ; i++)
    { KltCopyFeatureXYVal((*this)(i, feature_index), (*fh)[i]); }
  }
  void Write(const char* fname, const char* fmt = NULL)
  {
    KLTWriteFeatureTable(m_ft, const_cast<char*>(fname), const_cast<char*>(fmt));
  }
  void Read(const char* fname)
  {
    m_ft = KLTReadFeatureTable(m_ft, const_cast<char*>(fname));
  }

private:
  KLT_FeatureTable m_ft;
};

class KltTrackingContext
{
public:
  KltTrackingContext() { m_tc = KLTCreateTrackingContext(); }
  ~KltTrackingContext() { KLTFreeTrackingContext(m_tc); }
  operator const KLT_TrackingContext() const { return m_tc; }
  operator KLT_TrackingContext() { return m_tc; }

  void SelectGoodFeatures(KltFeatureList* fl,
                          const ImgGray& img)
  {
    KLTSelectGoodFeatures(*this, 
              const_cast<unsigned char*>(img.Begin()), 
              img.Width(), img.Height(), *fl);
  }
  void TrackFeatures(KltFeatureList* fl,
                      const ImgGray& img1, 
                      const ImgGray& img2)
  {
    assert(IsSameSize(img1, img2));
    KLTTrackFeatures(*this, 
              const_cast<unsigned char*>(img1.Begin()), 
              const_cast<unsigned char*>(img2.Begin()), 
              img1.Width(), img1.Height(), *fl);
  }
  void ReplaceLostFeatures(KltFeatureList* fl,
                           const ImgGray& img)
  {
    KLTReplaceLostFeatures(*this, 
              const_cast<unsigned char*>(img.Begin()), 
              img.Width(), img.Height(), *fl);
  }
  // Draw features as dots on image
  void OverlayFeatures(ImgBgr* img, const KltFeatureList& fl, const Bgr& color = Bgr(0, 0, 255), int size = 3)
  {
    for (int i=0 ; i<fl.GetNFeatures() ; i++)
    {
      Point pt( blepo_ex::Round( fl[i]->x ), blepo_ex::Round( fl[i]->y ) );
      if (fl[i]->val >=0)  DrawDot( pt, img, color, size);
    }
  }
  // Draw lines between old and new features as lines on image
  void OverlayFeatureLines(ImgBgr* img, const KltFeatureList& fl1, const KltFeatureList& fl2, const Bgr& color = Bgr(0, 0, 255), int size = 1)
  {
    assert(fl1.GetNFeatures() == fl2.GetNFeatures());
    for (int i=0 ; i<fl1.GetNFeatures() ; i++)
    {
      if (fl1[i]->val >= 0 && fl2[i]->val == 0)  
      {
        Point pt1( blepo_ex::Round( fl1[i]->x ), blepo_ex::Round( fl1[i]->y ) );
        Point pt2( blepo_ex::Round( fl2[i]->x ), blepo_ex::Round( fl2[i]->y ) );
        DrawLine( pt1, pt2, img, color, size);
      }
    }
  }

  // @param b  Whether to track in sequential mode
  void SetSequentialMode(bool b) { m_tc->sequentialMode = b; }

  // @param b  True means run affine consistency check, false means run no check
  void SetAffineConsistencyCheck(bool b) { m_tc->affineConsistencyCheck = b ? 2 : -1; }
private:
  KLT_TrackingContext m_tc;
};

//struct Point2f { float x, y; };
struct FeaturePoint 
{ 
  FeaturePoint(float xx, float yy, float vval) : x(xx), y(yy), val(vval) {}
  float x, y, val; 
};



// teaching-quality (not research-quality) feature detection
//void DetectFeatures(const ImgGray& img, std::vector<FeaturePoint>* features);

/**
Lucas-Kanade-Tomasi feature detection and tracking.
Built upon OpenCV's pyramidal implementation.
*/

class FastFeatureTracker
{
public:
  struct Params
  {
    Params();
    double quality, min_distance, k;
    int block_size;
    bool use_harris;
    bool refine_corners;
    int window_hw, window_hh;  // half-width and half-height of window
    int flags;
    int npyramid_levels;  // number of pyramid levels (>= 1)
  };

  enum FeatureStatus { FEAT_TRACKED = 0, FEAT_NEW = 1, FEAT_LOST = -1 };
  struct Feature
  {
    Feature() {}
    Feature(float xx, float yy, FeatureStatus ss) : x(xx), y(yy), status(ss) {}
    float x, y;
    FeatureStatus status;
  };
  typedef Array<Feature> FeatureArr;

public:
  FastFeatureTracker();
  ~FastFeatureTracker();
  void SelectFeatures(const ImgGray& img, int max_npoints, FeatureArr* points);
  void TrackFeatures(const ImgGray& img, FeatureArr* points, bool replace_lost_features);
  void TrackFeatures(const ImgGray& img1, const ImgGray& img2, FeatureArr* points, bool replace_lost_features);
  void DrawFeatures(ImgBgr* img, const FeatureArr& points, const Bgr& color);
  void SetParams(const Params& new_params);
  Params GetParams();

  void ReadFeaturesAscii(const char* fname, FeatureArr* features)
  {
    FILE* fp = fopen(fname, "rt");
    if (fp)
    {
      int n;
      fscanf(fp, "%d\n", &n);
      features->Reset(n);
      for (int i=0 ; i<n ; i++)
      {
        float x, y;
        fscanf(fp, "%f %f\n", &x, &y);
        (*features)[i].x = x;
        (*features)[i].y = y;
      }
      fclose(fp);
    }
    else
    {
      assert(0);
    }
  }
  bool SaveFeatures(const FeatureArr& points, const CString& filename);

private:
//  ImgFloat m_tmp1, m_tmp2;
//  ImgGray m_prev_img;
  void* m_data;
  Params m_params;
};

struct jFeature
{
  jFeature() : x(-1), y(-1), val(-1), dx(0), dy(0) {}
  jFeature(double xx, double yy, double vval) : x((float) xx), y((float) yy), val((float) vval), dx(0), dy(0) {}
  float x, y, val;
  bool operator<(const jFeature& other) const { return val < other.val; }
  bool operator>(const jFeature& other) const { return val > other.val; }
  float dx, dy;  // shift last time, for velocity prediction (not used)

  // used for computing affine warp
  // (Note:  Should go ahead and do LU decomposition on the 6x6 matrix up front
  //  for efficiency in solving the linear system later)
  ImgFloat m_aff_initial_window;
  MatDbl m_aff_grad6x6, m_aff_grad6x1; 
};

struct jGaussDerivPyramid
{
  std::vector< ImgFloat > image, gradx, grady;
  int NLevels() const
  {
    assert( image.size() == gradx.size() && image.size() == grady.size() );
    return image.size();
  }
};

typedef enum { FD_MINEIG, FD_MINMAXEIG, FD_HARRIS, FD_TRACEDET } FeatureDetectMeasure;
void jComputeGaussDerivPyramid(const ImgGray& img, jGaussDerivPyramid* out, int nlevels = 3);
void jDetectFeatures(const jGaussDerivPyramid& pyr, int nfeatures, std::vector<jFeature>* features, FeatureDetectMeasure fdm = FD_MINEIG);
void jTrackFeatures(std::vector<jFeature>* features, const jGaussDerivPyramid& pyr1, const jGaussDerivPyramid& pyr2);
void jReplaceFeatures(std::vector<jFeature>* features, const jGaussDerivPyramid& pyr, int mindist = 8);

void jDrawFeatures(ImgBgr* img, const std::vector<jFeature>& features);

int jCountValidFeatures(const std::vector<jFeature>& features);
int jFindClosestValidFeature(float x, float y, const std::vector<jFeature>& features);

void jTrackFeaturesJointly
(
  std::vector<jFeature>* features, 
  const jGaussDerivPyramid& pyr1, 
  const jGaussDerivPyramid& pyr2
);

//class KltFeatureList;
//class FastFeatureTracker;

void jConvertFromFeatureList(const KltFeatureList& fl, std::vector<jFeature>* features);
void jConvertToFeatureList(const std::vector<jFeature>& features, KltFeatureList* fl);
void jConvertFromFeatureArr(const FastFeatureTracker::FeatureArr& fa, std::vector<jFeature>* features);
void jConvertToFeatureArr(const std::vector<jFeature>& features, FastFeatureTracker::FeatureArr* fa);
//void jDetectFeatures(const ImgGray& img, std::vector<jFeature>* features);
//void jDetectFeaturesBaseline(const ImgGray& img, std::vector<jFeature>* features);
//void jTrackFeatures(const ImgGray& img1, const ImgGray& img2, std::vector<jFeature>* features);

void jReadFeaturesAscii(const char* fname, std::vector<jFeature>* features);

// Basic Horn-Schunck algorithm (no pyramids)
void HornSchunck(
  const jGaussDerivPyramid& pyr1, 
  const jGaussDerivPyramid& pyr2,
  ImgFloat* u_out,
  ImgFloat* v_out,
  float lambda
);

/**
@class EllipticalHeadTracker

Implements the algorithm in "Elliptical Head Tracking by Intensity Gradients and Color Histograms", CVPR 1998

The first time you instantiate an elliptical head tracking object, the pixel lists 
will be precomputed.  These are needed to speed up the run time computation, but 
they take awhile to compute.  Once computed, they will be automatically saved to a 
file in the current directory.  The constructor looks for this file and uses it if
found.  

Also, you can only instantiate one of these objects in a single application.  Sorry, but
this implementation is based on very old code.  

@author Stan Birchfield
*/

class EllipticalHeadTracker
{
public:
  
  // the state of the ellipse
  struct EllipseState
  {
    EllipseState() {}
    EllipseState(int xx, int yy, int size) : x(xx), y(yy), sz(size) {}
    int x;         // x location
    int y;         // y location
    int sz;        // size (the width of the ellipse is 2*sz+1; the height is this number times the fixed aspect ratio)
  };

public:

  EllipticalHeadTracker();
  virtual ~EllipticalHeadTracker();

  void SetState(const EllipseState& state) 
  { 
    m_state = state; 
    m_xvel = 0;
    m_yvel = 0;
  }
  void BuildModel(const ImgBgr& img, const EllipseState& state);
  void SaveModel(const char* filename);
  void LoadModel(const char* filename);

  // These functions are shortcuts for calling either
  //    SetState and BuildModel, or
  //    SetState and LoadModel
  void Init(const ImgBgr& img, const EllipseState& initial_state);
  void Init(const EllipseState& initial_state, const char* filename);

  // Tracks the target from the last known position, using constant velocity prediction.
  //
  // Important:  Before calling this function, be sure to 
  //                . set the ellipse state, and
  //                . build or load a model of the head to track
  //             To do this, either 
  //                . call Init(), or 
  //                . call SetState() and either BuildModel() or LoadModel()
  //
  // Parameters:
  // 'img' should be the same size each time you call this
  // 'use_gradient':  0 means do not consider gradient at all
  //                  1 means use gradient dot product (default)
  //                  2 means use gradient magnitude
  // 'use_color':     0 means do not use color histogram at all
  //                  1 means use color histogram (default)
  // Of course, do not set both 'use_gradient' and 'use_color' to 0, or you'll get garbage.
  EllipseState Track(const ImgBgr& img, int use_gradient = 1, int use_color_histogram = 1);

  void OverlayEllipse(const EllipseState& state, ImgBgr* img);

  // returns the minimum and maximum ellipse size
  int GetMinSize() const;
  int GetMaxSize() const;
  // returns half the search range (i.e., 4 means +/- 4, 1 means +/- 1, etc.)
  void GetSearchRange(int* x, int* y, int* size);

private:
  int m_xvel, m_yvel;  // velocities (for prediction)
  EllipseState m_state;
};

// the simplest implementation of level sets possible
void LevelSetPropagate(const ImgGray& img);

struct ChanVeseParams
{
  float mu;         // weight for length
  float nu;         // weight for area
  float lambda_i;   // weight for interior pixels
  float lambda_o;   // weight for exterior pixels
  int init_border;  // size of initial border
  int max_niter;    // maximum number of iterations
  bool display;     // whether to display results during algorithm
};

// Implements the Chan-Vese segmentation algorithm, described in 
// T. F. Chan and L. A. Vese, Active Contours without Edges, IEEE Trans. on Image Processing, 10(2), 2001.
void ChanVese(const ImgGray& img, ImgBinary* out, const ChanVeseParams& params);

// Fit contiguous straight lines to edge map (Douglas-Peucker algorithm)
class LineFitting  
{
public:
  struct Line
  {
    // the line segment goes from 'p1' to 'p2'
    CPoint p1;
    CPoint p2;

    // these are redundant values needed for internal computation
    // (we should be hiding these)
    float theta;
    float rho;

    float Len() { return sqrtf(float((p1.x-p2.x)*(p1.x-p2.x)+(p1.y-p2.y)*(p1.y-p2.y))); }
  };
	LineFitting();

	virtual ~LineFitting();
  void DetectLines(const ImgBgr& img, int min_len, std::vector<Line> *lines);
  void DetectLines(const ImgBinary& edgeim, int min_len, std::vector<Line> *lines);
  void FitLine2D(const std::vector<CPoint>& pts, LineFitting::Line *line);
};
// The Hough transform, implemented in OpenCV
class OpenCvHough
{
public:
  OpenCvHough();
  virtual ~OpenCvHough();
  void Houghlines(const ImgBgr& img, std::vector<LineFitting::Line> *lines);
  void Houghlines(const ImgGray& img, std::vector<LineFitting::Line> *lines);
  void Houghlines(const ImgBgr& img, float sigma, int threshold, int min_len, int gap, std::vector<LineFitting::Line> *lines);

private:
  CvMemStorage* storage;
  CvSeq* cvlines;
};

class ColorHistogramx
{

private:
	std::vector<int> val;
	int n_bins_color[3];
	int n_bins_tot;
	int binwidth_color[3];

public:
	ColorHistogramx();
	~ColorHistogramx();
	void ComputeHistogram(const ImgGray& color1,
									  const ImgGray& color2,
									  const ImgGray& color3);
	void ComputeHistogram(const ImgGray& color1,
									  const ImgGray& color2,
									  const ImgGray& color3,
									  const ImgBinary& mask);
	static double CompareHistograms(const ColorHistogramx& ch1, const ColorHistogramx& ch2);
	static double CompareHistogramsBhattacharyya(const ColorHistogramx& ch1, const ColorHistogramx& ch2);
	void WriteToFile(const char *fname) const;
	void WriteColorToFile(const char *fname) const;
	void ReadFromFile(const char *fname);
	int operator[] (int c) const {
		return val[c];
	};
	int GetIndex(unsigned char color1, unsigned char color2, unsigned char color3);
	void GetColor(int index, unsigned char* color1, unsigned char* color2, unsigned char* color3);
	int Sum(ImgBinary& mask){
		return(blepo::Sum(mask));
	};
	std::vector<double> Normalize(int totalUnits);

	int GetNumberOfBins(int color_number){
		return(n_bins_color[color_number-1]);
	};
	void SetNumberOfBins(int color_number, int number_bins){
		assert(256%number_bins==0);
		n_bins_color[color_number-1]=number_bins;
		binwidth_color[color_number-1]=256/number_bins;
		n_bins_tot=n_bins_color[0] * n_bins_color[1] * n_bins_color[2];
		val.resize(n_bins_tot);
	};
	int GetWidthOfBins(int color_number){
		return(binwidth_color[color_number-1]);
	};
	void SetWidthOfBins(int color_number, int width){
		assert(256%width==0);
		binwidth_color[color_number-1]=width;
		n_bins_color[color_number-1]=256/width;
		n_bins_tot=n_bins_color[0] * n_bins_color[1] * n_bins_color[2];
		val.resize(n_bins_tot);
	};
	int GetTotalBins() { return n_bins_tot; };

	ColorHistogramx& operator=(const ColorHistogramx& other)
	{
		for(int i=0; i<n_bins_tot; i++)
			val[i] = other[i];
		return *this;
	};
	ColorHistogramx& operator+=(const ColorHistogramx& other)
	{
		for(int i=0; i<n_bins_tot; i++)
			val[i] += other[i];
		return *this;
	};
	ColorHistogramx& operator-=(const ColorHistogramx& other)
	{
		for(int i=0; i<n_bins_tot; i++)
			val[i] -= other[i];
		return *this;
	};
};

};  // end namespace blepo

#endif // __BLEPO_IMAGEALGORITHMS_H__
