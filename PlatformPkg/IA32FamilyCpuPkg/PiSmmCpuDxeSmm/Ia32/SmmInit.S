//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
#------------------------------------------------------------------------------
#
# Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
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
#   SmmInit.S
#
# Abstract:
#
#   Functions for relocating SMBASE's for all processors
#
# Notes:
#
#------------------------------------------------------------------------------

.equ        SSM_SMBAS, 0xfef8
.equ        SSM_IEDBAS, 0xff04


ASM_GLOBAL   ASM_PFX(gSmmCr0)
ASM_GLOBAL   ASM_PFX(gSmmCr3)
ASM_GLOBAL   ASM_PFX(gSmmCr4)
ASM_GLOBAL   ASM_PFX(gcSmmInitTemplate)
ASM_GLOBAL   ASM_PFX(gcSmmInitSize)
ASM_GLOBAL   ASM_PFX(gSmmJmpAddr)
ASM_GLOBAL   ASM_PFX(SmmRelocationSemaphoreComplete)
ASM_GLOBAL   ASM_PFX(gSmmInitStack)

    .data

NullSeg:    .quad      0
DataSeg32:
            .word -1                    # LimitLow
            .word 0                     # BaseLow
            .byte 0                     # BaseMid
            .byte 0x93
            .byte 0xcf                  # LimitHigh
            .byte 0                     # BaseHigh
CodeSeg32:
            .word -1                    # LimitLow
            .word 0                     # BaseLow
            .byte 0                     # BaseMid
            .byte 0x9b
            .byte 0xcf                  # LimitHigh
            .byte 0                     # BaseHigh
.equ  GDT_SIZE, . - NullSeg

    .text

GdtDesc:
            .word      GDT_SIZE
            .long      NullSeg

SmmStartup:
    .byte   0x66,0xb8
ASM_PFX(gSmmCr3):    .space     4
    movl    %eax, %cr3
    .byte   0x67,0x66
    lgdt    %cs:(GdtDesc - SmmStartup)(%ebp)
    .byte   0x66,0xb8
ASM_PFX(gSmmCr4):    .space     4
    movl    %eax, %cr4
    .byte   0x66,0xb8
ASM_PFX(gSmmCr0):    .space     4
    .byte   0xbf,8,0                      # mov di, 8
    movl    %eax, %cr0
    .byte   0x66,0xea                     # jmp far [ptr48]
ASM_PFX(gSmmJmpAddr): .long Start32bit
    .word   0x10
Start32bit:
    movl    %edi,%ds
    movl    %edi,%es
    movl    %edi,%fs
    movl    %edi,%gs
    movl    %edi,%ss
    .byte   0xbc                        # mov esp, imm32
ASM_PFX(gSmmInitStack):  .space  4
    call    ASM_PFX(SmmInitHandler)
    rsm

ASM_PFX(gcSmmInitTemplate):

_SmmInitTemplate:
    .byte 0x66
    movl    $SmmStartup, %ebp
    .byte 0x66,0x2e,0x2b,0x2e           # sub ebp, cs:[SSM_SMBAS]
    .word      SSM_SMBAS
    jmp     *%bp                        # jmp ebp actually

ASM_PFX(gcSmmInitSize):   .word  . - ASM_PFX(gcSmmInitTemplate)


ASM_PFX(SmmRelocationSemaphoreComplete):
    pushl   %eax
    movl    ASM_PFX(mRebasedFlag), %eax
    movb    $1, (%eax)
    popl    %eax
    jmp     *ASM_PFX(mSmmRelocationOriginalAddress)
