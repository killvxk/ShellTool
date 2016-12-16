/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file

  Entry point of CPU DXE driver.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  Cpu.c

**/

#include "Cpu.h"



BOOLEAN                     mIsFlushingGCD;
EFI_HANDLE                  mHandle = NULL;

//
// Vector Table for user's interrupt handlers
//
EFI_CPU_INTERRUPT_HANDLER   mExternalVectorTable[INTERRUPT_VECTOR_NUMBER];

//
// The Cpu Architectural Protocol that this Driver produces
//
EFI_CPU_ARCH_PROTOCOL       mCpuArchProtocol = {
  FlushCpuDataCache,
  EnableInterrupt,
  DisableInterrupt,
  GetInterruptStateInstance,
  Init,
  RegisterInterruptHandler,
  GetTimerValue,
  SetMemoryAttributes,
  1,
  4,
};

/**
  This function flushes CPU data cache.

  This function implements FlushDataCache() service of CPU Architecture Protocol.
  It flushes a range of the processor's data cache.

  @param  This              The EFI_CPU_ARCH_PROTOCOL instance.
  @param  Start             Physical address to start flushing from.
  @param  Length            Number of bytes to flush. Round up to chipset granularity.
  @param  FlushType         Specifies the type of flush operation to perform.

  @retval EFI_SUCCESS       Cache is flushed.
  @retval EFI_UNSUPPORTED   Flush type is not supported.
  @retval EFI_DEVICE_ERROR  Requested range could not be flushed..

**/
EFI_STATUS
EFIAPI
FlushCpuDataCache (
  IN EFI_CPU_ARCH_PROTOCOL     *This,
  IN EFI_PHYSICAL_ADDRESS      Start,
  IN UINT64                    Length,
  IN EFI_CPU_FLUSH_TYPE        FlushType
  )
{
  if (FlushType == EfiCpuFlushTypeWriteBackInvalidate) {
    AsmWbinvd ();
    return EFI_SUCCESS;
  } else if (FlushType == EfiCpuFlushTypeInvalidate) {
    AsmInvd ();
    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
}

/**
  This function enables interrupt processing by the processor.

  This function implements EnableInterrupt() service of CPU Architecture Protocol.
  This function enables interrupt processing by the processor. This function is
  used to implement the Boot Services RaiseTPL() and RestoreTPL().

  @param  This              The EFI_CPU_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS       Interrupts are enabled on the processor.
  @retval EFI_DEVICE_ERROR  Interrupts could not be enabled on the processor.

**/
EFI_STATUS
EFIAPI
EnableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL          *This
  )
{
  EnableInterrupts ();
  return EFI_SUCCESS;
}

/**
  This function disables interrupt processing by the processor.

  This function implements DisableInterrupt() service of CPU Architecture Protocol.
  This function disables interrupt processing by the processor. This function is
  used to implement the Boot Services RaiseTPL() and RestoreTPL().

  @param  This              The EFI_CPU_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS       Interrupts are disabled on the processor.
  @retval EFI_DEVICE_ERROR  Interrupts could not be disabled on the processor.

**/
EFI_STATUS
EFIAPI
DisableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL     *This
  )
{
  DisableInterrupts ();
  return EFI_SUCCESS;
}

/**
  This function retrieves the processor's current interrupt state.

  This function implements GetInterruptState() service of CPU Architecture Protocol.
  This function retrieves the processor's current interrupt state.

  @param  This                   The EFI_CPU_ARCH_PROTOCOL instance.
  @param  State                  A pointer to the processor's current interrupt state.
                                 Set to TRUE if interrupts are enabled and FALSE if interrupts are disabled.

  @retval EFI_SUCCESS            The processor's current interrupt state was returned in State.
  @retval EFI_INVALID_PARAMETER  State is NULL.

**/
EFI_STATUS
EFIAPI
GetInterruptStateInstance (
  IN  EFI_CPU_ARCH_PROTOCOL     *This,
  OUT BOOLEAN                   *State
  )
{
  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *State = GetInterruptState ();
  return EFI_SUCCESS;
}

/**
  This function generates an INIT on the processor.

  This function implements Init() service of CPU Architecture Protocol.
  This function generates an INIT on the processor.

  @param  This                   The EFI_CPU_ARCH_PROTOCOL instance.
  @param  InitType               The type of processor INIT to perform.

  @retval EFI_SUCCESS            The processor INIT was performed. This return code should never be seen.
  @retval EFI_UNSUPPORTED        The processor INIT operation specified by InitType is not supported by this processor.
  @retval EFI_DEVICE_ERROR       The processor INIT failed.

**/
EFI_STATUS
EFIAPI
Init (
  IN EFI_CPU_ARCH_PROTOCOL      *This,
  IN EFI_CPU_INIT_TYPE          InitType
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This function Registers a function to be called from the processor interrupt handler.

  This function implements RegisterInterruptHandler() service of CPU Architecture Protocol.
  This function Registers a function to be called from the processor interrupt handler.

  @param  This                   The EFI_CPU_ARCH_PROTOCOL instance.
  @param  InterruptType          Defines which interrupt or exception to hook.
  @param  InterruptHandler       A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER
                                 that is called when a processor interrupt occurs.
                                 If this parameter is NULL, then the handler will be uninstalled.

  @retval EFI_SUCCESS            The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED    InterruptHandler is not NULL, and a handler for InterruptType was previously installed.
  @retval EFI_INVALID_PARAMETER  InterruptHandler is NULL, and a handler for InterruptType was not previously installed.
  @retval EFI_UNSUPPORTED        The interrupt specified by InterruptType is not supported.

**/
EFI_STATUS
EFIAPI
RegisterInterruptHandler (
  IN EFI_CPU_ARCH_PROTOCOL         *This,
  IN EFI_EXCEPTION_TYPE            InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER     InterruptHandler
  )
{
  if (InterruptType < 0 || InterruptType > 0xff) {
    return EFI_UNSUPPORTED;
  }

  if (InterruptHandler == NULL && mExternalVectorTable[InterruptType] == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (InterruptHandler != NULL && mExternalVectorTable[InterruptType] != NULL) {
    return EFI_ALREADY_STARTED;
  }

  if (InterruptHandler != NULL) {
    SetInterruptDescriptorTableHandlerAddress ((UINTN)InterruptType);
  } else {
    //
    // Restore the original IDT handler address if InterruptHandler is NULL.
    //
    RestoreInterruptDescriptorTableHandlerAddress ((UINTN)InterruptType);
  }

  mExternalVectorTable[InterruptType] = InterruptHandler;
  return EFI_SUCCESS;
}

/**
  This function returns a timer value from one of the processor's internal timers.

  This function implements GetTimerValue() service of CPU Architecture Protocol.
  This function returns a timer value from one of the processor's internal timers.

  @param  This                   The EFI_CPU_ARCH_PROTOCOL instance.
  @param  TimerIndex             Specifies which processor timer is to be returned in TimerValue.
                                 This parameter must be between 0 and EFI_CPU_ARCH_PROTOCOL.NumberOfTimers-1.
  @param  TimerValue             Pointer to the returned timer value.
  @param  TimerPeriod            A pointer to the amount of time that passes in femtoseconds for each
                                 increment of TimerValue. If TimerValue does not increment at a predictable
                                 rate, then 0 is returned.

  @retval EFI_SUCCESS            The processor timer value specified by TimerIndex was returned in TimerValue.
  @retval EFI_INVALID_PARAMETER  TimerValue is NULL.
  @retval EFI_INVALID_PARAMETER  TimerIndex is not valid.
  @retval EFI_UNSUPPORTEDT       The processor does not have any readable timers.
  @retval EFI_DEVICE_ERROR       An error occurred attempting to read one of the processor's timers.

**/
EFI_STATUS
EFIAPI
GetTimerValue (
  IN  EFI_CPU_ARCH_PROTOCOL     *This,
  IN  UINT32                    TimerIndex,
  OUT UINT64                    *TimerValue,
  OUT UINT64                    *TimerPeriod OPTIONAL
  )
{
  EFI_STATUS      Status;
  UINT64          Actual;
  UINT64          Standard;
  UINT32          Ratio;
  
  if (TimerValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (TimerIndex != 0) {
    return EFI_INVALID_PARAMETER;
  }
  
  *TimerValue = AsmReadTsc ();
  
  if (TimerPeriod != NULL) {
    Status = GetActualFrequency (&Actual);
    ASSERT_EFI_ERROR (Status);
    
    Status = GetCpuBusRatio (&Ratio);
    ASSERT_EFI_ERROR (Status);
    
    //
    // Since actual frequency might not be accurate, convert it into standand
    // frequency: 100, 133, 166, 200, ...
    //
    Actual2StandardFrequency (Actual, Ratio, &Standard);
    
    *TimerPeriod = DivU64x32 (1000000000, (UINT32) Standard);
  }
  
  return EFI_SUCCESS;
}

/**
  Implementation of SetMemoryAttributes() service of CPU Architecture Protocol.

  This function modifies the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  BaseAddress      The physical address that is the start address of a memory region.
  @param  Length           The size in bytes of the memory region.
  @param  Attributes       The bit mask of attributes to set for the memory region.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of attributes that
                                cannot be set together.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not support for the memory resource
                                range specified by BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
SetMemoryAttributes (
  IN EFI_CPU_ARCH_PROTOCOL     *This,
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes
  )
{
  RETURN_STATUS             Status;
  EFI_STATUS                MpStatus;
  MTRR_MEMORY_CACHE_TYPE    CacheType;
  EFI_MP_SERVICES_PROTOCOL  *MpService;

  //
  // If this function is called because GCD SetMemorySpaceAttributes () is called
  // by RefreshGcdMemoryAttributes (), then we are just synchronzing GCD memory 
  // map with MTRR values. So there is no need to modify MTRRs, just return immediately
  // to avoid unnecessary computing.
  //
  if (mIsFlushingGCD) {
    return EFI_SUCCESS;
  }

  switch (Attributes) {
  case EFI_MEMORY_UC:
    CacheType = CacheUncacheable;
    break;

  case EFI_MEMORY_WC:
    CacheType = CacheWriteCombining;
    break;

  case EFI_MEMORY_WT:
    CacheType = CacheWriteThrough;
    break;

  case EFI_MEMORY_WP:
    CacheType = CacheWriteProtected;
    break;

  case EFI_MEMORY_WB:
    CacheType = CacheWriteBack;
    break;

  case EFI_MEMORY_UCE:
  case EFI_MEMORY_RP:
  case EFI_MEMORY_XP:
  case EFI_MEMORY_RUNTIME:
    return EFI_UNSUPPORTED;

  default:
    return EFI_INVALID_PARAMETER;
  }
  //
  // call MTRR libary function
  //
  Status = MtrrSetMemoryAttribute(
             BaseAddress,
             Length,
             CacheType
           );
  if(EFI_ERROR(Status))	{
    DEBUG((EFI_D_ERROR, "[CpuArchDxe] MtrrSetMemoryAttribute(%lX,%lX,%X):%r\n", BaseAddress, Length, CacheType, Status));
  }		

  if (!RETURN_ERROR (Status)) {
    //
    // Sync saved MTRR settings
    //
    MtrrGetAllMtrrs (mMtrrTable);
    //
    // Synchronize the update with all APs
    //
    MpStatus = gBS->LocateProtocol (
                      &gEfiMpServiceProtocolGuid,
                      NULL,
                      (VOID **) &MpService
                      );
    if (!EFI_ERROR (MpStatus)) {
      MpStatus = MpService->StartupAllAPs (
                              MpService,
                              LoadMtrrData,
                              TRUE,
                              NULL,
                              0,
                              NULL,
                              NULL
                              );
      if (MpStatus == EFI_NOT_STARTED) {
        //
        // If no enabled APs exit in the system, return success
        //
        MpStatus = EFI_SUCCESS;
      }
      ASSERT_EFI_ERROR (MpStatus);
    }
  }
  return (EFI_STATUS) Status;
}

/**
  Entrypoint of CPU DXE module.
  
  This function is the entrypoint of CPU DXE module. It initializes system and installs
  CPU Architecture Protocol. 

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The entrypoint always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
InitializeCpu (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS              Status;
//EFI_PHYSICAL_ADDRESS    BaseAddress;

  //
  // Initialize GDT and IDT
  //
  InitializeDescriptorTables ();

  //
  // Install CPU Architectural Protocol
  //
  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiCpuArchProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mCpuArchProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Refresh GCD memory space map according to MTRR value.
  //
  RefreshGcdMemoryAttributes ();

  // Program Local APIC for virtual wire mode. Enable Spurious
  // Vector, program the LINT0 vector entry as ExtInt, and 
  // program the LINT1 vector entry as NMI
  //
  ProgramVirtualWireMode ();

  //
  // Allocates ACPI NVS memory for MTRR data.
  //
  InitializeMtrrData ();
  MtrrGetAllMtrrs (mMtrrTable);

  return EFI_SUCCESS;
}

/**
  Returns the CPU core to processor bus frequency ratio.

  This function returns the CPU core to processor bus frequency ratio.

  @param   Ratio                   Pointer to the CPU core to processor bus frequency ratio.

  @retval  EFI_SUCCESS             The ratio is returned successfully
  @retval  EFI_UNSUPPORTED         The ratio cannot be measured
  @retval  EFI_INVALID_PARAMETER   The input parameter is not valid

**/
EFI_STATUS
GetCpuBusRatio (
  OUT UINT32        *Ratio
  )
{
  UINT32              RegEax;
  UINT32              BasicFamilyId;
  UINT32              FamilyId;
  UINT32              ModelId;
  UINT32              ExtendedModelId;

  if (Ratio == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AsmCpuid (EFI_CPUID_VERSION_INFO, &RegEax, NULL, NULL, NULL);

  //
  // The Extended Family ID needs to be examined only when the Family ID is 0FH.
  //
  BasicFamilyId = BitFieldRead32 (RegEax, 8, 11);
  FamilyId      = BasicFamilyId;
  if (BasicFamilyId == 0x0F) {
    FamilyId += BitFieldRead32 (RegEax, 20, 27);
  }

  //
  // The Extended Model ID needs to be examined only when the Family ID is 06H or 0FH.
  //
  ModelId = BitFieldRead32 (RegEax, 4, 7);
  if (BasicFamilyId == 0x06 || BasicFamilyId == 0x0f) {
    ExtendedModelId = BitFieldRead32 (RegEax, 16, 19);
    ModelId        += ExtendedModelId << 4;
  }

  switch (FamilyId) {
  case 0x0F:
    switch (ModelId) {
    case 0x03:
    case 0x04:
    case 0x06:
      //
      // EFI_MSR_EBC_FREQUENCY_ID[24:31]
      //
      *Ratio = (UINT32) BitFieldRead64 (AsmReadMsr64 (EFI_MSR_EBC_FREQUENCY_ID), 24, 31);
      break;
   
    default:
      return EFI_UNSUPPORTED;
    }
    break;

  case 0x06:
    switch (ModelId) {
    case 0x0F:
    case 0x16:
     //
      // EFI_MSR_IA32_PERF_STS[12:8]
      //
      *Ratio = (UINT32) BitFieldRead64 (AsmReadMsr64 (EFI_MSR_IA32_PERF_STS), 8, 12);
      break;

    case BLOOMFIELD_MODEL_ID:
    case LYNNFIELD_MODEL_ID:     
    case CLARKDALE_MODEL_ID:
    case SANDYBRIDGE_CLIENT_MODEL_ID:
    case SANDYBRIDGE_SERVER_MODEL_ID:
    case IVYBRIDGE_CLIENT_MODEL_ID:
    case IVYBRIDGE_SERVER_MODEL_ID:

      //
      // MSR_PLATFORM_INFO[15:8]
      //
      *Ratio = (UINT32) BitFieldRead64 (AsmReadMsr64 (MSR_PLATFORM_INFO), 8, 15);
      break;

    default:
      return EFI_UNSUPPORTED;
    }
    break;

  default:
    return EFI_UNSUPPORTED;
  }
  return EFI_SUCCESS;
}

/**
  Returns the actual CPU core frequency in MHz.

  This function returns the actual CPU core frequency in MHz.

  @param   Frequency              Pointer to the CPU core frequency.
  
  @retval  EFI_SUCCESS            The frequency is returned successfully.
  @retval  EFI_INVALID_PARAMETER  Frequency is NULL.            

**/
EFI_STATUS
GetActualFrequency (
  OUT UINT64                        *Frequency
  )
{
  UINT64  BeginValue;
  UINT64  EndValue;

  if (Frequency == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Read timestamp counter before and after delay of 100 microseconds
  //
  BeginValue = AsmReadTsc ();
  MicroSecondDelay (100);
  EndValue   = AsmReadTsc ();

  //
  // Calculate the actual frequency
  //
  *Frequency  = DivU64x32Remainder (EndValue - BeginValue, 100, NULL);

  return EFI_SUCCESS;
}

/**
  Converts the actual frequency in MHz to the standard frequency in MHz.

  This function converts the actual frequency in MHz to the standard frequency in MHz.

  @param   Actual              Actual CPU core frequency.
  @param   Ratio               CPU core to processor bus frequency ratio.
  @param   Standard            Standard CPU core frequency.

**/
VOID
Actual2StandardFrequency (
  IN  UINT64     Actual,
  IN  UINT32     Ratio,
  OUT UINT64     *Standard
  )
{
  UINT64  Inflated;
  UINT64  Temp;

  //
  // Increase the Actual frequency a little, compute its
  // corresponding FSB frequency, and then discard the small fractions of it, to
  // get a clean, standard FSB frequency value.
  //
  // Increase Actual by 3%, since the error of actual frequency is within 1%.
  //
  Inflated = Actual + RShiftU64 (Actual, 5);

  //
  // The standard Fsb are: 100(300/3), 133(400/3), 166(500/3), 200(600/3), ...
  // so if we are able to calculate the digit(3, 4, 5, 6, etc) at the hundred's
  // position, the standard frequency can be calculated.
  //
  //
  // So multiple the Inflated frequency by 3, divide it with Ratio, and divide
  // it by 100, in order to get an integer (3, 4, 5, 6, ... respectively).
  //
  Temp = DivU64x32 (DivU64x32 (MultU64x32 (Inflated, 3), Ratio), 100);

  //
  // After getting the digit, convert it back to the Cpu frequency
  //
  *Standard = DivU64x32 (MultU64x32 (MultU64x32 (Temp, 100), Ratio), 3);
}

