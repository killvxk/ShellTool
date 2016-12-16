;
; This file contains an 'Intel Peripheral Driver' and is      
; licensed for Intel CPUs and chipsets under the terms of your
; license agreement with Intel or your vendor.  This file may 
; be modified by the user, subject to additional terms of the 
; license agreement                                           
;
;------------------------------------------------------------------------------
;
; Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
; This software and associated documentation (if any) is furnished
; under a license and may only be used or copied in accordance
; with the terms of the license. Except as permitted by such
; license, no part of this software or documentation may be
; reproduced, stored in a retrieval system, or transmitted in any
; form or by any means without the express written consent of
; Intel Corporation.
;
; Module Name:
;
;   SmiException.asm
;
; Abstract:
;
;   Exception handlers used in SM mode
;
;------------------------------------------------------------------------------

EXTERNDEF   gSmiMtrrs:QWORD
EXTERNDEF   gcSmiIdtr:FWORD
EXTERNDEF   gcSmiGdtr:FWORD
EXTERNDEF   gcPsd:BYTE
EXTERNDEF   gPhyMask:QWORD

CpuDeadLoop     PROTO
SmiPFHandler    PROTO

    .const

NullSeg     DQ      0                   ; reserved by architecture
            DQ      0                   ; reserved for future use
CodeSeg32   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      9bh
            DB      0cfh                ; LimitHigh
            DB      0                   ; BaseHigh
DataSeg32   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      93h
            DB      0cfh                ; LimitHigh
            DB      0                   ; BaseHigh
            DQ      0                   ; reserved for future use
CodeSeg16   LABEL   QWORD
            DW      -1
            DW      0
            DB      0
            DB      9bh
            DB      8fh
            DB      0
DataSeg16   LABEL   QWORD
            DW      -1
            DW      0
            DB      0
            DB      93h
            DB      8fh
            DB      0
CodeSeg64   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      9bh
            DB      0afh                ; LimitHigh
            DB      0                   ; BaseHigh
; TSS Segment for X64 specially
TssSeg      LABEL   QWORD
            DW      TSS_DESC_SIZE       ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      89h
            DB      080h                ; LimitHigh
            DB      0                   ; BaseHigh
            DD      0                   ; BaseUpper
            DD      0                   ; Reserved
GDT_SIZE = $ - offset NullSeg

; Create TSS Descriptor just after GDT
TssDescriptor LABEL BYTE
            DW      0                   ; PreviousTaskLink
            DW      0                   ; Reserved
            DD      0                   ; ESP0
            DW      0                   ; SS0
            DW      0                   ; Reserved
            DD      0                   ; ESP1
            DW      0                   ; SS1
            DW      0                   ; Reserved
            DD      0                   ; ESP2
            DW      0                   ; SS2
            DW      0                   ; Reserved
            DD      0                   ; CR3
            DD      0                   ; EIP
            DD      0                   ; EFLAGS
            DD      0                   ; EAX
            DD      0                   ; ECX
            DD      0                   ; EDX
            DD      0                   ; EBX
            DD      0                   ; ESP
            DD      0                   ; EBP
            DD      0                   ; ESI
            DD      0                   ; EDI
            DW      0                   ; ES
            DW      0                   ; Reserved
            DW      0                   ; CS
            DW      0                   ; Reserved
            DW      0                   ; SS
            DW      0                   ; Reserved
            DW      0                   ; DS
            DW      0                   ; Reserved
            DW      0                   ; FS
            DW      0                   ; Reserved
            DW      0                   ; GS
            DW      0                   ; Reserved
            DW      0                   ; LDT Selector
            DW      0                   ; Reserved
            DW      0                   ; T
            DW      0                   ; I/O Map Base
TSS_DESC_SIZE = $ - offset TssDescriptor

;
; This structure serves as a template for all processors.
;
gcPsd     LABEL   BYTE
            DB      'PSDSIG  '
            DW      PSD_SIZE
            DW      2
            DW      1 SHL 2
            DW      CODE_SEL
            DW      DATA_SEL
            DW      DATA_SEL
            DW      DATA_SEL
            DW      0
            DQ      0
            DQ      0
            DQ      0                   ; fixed in InitializeMpServiceData()
            DQ      offset NullSeg
            DD      GDT_SIZE
            DD      0
            DB      24 dup (0)
            DQ      offset gSmiMtrrs
PSD_SIZE  = $ - offset gcPsd

;
; CODE & DATA segments for SMM runtime
;
CODE_SEL    = offset CodeSeg64 - offset NullSeg
DATA_SEL    = offset DataSeg32 - offset NullSeg

gcSmiGdtr   LABEL   FWORD
    DW      GDT_SIZE - 1
    DQ      offset NullSeg

gcSmiIdtr   LABEL   FWORD
    DW      IDT_SIZE - 1
    DQ      offset _SmiIDT

    .data

;
; Here is the IDT. There are 32 (not 255) entries in it since only processor
; generated exceptions will be handled.
;
_SmiIDT:
REPEAT      32
    DW      0                           ; Offset 0:15
    DW      CODE_SEL                    ; Segment selector
    DB      0                           ; Unused
    DB      8eh                         ; Interrupt Gate, Present
    DW      0                           ; Offset 16:31
    DQ      0                           ; Offset 32:63
            ENDM
_SmiIDTEnd:

IDT_SIZE = (offset _SmiIDTEnd - offset _SmiIDT)

;
; CpuDeadLoop() is the default exception handler since it preserves the processor
; branch log.
;
gSmiExceptionHandlers   LABEL   QWORD
    DQ      14  dup (CpuDeadLoop)
    DQ      SmiPFHandler                ; Page Fault handler
    DQ      17  dup (CpuDeadLoop)

;
; Here are the global variables used by #PF exception handler.
;
_PFPML4     DD      0
_PFPDP      DD      0
_PFLOCK     DB      0

    .code

;------------------------------------------------------------------------------
; _SmiExceptionEntryPoints is the collection of exception entrypoints followed
; by a common exception handler.
;
; Stack frame would be as follows as specified in IA32 manuals:
;   SS      +68h
;   RSP     +60h
;   RFLAGS  +58h
;   CS      +50h
;   RIP     +48h
;   ErrCode +40h
;   INT#    +38h
;   RAX     +30h
;   RCX     +28h
;   RDX     +20h
;   R8      +18h
;   R9      +10h
;   R10     +8
;   R11     +0
;
; RSP set to odd multiple of 8 at @CommonEntryPoint means ErrCode PRESENT
;------------------------------------------------------------------------------
_SmiExceptionEntryPoints    PROC
IHDLRIDX    = 0
REPEAT      31                          ; handler for INT0~INT30
    push    IHDLRIDX
    jmp     @CommonEntryPoint
IHDLRIDX    = IHDLRIDX + 1
            ENDM
    push    IHDLRIDX                    ; handler for INT31
@CommonEntryPoint:
    test    spl, 8                      ; odd multiple of 8 => ErrCode present
    jnz     @F
    push    [rsp]                       ; duplicate INT# if no ErrCode
@@:
    push    rax                         ; save all volatile registers
    push    rcx
    push    rdx
    push    r8
    push    r9
    push    r10
    push    r11

; UEFI calling convention for x64 requires that Direction flag in EFLAGs is clear
    cld

    mov     rcx, [rsp + 38h]            ; INT#
    mov     rdx, [rsp + 40h]            ; ErrCode (if any)
    add     rsp, -20h
    lea     rax, gSmiExceptionHandlers
    call    qword ptr [rax + rcx*8]
    add     rsp, 20h
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rdx
    pop     rcx
    pop     rax                         ; restore all volatile registers
    add     rsp, 10h                    ; skip INT# & ErrCode
    iretq
_SmiExceptionEntryPoints    ENDP

InitializeIDT   PROC
    mov     ecx, (_SmiIDTEnd - _SmiIDT) SHR 2
    lea     rdx, _SmiIDT - 16
    lea     r10, _SmiExceptionEntryPoints - 4
@@:
    lea     rax, [r10 + rcx]            ; rax <- handler address
    mov     [rdx + rcx*4], ax
    shr     rax, 16
    mov     [rdx + rcx*4 + 6], ax
    shr     rax, 16
    mov     [rdx + rcx*4 + 8], eax
    add     ecx, -4
    jnz     @B
    ret
InitializeIDT   ENDP

    END
