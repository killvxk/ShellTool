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
#include <Library/DebugLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
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


typedef struct{
  UINT32      StatusFlag;
  UINT32      Signature;
  UINT32      FeatureFlags;
  UINT64      ApicId;
  UINT8       ApicVersion;
} CPU_INFO;


typedef struct{
  UINT16      OffSet_15_0;
  UINT16      CsSeg;
  UINT16      CtrlBits;
  UINT16      OffSet_31_16;
  UINT32      OffSet_63_32;
  UINT32      Reserved;
}IDT_TABLE;   //64-Bit IDT Gate Descriptors

    
CPU_INFO     *gCpuInfoEntry = NULL;
UINT32        gMSR          = 0;
UINT64        gMSRValue     = 0;
UINT64       *gValue        = 0;
IDT_TABLE     gExceptionGate[256];


void MyHandlerUD(void)
{
    Print(L"[#UD]: Invalid Opcode Exception!\n");
    while(1);
}

void MyHandlerGP(void)
{
    Print(L"[#GP]:General Protection Exception!\n");
    while(1);
}

void UpdateIdtEntry(
    IN IA32_DESCRIPTOR       *Idtr,
    IN UINT32                 Vecter,
    IN UINT64                 Handler
    )
{
  IDT_TABLE   *ExceptionGate;

  ExceptionGate = (IDT_TABLE *)((*Idtr).Base + Vecter * 16);

  //Backup IDT Entey
  gExceptionGate[Vecter].OffSet_15_0  = (*ExceptionGate).OffSet_15_0;
  gExceptionGate[Vecter].OffSet_31_16 = (*ExceptionGate).OffSet_15_0;
  gExceptionGate[Vecter].OffSet_63_32 = (*ExceptionGate).OffSet_63_32;

  //Change IDT Entry
  //(*ExceptionGate).CtrlBits = 0x8f00;
  (*ExceptionGate).OffSet_15_0  = Handler & 0xffff;
  (*ExceptionGate).OffSet_31_16 = (Handler >> 16) & 0xffff;
  (*ExceptionGate).OffSet_63_32 = (Handler >> 32) & 0xffffffff;
}


void RecoveryIdtEntry(
    IN IA32_DESCRIPTOR       *Idtr,
    IN UINT32                 Vecter,
    IN IDT_TABLE              IdtTable
    )
{
  IDT_TABLE   *ExceptionGate;

  ExceptionGate = (IDT_TABLE *)((*Idtr).Base + Vecter * 16);

  //Change IDT Entry
  //(*ExceptionGate).CtrlBits = 0x8f00;
  (*ExceptionGate).OffSet_15_0  = IdtTable.OffSet_15_0;
  (*ExceptionGate).OffSet_31_16 = IdtTable.OffSet_31_16;
  (*ExceptionGate).OffSet_63_32 = IdtTable.OffSet_63_32;
}


VOID
UpdateIDT()
{

    IA32_DESCRIPTOR      Idtr;

    //Read the IDTR 
    AsmReadIdtr(&Idtr);

    //Change IDT Entry
    UpdateIdtEntry(&Idtr, 6,  (UINT64)MyHandlerUD);
    UpdateIdtEntry(&Idtr, 13, (UINT64)MyHandlerGP);
        
    //Update the IDTR 
    AsmWriteIdtr(&Idtr);
}


VOID
RecoveryIDT()
{

    IA32_DESCRIPTOR      Idtr;
    
    //Read the IDTR 
    AsmReadIdtr(&Idtr);

    //Recovery IDT Entry
    RecoveryIdtEntry(&Idtr, 6,  gExceptionGate[6]);
    RecoveryIdtEntry(&Idtr, 13, gExceptionGate[13]);
        
    //Update the IDTR 
    AsmWriteIdtr(&Idtr);
}



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


EFI_STATUS 
CollectCpuInfo(
  IN EFI_MP_SERVICES_PROTOCOL        *MpService
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



VOID
ApReadMsr (
  IN  VOID  *Buffer
  )
{

  UINTN                     ProcessorIndex;

  ProcessorIndex = (UINTN)Buffer;
  
  gValue[ProcessorIndex] = AsmReadMsr64 (gMSR);
}


VOID
ApWriteMsr (
  IN  VOID  *Buffer
  )
{
  UINTN                     ProcessorIndex;

  ProcessorIndex = (UINTN)Buffer;

  AsmWriteMsr64 (gMSR, gMSRValue);

}


EFI_STATUS 
ReadMsrAll(
  IN EFI_MP_SERVICES_PROTOCOL        *MpService
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
      ApReadMsr ((VOID *)ProcessorIndex);
    } else {
      Status = MpService->StartupThisAP (
                            MpService, 
                            ApReadMsr, 
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
WriteMsrAll(
  IN EFI_MP_SERVICES_PROTOCOL        *MpService
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
      ApWriteMsr((VOID *)ProcessorIndex);
    } else {
      Status = MpService->StartupThisAP (
                            MpService, 
                            ApWriteMsr, 
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
  Print the CPU ID INFO.
**/
VOID
PrintBinary(
  IN UINT64      Num
  )
{
    INT8       i;

    Print(L"[63-0]: ");
    for(i=63; i>=0; i--)
    {
        if((Num>>i) & BIT0)
        {
	        Print(L"1");
	    }else{
		    Print(L"0");
	    }
	    if(i%8==0)
	    {
		    Print(L" ");
		}
    }
    Print(L"\n");
}


#if 0
/**
  Print the CPU ID INFO.
**/
VOID
PrintBinary(
  IN UINT64      Num
  )
{
    INT8       i;

    Print(L"[ ");
    for(i=63; i>=0; i--)
    {
        if((Num>>i) & BIT0)
        {
	        Print(L"BIT%d ", i);
	    }
	}
	Print(L"]\n");
}
#endif


/**
  Print the CPU ID INFO.
**/
EFI_STATUS
PrintMsrAll(
  IN EFI_MP_SERVICES_PROTOCOL        *MpService
  )
{
    EFI_STATUS             Status;
    UINTN                  NumberOfProcessors = 1;
    UINTN                  NumberOfEnabledProcessors;
    UINTN                  ProcessorIndex = 0;

    Status = MpService->GetNumberOfProcessors (
                          MpService, 
                          &NumberOfProcessors, 
                          &NumberOfEnabledProcessors
                          );
    ASSERT_EFI_ERROR(Status);


    Print(L"[MSR 0x%lx]:\n", gMSR);
    for (ProcessorIndex = 0; ProcessorIndex < NumberOfProcessors; ProcessorIndex++) 
    {
        Print(L"Core[%02d]: MSR[0x%x] = %lx,  ", ProcessorIndex, gMSR, gValue[ProcessorIndex]);
        PrintBinary(gValue[ProcessorIndex]);
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
  IN UINTN       argc,
  IN CHAR16    **argv
  )
{
    EFI_STATUS                       Status;
    CHAR8                            Argv[5];
    EFI_MP_SERVICES_PROTOCOL        *MpService;


    if( argc==3 || argc==4 || argc==5 )
    {
        UpdateIDT();
        UnicodeStrToAsciiStr(argv[1],&Argv[1]);
        if(argc==3 && AsciiStriCmp(&Argv[1],"R")==0 ){
	        gMSR = (UINT32)StrHexToUintn(argv[2]);
	        ApReadMsr (0);
	        Print(L"MSR[0x%x] = 0x%lx,  ", gMSR, gValue[0]);
	        PrintBinary(gValue[0]);
        }else if(argc==4){
            UnicodeStrToAsciiStr(argv[3],&Argv[3]);
            if(AsciiStriCmp(&Argv[1],"R")==0 && AsciiStriCmp(&Argv[3],"ALL")==0){
                // Find the MP Protocol. This is an MP platform, so MP protocol must be there.
	        	Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **) &MpService);
	        	if(EFI_ERROR (Status)){
	        	      Print(L"[ERROR]: Locate the MP Services Protocol Fail!\n");
	        	      return EFI_UNSUPPORTED;
	        	}
	        
	            Status = CollectCpuInfo(MpService);
	            ASSERT_EFI_ERROR(Status); 
                
		        gMSR   = (UINT32)StrHexToUintn(argv[2]);
		        Status = ReadMsrAll(MpService);
		        ASSERT_EFI_ERROR(Status); 
		        PrintMsrAll(MpService);
	        }else if(AsciiStriCmp(&Argv[1],"W")==0 ){ 
	        	gMSR        = (UINT32)StrHexToUintn(argv[2]);
	        	gMSRValue   = (UINT32)StrHexToUintn(argv[3]);
	        	ApWriteMsr (0);
                ApReadMsr (0);
	        	Print(L"MSR[0x%x] = 0x%lx,  ", gMSR, gValue[0]);
	        	PrintBinary(gValue[0]);
            }else{
		        Print(L"[INPUT]: RWMsr.efi R REG [ALL]\n");
		        Print(L"[INPUT]: RWMsr.efi W REG VALUE [ALL]\n");
            }
        }else if(argc==5 && AsciiStriCmp(&Argv[1],"W")==0 ){
            UnicodeStrToAsciiStr(argv[4],&Argv[4]);
            if(AsciiStriCmp(&Argv[4],"ALL")==0){
                // Find the MP Protocol. This is an MP platform, so MP protocol must be there.
	        	Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **) &MpService);
	        	if(EFI_ERROR (Status)){
	        	      Print(L"[ERROR]: Locate the MP Services Protocol Fail!\n");
	        	      return EFI_UNSUPPORTED;
	        	}
	        
	            Status = CollectCpuInfo(MpService);
	            ASSERT_EFI_ERROR(Status); 
                
	    		gMSR        = (UINT32)StrHexToUintn(argv[2]);
		        gMSRValue   = (UINT32)StrHexToUintn(argv[3]);
		    	Status = WriteMsrAll(MpService);
		        ASSERT_EFI_ERROR(Status); 
		        Status = ReadMsrAll(MpService);
		        ASSERT_EFI_ERROR(Status);
		        PrintMsrAll(MpService);
            }else{
		        Print(L"[INPUT]: RWMsr.efi R REG [ALL]\n");
		        Print(L"[INPUT]: RWMsr.efi W REG VALUE [ALL]\n");
        	}
        }else{
	        Print(L"[INPUT]: RWMsr.efi R REG [ALL]\n");
	        Print(L"[INPUT]: RWMsr.efi W REG VALUE [ALL]\n");
        }
        RecoveryIDT();
    }else{
        Print(L"[INPUT]: RWMsr.efi R REG [ALL]\n");
        Print(L"[INPUT]: RWMsr.efi W REG VALUE [ALL]\n");
    }
    
    return EFI_SUCCESS;
}
