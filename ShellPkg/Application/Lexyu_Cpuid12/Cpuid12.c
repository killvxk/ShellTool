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
#include <Library/DxeServicesTableLib.h>
#include <Library/PciLib.h>
#include <Library/IoLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/HiiLib.h>

#include <Guid/ShellLibHiiGuid.h>

#include <Protocol/MpService.h>


#include <MpServicesOnFrameworkMpServicesThunk/MpServicesOnFrameworkMpServicesThunk.h>
#include <MpServicesOnFrameworkMpServicesThunk/MpServicesOnFrameworkMpServicesThunk.c>


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
UINT32        gCpuid        = 0x1;



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
  UINTN                     NumberOfEnabledProcessors;
  UINTN                     NumberOfProcessors;
  UINTN                     NumberOfThreadsPerCore;
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
  UINTN                     NumberOfEnabledProcessors;
  UINTN                     NumberOfProcessors;
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
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/


EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE                  ImageHandle,
  IN EFI_SYSTEM_TABLE           *SystemTable
  )
{
      EFI_STATUS                 Status          = SHELL_SUCCESS;
      EFI_MP_SERVICES_PROTOCOL  *MpService;
      EFI_HANDLE                 gCpuidHiiHandle = NULL;

      LIST_ENTRY                *Package;
      CHAR16                    *ProblemParam;
      SHELL_STATUS               ShellStatus     = SHELL_SUCCESS;
      CONST CHAR16              *CpuidString;
      UINT64                     Cpuid           = 0x0;


      //
      // initialize the shell lib (we must be in non-auto-init...)
      //
      Status = ShellInitialize();
      ASSERT_EFI_ERROR(Status);
      Status = CommandInit();
      ASSERT_EFI_ERROR(Status);
      
      
      gCpuidHiiHandle = HiiAddPackages (&gCpuidHiiGuid, gImageHandle, Cpuid12Strings, NULL);
      if (gCpuidHiiHandle == NULL) {
        return (EFI_DEVICE_ERROR);
      }

      //
      // parse the command line
      //
      Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
      if (EFI_ERROR(Status)) {
	      if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
	          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_GEN_PROBLEM), gCpuidHiiHandle, ProblemParam);
	          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_MANUAL), gCpuidHiiHandle);
	          FreePool(ProblemParam);
	          ShellStatus = SHELL_INVALID_PARAMETER;
	      }else{
	       	  ASSERT(FALSE);
	      }
      }else{
	      if (ShellCommandLineGetCount(Package) > 2) 
	      {
	          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_GEN_TOO_MANY), gCpuidHiiHandle);
	          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_MANUAL),  gCpuidHiiHandle);
	          ShellStatus = SHELL_INVALID_PARAMETER;
	      }else{
              // Find the MP Protocol. This is an MP platform, so MP protocol must be there.
		      Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **) &MpService);
		      if(EFI_ERROR (Status)){
		            Status = InitializeMpServicesProtocol(ImageHandle, SystemTable);
		            ASSERT_EFI_ERROR (Status);
		            Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **) &MpService);
		            if(EFI_ERROR (Status)){
     	            	Print(L"[ERROR]: Locate the MP Services Protocol Fail!\n");
     	                return EFI_UNSUPPORTED;
		      		}
		      }

		      Status = CollectCpuInfo(MpService);
			  ASSERT_EFI_ERROR (Status);

              if(ShellCommandLineGetCount(Package) ==1)
              {
    	     	  Status = PrintCpuInfo(MpService);
    	          ASSERT_EFI_ERROR (Status);
              }else{
		          //
		          // Parse the params
		          //
		          CpuidString = ShellCommandLineGetRawValue(Package, 1);
		    
		          if (CpuidString == NULL) {
		          	  Cpuid = 0x1;
		          }else{
		              if(!ShellIsHexOrDecimalNumber(CpuidString, TRUE, FALSE)) {
		                  ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_GEN_PROBLEM), gCpuidHiiHandle, CpuidString);
		                  ShellStatus = SHELL_INVALID_PARAMETER;
		              }
		              ShellConvertStringToUint64(CpuidString, &Cpuid, TRUE, FALSE);	
		          }

                  gCpuid=(UINT32) Cpuid;
		          if (ShellStatus == SHELL_SUCCESS){
				      Status = CollectCpuId(MpService);
				      ASSERT_EFI_ERROR (Status);
				      Status = PrintCpuId(MpService);
				      ASSERT_EFI_ERROR (Status);
		          }
              }
	      }
	  
	      ShellCommandLineFreeVarList (Package);
	  }

      return EFI_SUCCESS;
}

