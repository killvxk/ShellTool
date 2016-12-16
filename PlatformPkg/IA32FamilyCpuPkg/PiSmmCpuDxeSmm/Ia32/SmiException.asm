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

    .686p
    .model  flat,C

CpuSleep    PROTO   C

EXTERNDEF   gSmiMtrrs:QWORD
EXTERNDEF   gcSmiIdtr:FWORD
EXTERNDEF   gcSmiGdtr:FWORD
EXTERNDEF   gcPsd:BYTE

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
GDT_SIZE = $ - offset NullSeg

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
            DQ      0
            DQ      offset NullSeg
            DD      GDT_SIZE
            DD      0
            DB      24 dup (0)
            DQ      offset gSmiMtrrs
PSD_SIZE  = $ - offset gcPsd

CODE_SEL    = offset CodeSeg32 - offset NullSeg
DATA_SEL    = offset DataSeg32 - offset NullSeg

gcSmiGdtr   LABEL   FWORD
    DW      GDT_SIZE - 1
    DD      offset NullSeg

gcSmiIdtr   LABEL   FWORD
    DW      IDT_SIZE - 1
    DD      offset _SmiIDT

    .data

_SmiIDT     LABEL   QWORD
REPEAT      32
    DW      0                           ; Offset 0:15
    DW      CODE_SEL                    ; Segment selector
    DB      0                           ; Unused
    DB      8eh                         ; Interrupt Gate, Present
    DW      0                           ; Offset 16:31
            ENDM
IDT_SIZE = $ - offset _SmiIDT

gSmiExceptionHandlers   LABEL   QWORD
    DD      32 dup (CpuSleep)

    .code

;------------------------------------------------------------------------------
; Exception handlers
;------------------------------------------------------------------------------
_SmiExceptionHandlers   PROC
IHDLRIDX = 0
REPEAT      8                           ; INT0 ~ INT7
    push    eax                         ; dummy error code
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
IHDLRIDX = IHDLRIDX + 1
            ENDM
REPEAT      1                           ; INT8
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
    int     3
IHDLRIDX = IHDLRIDX + 1
            ENDM
REPEAT      1                           ; INT9
    push    eax                         ; dummy error code
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
IHDLRIDX = IHDLRIDX + 1
            ENDM
REPEAT      5                           ; INT10 ~ INT14
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
    int     3
IHDLRIDX = IHDLRIDX + 1
            ENDM
REPEAT      2                           ; INT15 ~ INT16
    push    eax                         ; dummy error code
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
IHDLRIDX = IHDLRIDX + 1
            ENDM
REPEAT      1                           ; INT17
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
    int     3
IHDLRIDX = IHDLRIDX + 1
            ENDM
REPEAT      14                          ; INT18 ~ INT31
    push    eax                         ; dummy error code
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
IHDLRIDX = IHDLRIDX + 1
            ENDM
_SmiExceptionHandlers   ENDP

;------------------------------------------------------------------------------
; _SmiExceptionEntryPoint is the entry point for all exceptions
;
; Stack frame would be as follows as specified in IA32 manuals:
;   RFLAGS  +1ch
;   CS      +18h
;   RIP     +14h
;   ErrCode +10h
;   INT#    +0ch
;   EAX     +8
;   ECX     +4
;   EDX     +0
;
; RSP set to odd multiple of 8 means ErrCode PRESENT
;------------------------------------------------------------------------------
_SmiExceptionEntryPoint PROC
    push    eax
    push    ecx
    push    edx

; UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

    lea     eax, gSmiExceptionHandlers
    mov     ecx, [esp + 0ch]
    push    [esp + 10h]
    push    ecx
    call    dword ptr [eax + ecx*4]
    pop     ecx
    pop     ecx
    pop     edx
    pop     ecx
    pop     eax
    add     esp, 8                      ; skip INT# & ErrCode
    iretd
_SmiExceptionEntryPoint ENDP

InitializeIDT   PROC    USES    ebx
    lea     edx, _SmiIDT - 8
;    push    IDT_SIZE / 8
    DB      68h                         ; push /id
    DD      IDT_SIZE / 8
    lea     ebx, _SmiExceptionHandlers - 8
    pop     ecx
@@:
    lea     eax, [ebx + ecx*8]
    mov     [edx + ecx*8], ax
    shr     eax, 16
    mov     [edx + ecx*8 + 6], ax
    loop    @B
    ret
InitializeIDT   ENDP

    END
