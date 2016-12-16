/** @file
  This is a test application that demonstrates how to use the C-style entry point
  for a shell application.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <PiDxe.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/MpService.h>



#define MSR_EXT_XAPIC_LOGICAL_APIC_ID   0x802
#define MSR_EXT_XAPIC_VERSION           0x803
#define MSR_XAPIC_BASE                  0x1B
#define APIC_VERSION_REGISTER           0x30

#define LOCAL_APIC_BASE                 0xFEE00000
#define IO_APIC_BASE_ADDRESS_BOTTOM     0xFEC00000
#define IO_APIC_BASE_ADDRESS_TOP        0xFED00000


typedef struct _CPU_INFO {
  UINT32      StatusFlag;
  UINT32      Signature;
  UINT32      FeatureFlags;
  UINT64      ApicId;
  UINT8       ApicVersion;
} CPU_INFO;

typedef struct _CPU_ID {
  UINT32      RegEax;
  UINT32      RegEbx;
  UINT32      RegEcx;
  UINT32      RegEdx;
} CPU_ID;

CPU_INFO     *gCpuInfoEntry = NULL;
CPU_ID       *gCpuIdEntry   = NULL;
UINT32        gCpuid        = 0x0;



VOID
ApGetCpuInfo (
  IN  VOID  *Buffer
  )
{
  UINT32                    RegEax;
  UINT32                    RegEdx;
  UINTN                     ProcessorIndex;

  ProcessorIndex = (UINTN)Buffer;

  if (((AsmReadMsr64 (MSR_XAPIC_BASE)) & (BIT11 + BIT10)) == (BIT11 + BIT10)) {
    gCpuInfoEntry[ProcessorIndex].ApicVersion = (UINT8)AsmReadMsr64 (MSR_EXT_XAPIC_VERSION);
  } else {
    gCpuInfoEntry[ProcessorIndex].ApicVersion = (UINT8)(*(volatile UINT32*)(UINTN)(LOCAL_APIC_BASE + APIC_VERSION_REGISTER));
  }

  AsmCpuid (1, &RegEax, NULL, NULL, &RegEdx);
  gCpuInfoEntry[ProcessorIndex].Signature    = RegEax;
  gCpuInfoEntry[ProcessorIndex].FeatureFlags = RegEdx;

}


VOID
ApGetCpuId (
  IN  VOID  *Buffer
  )
{
  UINT32                    RegEax;
  UINT32                    RegEbx;
  UINT32                    RegEcx;
  UINT32                    RegEdx;
  UINTN                     ProcessorIndex;

  ProcessorIndex = (UINTN)Buffer;

  AsmCpuid (gCpuid, &RegEax, &RegEbx, &RegEcx, &RegEdx);
  gCpuIdEntry[ProcessorIndex].RegEax  = RegEax;
  gCpuIdEntry[ProcessorIndex].RegEbx  = RegEdx;
  gCpuIdEntry[ProcessorIndex].RegEcx  = RegEcx;
  gCpuIdEntry[ProcessorIndex].RegEdx  = RegEcx; 
}




EFI_STATUS 
CollectCpuInfo (
  IN     EFI_MP_SERVICES_PROTOCOL  *MpService
  )
{
  EFI_STATUS                Status;
  EFI_PROCESSOR_INFORMATION ProcessorInfoBuffer;
  UINTN                     ProcessorIndex;
  UINTN                     NumberOfProcessors;
  UINTN                     NumberOfThreadsPerCore;
  UINTN                     NumberOfEnabledProcessors;
  BOOLEAN                   Finished;


  Status = MpService->GetNumberOfProcessors (
                        MpService, 
                        &NumberOfProcessors, 
                        &NumberOfEnabledProcessors
                        );
  ASSERT_EFI_ERROR(Status);

  if(gCpuInfoEntry==NULL){
    gCpuInfoEntry = AllocatePool(sizeof(CPU_INFO) * (NumberOfProcessors));
    ASSERT(gCpuInfoEntry!=NULL);
    ZeroMem(gCpuInfoEntry, sizeof(CPU_INFO) * (NumberOfProcessors));
  }
	
  NumberOfThreadsPerCore = 0;  
  for (ProcessorIndex = 0; ProcessorIndex < NumberOfProcessors; ProcessorIndex++) {
    Status = MpService->GetProcessorInfo (
                          MpService,
                          ProcessorIndex,
                          &ProcessorInfoBuffer
                          );
    ASSERT_EFI_ERROR (Status);
    
    //
    // Both of NumberOfThreads and NumberOfCores are 0 based
    //
    if (NumberOfThreadsPerCore < ProcessorInfoBuffer.Location.Thread + 1) {
      NumberOfThreadsPerCore = ProcessorInfoBuffer.Location.Thread + 1;
    }

    gCpuInfoEntry[ProcessorIndex].StatusFlag = ProcessorInfoBuffer.StatusFlag;
    gCpuInfoEntry[ProcessorIndex].ApicId     = ProcessorInfoBuffer.ProcessorId;

    if (ProcessorInfoBuffer.StatusFlag & PROCESSOR_AS_BSP_BIT) {
      ApGetCpuInfo ((VOID *)ProcessorIndex);
    } else {
      Status = MpService->StartupThisAP (
                            MpService, 
                            ApGetCpuInfo, 
                            ProcessorIndex, 
                            NULL, 
                            0, 
                            (VOID *)ProcessorIndex,
                            &Finished
                            );
      ASSERT_EFI_ERROR(Status);
    }
  }

  return EFI_SUCCESS;
}



EFI_STATUS 
CollectCpuId (
  IN     EFI_MP_SERVICES_PROTOCOL  *MpService
  )
{
  EFI_STATUS                Status;
  UINTN                     ProcessorIndex;
  UINTN                     NumberOfProcessors;
  UINTN                     NumberOfEnabledProcessors;
  BOOLEAN                   Finished;


  Status = MpService->GetNumberOfProcessors (
                        MpService, 
                        &NumberOfProcessors, 
                        &NumberOfEnabledProcessors
                        );
  ASSERT_EFI_ERROR(Status);

  
  for (ProcessorIndex = 0; ProcessorIndex < NumberOfProcessors; ProcessorIndex++) {
	  if (gCpuInfoEntry[ProcessorIndex].StatusFlag & PROCESSOR_AS_BSP_BIT) {
	      ApGetCpuId ((VOID *)ProcessorIndex);
	  } else {
		  Status = MpService->StartupThisAP (
		                        MpService, 
		                        ApGetCpuId, 
		                        ProcessorIndex, 
		                        NULL, 
		                        0, 
		                        (VOID *)ProcessorIndex,
		                        &Finished
		                        );
		  ASSERT_EFI_ERROR(Status);
	  }
  }

  return EFI_SUCCESS;
}



/**
  Print the CPU INFO.
**/
EFI_STATUS
PrintCpuInfo (
    IN     EFI_MP_SERVICES_PROTOCOL  *MpService
    )
{
    EFI_STATUS             Status;
    UINTN                  NumberOfProcessors;
    UINTN                  NumberOfEnabledProcessors;
    UINTN                  ProcessorIndex;

    Status = MpService->GetNumberOfProcessors (
                          MpService, 
                          &NumberOfProcessors, 
                          &NumberOfEnabledProcessors
                          );
    ASSERT_EFI_ERROR(Status);
    
    for (ProcessorIndex = 0; ProcessorIndex < NumberOfProcessors; ProcessorIndex++) {
        Print(L"Core[%02d].Signature    =  %x\n",    ProcessorIndex, gCpuInfoEntry[ProcessorIndex].Signature);
        Print(L"Core[%02d].StatusFlag   =  %x\n",    ProcessorIndex, gCpuInfoEntry[ProcessorIndex].StatusFlag);
        Print(L"Core[%02d].FeatureFlags =  %x\n",    ProcessorIndex, gCpuInfoEntry[ProcessorIndex].FeatureFlags);
        Print(L"Core[%02d].ApicId       =  %x\n",    ProcessorIndex, gCpuInfoEntry[ProcessorIndex].ApicId);
        Print(L"Core[%02d].ApicVersion  =  %x\n\n",  ProcessorIndex, gCpuInfoEntry[ProcessorIndex].ApicVersion);
    }  
    
    return EFI_SUCCESS;
}



/**
  Print the CPU ID INFO.
**/
EFI_STATUS
PrintCpuId (
    IN     EFI_MP_SERVICES_PROTOCOL  *MpService
    )
{
    EFI_STATUS             Status;
    UINTN                  NumberOfProcessors;
    UINTN                  NumberOfEnabledProcessors;
    UINTN                  ProcessorIndex;

    Status = MpService->GetNumberOfProcessors (
                          MpService, 
                          &NumberOfProcessors, 
                          &NumberOfEnabledProcessors
                          );
    ASSERT_EFI_ERROR(Status);

    if(gCpuIdEntry[0].RegEax == 0 && gCpuIdEntry[0].RegEbx == 0 && gCpuIdEntry[0].RegEcx == 0 && gCpuIdEntry[0].RegEdx == 0){
		Print(L"[CPUID 0x%x]: NOT SUPPORTED\n", gCpuid);
    }else{
        Print(L"[CPUID 0x%x]:\n", gCpuid);
	    for (ProcessorIndex = 0; ProcessorIndex < NumberOfProcessors; ProcessorIndex++) {
	        Print(L"Core[%02d]: RegEax:RegEbx:RegEcx:RegEdx = %x:%x:%x:%x\n", ProcessorIndex, gCpuIdEntry[ProcessorIndex].RegEax, gCpuIdEntry[ProcessorIndex].RegEbx, gCpuIdEntry[ProcessorIndex].RegEcx, gCpuIdEntry[ProcessorIndex].RegEdx);
	    }
    }
    return EFI_SUCCESS;
}




/**
  UEFI application entry point which has an interface similar to a
  standard C main function.

  The ShellCEntryLib library instance wrappers the actual UEFI application
  entry point and calls this ShellAppMain function.

  @param[in] Argc     The number of items in Argv.
  @param[in] Argv     Array of pointers to strings.

  @retval  0               The application exited normally.
  @retval  Other           An error occurred.

**/
INTN
EFIAPI
ShellAppMain (
  IN UINTN           argc,
  IN CHAR16        **argv
  )
{
  EFI_STATUS                           Status;
  EFI_MP_SERVICES_PROTOCOL            *MpService;


  // Find the MP Protocol. This is an MP platform, so MP protocol must be there.
  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **) &MpService);
  if(EFI_ERROR (Status)){
        Print(L"[ERROR]: Locate the MP Services Protocol Fail!\n");
        return EFI_UNSUPPORTED;
  }

  Status = CollectCpuInfo(MpService);
  ASSERT_EFI_ERROR(Status);  

  if(argc==1){
        Status = PrintCpuInfo(MpService);
		ASSERT_EFI_ERROR(Status); 
		Status = CollectCpuId(MpService);
        ASSERT_EFI_ERROR(Status);

        Status = PrintCpuId(MpService);
		ASSERT_EFI_ERROR(Status); 
  }else if(argc==2){
        gCpuid = (UINT32)StrHexToUintn(argv[1]);
        Status = CollectCpuId(MpService);
        ASSERT_EFI_ERROR(Status);

        Status = PrintCpuId(MpService);
		ASSERT_EFI_ERROR(Status);    
  }else
  {
        Print(L"[INPUT]: Cpuid.efi [CPUID]\n");
        return EFI_UNSUPPORTED;
  }
  
  return EFI_SUCCESS;
}
