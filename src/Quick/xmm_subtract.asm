; Copyright (c) 2004,2006 Clemson University.
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
; void xmm_subtract(unsigned char *src1, unsigned char *src2, unsigned char *dst, int num_doublequadwords);
;
; Saturated subtraction between two one-dimensional byte arrays, using XMM registers.
;   (If the result is less than 0, it is set to 0.)
;
; Notes:  * a doublequadwords is 16 bytes
;         * num_doublequadwords must be > 0
;         * This code assumes that SSE2 commands are available
;         * It is okay if src1==src2 and/or src1==dst and/or src2==dst
;         * No assumption is made about the doublequadword-alignment of the pointers
;--------------------------------------------------------------------------------------
;
; @author Manish Shiralkar
; @author Stan Birchfield

bits 32                         ;for 32 bit protected mode

section .text code              ;code starts here 
global _xmm_subtract                 

_xmm_subtract:

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

.loop:
  movdqu xmm1, [esi+eax]        ;fetch 16 bytes of 1st img
  movdqu xmm3, xmm1             ;copy 16 bytes of 1st img
  movdqu xmm2, [ebx+eax]        ;fetch 16 bytes of 2nd img
  psubusb xmm1, xmm2            ;xmm1 - xmm2 (-ve results set to 0)
  movdqu [edi+eax], xmm1        ;store the 'ABS DIFFERENCE' result in output img
  add eax, 16                   ;This will get next 16 bytes in next iteration
  loop .loop

  pop ebx
  pop edi
  pop esi
  pop ebp
ret
