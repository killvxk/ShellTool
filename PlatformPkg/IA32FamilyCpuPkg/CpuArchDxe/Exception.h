//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file

  Include file for exception handler

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  Exception.h

**/
#ifndef _CPU_EXCEPTION_H_
#define _CPU_EXCEPTION_H_

#include <Protocol/DebugSupport.h>

#define INTERRUPT_HANDLER_DIVIDE_ZERO           0x00
#define INTERRUPT_HANDLER_DEBUG                 0x01
#define INTERRUPT_HANDLER_NMI                   0x02
#define INTERRUPT_HANDLER_BREAKPOINT            0x03
#define INTERRUPT_HANDLER_OVERFLOW              0x04
#define INTERRUPT_HANDLER_BOUND                 0x05
#define INTERRUPT_HANDLER_INVALID_OPCODE        0x06
#define INTERRUPT_HANDLER_DEVICE_NOT_AVAILABLE  0x07
#define INTERRUPT_HANDLER_TIMER                 0x08
#define INTERRUPT_HANDLER_COPROCESSOR_OVERRUN   0x09
#define INTERRUPT_HANDLER_INVALID_TSS           0x0A
#define INTERRUPT_HANDLER_SEGMENT_NOT_PRESENT   0x0B
#define INTERRUPT_HANDLER_STACK_SEGMENT_FAULT   0x0C
#define INTERRUPT_HANDLER_GP_FAULT              0x0D
#define INTERRUPT_HANDLER_PAGE_FAULT            0x0E
#define INTERRUPT_HANDLER_RESERVED              0x0F
#define INTERRUPT_HANDLER_MATH_FAULT            0x10
#define INTERRUPT_HANDLER_ALIGNMENT_FAULT       0x11
#define INTERRUPT_HANDLER_MACHINE_CHECK         0x12
#define INTERRUPT_HANDLER_STREAMING_SIMD        0x13

//
// Register Structure Definitions
//
typedef union {
  EFI_SYSTEM_CONTEXT_IA32 SystemContextIa32;
  EFI_SYSTEM_CONTEXT_X64  SystemContextX64;
} SYSTEM_CONTEXT;

//
// Register Structure Definitions
//
typedef struct {
  EFI_STATUS_CODE_DATA    Header;
  SYSTEM_CONTEXT          SystemContext;
} CPU_STATUS_CODE_TEMPLATE;

typedef struct {
  UINT32        ErrorMessage;
  UINT8         Interrupt;
} EFI_EXCEPTION_HANDLER;

/**
  The sample exception handler.

  This function is called by primitive interrupt handler in assembly code, and
  parameters of InterruptType and SystemContext are prepared by it beforehand.
  This function outputs context of all registers when exception occurs. It then
  reports status code for the exception.

  @param  InterruptType  Exception type.
  @param  SystemContext  System context data.

**/
VOID
EFIAPI
CommonExceptionHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  );

/**
  Installs the Exception Handler.

  This function installs the Exception Handler.

  @param  ExceptionBegin   The begin number of exception

**/
VOID
InitializeException (
  IN UINT32              ExceptionBegin         
  );

/**
  Set Interrupt Descriptor Table Handler Address.

  @param Index        The Index of the interrupt descriptor table handle.

**/
VOID
SetInterruptDescriptorTableHandlerAddress (
  IN UINTN       Index
  );

/**
  Restore original Interrupt Descriptor Table Handler Address.

  @param Index        The Index of the interrupt descriptor table handle.

**/
VOID
RestoreInterruptDescriptorTableHandlerAddress (
  IN UINTN       Index
  );

#endif
