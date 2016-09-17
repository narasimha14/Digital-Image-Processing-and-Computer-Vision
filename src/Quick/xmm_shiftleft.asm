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
; void xmm_shiftleft(unsigned char *src, unsigned char *dst, int shift_amount, int num_doublequadwords);
;
; Shifts a contiguous array of bits left by a fixed number of bits, using XMM registers.
;    'shift_amount' is the number of bits by which to shift.
;    Bits are shifted across byte/word/quadword/doublequadword etc. boundaries within the array.
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
; Notes:  * a doublequadword is 16 bytes
;         * num_doublequadwords must be > 0
;         * shift_amount must be >= 0 and <= 64 (since 64 bits are in a doublequad word -- half of an XMM register)
;         * This code assumes that SSE2 commands are available
;         * It is okay if src==dst
;         * No assumption is made about the doublequadword-alignment of the pointers
;--------------------------------------------------------------------------------------
;
; @author Stan Birchfield

bits 32
section .data                   ;data segment
  maskval db 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF  
section .text code
global _xmm_shiftleft

_xmm_shiftleft:

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
  movd  xmm6, ebx                  ;set xmm6 = ebx (amount to shift left)
  movd  xmm7, edx                  ;set xmm7 = edx (amount to shift previous value right)

  ;Code to process the array
  ;  Note that the Intel instructions treat the 128-bit XMM registers as two 64-bit
  ;    registers.  Therefore, shifting does not preserve bits across the boundary
  ;    between the two quadwords.  The extra instructions before the shift commands
  ;    below are necessary to handle this problem. 
  ;
  pxor   xmm0, xmm0                 ;set xmm0 = 0 (for first iteration)
  movq   xmm5, [maskval]            ;set xmm5 = 0
.loop:
  movdqu xmm1, [esi+eax]            ;fetch 16 bytes of img to xmm1
  movdqu xmm2, xmm1                 ;copy doublequadword to xmm2 (for later)
  movdqu xmm4, xmm1                 ;shift doublequadword in xmm1 left by shift_amount
  pand   xmm4, xmm5                 ;  (extra instructions are needed to save bits near
  psrlq  xmm4, xmm7                 ;   boundary b/w two quadwords -- bit 63, 62, etc..
  pshufd xmm4, xmm4, 0x4e           ;   These bits are saved in xmm4, shifted down,
  psllq  xmm1, xmm6                 ;   low and hi quadwords swapped, and or'ed with xmm1
  por    xmm1, xmm4                 ;   to retain these bits across the boundary)
  pshufd xmm0, xmm0, 0x4e           ;shift doublequadword in xmm0 right by (128 minus shift_amount)
  pand   xmm0, xmm5                 ;  (swap low and hi quadwords, mask out hi quadword,
  psrlq  xmm0, xmm7                 ;   and shift low quadword)
  por    xmm1, xmm0                 ;logical-or high bits in xmm0 with low bits in xmm1
  movdqu [edi+eax], xmm1            ;store the shifted result in output img
  movdqu xmm0, xmm2                 ;copy xmm2 to xmm0 for next time
  add    eax,  16                   ;This will get next 16 bytes in next iteration
  loop  .loop

  pop edx
  pop ebx
  pop eax
  pop edi
  pop esi
  pop ebp
ret
