/*++
Copyright (c) 2010 Intel Corporation. All rights reserved.
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  SetupStrings.c

Abstract:

  Internal header of the Setup Component.
  
Bug 2733: Can not configured TPM option in BIOS TPM Setup menu .
TIME: 2011-8-16
$AUTHOR: Ken Zhu
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Add one new member InitTpmStrings() in array mInitStringsFuncList[].
$END--------------------------------------------------------------------
--*/
#include "PlatformSetup.h"
#include <AutoBuildTime.h>
#include <BiosVersion.h>
#include "CpuSetup.h"
#include <Protocol/DiskInfo.h>
#include <IndustryStandard/Atapi.h>
#include "SetupItemId.h"
#include "VfrExtension.h"
#include <Library/PlatformCommLib.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <AsiaCpuProtocol.h>
#include <AsiaNbProtocol.h>
#include <AsiaSbProtocol.h>
#include <CHX001Cfg.h>
#include <CHX001/GENERAL.h>



//
// Length of temp string buffer to store value string.
//
#define _48_BIT_ADDRESS_FEATURE_SET_SUPPORTED   0x0400
#define ATAPI_DEVICE                            0x8000

//

//
// Supported Languages 
//
CHAR8* Iso6392LanguageList[] = {"eng",  "zho", "uqi", NULL}; 
CHAR8* Rfc4646LanguageList[] = {"en-US", "zh-Hans", "uqi", NULL}; 

CHAR16            *mUnknownString = L"!";
EFI_STRING_ID            *gLanguageToken = NULL;
UINTN                          gDefaultLanguageOption = 0;

INIT_STRINGS_FUNCTION mInitStringsFuncList[] = {
  InitCpuStrings,		
  InitPlatformInfoStrings, 
  NULL
  };
  
  
VOID 
InitString (
  EFI_HII_HANDLE HiiHandle, 
  STRING_REF     StrRef, 
  CHAR16         *sFormat, ...
  )
{
  STATIC CHAR16 s[1024];
  VA_LIST  Marker;

  VA_START (Marker, sFormat);
  UnicodeVSPrint (s, sizeof (s),  sFormat, Marker);
  VA_END (Marker);
    
  HiiSetString (HiiHandle, StrRef, s, NULL);
}

/**
  Get the string based on the StringId and HII Package List Handle.

  @param  Token                  The String's ID.
  @param  HiiHandle              The package list in the HII database to search for
                                 the specified string.

  @return The output string.

**/
CHAR16 *
GetToken (
  IN  EFI_STRING_ID                Token,
  IN  EFI_HII_HANDLE               HiiHandle
  )
{
  EFI_STRING  String;

  if (HiiHandle == NULL) {
    return NULL;
  }

  String = HiiGetString (HiiHandle, Token, NULL);
  if (String == NULL) {
    String = AllocateCopyPool (sizeof (mUnknownString), mUnknownString);
    ASSERT (String != NULL);
  }
  return (CHAR16 *) String;
}




STATIC CHAR8 *TrimStr8(CHAR8 *Str)
{
  UINTN    StringLength;
  CHAR8    *NewString;
  BOOLEAN  HasLeading;
  UINTN    SrcIndex;
  UINTN    TarIndex;
  
  NewString = NULL;

  StringLength = AsciiStrLen(Str);
  if(StringLength==0){
    return Str; 
  }

  NewString = AllocatePool(StringLength + 1);
  if(NewString==NULL){
    return Str; 
  }

	HasLeading = TRUE;
	TarIndex   = 0;
	SrcIndex   = 0;
	for(;SrcIndex<StringLength;SrcIndex++){
		if(HasLeading){
			if(Str[SrcIndex] == ' '){
				continue;
			}else{
				HasLeading = FALSE;
			}
		}
		if(TarIndex>0){
			if((Str[SrcIndex-1]==Str[SrcIndex]) && (Str[SrcIndex]==' ')){
				continue;
			}
		}
		NewString[TarIndex++] = Str[SrcIndex];
	}

  if(TarIndex && NewString[TarIndex-1] == L' '){
    TarIndex--;
  }
  NewString[TarIndex] = 0;
  ASSERT(TarIndex<=StringLength);
  CopyMem(Str, NewString, (TarIndex+1)*sizeof(Str[0]));
  FreePool(NewString);
  return Str;
}




VOID
GetCpuInfo (
  EFI_HII_HANDLE HiiHandle
  )
{
  EFI_ASIA_CPU_PROTOCOL  *PtAsiaCpu;
  ACPU_SPEED_INFO        CpuSpeedInfo;
  CHAR8                  BrandString[48+1];
  UINT32                 IntendedFreq;
  UINT8                  NumCores;
  UINT8                  NumClusters;
  EFI_STATUS             Status;
  UINT16                 Value1;
  UINT16                 Value2;
  EFI_PEI_HOB_POINTERS	  GuidHob;
  ASIA_CPU_CONFIGURATION     *CpuFeature;  

  UINT16                CacheSize[3];     // unit in KB
  ACPU_CACHE_INFO       *CacheInfo; 
  UINT32               CPUBusSpeed;
  GuidHob.Raw = GetFirstGuidHob(&gAsiaCpuCfgHobGuid);
  ASSERT(GuidHob.Raw!=NULL);
  CpuFeature = (ASIA_CPU_CONFIGURATION*)(GuidHob.Guid+1); 

  if(((CpuFeature->VrmSupport)&BIT4)==BIT4){  // VRM-3
   InitString(
	HiiHandle,
	STRING_TOKEN(STR_VRM_VALUE), 
	L"SVID %d-Phase", 
	(CpuFeature->VrmSupport)&0xF
  );
	  }
  else{
  InitString(
	HiiHandle,
	STRING_TOKEN(STR_VRM_VALUE), 
	L"PVID %d-Phase", 
	 (CpuFeature->VrmSupport)&0xF
  );
	  }
  /// Retrieve CpuInfo from ASIA
  Status = gBS->LocateProtocol(&gAsiaCpuProtocolGuid, NULL, (VOID**)&PtAsiaCpu);
  ASSERT_EFI_ERROR(Status);
  
  PtAsiaCpu->GetCpuBrandString(BrandString);
  PtAsiaCpu->GetCpuSpeedInfo( &CpuSpeedInfo);
  NumCores = PtAsiaCpu->GetCpuCores();
  NumClusters=PtAsiaCpu->GetCpuClusters();

  DEBUG((EFI_D_INFO, "CPUFreq:%d * %d\n", CpuSpeedInfo.CPUBusSpeed, CpuSpeedInfo.CurrentBusRatio));
	
  IntendedFreq = (CpuSpeedInfo.CPUBusSpeed * CpuSpeedInfo.CurrentBusRatio) / 2;
  IntendedFreq = ((IntendedFreq + 5) / 10) * 10;
  Value1 = (UINT16)(IntendedFreq / 1000);
  Value2 = (UINT16)(IntendedFreq % 1000);
  Value2 = Value2/10;
  
  InitString(
    HiiHandle,
    STRING_TOKEN(STR_PROCESSOR_VERSION_VALUE), 
    L"%a", 
    BrandString
  );

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_PROCESSOR_SPEED_VALUE),
    L"%d.%02d GHz",
    Value1,
    Value2
  );

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_PROCESSOR_CORE_COUNT_VALUE),
    L"%d",
    NumCores
  );

  InitString (
    HiiHandle,
    STRING_TOKEN(STR_PROCESSOR_CLUSTER_COUNT_VALUE),
    L"%d",
    NumClusters
  );

  CacheInfo = NULL;
  CacheInfo = AllocatePool(3*sizeof(ACPU_CACHE_INFO));
  ASSERT(CacheInfo != NULL);
   PtAsiaCpu->GetCpuCacheInfo(CacheInfo);
  
   CacheSize[0] = CacheInfo[0].CacheSize;
   CacheSize[1] = CacheInfo[1].CacheSize;
   CacheSize[2] = CacheInfo[2].CacheSize;
    

  InitString(
	 HiiHandle,
	 STRING_TOKEN(STR_CACHE_L1I_SIZE_VALUE), 
	 L"%d KB", 
	 CacheSize[0]
   );

    InitString(
	 HiiHandle,
	 STRING_TOKEN(STR_CACHE_L1D_SIZE_VALUE), 
	 L"%d KB", 
	 CacheSize[1]
   );

    InitString(
	 HiiHandle,
	 STRING_TOKEN(STR_CACHE_L2_SIZE_VALUE), 
	 L"%d KB", 
	 CacheSize[2]
   );

	CPUBusSpeed=CpuSpeedInfo.CPUBusSpeed;
	InitString(
	 HiiHandle,
	 STRING_TOKEN(STR_CURRENT_CPUBUS_FREQ_VALUE), 
	 L"%d Mhz", 
	 CPUBusSpeed
   );
  return;
}

VOID
GetMemoryInfo(
  EFI_HII_HANDLE HiiHandle
  )
	{
	 // PLATFORM_MEMORY_INFO	*MemInfo; 
	  EFI_PEI_HOB_POINTERS	  GuidHob;
	  PLAT_DIMM_INFO		  *DimmInfo;
	  int index;
	
	  /// PlatformPkg/Platform/Pei/MemoryCallback.c will create this hob.
	  GuidHob.Raw = GetFirstGuidHob(&gEfiPlatDimmInfoGuid);
	  ASSERT(GuidHob.Raw != NULL);	
	  DimmInfo = (PLAT_DIMM_INFO*)(GuidHob.Guid+1);
	for(index=0;index<DimmInfo->DimmCount;index++)
	  if(DimmInfo->SpdInfo[0].DimmSize){
		  InitString(
			 HiiHandle,
			 STRING_TOKEN(STR_MEMORY_SLOT0_VALUE), 
			 L"%d MB (DDR3)", 
			 DimmInfo->SpdInfo[0].DimmSize
		   );
	  }
		if(DimmInfo->SpdInfo[1].DimmSize){
		  InitString(
			 HiiHandle,
			 STRING_TOKEN(STR_MEMORY_SLOT1_VALUE), 
			 L"%d MB (DDR3)", 
			 DimmInfo->SpdInfo[1].DimmSize
		   );
	  }
		if(DimmInfo->SpdInfo[2].DimmSize){
	  InitString(
			   HiiHandle,
			   STRING_TOKEN(STR_MEMORY_SLOT2_VALUE), 
			   L"%d MB (DDR3)", 
			   DimmInfo->SpdInfo[2].DimmSize
			 );
		}
		if(DimmInfo->SpdInfo[3].DimmSize){
			InitString(
			   HiiHandle,
			   STRING_TOKEN(STR_MEMORY_SLOT3_VALUE), 
			   L"%d MB (DDR3)", 
			   DimmInfo->SpdInfo[3].DimmSize
			 );
		}
	  InitString(
		HiiHandle,
		STRING_TOKEN(STR_TOTAL_MEMORY_VALUE), 
		L"%d MB", 
		DimmInfo->DimmTotalSizeMB
	  );
	
	  InitString(
		HiiHandle,
		STRING_TOKEN(STR_DRAM_CURRENT_FREQ_VALUE), 
		L"%d MHz", 
		DimmInfo->DimmFreq
	  );
	
	}


VOID
GetDramInfo(
  EFI_HII_HANDLE HiiHandle
  )
{
  PLATFORM_MEMORY_INFO    *MemInfo; 
  EFI_PEI_HOB_POINTERS    GuidHob;
 
  GuidHob.Raw = GetFirstGuidHob(&gEfiPlatformMemInfoGuid);
  ASSERT(GuidHob.Raw != NULL);  
  MemInfo = (PLATFORM_MEMORY_INFO*)(GuidHob.Guid+1);

 
InitString(
    HiiHandle,
    STRING_TOKEN(STR_DRAN_CL_VALUE), 
    L"%d", 
    MemInfo->DramCL
  );

InitString(
    HiiHandle,
    STRING_TOKEN(STR_DRAMTRP_VALUE), 
    L"%d", 
    MemInfo->DramTrp
  );

InitString(
    HiiHandle,
    STRING_TOKEN(STR_DRAMTRCD_VALUE), 
    L"%d", 
    MemInfo->DramTrcd
  );

InitString(
    HiiHandle,
    STRING_TOKEN(STR_DRAMTRAS_VALUE), 
    L"%d", 
    MemInfo->DramTras
  );

}



STATIC UINT8 gInvalidMacAddress1[6] = {0, 0, 0, 0, 0, 0};
STATIC UINT8 gInvalidMacAddress2[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

EFI_STATUS
UpdateOnboardLanMac (
  EFI_HII_HANDLE HiiHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_HANDLE                Handle;
	EFI_PCI_IO_PROTOCOL       *PciIo; 
  UINT32                    PciId;
  UINT8                     MacAddr[6];
  
  
  DevPath = (EFI_DEVICE_PATH_PROTOCOL*)gObLanDp;
  Status  = gBS->LocateDevicePath(&gEfiPciIoProtocolGuid, &DevPath, &Handle);
  if(EFI_ERROR(Status)){goto ProcExit;}  
  
  Status = gBS->HandleProtocol(
                  Handle,
                  &gEfiPciIoProtocolGuid, 
                  &PciIo
                  );
  ASSERT(!EFI_ERROR(Status));

  Status = PciIo->Attributes(PciIo, EfiPciIoAttributeOperationEnable, EFI_PCI_IO_ATTRIBUTE_IO, NULL);
  if(EFI_ERROR(Status)){goto ProcExit;} 

  Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, 0, 1, &PciId);
  if(PciId == 0x816810EC){
    Status = PciIo->Io.Read(PciIo, EfiPciIoWidthUint8, 0, 0, 6, MacAddr);    
  } else {
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }

  DEBUG((EFI_D_INFO, "Mac:%02X-%02X-%02X-%02X-%02X-%02X\n", \
    MacAddr[0], MacAddr[1], MacAddr[2], MacAddr[3], MacAddr[4], MacAddr[5]));

  if(CompareMem(MacAddr, gInvalidMacAddress1, sizeof(MacAddr))==0 ||
     CompareMem(MacAddr, gInvalidMacAddress2, sizeof(MacAddr))==0){
    Status = EFI_UNSUPPORTED;
    goto ProcExit;    
  }  

  InitString(
    HiiHandle,
    STRING_TOKEN(STR_LAN_MAC_ADDR_VALUE), 
    L"%02X-%02X-%02X-%02X-%02X-%02X", 
    MacAddr[0], MacAddr[1], MacAddr[2], MacAddr[3], MacAddr[4], MacAddr[5]
    );
  
ProcExit:
  return Status;  
}











VOID SwapEntries (
  IN CHAR8    *Data,
  IN UINT16   Size
  )
{
  UINT16  Index;
  CHAR8   Temp8;

  for (Index = 0; (Index + 1) < Size; Index += 2) {
    Temp8           = Data[Index];
    Data[Index]     = Data[Index + 1];
    Data[Index + 1] = Temp8;
  }
}

/**
  Try to get the controller's ATA/ATAPI description.

  @param Handle                Controller handle.

  @return  The description string.
**/
VOID
GetSataPortInfo (
  EFI_HII_HANDLE         HiiHandle,
  SETUP_DATA             *pSystemConfiguration
  )
{
  UINTN                        Index;
//  UINTN                        NameIndex;
  EFI_STATUS                   Status;
  EFI_DISK_INFO_PROTOCOL       *DiskInfo;
  UINT32                       BufferSize;
  ATA_IDENTIFY_DATA      IdentifyData;
  CHAR16                       *Description;
  UINTN                           HandleCount;
  EFI_HANDLE                      *HandleBuffer;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePathNode;
  PCI_DEVICE_PATH                 *PciDevicePath;
  CHAR8                           *String;
  CHAR8                           *ModelNumber;
  UINT32                          SataPortIndex, IdeChannel;
  UINT32                          PortIndex;
 #ifdef CHIPSET100_EVB103
  UINTN                             DevNum;
  UINTN                             FuncNum;
 #endif
 // UINT8                             ControllerNum=0;
  UINT64                          NumSectors = 0; 
  UINT64                          DriveSizeInBytes = 0;
  UINT64                          RemainderInBytes = 0;
  UINT32                          DriveSizeInGB = 0;
  UINT32                          NumTenthsOfGB = 0;  
  
  String = AllocateZeroPool (0x100);
  ASSERT(String != NULL);
  ModelNumber = AllocateZeroPool(42);
  ASSERT (ModelNumber != NULL);  
  ZeroMem (ModelNumber, 42);

  PciDevicePath = NULL;
  Description = NULL;
  Status = gBS->LocateHandleBuffer (
                                ByProtocol,
                                &gEfiDiskInfoProtocolGuid,
                                NULL,
                                &HandleCount,
                                &HandleBuffer
                                );

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                                HandleBuffer[Index],
                                &gEfiDevicePathProtocolGuid,
                                (VOID *) &DevicePath
                                );
    ASSERT_EFI_ERROR(Status);
    
    
    DevicePathNode = DevicePath;
    while (!IsDevicePathEndType (DevicePathNode)) {
      if ((DevicePathNode->Type == HARDWARE_DEVICE_PATH) && (DevicePathNode->SubType == HW_PCI_DP)) {
        PciDevicePath = (PCI_DEVICE_PATH *) DevicePathNode;
        break;
      }
      
      DevicePathNode = NextDevicePathNode (DevicePathNode);
    }
    
    if (PciDevicePath == NULL) 
      continue;
    
    Status = gBS->HandleProtocol (
                                  HandleBuffer[Index],
                                  &gEfiDiskInfoProtocolGuid,
                                  &DiskInfo
                                  );
    ASSERT_EFI_ERROR (Status);
  
    //
    // SATA MODE
    //
    Status = DiskInfo->WhichIde (
                           DiskInfo,
                           &IdeChannel,
                           &SataPortIndex
                           );

    if (pSystemConfiguration->SataMode == 0) {
      PortIndex = SataPortIndex;
    } else {
        PortIndex = IdeChannel;
      }
    
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR,"No Disk!\n"));
      continue;
    }
    BufferSize = sizeof(ATA_IDENTIFY_DATA);
    Status = DiskInfo->Identify (
                         DiskInfo,
                         &IdentifyData,
                         &BufferSize
                         );
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR,"Identify failed!\n"));
      continue;
    }
    ASSERT_EFI_ERROR(Status);
    CopyMem (ModelNumber, IdentifyData.ModelName, 40);
    SwapEntries (ModelNumber, 40);
    //
     // Truncate it at 14 characters
     //
     ModelNumber[14] = '\0';
     
     //
     // For HardDisk append the size. Otherwise display atapi
     //
   
     if ((!(IdentifyData.config & ATAPI_DEVICE)) || (IdentifyData.config == 0x848A)) {
       if (IdentifyData.command_set_supported_83 & _48_BIT_ADDRESS_FEATURE_SET_SUPPORTED) { 
         NumSectors = *(UINT64 *)&IdentifyData.maximum_lba_for_48bit_addressing; 
       } else {
          NumSectors = IdentifyData.user_addressable_sectors_lo + (IdentifyData.user_addressable_sectors_hi << 16) ; 
       }
       DriveSizeInBytes = MultU64x32 (NumSectors, 512); 
     
       //
       // DriveSizeInGB is DriveSizeInBytes / 1 GB (1 Binary GB = 2^30 bytes)
       // DriveSizeInGB = (UINT32) Div64(DriveSizeInBytes, (1 << 30), &RemainderInBytes); 
       // Convert the Remainder, which is in bytes, to number of tenths of a Binary GB.
       // NumTenthsOfGB = GetNumTenthsOfGB(RemainderInBytes); 
       // DriveSizeInGB is DriveSizeInBytes / 1 GB (1 Decimal GB = 10^9 bytes)
       //
       DriveSizeInGB = (UINT32) DivU64x64Remainder (DriveSizeInBytes, 1000000000, &RemainderInBytes);  
     
       //
       // Convert the Remainder, which is in bytes, to number of tenths of a Decimal GB.
       //
       NumTenthsOfGB = (UINT32) DivU64x64Remainder (RemainderInBytes, 100000000, NULL);       
       AsciiSPrint (String, 0x100, "Hard Disk %a", ModelNumber);
   
     /*  if (PortIndex == 0) {
            InitString(
              HiiHandle,
              STRING_TOKEN(STR_SATA_DRIVE1_VALUE), 
              L"%a", 
              String
              );
       } else {
              InitString(
                HiiHandle,
                STRING_TOKEN(STR_SATA_DRIVE2_VALUE), 
                L"%a", 
                String
                );
       }   */
     } else {
          AsciiSPrint(String, 0x100, "CD/DVD ROM %a", ModelNumber);  

          /*if (PortIndex == 0) {
            InitString(
              HiiHandle,
              STRING_TOKEN(STR_SATA_DRIVE1_VALUE), 
              L"%a", 
              String
              );
           } else {
                InitString(
                  HiiHandle,
                  STRING_TOKEN(STR_SATA_DRIVE2_VALUE), 
                  L"%a", 
                  String
                  );
           }*/
	 }
        
   }
  
  if (HandleBuffer != NULL){
    FreePool (HandleBuffer);
  }
  
  gBS->FreePool(ModelNumber); 
  gBS->FreePool(String); 

  return;  
}



VOID
InitMain (
  EFI_HII_HANDLE HiiHandle
  )
{
  EFI_STATUS                Status;
  SETUP_DATA                SetupData;
  UINTN                     VarSize;
  EFI_ASIA_CPU_PROTOCOL     *AsiaCpu;
  EFI_ASIA_SB_PROTOCOL      *AsiaSb;
  EFI_ASIA_NB_PROTOCOL      *AsiaNb;
  ASIA_NB_CONFIGURATION     *AsiaNbCfg;
  ASIA_SB_CONFIGURATION     *AsiaSbCfg;  
  PLAT_DIMM_INFO            *DimmInfo;
  CHAR8                     *NbVerStr;
  CHAR8                     *SbVerStr;
  

  Status = gBS->LocateProtocol(&gAsiaCpuProtocolGuid, NULL, (VOID**)&AsiaCpu);
  ASSERT_EFI_ERROR(Status);
  Status = gBS->LocateProtocol(&gAsiaNbProtocolGuid,  NULL, (VOID**)&AsiaNb);
  ASSERT_EFI_ERROR(Status);
  Status = gBS->LocateProtocol(&gAsiaSbProtocolGuid,  NULL, (VOID**)&AsiaSb);
  ASSERT_EFI_ERROR(Status);
  AsiaNbCfg = (ASIA_NB_CONFIGURATION*)AsiaNb->NbCfg;  
  AsiaSbCfg = (ASIA_SB_CONFIGURATION*)AsiaSb->SbCfg; 

  DimmInfo = (PLAT_DIMM_INFO*)GetPlatformDimmInfo();

  VarSize = sizeof(SETUP_DATA);
  Status = gRT->GetVariable (
                  PLATFORM_SETUP_VARIABLE_NAME,
                  &gPlatformSetupVariableGuid,
                  NULL,
                  &VarSize,
                  &SetupData
                  );

  ////
  /*
  if(AsiaNbCfg->ChipRevision == NB_REVISION_A0){
    NbVerStr = "A0";
  } else if(AsiaNbCfg->ChipRevision == NB_REVISION_A1){
    NbVerStr = "A1";
  } else {
    NbVerStr = "Unknown";
  }
  if(AsiaSbCfg->ChipRevision == SB_REVISION_A0){
    SbVerStr = "A0";
  } else if(AsiaSbCfg->ChipRevision == SB_REVISION_A1){
    SbVerStr = "A1";
  } else {
    SbVerStr = "Unknown";
  }
  */
  ///
  NbVerStr = "A0";
  SbVerStr = "A0";
  ///
  InitString(
    HiiHandle,
    STRING_TOKEN(STR_CHIP_VERSION_VALUE), 
    L"%a/%a", 
    NbVerStr,
    SbVerStr
    );

  
  InitString(
    HiiHandle,
    STRING_TOKEN(STR_BIOS_VENDOR_VALUE), 
    L"%s", 
    gST->FirmwareVendor != NULL ? gST->FirmwareVendor : L"Byosoft"
    );
    
  


  UpdateOnboardLanMac(HiiHandle);
}
	VOID
InitAdvanced (
  EFI_HII_HANDLE HiiHandle
  )
{
    
  GetCpuInfo(HiiHandle);
  GetMemoryInfo(HiiHandle);
  GetDramInfo(HiiHandle);
}

VOID 
InitSetupStrings (
  EFI_HII_HANDLE      HiiHandle,
  SETUP_CALLBACK_INFO *pCallbackInfo
  )
{
  UINTN                 i;
  INIT_STRINGS_FUNCTION pInitStringsFunc;
  
  if (pCallbackInfo == NULL || pCallbackInfo->HiiHandle == NULL)
    return;
    
  for (i = 0; mInitStringsFuncList[i] != NULL; i++) {
    pInitStringsFunc = mInitStringsFuncList[i];
    pInitStringsFunc (HiiHandle, pCallbackInfo->Class);
  }

  
  switch (pCallbackInfo->Class) {
  case MAIN_FORM_SET_CLASS:
    InitMain (HiiHandle);
    break;
  case ADVANCED_FORM_SET_CLASS:
	 InitAdvanced(HiiHandle);
	 break;

  default:
    break;
  }
}
VOID
InitLanguages (
  EFI_HII_HANDLE HiiHandle
  )
{
  UINTN      Size;
  CHAR8      *LangCodes;

  Size = 0;
  LangCodes = NULL;

  LangCodes = HiiGetSupportedLanguages (HiiHandle);
  if (LangCodes == NULL) {
    Size = sizeof (DEFAULT_LANGUAGE_CODE);
    LangCodes = AllocatePool (Size);
    CopyMem (
      LangCodes, 
      DEFAULT_LANGUAGE_CODE, 
      Size
      );
  } 

  //
  // LangCodes is a volatile variable and needs to be initialized during every boot
  // However, this routine is not invoked during every boot,
  // It is always invoked during the first boot.
  // During subsequent boots it is only invoked when user is trying to enter Setup
  // In order to initialize LanCodes when this routine is not invoked, let's create
  // a non-volatile variable LastLangCodes with the copy of LanCodes.
  // This copy is used to initialize LanCodes when this routine is not invoked.
  // (This code is part of SynchornizeLanguageVariables routine)
  //
  Size = AsciiStrLen(LangCodes) + 1;
  gRT->SetVariable ( 
         LastLangCodes,
         &gPlatformSetupVariableGuid,
         EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
         Size,
         LangCodes
         );

  FreePool (LangCodes);
}

CHAR8*
IsoToRfc (
  IN  CHAR8 *IsoLang, 
  OUT UINTN *RfcLangSize
  )
{
  UINTN i;
  
  if (IsoLang == NULL) 
    return NULL;
  for (i = 0; Iso6392LanguageList[i] != NULL; i++) {
    if (CompareMem (Iso6392LanguageList[i], IsoLang, 3) == 0) {
      if (RfcLangSize != NULL)
        *RfcLangSize = AsciiStrLen (Rfc4646LanguageList[i]) + 1;
      return Rfc4646LanguageList[i];
    }
  }
  return NULL;
}

CHAR8* 
RfcToIso (
  IN  CHAR8 *RfcLang, 
  OUT UINTN *IsoLangSize
  )
{
  UINTN i;
  if (RfcLang == NULL) 
    return NULL;
  for (i = 0; Rfc4646LanguageList[i] != NULL; i++) {
    if (AsciiStrCmp (Rfc4646LanguageList[i], RfcLang) == 0) {
      if (IsoLangSize != NULL) 
        *IsoLangSize = AsciiStrSize(Iso6392LanguageList[i]);
      return Iso6392LanguageList[i];
    }
  }
  return NULL;
}
CHAR8* 
IsoListToRfcList (
  IN  CHAR8 *IsoList, 
  IN  UINTN IsoListSize, 
  OUT UINTN *RfcListSize
  )
{
  CHAR8 *IsoPtr;
  CHAR8 *RfcList;
  CHAR8 *RfcPtr;
  UINTN Length;
  CHAR8 *TmpPtr;

  if (IsoList == NULL || *IsoList == 0 || IsoListSize < 3) 
    return NULL;
  
  //
  // IsoListSize/3 = number of languages in the list
  // RFC4646 defines max lang tag size of 42, plus ';'
  //
  RfcList = AllocatePool (IsoListSize / 3 * (42 + 1) );
  if (RfcList == NULL) 
    return NULL;
  
  for (IsoPtr = IsoList, RfcPtr = RfcList;
       IsoPtr < IsoList + IsoListSize;
       IsoPtr += 3
      ) {
    
    TmpPtr = IsoToRfc (IsoPtr, &Length);
    if (TmpPtr == NULL) 
      continue;
    
    Length--; // string length  = string size - 1
    CopyMem (RfcPtr, TmpPtr, Length);
    RfcPtr += Length;
    *RfcPtr++ = ';';
  }
  if (RfcPtr == RfcList) {
    FreePool (RfcList);
    if (RfcListSize != NULL) 
      *RfcListSize = 0;
    return NULL;
  }
  * (RfcPtr - 1) = 0;
  if (RfcListSize != 0) 
    *RfcListSize = RfcPtr - RfcList;
  return RfcList;
}

CHAR8* 
RfcListToIsoList (
  IN  CHAR8 *RfcList, 
  IN  UINTN RfcListSize, 
  OUT UINTN *IsoListSize
  )
{
  UINTN Size;
  CHAR8 *RfcPtr;
  CHAR8 *RfcEndPtr;
  CHAR8 *IsoPtr;
  CHAR8 *IsoList;
  CHAR8 *IsoLang;
  CHAR8 OriginalChar;

  if (RfcList == NULL || *RfcList == 0) return NULL;
  
  //calculate number of languages in the RfcList
  for (Size = 3, RfcPtr = RfcList; *RfcPtr != 0; RfcPtr++) {
    if (*RfcPtr == ';') Size += 3;
  }
  IsoList = AllocatePool (Size);
  IsoPtr = IsoList;
  RfcEndPtr = RfcList;
  while (RfcEndPtr < RfcList + RfcListSize) {
    RfcPtr = RfcEndPtr;
    while (*RfcEndPtr != ';' && *RfcEndPtr != 0) RfcEndPtr++;
    OriginalChar = *RfcEndPtr;
    *RfcEndPtr = 0;
    IsoLang = RfcToIso (RfcPtr, NULL);
    if (IsoLang != NULL) {
      CopyMem (IsoPtr, IsoLang, 3);
      IsoPtr += 3;
    }
    *RfcEndPtr = OriginalChar;
    RfcEndPtr++;
  }
  
  if (IsoPtr == IsoList) {
    FreePool (IsoList);
    if (IsoListSize != NULL) 
      *IsoListSize = 0;
    return NULL;
  }
  
  if (IsoListSize != NULL) 
    *IsoListSize = IsoPtr - IsoList;
  return IsoList;
}

CHAR8* 
GetFirstIsoLang (
  CHAR8* IsoList, 
  UINTN *LangSize
  )
{
  if (LangSize != NULL) 
    *LangSize = 3;
  return IsoList;
}

CHAR8*
GetFirstRfcLang (
  CHAR8* RfcList, 
  UINTN *LangSize
  )
{
  CHAR8 *RfcPtr;

  for (RfcPtr = RfcList; *RfcPtr != 0 && *RfcPtr != ';'; RfcPtr++);
  *RfcPtr = 0;
  
  if (LangSize != NULL) 
    *LangSize = RfcPtr - RfcList + 1;
  return RfcList;
}

CHAR8* 
GetLangVariable (
  CHAR16   *VariableName, 
  EFI_GUID *VariableGuid, 
  UINTN    *VariableSize
 )
{
  UINTN Size = 0;
  CHAR8 *Buffer = NULL;
  EFI_STATUS Status;

  Status = GetEfiVariable (VariableName, VariableGuid, NULL, &Size, &Buffer);
  if (EFI_ERROR (Status) ) 
    Buffer = NULL;
  else if (VariableSize != NULL)
    *VariableSize = Size;
  return Buffer;
}

VOID
SetLangCodes (
  CHAR16 *VariableName, 
  CHAR8* LangBuffer, 
  UINTN LangBufferSize
  )
{
  if (LangBuffer == NULL) 
    return;
  
  gRT->SetVariable (
         VariableName, 
         &gEfiGlobalVariableGuid,
         EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
         LangBufferSize, 
         LangBuffer
  );
}

VOID
SetLang (
  CHAR16 *VariableName, 
  CHAR8* LangBuffer, 
  UINTN LangBufferSize
  )
{
  if (LangBuffer == NULL) 
    return;
  gRT->SetVariable (
         VariableName, 
         &gEfiGlobalVariableGuid,
         EFI_VARIABLE_NON_VOLATILE |  EFI_VARIABLE_BOOTSERVICE_ACCESS | 
         EFI_VARIABLE_RUNTIME_ACCESS,
         LangBufferSize, 
         LangBuffer 
         );
}


VOID 
SynchornizeLanguageVariables (
  )
{
  CHAR8   *NativeLangBuffer;
  CHAR8   *CompatLangBuffer;
  CHAR8   *NativeLangCodesBuffer;
  CHAR8   *CompatLangCodesBuffer;
  UINTN   NativeLangSize;
  UINTN   CompatLangSize;
  UINTN   NativeLangCodesSize;
  UINTN   CompatLangCodesSize;
  BOOLEAN FreeNativeLangBuffer;
  CHAR8   *CompatLangInNativeFormat;
  CHAR8   *LastLangBuffer;


  DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));

  //
  // Set LanCodes variables
  //
  NativeLangCodesBuffer = GetLangVariable (LastLangCodes, &gPlatformSetupVariableGuid, &NativeLangCodesSize);
  SetLangCodes (NativeLangCodes, NativeLangCodesBuffer, NativeLangCodesSize);
  CompatLangCodesBuffer = RfcListToIsoList (
                            NativeLangCodesBuffer, 
                            NativeLangCodesSize, 
                            &CompatLangCodesSize
                            );
  SetLangCodes (CompatLangCodes, CompatLangCodesBuffer, CompatLangCodesSize);
  FreePool (CompatLangCodesBuffer);
  FreeNativeLangBuffer = FALSE;

  //
  // Set NativeLang variables
  //
  NativeLangBuffer = GetLangVariable (NativeLang, &gEfiGlobalVariableGuid, &NativeLangSize);
  CompatLangBuffer = GetLangVariable (CompatLang, &gEfiGlobalVariableGuid, &CompatLangSize);
  if ((NativeLangBuffer == NULL) && (CompatLangBuffer != NULL)) {
    NativeLangBuffer = IsoToRfc (CompatLangBuffer, &NativeLangSize);
    SetLang (NativeLang, NativeLangBuffer, NativeLangSize);
    FreePool (CompatLangBuffer);
   } else if ((NativeLangBuffer != NULL) && (CompatLangBuffer == NULL)) {
    CompatLangBuffer = RfcToIso (NativeLangBuffer, &CompatLangSize);
    SetLang (CompatLang, CompatLangBuffer, CompatLangSize);
    FreeNativeLangBuffer = TRUE;
  } else if ((NativeLangBuffer == NULL) && (CompatLangBuffer == NULL)) {
    NativeLangBuffer = GetFirstRfcLang (NativeLangCodesBuffer, &NativeLangSize);
    SetLang (NativeLang, NativeLangBuffer, NativeLangSize);
    CompatLangBuffer = RfcToIso (NativeLangBuffer, &CompatLangSize);
    SetLang (CompatLang, CompatLangBuffer, CompatLangSize);
  } else { 
    //
    // NativeLang && CompatLang
    //
    CompatLangInNativeFormat = IsoToRfc (CompatLangBuffer, NULL);
    if (AsciiStrCmp(CompatLangInNativeFormat, NativeLangBuffer) != 0) {
      LastLangBuffer = GetLangVariable (LastLang, &gPlatformSetupVariableGuid, NULL);
      if ((LastLangBuffer != NULL) && 
          (AsciiStrCmp (LastLangBuffer, NativeLangBuffer) == 0)) {
        FreePool (NativeLangBuffer);
        NativeLangBuffer = IsoToRfc (CompatLangBuffer, &NativeLangSize);
        SetLang (NativeLang, NativeLangBuffer, NativeLangSize);
      } else {
        FreePool (CompatLangBuffer);
        CompatLangBuffer = RfcToIso (NativeLangBuffer, &CompatLangSize);
        SetLang (CompatLang, CompatLangBuffer, CompatLangSize);
        FreeNativeLangBuffer = TRUE;
      }
      if (LastLangBuffer != NULL)
        FreePool (LastLangBuffer);
    }
  }
  
  gRT->SetVariable (
         LastLang, 
         &gPlatformSetupVariableGuid,
         EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
         NativeLangSize, 
         NativeLangBuffer
         );
  
  if (FreeNativeLangBuffer) 
    FreePool (NativeLangBuffer);
  FreePool (NativeLangCodesBuffer);
}


EFI_STATUS
GetEfiVariable (
  IN     CHAR16    *sName, 
  IN     EFI_GUID  *pGuid,
  OUT    UINT32    *pAttributes OPTIONAL,
  IN OUT UINTN     *pDataSize, 
  OUT    VOID      **ppData
  )
{
  EFI_STATUS Status;
  
  if (*ppData != NULL) 
    *pDataSize = 0;
  
  Status = gRT->GetVariable (sName, pGuid, pAttributes, pDataSize, *ppData);
  if (!EFI_ERROR(Status)) 
    return Status;
  
  if (Status == EFI_BUFFER_TOO_SMALL) {
    if (*ppData != NULL)
      FreePool(*ppData);
    
    *ppData = AllocatePool(*pDataSize);
    if (*ppData == NULL) 
      return EFI_OUT_OF_RESOURCES;
    
    Status = gRT->GetVariable (sName, pGuid, pAttributes, pDataSize, *ppData);
  }
  return Status;
}


/**
  Get next language from language code list (with separator ';').

  If LangCode is NULL, then ASSERT.
  If Lang is NULL, then ASSERT.

  @param  LangCode    On input: point to first language in the list. On
                                 output: point to next language in the list, or
                                 NULL if no more language in the list.
  @param  Lang           The first language in the list.

**/
VOID
EFIAPI
GetNextLanguage (
  IN OUT CHAR8      **LangCode,
  OUT CHAR8         *Lang
  )
{
  UINTN  Index;
  CHAR8  *StringPtr;

  ASSERT (LangCode != NULL);
  ASSERT (*LangCode != NULL);
  ASSERT (Lang != NULL);

  Index = 0;
  StringPtr = *LangCode;
  while (StringPtr[Index] != 0 && StringPtr[Index] != ';') {
    Index++;
  }

  CopyMem (Lang, StringPtr, Index);
  Lang[Index] = 0;

  if (StringPtr[Index] == ';') {
    Index++;
  }
  *LangCode = StringPtr + Index;
}

VOID
UpdateLanguageSettingItem (
  EFI_HII_HANDLE HiiHandle
  )
{
  EFI_STATUS                  Status;
  CHAR8                       *LanguageString;
  CHAR8                       *LangCode;
  CHAR8                       *Lang;
  CHAR8                       *CurrentLang;
  CHAR8                       *BestLanguage;
  UINTN                       OptionCount;
  CHAR16                      *StringBuffer;
  VOID                        *OptionsOpCodeHandle;
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartLabel;
  EFI_IFR_GUID_LABEL          *EndLabel;
  BOOLEAN                     FirstFlag;  
  EFI_GUID              MainFormSetGuid      = MAIN_FORM_SET_GUID;
  //
  // Init OpCode Handle and Allocate space for creation of UpdateData Buffer
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);
  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_SELECT_LANGUAGE;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  //
  // Collect the languages from what our current Language support is based on our VFR
  //
  LanguageString = HiiGetSupportedLanguages (HiiHandle);
  ASSERT (LanguageString != NULL);
  //
  // Allocate working buffer for RFC 4646 language in supported LanguageString.
  //
  Lang = AllocatePool (AsciiStrSize (LanguageString));
  ASSERT (Lang != NULL);

  CurrentLang = GetEfiGlobalVariable (L"PlatformLang");
  //
  // Select the best language in LanguageString as the default one.
  //
  BestLanguage = GetBestLanguage (
                   LanguageString,
                   FALSE,
                   (CurrentLang != NULL) ? CurrentLang : "",
                   (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLang),
                   LanguageString,
                   NULL
                   );
  //
  // BestLanguage must be selected as it is the first language in LanguageString by default
  //
  ASSERT (BestLanguage != NULL);

  OptionCount = 0;
  LangCode    = LanguageString;
  FirstFlag   = FALSE;

 if (NULL == gLanguageToken) {
    while (*LangCode != 0) {
      GetNextLanguage (&LangCode, Lang);
      OptionCount ++;
    }
    gLanguageToken = AllocatePool (OptionCount * sizeof (EFI_STRING_ID));
    ASSERT (gLanguageToken != NULL);
    FirstFlag = TRUE; 
  }

  OptionCount = 0;
  LangCode = LanguageString;
  while (*LangCode != 0) {
    GetNextLanguage (&LangCode, Lang);	
    if (AsciiStrCmp (Lang, "uqi") == 0) {
      continue;
    }
    if (FirstFlag) {
      StringBuffer = HiiGetString (HiiHandle, PRINTABLE_LANGUAGE_NAME_STRING_ID, Lang);
      ASSERT (StringBuffer != NULL);
      //
      // Save the string Id for each language
      //
      gLanguageToken[OptionCount] = HiiSetString (HiiHandle, 0, StringBuffer, NULL);
      FreePool (StringBuffer);
    }

    if (AsciiStrCmp (Lang, BestLanguage) == 0) {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        gLanguageToken[OptionCount],
        EFI_IFR_OPTION_DEFAULT,
        EFI_IFR_NUMERIC_SIZE_1,
        (UINT8) OptionCount
        );
        gDefaultLanguageOption = OptionCount;
    } else {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        gLanguageToken[OptionCount],
        0,
        EFI_IFR_NUMERIC_SIZE_1,
        (UINT8) OptionCount
        );
    }

    OptionCount++;
  }

  if (CurrentLang != NULL) {
    FreePool (CurrentLang);
  }
  FreePool (BestLanguage);
  FreePool (Lang);
  FreePool (LanguageString);

  HiiCreateOneOfOpCode (
    StartOpCodeHandle,
    MAIN_PAGE_KEY_LANGUAGE,
    0,
    0,
    STRING_TOKEN (STR_MAIN_LANGUAGE_SELECT),
    STRING_TOKEN (STR_MAIN_LANGUAGE_SELECT_HELP),
    EFI_IFR_FLAG_CALLBACK,
    EFI_IFR_NUMERIC_SIZE_1,
    OptionsOpCodeHandle,
    NULL
    );

  Status = HiiUpdateForm (
             HiiHandle,
             &MainFormSetGuid,
             MAIN_PAGE_FORM_ID,
             StartOpCodeHandle, // LABEL_SELECT_LANGUAGE
             EndOpCodeHandle    // LABEL_END
             );
  ASSERT (Status == EFI_SUCCESS);

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

}


EFI_STATUS
SetLanguageCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest  OPTIONAL
  )
{
  CHAR8                         *LanguageString;
  CHAR8                         *LangCode;
  CHAR8                         *Lang;
  UINTN                         Index;
  EFI_STATUS                    Status;
  CHAR8                         *PlatformSupportedLanguages;
  CHAR8                         *BestLanguage;

  if (Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD) {
    if (QuestionId == MAIN_PAGE_KEY_LANGUAGE) {
      Value->u8 = (UINT8) gDefaultLanguageOption;		
      return EFI_SUCCESS;
    }
  }

  if (Action != EFI_BROWSER_ACTION_CHANGING) {
    //
    // Do nothing for UEFI OPEN/CLOSE Action
    //
    return EFI_SUCCESS;
  }

  switch (QuestionId) {
    case MAIN_PAGE_KEY_LANGUAGE:
      //
      // Collect the languages from what our current Language support is based on our VFR
      //
      LanguageString = HiiGetSupportedLanguages (gSetupCallbackInfo[0].HiiHandle);
      ASSERT (LanguageString != NULL);
      //
      // Allocate working buffer for RFC 4646 language in supported LanguageString.
      //	  
      Lang = AllocatePool (AsciiStrSize (LanguageString));
      ASSERT (Lang != NULL);

      Index = 0;
      LangCode = LanguageString;
      while (*LangCode != 0) {
        GetNextLanguage (&LangCode, Lang);
        if (AsciiStrCmp (Lang, "uqi") == 0) {
         continue;
       }
        if (Index == Value->u8) {
          gDefaultLanguageOption = Index;
          break;
        }
        Index++;
      }

      PlatformSupportedLanguages = GetEfiGlobalVariable (L"PlatformLangCodes");
      if (PlatformSupportedLanguages == NULL) {
        PlatformSupportedLanguages = AllocateCopyPool (
                                       AsciiStrSize ((CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes)),
                                       (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes)
                                       );
        ASSERT (PlatformSupportedLanguages != NULL);
      }
      //
      // Select the best language in platform supported Language.
      //
      BestLanguage = GetBestLanguage (
                       PlatformSupportedLanguages,
                       FALSE,
                       Lang,
                       NULL
                       );
      if (BestLanguage != NULL) {
        Status = gRT->SetVariable (
                        L"PlatformLang",
                        &gEfiGlobalVariableGuid,
                        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                        AsciiStrSize (BestLanguage),
                        BestLanguage
                        );
        ASSERT_EFI_ERROR(Status);
        FreePool (BestLanguage);
      } else {
        ASSERT (FALSE);
      }

      FreePool (PlatformSupportedLanguages);
      FreePool (Lang);
      FreePool (LanguageString);
      break;
	  
    default:
      break;
  }

  return EFI_SUCCESS;
}

