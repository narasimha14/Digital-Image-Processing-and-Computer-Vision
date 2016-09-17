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
;File: mmx_supported.asm
;Tests if MMX/SSE/SSE2/SSE3 support is available using cpuid instruction.
;
;WARNING: Assumes that cpuid instruction is available. (i.e. assumes 486 and higher) 
;To be modified to check cpuid availability.
;
;For Visual C++
;Use Custom-Build: Add MMXSupport.asm in the project then 
;    Right Click on .asm->Settings->general->Always use custom build setup
;
;Custom Build command:	nasmw.exe -f win32 -o $(InputName).obj $(InputPath) 
;Output:  $(InputName).obj
;
;-l (generate listfile) is optional.
;
; CPUID instruction info at: http://prioris.mini.pw.edu.pl/~michalj/cpuid3
; Using NASM with VC++ at: http://www.cs.uaf.edu/~cs301/usingnasm.htm
; Also see Intel Software Programming Manuals online
;
;C Function prototype: extern "C" int mmx_supported(void)
;
; returns 
;   0  if nothing supported
;   1  if MMX supported
;   3  if SSE supported 
;   4  if SSE2 supported
;   5  if SSE3 supported
;
; @author Neeraj Kanhere
; @author Stan Birchfield (added SSE, SSE2, SSE3)
;-------------------------------------------------------------------------------------------------

  bits 32                 ; for 32 bit protected mode
  
  section .text	code      ; code starts here 

  global _mmx_supported   ; export label
  align 16                ; align segment for faster access.. not sure why not align 4 instead of 16			


_mmx_supported:
  push  ebx
  push  ecx
  push  edx
  mov   eax, 1            ; eax = 00000001h, i.e., cpuid will get CPU signature and built-in features
  cpuid                   ; CPU signature information is returned in EAX,EBX,ECX and EDX.
  xor   eax, eax          ; set eax to 0
  test  edx, 0x00800000   ; check bit 23 of EDX  (whether MMX is supported)
  jz    .DONE             ; 
  mov   eax, 1            ; MMX is supported, so set return value to 1
  test  edx, 0x02000000   ; check bit 25 of EDX  (whether SSE is supported)
  jz    .DONE             ; 
  mov   eax, 3            ; SSE is supported, so set return value to 3
  test  edx, 0x04000000   ; check bit 26 of EDX  (whether SSE2 is supported)
  jz    .DONE             ; 
  mov   eax, 4            ; SSE2 is supported, so set return value to 4
  test  ecx, 0x00000001   ; check ECX  (whether SSE3 is supported)
  jz    .DONE             ; 
  mov   eax, 5            ; SSE3 is supported, so set return value to 5

.DONE:
  pop   edx
  pop   ecx
  pop   ebx
  ret
