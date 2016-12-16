;
; This file contains an 'Intel Peripheral Driver' and is      
; licensed for Intel CPUs and chipsets under the terms of your
; license agreement with Intel or your vendor.  This file may 
; be modified by the user, subject to additional terms of the 
; license agreement                                           
;
;------------------------------------------------------------------------------
;
; Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
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
;   SmmInit.Asm
;
; Abstract:
;
;   Functions for relocating SMBASE's for all processors
;
; Notes:
;
;------------------------------------------------------------------------------

SSM_SMBAS   EQU     0fef8h
SSM_IEDBAS  EQU     0ff04h

EXTERNDEF   SmmInitHandler:PROC
EXTERNDEF   gSmmCr0:DWORD
EXTERNDEF   gSmmCr3:DWORD
EXTERNDEF   gSmmCr4:DWORD
EXTERNDEF   gSmmJmpAddr:QWORD
EXTERNDEF   gcSmmInitTemplate:BYTE
EXTERNDEF   gcSmmInitSize:WORD
EXTERNDEF   mRebasedFlag:PTR BYTE
EXTERNDEF   mSmmRelocationOriginalAddress:QWORD
EXTERNDEF   mRebasedFlagAddr32:DWORD
EXTERNDEF   mSmmRelocationOriginalAddressPtr32:DWORD
EXTERNDEF   gSmmInitStack:QWORD

    .data

NullSeg     DQ      0                   ; reserved by architecture
CodeSeg64   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      9bh
            DB      0afh                ; LimitHigh
            DB      0                   ; BaseHigh
GDT_SIZE = $ - offset NullSeg

    .code

GdtDesc     LABEL   FWORD
            DW      GDT_SIZE
            DQ      offset NullSeg

SmmStartup  PROC
    DB      66h, 0b8h                   ; mov eax, imm32
gSmmCr3     DD      ?
    mov     cr3, rax
    DB      66h, 2eh
    lgdt    fword ptr [ebp + (offset GdtDesc - SmmStartup)]
    DB      66h, 0b8h                   ; mov eax, imm32
gSmmCr4     DD      ?
    mov     cr4, rax
    DB      66h
    mov     ecx, 0c0000080h             ; IA32_EFER MSR
    rdmsr
    or      ah, 1                       ; set LME bit
    wrmsr
    DB      66h, 0b8h                   ; mov eax, imm32
gSmmCr0     DD      ?
    mov     cr0, rax                    ; enable protected mode & paging
    DB      66h, 0eah                   ; far jmp to long mode
gSmmJmpAddr DQ      @LongMode
@LongMode:                              ; long-mode starts here
    DB      48h, 0bch                   ; mov rsp, imm64
gSmmInitStack   DQ      ?
    and     sp, 0fff0h                  ; make sure RSP is 16-byte aligned
    ;
    ; Accoring to X64 calling convention, XMM0~5 are volatile, we need to save
    ; them before calling C-function.
    ;
    sub     rsp, 60h
    movdqa  [rsp], xmm0
    movdqa  [rsp + 10h], xmm1
    movdqa  [rsp + 20h], xmm2
    movdqa  [rsp + 30h], xmm3
    movdqa  [rsp + 40h], xmm4
    movdqa  [rsp + 50h], xmm5

    add     rsp, -20h   
    call    SmmInitHandler
    add     rsp, 20h

    ;
    ; Restore XMM0~5 after calling C-function.
    ;
    movdqa  xmm0, [rsp]
    movdqa  xmm1, [rsp + 10h]
    movdqa  xmm2, [rsp + 20h]
    movdqa  xmm3, [rsp + 30h]
    movdqa  xmm4, [rsp + 40h]
    movdqa  xmm5, [rsp + 50h]    

    rsm
SmmStartup  ENDP

gcSmmInitTemplate   LABEL   BYTE

_SmmInitTemplate    PROC
    DB      66h, 2eh, 8bh, 2eh          ; mov ebp, cs:[@F]
    DW      @L1 - _SmmInitTemplate + 8000h
    DB      66h, 2eh, 2bh, 2eh          ; sub ebp, cs:[SSM_SMBAS]
    DW      SSM_SMBAS
    jmp     bp                          ; jmp ebp actually
@L1:
    DQ      SmmStartup
_SmmInitTemplate    ENDP

gcSmmInitSize   DW  $ - gcSmmInitTemplate

SmmRelocationSemaphoreComplete PROC
    push    rax
    mov     rax, mRebasedFlag
    mov     byte ptr [rax], 1
    pop     rax
    jmp     [mSmmRelocationOriginalAddress]
SmmRelocationSemaphoreComplete ENDP

;
; Semaphore code running in 32-bit mode
;
SmmRelocationSemaphoreComplete32 PROC
    ;
    ; mov byte ptr [], 1
    ;
    db      0c6h, 05h
mRebasedFlagAddr32    dd      0
    db      1
    ;
    ; jmp dword ptr []
    ;
    db      0ffh, 25h
mSmmRelocationOriginalAddressPtr32    dd      0
SmmRelocationSemaphoreComplete32 ENDP

    END
