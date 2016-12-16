

#include "PlatformDxe.h"
#include <Guid/Acpi.h>
#include <Guid/AcpiS3Context.h>
#include <IndustryStandard/Pci.h>
#include <Library/SerialPortLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/LockBoxLib.h>
#include <Library/MtrrLib.h>
#include <Protocol/ExitPmAuth.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/NvMediaAccess.h>
#include <CHX001Reg.h>

VOID
SmbiosCallback (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  );

extern EFI_GUID gBdsAllDriversConnectedProtocolGuid;
extern EFI_GUID gEfiSetupEnterGuid;

EFI_ASIA_CPU_PROTOCOL *gPtAsiaCpu = NULL;
EFI_ASIA_SB_PROTOCOL  *gPtAsiaSb  = NULL;
EFI_ASIA_NB_PROTOCOL  *gPtAsiaNb  = NULL;
ASIA_NB_CONFIGURATION *gAsiaNbCfg = NULL;
ASIA_SB_CONFIGURATION *gAsiaSbCfg = NULL;
CONST SETUP_DATA      *gSetupData;




VOID
SaveBridgeRegistersForS3 (
  IN  UINTN  BaseAddr
)  
{
  EFI_STATUS  Status;

  if(MmioRead16(BaseAddr + PCI_VID_REG) == 0xFFFF){
    return;
  }
  
  Status = S3BootScriptSaveMemWrite (
             S3BootScriptWidthUint32,
             BaseAddr + PCI_PBN_REG,
             7,
             (VOID*)(UINTN)(BaseAddr + PCI_PBN_REG)
             );
  ASSERT_EFI_ERROR(Status);

  Status = S3BootScriptSaveMemWrite (
             S3BootScriptWidthUint32,
             BaseAddr + PCI_INT_LINE_REG,
             1,
             (VOID*)(UINTN)(BaseAddr + PCI_INT_LINE_REG)
             );
  ASSERT_EFI_ERROR(Status);   
  
  Status = S3BootScriptSaveMemWrite (
             S3BootScriptWidthUint32,
             BaseAddr + PCI_CMD_REG,
             1,
             (VOID*)(UINTN)(BaseAddr + PCI_CMD_REG)
             );
  ASSERT_EFI_ERROR(Status);  
}



VOID
ExitPmAuthCallBack (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  VOID              *Interface;
  EFI_STATUS        Status;
 // UINT32            Data32And, Data32Or;
  UINT32            PciId;
  UINT32            Data32;
  

  Status = gBS->LocateProtocol(&gExitPmAuthProtocolGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }
  gBS->CloseEvent(Event);
  DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));  

  
  Status = gPtAsiaSb->PreBootInit(gPtAsiaSb);
  ASSERT_EFI_ERROR(Status);
 
  Status = gPtAsiaNb->PreBootInit(gPtAsiaNb);
  ASSERT_EFI_ERROR(Status);

  Status = S3BootScriptSaveMemWrite (
             S3BootScriptWidthUint32,
             HIF_PCI_REG(PAGE_C_SHADOW_CTRL_REG),
             1,
             (VOID*)(UINTN)HIF_PCI_REG(PAGE_C_SHADOW_CTRL_REG)
             ); 
  ASSERT_EFI_ERROR(Status);    

  if(MmioRead16(IGDAC_PCI_REG(PCI_VID_REG))!=0xFFFF){
    Status = S3BootScriptSaveMemWrite (
               S3BootScriptWidthUint32,
               IGDAC_PCI_REG(PCI_BAR0_REG),
               1,
               (VOID*)(UINTN)IGDAC_PCI_REG(PCI_BAR0_REG)
               );
    ASSERT_EFI_ERROR(Status);
  }

// update IGD SSID.
  PciId = MmioRead32(IGD_PCI_REG(PCI_VID_REG));
  if((UINT16)PciId!=0xFFFF){
    Data32 = MmioRead32(IGD_PCI_REG(PCI_BAR0_REG)) & 0xFFFFFFF0;
    MmioWrite32(Data32 + IGD_SVID_MMIO_OFFSET, PciId);
    MmioWrite32(Data32 + IGDAC_SVID_MMIO_OFFSET, MmioRead32(IGDAC_PCI_REG(PCI_VID_REG)));

    Status = S3BootScriptSaveMemWrite (
               S3BootScriptWidthUint32,
               IGD_PCI_REG(PCI_BAR0_REG),
               3,
               (VOID*)(UINTN)IGD_PCI_REG(PCI_BAR0_REG)
               );
    ASSERT_EFI_ERROR(Status);

    Status = S3BootScriptSaveMemWrite (
               S3BootScriptWidthUint8,
               IGD_PCI_REG(PCI_CMD_REG),
               1,
               (VOID*)(UINTN)IGD_PCI_REG(PCI_CMD_REG)
               );
    ASSERT_EFI_ERROR(Status);

    Status = S3BootScriptSaveMemWrite (
               S3BootScriptWidthUint32,
               Data32 + IGD_SVID_MMIO_OFFSET,
               1,
               (VOID*)(UINTN)(Data32 + IGD_SVID_MMIO_OFFSET)
               );
    ASSERT_EFI_ERROR(Status);

    Status = S3BootScriptSaveMemWrite (
               S3BootScriptWidthUint32,
               Data32 + IGDAC_SVID_MMIO_OFFSET,
               1,
               (VOID*)(UINTN)(Data32 + IGDAC_SVID_MMIO_OFFSET)
               );
    ASSERT_EFI_ERROR(Status);     
  }  
  
// VISA has saved azalia info in InitAzaliaAudio() for S3. 
//LNA-20161031-S
// Save Bridge Registers
  SaveBridgeRegistersForS3(PEG_PCI_REG(PCI_VID_REG));
  SaveBridgeRegistersForS3(PE0_PCI_REG(PCI_VID_REG));  
  SaveBridgeRegistersForS3(PE1_PCI_REG(PCI_VID_REG)); 
  SaveBridgeRegistersForS3(PE2_PCI_REG(PCI_VID_REG));  
  SaveBridgeRegistersForS3(PE3_PCI_REG(PCI_VID_REG)); 
  SaveBridgeRegistersForS3(PEG0_PCI_REG(PCI_VID_REG)); 	
  SaveBridgeRegistersForS3(PEG1_PCI_REG(PCI_VID_REG)); 	
  SaveBridgeRegistersForS3(PEG2_PCI_REG(PCI_VID_REG)); 	
  SaveBridgeRegistersForS3(PEG3_PCI_REG(PCI_VID_REG)); 	
//LNA-20161031-E


}



STATIC PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformIgdDevice = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x1
  },
  gEndEntire
};


UINT32 GetAcpiTableSmmCommAddr(VOID)
{
  EFI_ACPI_4_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Xsdt;
  UINTN                                         Index;
  UINT64                                        *XTableAddress;
  UINTN                                         TableCount;
  EFI_STATUS                                    Status;	
  EFI_SMM_COMMUNICATION_ACPI_TABLE              *SmmAcpi;
	

  Status = EfiGetSystemConfigurationTable(&gEfiAcpiTableGuid, &Rsdp);
  ASSERT(!EFI_ERROR(Status));	
  if (EFI_ERROR(Status)) {
    return 0;
  }

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER*)(UINTN)Rsdp->XsdtAddress;
  TableCount = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT64);
  XTableAddress = (UINT64 *)(Xsdt + 1);
  for (Index = 0; Index < TableCount; Index++) {
    SmmAcpi = (EFI_SMM_COMMUNICATION_ACPI_TABLE *)(UINTN)XTableAddress[Index];
    if ((SmmAcpi->UefiAcpiDataTable.Header.Signature == EFI_ACPI_4_0_UEFI_ACPI_DATA_TABLE_SIGNATURE) &&
        (SmmAcpi->UefiAcpiDataTable.Header.Length == sizeof (EFI_SMM_COMMUNICATION_ACPI_TABLE)) &&
        CompareGuid (&(SmmAcpi->UefiAcpiDataTable.Identifier), &gEfiSmmCommunicationProtocolGuid) ) {
      return (UINT32)(UINTN)SmmAcpi;
    }
  }

  ASSERT(FALSE);
  return 0;
}


#ifndef MDEPKG_NDEBUG	
VOID DumpSysSetting()
{
  PLATFORM_S3_RECORD    *S3Record;
  UINT32                Index;
  

  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
 
  DEBUG((EFI_D_INFO, "XhciMcuFw    :%X\n", S3Record->XhciMcuFw));
  DEBUG((EFI_D_INFO, "XhciMcuFwSize:%X\n", S3Record->XhciMcuFwSize));  
  DEBUG((EFI_D_INFO, "XhciHubFw    :%X\n", S3Record->XhciHubFw));
  DEBUG((EFI_D_INFO, "XhciHubFwSize:%X\n", S3Record->XhciHubFwSize));
  DEBUG((EFI_D_INFO, "S3StackBase  :%X\n", S3Record->S3StackBase)); 
  DEBUG((EFI_D_INFO, "S3StackSize  :%X\n", S3Record->S3StackSize));   
  DEBUG((EFI_D_INFO, "S3Cr3        :%X\n", S3Record->S3Cr3));
  DEBUG((EFI_D_INFO, "ScatAddr     :%X\n", S3Record->ScatAddr));
  DEBUG((EFI_D_INFO, "MtrrTable    :%p\n", &S3Record->MtrrTable));   
  DEBUG((EFI_D_INFO, "CpuCount     :%X\n", S3Record->CpuCount)); 
  DEBUG((EFI_D_INFO, "SmmUcData    :%X\n", S3Record->SmmUcData));
  DEBUG((EFI_D_INFO, "SmmUcDataSize:%X\n", S3Record->SmmUcDataSize));
  DEBUG((EFI_D_INFO, "SmrrBase     :%X\n", S3Record->SmrrBase));
  DEBUG((EFI_D_INFO, "SmrrSize     :%X\n", S3Record->SmrrSize));
  DEBUG((EFI_D_INFO, "CpuApVector  :%X\n", S3Record->CpuApVector));

  for(Index=0;Index<S3Record->CpuCount;Index++){
    DEBUG((EFI_D_INFO, "CPU[%d] ID:%X BASE:%X\n", Index, \
      S3Record->CpuApicId[Index], S3Record->CpuSmBase[Index]));
  }  
}
#endif

//// Watchdog Timer related setting. TGR-2016-10-23+S
EFI_STATUS
ZX_UpdateWdrt (
  )
{
  EFI_STATUS                                      Status;
  UINT64              	                          Address;
  UINT32                                          WdrtBaseAddress;
  UINT8                                           TempB;
  UINT16                                          TempW;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL                 *PciRootBridgeIo;
  ////
  UINTN      VariableSize;
  SETUP_DATA BackupSetupData;

  ////
  Status = EFI_SUCCESS;

  ////
  VariableSize = sizeof (SETUP_DATA);
  Status = gRT->GetVariable (
                  PLATFORM_SETUP_VARIABLE_NAME,
                  &gPlatformSetupVariableGuid,
                  NULL,
                  &VariableSize,
                  &BackupSetupData
                  );
  ASSERT_EFI_ERROR (Status);  
  //// gEfiPciRootBridgeIoProtocolGuid
  Status = gBS->LocateProtocol (
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  &PciRootBridgeIo
                  );
  ASSERT_EFI_ERROR (Status);    
  //
  // If WDRT is disabled in setup, don't publish the table.
  //
  if (!BackupSetupData.WatchDogTimer) {
    DEBUG((EFI_D_ERROR,"\n D17F0_PMU WatchDogTimer is disable now \n"));
    return EFI_UNSUPPORTED;
  }

  //program Base register
  WdrtBaseAddress = 0xFED01000; // Ref ASIAConfig.h
  Address = (CHX001_BUSC|D17F0_PMU_WATCHDOG_TIMER_MEM_BASE);
  Status = PciRootBridgeIo->Pci.Write (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint32,
                                  Address,
                                  1,
                                  &WdrtBaseAddress
                                  );

  //SCRIPT_PCI_CFG_WRITE (
  //  EFI_ACPI_S3_RESUME_SCRIPT_TABLE,
  //  EfiBootScriptWidthUint32,
  //  Address,
  //  1,
  //  &WdrtBaseAddress
  //  );

  //program Enable bits
  Address = (CHX001_BUSC|D17F0_PMU_WATCHDOG_TIMER_CTL_C3_LATENCY_CTL);
  Status = PciRootBridgeIo->Pci.Read (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint8,
                                  Address,
                                  1,
                                  &TempB
                                  );
  TempB |= 0x03;
  Status = PciRootBridgeIo->Pci.Write (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint8,
                                  Address,
                                  1,
                                  &TempB
                                  );

  //SCRIPT_PCI_CFG_WRITE (
  //  EFI_ACPI_S3_RESUME_SCRIPT_TABLE,
  //  EfiBootScriptWidthUint8,
  //  Address,
  //  1,
  //  &TempB
  //  );

  //program Action and Run/Stop
  TempB = 0;
  if (BackupSetupData.WatchDogTimerAction) TempB |=0x04;
  if (BackupSetupData.WatchDogTimerRunStop) TempB |=0x01;
  Address = WdrtBaseAddress;
  Status = PciRootBridgeIo->Mem.Write (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint8,
                                  Address,
                                  1,
                                  &TempB
                                  );

  //program Count
  switch(BackupSetupData.WatchDogTimerCount) {
    case 0:
      TempW = 72;
      break;
    case 1:
      TempW = 389;
      break;
    case 2:
      TempW = 706;
      break;
    case 3:
      TempW = 1023;
      break;
  }

  Address = WdrtBaseAddress + 4;
  Status = PciRootBridgeIo->Mem.Write (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint16,
                                  Address,
                                  1,
                                  &TempW
                                  );

  //program Trigger
  Address = WdrtBaseAddress;
  Status = PciRootBridgeIo->Mem.Read(
                                  PciRootBridgeIo,
                                  EfiPciWidthUint8,
                                  Address,
                                  1,
                                  &TempB
                                  );
  TempB &= 0xFD;
  TempB |= 0x80;
  Status = PciRootBridgeIo->Mem.Write(
                                  PciRootBridgeIo,
                                  EfiPciWidthUint8,
                                  Address,
                                  1,
                                  &TempB
                                  );

  return Status;
}

//// 2016-10-23+E

VOID
EFIAPI
PlatOnReadyToBoot (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  PLATFORM_MEMORY_INFO  *MemInfo;
  EFI_STATUS            Status;
  PLATFORM_S3_RECORD    *S3Record;  
  ACPI_S3_CONTEXT       S3Ctx;	
  UINTN                 S3CtxSize;
  UINT64                HighMemSize;
  EFI_PHYSICAL_ADDRESS  PmmMemory;
    
  
  DEBUG((EFI_D_INFO, __FUNCTION__"()\n")); 
  gBS->CloseEvent(Event);

  MemInfo = (PLATFORM_MEMORY_INFO*)GetPlatformMemInfo(); 
  
  if(MemInfo->PhyMemSize > (UINT64)MemInfo->Tolum){
	  //ECS20161104 Update E820 table for UMA enable -S
	  //HighMemSize = MemInfo->PhyMemSize - MemInfo->Tolum;
	  HighMemSize = MemInfo->PhyMemSize - MemInfo->Tolum - MemInfo->VgaBufSize;
	  //ECS20161104 Update E820 table for UMA enable -E
    if(SIZE_4GB + HighMemSize > MemInfo->Pci64Base){
      HighMemSize = MemInfo->Pci64Base - SIZE_4GB;
    }  
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeSystemMemory,
                    SIZE_4GB,
                    HighMemSize,
                    EFI_MEMORY_UC|EFI_MEMORY_WC|EFI_MEMORY_WT|EFI_MEMORY_WB
                    );
    ASSERT_EFI_ERROR(Status);                    
  }

  if(MemInfo->PmmBase && MemInfo->PmmSize){
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeSystemMemory,
                    MemInfo->PmmBase,
                    MemInfo->PmmSize,
                    EFI_MEMORY_UC|EFI_MEMORY_WC|EFI_MEMORY_WT|EFI_MEMORY_WB
                    );
    ASSERT_EFI_ERROR(Status);

    PmmMemory = MemInfo->PmmBase;
    Status = gBS->AllocatePages(
                    AllocateAddress,
                    EfiBootServicesData,
                    EFI_SIZE_TO_PAGES(MemInfo->PmmSize),
                    &PmmMemory
                    );
    ASSERT((!EFI_ERROR(Status)) && (PmmMemory == MemInfo->PmmBase));
  }

  
  Status = AzaliaLoadVerbTable(gOemVerbTable, gOemVerbTableSize);
  ASSERT_EFI_ERROR(Status);
  
  IoWrite16(PMIO_REG(PMIO_STS_REG), PMIO_STS_TMROF|PMIO_STS_BM);
  IoWrite16(PMIO_REG(PMIO_GBLSTS_REG), PMIO_GBLSTS_PIRQ);  

  S3Record = (PLATFORM_S3_RECORD*)GetS3RecordTable();
	S3Record->ScatAddr = GetAcpiTableSmmCommAddr();
  // PCD : PlatformPkg\Override\IntelFrameworkModulePkg\Csm\LegacyBiosDxe\LegacyBda.c will set it.	
  S3Record->CpuApVector = PcdGet32(PcdCpuS3ApVectorAddress);

  S3CtxSize = sizeof(S3Ctx);
  RestoreLockBox(&gEfiAcpiS3ContextGuid, &S3Ctx, &S3CtxSize);	
  S3Record->S3StackBase = (UINT32)S3Ctx.BootScriptStackBase;
  S3Record->S3StackSize = (UINT32)S3Ctx.BootScriptStackSize;
  S3Record->S3Cr3       = (UINT32)S3Ctx.S3NvsPageTableAddress;

  MtrrGetAllMtrrs(&S3Record->MtrrTable);

  ////
  DEBUG((EFI_D_ERROR,"Enable or Disable D17F0_PMU Watchdog timer \n"));
  ZX_UpdateWdrt();
  ////
#ifndef MDEPKG_NDEBUG	  
  DumpSysSetting(); // bf boot OS, dump all function settings.
  //ZX_DumpPciDevSetting(); // tiger added. 2016-06-23
#endif  
}



VOID
EFIAPI
AllDriversConnectedCallBack (
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  VOID        *Interface;
  EFI_STATUS  Status;
  DEBUG((EFI_D_ERROR,"mike_ConnectedCallBack\n"));
  Status = gBS->LocateProtocol(&gBdsAllDriversConnectedProtocolGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }
  gBS->CloseEvent(Event);

  UpdatePs2State();

  InstallAcpiTableDmar();
}






STATIC EFI_STATUS RmObLanVfr()
{
  EFI_STATUS                      Status;
  EFI_HII_DATABASE_PROTOCOL       *HiiDB;
  EFI_HII_HANDLE                  *Handles = NULL;
  UINTN                           HandleLen;
  UINTN                           HiiHandleCount;
  UINTN                           Index;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *HiiCfgAcc; 
  EFI_DEVICE_PATH_PROTOCOL        *DevPath;  
  UINTN                           CmpSize;
  

  Status = gBS->LocateProtocol(&gEfiHiiDatabaseProtocolGuid, NULL, &HiiDB);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  HandleLen = 0;
  Status = HiiDB->ListPackageLists(
                    HiiDB, 
                    EFI_HII_PACKAGE_TYPE_ALL, 
                    NULL, 
                    &HandleLen, 
                    Handles
                    );
  if(Status != EFI_BUFFER_TOO_SMALL){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }  
  
  Handles = AllocatePool(HandleLen);
  if(Handles == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;    
  }   
  Status = HiiDB->ListPackageLists(
                    HiiDB, 
                    EFI_HII_PACKAGE_TYPE_ALL, 
                    NULL, 
                    &HandleLen, 
                    Handles
                    );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  CmpSize = gObLanDpSize - sizeof(EFI_DEVICE_PATH_PROTOCOL);
  HiiHandleCount = HandleLen/sizeof(EFI_HII_HANDLE);
  for(Index = 0; Index < HiiHandleCount; Index++){
    Status = HiiDB->GetPackageListHandle(HiiDB, Handles[Index], &DriverHandle);
    if(EFI_ERROR(Status) || DriverHandle == NULL){
      continue;
    }  
    Status = gBS->HandleProtocol(
                    DriverHandle,
                    &gEfiHiiConfigAccessProtocolGuid,
                    &HiiCfgAcc
                    );
    if(EFI_ERROR(Status)){
      continue;
    }  
    Status = gBS->HandleProtocol(
                    DriverHandle,
                    &gEfiDevicePathProtocolGuid,
                    &DevPath
                    );
    if(EFI_ERROR(Status)){
      continue;
    }  
    if(CompareMem(gObLanDp, DevPath, CmpSize) == 0){ 
      Status = HiiDB->RemovePackageList(HiiDB, Handles[Index]);
      DEBUG((EFI_D_INFO, "RemovePackageList(%X):%r\n", Handles[Index], Status));
    }

  }  


ProcExit:
  if(Handles != NULL){
    FreePool(Handles);
  }  
  return Status;
}



VOID
EFIAPI
EnterSetupCallBack (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  VOID        *Interface;
  EFI_STATUS  Status;
  DEBUG((EFI_D_ERROR,"mike_ConnectedCallBack\n"));
  Status = gBS->LocateProtocol(&gEfiSetupEnterGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }
  gBS->CloseEvent(Event);

  RmObLanVfr();
}

VOID
EFIAPI
NvMediaAccessSpiCallBack (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  NV_MEDIA_ACCESS_PROTOCOL  *NvAcc;
  EFI_STATUS                Status;
  UINT8                     *BiosData = NULL;
  UINTN                     Address;
  UINTN                     SectorSize;
  UINTN                     Size;
  UINTN                     RomSipOffset;
  VOID                      *Interface;
  UINTN                     RomSipAddr;
  
  
  Status = gBS->LocateProtocol(&gEfiNvMediaAccessSpiReadyGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }
  gBS->CloseEvent(Event);

  //if(!gAsiaNbCfg->RomsipStatus){
  //  goto ProcExit;
  //}  
  DEBUG((EFI_D_INFO, "Update ROMSIP\n"));

  Status = gBS->LocateProtocol(&gEfiNvMediaAccessProtocolGuid, NULL, &NvAcc);
  ASSERT(!EFI_ERROR(Status));

  SectorSize = SIZE_4KB;
  BiosData = AllocatePool(SectorSize);
  if(BiosData == NULL){
    goto ProcExit;
  }  

  Address = 0xFFFFFFFF - SectorSize + 1;
  Size    = SectorSize;
  Status  = NvAcc->Read(NvAcc, Address, BiosData, &Size, SPI_MEDIA_TYPE);
  if(EFI_ERROR(Status)){
    DEBUG((EFI_D_ERROR, "Read Last Sector:%r\n", Status)); 
    goto ProcExit;
  }

  Status = NvAcc->Erase(NvAcc, Address, SectorSize, SPI_MEDIA_TYPE);
  if(EFI_ERROR(Status)){
    DEBUG((EFI_D_ERROR, "Erase Last Sector:%r\n", Status)); 
    goto ProcExit;
  }

  RomSipAddr   = *(UINT32*)&BiosData[SectorSize - (0xFFFFFFFF - 0xFFFFFFD0 + 1)];
  RomSipOffset = SectorSize - (0xFFFFFFFF - RomSipAddr + 1);
  DEBUG((EFI_D_INFO, "RomSip Offset:%X, Addr:%X\n", RomSipOffset, RomSipAddr));

  //DumpMem32(&gAsiaNbCfg->Romsip_Tbl[0], sizeof(gAsiaNbCfg->Romsip_Tbl));

  /// del it for compiler. tiger 2016-06-12+S
  //CopyMem(
  //  BiosData + RomSipOffset, 
  //  &gAsiaNbCfg->Romsip_Tbl[0], 
  //  sizeof(gAsiaNbCfg->Romsip_Tbl)
  //  );
  /// tiger 2016-06-12+E

  Status = NvAcc->Write(NvAcc, Address, BiosData, SectorSize, SPI_MEDIA_TYPE);
  if(EFI_ERROR(Status)){
    DEBUG((EFI_D_ERROR, "Write Last Sector:%r\n", Status)); 
    goto ProcExit;
  }

//  gAsiaNbCfg->RomsipStatus = 0;

ProcExit:
  if(BiosData != NULL){FreePool(BiosData);}
  return;
}



EFI_STATUS
EFIAPI
PlatformDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS              Status;
  VOID                    *Registration;
  EFI_EVENT               Event;  
   
  
  DEBUG((DEBUG_INFO, "%a()\n", __FUNCTION__));
  ASSERT(sizeof(DATA_64)==8);  
  
  gSetupData = GetSetupDataHobData();
  
  Status = gBS->LocateProtocol(&gAsiaNbProtocolGuid,  NULL, (VOID**)&gPtAsiaNb);
  ASSERT_EFI_ERROR(Status);
  Status = gBS->LocateProtocol(&gAsiaSbProtocolGuid,  NULL, (VOID**)&gPtAsiaSb);
  ASSERT_EFI_ERROR(Status);
  Status = gBS->LocateProtocol(&gAsiaCpuProtocolGuid, NULL, (VOID**)&gPtAsiaCpu);
  ASSERT_EFI_ERROR(Status);  
  gAsiaNbCfg = (ASIA_NB_CONFIGURATION*)gPtAsiaNb->NbCfg;  
  gAsiaSbCfg = (ASIA_SB_CONFIGURATION*)gPtAsiaSb->SbCfg;  	

  PlatformDebugAtEntryPoint(ImageHandle, SystemTable);

  Status = gPtAsiaSb->PrePciInit(gPtAsiaSb);
  ASSERT_EFI_ERROR(Status);
  Status = gPtAsiaNb->PrePciInit(gPtAsiaNb);
  ASSERT_EFI_ERROR(Status);
  DEBUG((EFI_D_ERROR,"mike_LegacyInterruptInstall\n"));
  Status = LegacyInterruptInstall(ImageHandle, SystemTable);  
  ASSERT_EFI_ERROR(Status);
  
  Status = LegacyBiosPlatformInstall(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);  
  DEBUG((EFI_D_ERROR,"mike_SmmAccess2Install\n"));
  Status = SmmAccess2Install(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);
  
  Status = LegacyRegion2Install(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);  
  DEBUG((EFI_D_ERROR,"mike_PciPlatformInstall\n"));
  Status = PciPlatformInstall(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);    

  /// No SATA for CHX001. tiger-2016-06-16+S
  //Status = SataControllerInstall(ImageHandle, SystemTable);
  //ASSERT_EFI_ERROR(Status);
  /// tiger-2016-06-16+E
  Status = IncompatiblePciDeviceSupportEntryPoint(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);  
  
  Status = MiscConfigDxe();
  ASSERT_EFI_ERROR(Status);
  
#if PCAL6416A_PCIE_HOTPLUG_SUPPORT
  Status = PciHotPlugEntryPoint(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);  
#endif
  DEBUG((EFI_D_ERROR,"mike_ExitPmAuthCallBack_S\n"));
  EfiCreateProtocolNotifyEvent (
    &gExitPmAuthProtocolGuid,
    TPL_CALLBACK,
    ExitPmAuthCallBack,
    NULL,
    &Registration
    );   
  DEBUG((EFI_D_ERROR,"mike_ExitPmAuthCallBack_E\n"));
  EfiCreateProtocolNotifyEvent (
    &gBdsAllDriversConnectedProtocolGuid,
    TPL_CALLBACK,
    AllDriversConnectedCallBack,
    NULL,
    &Registration
    );      
  DEBUG((EFI_D_ERROR,"mike_gBdsAllDriversConnectedProtocolGuid_E\n"));
  EfiCreateProtocolNotifyEvent (
    &gEfiSmbiosProtocolGuid,
    TPL_CALLBACK,
    SmbiosCallback,
    NULL,
    &Registration
    );       
  DEBUG((EFI_D_ERROR,"mike_Smbios_E\n")); 
  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             PlatOnReadyToBoot,
             NULL,
             &Event
             ); 
  ASSERT_EFI_ERROR(Status);             
  DEBUG((EFI_D_ERROR,"mike_PlatOnReadyToBoot_E\n")); 
  EfiCreateProtocolNotifyEvent (
    &gEfiSetupEnterGuid,
    TPL_CALLBACK,
    EnterSetupCallBack,
    NULL,
    &Registration
    ); 
  DEBUG((EFI_D_ERROR,"EnterSetupCallBack_E\n")); 
  /// tiger del it. 2016-06-12+S
  //EfiCreateProtocolNotifyEvent (
  //  &gEfiNvMediaAccessSpiReadyGuid,
  //  TPL_CALLBACK,
  //  NvMediaAccessSpiCallBack,
  //  NULL,
  //   &Registration
  //  ); 
  /// 2016-06-12+E
  
  return Status;
}




