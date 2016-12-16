/** @file
  This is a simple shell application

  Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <PiDxe.h>
#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Guid/FileInfo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/EfiShellInterface.h>
#include <Protocol/EfiShellEnvironment2.h>
#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellParameters.h>
#include <ShellBase.h>
#include <Protocol/EfiShell.h>
#include <Library/ShellLib.h>
#include <Library/PathLib.h>
#include <Library/FileHandleLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/MpService.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/HandleParsingLib.h>






UINT32                              gMicroCodeAddress;
CHAR16* 				            gBuffer=NULL;
EFI_MP_SERVICES_PROTOCOL  *gMpService;
STATIC SPIN_LOCK                     CpuSpinLock;
CHAR16 	  pLogBuffer[512];
CONST CHAR16	            LogFileName[256]={0};
CONST CHAR16                AppPath[256] = {0};
#define EFI_MSR_IA32_BIOS_SIGN_ID             0x8B
#define EFI_MSR_IA32_BIOS_UPDT_TRIG   0x79
#define EFI_CPUID_VERSION_INFO      0x1
#define VCPU_MICROCODE_UPDATE_SIGNATURE   0x53415252
typedef struct  _VCPU_MICROCODE_UPDATE_HEADER{
     UINT32   Signature;
     UINT32   UpdateResion;
     UINT16   Year;
     UINT8     Day;
     UINT8     Month;
     UINT32   ProcessorId;
     UINT32   Checksum;           //checksum of the entire microcode update file excluding the header
     UINT32   LoaderRevision;
     UINT32   Reserved1;
     UINT32   DataSize;
     UINT32   TotalSize;
     UINT32   Reserved2;
     UINT32   Reserved3;
     UINT32   Reserved4;
}VCPU_MICROCODE_UPDATE_HEADER;
EFI_STATUS
WriteLogFileLine(
);

BOOLEAN 
UpdateMicroCode (
  VOID
  )
{
  UINT32  McAddr;
  UINT64  MsrVal;
  UINT32  RegEax;
  McAddr = gMicroCodeAddress;
  if(McAddr){
    AsmWriteMsr64(EFI_MSR_IA32_BIOS_SIGN_ID, 0);
    AsmWriteMsr64(EFI_MSR_IA32_BIOS_UPDT_TRIG, McAddr);
    ASSERT((AsmReadMsr64(0x1205)&0xFF) == 0x01); // Insure Ucode update success!
    if((AsmReadMsr64(0x1205)&0xFF) == 0x01) {
	    MsrVal = AsmReadMsr64(0x1A0);
		MsrVal |= 0x110000;
		AsmWriteMsr64(0x1A0, MsrVal);
		AsmCpuid(EFI_CPUID_VERSION_INFO, &RegEax, NULL, NULL, NULL);
        if(RegEax >= 0x6F8 && RegEax < 0x6FE) {
		    MsrVal = AsmReadMsr64(0x120e);
			MsrVal |= 0xB0;
		    AsmWriteMsr64(0x120e, MsrVal);
        }
		return TRUE;
    }
	return FALSE;
  }		
  return FALSE;
}
VOID 
EFIAPI
CpuMission (
  IN OUT VOID  *Buffer
  )
{

 UINTN ProcessorNumber = 0;
 BOOLEAN  LoadStatus = FALSE;
 CHAR16   Temp[512];
 AcquireSpinLock(&CpuSpinLock);
 gMpService->WhoAmI(gMpService,&ProcessorNumber);
 UnicodeSPrint(Temp,sizeof(Temp),L"CPU%d:Load %s:",ProcessorNumber,Buffer);
 StrCat(pLogBuffer,Temp);
 //Print(L"%s\n",Temp);
 LoadStatus = UpdateMicroCode();
 if(LoadStatus==TRUE){
  StrCat(pLogBuffer,L"Success!");
 // Print(L"Success!\n");
 }
 else{
  StrCat(pLogBuffer,L"Failed!");
 // Print(L"Failed!\n");
 }
 ReleaseSpinLock(&CpuSpinLock);
}

VCPU_MICROCODE_UPDATE_HEADER*
IsProperCpuMicroCode(
)
{
  VCPU_MICROCODE_UPDATE_HEADER   *EntryPoint;
  VCPU_MICROCODE_UPDATE_HEADER   *NewEntryPoint;
  UINT32                         RegEax;
  AsmCpuid(1, &RegEax, NULL, NULL, NULL); // this will cause HAPS hang  sometimes?
  EntryPoint = (VCPU_MICROCODE_UPDATE_HEADER*)gBuffer;
  NewEntryPoint = (VCPU_MICROCODE_UPDATE_HEADER*)((UINT8*)EntryPoint+528);
  //Via Old Format MicroCode
  if(EntryPoint->Signature==VCPU_MICROCODE_UPDATE_SIGNATURE){	
    }
  //Via New Format MicroCode
  else if(NewEntryPoint->Signature==VCPU_MICROCODE_UPDATE_SIGNATURE){
  	EntryPoint = NewEntryPoint;
  }
  //Not a Valid MicroCode
  else{
  	return NULL;
  }
  if (EntryPoint->Signature == VCPU_MICROCODE_UPDATE_SIGNATURE &&
        EntryPoint->ProcessorId == RegEax &&
        EntryPoint->TotalSize != 0){
	  return EntryPoint;
  }
  return NULL;
}
BOOLEAN
ReadMicroCode(CHAR16* FileName)
{
  EFI_STATUS                Status;
  SHELL_FILE_HANDLE         ShellFileHandle;
  UINTN                     FileSize=0;
  VCPU_MICROCODE_UPDATE_HEADER   *EntryPoint;
  Status=gEfiShellProtocol->OpenFileByName(FileName,&ShellFileHandle,EFI_FILE_MODE_READ);
  if (EFI_ERROR(Status) || ShellFileHandle == NULL) {
    DEBUG((EFI_D_ERROR,"OpenFileByName Error\n"));
    return FALSE;
  }
  Status=gEfiShellProtocol->GetFileSize(ShellFileHandle,&FileSize);
  gBuffer = AllocateZeroPool(FileSize);
  if(gBuffer==NULL){
  DEBUG((EFI_D_ERROR,"Alloc gBuffer File fail\n"));
  return FALSE;
  }
  Status = FileHandleRead(ShellFileHandle,&FileSize,gBuffer);
  if (EFI_ERROR(Status) && (gBuffer != NULL)) {
  	DEBUG((EFI_D_ERROR,"File_Read_error\n"));
    FreePool(gBuffer);
    gBuffer = NULL;
	return FALSE;
  }
  EntryPoint = IsProperCpuMicroCode();
  if(EntryPoint!=NULL){
  	gMicroCodeAddress = (UINT32)(UINTN)(EntryPoint+1);
	return TRUE;
  }
  return FALSE;
}
EFI_STATUS
CreateLogFile(
VOID)
{
  EFI_STATUS                Status;
  SHELL_FILE_HANDLE         ShellFileHandle;
  StrCpy((CHAR16*)LogFileName,(CHAR16*)(gEfiShellProtocol->GetCurDir(NULL)));
  StrCpy((CHAR16*)AppPath,(CHAR16*)LogFileName);
  StrCat((CHAR16*)LogFileName,L"test.log");
  //Print(L"LogFileName:%s\n",LogFileName);
  Status=gEfiShellProtocol->OpenFileByName(LogFileName,&ShellFileHandle,EFI_FILE_MODE_READ);
  if(!EFI_ERROR(Status)){
  	DEBUG((EFI_D_ERROR,"test.log has existed,delete\n"));
  	Status=gEfiShellProtocol->CloseFile(ShellFileHandle);
  	Status=gEfiShellProtocol->DeleteFileByName(LogFileName);
	if(EFI_ERROR(Status))
	  DEBUG((EFI_D_ERROR,"Can't delete existed test.log\n"));	
  }
  Status=gEfiShellProtocol->OpenFileByName(LogFileName,&ShellFileHandle,EFI_FILE_MODE_CREATE|EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE);
  if (EFI_ERROR(Status) || ShellFileHandle == NULL) {
  	Print(L"Can't create test.log\n");
    DEBUG((EFI_D_ERROR,"Can't create test.log\n"));
    return EFI_DEVICE_ERROR;
  }
  else{
  	 Status=gEfiShellProtocol->CloseFile(ShellFileHandle);
	 if (EFI_ERROR(Status)){
	 	DEBUG((EFI_D_ERROR,"Close test.log Error\n"));
		return EFI_DEVICE_ERROR;
	 }
  }
  return EFI_SUCCESS;
}
EFI_STATUS
WriteLogFileLine(
)
{
  EFI_STATUS                Status;
  SHELL_FILE_HANDLE         ShellFileHandle;
  UINT64                    FileSize;
  Status=gEfiShellProtocol->OpenFileByName(LogFileName,&ShellFileHandle,EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE);
  if(EFI_ERROR(Status)){
  	DEBUG((EFI_D_ERROR,"open test.log failed\n"));
	return EFI_LOAD_ERROR;

  }
  gEfiShellProtocol->GetFileSize(ShellFileHandle,&FileSize);
  gEfiShellProtocol->SetFilePosition(ShellFileHandle,FileSize);
  FileHandleWriteLine(ShellFileHandle,pLogBuffer);
  Status=gEfiShellProtocol->CloseFile(ShellFileHandle);
  if (EFI_ERROR(Status)){
	 DEBUG((EFI_D_ERROR,"Close test.log Error\n"));
	 return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  CONST CHAR16*	            CurDir;
  EFI_SHELL_FILE_INFO       *ListHead=NULL;
  EFI_SHELL_FILE_INFO       *Node; 
  CHAR16*                   pTemp;
  UINTN          NumberOfProcessors;
  UINTN          NumberOfEnabledProcessors=0;
  UINT32         RegEax=0;
  LIST_ENTRY                *Package;
  CHAR16                    *ProblemParam;
  CONST CHAR16              *BinPath;
  CONST CHAR16              *LoopCountString;
  UINTN                      LoopCount;
  BOOLEAN                    LoopForEver=FALSE;
  UINT32                     i = 1;
  BOOLEAN                    FindMicro = FALSE;
  //Initialize Shell
  Status = ShellInitialize();
  //Create Log File to Record run result
  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);
  InitializeSpinLock(&CpuSpinLock);
  CreateLogFile();
  //Locate MpService
  Status = gBS->LocateProtocol (
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  &gMpService
                  );
  ASSERT_EFI_ERROR(Status);
  if(gMpService) {
    Status = gMpService->GetNumberOfProcessors (gMpService, &NumberOfProcessors, &NumberOfEnabledProcessors);
    if(!EFI_ERROR(Status))
	  UnicodeSPrint(pLogBuffer,sizeof(pLogBuffer),L"CPU Information:");
	  Print(L"%s\n",pLogBuffer);
	  WriteLogFileLine();
	  //Record CPU Information
	  AsmCpuid(1, &RegEax, NULL, NULL, NULL);
	  UnicodeSPrint(pLogBuffer,sizeof(pLogBuffer),L"CPU FMS: %x",RegEax);
	  Print(L"%s\n",pLogBuffer);
	  WriteLogFileLine();
	  UnicodeSPrint(pLogBuffer,sizeof(pLogBuffer),L"%d processors found, %d enabled!",NumberOfProcessors,NumberOfEnabledProcessors);
	  Print(L"%s\n",pLogBuffer);
	  WriteLogFileLine();
  } 
  //Parse CommandLine to Get Microcode Bin Path and LoopCount
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  BinPath = ShellCommandLineGetRawValue(Package, 1);
  LoopCountString= ShellCommandLineGetRawValue(Package, 2);
  if(ShellCommandLineGetCount(Package)<2){
  Print(L"Too Few Argumet\n");
  Print(L"patch [micorcode path] loopcount\n");
  Print(L"path use relative path ,if path is current,[path]can Omissed\n");
  Print(L"loopcount=0 means: loop forever\n");
  return EFI_SUCCESS;
  }
  ShellConvertStringToUint64(LoopCountString, &LoopCount, TRUE, FALSE);
  if(LoopCount==0){
  	LoopForEver = TRUE;
  }
  CurDir = AllocateZeroPool(256);
  if((ShellCommandLineGetCount(Package)==0x3)&&(StrCmp((CHAR16*)BinPath,L"..")!=0)){
  	StrCpy((CHAR16*)CurDir,(CHAR16*)gEfiShellProtocol->GetCurDir(NULL));
    BinPath =(CHAR16*)PathCleanUpDirectories((CHAR16*)BinPath);
    StrCat((CHAR16*)CurDir,(CHAR16*)BinPath);
    gEfiShellProtocol->SetCurDir(NULL,CurDir);
  }
  //Scan File in the CurDir
  StrCpy((CHAR16*)CurDir,(CHAR16*)gEfiShellProtocol->GetCurDir(NULL));
  //Print(L"%s\n",CurDir);
  Status = ShellOpenFileMetaArg(L"*", EFI_FILE_MODE_READ, &ListHead);
  if (EFI_ERROR(Status)) {
  	DEBUG((EFI_D_ERROR,"ShellOpenFileMetaArg_Error\n"));
    return (SHELL_DEVICE_ERROR);
  }
  if (ListHead == NULL || IsListEmpty(&ListHead->Link)) {
  DEBUG((EFI_D_ERROR,"No Files\n"));
  } 
  else
  {
    UnicodeSPrint(pLogBuffer,sizeof(pLogBuffer),L"Start to Test:");
	Print(L"%s\n",pLogBuffer);
	WriteLogFileLine();
	UnicodeSPrint(pLogBuffer,sizeof(pLogBuffer),L"-----------------------");
	Print(L"%s\n",pLogBuffer);
	WriteLogFileLine();
    do{
		UnicodeSPrint(pLogBuffer,sizeof(pLogBuffer),L"Loop Count:%d",i);
	    Print(L"%s\n",pLogBuffer);
	    WriteLogFileLine();
		for ( Node = (EFI_SHELL_FILE_INFO*)GetFirstNode(&ListHead->Link)
                ; !IsNull(&ListHead->Link, &Node->Link)
                ; Node = (EFI_SHELL_FILE_INFO*)GetNextNode(&ListHead->Link, &Node->Link)
               )
        {
           //Not a Directory
	      if (FileHandleIsDirectory(Node->Handle) != EFI_SUCCESS)
	     {
	       pTemp = (CHAR16*)Node->FileName;
		   UnicodeSPrint(pLogBuffer,sizeof(pLogBuffer),L"File:%s",pTemp);
		   Print(L"%s\n",pLogBuffer);
		   WriteLogFileLine();
		   //Read a MicroCode bin into Memory
		   if(ReadMicroCode(pTemp)==TRUE){
		   	  FindMicro = TRUE;
		      //BSP Update MicroCode
		      StrCpy(pLogBuffer,L"\0");
		      CpuMission(pTemp);
			  Print(L"%s\n",pLogBuffer);
		      WriteLogFileLine();
		      StrCpy(pLogBuffer,L"\0");
		      //APs Update MicroCode
             if(gMpService) {
              Status = gMpService->StartupAllAPs (gMpService, CpuMission, TRUE, NULL, 0, pTemp, NULL);
              Print(L"%s\n",pLogBuffer);
			  WriteLogFileLine();
              if(EFI_ERROR(Status)) {
	           DEBUG ((EFI_D_ERROR, "Start AP fail!\n")); 
               NumberOfEnabledProcessors = 1;
              }
		      else{
  	           FreePool(gBuffer);
               gBuffer = NULL;
		      }	  
            }
	      }	
	  }
    }
	if(!LoopForEver)
   	LoopCount--;
	i++;
	if(!FindMicro){
	 UnicodeSPrint(pLogBuffer,sizeof(pLogBuffer),L"No Proper MicroCode Bin File");
	 Print(L"%s\n",pLogBuffer);
	 WriteLogFileLine();
	 return EFI_SUCCESS;
	}
   }while(LoopCount>0||LoopForEver);
   
  }
  gEfiShellProtocol->SetCurDir(NULL,AppPath);
  return EFI_SUCCESS;
}
