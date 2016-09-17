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
; void mmx_const_or(unsigned char *src, const unsigned char val, unsigned char *dst, int num_quadwords);
;
; Bitwise OR of a one-dimensional byte array with another byte, using MMX registers
;
; Notes:  * a quadword is 8 bytes
;         * num_quadwords must be > 0
;         * This code assumes that MMX commands are available
;         * It is okay if src==dst 
;         * No assumption is made about the quadword-alignment of the pointers
;--------------------------------------------------------------------------------------
;
; @author Manish Shiralkar

bits 32                   ;for 32 bit protected mode

section .data             ;data segment
buffer dd 0x00000000
       dd 0x00000000

section .text code        ;code starts here 
global _mmx_const_or      ;export label

_mmx_const_or:

  push ebp                ;save stack pointer of caller function
  mov ebp, esp            ;copy stack pointer to ebp
                          ;ebp+0:retAddr, ebp+4:caller fn ebp, ebp+8:1st param
  push esi
  push edi
  push ebx

  mov esi, [ebp+8]        ;pointer to first input image
  mov bx, [ebp+12]        ;const value
  mov edi, [ebp+16]       ;pointer to output image
  mov ecx, [ebp+20]       ;number of loops

  
  xor eax, eax            ;set eax to 00000000H (zero)
  mov edx, 8
.label1:                  ;fill the buffer with copies of the const val
  mov [buffer + eax], bx
  inc eax
  dec edx
  jnz .label1

  movq mm1, [buffer]      ;fetch 8 bytes buffer
  xor eax, eax            ;set eax to 00000000H (zero) 
.loop:
  movq mm0, [esi+eax*8]   ;fetch 8 bytes of 1st img
  por mm0, mm1           ;'OR' 1st img  with const val buffer
  movq [edi+eax*8], mm0   ;store the 'OR' result in output img
  inc eax                 ;This will get next 8 bytes in next iteration
  loop .loop

  pop ebx
  pop edi
  pop esi
  pop ebp
  emms                    ;finished using MMX
ret
