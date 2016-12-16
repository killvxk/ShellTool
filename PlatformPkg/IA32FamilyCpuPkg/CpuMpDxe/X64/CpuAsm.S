//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
#-------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
# This software and associated documentation (if any) is furnished
# under a license and may only be used or copied in accordance
# with the terms of the license. Except as permitted by such
# license, no part of this software or documentation may be
# reproduced, stored in a retrieval system, or transmitted in any
# form or by any means without the express written consent of
# Intel Corporation.

#
#
# Module Name:
#
#   CpuAsm.S
# 
# Abstract:
# 
#
#-------------------------------------------------------------------------------


.text

ASM_GLOBAL ASM_PFX(ApMachineCheckHandler)
ASM_PFX(ApMachineCheckHandler):
  #
  # Clear MCIP flag of IA32_MCG_STATUS register
  #
  movl    $0x17a,%ecx
  rdmsr
  btrl    $2,%eax
  wrmsr

  iretq

ASM_GLOBAL ASM_PFX(ApMachineCheckHandlerEnd)

ASM_PFX(ApMachineCheckHandlerEnd):

#------------------------------------------------------------------------------
#  VOID
#  C1eExceptionHandler (
#    VOID
#    )
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(C1eExceptionHandler)
ASM_PFX(C1eExceptionHandler):

  pushq    %rbp
  movq     %rsp, %rbp

  pushfq
  cli

  # Fix the return address on stack to skip offending
  # code which caused the exception.
  movq    8(%rbp), %rax
  addq    $2, %rax
  movq    %rax, 8(%rbp)

  popfq
  popq    %rbp

  addq    $8, %rsp

  iretq


#------------------------------------------------------------------------------
#  VOID
#  CpuOnlyResetResumeEntryWrapper (
#    VOID                 *Context1,
#    VOID                 *Context2
#    )
# NOTE: This function is written in Long mode instructions and build by 64-bit 
#       assembler, BUT running in 32-bit protected mode
# This function is the real entry point after CPU only reset. SEC will directly 
# jump into this function when SEC determine the reset is a CPU only reset        
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(CpuOnlyResetResumeEntryWrapper)
ASM_PFX(CpuOnlyResetResumeEntryWrapper):
  popq %rax
  popq %rax
  popq %rax
  #
  # here esp is pointer to the temporary GDT description: TemporaryGdtLimit and
  # TemporaryGdtBase
  # load the temporary gdtr

  lgdt  (%rsp)

  # discards temporary GDT description and now esp is pointing to TemporaryCR3       
  # totally 8 byte
  popq %rax
  popq %rax

  popq %rax
  movq %rax, %cr3

  #
  # now ESP is point the reserve1, all parameter has already prepared in
  # the stack
  #
  jmp  ASM_PFX(InternalX86EnablePaging64)


