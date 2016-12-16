/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file

  EM64T exception handler.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  Exception.c

**/

#include "Cpu.h"
#include "Exception.h"

//
// Error code flag indicating whether or not an error code will be 
// pushed on the stack if an exception occurs.
//
// 1 means an error code will be pushed, otherwise 0
//
// bit 0 - exception 0
// bit 1 - exception 1
// etc.
//
UINT32 mErrorCodeFlag = 0x00027d00;

//
// Local Table
//
EFI_EXCEPTION_HANDLER mExceptionTable[] = {
          { EFI_SW_EC_IA32_DIVIDE_ERROR,     INTERRUPT_HANDLER_DIVIDE_ZERO },
          { EFI_SW_EC_IA32_DEBUG,            INTERRUPT_HANDLER_DEBUG },
          { EFI_SW_EC_IA32_NMI,              INTERRUPT_HANDLER_NMI },
          { EFI_SW_EC_IA32_BREAKPOINT,       INTERRUPT_HANDLER_BREAKPOINT },
          { EFI_SW_EC_IA32_OVERFLOW,         INTERRUPT_HANDLER_OVERFLOW },
          { EFI_SW_EC_IA32_BOUND,            INTERRUPT_HANDLER_BOUND }, 
          { EFI_SW_EC_IA32_INVALID_OPCODE,   INTERRUPT_HANDLER_INVALID_OPCODE },
//
// Interrupt 7, 9, 15 not defined in the debug support protocol. Hence no status codes for them!
//
// Yup, we skipped Interrupt 8 (timer)
//
//          { DoubleFaultHandler, INTERRUPT_HANDLER_TIMER },
          { EFI_SW_EC_IA32_INVALID_TSS,      INTERRUPT_HANDLER_INVALID_TSS },        
          { EFI_SW_EC_IA32_SEG_NOT_PRESENT,  INTERRUPT_HANDLER_SEGMENT_NOT_PRESENT },
          { EFI_SW_EC_IA32_STACK_FAULT,      INTERRUPT_HANDLER_STACK_SEGMENT_FAULT },
          { EFI_SW_EC_IA32_GP_FAULT,         INTERRUPT_HANDLER_GP_FAULT },           
          { EFI_SW_EC_IA32_PAGE_FAULT,       INTERRUPT_HANDLER_PAGE_FAULT },         
          { EFI_SW_EC_IA32_FP_ERROR,         INTERRUPT_HANDLER_MATH_FAULT },         
          { EFI_SW_EC_IA32_ALIGNMENT_CHECK,  INTERRUPT_HANDLER_ALIGNMENT_FAULT },    
          { EFI_SW_EC_IA32_MACHINE_CHECK,    INTERRUPT_HANDLER_MACHINE_CHECK },
          { EFI_SW_EC_IA32_SIMD,             INTERRUPT_HANDLER_STREAMING_SIMD }
          };

UINTN     mExceptionNumber = sizeof (mExceptionTable) / sizeof (EFI_EXCEPTION_HANDLER);

CPU_STATUS_CODE_TEMPLATE  mStatusCodeData  =  {
  {
    sizeof (EFI_STATUS_CODE_DATA),
    sizeof (EFI_SYSTEM_CONTEXT_X64),
    { 0 }
  },
  {
    {0}
  }
};

/**
  Reports StatusCode for Exception

  This function reports status code for exception.

  @param  InterruptType   Interrupt type
  @param  SystemContext   EFI_SYSTEM_CONTEXT

**/
VOID
ReportData (
  IN EFI_EXCEPTION_TYPE   InterruptType, 
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  UINT32                  ErrorMessage;
  UINT32                  Index;

  CopyMem (
    &mStatusCodeData.SystemContext.SystemContextX64,
    SystemContext.SystemContextX64,
    sizeof (EFI_SYSTEM_CONTEXT_X64)
    );

  ErrorMessage = EFI_SOFTWARE_DXE_BS_DRIVER;
  for (Index = 0; Index < mExceptionNumber; Index++) {
    if (mExceptionTable[Index].Interrupt == InterruptType) {
      ErrorMessage |= mExceptionTable[Index].ErrorMessage;
      break;
    }
  }
  
  REPORT_STATUS_CODE_EX (
    (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED),
    EFI_SOFTWARE_UNSPECIFIED | ErrorMessage,
    0,
    NULL,
    NULL,
    (UINT8 *)&mStatusCodeData + sizeof (EFI_STATUS_CODE_DATA),
    sizeof (EFI_SYSTEM_CONTEXT_X64)
    ); 
}

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
  )
{
  //
  // Output context of all registers when exception occurs.
  //
  DEBUG ((
    EFI_D_ERROR,
    "!!!! X64 Exception Type - %016lx !!!!\n",
    InterruptType
    ));
  DEBUG ((
    EFI_D_ERROR,
    "RIP - %016lx, CS - %016lx, RFLAGS - %016lx\n",
    SystemContext.SystemContextX64->Rip,
    SystemContext.SystemContextX64->Cs,
    SystemContext.SystemContextX64->Rflags
    ));
  if ((mErrorCodeFlag & (1 << InterruptType)) != 0) {
    DEBUG ((
      EFI_D_ERROR,
      "ExceptionData - %016lx\n",
      SystemContext.SystemContextX64->ExceptionData
      ));
  }
  DEBUG ((
    EFI_D_ERROR,
    "RAX - %016lx, RCX - %016lx, RDX - %016lx\n",
    SystemContext.SystemContextX64->Rax,
    SystemContext.SystemContextX64->Rcx,
    SystemContext.SystemContextX64->Rdx
    ));
  DEBUG ((
    EFI_D_ERROR,
    "RBX - %016lx, RSP - %016lx, RBP - %016lx\n",
    SystemContext.SystemContextX64->Rbx,
    SystemContext.SystemContextX64->Rsp,
    SystemContext.SystemContextX64->Rbp
    ));
  DEBUG ((
    EFI_D_ERROR,
    "RSI - %016lx, RDI - %016lx\n",
    SystemContext.SystemContextX64->Rsi,
    SystemContext.SystemContextX64->Rdi
    ));
  DEBUG ((
    EFI_D_ERROR,
    "R8 - %016lx, R9 - %016lx, R10 - %016lx\n",
    SystemContext.SystemContextX64->R8,
    SystemContext.SystemContextX64->R9,
    SystemContext.SystemContextX64->R10
    ));
  DEBUG ((
    EFI_D_ERROR,
    "R11 - %016lx, R12 - %016lx, R13 - %016lx\n",
    SystemContext.SystemContextX64->R11,
    SystemContext.SystemContextX64->R12,
    SystemContext.SystemContextX64->R13
    ));
  DEBUG ((
    EFI_D_ERROR,
    "R14 - %016lx, R15 - %016lx\n",
    SystemContext.SystemContextX64->R14,
    SystemContext.SystemContextX64->R15
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DS - %016lx, ES - %016lx, FS - %016lx\n",
    SystemContext.SystemContextX64->Ds,
    SystemContext.SystemContextX64->Es,
    SystemContext.SystemContextX64->Fs
    ));
  DEBUG ((
    EFI_D_ERROR,
    "GS - %016lx, SS - %016lx\n",
    SystemContext.SystemContextX64->Gs,
    SystemContext.SystemContextX64->Ss
    ));
  DEBUG ((
    EFI_D_ERROR,
    "GDTR - %016lx %016lx, LDTR - %016lx\n",
    SystemContext.SystemContextX64->Gdtr[0],
    SystemContext.SystemContextX64->Gdtr[1],
    SystemContext.SystemContextX64->Ldtr
    ));
  DEBUG ((
    EFI_D_ERROR,
    "IDTR - %016lx %016lx, TR - %016lx\n",
    SystemContext.SystemContextX64->Idtr[0],
    SystemContext.SystemContextX64->Idtr[1],
    SystemContext.SystemContextX64->Tr
    ));
  DEBUG ((
    EFI_D_ERROR,
    "CR0 - %016lx, CR2 - %016lx, CR3 - %016lx\n",
    SystemContext.SystemContextX64->Cr0,
    SystemContext.SystemContextX64->Cr2,
    SystemContext.SystemContextX64->Cr3
    ));
  DEBUG ((
    EFI_D_ERROR,
    "CR4 - %016lx, CR8 - %016lx\n",
    SystemContext.SystemContextX64->Cr4,
    SystemContext.SystemContextX64->Cr8
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR0 - %016lx, DR1 - %016lx, DR2 - %016lx\n",
    SystemContext.SystemContextX64->Dr0,
    SystemContext.SystemContextX64->Dr1,
    SystemContext.SystemContextX64->Dr2
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR3 - %016lx, DR6 - %016lx, DR7 - %016lx\n",
    SystemContext.SystemContextX64->Dr3,
    SystemContext.SystemContextX64->Dr6,
    SystemContext.SystemContextX64->Dr7
    ));

  //
  // Report Status Code
  //
  ReportData (InterruptType, SystemContext);

  //
  // Enter a dead loop.
  //
  CpuDeadLoop ();

  return ;
}

/**
  Installs the Exception Handler.

  This function installs the Exception Handler.

  @param  ExceptionBegin   The begin number of exception

**/
VOID
InitializeException (
  IN UINT32              ExceptionBegin         
  )
{
  EFI_STATUS                      Status;
  UINT32                          Index;
  BOOLEAN                         OldInterruptState;

  //
  // Disable interrupt
  //
  OldInterruptState = SaveAndDisableInterrupts ();

  for (Index = ExceptionBegin; Index < mExceptionNumber; Index++) {
    //
    // Register sample excception handler
    //
    Status = mCpuArchProtocol.RegisterInterruptHandler (
                                 &mCpuArchProtocol, 
                                 mExceptionTable[Index].Interrupt,
                                 CommonExceptionHandler
                                 );
    ASSERT_EFI_ERROR (Status);
  }
  //
  // Restore interrupt state.
  //
  SetInterruptState (OldInterruptState);
}
