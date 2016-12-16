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

;
; Constants relating to PROCESSOR_SMM_DESCRIPTOR
;
DSC_OFFSET    EQU     0fb00h
DSC_GDTPTR    EQU     30h
DSC_GDTSIZ    EQU     38h
DSC_CS        EQU     14
DSC_DS        EQU     16
DSC_SS        EQU     18
DSC_OTHERSEG  EQU     20

;
; Constants relating to CPU State Save Area
;
SSM_SMBAS       EQU     0fef8h
SSM_DR6         EQU     0ffd0h
SSM_DR7         EQU     0ffc8h

;
; Variables referrenced by C code
;
EXTERNDEF   SmiRendezvous:PROC
EXTERNDEF   gcSmiHandlerTemplate:BYTE
EXTERNDEF   gcSmiHandlerSize:WORD
EXTERNDEF   gSmiCr3:DWORD
EXTERNDEF   gcSmiHandlerOffset:WORD
EXTERNDEF   gSmiStack:QWORD
EXTERNDEF   FeaturePcdGet (PcdCpuSmmDebug):BYTE

PROTECT_MODE_CS EQU     10h
PROTECT_MODE_DS EQU     18h
TSS_SEGMENT     EQU     40h
GDT_SIZE        EQU     50h

    .const

gcSmiHandlerOffset  DW      _SmiHandler - _SmiEntryPoint + 8000h

    .code

gcSmiHandlerTemplate    LABEL   BYTE

_SmiEntryPoint  PROC
    ;
    ; The encoding of BX in 16-bit addressing mode is the same as of RDI in 64-
    ; bit addressing mode. And that coincidence has been used in the following
    ; "64-bit like" 16-bit code. Be aware that once RDI is referrenced as a
    ; base address register, it is actually BX that is referrenced.
    ;
    DB      0bbh                        ; mov bx, imm16
    DW      offset _GdtDesc - _SmiEntryPoint + 8000h  ; bx = GdtDesc offset

; fix GDT TSS table
    DB      66h, 2eh, 0a1h              ; mov eax, cs:[offset16] ; eax = GDT base
    DW      DSC_OFFSET + DSC_GDTPTR
    mov     dx, ax
    mov     bp, ax                      ; ebp = GDT base
    add     dx, GDT_SIZE                ; edx = TSS descriptor base
    mov     [eax + TSS_SEGMENT + 2],edx
    shr     dx, 16
    mov     [eax + TSS_SEGMENT + 4], dl
    mov     [eax + TSS_SEGMENT + 7], dh
; fix GDT descriptor
    DB      2eh, 0a1h                   ; mov ax, cs:[offset16]
    DW      DSC_OFFSET + DSC_GDTSIZ
    DB      48h                         ; dec ax
    DB      2eh
    mov     [rdi], eax                  ; mov cs:[bx], ax
    DB      66h, 2eh, 0a1h              ; mov eax, cs:[offset16]
    DW      DSC_OFFSET + DSC_GDTPTR
    DB      2eh
    mov     [rdi + 2], ax               ; mov cs:[bx + 2], eax
    DB      66h, 2eh
    lgdt    fword ptr [rdi]             ; lgdt fword ptr cs:[bx]
    DB      66h, 0b8h                   ; mov eax, imm32
gSmiCr3     DD      ?
    mov     cr3, rax
    DB      66h
    mov     eax, 668h                   ; as cr4.PGE is not set here, refresh cr3
    mov     cr4, rax                    ; in PreModifyMtrrs() to flush TLB.
; Patch LongMode Segment
    DB      2eh, 0a1h                   ; mov ax, cs:[offset16]
    DW      DSC_OFFSET + DSC_CS
    DB      2eh
    mov     [rdi + (sizeof (FWORD) + (@LongMode - @ProtectedMode)) - 2], eax            ; mov cs:[bx - 2], ax
; Patch ProtectedMode Segment
    DB      0b8h                        ; mov ax, imm16
    DW      PROTECT_MODE_CS             ; set AX for segment directly
    DB      2eh
    mov     [rdi - 2], eax              ; mov cs:[bx - 2], ax
; Patch LongMode entry
    DB      66h, 2eh, 8bh, 3eh          ; mov edi, cs:[offset16]
    DW      SSM_SMBAS
    lea     ax, [edi + (@LongMode - _SmiEntryPoint) + 8000h]
    DB      2eh
    mov     [rdi + (sizeof (FWORD) + (@LongMode - @ProtectedMode)) - 6], ax             ; mov cs:[bx - 6], eax
; Patch ProtectedMode entry
    lea     ax, [edi + (@ProtectedMode - _SmiEntryPoint) + 8000h]
    DB      2eh
    mov     [rdi - 6], ax               ; mov cs:[bx - 6], eax
; Switch into @ProtectedMode
    DB      66h
    mov     ecx, 0c0000080h
    DB      66h
    mov     ebx, 00000023h

    mov     cr0, rbx
    DB      66h, 0eah
    DD      ?
    DW      ?

_GdtDesc    FWORD   ?
@ProtectedMode:
    mov     ax, PROTECT_MODE_DS
    mov     ds, eax
    mov     es, eax
    mov     ss, eax

; Load TSS
    mov     eax, ebp                    ; ebp = GDT base
    mov     edx, eax
    mov     dl, 89h
    mov     [rax + TSS_SEGMENT + 5], dl ; clear busy flag

    mov     eax, TSS_SEGMENT
    ltr     ax

; Switch into @LongMode
    rdmsr
    or      ah, 1
    wrmsr
    mov     ebx, 80000023h
    mov     cr0, rbx
    DB      67h, 0eah
    DD      ?
    DW      ?

@LongMode:                              ; long mode (64-bit code) starts here
    lea     ebx, [rdi + DSC_OFFSET]
    mov     ax, [rbx + DSC_DS]
    mov     ds, eax
    mov     ax, [rbx + DSC_OTHERSEG]
    mov     es, eax
    mov     fs, eax
    mov     gs, eax
    mov     ax, [rbx + DSC_SS]
    mov     ss, eax
;   jmp     _SmiHandler                 ; instruction is not needed
_SmiEntryPoint  ENDP

_SmiHandler PROC
    DB      48h, 0bch                   ; mov rsp, imm64
gSmiStack   DQ      ?
;
; The following lines restore DR6 & DR7 before running C code. They are useful
; when you want to enable hardware breakpoints in SMM without setting
; smmentrybreak=true in ITP.
;
; NOTE: These lines might not be appreciated in runtime since they might
;       conflict with OS debugging facilities. Turn them off in RELEASE.
;
    mov     rax, offset FeaturePcdGet (PcdCpuSmmDebug) ;Get absolute address. Avoid RIP relative addressing
    cmp     byte ptr [rax], 0
    jz      @1
    DB      48h, 8bh, 0dh               ; mov rcx, [rip + disp32]
    DD      SSM_DR6 - ($ + 4 - _SmiEntryPoint + 8000h)
    DB      48h, 8bh, 15h               ; mov rdx, [rip + disp32]
    DD      SSM_DR7 - ($ + 4 - _SmiEntryPoint + 8000h)
    mov     dr6, rcx
    mov     dr7, rdx
@1:
    mov     rcx, [rsp]                  ; rcx <- CpuIndex
    mov     rax, SmiRendezvous          ; rax <- absolute addr of SmiRedezvous

    ;
    ; Save FP registers
    ;
    sub     rsp, 208h
    DB      48h                         ; FXSAVE64
    fxsave  [rsp]
    
    add     rsp, -20h
    call    rax
    add     rsp, 20h

    ;
    ; Restore FP registers
    ;
    DB      48h                         ; FXRSTOR64
    fxrstor [rsp]  
 
    mov     rax, offset FeaturePcdGet (PcdCpuSmmDebug) ;Get absolute address. Avoid RIP relative addressing
    cmp     byte ptr [rax], 0
    jz      @2
    mov     rdx, dr7
    mov     rcx, dr6
    DB      48h, 89h, 15h               ; mov [rip + disp32], rdx
    DD      SSM_DR7 - ($ + 4 - _SmiEntryPoint + 8000h)
    DB      48h, 89h, 0dh               ; mov [rip + disp32], rcx
    DD      SSM_DR6 - ($ + 4 - _SmiEntryPoint + 8000h)
@2:
    rsm
_SmiHandler ENDP

gcSmiHandlerSize    DW      $ - _SmiEntryPoint

    END
