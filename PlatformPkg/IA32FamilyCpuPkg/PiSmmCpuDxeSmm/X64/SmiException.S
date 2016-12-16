//
// This file contains an 'Intel Peripheral Driver' and is
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may
// be modified by the user, subject to additional terms of the
// license agreement
//
#------------------------------------------------------------------------------
#
# Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
# This software and associated documentation (if any) is furnished
# under a license and may only be used or copied in accordance
# with the terms of the license. Except as permitted by such
# license, no part of this software or documentation may be
# reproduced, stored in a retrieval system, or transmitted in any
# form or by any means without the express written consent of
# Intel Corporation.
#
# Module Name:
#
#   SmiException.S
#
# Abstract:
#
#   Exception handlers used in SM mode
#
#------------------------------------------------------------------------------

ASM_GLOBAL  ASM_PFX(gSmiMtrrs)
ASM_GLOBAL  ASM_PFX(gcSmiIdtr)
ASM_GLOBAL  ASM_PFX(gcSmiGdtr)
ASM_GLOBAL  ASM_PFX(gcPsd)

    .data

NullSeg:    .quad 0
            .quad 0                     # reserved for future use
CodeSeg32:
            .word -1                    # LimitLow
            .word 0                     # BaseLow
            .byte 0                     # BaseMid
            .byte 0x9b
            .byte 0xcf                  # LimitHigh
            .byte 0                     # BaseHigh
DataSeg32:
            .word -1                    # LimitLow
            .word 0                     # BaseLow
            .byte 0                     # BaseMid
            .byte 0x93
            .byte 0xcf                  # LimitHigh
            .byte 0                     # BaseHigh
            .quad 0                     # reserved for future use
CodeSeg16:
            .word -1
            .word 0
            .byte 0
            .byte 0x9b
            .byte 0x8f
            .byte 0
DataSeg16:
            .word -1
            .word 0
            .byte 0
            .byte 0x93
            .byte 0x8f
            .byte 0
CodeSeg64:
            .word -1                    # LimitLow
            .word 0                     # BaseLow
            .byte 0                     # BaseMid
            .byte 0x9b
            .byte 0xaf                  # LimitHigh
            .byte 0                     # BaseHigh
# TSS Segment for X64 specially
TssSeg:
            .word TSS_DESC_SIZE         # LimitLow
            .word 0                     # BaseLow
            .byte 0                     # BaseMid
            .byte 0x89
            .byte 0xDB                  # LimitHigh
            .byte 0                     # BaseHigh
            .long 0                     # BaseUpper
            .long 0                     # Reserved
.equ  GDT_SIZE, .- NullSeg

TssDescriptor:
            .space 104, 0
.equ  TSS_DESC_SIZE, .- TssDescriptor

#
# This structure serves as a template for all processors.
#
ASM_PFX(gcPsd):
            .ascii  "PSDSIG  "
            .word      PSD_SIZE
            .word 2
            .word      1 << 2
            .word      CODE_SEL
            .word      DATA_SEL
            .word      DATA_SEL
            .word      DATA_SEL
            .word 0
            .quad 0
            .quad 0
            .quad 0                     # fixed in InitializeMpServiceData()
            .quad      NullSeg
            .long      GDT_SIZE
            .long 0
            .space 24, 0
            .quad      ASM_PFX(gSmiMtrrs)
.equ  PSD_SIZE,  . - ASM_PFX(gcPsd)

#
# CODE & DATA segments for SMM runtime
#
.equ  CODE_SEL,    CodeSeg64 - NullSeg
.equ  DATA_SEL,    DataSeg32 - NullSeg

ASM_PFX(gcSmiGdtr):
    .word      GDT_SIZE - 1
    .quad      NullSeg

ASM_PFX(gcSmiIdtr):
    .word      IDT_SIZE - 1
    .quad      _SmiIDT


#
# Here is the IDT. There are 32 (not 255) entries in it since only processor
# generated exceptions will be handled.
#
_SmiIDT:
# The following segment repeats 32 times:
# No. 1
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 2
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 3
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 4
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 5
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 6
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 7
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 8
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 9
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 10
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 11
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 12
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 13
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 14
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 15
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 16
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 17
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 18
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 19
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 20
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 21
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 22
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 23
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 24
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 25
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 26
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 27
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 28
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 29
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 30
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 31
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63
# No. 32
    .word 0                             # Offset 0:15
    .word CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
    .quad 0                             # Offset 32:63

_SmiIDTEnd:

.equ  IDT_SIZE, (_SmiIDTEnd - _SmiIDT)

#
# CpuSleep() is the default exception handler since it preserves the processor
# branch log.
#
ASM_PFX(gSmiExceptionHandlers):
# 32 Entries
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(SmiPFHandler)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)
    .quad   ASM_PFX(CpuDeadLoop)


#
# Here are the global variables used by #PF exception handler.
#
_PFPML4:     .long      0
_PFPDP:      .long      0
_PFLOCK:     .byte      0

    .text

#------------------------------------------------------------------------------
# _SmiExceptionEntryPoints is the collection of exception entrypoints followed
# by a common exception handler.
#
# Stack frame would be as follows as specified in IA32 manuals:
#   SS      +68h
#   RSP     +60h
#   RFLAGS  +58h
#   CS      +50h
#   RIP     +48h
#   ErrCode +40h
#   INT#    +38h
#   RAX     +30h
#   RCX     +28h
#   RDX     +20h
#   R8      +18h
#   R9      +10h
#   R10     +8
#   R11     +0
#
# RSP set to odd multiple of 8 at @CommonEntryPoint means ErrCode PRESENT
#------------------------------------------------------------------------------
_SmiExceptionEntryPoints:
.equ  IHDLRIDX,  0
# The following segment repeats 31 times:
# No. 1
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 2
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 3
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 4
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 5
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 6
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 7
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 8
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 9
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 10
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 11
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 12
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 13
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 14
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 15
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 16
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 17
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 18
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 19
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 20
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 21
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 22
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 23
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 24
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 25
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 26
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 27
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 28
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 29
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 30
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 31
    pushq   $IHDLRIDX
    jmp     CommonEntryPoint
.equ  IHDLRIDX, IHDLRIDX + 1


    pushq   $IHDLRIDX
CommonEntryPoint:
    .byte   0x40, 0xf6, 0xc4, 0x08    #test    spl, 8
    jnz     L1
    pushq   (%rsp)
L1:
    pushq   %rax
    pushq   %rcx
    pushq   %rdx
    pushq   %r8
    pushq   %r9
    pushq   %r10
    pushq   %r11

# UEFI calling convention for x64 requires that Direction flag in EFLAGs is clear
    cld

    movq    0x38(%rsp), %rcx
    movq    0x40(%rsp), %rdx
    addq    $-0x20, %rsp
    movabsq $ASM_PFX(gSmiExceptionHandlers), %rax
    movq    (%rax), %rax
    call    *(%rax, %rcx, 8)
    addq    $0x20, %rsp
    popq    %r11
    popq    %r10
    popq    %r9
    popq    %r8
    popq    %rdx
    popq    %rcx
    popq    %rax
    addq    $0x10, %rsp
    iretq

ASM_GLOBAL ASM_PFX(InitializeIDT)
ASM_PFX(InitializeIDT):
    movl     $((_SmiIDTEnd - _SmiIDT) >> 2), %ecx
    movabsq  $_SmiIDT - 16, %rdx
    movabsq  $_SmiExceptionEntryPoints - 4, %r10
L2:
    lea      (%r10, %rcx), %rax
    movw     %ax, (%rdx, %rcx, 4)
    shr      $16, %rax
    movw     %ax, 6(%rdx, %rcx, 4)
    shr      $16, %rax
    movl     %eax, 8(%rdx, %rcx, 4)
    addl     $-4, %ecx
    jnz      L2
    ret
