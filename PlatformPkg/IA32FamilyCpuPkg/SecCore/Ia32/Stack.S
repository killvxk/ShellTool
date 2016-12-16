//
// This file contains an 'Intel Pre-EFI Module' and is licensed
// for Intel CPUs and Chipsets under the terms of your license 
// agreement with Intel or your vendor.  This file may be      
// modified by the user, subject to additional terms of the    
// license agreement                                           
//
#------------------------------------------------------------------------------
#
# Copyright (c) 2007 - 2009, Intel Corporation. All rights reserved.<BR>
# This software and associated documentation (if any) is furnished
# under a license and may only be used or copied in accordance
# with the terms of the license. Except as permitted by such
# license, no part of this software or documentation may be
# reproduced, stored in a retrieval system, or transmitted in any
# form or by any means without the express written consent of
# Intel Corporation.
#
# Abstract:
#
#   Switch the stack from temporary memory to permenent memory.
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# VOID
# EFIAPI
# SecSwitchStack (
#   UINT32   TemporaryMemoryBase,
#   UINT32   PermenentMemoryBase
#   )#
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX (SecSwitchStack)
ASM_PFX(SecSwitchStack):
    #
    # Save standard registers so they can be used to change stack
    #
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %edx

    #
    # !!CAUTION!! this function address's is pushed into stack after
    # migration of whole temporary memory, so need save it to permenent
    # memory at first!
    #
    movl  20(%esp), %ebx         # Save the first parameter
    movl  24(%esp), %ecx         # Save the second parameter

    #
    # Save this function's return address into permenent memory at first.
    # Then, Fixup the esp point to permenent memory
    #
    movl  %esp, %eax
    subl  %ebx, %eax
    addl  %ecx, %eax
    movl  0(%esp), %edx          # copy pushed register's value to permenent memory
    movl  %edx, 0(%eax)
    movl  4(%esp), %edx
    movl  %edx, 4(%eax)
    movl  8(%esp), %edx
    movl  %edx, 8(%eax)
    movl  12(%esp), %edx
    movl  %edx, 12(%eax)
    movl  16(%esp), %edx        # Update this function's return address into permenent memory
    movl  %edx, 16(%eax)
    movl  %eax, %esp            # From now, esp is pointed to permenent memory

    #
    # Fixup the ebp point to permenent memory
    #
    movl  %ebp, %eax
    subl  %ebx, %eax
    addl  %ecx, %eax
    movl  %eax, %ebp            # From now, ebp is pointed to permenent memory

    popl  %edx
    popl  %ecx
    popl  %ebx
    popl  %eax
    ret


