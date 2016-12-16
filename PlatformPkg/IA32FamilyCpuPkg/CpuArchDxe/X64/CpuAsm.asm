;
; This file contains an 'Intel Peripheral Driver' and is      
; licensed for Intel CPUs and chipsets under the terms of your
; license agreement with Intel or your vendor.  This file may 
; be modified by the user, subject to additional terms of the 
; license agreement                                           
;
  page    ,132
  title   CPU ARCHITECTURAL DXE PROTOCOL ASSEMBLY HOOKS
;-------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
; This software and associated documentation (if any) is furnished
; under a license and may only be used or copied in accordance
; with the terms of the license. Except as permitted by such
; license, no part of this software or documentation may be
; reproduced, stored in a retrieval system, or transmitted in any
; form or by any means without the express written consent of
; Intel Corporation.

;
;
; Module Name:
;
;   CpuAsm.asm
; 
; Abstract:
; 
;
;-------------------------------------------------------------------------------


text  SEGMENT

EXTRN mErrorCodeFlag:DWORD ; Error code flags for exceptions

ExternalVectorTablePtr QWORD 0 ; point to the external interrupt vector table

InitializeExternalVectorTablePtr PROC 
	mov	ExternalVectorTablePtr, rcx
	ret
InitializeExternalVectorTablePtr ENDP	
;
;
;
;------------------------------------------------------------------------------
;  Generic IDT Vector Handlers for the Host. They are all the same so they
;  will compress really well.
;
;  By knowing the return address for Vector 00 you can can calculate the
;  vector number by looking at the call CommonInterruptEntry return address.
;  (return address - AsmIdtVector00Base)/8 == IDT index
;
;------------------------------------------------------------------------------

ALIGN   8

PUBLIC	AsmIdtVector00
        
AsmIdtVector00 LABEL BYTE
REPEAT  256
        call  CommonInterruptEntry
        dw ( $ - AsmIdtVector00 - 5 ) / 8 ; vector number
        nop
ENDM

;---------------------------------------;
; CommonInterruptEntry                  ;
;---------------------------------------;
; The follow algorithm is used for the common interrupt routine.

;
; +---------------------+ <-- 16-byte aligned ensured by processor
; +    Old SS           +
; +---------------------+
; +    Old RSP          +
; +---------------------+
; +    RFlags           +
; +---------------------+
; +    CS               +
; +---------------------+
; +    RIP              +
; +---------------------+
; +    Error Code       +
; +---------------------+
; + RCX / Vector Number +
; +---------------------+
; +    RBP              +
; +---------------------+ <-- RBP, 16-byte aligned
;

CommonInterruptEntry PROC 
  cli
  ;
  ; All interrupt handlers are invoked through interrupt gates, so
  ; IF flag automatically cleared at the entry point
  ;
  ;
  ; Calculate vector number
  ;
  xchg    rcx, [rsp] ; get the return address of call, actually, it is the address of vector number.
  movzx   ecx, word ptr [rcx]        
  cmp     ecx, 32         ; Intel reserved vector for exceptions?
  jae     NoErrorCode
  bt      mErrorCodeFlag, ecx
  jc      @F

NoErrorCode:
  ;
  ; Push a dummy error code on the stack
  ; to maintain coherent stack map
  ;
  push    [rsp]
  mov     qword ptr [rsp + 8], 0
@@:       
  push    rbp
  mov     rbp, rsp

  ;
  ; Since here the stack pointer is 16-byte aligned, so
  ; EFI_FX_SAVE_STATE_X64 of EFI_SYSTEM_CONTEXT_x64
  ; is 16-byte aligned
  ;       

;; UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
;; UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
  push r15
  push r14
  push r13
  push r12
  push r11
  push r10
  push r9
  push r8
  push rax
  push qword ptr [rbp + 8]   ; RCX
  push rdx
  push rbx
  push qword ptr [rbp + 48]  ; RSP
  push qword ptr [rbp]       ; RBP
  push rsi
  push rdi

;; UINT64  Gs, Fs, Es, Ds, Cs, Ss;  insure high 16 bits of each is zero
  movzx   rax, word ptr [rbp + 56]
  push    rax                      ; for ss
  movzx   rax, word ptr [rbp + 32]
  push    rax                      ; for cs
  mov     rax, ds
  push    rax
  mov     rax, es
  push    rax
  mov     rax, fs
  push    rax
  mov     rax, gs
  push    rax

  mov     [rbp + 8], rcx               ; save vector number

;; UINT64  Rip;
  push    qword ptr [rbp + 24]

;; UINT64  Gdtr[2], Idtr[2];
  sub     rsp, 16
  sidt    fword ptr [rsp]
  sub     rsp, 16
  sgdt    fword ptr [rsp]

;; UINT64  Ldtr, Tr;
  xor     rax, rax
  str     ax
  push    rax
  sldt    ax
  push    rax

;; UINT64  RFlags;
  push    qword ptr [rbp + 40]

;; UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
  mov     rax, cr8
  push    rax
  mov     rax, cr4
  or      rax, 208h
  mov     cr4, rax
  push    rax
  mov     rax, cr3
  push    rax
  mov     rax, cr2
  push    rax
  xor     rax, rax
  push    rax
  mov     rax, cr0
  push    rax

;; UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
  mov     rax, dr7
  push    rax
  mov     rax, dr6
  push    rax
  mov     rax, dr3
  push    rax
  mov     rax, dr2
  push    rax
  mov     rax, dr1
  push    rax
  mov     rax, dr0
  push    rax

;; FX_SAVE_STATE_X64 FxSaveState;

  sub rsp, 512
  mov rdi, rsp
  db 0fh, 0aeh, 00000111y ;fxsave [rdi]

;; UEFI calling convention for x64 requires that Direction flag in EFLAGs is clear
  cld

;; UINT32  ExceptionData;
  push    qword ptr [rbp + 16]

;; call into exception handler
  mov     rcx, [rbp + 8]
  mov     rax, ExternalVectorTablePtr  ; get the interrupt vectors base
  mov     rax, [rax + rcx * 8]       
  or      rax, rax                        ; NULL?

  je    nonNullValue;

;; Prepare parameter and call
;  mov     rcx, [rbp + 8]
  mov     rdx, rsp
  ;
  ; Per X64 calling convention, allocate maximum parameter stack space
  ; and make sure RSP is 16-byte aligned
  ;
  sub     rsp, 4 * 8 + 8
  call    rax
  add     rsp, 4 * 8 + 8

nonNullValue:
  cli
;; UINT64  ExceptionData;
  add     rsp, 8

;; FX_SAVE_STATE_X64 FxSaveState;

  mov rsi, rsp
  db 0fh, 0aeh, 00001110y ; fxrstor [rsi]
  add rsp, 512

;; UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
;; Skip restoration of DRx registers to support in-circuit emualators
;; or debuggers set breakpoint in interrupt/exception context
  add     rsp, 8 * 6

;; UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
  pop     rax
  mov     cr0, rax
  add     rsp, 8   ; not for Cr1
  pop     rax
  mov     cr2, rax
  pop     rax
  mov     cr3, rax
  pop     rax
  mov     cr4, rax
  pop     rax
  mov     cr8, rax

;; UINT64  RFlags;
  pop     qword ptr [rbp + 40]

;; UINT64  Ldtr, Tr;
;; UINT64  Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
  add     rsp, 48

;; UINT64  Rip;
  pop     qword ptr [rbp + 24]

;; UINT64  Gs, Fs, Es, Ds, Cs, Ss;
  pop     rax
  ; mov     gs, rax ; not for gs
  pop     rax
  ; mov     fs, rax ; not for fs
  ; (X64 will not use fs and gs, so we do not restore it)
  pop     rax
  mov     es, rax
  pop     rax
  mov     ds, rax
  pop     qword ptr [rbp + 32]  ; for cs
  pop     qword ptr [rbp + 56]  ; for ss

;; UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
;; UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
  pop     rdi
  pop     rsi
  add     rsp, 8               ; not for rbp
  pop     qword ptr [rbp + 48] ; for rsp
  pop     rbx
  pop     rdx
  pop     rcx
  pop     rax
  pop     r8
  pop     r9
  pop     r10
  pop     r11
  pop     r12
  pop     r13
  pop     r14
  pop     r15

  mov     rsp, rbp
  pop     rbp
  add     rsp, 16
  iretq

CommonInterruptEntry ENDP


CODE_SELECTOR EQU 38h
DATA_SELECTOR EQU 30h

InitializeGdt  proc 

  mov     rdx, OFFSET Gdtr    ; Load GDT register

  mov     ax,cs               ; Get the selector data from our code image          
  mov     ds,ax
  lgdt    FWORD PTR [rdx]     ; and update the GDTR

SelectorRld::
  mov     ax, DATA_SELECTOR ; Update the Base for the new selectors, too
  mov     ds, ax
  mov     es, ax
  mov     fs, ax
  mov     gs, ax
  mov     ss, ax  

  ret
InitializeGdt  endp

;-----------------------------------------------------------------------------;
; data
;-----------------------------------------------------------------------------;

        align 16

gdtr    dw GDT_END - GDT_BASE - 1   ; GDT limit
        dq OFFSET GDT_BASE          ; (GDT base gets set above)

;-----------------------------------------------------------------------------;
;   global descriptor table (GDT)
;-----------------------------------------------------------------------------;

        align 16

public GDT_BASE
GDT_BASE:
; null descriptor
NULL_SEL            equ $-GDT_BASE    ; Selector [0]
    dw  0  ; limit 15:0
    dw  0  ; base  15:0
    db  0  ; base  23:16
    db  0  ;
    db  0  ; type & limit 19:16
    db  0  ; base  31:24

; linear data segment descriptor
LINEAR_SEL      equ $-GDT_BASE        ; Selector [0x8]
    dw  0ffffh  ; limit 15:0
    dw  0       ; base  15:0
    db  0       ; base  23:16
    db  92h     ;present, ring 0, data, expand-up writable
    db  0cfh    ; type & limit 19:16 
    db  0       ; base  31:24

; linear code segment descriptor
LINEAR_CODE_SEL equ $-GDT_BASE        ; Selector [0x10]
    dw  0ffffh      ; limit 15:0
    dw  0          ; base  15:0
    db  0          ; base  23:16
    db  9ah        ; present, ring 0, code, expand-up writable
    db  0cfh       ; type & limit 19:16   
    db  0          ; base  31:24

; system data segment descriptor
SYS_DATA_SEL    equ $-GDT_BASE        ; Selector [0x18]
    dw  0ffffh     ; limit 15:0
    dw  0          ; base  15:0
    db  0          ; base  23:16
    db  92h       ; type & limit 19:16
    db  0cfh
    db  0          ; base  31:24

; system code segment descriptor
SYS_CODE_SEL    equ $-GDT_BASE        ; Selector [0x20]
    dw  0ffffh     ; limit 15:0
    dw  0      ; base  15:0
    db  0      ; base  23:16
    db  9ah ; type & limit 19:16
    db  0cfh
    db  0      ; base  31:24

; spare segment descriptor             ; Selector [0x28]
SPARE3_SEL  equ $-GDT_BASE
    dw  0  ; limit 15:0
    dw  0  ; base  15:0
    db  0  ; base  23:16
    db  0  ; type & limit 19:16
    db  0  ; base  31:24
    db  0

; 64bit data segment descriptor
LONG_MODE_DATA_SEL    equ $-GDT_BASE        ; Selector [0x30]
    dw  0ffffh  ; limit 15:0
    dw  0      ; base  15:0
    db  0      ; base  23:16
    db  92h    ; type & limit 19:16
    db  0cfh
    db  0      ; base  31:24

; 64bit code segment descriptor
LONG_MODE_CODE    equ $-GDT_BASE        ; Selector [0x38]
   dw  0ffffh  ; limit 15:0
   dw  0      ; base  15:0
   db  0      ; base  23:16
   db  9ah ; type & limit 19:16
   db  0afh
   db  0      ; base  31:24

; spare segment descriptor
SPARE5_SEL  equ $-GDT_BASE               ; Selector [0x40]
    dw  0  ; limit 15:0
    dw  0  ; base  15:0
    db  0  ; base  23:16
    db  0  ; type & limit 19:16
    db  0  ; base  31:24
    db  0

GDT_END:

text  ENDS
END

