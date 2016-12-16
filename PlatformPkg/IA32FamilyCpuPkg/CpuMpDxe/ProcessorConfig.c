/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file
  Code for processor configuration.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  ProcessorConfig.c

**/

#include "MpService.h"
#include "Cpu.h"

MP_SYSTEM_DATA                      mMPSystemData;
CPU_CONFIG_CONTEXT_BUFFER           mCpuConfigConextBuffer;
EFI_PHYSICAL_ADDRESS                mStartupVector;
MTRR_SETTINGS                       *mMtrrSettings;
EFI_TIMER_ARCH_PROTOCOL             *mTimer;

/**
  Prepares memory region for processor configuration.
  
  This function prepares memory region for processor configuration.

**/
VOID
PrepareMemoryForConfiguration (
  VOID
  )
{
  UINTN                Index;
  MONITOR_MWAIT_DATA   *MonitorData;

  //
  // Initialize Spin Locks for system
  //
  InitializeSpinLock (&mMPSystemData.APSerializeLock);
  for (Index = 0; Index < PcdGet32(PcdCpuMaxLogicalProcessorNumber); Index++) {
    InitializeSpinLock (&mMPSystemData.CpuData[Index].CpuDataLock);
  }

  //
  // Claim memory for AP stack.
  //
  mExchangeInfo->StackStart = AllocateAcpiNvsMemoryBelow4G(PcdGet32(PcdCpuMaxLogicalProcessorNumber) * PcdGet32(PcdCpuApStackSize));
  mExchangeInfo->StackSize  = PcdGet32(PcdCpuApStackSize);
  DEBUG((EFI_D_ERROR, "ExchangeInfo.Stack(%X,%X*%X)\n",          \
                      (UINT32)(UINTN)mExchangeInfo->StackStart,   \
                      PcdGet32(PcdCpuMaxLogicalProcessorNumber),  \
                      PcdGet32(PcdCpuApStackSize)                 \
                      ));

// Initialize the Monitor Data structure in APs' stack
  for (Index = 0; Index < PcdGet32(PcdCpuMaxLogicalProcessorNumber); Index++) {
    MonitorData = GetMonitorDataAddress(Index);
    MonitorData->ApLoopMode = ApInHltLoop;
  }

// Initialize data for CPU configuration context buffer
  mCpuConfigConextBuffer.CollectedDataBuffer = AllocateZeroPool(sizeof(CPU_COLLECTED_DATA) * mCpuConfigConextBuffer.NumberOfProcessors);
  ASSERT(mCpuConfigConextBuffer.CollectedDataBuffer != NULL);	
}

/*
VOID 
UpdateMicroCode (
  VOID
  )
{
  UINT32  Rev;
  UINT32  McAddr;	

  AsmWriteMsr64(EFI_MSR_IA32_BIOS_SIGN_ID, 0);
  AsmCpuid(EFI_CPUID_VERSION_INFO, NULL, NULL, NULL, NULL);
  Rev = (UINT32)RShiftU64(AsmReadMsr64(EFI_MSR_IA32_BIOS_SIGN_ID),32);
  McAddr = gMicroCodeAddress;

	
  if(Rev == 0 && McAddr){
    AsmWriteMsr64(EFI_MSR_IA32_BIOS_SIGN_ID, 0);
    AsmWriteMsr64(EFI_MSR_IA32_BIOS_UPDT_TRIG, McAddr);
    ASSERT((AsmReadMsr64(0x1205)&0xFF) == 0x01);
  }		
}
*/



VOID
EarlyMpInit (
  IN UINTN  ProcessorNumber
  )
{

//UpdateMicroCode();

  if(ProcessorNumber != mCpuConfigConextBuffer.BspNumber){
    MtrrSetAllMtrrs(mMtrrSettings);
  }  
  
  CollectBasicProcessorData(ProcessorNumber);
}









/**
  Configures all logical processors with three-phase architecture.
  
  This function configures all logical processors with three-phase architecture.

**/
VOID
ProcessorConfiguration (
  VOID
  )
{
  EFI_STATUS    Status;
  VOID          *Registration;	


  WakeupAPAndCollectBist();
  PrepareMemoryForConfiguration();

  mMtrrSettings = (MTRR_SETTINGS*)(UINTN)PcdGet64(PcdCpuMtrrTableAddress);
  ASSERT(mMtrrSettings!=NULL);
  MtrrGetAllMtrrs(mMtrrSettings);

  EarlyMpInit(mCpuConfigConextBuffer.BspNumber);
  DispatchAPAndWait (
    TRUE,
    0,
    EarlyMpInit
    );

// Locate Timer Arch Protocol
  Status = gBS->LocateProtocol(&gEfiTimerArchProtocolGuid, NULL, (VOID**)&mTimer);
  ASSERT_EFI_ERROR(Status);

//SaveCpuS3Data();

// Create timer event to check AP state for non-blocking execution.
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  CheckAPsStatus,
                  NULL,
                  &mMPSystemData.CheckAPsEvent
                  );
  ASSERT_EFI_ERROR (Status);
  Status = gBS->SetTimer (
                  mMPSystemData.CheckAPsEvent,
                  TimerPeriodic,
                  100000 * MICROSECOND		// 100ms
                  );
  ASSERT_EFI_ERROR (Status);

// Setup notification on Legacy BIOS Protocol to reallocate AP wakeup
  EfiCreateProtocolNotifyEvent (
    &gExitPmAuthProtocolGuid,
    TPL_CALLBACK,
    ReAllocateMemoryForAP,
    NULL,
    &Registration
    );

  Status = gBS->InstallProtocolInterface (
                  &gImageHandle,
                  &gEfiMpServiceProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMpService
                  );
  ASSERT_EFI_ERROR (Status);
}


/**
  Callback function for idle events.
 
  @param  Event                 Event whose notification function is being invoked.
  @param  Context               The pointer to the notification function's context,
                                which is implementation-dependent.

**/
VOID
EFIAPI
IdleLoopEventCallback (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  CpuSleep ();
}

/**
  Entrypoint of CPU MP DXE module.
  
  This function is the entrypoint of CPU MP DXE module.
  It initializes Multi-processor configuration and installs MP Services Protocol.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The entrypoint always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
MultiProcessorInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   IdleLoopEvent;
  EFI_EVENT   ExitBootServiceEvent;
  EFI_EVENT   LegacyToBootEvent;

  DEBUG((EFI_D_INFO, "CpuMpDxe\n"));

  ProcessorConfiguration();

// Create EXIT_BOOT_SERIVES Event to set AP to suitable status
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  ChangeApLoopModeCallBack,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &ExitBootServiceEvent
                  );
  ASSERT_EFI_ERROR (Status);

// Create an event to be signalled when Legacy Boot occurs 
// to set AP to suitable status  
  Status = EfiCreateEventLegacyBootEx(
             TPL_NOTIFY, 
             ChangeApLoopModeCallBack, 
             NULL, 
             &LegacyToBootEvent
             );
  ASSERT_EFI_ERROR (Status);


// Setup a callback for idle events
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IdleLoopEventCallback,
                  NULL,
                  &gIdleLoopEventGuid,
                  &IdleLoopEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}




VOID
WakeupAPAndCollectBist (
  VOID
  )
{
  UINTN  Index1;
  UINTN  Index2;
  UINTN  Index3;
  UINT32 ApicId;
  UINT32 Bist;


  PrepareAPStartupVector();

  mCpuConfigConextBuffer.NumberOfProcessors = 1;
  mCpuConfigConextBuffer.BspNumber = 0;

  mExchangeInfo->BistBuffer[0].ApicId = GetApicId();  // Item 0 of BistBuffer is for BSP.
	
  SendInitSipiSipiIpis (
    TRUE,
    0,
    NULL
    );

  MicroSecondDelay(PcdGet32(PcdCpuApInitTimeOutInMicroSeconds));
	
  if (mExchangeInfo->ApCount == 0) {
    goto NoApPresent;
  }  

  ASSERT((mExchangeInfo->ApCount+1 <= FixedPcdGet32(PcdCpuMaxLogicalProcessorNumber)));

  for (Index1 = 0; Index1 < mExchangeInfo->ApCount; Index1++) {
    Index3 = Index1;
    ApicId = mExchangeInfo->BistBuffer[Index1].ApicId;
		
    for (Index2 = Index1 + 1; Index2 <= mExchangeInfo->ApCount; Index2++) {
      if (ApicId > mExchangeInfo->BistBuffer[Index2].ApicId) {
        Index3 = Index2;
        ApicId = mExchangeInfo->BistBuffer[Index2].ApicId;
      }
    }
    
    if (Index3 != Index1) {
      mExchangeInfo->BistBuffer[Index3].ApicId = mExchangeInfo->BistBuffer[Index1].ApicId;
      mExchangeInfo->BistBuffer[Index1].ApicId = ApicId;
      Bist = mExchangeInfo->BistBuffer[Index3].Bist;
      mExchangeInfo->BistBuffer[Index3].Bist = mExchangeInfo->BistBuffer[Index1].Bist;
      mExchangeInfo->BistBuffer[Index1].Bist = Bist;
    }
  }
  
  mCpuConfigConextBuffer.NumberOfProcessors += mExchangeInfo->ApCount;

  ApicId = GetApicId();
  for (Index1 = 0; Index1 < mCpuConfigConextBuffer.NumberOfProcessors; Index1++) {
    if (mExchangeInfo->BistBuffer[Index1].ApicId == ApicId) {
      mCpuConfigConextBuffer.BspNumber = Index1;
      break;
    }
  }
  
NoApPresent:  
  mCpuConfigConextBuffer.PhysicalCoreCount = mCpuConfigConextBuffer.NumberOfProcessors;	
  mExchangeInfo->InitFlag = 0;
  
  DEBUG((EFI_D_INFO, "CpuCores:%d, BspNo:%d\n", mCpuConfigConextBuffer.NumberOfProcessors, mCpuConfigConextBuffer.BspNumber));	
  for(Index1=0;Index1<mCpuConfigConextBuffer.NumberOfProcessors;Index1++){
    DEBUG((EFI_D_INFO, "ApicId:%X\n", mExchangeInfo->BistBuffer[Index1].ApicId));
  }
}



