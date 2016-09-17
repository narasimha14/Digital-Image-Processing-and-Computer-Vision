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
; void mmx_not(unsigned char *src, unsigned char *dst, int num_quadwords);
;
; Bitwise negation of a one-dimensional byte array, using MMX registers
;
; Notes:  * a quadword is 8 bytes
;         * num_quadwords must be > 0
;         * This code assumes that MMX commands are available
;         * It is okay if src==dst
;         * No assumption is made about the quadword-alignment of the pointers
;--------------------------------------------------------------------------------------
;
; @author Manish Shiralkar

bits 32

section .data
andMask dd 0xFFFFFFFF
        dd 0xFFFFFFFF

section .text code
global _mmx_not

_mmx_not:

  push ebp                        ;save caller functions stack pointer 
  mov ebp, esp                    ;copy stack pointer to ebp
                                  ;ebp+0:retAddr, ebp+4:caller fn ebp, ebp+8:1st param
  push esi
  push edi
  push ebx

  mov esi, [ebp+8]                ;pointer to first input image
  mov edi, [ebp+12]               ;pointer to output image
  mov ecx, [ebp+16]               ;number of loops

  xor eax, eax                    ;set eax to 00000000H (zero)

  movq mm0, [andMask]             ;fetch 8 bytes mask

.loop:
  movq mm1, [esi+eax*8]           ;fetch 8 bytes of 1st img
  pandn mm1, mm0                  ;fetch 8 bytes of 2nd img and 'ANDN' with those of 1st
  movq [edi+eax*8], mm1           ;store the 'NOT' result in output img
  inc eax                         ;This will get next 8 bytes in next iteration
  loop .loop

  pop ebx
  pop edi
  pop esi
  pop ebp
  emms
ret
