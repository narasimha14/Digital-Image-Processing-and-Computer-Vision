; Copyright (c) 2005 Clemson University.
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
; void mmx_erode(unsigned char *src1, unsigned char *src2, unsigned char *dst, int width, int height);
; Func :  * Morphological erosion of input images with structuring element using MMX registers
; Notes:  * This code assumes that MMX commands are available
;         * No assumption is made about the alignment of the pointers
;         * Sets the one-pixel-wide border around the image to zero
;--------------------------------------------------------------------------------------
;
; @author Manish Shiralkar

bits 32                         ;for 32 bit protected mode

section .data                   ;data segment
  buffer1 dd 0x00FF00FF
  buffer2 dd 0x000000FF
section .text code              ;code starts here
global _mmx_erode               ;export label

_mmx_erode:

  push ebp                      ;save caller functions stack pointer 
  mov ebp, esp                  ;copy stack pointer to ebp
                                ;ebp+0:retAddr, ebp+4:caller fn ebp, ebp+8:1st param
  push esi
  push edi
  push ebx

  mov esi, [ebp+8]              ;pointer to first row of Image
  mov ebx, [ebp+12]             ;pointer to first row of template
  mov edi, [ebp+16]             ;pointer to output image
  ;--mov edx, [ebp+24]          ;number of rows

  movq mm0, [ebx]               ;get TEMPLATE data
  punpcklbw mm1, mm0            ;unpack 4 words into mm1
  psrlw mm1, 8                  ;move pixels to lsb of the words
  movq mm0, [ebx+4]             ;get 8 pixels
  punpcklbw mm2, mm0            ;unpack 4 words into mm2
  psrlw mm2, 8                  ;move pixels to lsb of the words
  movq mm0, [ebx+8]             ;get 8 pixels
  punpcklbw mm3, mm0            ;unpack 4 words into mm3
  psrlw mm3, 8                  ;move pixels to lsb of the words

  ;redefinition of ebx
  ;-------------------
  mov ebx, [ebp+20]             ;width or (number of cols)

  xor eax, eax                  ;row index
  add eax, 2                    ;last row index = height - 2

                                ;set first row to all zeros
  mov ecx, [ebp+20]             ;width or (number of cols)
  set_first_row:
  ;------------  
  mov byte [edi+ecx-1], 0
  loop set_first_row            ;loop for width times    

outer_loop:
  ;---------
  mov ecx, [ebp+20]
  sub ecx, 2                    ;loopcount = width - 2

  mov byte [edi+ebx], 0         ;Set first col pixel to 0

inner_loop:
  ;---------
  pxor mm7, mm7                 ;accumulator
  movq mm0, [esi]               ;get IMAGE data
  punpcklbw mm4, mm0            ;unpack 4 words into mm4
  psrlw mm4, 8                  ;move pixels to lsb of the words
  movq mm0, [esi+ebx]           ;get 8 pixels
  punpcklbw mm5, mm0            ;unpack 4 words into mm5
  psrlw mm5, 8                  ;move pixels to lsb of the words
  movq mm0, [esi+2*ebx]         ;get 8 pixels
  punpcklbw mm6, mm0            ;unpack 4 words into mm6
  psrlw mm6, 8                  ;move pixels to lsb of the words

  pand mm4, mm1                 ;detect if there is any match   
  pand mm5, mm2                  
  pand mm6, mm3
  
  pxor mm4, mm1                 ;xor with template should result   
  pxor mm5, mm2                 ;in zero if the template match is found   
  pxor mm6, mm3                 ;otherwise non zero value will be generated.  
 
  por  mm4, mm5                 ;even a single '1' is good to terminate erode
  por  mm4, mm6
  
  movq mm0, mm4                 ;result is sitting in two pieces in mm4
  psrlq mm4, 32                 ;slide upper piece to bottom of mm4
  por  mm0, mm4                 ;complete result in lower half of mm0   

  movd edx, mm0
  cmp edx, 0
  
  jz .not_found
  mov byte [edi+ebx+1], 0       ;write result
  jmp .skip_not_found

.not_found:
  ;---------
  mov byte [edi+ebx+1], 255     ;write result

.skip_not_found:
  ;-----
  inc esi                       ;for operation at next pixel location
  inc edi                       ;keep edi and esi in sync
  loop inner_loop               ;loop for width-2 times
  ;--------------

  mov byte [edi+ebx+1], 0       ;Set last col pixel to 0

  add esi, 2                    ;point to new row
  add edi, 2                    ;keep edi and esi in sync

  inc eax                       ;increment row index
  cmp eax, [ebp+24]             ;loop until done with all rows 
  jl outer_loop
  ;------------

                                ;set last row to all zeros
  add edi, ebx                  ;point edi to last but one row   
  mov ecx, [ebp+20]             ;width or (number of cols)
  set_last_row:
  ;------------  
  mov byte [edi+ecx-1], 0
  loop set_last_row             ;loop for width times    

  pop ebx
  pop edi
  pop esi
  pop ebp
  emms
ret
