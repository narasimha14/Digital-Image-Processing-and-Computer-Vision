; Copyright (c) 2004 Clemson University.
; 
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or (at
; your option) any later version.
;  
; This program is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; General Public License for more details.
;  
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;
;--------------------------------------------------------------------------------------
; void mmx_convolve_prewitt_vert_abs(const unsigned char* src, unsigned char* dst, int width, int height);
;
; Absolute difference between pixels that are vertically spaced exactly two pixels apart,
;   in a grayscale image with one byte per pixel.
;   (i.e., the absolute value of the convolution with a vertical Prewitt [-1 0 1]^T mask.
;    There is no (1/3)*[1 1 1] horizontal smoothing.)
;
; Notes:  * width and height must be > 0
;         * This code assumes that MMX commands are available
;         * It is NOT okay if src==dst 
;         * No assumption is made about the quadword-alignment of the pointers
;--------------------------------------------------------------------------------------
;
; @author Manish Shiralkar

bits 32

section .text code
global _mmx_convolve_prewitt_vert_abs

_mmx_convolve_prewitt_vert_abs:

  push ebp                      ;save caller functions stack pointer 
  mov ebp, esp                  ;copy stack pointer to ebp
                                ;ebp+0:retAddr, ebp+4:caller fn ebp, ebp+8:1st param
  push esi
  push edi
  push ebx

  mov esi, [ebp+8]              ;pointer to first row of Image
  mov edi, [ebp+12]             ;pointer to output image
  mov ebx, [ebp+16]             ;width

  mov edx, 3                    ;outer loop count
outer_loop:
  mov ecx, ebx                  ;width
  shr ecx, 3                    ;width is now treated as multiple of 8
  
  xor eax, eax
  push edx

inner_loop:
  movq mm0, [esi+eax*8]         ;get 8 bytes of 1st row
  mov edx, esi                  ;2nd row is not considered as it is multiplied by 0's
  add edx, ebx
  add edx, ebx                  ;edx = esi + 2*ebx  
  movq mm1, [edx+eax*8]         ;get 8 bytes of 3rd row
 
  psubusb mm0, mm1              ;mm0 - mm1 (-ve results set to 0)
  psubusb mm1, [esi+eax*8]      ;mm1 - mm0 (-ve results set to 0)
  por mm0, mm1                  ;abs diff between row1 and row3

  mov edx, edi
  add edx, ebx                  ;edx = edi + ebx  
  movq [edx+eax*8], mm0          ;write convolution result in 2nd row	

  inc eax                       ;for next set of 8 pixels
  loop inner_loop               ;loop till row is processed, i.e till cx==0
  
  pop edx
  add esi, ebx                  ;point to new row
  add edi, ebx                  ;keep edi and esi in sync

  inc edx                       ;increment row index
  cmp edx, [ebp+20]             ;loop until done with all rows 
  jl outer_loop

  pop ebx
  pop edi
  pop esi
  pop ebp
  emms
ret
