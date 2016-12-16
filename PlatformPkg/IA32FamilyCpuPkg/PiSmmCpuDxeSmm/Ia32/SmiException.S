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
.equ  GDT_SIZE, .- NullSeg

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
            .long 0
            .long 0
            .long 0
            .long 0
            .quad 0
            .long      NullSeg
            .long 0
            .long      GDT_SIZE
            .long 0
            .space 24, 0
            .long      ASM_PFX(gSmiMtrrs)
            .long 0
.equ  PSD_SIZE,  . - ASM_PFX(gcPsd)

.equ  CODE_SEL,    CodeSeg32 - NullSeg
.equ  DATA_SEL,    DataSeg32 - NullSeg

ASM_PFX(gcSmiGdtr):  .word      GDT_SIZE - 1
                     .long      NullSeg

ASM_PFX(gcSmiIdtr):  .word      IDT_SIZE - 1
                     .long      _SmiIDT

_SmiIDT:
# The following segment repeats 32 times:
# No. 1
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 2
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 3
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 4
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 5
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 6
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 7
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 8
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 9
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 10
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 11
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 12
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 13
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 14
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 15
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 16
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 17
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 18
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 19
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 20
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 21
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 22
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 23
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 24
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 25
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 26
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 27
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 28
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 29
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 30
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 31
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 32
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31

.equ  IDT_SIZE, . - _SmiIDT

gSmiExceptionHandlers: 
# 32 Entries
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)
    .long  ASM_PFX(CpuSleep)

    .text

#------------------------------------------------------------------------------
# Exception handlers
#------------------------------------------------------------------------------
_SmiExceptionHandlers:
.equ  IHDLRIDX, 0
# The following segment repeats 8 times:
# No. 1
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 2
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 3
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 4
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 5
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 6
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 7
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 8
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1

    pushl    $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
    int     $3
.equ  IHDLRIDX, IHDLRIDX + 1

    pushl   %eax                        # dummy error code
    pushl    $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1

# The following segment repeats 5 times:
# No. 1
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
    int     $3
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 2
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
    int     $3
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 3
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
    int     $3
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 4
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
    int     $3
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 5
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
    int     $3
.equ  IHDLRIDX, IHDLRIDX + 1

# The following segment repeats 2 times:
# No. 1
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 2
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1

    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
    int     $3
.equ  IHDLRIDX, IHDLRIDX + 1

# The following segment repeats 14 times:
# No. 1
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 2
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 3
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 4
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 5
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 6
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 7
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 8
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 9
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 10
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 11
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 12
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 13
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1
# No. 14
    pushl   %eax                        # dummy error code
    pushl   $IHDLRIDX
    .byte 0xe9                          # jmp disp32
    .long      _SmiExceptionEntryPoint - (. + 4)
.equ  IHDLRIDX, IHDLRIDX + 1


#------------------------------------------------------------------------------
# _SmiExceptionEntryPoint is the entry point for all exceptions
#
# Stack frame would be as follows as specified in IA32 manuals:
#   RFLAGS  +1ch
#   CS      +18h
#   RIP     +14h
#   ErrCode +10h
#   INT#    +0ch
#   EAX     +8
#   ECX     +4
#   EDX     +0
#
# RSP set to odd multiple of 8 means ErrCode PRESENT
#------------------------------------------------------------------------------
_SmiExceptionEntryPoint:
    pushl   %eax
    pushl   %ecx
    pushl   %edx

# UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

    lea     gSmiExceptionHandlers, %eax
    movl    0xc(%esp),%ecx
    pushl   0x10(%esp)
    pushl   %ecx
    call    *(%eax, %ecx, 4)
    popl    %ecx
    popl    %ecx
    popl    %edx
    popl    %ecx
    popl    %eax
    addl    $8,%esp                     # skip INT# & ErrCode
    iretl                               # iretl(32-bit iret)

ASM_GLOBAL ASM_PFX(InitializeIDT)
ASM_PFX(InitializeIDT):
    pushl   %ebx
    lea     _SmiIDT - 8, %edx
#    push    IDT_SIZE / 8
    .byte 0x68                          # push /id
    .long      IDT_SIZE / 8
    lea      _SmiExceptionHandlers - 8, %ebx
    popl    %ecx
L1: 
    leal    (%ebx,%ecx,8),%eax
    movw    %ax,(%edx,%ecx,8)
    shrl    $16,%eax
    movw    %ax, 6(%edx, %ecx, 8)
    loop    L1
    popl    %ebx
    ret

