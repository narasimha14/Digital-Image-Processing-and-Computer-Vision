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
; void mmx_sum(unsigned char *src1, unsigned char *src2, unsigned char *dst, int num_quadwords);
;
; Saturated sum of two one-dimensional byte arrays, using MMX registers
;
; Notes:  * a quadword is 8 bytes
;         * num_quadwords must be > 0
;         * This code assumes that MMX commands are available
;         * It is okay if src1==src2 and/or src1==dst and/or src2==dst
;         * No assumption is made about the quadword-alignment of the pointers
;--------------------------------------------------------------------------------------
;
; @author Manish Shiralkar

bits 32                         ;for 32 bit protected mode

section .text code              ;code starts here 
global _mmx_sum                 ;Add unsigned bytes with saturation, ie paddusb 

_mmx_sum:

  push ebp                      ;save caller functions stack pointer 
  mov ebp, esp                  ;copy stack pointer to ebp
                                ;ebp+0:retAddr, ebp+4:caller fn ebp, ebp+8:1st param
  push esi
  push edi
  push ebx

  mov esi, [ebp+8]              ;pointer to first input image
  mov ebx, [ebp+12]             ;pointer to second input image
  mov edi, [ebp+16]             ;pointer to output image
  mov ecx, [ebp+20]             ;number of loops

  xor eax, eax                  ;set eax to 00000000H (zero)

  ;Code to sum the image
  ;
.loop:
  movq mm0, [esi+eax*8]         ;fetch 8 bytes of 1st img
  paddusb mm0, [ebx+eax*8]      ;fetch 8 bytes of 2nd img and 'SUM' with those of 1st
  movq [edi+eax*8], mm0         ;store the 'SUM' result in output img
  inc eax                       ;This will get next 8 bytes in next iteration
  loop .loop

  pop ebx
  pop edi
  pop esi
  pop ebp
  emms
ret
