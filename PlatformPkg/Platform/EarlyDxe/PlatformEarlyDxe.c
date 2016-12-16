

#include <Uefi.h>
#include <Guid/SetupPassword.h>
#include <Guid/MtcVendor.h>
#include <Library/IoLib.h>
#include <Library/SerialPortLib.h>
#include <Protocol/VariableWrite.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/PciRootBridgeIo.h>                   // AsiaNbProtocol.h +
#include <Protocol/PciHostBridgeResourceAllocation.h>   // AsiaNbProtocol.h +
#include <Protocol/PciIo.h>                             // AsiaNbProtocol.h +
#include <PlatformDefinition.h>
#include <AsiaNbProtocol.h>
#include <CHX001Cfg.h>
#include <AsiaCpuProtocol.h>
#include <SetupVariable.h>
#include <Library/BaseCryptLib.h>
#include <Library/PlatformCommLib.h>
#include <PlatS3Record.h>


extern EFI_GUID gIgdGopDepexProtocolGuid;
extern EFI_GUID gObLanUndiDxeDepexProtocolGuid;
CONST SETUP_DATA *gSetupHob;
TSESETUP gTse;
PLATFORM_S3_RECORD  *gS3Record;
CONST CHAR16 *gDefaultSetupAdminPassword = (CHAR16*)PcdGetPtr(PcdDefaultSetupAdminPassword);




/*
EFI_STATUS 
AllocAlignPageBelow4G (
  UINTN  Size, 
  UINTN  Alignment,
  VOID   **Memory
  )
{
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 Pages;  
  UINTN                 TotalPages;
  EFI_STATUS            Status;
  UINTN                 AlignedAddr;
  UINTN                 Delta;
  UINTN                 PageEnd;


  if(Alignment & 0xFFF){
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit;
  }  

  Address    = 0xFFFFFFFF;
  TotalPages = EFI_SIZE_TO_PAGES(Size + Alignment);
  Status     = gBS->AllocatePages(
                      AllocateMaxAddress,
                      EfiACPIMemoryNVS,
                      TotalPages,
                      &Address
                      );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  DEBUG((EFI_D_INFO, "Pages(%X,%X) L:%X\n", (UINT32)Address, TotalPages, Size));


  AlignedAddr = (UINTN)ALIGN_VALUE(Address, Alignment);

  Delta = AlignedAddr - (UINTN)Address;
  if(Delta){
    Pages = EFI_SIZE_TO_PAGES(Delta);
    DEBUG((EFI_D_INFO, "FreePage(%X,%X)\n", Address, Pages));    
    gBS->FreePages(Address, Pages);
  }  
  
  Pages   = EFI_SIZE_TO_PAGES(Size);
  PageEnd = AlignedAddr + EFI_PAGES_TO_SIZE(Pages);
  Delta   = Address + EFI_PAGES_TO_SIZE(TotalPages) - PageEnd;
  if(Delta){
    Pages = EFI_SIZE_TO_PAGES(Delta);
    DEBUG((EFI_D_INFO, "FreePage(%X,%X)\n", PageEnd, Pages));    
    gBS->FreePages(PageEnd, Pages);
  }  

  *Memory = (VOID*)AlignedAddr;

ProcExit:
  return Status;
}
*/



STATIC
EFI_STATUS
ReadFileFromFv(
  IN  CONST EFI_GUID    *NameGuid,
  IN  EFI_SECTION_TYPE  SectionType,
  IN  UINTN             SectionInstance,
  OUT VOID              **FileData,
  OUT UINTN             *FileDataSize
    );

/*
STATIC EFI_STATUS HandleXhciFw(VOID)
{
  EFI_STATUS            Status;
  VOID                  *Buffer;
  UINTN                 Size;
  VOID                  *HubFw;
  VOID                  *McuFw;
  PLATFORM_S3_RECORD    *S3Record;
  

  if(MmioRead16(XHCI_PCI_REG(PCI_VID_REG)) == 0xFFFF){
    Status = EFI_SUCCESS;
    goto ProcExit;
  }

  Status = ReadFileFromFv (
             (EFI_GUID*)PcdGetPtr(PcdXhciHubFwFile),
             EFI_SECTION_RAW,
             0,
             &Buffer,
             &Size
             );
  ASSERT_EFI_ERROR(Status);
  if(EFI_ERROR(Status)){goto ProcExit;}

  Status = AllocAlignPageBelow4G(Size, SIZE_64KB, &HubFw);
  ASSERT_EFI_ERROR(Status);
  if(EFI_ERROR(Status)){goto ProcExit;}
  DEBUG((EFI_D_INFO, "HubFw:%X\n", HubFw));
  CopyMem(HubFw, Buffer, Size);
  gBS->FreePool(Buffer);


  Status = ReadFileFromFv (
             (EFI_GUID*)PcdGetPtr(PcdXhciMcuFwFile),
             EFI_SECTION_RAW,
             0,
             &Buffer,
             &Size
             );
  ASSERT_EFI_ERROR(Status);
  if(EFI_ERROR(Status)){goto ProcExit;}

  Status = AllocAlignPageBelow4G(Size, SIZE_64KB, &McuFw);
  ASSERT_EFI_ERROR(Status);
  if(EFI_ERROR(Status)){goto ProcExit;}
  DEBUG((EFI_D_INFO, "McuFw:%X\n", McuFw));  
  CopyMem(McuFw, Buffer, Size);
  gBS->FreePool(Buffer);


  Status = LoadXhciFw(HubFw, McuFw);
  ASSERT_EFI_ERROR(Status);

  ASSERT((UINT64)(UINTN)McuFw < SIZE_4GB);
  ASSERT((UINT64)(UINTN)HubFw < SIZE_4GB);
  S3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  S3Record->XhciHubFw = (UINT32)(UINTN)HubFw;
  S3Record->XhciMcuFw = (UINT32)(UINTN)McuFw;

ProcExit:
  return Status;
}
*/


STATIC EFI_STATUS HandleXhciFw(VOID)
{
  /// TGR-2016062201. SOC not include XHCI Controller
#if 0
  EFI_STATUS            Status;
  VOID                  *Buffer;
  UINTN                 Size;
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 Pages;
  VOID                  *HubFw;
  VOID                  *McuFw;
  UINT32                Crc32;
  

  if(MmioRead16(XHCI_PCI_REG(PCI_VID_REG)) == 0xFFFF){
    Status = EFI_SUCCESS;
    goto ProcExit;
  }

  Status = ReadFileFromFv (
             (EFI_GUID*)PcdGetPtr(PcdXhciHubFwFile),
             EFI_SECTION_RAW,
             0,
             &Buffer,
             &Size
             );
  ASSERT_EFI_ERROR(Status);
  if(EFI_ERROR(Status)){goto ProcExit;}
  Address = 0xFFFFFFFF;
  Pages   = Size + SIZE_64KB;                           // need 64K align
  Pages   = EFI_SIZE_TO_PAGES(Pages);
  Status  = gBS->AllocatePages(
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  Pages,
                  &Address
                  );
  ASSERT_EFI_ERROR(Status);
  HubFw = (VOID*)(UINTN)ALIGN_VALUE(Address, SIZE_64KB);
  CopyMem(HubFw, Buffer, Size);
  gBS->FreePool(Buffer);
  LibCalcCrc32(HubFw, Size, &Crc32);
  gS3Record->XhciHubFwCrc32 = Crc32;
  gS3Record->XhciHubFwSize  = (UINT32)Size;

  Status = ReadFileFromFv (
             (EFI_GUID*)PcdGetPtr(PcdXhciMcuFwFile),
             EFI_SECTION_RAW,
             0,
             &Buffer,
             &Size
             );
  ASSERT_EFI_ERROR(Status);
  if(EFI_ERROR(Status)){goto ProcExit;}
  Address = 0xFFFFFFFF;
  Pages   = Size + SIZE_64KB;                           // need 64K align
  Pages   = EFI_SIZE_TO_PAGES(Pages);
  Status  = gBS->AllocatePages(
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  Pages,
                  &Address
                  );
  ASSERT_EFI_ERROR(Status);
  McuFw = (VOID*)(UINTN)ALIGN_VALUE(Address, SIZE_64KB);
  CopyMem(McuFw, Buffer, Size);
  gBS->FreePool(Buffer);
  LibCalcCrc32(McuFw, Size, &Crc32);
  gS3Record->XhciMcuFwCrc32 = Crc32;
  gS3Record->XhciMcuFwSize  = (UINT32)Size;  

  Status = LoadXhciFw(HubFw, McuFw);
  ASSERT_EFI_ERROR(Status);

  ASSERT((UINT64)(UINTN)McuFw < SIZE_4GB);
  ASSERT((UINT64)(UINTN)HubFw < SIZE_4GB);

  gS3Record->XhciHubFw = (UINT32)(UINTN)HubFw;
  gS3Record->XhciMcuFw = (UINT32)(UINTN)McuFw;

ProcExit:
  return Status;
#else
  return EFI_SUCCESS;
#endif
}




VOID
AsiaNbPtCallBack (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_ASIA_NB_PROTOCOL  *ptAsiaNb;
  ASIA_NB_CONFIGURATION *AsiaNbCfg;
  EFI_STATUS            Status;
  VOID                  *Buffer;
  UINTN                 Size;

  Status = gBS->LocateProtocol(&gAsiaNbProtocolGuid, NULL, &ptAsiaNb);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  gBS->CloseEvent(Event);

  if(MmioRead16(IGD_PCI_REG(PCI_VID_REG)) == 0xFFFF){
    goto ProcExit;
  }  
  
  Status = ReadFileFromFv (
             (EFI_GUID*)PcdGetPtr(PcdNbVcpFile),
             EFI_SECTION_RAW,
             0,
             &Buffer,
             &Size
             );
  ASSERT_EFI_ERROR(Status);
  if(EFI_ERROR(Status)){
    return;
  }

  AsiaNbCfg = (ASIA_NB_CONFIGURATION*)ptAsiaNb->NbCfg;
  ASSERT((UINT64)(UINTN)Buffer < SIZE_4GB);
  AsiaNbCfg->VcpFileBaseAddr = (UINT32)(UINTN)Buffer;   // ASIA will free this.
  AsiaNbCfg->VcpFileSize     = (UINT16)Size;
  
ProcExit:
  return;  
}


STATIC
EFI_STATUS
ReadFileFromFv(
  IN  CONST EFI_GUID    *NameGuid,
  IN  EFI_SECTION_TYPE  SectionType,
  IN  UINTN             SectionInstance,
  OUT VOID              **FileData,
  OUT UINTN             *FileDataSize
    )
{
  UINTN                         FvHandleCount;
  EFI_HANDLE                    *FvHandleBuffer;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv2;
	UINT32                        AuthenticationStatus;
  EFI_FV_FILETYPE               FoundType;
  UINT32                        FvStatus;
  UINT32                        Attributes;
  VOID                          *MyFileData;
  UINTN                         MyFileDataSize;
  EFI_STATUS                    Status;
  UINTN                         Index;


  Fv2 = NULL;
	FvHandleCount  = 0;
	FvHandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &FvHandleCount,
                  &FvHandleBuffer
                  );
  if(EFI_ERROR(Status) || FvHandleCount==0){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }

  for (Index = 0; Index < FvHandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    FvHandleBuffer[Index],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID**)&Fv2
                    );
    ASSERT(!EFI_ERROR(Status));
    Status = Fv2->ReadFile (
                  Fv2,
                  NameGuid,
                  NULL,
                  &MyFileDataSize,
                  &FoundType,
                  &Attributes,
                  &AuthenticationStatus
                  );
    if (!EFI_ERROR(Status)) {
      break;
    }
  }
  if(Index >= FvHandleCount){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }

  MyFileData     = NULL;  //!!! NULL means memory allocated by callee.
  MyFileDataSize = 0;
  Status = Fv2->ReadSection (
                    Fv2,
                    NameGuid,
                    SectionType,
                    SectionInstance,
                    &MyFileData,
                    &MyFileDataSize,
                    &FvStatus
                    );
  if (!EFI_ERROR(Status)) {
    *FileData     = MyFileData;
    *FileDataSize = MyFileDataSize;
  }

ProcExit:
  if(FvHandleBuffer != NULL){
    gBS->FreePool(FvHandleBuffer);
  }
  return Status;
}

// JIH-Add PEMCU Option - s
STATIC EFI_STATUS HandlePeMcuFw(VOID)
{
  EFI_STATUS            Status;
  VOID                  *Buffer;
  UINTN                 Size;
 EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 Pages;
  VOID                  *PeMcuFw;
   VOID                  *PeMcuData;
  UINTN               PCIEPhyMMIOBase;
  UINT32           FwAddr;
  UINT8 		TmpReg;
  UINT16		TmpReg16;
  //UINT32		TmpReg32;


  /* VOID                  *McuFw;
  PLATFORM_NV_INFO      *NvInfo;
  EFI_HOB_GUID_TYPE     *GuidHob;
  UINT64                Tsc;*/



  Status = ReadFileFromFv (
             (EFI_GUID*)PcdGetPtr(PcdPeMcuFwFile),
             EFI_SECTION_RAW,
             0,
             &Buffer,
             &Size
             );
  ASSERT_EFI_ERROR(Status);

  if(EFI_ERROR(Status)){goto ProcExit;}
  //allocate mcu instruction space
  Address = 0xFFFFFFFF;
  Pages   = Size + SIZE_64KB;                           		// need 64K align
  Pages   = EFI_SIZE_TO_PAGES(Pages);
  Status  = gBS->AllocatePages(
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  Pages,
                  &Address
                  );
  ASSERT_EFI_ERROR(Status);
  
#if 1      									
  PeMcuFw = (VOID*)(UINTN)ALIGN_VALUE(Address, SIZE_64KB);		
#else
  PeMcuFw = (void*)((UINT64)0x20000000);						//Appoint address	
#endif

  DEBUG((EFI_D_ERROR, " PeMcuFw = %x  FwSize = %d[0x%x]Byte\n",PeMcuFw,Size,Size));
  CopyMem(PeMcuFw, Buffer, Size);								//Copy FW from Buffer to the space we allocated
  gBS->FreePool(Buffer);

  //allocate mcu data space
  Address = 0xFFFFFFFF;
  Pages   = Size + SIZE_64KB;                           		// need 64K align
  Pages   = EFI_SIZE_TO_PAGES(Pages);
  Status  = gBS->AllocatePages(			
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  Pages,
                  &Address
                  );
  ASSERT_EFI_ERROR(Status);
  PeMcuData = (VOID*)(UINTN)ALIGN_VALUE(Address, SIZE_64KB);
  gBS->SetMem(PeMcuData,SIZE_64KB,0);

  // Jimmy Debug: set pcie ephy base address
  //MmioWrite32((0xE0005000|PEMCU_MMIO_BAR_ADDR),0x00FE0140); //TGR del it, ASIA has set it in PEIM.

  PCIEPhyMMIOBase=(UINTN)MmioRead32((PCI_DEV_MMBASE(0,0,5)+D0F5_PCIE_EPHY_BASE_ADR)); 	//get base address
  PCIEPhyMMIOBase = PCIEPhyMMIOBase<<8;
  // PCIEPhyMMIOBase = 0xFE014000;
  DEBUG((DEBUG_ERROR," PCIEPhyMMIOBase: %x\n", PCIEPhyMMIOBase));
  MmioWrite16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_START_ADR),0);						//set firmware auto fill address
  TmpReg16 = MmioRead16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_START_ADR));  // Jimmy Debug - For cache flush
  DEBUG((DEBUG_ERROR," PCIEPHYCFG_PEMCU_AUTO_FILL_START_ADR: %x\n", TmpReg16));
  MmioWrite16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_LEN),SIZE_8KB);					//set firmware auto fill length 8k?
  TmpReg16 = MmioRead16((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_AUTO_FILL_LEN));  // Jimmy Debug - For cache flush
  DEBUG((DEBUG_ERROR," PCIEPHYCFG_PEMCU_AUTO_FILL_LEN: %x\n", TmpReg16));
  
  FwAddr= (UINT32)((UINT64)((UINT32*)PeMcuFw));
  //FwAddr= (UINT32)(*(UINT32*)PeMcuFw);
  DEBUG((EFI_D_ERROR, "[Line:%d] FwAddr = %x\n",__LINE__,FwAddr));
  FwAddr=FwAddr>>16;
  DEBUG((EFI_D_ERROR, "[Line:%d] FwAddr = %x\n",__LINE__,FwAddr));
  MmioWrite32((PCIEPhyMMIOBase|PCIEPHYCFG_BASE_ADR_OF_PEMCU_FW_FOR_INSTRUCTION),FwAddr); //set instruction base address
  MmioRead32((PCIEPhyMMIOBase|PCIEPHYCFG_BASE_ADR_OF_PEMCU_FW_FOR_INSTRUCTION));  // Jimmy Debug - For cache flush
  //Flush cache[xxx]
  AsmWbinvd();

  DEBUG((EFI_D_ERROR, "[Line:%d] Flush Cache\n",__LINE__));

  
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE),PCIEPHYCFG_INST_AUTOFILL_EN); 			//auto fill enable
  TmpReg=0;
  while((TmpReg&0x02)==0) //wait for auto fill ready
  {
  	TmpReg=MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE));
  }
  TmpReg=MmioRead8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE));
  DEBUG((EFI_D_ERROR, "[Line:%d] Fill done status = %X\n",__LINE__, TmpReg)); // Jimmy Debug - For cache flush

  DEBUG((EFI_D_ERROR, "[Line:%d] FW AutoFill Done\n",__LINE__));
  
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE),0x00); 	
  MmioWrite8((PCIEPhyMMIOBase|PCIEPHYCFG_PEMCU_RESET_AND_INSTRUCTION_AUTO_FILL_ENABLE_AND_DONE),PCIEPHYCFG_MCU_RST); //reset mcu

#if 0								 ///Add for Redo-EQ in DXE stage.  	
   
  ///For Debug - Add by ChrisJWang 2015.07.31 
  ///[Possible Bug:When No UART debug message output, Pemcu AutoFill Failed]
  ///Those Code can check whether FW autofill success 
  gBS->Stall(100); 					//Wait 100us
  TmpReg = MmioRead8(PCIEPhyMMIOBase+PEMCU_RESET_AUTOFILL_EN);
  if(TmpReg == 0x00){
  	DEBUG((EFI_D_ERROR, "[AutoFill-001] AutoFill Failed\n"));
	while(1){
		IoWrite8(0x80,0x52); 						//80 Port show 0x52	
	}
  } 

  //After loaded,FW shold set RxCCC=0x22
  TmpReg = 0x00;
  while(TmpReg != 0x22){
  	  //wait until it turn to 0x22(It mean FW runing successful)
	  TmpReg = MmioRead8(PCIEPhyMMIOBase + PCIEPHYCFG_PEMCU2BIOS_SCRATCH);
	  //DEBUG((EFI_D_ERROR, "[AutoFill-001] AutoFill Pass But  RxCCD!=0x22\n"));
	  DEBUG((EFI_D_ERROR, "[AutoFill-001] AutoFill Pass But FW not running\n"));
	  IoWrite8(0x80,0x53);							//80 Port show 0x53
  }
  DEBUG((EFI_D_ERROR, "[AutoFill-001] AutoFill Pass and FW running success\n"));

  //Trigger Redo EQ  -Add by ChrisJWang 2015.08.31
  MmioWrite8(0xE0010000 + 0x70,0x02);
  do{
  	  //wait 
	  TmpReg = MmioRead8(0xE0010000 + 0x52);
	  DEBUG((EFI_D_ERROR, "[AutoFill-001] Waiting for speed to Gen2\n"));
	  IoWrite8(0x80,0x54);							//80 Port show 0x53
  }while((TmpReg&0xF) != 0x02);
  DEBUG((EFI_D_ERROR, "[AutoFill-001] Speed to Gen2\n"));

  //gBS->Stall(5000000);
  
  MmioWrite8(0xE0010000 + 0x70,0x03);  //speed to gen3 : RedoEQ
  do{
  	  //wait 
	  TmpReg = MmioRead8(0xE0010000 + 0x52);
	  DEBUG((EFI_D_ERROR, "[AutoFill-001] Waiting for speed to Gen3\n"));
	  IoWrite8(0x80,0x55);							//80 Port show 0x53
  }while((TmpReg&0xF) != 0x03);
  DEBUG((EFI_D_ERROR, "[AutoFill-001] Speed to Gen3\n"));


#endif
 
  DEBUG((EFI_D_ERROR, "[Line:%d] Exit %a()\n", __LINE__, __FUNCTION__));
 
  /*Those code for EQ procedure - ChrisJWang*/	
  /*
  //But those code seem no use??? -- ChrisJWang 2015.07.28
  //Those Setting should be set in VISA NbPcieInit-PhyDriving.
  MmioWrite32((PCIEPhyMMIOBase+0x166C),0x6C2C1B11);
  MmioWrite32((PCIEPhyMMIOBase+0x176C),0x6C2C1B11);
  MmioWrite32((PCIEPhyMMIOBase+0x1678),0x6C2C1B11);
  MmioWrite32((PCIEPhyMMIOBase+0x1778),0x6C2C1B11);

  MmioWrite32((PCIEPhyMMIOBase+0x1878),0x6c1a1b1a);
  MmioWrite32((PCIEPhyMMIOBase+0x1978),0x6c1a1b1a);
  
  MmioWrite32((PCIEPhyMMIOBase+0x1A6C),0x6c3d1b00);
  MmioWrite32((PCIEPhyMMIOBase+0x1B6C),0x6c3d1b00);
  MmioWrite32((PCIEPhyMMIOBase+0x1A78),0x6c3d1b00);
  MmioWrite32((PCIEPhyMMIOBase+0x1B78),0x6c3d1b00);
  */


  ProcExit:
  return Status;
}
// JIH-Add PEMCU Option - e

VOID ClearCmosBad()
{
  UINT8  Cmos0E;

  Cmos0E = CmosRead(0xE);
  if(Cmos0E & BIT7){
    CmosWrite(0xE, Cmos0E & (UINT8)~BIT7);
  }
}


EFI_STATUS
EFIAPI
Sha256Hash (
  IN  CONST VOID        *Data,
  IN        UINTN       DataLen,
  OUT       UINT8       *Digest,
  IN        UINTN       DigestSize  
  )
{
  VOID     *Sha256Ctx;
  UINTN    CtxSize;

  ASSERT(Data != NULL && DataLen != 0 && Digest != NULL && DigestSize >= 32);

  CtxSize = Sha256GetContextSize();
  Sha256Ctx = AllocatePool(CtxSize);
  ASSERT(Sha256Ctx != NULL);

  Sha256Init (Sha256Ctx);
  Sha256Update (Sha256Ctx, Data, DataLen);
  Sha256Final (Sha256Ctx, Digest);

  FreePool(Sha256Ctx);

  return EFI_SUCCESS;
}



VOID
VariableWriteReadyCallBack (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  VOID                 *Interface;
  EFI_STATUS           Status;
  UINTN                VarSize;
  TSESETUP             *Tse;
  BOOLEAN              IsCmosBad;


  Status = gBS->LocateProtocol(&gEfiVariableWriteArchProtocolGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }
  gBS->CloseEvent(Event);


//------------------------------------------------------------------------------
  if(PcdGetBool(PcdVarServiceUseEmu)){
    Status = gRT->SetVariable (
                    PLATFORM_SETUP_VARIABLE_NAME,
                    &gPlatformSetupVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof(SETUP_DATA),
                    (VOID*)gSetupHob
                    );
    DEBUG((EFI_D_INFO, "[EMU] Sync SetupData:%r\n"));		
  }

  IsCmosBad = (CmosRead(0x0E) & BIT7)?TRUE:FALSE;

  VarSize = sizeof(TSESETUP);
  Tse     = &gTse;
  Status  = gRT->GetVariable (
                  SETUP_PASSWORD_VARTSE_NAME,
                  &gSetupPasswordVariableTseGuid,
                  NULL,
                  &VarSize,
                  Tse
                  );
  if(Status == EFI_NOT_FOUND || IsCmosBad){
    VarSize = sizeof(TSESETUP);
    ZeroMem(Tse, VarSize);
    
    if(gDefaultSetupAdminPassword[0]){
      Sha256Hash(
        gDefaultSetupAdminPassword, 
        StrSize(gDefaultSetupAdminPassword), 
        Tse->AdminPasswdHash, 
        sizeof(Tse->AdminPasswdHash)
        );
      Tse->AdminPasswdPresent = TRUE;
    }

    Status = gRT->SetVariable (
                    SETUP_PASSWORD_VARTSE_NAME,
                    &gSetupPasswordVariableTseGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    VarSize,
                    Tse
                    );
  }
  ASSERT_EFI_ERROR(Status);






  if(IsCmosBad){ClearCmosBad();}	
}


typedef struct {
  UINT32  StartAddr;
  UINT32  RangeSize;
}	EFI_RESERVED_MEMORY_LIST;

/*
[037]    MMIO 00000000FE011000 00000000FE012FFF 0000000000002000 UC RT
[038]    MMIO 00000000FE014000 00000000FE015FFF 0000000000002000 UC RT
[039]    MMIO 00000000FEC00000 00000000FEC3FFFF 0000000000040000 UC RT
[040]    MMIO 00000000FECC0000 00000000FED7FFFF 00000000000C0000 UC RT
[041]    MMIO 00000000FEE00000 00000000FEEFFFFF 0000000000100000 UC RT
[042]    MMIO 00000000FF800000 00000000FFFFFFFF 0000000000800000 UC RT
*/
STATIC EFI_RESERVED_MEMORY_LIST gRsvdMemList[] = {
  {0xFE011000, 0x00002000},   // RCRB V+H
  {0xFE014000, 0x00004000},   // PcieEPHY 
  {0xFE020000, 0x00001000},// CRMCA Bar  
  {0xFEC00000, 0x00040000},   // ApicBar
  {0xFECC0000, 0x000C0000},   // NbApicBaseAddress
  {0xFEE00000, 0x00100000},   // CPU_MSI_MEMORY_BASE
  {0xFEF00000, 0X00100000},  // EX-IPI range
  {_PCD_VALUE_PcdPciExpressBaseAddress, SIZE_256MB}, // E000 0000 -- F000 0000
  {_PCD_VALUE_PcdFlashAreaBaseAddress, _PCD_VALUE_PcdFlashAreaSize},   // bios, FFC0 0000, Size = 4MB
};  


VOID SetReservedMmio()
{
  UINTN       Index;  
  EFI_STATUS  Status;
  
  for(Index = 0; Index < sizeof(gRsvdMemList)/sizeof(gRsvdMemList[0]); Index++){
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeReserved,
                    gRsvdMemList[Index].StartAddr,
                    gRsvdMemList[Index].RangeSize,
                    EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                    );
    ASSERT_EFI_ERROR(Status);

    Status = gDS->SetMemorySpaceAttributes(
                    gRsvdMemList[Index].StartAddr, 
                    gRsvdMemList[Index].RangeSize,
                    EFI_MEMORY_RUNTIME
                    );
    DEBUG((EFI_D_INFO, "[%d] %X %X %r(%lX)\n", Index, \
      gRsvdMemList[Index].StartAddr, gRsvdMemList[Index].RangeSize, Status, (UINT64)Status));    
    ASSERT_EFI_ERROR(Status);    
  }
}


EFI_STATUS
EFIAPI
PlatformEarlyDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status = EFI_SUCCESS;
  VOID                          *Registration;
  EFI_BOOT_MODE                 BootMode;
  EFI_HANDLE                    Handle = NULL;
  UINT8                         Data8;	
  UINT32                        SystemMiscCfg;


  SetReservedMmio();

  BootMode = GetBootModeHob();
  DEBUG((EFI_D_INFO, "BootMode:%X\n", BootMode));

  gSetupHob = GetSetupDataHobData();
  gS3Record = (PLATFORM_S3_RECORD*)(UINTN)PcdGet32(PcdS3RecordAddr);
  ASSERT(gS3Record->Signature == PLAT_S3_RECORD_SIGNATURE);

// JIH-Add PEMCU Option - s
#if 1
//open this for enable EQ FW load
if(gSetupHob->PEMCU_LoadFW_WhenBoot){
	DEBUG((DEBUG_ERROR,"\nPCIe PeMcu FW...\n"));  //ChrisJWang Add for Debug
  	Status=HandlePeMcuFw();
       ASSERT_EFI_ERROR(Status);
}
#endif
  // JIH-Add PEMCU Option - e

  if (BootMode != BOOT_IN_RECOVERY_MODE) {
    Status = HandleXhciFw(); // for Normal Boot, will load XHCI FW.
    ASSERT_EFI_ERROR(Status);
  }

  PcdSet8(PcdLegacyBiosSupport, gSetupHob->Csm);
  PcdSet8(PcdDisplayOptionRomMessage, gSetupHob->OpromMessage);
  PcdSet8(PcdUCREnable, gSetupHob->UCREnable);
  PcdSetBool(PcdSerialTerminalSupport, gSetupHob->UCREnable);	


  SystemMiscCfg = PcdGet32(PcdSystemMiscConfig);
  if(gSetupHob->VideoDualVga){
    SystemMiscCfg |= SYS_MISC_CFG_DUAL_VGA;
  }  
  if(gSetupHob->VideoPrimaryAdapter == DISPLAY_PRIMARY_IGD){
    SystemMiscCfg |= SYS_MISC_CFG_PRI_VGA_IGD;
  }
  if(gSetupHob->Pci64){
    SystemMiscCfg |= SYS_MISC_CFG_PCI64;
  }  
  PcdSet32(PcdSystemMiscConfig, SystemMiscCfg);

  

  switch(gSetupHob->SMRREnable){
    case SMRR_TYPE_DISABLE:
      gS3Record->SmrrType = 0xFF;
      break;
      
    default:
    case SMRR_TYPE_WT:
      gS3Record->SmrrType = CacheWriteThrough;
      break;

    case SMRR_TYPE_WB:
      gS3Record->SmrrType = CacheWriteBack;
      break;       
  }

#ifdef ZX_SECRET_CODE
    ///xxx
#else
    ////
#endif
// 0 - all
// 1 - Uefi only
// 2 - Legacy only
  Data8 = gSetupHob->BootModeType;
  if(!gSetupHob->Csm){Data8 = 1;}
  PcdSet8(PcdBiosBootModeType, Data8);

// default FALSE.
  if(BootMode == BOOT_IN_RECOVERY_MODE || BootMode == BOOT_ON_FLASH_UPDATE){
    PcdSetBool(PcdVarServiceUseEmu, TRUE);
  }	

  if (!gSetupHob->Csm) {              // EFI only
    gBS->InstallProtocolInterface (
           &Handle,
           &gIgdGopDepexProtocolGuid,
           EFI_NATIVE_INTERFACE,
           NULL
           );
    DEBUG((EFI_D_INFO, "InsIgdGopDpx\n"));
  }

// csm LegacyPXE
//   0       N/A    (v)
//   1         0    (v)
//   1         1    (x)
  if(gSetupHob->OnboardLan && 
    !(gSetupHob->Csm && gSetupHob->LegacyPxeBoot) && 
    (gSetupHob->UefiPxeIPV4 || gSetupHob->UefiPxeIPV6)){
    gBS->InstallProtocolInterface (
           &Handle,
           &gObLanUndiDxeDepexProtocolGuid,
           EFI_NATIVE_INTERFACE,
           NULL
           );
  }    

  EfiCreateProtocolNotifyEvent (
    &gAsiaNbProtocolGuid,
    TPL_CALLBACK,
    AsiaNbPtCallBack,
    NULL,
    &Registration
    );

  EfiCreateProtocolNotifyEvent (
    &gEfiVariableWriteArchProtocolGuid,
    TPL_CALLBACK,
    VariableWriteReadyCallBack,
    NULL,
    &Registration
    );

  DEBUG((EFI_D_INFO, "PlatformEarlyDxe Exit\n"));

  return EFI_SUCCESS;
}



