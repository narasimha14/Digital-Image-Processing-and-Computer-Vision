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
; void mmx_gauss_1x3(unsigned char *src, unsigned char *dst, int width, int height);
;
; Convolution with a horizontal (1/4)*[1 2 1] Gaussian kernel,
;   in a grayscale image with one byte per pixel.
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
global _mmx_gauss_1x3

_mmx_gauss_1x3:

  push ebp                      ;save caller functions stack pointer 
  mov ebp, esp                  ;copy stack pointer to ebp
                                ;ebp+0:retAddr, ebp+4:caller fn ebp, ebp+8:1st param
  push esi
  push edi
  push ebx

  mov esi, [ebp+8]              ;pointer to first row of Image
  mov edi, [ebp+12]             ;pointer to output image
  mov ebx, [ebp+16]             ;width

  pxor mm3, mm3                 ;used in pack operation  

  mov edx, 2                    ;outer loop count
outer_loop:
  xor eax, eax
  mov ecx, ebx                  ;width
  shr ecx, 2                    ;width is now treated as multiple of 4

inner_loop:
  
  movq mm0, [esi+eax]           ;1st row data
  punpcklbw mm1, mm0            ;unpack 4 words into mm1
  psrlw mm1, 8                  ;move pixels to lsb of the words

  inc eax                       ;for accessing 2nd row  

  movq mm0, [esi+eax]           ;2nd row data
  punpcklbw mm2, mm0            ;unpack 4 words into mm2
  psrlw mm2, 8                  ;move pixels to lsb of the words

  psllw mm2, 1                  ;multiply 2nd row data by 2
  paddusw mm1, mm2              ;(1st row) + 2*(2nd row)

  inc eax                       ;for accessing 3rd row

  movq mm0, [esi+eax]           ;3rd row data
  punpcklbw mm2, mm0            ;unpack 4 words into mm2
  psrlw mm2, 8                  ;move pixels to lsb of the words

  paddusw mm1, mm2              ;(1st row) + 2*(2nd row) + (3rd row)
  psrlw mm1, 2                  ;divide each word (2nd row data) by 4
                                ;multiplication by (1/4)*[1 2 1] complete

  packuswb mm1, mm3             ;convert word to byte
  dec eax                       
  movq [edi+eax], mm1           ;write in 2nd row

  add eax, 3                    ;for next set of 4 pixels
  loop inner_loop               ;loop till row is processed, i.e till cx==0
  
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
