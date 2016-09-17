; Copyright (c) 2007 Clemson University.
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
; void mmx_shiftleft(unsigned char *src, unsigned char *dst, int shift_amount, int num_quadwords);
;
; Shifts a contiguous array of bits left by a fixed number of bits, using MMX registers.
;    'shift_amount' is the number of bits by which to shift.
;    Bits are shifted across byte/word/quadword etc. boundaries within the array.
;    The final 'shift_amount' bits at the very end of the array are filled with zeros.
;    
;    The format of the array is the following:  
;       Bit 0 of the array is in the least significant bit of the first byte
;           ...
;       Bit 7 of the array is in the most significant bit of the first byte
;       Bit 8 of the array is in the least significant bit of the second byte
;           ...
;       and so on.  In other words, Bit i of the array is in bit i%8 of byte i/8
;    The array can be visualized in right-to-left order as follows:
;
;          ... b16 | b15 b14 b13 b12 b11 b10 b9 b8 | b7 b6 b5 b4 b3 b2 b1 b0 |
;             B2   |               B1              |            B0           |
;
;       where bi is bit i of the array, and Bi is byte i of the array.  Adjacent
;       pixels in this visualization are adjacent in the array.
;    Note that a bitwise left shift of the array is equivalent to the following:
;           { for (i=n-1 ; i>=0 ; i++)  b[i] = b[i-1]; }
;       In other words, values shift up by one index, which is counterintuitive
;       to anyone accustomed to the left-to-right convention of Western languages.
;
; Notes:  * a quadword is 8 bytes
;         * num_quadwords must be > 0
;         * shift_amount must be >= 0 and <= 64 (since 64 bits are in an MMX register)
;         * This code assumes that MMX commands are available
;         * It is okay if src==dst
;         * No assumption is made about the quadword-alignment of the pointers
;--------------------------------------------------------------------------------------
;
; @author Stan Birchfield

bits 32
;section .data                   ;data segment
;  swapval db 0x01,0x02,0x03,0x04,0x05,0x06,0x07  ; to load, "movq mm4, [swapval]"; for pshufb, which does not work with current version of nasm
section .text code
global _mmx_shiftleft

_mmx_shiftleft:

  push  ebp                        ;save caller functions stack pointer 
  mov   ebp, esp                   ;copy stack pointer to ebp
                                   ;ebp+0:retAddr, ebp+4:caller fn ebp, ebp+8:1st param
  push  esi
  push  edi
  push  eax
  push  ebx
  push  edx

  mov   esi, [ebp+8]               ;pointer to input image
  mov   edi, [ebp+12]              ;pointer to output image
  mov   ebx, [ebp+16]              ;pointer to number of bits to shift
  mov   ecx, [ebp+20]              ;number of loops

  xor   eax, eax                   ;set eax = 0
  mov   edx, 64                    ;set edx = (64 - ebx)
  sub   edx, ebx                   ;
  movd  mm6, ebx                   ;set mm6 = ebx (amount to shift left)
  movd  mm7, edx                   ;set mm7 = edx (amount to shift previous value right)

  ;Code to process the array
  ;
  pxor  mm0, mm0                   ;set mm0 = 0 (for first iteration)
.loop:
  movq  mm1, [esi+eax*8]           ;fetch 8 bytes of img to mm1
  movq  mm2, mm1                   ;copy quadword to mm2 (for later)
  psllq mm1, mm6                   ;shift quadword in mm1 right by shift_amount
  psrlq mm2, mm7                   ;shift quadword in mm0 left by (64 minus shift_amount)
  por   mm1, mm0                   ;logical-or high bits in mm0 with low bits in mm1
  movq  [edi+eax*8], mm1           ;store the shifted result in output img
  movq  mm0, mm2                   ;copy mm2 to mm0 for next time
  inc   eax                        ;This will get next 8 bytes in next iteration
  loop  .loop

  pop edx
  pop ebx
  pop eax
  pop edi
  pop esi
  pop ebp
  emms
ret
