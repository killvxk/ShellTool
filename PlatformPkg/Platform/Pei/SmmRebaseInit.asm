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

    .686p
    .xmm
    .model  flat,C

SSM_SMBAS   EQU     0fef8h
SSM_IEDBAS  EQU     0ff04h

SmmInitHandler  PROTO   C

EXTERNDEF   C   gSmmCr0:DWORD
EXTERNDEF   C   gSmmCr3:DWORD
EXTERNDEF   C   gSmmCr4:DWORD
EXTERNDEF   C   gcSmmInitTemplate:BYTE
EXTERNDEF   C   gcSmmInitSize:WORD
EXTERNDEF   C   gSmmJmpAddr:QWORD
EXTERNDEF   C   mRebasedFlag:PTR BYTE
EXTERNDEF   C   mSmmRelocationOriginalAddress:DWORD
EXTERNDEF   C   gSmmInitStack:DWORD

    .data

NullSeg     DQ      0                   ; reserved by architecture
DataSeg32   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      93h
            DB      0cfh                ; LimitHigh
            DB      0                   ; BaseHigh
CodeSeg32   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      9bh
            DB      0cfh                ; LimitHigh
            DB      0                   ; BaseHigh
GDT_SIZE = $ - offset NullSeg

    .code

GdtDesc     LABEL   FWORD
            DW      GDT_SIZE
            DD      offset NullSeg

SmmStartup  PROC
    DB      66h, 0b8h
gSmmCr3     DD      ?
    mov     cr3, eax
    DB      67h, 66h
    lgdt    fword ptr cs:[ebp + (offset GdtDesc - SmmStartup)]
    DB      66h, 0b8h
gSmmCr4     DD      ?
    mov     cr4, eax
    DB      66h, 0b8h
gSmmCr0     DD      ?
    DB      0bfh, 8, 0                  ; mov di, 8
    mov     cr0, eax
    DB      66h, 0eah                   ; jmp far [ptr48]
gSmmJmpAddr LABEL   QWORD
    DD      @32bit
    DW      10h
@32bit:
    mov     ds, edi
    mov     es, edi
    mov     fs, edi
    mov     gs, edi
    mov     ss, edi
    DB      0bch                        ; mov esp, imm32
gSmmInitStack  DD ?
    call    SmmInitHandler
    rsm
SmmStartup  ENDP

gcSmmInitTemplate   LABEL   BYTE

_SmmInitTemplate    PROC
    DB      66h
    mov     ebp, SmmStartup
    DB      66h, 2eh, 2bh, 2eh          ; sub ebp, cs:[SSM_SMBAS]
    DW      SSM_SMBAS
    jmp     bp                          ; jmp ebp actually
_SmmInitTemplate    ENDP

gcSmmInitSize   DW  $ - gcSmmInitTemplate

SmmRelocationSemaphoreComplete PROC
    push    eax
    mov     eax, mRebasedFlag
    mov     byte ptr [eax], 1
    pop     eax
    jmp     [mSmmRelocationOriginalAddress]
SmmRelocationSemaphoreComplete ENDP
    END
