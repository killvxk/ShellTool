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
;   SmiEntry.asm
;
; Abstract:
;
;   Code template of the SMI handler for a particular processor
;
;------------------------------------------------------------------------------

    .686p
    .model  flat,C
    .xmm

DSC_OFFSET    EQU     0fb00h
DSC_GDTPTR    EQU     30h
DSC_GDTSIZ    EQU     38h
DSC_CS        EQU     14
DSC_DS        EQU     16
DSC_SS        EQU     18
DSC_OTHERSEG  EQU     20

SSM_SMBAS       EQU     0fef8h

SmiRendezvous   PROTO   C

EXTERNDEF   gcSmiHandlerTemplate:BYTE
EXTERNDEF   gcSmiHandlerSize:WORD
EXTERNDEF   gSmiCr3:DWORD
EXTERNDEF   gcSmiHandlerOffset:WORD
EXTERNDEF   gSmiStack:DWORD
EXTERNDEF   FeaturePcdGet (PcdCpuSmmDebug):BYTE

    .const

gcSmiHandlerOffset  DW      _SmiHandler - _SmiEntryPoint + 8000h

    .code

gcSmiHandlerTemplate    LABEL   BYTE

_SmiEntryPoint  PROC
    DB      0bbh                        ; mov bx, imm16
    DW      offset _GdtDesc - _SmiEntryPoint + 8000h
    DB      2eh, 0a1h                   ; mov ax, cs:[offset16]
    DW      DSC_OFFSET + DSC_GDTSIZ
    dec     eax
    mov     cs:[edi], eax               ; mov cs:[bx], ax
    DB      66h, 2eh, 0a1h              ; mov eax, cs:[offset16]
    DW      DSC_OFFSET + DSC_GDTPTR
    mov     cs:[edi + 2], ax            ; mov cs:[bx + 2], eax
    DB      66h
    lgdt    fword ptr cs:[edi]          ; lgdt fword ptr cs:[bx]
    DB      66h, 0b8h                   ; mov eax, imm32
gSmiCr3     DD      ?
    mov     cr3, eax
    DB      66h
    mov     eax, 668h                   ; as cr4.PGE is not set here, refresh cr3
    mov     cr4, eax                    ; in PreModifyMtrrs() to flush TLB.
    DB      2eh, 0a1h                   ; mov ax, cs:[offset16]
    DW      DSC_OFFSET + DSC_CS
    mov     cs:[edi - 2], eax           ; mov cs:[bx - 2], ax
    DB      66h, 2eh, 8bh, 3eh          ; mov edi, cs:[offset16]
    DW      SSM_SMBAS
    DB      67h
    lea     ax, [edi + (@32bit - _SmiEntryPoint) + 8000h]
    mov     cs:[edi - 6], ax            ; mov cs:[bx - 6], eax
    DB      66h
    mov     ebx, 80000023h
    mov     cr0, ebx
    DB      66h, 0eah
    DD      ?
    DW      ?
_GdtDesc    FWORD   ?
@32bit:
    lea     ebx, [edi + DSC_OFFSET]
    mov     ax, [ebx + DSC_DS]
    mov     ds, eax
    mov     ax, [ebx + DSC_OTHERSEG]
    mov     es, eax
    mov     fs, eax
    mov     gs, eax
    mov     ax, [ebx + DSC_SS]
    mov     ss, eax
;   jmp     _SmiHandler                 ; instruction is not needed
_SmiEntryPoint  ENDP

_SmiHandler PROC
    DB      0bch                        ; mov esp, imm32
gSmiStack   DD      ?
    cmp     FeaturePcdGet (PcdCpuSmmDebug), 0
    jz      @3
    call    @1
@1:
    pop     ebp
    mov     eax, 80000001h
    cpuid
    bt      edx, 29                     ; check cpuid to identify X64 or IA32 
    lea     edi, [ebp - (@1 - _SmiEntryPoint) + 7fc8h]
    lea     esi, [edi + 4]
    jnc     @2
    add     esi, 4
@2:
    mov     ecx, [esi]
    mov     edx, [edi]
    mov     dr6, ecx
    mov     dr7, edx                    ; restore DR6 & DR7 before running C code
@3:
    mov     ecx, [esp]                  ; CPU Index
    
    push    ecx
    mov     eax, SmiRendezvous
    call    eax
    pop     ecx

    cmp     FeaturePcdGet (PcdCpuSmmDebug), 0
    jz	    @4
    mov     ecx, dr6
    mov     edx, dr7
    mov     [esi], ecx
    mov     [edi], edx
@4:
    rsm
_SmiHandler ENDP

gcSmiHandlerSize    DW      $ - _SmiEntryPoint

    END
