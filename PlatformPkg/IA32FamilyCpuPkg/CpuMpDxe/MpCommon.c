/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  MpCommon.c

Abstract: 
  Framework PEIM to initialize memory on a TC Memory Controller.

Revision History:
Bug 3192:   System will hang during post.
TIME:       2011-12-07
$AUTHOR:    Guan Junfeng 
$REVIEWERS:   
$SCOPE:     Tunnel Creek CPU Board
$TECHNICAL: 
1. Mov CPU wakeup buffer form 7F000 to 3F000.
2. Update stack size.
$END--------------------------------------------------------------------
/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file

  Common functions for CPU DXE module.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  MpCommon.c

**/

#include "MpCommon.h"
#include "MpService.h"
#include "Cpu.h"
#include "ArchSpecificDef.h"


AP_PROCEDURE          mApFunction = NULL;
AP_PROCEDURE          SimpleAPProcedure;
UINT32                NumberToFinish;
UINTN                 mApCount = 0;
UINTN                 mStartupVectorSize;
EFI_PHYSICAL_ADDRESS  mApMachineCheckHandlerBase;
UINT32                mApMachineCheckHandlerSize;


/**
  Allocates startup vector for APs.

  This function allocates Startup vector for APs.

  @param  Size  The size of startup vector.

**/
VOID
AllocateStartupVector (
  UINTN   Size
  )
{
  EFI_STATUS                            Status;
  EFI_GENERIC_MEMORY_TEST_PROTOCOL      *GenMemoryTest;
  EFI_PHYSICAL_ADDRESS                  StartAddress;
  UINTN                                 NeedSize;
  BOOLEAN                               GetMem;

  Status = gBS->LocateProtocol (
                  &gEfiGenericMemTestProtocolGuid,
                  NULL,
                  (VOID**)&GenMemoryTest
                  );
  if (EFI_ERROR (Status)) {
    GenMemoryTest = NULL;
  }

  GetMem = FALSE;
  NeedSize = EFI_SIZE_TO_PAGES(Size) * EFI_PAGE_SIZE;
  StartAddress = 0x40000 - NeedSize;
  while(1){
    if(StartAddress < 0x2000 || StartAddress <= NeedSize){
      break;
    }
    Status = gBS->AllocatePages (
                    AllocateAddress,
                    EfiBootServicesData,
                    EFI_SIZE_TO_PAGES(Size),
                    &StartAddress
                    );
    if(!EFI_ERROR(Status)){
      if (GenMemoryTest != NULL) {
        Status = GenMemoryTest->CompatibleRangeTest (
                                  GenMemoryTest,
                                  StartAddress,
                                  NeedSize
                                  );
        if (EFI_ERROR (Status)) {
          continue;
        }
      }
      GetMem = TRUE;
      break;      
    }
    StartAddress -= NeedSize;
  }

  ASSERT(GetMem);
  mStartupVectorSize = Size;
  mStartupVector     = StartAddress;
  DEBUG((EFI_D_ERROR, "ApVector:%lX\n", mStartupVector));

  ASSERT((PcdGet16(PcdCpuS3ApVectorMaxSize)&0xFFF)==0);
  ASSERT((UINTN)PcdGet16(PcdCpuS3ApVectorMaxSize) >= mStartupVectorSize);

}

 /**
  Count the number of APs that have been switched
  to E0000 or F0000 segments by ReAllocateMemoryForAP().

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
LegacyRegionAPCount (
  IN UINTN  ProcessorNumber
  )
{
  AcquireSpinLock (&mMPSystemData.APSerializeLock);

  mApCount++;

  ReleaseSpinLock (&mMPSystemData.APSerializeLock);
}


/**
  This AP function will place AP to the suitable state.
  
  If C-State is enable, try to place AP to the Mwait-Loop with deepest C-State, 
  otherwize place AP to  Hlt-Loop state.

  @param  Buffer  Pointer to MP Services.
**/
VOID
EFIAPI
ChangeApLoopMode (
  IN  VOID     *Buffer
  ) 
{
  UINTN                      ProcessorNumber;
  MONITOR_MWAIT_DATA         *MonitorAddr;

  WhoAmI (&mMpService, &ProcessorNumber);

  //
  // Set the deepest C-State value and ReadyToBootFlag for AP.
  //
  MonitorAddr = GetMonitorDataAddress(ProcessorNumber);

  MonitorAddr->CStateEnable = 0;
  if (MonitorAddr->CStateEnable) {
    MonitorAddr->MwaitTargetCstate = 0;
  }

  MonitorAddr->ReadyToBootFlag = TRUE;
}

/**
  Protocol notification that will wake up and place AP to the suitable state
  before booting to OS.

  @param  Event                 The triggered event.
  @param  Context               Context for this event.
**/
VOID
EFIAPI
ChangeApLoopModeCallBack (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS   Status;
  UINTN        NumberOfProcessors;
  UINTN        NumberOfEnabledProcessors;  


  while (ApRunning()) {
    CpuPause ();
  }

  Status = mMpService.GetNumberOfProcessors(&mMpService, &NumberOfProcessors, &NumberOfEnabledProcessors);
  ASSERT_EFI_ERROR(Status);  
  if(MIN(NumberOfProcessors, NumberOfEnabledProcessors) >= 2){
    Status = mMpService.StartupAllAPs (
                          &mMpService,
                          ChangeApLoopMode,
                          FALSE,
                          NULL,
                          0,
                          NULL,
                          NULL
                          );
    ASSERT_EFI_ERROR (Status);
  }  
}



// Data in EBDA may be moved to another position, so we should not directly use it for AP vector.
/**
  This function is invoked when LegacyBios protocol is installed, we must
  allocate reserved memory under 1M for AP.

  @param[in] Event    - The triggered event.
  @param[in] Context  - Context for this event.
**/
/*
VOID
EFIAPI
ReAllocateEbdaMemoryForAP (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  EFI_LEGACY_BIOS_PROTOCOL *LegacyBios;
  EFI_PHYSICAL_ADDRESS     EbdaOld;
  EFI_PHYSICAL_ADDRESS     EbdaNew;
  UINTN                    EbdaSize;
  EFI_STATUS               Status;

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return;
  }
  gBS->CloseEvent(Event);

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
	
  EbdaOld                     = (EFI_PHYSICAL_ADDRESS) (*(UINT16 *) (UINTN) 0x40E) << 4;;
  EbdaSize                    = (UINTN) (*((UINT8 *) (UINTN) EbdaOld));
  mLegacyRegion               = EbdaOld + (EbdaSize << 10);
  mLegacyRegion               = (mLegacyRegion - 0x1000) & 0xFFFFF000;
  EbdaNew                     = mLegacyRegion - (EbdaSize << 10);
  (*(UINT16 *) (UINTN) 0x40E) = (UINT16) (EbdaNew >> 4);
  CopyMem ((VOID *) (UINTN) EbdaNew, (VOID *) (UINTN) EbdaOld, EbdaSize << 10);

  *(UINT16 *) (UINTN) 0x413 = (*(UINT16 *) (UINTN) 0x413) - (UINT16) (((EbdaOld - EbdaNew) >> 10));

  DEBUG ((EFI_D_INFO, "mLegacyRegion CSM - %X\n", mLegacyRegion));

}
*/





/**
  Protocol notification that is fired when LegacyBios protocol is installed.

  Re-allocate a wakeup buffer from E/F segment because the previous wakeup buffer
  under 640K won't be preserved by the legacy OS.

  @param  Event                 The triggered event.
  @param  Context               Context for this event.
**/
VOID
EFIAPI
ReAllocateMemoryForAP (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                 Status;
  MP_CPU_EXCHANGE_INFO       *ExchangeInfo;
  MP_ASSEMBLY_ADDRESS_MAP    AddressMap;
  EFI_PHYSICAL_ADDRESS       LegacyRegion; 
  EFI_PHYSICAL_ADDRESS       OldStartupVector; 
  VOID                       *Interface;	

  
  Status = gBS->LocateProtocol(&gExitPmAuthProtocolGuid, NULL, (VOID**)&Interface);
  if (EFI_ERROR (Status)) {
    return ;
  }
  gBS->CloseEvent(Event);  
  
  while (ApRunning ()) {
    CpuPause ();
  }  

  LegacyRegion = PcdGet32(PcdCpuS3ApVectorAddress);
  ASSERT(LegacyRegion != 0xFFFFFFFF);		
  DEBUG((EFI_D_INFO, "ApVector:%lX\n", LegacyRegion));
  ASSERT (!((UINTN) LegacyRegion & 0x0FFF) && ((UINTN) LegacyRegion < 0xA0000));  
  CopyMem((VOID*)(UINTN)LegacyRegion, (VOID*)(UINTN)mStartupVector, mStartupVectorSize);
  
  OldStartupVector = mStartupVector;
  mStartupVector = LegacyRegion;
  AsmGetAddressMap (&AddressMap);
  ExchangeInfo = (MP_CPU_EXCHANGE_INFO*)(UINTN)(mStartupVector + AddressMap.Size);
  ExchangeInfo->BufferStart = (UINT32)(UINTN)LegacyRegion;
  RedirectFarJump (&AddressMap, (EFI_PHYSICAL_ADDRESS)(UINTN)LegacyRegion);
  mStartupVector = OldStartupVector;

  SendInitSipiSipiIpis (
    TRUE,
    0,
    LegacyRegionAPCount
    );
	
  while (mApCount < mCpuConfigConextBuffer.NumberOfProcessors - 1) {
    CpuPause();
  }

  FreePages ((VOID*)(UINTN)mStartupVector, EFI_SIZE_TO_PAGES(mStartupVectorSize));

  mStartupVector = (EFI_PHYSICAL_ADDRESS)(UINTN)LegacyRegion;
  mExchangeInfo = (MP_CPU_EXCHANGE_INFO *) (UINTN) (mStartupVector + AddressMap.Size);
}

/**
  Allocate aligned ACPI NVS memory below 4G.

  This function allocates aligned ACPI NVS memory below 4G.

  @param  Size       Size of memory region to allocate
  @param  Alignment  Alignment in bytes

  @return Base address of the allocated region

**/
VOID*
AllocateAlignedAcpiNvsMemory (
  IN  UINTN         Size,
  IN  UINTN         Alignment
  )
{
  UINTN       PointerValue;
  VOID        *Pointer;

  Pointer = AllocateAcpiNvsMemoryBelow4G (Size + Alignment - 1);

  PointerValue  = (UINTN) Pointer;
  PointerValue  = (PointerValue + Alignment - 1) / Alignment * Alignment;

  Pointer      = (VOID *) PointerValue;

  return Pointer;
}

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param  Size         Size of memory to allocate.

  @return Allocated address for output.

**/
VOID*
AllocateAcpiNvsMemoryBelow4G (
  IN   UINTN   Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer;

  Pages = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiACPIMemoryNVS,
                   Pages,
                   &Address
                   );
  ASSERT_EFI_ERROR (Status);

  Buffer = (VOID *) (UINTN) Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}

/**
  Sends INIT-SIPI-SIPI to AP.

  This function sends INIT-SIPI-SIPI to AP, and assign procedure specified by ApFunction.

  @param  Broadcast   If TRUE, broadcase IPI to all APs; otherwise, send to specified AP.
  @param  ApicID      The Local APIC ID of the specified AP. If Broadcast is TRUE, it is ignored.
  @param  ApFunction  The procedure for AP to work on.

**/
VOID
SendInitSipiSipiIpis (
  IN BOOLEAN            Broadcast,
  IN UINT32             ApicID,
  IN AP_PROCEDURE       ApFunction
  )
{
  mApFunction = ApFunction;

  if (Broadcast) {
    SendInitSipiSipiAllExcludingSelf ((UINT32) mStartupVector);
  } else {
    SendInitSipiSipi (ApicID, (UINT32) mStartupVector);
  }
}

/**
  A simple wrapper function dispatched to AP.

  This function is a simple wrapper function dispatched to AP. It invokes task for AP, and count down
  the number.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
SimpleApProcWrapper (
  IN UINTN  ProcessorNumber
  )
{
  //
  // Program virtual wire mode for AP, since it will be lost after AP wake up
  //
  ProgramVirtualWireMode ();
  DisableLvtInterrupts ();

  // Initialize Debug Agent to support source level debug on AP code.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_DXE_AP, NULL, NULL);

  //
  // Invoke task for AP.
  //
  SimpleAPProcedure (ProcessorNumber);

  //
  // Count down the number with lock mechanism.
  //
  InterlockedDecrement (&NumberToFinish);
}

/**
  Dispatches task to AP.

  This function dispatches task to AP. The BSP waits until specified APs have finished.

  @param  Broadcast   If TRUE, send task to all APs; otherwise, send to specified AP.
  @param  ApicID      The Local APIC ID of the specified AP. If Broadcast is TRUE, it is ignored.
  @param  Procedure   The procedure for AP to work on.

**/
VOID
DispatchAPAndWait (
  IN BOOLEAN             Broadcast,
  IN UINT32              ApicID,
  IN AP_PROCEDURE        Procedure
  )
{
  //
  // Prepares the task for AP. It will invoked by SimpleApProcWrapper.
  //
  SimpleAPProcedure = Procedure;

  //
  // Checks whether the function is for broadcast.
  //
  if (Broadcast) {
    //
    // If in broadcast mode, the number to finish is the number of all APs
    //
    NumberToFinish = (UINT32) mCpuConfigConextBuffer.NumberOfProcessors - 1;
  } else {
    //
    // If in non-broadcast mode, the number to finish is 1
    //
    NumberToFinish = 1;
  }

  //
  // Wake up specified AP(s), and make them work on SimpleApProcWrapper, which
  // will in turn invoke Procedure.
  //
  SendInitSipiSipiIpis (
    Broadcast,
    ApicID,
    SimpleApProcWrapper
    );

  //
  // BSP waits until specified AP(s) have finished.
  //
  while (NumberToFinish > 0) {
    CpuPause ();
  }
}

/**
  Creates a copy of GDT and IDT for all APs.

  This function creates a copy of GDT and IDT for all APs.

  @param  Gdtr   Base and limit of GDT for AP
  @param  Idtr   Base and limit of IDT for AP

**/
VOID
PrepareGdtIdtForAP (
  OUT IA32_DESCRIPTOR          *Gdtr,
  OUT IA32_DESCRIPTOR          *Idtr
  )
{
  SEGMENT_DESCRIPTOR        *GdtForAP;
  INTERRUPT_GATE_DESCRIPTOR *IdtForAP;
  IA32_DESCRIPTOR           GdtrForBSP;
  IA32_DESCRIPTOR           IdtrForBSP;
  VOID                      *MachineCheckHandlerBuffer;

  //
  // Get the BSP's data of GDT and IDT
  //
  AsmReadGdtr ((IA32_DESCRIPTOR *) &GdtrForBSP);
  AsmReadIdtr ((IA32_DESCRIPTOR *) &IdtrForBSP);

  //
  // Allocate ACPI NVS memory for GDT, IDT, and machine check handler.
  // Combine allocation for ACPI NVS memory under 4G to save memory.
  //
  GdtForAP = AllocateAlignedAcpiNvsMemory (
               (GdtrForBSP.Limit + 1) + (IdtrForBSP.Limit + 1) + (UINTN) ApMachineCheckHandlerEnd - (UINTN) ApMachineCheckHandler,
               8
               );

  //
  // GDT base is 8-bype aligned, and its size is multiple of 8-bype, so IDT base here is
  // also 8-bype aligned.
  //
  IdtForAP = (INTERRUPT_GATE_DESCRIPTOR *) ((UINTN) GdtForAP + GdtrForBSP.Limit + 1);
  MachineCheckHandlerBuffer = (VOID *) ((UINTN) GdtForAP + (GdtrForBSP.Limit + 1) + (IdtrForBSP.Limit + 1));
  //
  // Make copy for APs' GDT & IDT
  //
  CopyMem (GdtForAP, (VOID *) GdtrForBSP.Base, GdtrForBSP.Limit + 1);
  CopyMem (IdtForAP, (VOID *) IdtrForBSP.Base, IdtrForBSP.Limit + 1);

  //
  // Copy code for AP's machine check handler to ACPI NVS memory, and register in IDT
  //
  CopyMem (
    MachineCheckHandlerBuffer,
    (VOID *) (UINTN) ApMachineCheckHandler,
    (UINTN) ApMachineCheckHandlerEnd - (UINTN) ApMachineCheckHandler
    );
  SetIdtEntry ((UINTN) MachineCheckHandlerBuffer, &IdtForAP[INTERRUPT_HANDLER_MACHINE_CHECK]);

  //
  // Set AP's profile for GDTR and IDTR
  //
  Gdtr->Base  = (UINTN) GdtForAP;
  Gdtr->Limit = GdtrForBSP.Limit;

  Idtr->Base  = (UINTN) IdtForAP;
  Idtr->Limit = IdtrForBSP.Limit;

  //
  // Save the AP's machine check handler information
  //
  mApMachineCheckHandlerBase = (EFI_PHYSICAL_ADDRESS) (UINTN) MachineCheckHandlerBuffer;
  mApMachineCheckHandlerSize = (UINT32) ((UINTN) ApMachineCheckHandlerEnd - (UINTN) ApMachineCheckHandler);
}

/**
  Get mointor data address for specified processor.

  @param  ProcessorNumber    Handle number of specified logical processor.

  @return Pointer to monitor data.
**/
MONITOR_MWAIT_DATA *
EFIAPI
GetMonitorDataAddress (
  IN UINTN  ProcessorNumber
  )
{
  return (MONITOR_MWAIT_DATA *) ((UINT8*) mExchangeInfo->StackStart + (ProcessorNumber + 1) *  mExchangeInfo->StackSize - MONITOR_FILTER_SIZE);
}
