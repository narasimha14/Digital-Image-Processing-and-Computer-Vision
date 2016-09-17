;                   INSTRSET.ASM                        © Agner Fog 2004-05-30
;
; InstructionSet
; ==============
; This function returns an integer indicating which instruction set is
; supported by the microprocessor and operating system. A program can
; call this function to determine if a particular set of instructions can
; be used.
;
; The method used here for detecting whether XMM instructions are enabled by
; the operating system is different from the method recommended by Intel.
; The method used here has the advantage that it is independent of the 
; ability of the operating system to catch invalid opcode exceptions. The
; method used here has been thoroughly tested on many different versions of
; Intel and AMD microprocessors, and is believed to work reliably. For further
; discussion of this method, see my manual "How to optimize for the Pentium 
; family of microprocessors", 2004. (www.agner.org/assem/pentopt.pdf).
; 
; © 2003, 2004 GNU General Public License www.gnu.org/copyleft/gpl.html
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	  bits 32					; for 32 bit protected mode
	  
	  section .text	code    	; code starts here 

	  global _InstructionSet		; export label
	  align 16					; align segment for faster access.. not sure why not align 4 instead of 16			

;.686
;.xmm
;.model flat
;.code

;PublicAlias MACRO MangledName ; macro for giving a function alias public names
;        MangledName label near
;        public MangledName
;ENDM

; ********** InstructionSet function **********
; C++ prototype:
; extern "C" int InstructionSet (void);

; return value:
;  0 = use 80386 instruction set
;  1 or above = MMX instructions can be used
;  2 or above = conditional move and FCOMI can be used
;  3 or above = SSE (XMM) supported by processor and enabled by O.S.
;  4 or above = SSE2 supported by processor and enabled by O.S.
;  5 or above = SSE3 supported by processor and enabled by O.S.

;InstructionSet PROC NEAR
;PUBLIC InstructionSet                  ; extern "C" name (LINUX)
;PublicAlias _InstructionSet            ; extern "C" name (Windows)
;PublicAlias ?InstructionSet@@YAHXZ     ; MS mangled name
;PublicAlias @InstructionSet$qv         ; Borland mangled name
;PublicAlias _InstructionSet__Fv        ; Gnu mangled name (Windows)
;PublicAlias InstructionSet__Fv         ; Gnu mangled name (LINUX)
;PublicAlias _Z14InstructionSetv        ; Gnu mangled name (UNIX)

        nop                       ; circumvent bug in objcopy object file conversion utility
        nop
        nop
        nop
        push    ebx
        ; detect if CPUID instruction supported by microprocessor:
        pushfd
        pop     eax
        xor     eax, 1 SHL 21     ; check if CPUID bit can toggle
        push    eax
        popfd
        pushfd
        pop     ebx
        xor     ebx, eax
        xor     eax, eax          ; 0
        test    ebx, 1 SHL 21
        jnz     ISEND             ; CPUID not supported
        cpuid                     ; get number of CPUID functions
        test    eax, eax
        jz      ISEND             ; function 1 not supported
        mov     eax, 1
        cpuid                     ; get features
        xor     eax, eax          ; 0
        test    edx, 1 SHL 23     ; MMX support        
        jz      ISEND
        inc     eax               ; 1
        test    edx, 1 SHL 15     ; conditional move support
        jz      ISEND
        inc     eax               ; 2
        test    edx, 1 SHL 24     ; FXSAVE support by microprocessor
        jz      ISEND
        test    edx, 1 SHL 25     ; SSE support by microprocessor
        jz      ISEND
        inc     eax               ; 3
        test    edx, 1 SHL 26     ; SSE2 support by microprocessor
        jz      OSXMM
        inc     eax               ; 4
        test    ecx, 1            ; SSE3 support by microprocessor
        jz      OSXMM
        inc     eax               ; 5
OSXMM:  ; test if operating system supports XMM registers:
        mov     ebx, esp          ; save stack pointer
        sub     esp, 200H         ; allocate space for FXSAVE
        and     esp, -10H         ; align by 16
TESTDATA = 0D95A34BEH             ; random test value
TESTPS   = 10CH                   ; position to write TESTDATA = upper part of XMM6 image
        fxsave  [esp]             ; save FP/MMX and XMM registers
        mov     ecx,[esp+TESTPS]  ; read part of XMM6 register
        xor     DWORD PTR [esp+TESTPS],TESTDATA  ; change value
        fxrstor [esp]             ; load changed value into XMM6
        mov     [esp+TESTPS],ecx  ; restore old value in buffer
        fxsave  [esp]             ; save again
        mov     edx,[esp+TESTPS]  ; read changed XMM6 register
        mov     [esp+TESTPS],ecx  ; restore old value
        fxrstor [esp]             ; load old value into XMM6
        xor     ecx, edx          ; get difference between old and new value
        mov     esp, ebx          ; restore stack pointer
        cmp     ecx, TESTDATA     ; test if XMM6 was changed correctly
        je      ISEND             ; test successful
        mov     al, 2             ; test failed. XMM not supported by operating system
ISEND:  pop     ebx
        ret                       ; return value is in eax

InstructionSet ENDP

END
