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
; void xmm_not(unsigned char *src, unsigned char *dst, int num_doublequadwords);
;
; Bitwise negation of a one-dimensional byte array, using XMM registers
;
; Notes:  * a doublequadword is 16 bytes
;         * num_doublequadwords must be > 0
;         * This code assumes that SSE2 commands are available
;         * It is okay if src==dst
;         * No assumption is made about the doublequadword-alignment of the pointers
;           (If the pointers are known to be aligned, i.e., ((int) src1) % 16 == 0, etc.,
;            then the code could presumably be sped up by replacing movdqu with movdq,
;            and by calling 'pand xmm1 [ebx+eax]' directly rather than loading the values
;            into xmm2)
;--------------------------------------------------------------------------------------
;
; @author Manish Shiralkar
; @author Stan Birchfield

bits 32

section .data
andMask dd 0xFFFFFFFF
        dd 0xFFFFFFFF
        dd 0xFFFFFFFF
        dd 0xFFFFFFFF

section .text code
global _xmm_not

_xmm_not:

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

  movdqu xmm0, [andMask]          ;fetch 8 bytes mask

.loop:
  movdqu xmm1, [esi+eax]          ;fetch 16 bytes of 1st img
  pandn xmm1, xmm0                ;'ANDN' with all 1's (i.e.g, negate the pixels)
  movdqu [edi+eax], xmm1          ;store the 'NOT' result in output img
  add eax, 16                     ;This will get next 16 bytes in next iteration
  loop .loop

  pop ebx
  pop edi
  pop esi
  pop ebp
ret
