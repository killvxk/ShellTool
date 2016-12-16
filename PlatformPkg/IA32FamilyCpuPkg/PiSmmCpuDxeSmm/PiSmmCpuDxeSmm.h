//
// This file contains an 'Intel Peripheral Driver' and is      
// licensed for Intel CPUs and chipsets under the terms of your
// license agreement with Intel or your vendor.  This file may 
// be modified by the user, subject to additional terms of the 
// license agreement                                           
//
/** @file
  Agent Module to load other modules to deploy SMM Entry Vector for X86 CPU.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

**/
#ifndef _CPU_PISMMCPUDXESMM_H_
#define _CPU_PISMMCPUDXESMM_H_
#include <PiSmm.h>
#include <Protocol/MpService.h>
#include <Protocol/SmmConfiguration.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmAccess2.h>
#include <Protocol/SmmCpuSaveState.h>
#include <Protocol/SmmReadyToLock.h>
#include <Guid/AcpiS3Context.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/SmmLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MtrrLib.h>
#include <Library/SocketLga775Lib.h>
#include <Library/SocketLga1156Lib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/HobLib.h>
#include <Library/LocalApicLib.h>
#include <Library/UefiCpuLib.h>
#include <SmmFeatures.h>

//
// Size of Task-State Segment defined in IA32 Manual
//
#define TSS_SIZE              104

//
// The size 0x20 must be bigger than
// the size of template code of SmmInit. Currently,
// the size of SmmInit requires the 0x16 Bytes buffer
// at least.
// 
#define BACK_BUF_SIZE  0x20

//
// Private structure for the SMM CPU module that is stored in DXE Runtime memory
// Contains the SMM Configuration Protocols that is produced.
// Contains a mix of DXE and SMM contents.  All the fields must be used properly.
//
#define SMM_CPU_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('s', 'c', 'p', 'u')

typedef struct {
  UINTN                           Signature;

  EFI_HANDLE                      SmmCpuHandle;

  UINT32                          SmrrBase;
  UINT32                          SmrrSize;
  UINT32                          ApicIds          [FixedPcdGet32 (PcdCpuMaxLogicalProcessorNumber)];
  UINTN                           SmBases          [FixedPcdGet32 (PcdCpuMaxLogicalProcessorNumber)];
  UINTN                           CpuSaveStateSize [FixedPcdGet32 (PcdCpuMaxLogicalProcessorNumber)];
  VOID                            *CpuSaveState    [FixedPcdGet32 (PcdCpuMaxLogicalProcessorNumber)];

  EFI_SMM_RESERVED_SMRAM_REGION   SmmReservedSmramRegion[1];
  EFI_SMM_ENTRY_CONTEXT           SmmCoreEntryContext;
  EFI_SMM_ENTRY_POINT             SmmCoreEntry;

  EFI_SMM_CONFIGURATION_PROTOCOL  SmmConfiguration;
} SMM_CPU_PRIVATE_DATA;

extern SMM_CPU_PRIVATE_DATA  *gSmmCpuPrivate;

//
// SMM CPU Protocol function prototypes.
//

/**
  Read information from the CPU save state.

  @param  This      EFI_SMM_CPU_PROTOCOL instance
  @param  Widthe    The number of bytes to read from the CPU save state.
  @param  Register  Specifies the CPU register to read form the save state.
  @param  CpuIndex  Specifies the zero-based index of the CPU save state
  @param  Buffer    Upon return, this holds the CPU register value read from the save state.

  @retval EFI_SUCCESS   The register was read from Save State
  @retval EFI_NOT_FOUND The register is not defined for the Save State of Processor  
  @retval EFI_INVALID_PARAMTER   This or Buffer is NULL.

**/
EFI_STATUS
EFIAPI
SmmReadSaveState (
  IN CONST EFI_SMM_CPU_PROTOCOL         *This,
  IN UINTN                              Width,
  IN EFI_SMM_SAVE_STATE_REGISTER        Register,
  IN UINTN                              CpuIndex,
  OUT VOID                              *Buffer
  );

/**
  Write data to the CPU save state.

  @param  This      EFI_SMM_CPU_PROTOCOL instance
  @param  Widthe    The number of bytes to read from the CPU save state.
  @param  Register  Specifies the CPU register to write to the save state.
  @param  CpuIndex  Specifies the zero-based index of the CPU save state
  @param  Buffer    Upon entry, this holds the new CPU register value.

  @retval EFI_SUCCESS   The register was written from Save State
  @retval EFI_NOT_FOUND The register is not defined for the Save State of Processor  
  @retval EFI_INVALID_PARAMTER   ProcessorIndex or Width is not correct

**/
EFI_STATUS
EFIAPI
SmmWriteSaveState (
  IN CONST EFI_SMM_CPU_PROTOCOL         *This,
  IN UINTN                              Width,
  IN EFI_SMM_SAVE_STATE_REGISTER        Register,
  IN UINTN                              CpuIndex,
  IN CONST VOID                         *Buffer
  );

///
/// Structure used to describe a range of registers
///
typedef struct {
  EFI_SMM_SAVE_STATE_REGISTER  Start;
  EFI_SMM_SAVE_STATE_REGISTER  End;
  UINTN                        Length;
} CPU_SMM_SAVE_STATE_REGISTER_RANGE;

///
/// Structure used to build a lookup table to retrieve the widths and offsets
/// associated with each supported EFI_SMM_SAVE_STATE_REGISTER value
///
typedef struct {
  UINT8   Width32;
  UINT8   Width64;
  UINT16  Offset32;
  UINT16  Offset64Lo;
  UINT16  Offset64Hi;
} CPU_SMM_SAVE_STATE_LOOKUP_ENTRY;

///
/// Structure used to build a lookup table for the IOMisc width information
///
typedef struct {
  UINT8                        Width;
  EFI_SMM_SAVE_STATE_IO_WIDTH  IoWidth;
} CPU_SMM_SAVE_STATE_IO_WIDTH;

///
/// Structure used to build a lookup table for the IOMisc data offset information
///
typedef struct {
  EFI_SMM_SAVE_STATE_IO_TYPE  IoType;
  UINT16                      IoDataOffset32;
  UINT16                      IoDataOffset64;
} CPU_SMM_SAVE_STATE_IO_TYPE;

//
//
//
typedef struct {
  UINT32                            Offset;
  UINT16                            Segment;
  UINT16                            Reserved;
} IA32_FAR_ADDRESS;

extern IA32_FAR_ADDRESS             gSmmJmpAddr;

extern CONST UINT8                  gcSmmInitTemplate[];
extern CONST UINT16                 gcSmmInitSize;
extern UINT32                       gSmmCr0;
extern UINT32                       gSmmCr3;
extern UINT32                       gSmmCr4;
extern UINTN                        gSmmInitStack;

/**
  Seamphore operation for all processor relocate SMMBase.
**/
VOID
EFIAPI
SmmRelocationSemaphoreComplete (
  VOID
  );

/**
  Hook return address of SMM Save State so that semaphore code
  can be executed immediately after AP exits SMM to indicate to
  the BSP that an AP has exited SMM after SMBASE relocation.

  @param RebasedFlag  Pointer to a flag. AP sets it to be TRUE after exit of SMM.
  @param CpuState     Pointer to the SMM Save State Map.
**/
VOID
SemaphoreHook (
  IN BOOLEAN                       *RebasedFlag,
  IN SOCKET_LGA_775_SMM_CPU_STATE  *CpuState
  );

///
/// The type of SMM CPU Information
///
typedef struct {
  SPIN_LOCK                         Busy;
  volatile EFI_AP_PROCEDURE         Procedure;
  volatile VOID                     *Parameter;
  volatile UINT32                   Run;
  volatile BOOLEAN                  Present;
} SMM_CPU_DATA_BLOCK;

typedef struct {
  SMM_CPU_DATA_BLOCK  CpuData[FixedPcdGet32 (PcdCpuMaxLogicalProcessorNumber)];
  volatile UINT32     Counter;
  volatile UINT32     BspIndex;
  volatile BOOLEAN    InsideSmm;
  volatile BOOLEAN    AllCpusInSync;
} SMM_DISPATCHER_UC_DATA;

extern CONST IA32_DESCRIPTOR               gcSmiGdtr;
extern CONST IA32_DESCRIPTOR               gcSmiIdtr;
extern UINT32                              gSmiCr3;
extern volatile UINTN                      gSmiStack;
extern CONST PROCESSOR_SMM_DESCRIPTOR      gcPsd;
extern volatile UINT8                      gcSmiHandlerTemplate[];
extern CONST UINT16                        gcSmiHandlerSize;
extern CONST UINT16                        gcSmiHandlerOffset;
extern UINT64                              gPhyMask;
extern SMM_DISPATCHER_UC_DATA              *mSmmUcData;
extern CPU_SMM_FEATURE_CONTEXT             gFeatureContext;


/**
  Create 4G PageTable in SMRAM.

  @param          ExtraPages       Additional page numbers besides for 4G memory
  @return         PageTable Address

**/
UINT32
Gen4GPageTable (
  IN      UINTN                     ExtraPages
  );


/**
  Initialize global data for MP synchronization.

  @param ImageHandle  File Image Handle.
  @param Stacks       Base address of SMI stack buffer for all processors.

**/
VOID
InitializeMpServiceData (
  IN EFI_HANDLE  ImageHandle,
  IN VOID        *Stacks
  );

/**
  Initialize Timer for Smm AP Sync.

**/
VOID
InitializeSmmTimer (
  VOID
  );

/**
  Start Timer for Smm AP Sync.

**/
UINT64
EFIAPI
StartSyncTimer (
  VOID
  );

/**
  Check if the Smm AP Sync timer is timeout.

  @param Timer  The start timer from the begin.

**/
BOOLEAN
EFIAPI
IsSyncTimerTimeout (
  IN      UINT64                    Timer
  );

/**
  Initialize IDT for SM mode.

**/
VOID
EFIAPI
InitializeIDT (
  VOID
  );

/**

  Register the SMM Foundation entry point.

  @param          This              Pointer to EFI_SMM_CONFIGURATION_PROTOCOL instance
  @param          SmmEntryPoint     SMM Foundation EntryPoint

  @retval         EFI_SUCCESS       Successfully to register SMM foundation entry point

**/
EFI_STATUS
EFIAPI
RegisterSmmEntry (
  IN CONST EFI_SMM_CONFIGURATION_PROTOCOL  *This,
  IN EFI_SMM_ENTRY_POINT                   SmmEntryPoint
  );

/**
  Create PageTable for SMM use.

  @return     PageTable Address

**/
UINT32
SmmInitPageTable (
  VOID
  );

/**
  Schedule a procedure to run on the specified CPU.

  @param   Procedure        The address of the procedure to run
  @param   CpuIndex         Target CPU number
  @param   ProcArguments    The parameter to pass to the procedure

  @retval   EFI_INVALID_PARAMETER    CpuNumber not valid
  @retval   EFI_INVALID_PARAMETER    CpuNumber specifying BSP
  @retval   EFI_INVALID_PARAMETER    The AP specified by CpuNumber did not enter SMM
  @retval   EFI_INVALID_PARAMETER    The AP specified by CpuNumber is busy
  @retval   EFI_SUCCESS - The procedure has been successfully scheduled

**/
EFI_STATUS
EFIAPI
SmmStartupThisAp (
  IN      EFI_AP_PROCEDURE          Procedure,
  IN      UINTN                     CpuIndex,
  IN OUT  VOID                      *ProcArguments OPTIONAL
  );

/**
  Initialize un-cacheable data.

**/
VOID
EFIAPI
InitializeUcData (
  VOID
  );

/**

  Find out SMRAM information including SMRR base, SMRR size and IEDBase.

  @param          SmrrBase          SMRR base
  @param          SmrrSize          SMRR size
  
**/
VOID
FindSmramInfo (
  OUT UINT32   *SmrrBase,
  OUT UINT32   *SmrrSize
  );

/**
  The function is invoked before SMBASE relocation in S3 path to restors CPU status.

  The function is invoked before SMBASE relocation in S3 path. It does first time microcode load 
  and restores MTRRs for both BSP and APs.

**/
VOID
EarlyInitializeCpu (
  VOID
  );

/**
  The function is invoked after SMBASE relocation in S3 path to restors CPU status.

  The function is invoked after SMBASE relocation in S3 path. It restores configuration according to 
  data saved by normal boot path for both BSP and APs.

**/
VOID
InitializeCpu (
  VOID
  );
#endif
