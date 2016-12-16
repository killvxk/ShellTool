//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
#------------------------------------------------------------------------------
#
# Copyright (c) 1999 - 2008, Intel Corporation. All rights reserved.<BR>
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
#   CpuAsm.S
# 
# Abstract:
# 
#   Assembly code that supports IA32 CPU architectural protocol
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(ApMachineCheckHandler)
ASM_PFX(ApMachineCheckHandler):
  #
  # Clear MCIP flag of IA32_MCG_STATUS register
  #
  movl    $0x17a,%ecx
  rdmsr
  btrl    $2,%eax
  wrmsr

  iret

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
  pushl   %ebp            # C prolog
  movl    %esp,%ebp
  pushfl
  cli
  pushal

  # Fix the return address on stack to skip offending
  # code which caused the exception.
  movl   8(%ebp), %eax
  addl   $2,%eax
  movl   %eax , 8(%ebp)

  popal
  popfl
  popl   %ebp
  addl   $4,%esp
  iretl
