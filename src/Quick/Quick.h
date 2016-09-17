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
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __BLEPO_QUICK_H__
#define __BLEPO_QUICK_H__

namespace blepo
{

// These functions return whether the processor is capable of performing MMX/SSE/SSE2/SSE3 operations.
// The capability is automatically checked once upon startup.
bool CanDoMmx();   // mmx
bool CanDoSse();
bool CanDoSse2();  // xmm
bool CanDoSse3();

// This function allows the user to change the return value of the 'CanDo' functions above.
// After calling this function, all of the 'CanDo' functions will return false.
void TurnOffAllMmxSse();

const int nbytes_per_word = 2;              //  2
const int nbytes_per_quadword = 8;          //  8  (size of MMX registers)
const int nbytes_per_doublequadword = 16;   // 16  (size of XMM registers)

extern "C" {

// logical
void mmx_and(const unsigned char *src1, const unsigned char *src2, unsigned char *dst, int num_quadwords      );
void xmm_and(const unsigned char *src1, const unsigned char *src2, unsigned char *dst, int num_doublequadwords);
void mmx_or (const unsigned char *src1, const unsigned char *src2, unsigned char *dst, int num_quadwords      );
void xmm_or (const unsigned char *src1, const unsigned char *src2, unsigned char *dst, int num_doublequadwords);
void mmx_xor(const unsigned char *src1, const unsigned char *src2, unsigned char *dst, int num_quadwords      );
void xmm_xor(const unsigned char *src1, const unsigned char *src2, unsigned char *dst, int num_doublequadwords);
void mmx_not(const unsigned char *src , unsigned char *dst, int num_quadwords      );
void xmm_not(const unsigned char *src , unsigned char *dst, int num_doublequadwords);
void mmx_const_and(const unsigned char *src, const unsigned char val, unsigned char *dst, int num_quadwords      );
void xmm_const_and(const unsigned char *src, const unsigned char val, unsigned char *dst, int num_doublequadwords);
void mmx_const_or (const unsigned char *src, const unsigned char val, unsigned char *dst, int num_quadwords      );
void xmm_const_or (const unsigned char *src, const unsigned char val, unsigned char *dst, int num_doublequadwords);
void mmx_const_xor(const unsigned char *src, const unsigned char val, unsigned char *dst, int num_quadwords      );
void xmm_const_xor(const unsigned char *src, const unsigned char val, unsigned char *dst, int num_doublequadwords);

// shift an entire array of bits.  Bit i of the array is in bit i%8 of byte i/8.
void mmx_shiftleft (const unsigned char *src, unsigned char *dst, int shift_amount, int num_quadwords);
void xmm_shiftleft (const unsigned char *src, unsigned char *dst, int shift_amount, int num_doublequadwords);
void mmx_shiftright(const unsigned char *src, unsigned char *dst, int shift_amount, int num_quadwords);
void xmm_shiftright(const unsigned char *src, unsigned char *dst, int shift_amount, int num_doublequadwords);

// arithmetic
void mmx_absdiff   (const unsigned char *src1, const unsigned char *src2, unsigned char *dst, int num_quadwords      );
void xmm_absdiff   (const unsigned char *src1, const unsigned char *src2, unsigned char *dst, int num_doublequadwords);
void mmx_sum       (const unsigned char *src1, const unsigned char *src2, unsigned char *dst, int num_quadwords      );
void xmm_sum       (const unsigned char *src1, const unsigned char *src2, unsigned char *dst, int num_doublequadwords);
void mmx_subtract  (const unsigned char *src1, const unsigned char *src2, unsigned char *dst, int num_quadwords      );
void xmm_subtract  (const unsigned char *src1, const unsigned char *src2, unsigned char *dst, int num_doublequadwords);
void mmx_const_diff(const unsigned char *src , const unsigned char val  , unsigned char *dst, int num_quadwords      );
void xmm_const_diff(const unsigned char *src , const unsigned char val  , unsigned char *dst, int num_doublequadwords);
void mmx_const_sum (const unsigned char *src , const unsigned char val  , unsigned char *dst, int num_quadwords      );
void xmm_const_sum (const unsigned char *src , const unsigned char val  , unsigned char *dst, int num_doublequadwords);

// image processing
void mmx_convolve_prewitt_horiz_abs(const unsigned char* src, unsigned char* dst, int width, int height);
void mmx_convolve_prewitt_vert_abs (const unsigned char* src, unsigned char* dst, int width, int height);
void mmx_gauss_1x3(const unsigned char *src, unsigned char *dst, int width, int height);
void mmx_gauss_3x1(const unsigned char *src, unsigned char *dst, int width, int height);
void mmx_erode    (const unsigned char* src, const unsigned char* kernel, unsigned char* dst, int width, int height);
void mmx_dilate   (const unsigned char* src, const unsigned char* kernel, unsigned char* dst, int width, int height);

}

};  // end namespace blepo

#endif //__BLEPO_QUICK_H__
