/* 
 * Copyright (c) 2004 Clemson University.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details..
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __BLEPO_MATH_H__
#define __BLEPO_MATH_H__

#include <stdlib.h>  // RAND_MAX
#include <math.h>  // pow, powf, exp, expf
#include <assert.h>
#include <float.h>  // _isnan
/**
@namespace blepo_ex

  Basic math-related functions.

@author Stan Birchfield (STB)
*/

namespace blepo_ex
{
  template <typename T> inline T Abs(T a) { return (a>0) ? a : -a; }

  template <typename T> inline T Min(T a, T b) { return (a<b) ? a : b; }
  template <typename T> inline T Max(T a, T b) { return (a>b) ? a : b; }
  inline int Round (float a)  
  {
    assert( !_isnan( a ) );
//    int b;
//    _asm
//    { // assumes that we are in 'round mode', which is the default; should probably check
//      FLD a   ; load floating-point value
//      FIST b  ; store integer
//    };
//#ifndef NDEBUG
//    int c = static_cast<int>(a>=0 ? a+0.5f : a-0.5f);
//#endif
//    assert( b == static_cast<int>(a>=0 ? a+0.5f : a-0.5f) );
//    return b;   
    return static_cast<int>(a>=0 ? a+0.5f : a-0.5f); 
  } 
  inline int Round (double a)
  {
    assert( !_isnan( a ) );
//    int b;
//    _asm
//    { // assumes that we are in 'round mode', which is the default; should probably check
//      FLD a   ; load floating-point value
//      FIST b  ; store integer
//    };
//#ifndef NDEBUG
//    int c = static_cast<int>(a>=0 ? a+0.5f : a-0.5f);
//#endif
//    assert( b == static_cast<int>(a>=0 ? a+0.5f : a-0.5f) );
//    return b;   
    return static_cast<int>(a>=0 ? a+0.5f : a-0.5f); 
  } 


//  template <typename T> T Pow(T a, T b);
//  template <typename T> T Exp(T a);
  inline int Ceil(float a) { return static_cast<int>(ceil(a)); }
  inline int Floor(float a) { return static_cast<int>(floor(a)); }

  // return the closest value to 'a' within the range [minn, maxx]
  template <typename T> inline T Clamp(T a, T minn, T maxx)
    { return (a < minn) ? minn : ( (a > maxx) ? maxx : a ); }

  inline float Pow(float a, float b) { return powf(a,b); }
  inline double Pow(double a, double b) { return pow(a,b); }
  inline int Pow(int a, int b) { return static_cast<int>(pow((double)a,(double)b)); }

  inline float Exp(float a) { return expf(a); }
  inline double Exp(double a) { return exp(a); }

  // @name Random number functions
  /// Get a random integer between 0 and modulus-1, inclusive.
  /// The seed is automatically initialized the first time GetRand() is called
  /// @modulus Must be no greater than RAND_MAX+1
  int GetRand(unsigned int modulus = RAND_MAX+1);
  /// Get a random integer between low and high-1, inclusive
  int GetRand(int low, int high);
  /// Get a random double in the range [0,1) using uniform distribution
  double GetRandDbl();

  /// Fast algorithm for tmp=a;  a=b;  b=a;
  template <typename T> inline void Exchange(T* a, T* b)
  { 
    T tmp = *a;  *a = *b;  *b = tmp;
//    unsigned long long int& aa = *(reinterpret_cast<unsigned long long int*>(a)); 
//    unsigned long long int& bb = *(reinterpret_cast<unsigned long long int*>(b)); 
//    aa^=bb;  bb^=aa;  aa^=bb;
//    { a^=b;  b^=a;  a^=b; }
  }

  inline bool Similar(double d1, double d2, double tolerance)
  {
    return fabs(d1-d2) <= tolerance;
  }

  /// The number pi, both double and float versions
  extern const double Pi;
  extern const float Pif;

  inline double Deg2Rad(double deg) { return deg * Pi / 180.0; }
  inline double Rad2Deg(double rad) { return rad * 180.0 / Pi; }

};  // namespace blepo_ex

#endif //__BLEPO_MATH_H__
