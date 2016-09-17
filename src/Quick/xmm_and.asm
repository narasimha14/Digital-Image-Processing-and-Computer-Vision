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
; void xmm_and(unsigned char *src1, unsigned char *src2, unsigned char *dst, int num_doublequadwords);
;
; Bitwise AND of two one-dimensional byte arrays, using XMM registers
;
; Notes:  * a doublequadword is 16 bytes
;         * num_doublequadwords must be > 0
;         * This code assumes that SSE2 commands are available
;         * It is okay if src1==src2 and/or src1==dst and/or src2==dst
;         * No assumption is made about the doublequadword-alignment of the pointers
;           (If the pointers are known to be aligned, i.e., ((int) src1) % 16 == 0, etc.,
;            then the code could presumably be sped up by replacing movdqu with movdq,
;            and by calling 'pand xmm1 [ebx+eax]' directly rather than loading the values
;            into xmm2)
;--------------------------------------------------------------------------------------
; @author Manish Shiralkar
; @author Stan Birchfield

bits 32                   ; for 32 bit protected mode
section .text code        ; code starts here 
global _xmm_and           ; export label

_xmm_and:

  push ebp                ;save stack pointer of caller function
  mov ebp, esp            ;copy stack pointer to ebp
                          ;ebp+0:retAddr, ebp+4:caller fn ebp, ebp+8:1st param
  push esi
  push edi
  push ebx

  mov esi, [ebp+8]        ;pointer to first input image
  mov ebx, [ebp+12]       ;pointer to second input image
  mov edi, [ebp+16]       ;pointer to output image
  mov ecx, [ebp+20]       ;number of loops

  xor eax, eax            ;set eax to 00000000H (zero)

.loop:
  movdqu xmm1, [esi+eax]  ;fetch 16 bytes of 1st img
  movdqu xmm2, [ebx+eax]  ;fetch 16 bytes of 2nd img
  pand xmm1, xmm2         ;'AND' 
  movdqu [edi+eax], xmm1  ;store the 'AND' result in output img
  add eax, 16             ;This will get next 16 bytes in next iteration
  dec ecx
  jnz .loop

  pop ebx
  pop edi
  pop esi
  pop ebp
ret
