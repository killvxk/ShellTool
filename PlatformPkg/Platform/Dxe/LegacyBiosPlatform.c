
#include "EVB_PIRT.h"
#include <IndustryStandard/Pci.h>
#include <Protocol/SwSmiValuePolicyData.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/PciIo.h>
#include <Protocol/LegacyOptionRomInfo.h>
#include <SetupVariable.h>
#include <Csm/LegacyBiosDxe/LegacyBiosInterface.h>

#define ROM_MAX_ENTRIES               24
#define VGA_HANDLE_BUF_MAX_COUNT      0x20
#define IDE_HANDLE_BUF_MAX_COUNT      0x20
#define ISA_HANDLE_BUF_MAX_COUNT      0x20

#define EFI_AHCI_BAR_INDEX                                  5
#define VESA_BIOS_EXTENSIONS_RETURN_CONTROLLER_INFORMATION  0x4f00
#define VESA_BIOS_EXTENSIONS_VERSION_2_0                    0x0200
#define VESA_BIOS_EXTENSIONS_STATUS_SUCCESS                 0x004f
#define VESA_BIOS_EXTENSIONS_VESA_SIGNATURE                 SIGNATURE_32 ('V', 'E', 'S', 'A')
#define VESA_BIOS_EXTENSIONS_VBE2_SIGNATURE                 SIGNATURE_32 ('V', 'B', 'E', '2')

#pragma pack(1)
typedef struct {
  UINT32  VESASignature;      // 'VESA' 4 byte signature
  UINT16  VESAVersion;        // VBE version number
  UINT32  OEMStringPtr;       // Pointer to OEM string
  UINT32  Capabilities;       // Capabilities of video card
  UINT32  VideoModePtr;       // Pointer to an array of 16-bit supported modes values terminated by 0xFFFF
  UINT16  TotalMemory;        // Number of 64kb memory blocks
  UINT16  OemSoftwareRev;     // VBE implementation Software revision
  UINT32  OemVendorNamePtr;   // VbeFarPtr to Vendor Name String
  UINT32  OemProductNamePtr;  // VbeFarPtr to Product Name String
  UINT32  OemProductRevPtr;   // VbeFarPtr to Product Revision String
  UINT8   Reserved[222];      // Reserved for VBE implementation scratch area
  UINT8   OemData[256];       // Data area for OEM strings.  Pad to 512 byte block size
} VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK;
#pragma pack()

typedef struct {
  UINT8   *Address;
  UINT8   *Data;
  UINT8   *RtData;
  UINT8   *RtDataAddr;
  UINT16  RtDataSize;
} SAVED_PCI_ROM;

typedef struct {
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINTN                 PortCount;
  UINT64                NativeFis[32];
  UINT64                NativeCmdList[32];  
  UINT64                LegacyFis[32];
  UINT64                LegacyCmdList[32];
} AHCI_FIS_CMDLIST_SAVE;  

#define AHCI_ADDR_SAVE_MAX_HOST_COUNT          4

EFI_HANDLE  gVgaHandles[VGA_HANDLE_BUF_MAX_COUNT] = {NULL};
EFI_HANDLE  gIdeHandles[IDE_HANDLE_BUF_MAX_COUNT];
EFI_HANDLE  gIsaHandles[ISA_HANDLE_BUF_MAX_COUNT];

AHCI_FIS_CMDLIST_SAVE               *gAhciAddrSaveArray = NULL;
UINTN                               gAhciAddrSaveCurCount = 0;
UINTN                               mSavedCount = 0;
SAVED_PCI_ROM                       mSaveRomEntry[ROM_MAX_ENTRIES];
UINT16                              mBbsRomSegment;
LEGACY_PNP_EXPANSION_HEADER         *mBasePnpPtr;
UINT16                              mEbdaSegOrg;
UINT8                               mEbdaSizeOrg;
BOOLEAN                             mInt13VectorSaved = FALSE;
UINT32                              mInt13Vector = 0;
BOOLEAN                             mVideoModeSave = FALSE;
UINT16                              mVideoMode    = 0xFFFF;
BOOLEAN                             mNativeAhciDriver = FALSE;


UINT16 gTotalEntryNum = 0;
UINT16 gPirqTableSize = 0;


///MTN-20161010-S
VOID CollectPCIBusInfo (VOID)
{
  EFI_STATUS Status;
  EFI_HANDLE *pHandleBuffer;
  UINTN      NumberOfHandles;
  UINTN      i;
  EFI_PCI_IO_PROTOCOL *pPciIoProtocol;
  UINT8   PciData[4];

  //Yankui-add-s
  UINTN  Index;
  UINTN  Seg, Bus, Dev, Fun;
  //Yankui-add-e

  DEBUG((EFI_D_ERROR,"[MTN-DBG]:CollectPCIBusInfo In \n"));

  //
  // detect all other buses via PCI bridges
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol, 
                  &gEfiPciIoProtocolGuid,
                  NULL, 
                  &NumberOfHandles, 
                  &pHandleBuffer
                  );
  if (EFI_ERROR (Status))
    return;

  for (i = 0; i < NumberOfHandles; i++) {
    Status = gBS->HandleProtocol (
                    pHandleBuffer[i], 
                    &gEfiPciIoProtocolGuid,
                    (VOID**) &pPciIoProtocol
                    );
    if (EFI_ERROR (Status))
      continue;

    //
    // Read class code information at 0x8 offset in PCI header
    Status = pPciIoProtocol->Pci.Read (
                                   pPciIoProtocol, 
                                   EfiPciIoWidthUint32,
                                   0x8, 
                                   1, 
                                   (VOID*) PciData
                                   );
    if (EFI_ERROR (Status))
      continue;

    //
    // not bridge device
    //
    if (PciData[3] != 0x6)
      continue;

    //
    // PCI host bridge we already know, so skip it or unknown bridge - skip it also
    //
    if ((PciData[2] == 0) || (PciData[2] > 6)) 
      continue;                                

    //
    // PCI to PCI bridge
    //
    if (PciData[2] == 4) {
      //
      // Read primary and secondary bus numbers
      //
      Status = pPciIoProtocol->Pci.Read (
                                     pPciIoProtocol, 
                                     EfiPciIoWidthUint32,
                                     0x18, 
                                     1, 
                                     (VOID*) PciData
                                     );
      //

	//YKN_20160309 +S
	pPciIoProtocol->GetLocation(pPciIoProtocol, &Seg, &Bus, &Dev, &Fun);
	DEBUG((EFI_D_ERROR,"[MTN-DBG]:Seg=0x%x Bus=0x%x Dev=0x%x Fun=0x%x \n",Seg,Bus,Dev,Fun));


	for(Index=0; Index < PCI_BRIDGES_NUM; Index++) {
		if((Bus == gPlatformPciBridgesInfo[Index].Bus) &&
		(Dev == gPlatformPciBridgesInfo[Index].Dev) &&(Fun == gPlatformPciBridgesInfo[Index].Fun)) {
	                gPlatformPciBridgesInfo[Index].Enabled = 1;
		gPlatformPciBridgesInfo[Index].PriBus = PciData[0];
		gPlatformPciBridgesInfo[Index].SecBus = PciData[1];
		break;
	}
	}

	////ASSERT(Index < PCI_BRIDGES_NUM);
	  //YKN_20160309 +E
    } 
}
}
///MTN-20161010-E



EFI_STATUS
EFIAPI
GetPlatformInfo (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL            *This,
  IN EFI_GET_PLATFORM_INFO_MODE                   Mode,
  OUT VOID                                        **Table,
  OUT UINTN                                       *TableSize,
  OUT UINTN                                       *Location,
  OUT UINTN                                       *Alignment,
  IN  UINT16                                      LegacySegment,
  IN  UINT16                                      LegacyOffset
)
{
  EFI_STATUS    Status;
  DEBUG((EFI_D_INFO, "%a() M:%d\n", __FUNCTION__, Mode));
  
  switch (Mode) {
    case EfiGetPlatformBinarySystemRom:
      Status = GetSectionFromFv (
                 PcdGetPtr(PcdCsm16File),
                 EFI_SECTION_RAW,
                 0,
                 Table,
                 TableSize
               );
      break;
    
    case EfiGetPlatformPciExpressBase:
      DEBUG((EFI_D_ERROR,"\n **** \n Get PcdPciExpressBaseAddress \n"));		
      *Location = (UINTN)PcdGet64(PcdPciExpressBaseAddress);
      Status = EFI_SUCCESS;
      break;

    default:
      *Table     = NULL;
      *TableSize = 0;
      *Location  = 0;
      *Alignment = 0;
      Status = EFI_UNSUPPORTED;
      break;
  }

  return Status;
}


EFI_STATUS
FillHddInfo (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  OUT HDD_INFO             *HddInfo,
  OUT UINTN                *ChannelCount
  )
{
  EFI_STATUS  Status;
  PCI_TYPE00  PciConfigHeader;  
  UINTN       Segment;
  UINTN       Bus;
  UINTN       Device;
  UINTN       Function;
  UINT16      BusMasterAddress;  
 
  
  *ChannelCount = 0;
  
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof(PciConfigHeader) / sizeof(UINT32),
                        &PciConfigHeader
                        );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  Status = PciIo->GetLocation (
                    PciIo,
                    &Segment,
                    &Bus,
                    &Device,
                    &Function
                    );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }

  HddInfo->Status  |= HDD_PRIMARY;
  HddInfo->Bus      = (UINT32)Bus;
  HddInfo->Device   = (UINT32)Device;
  HddInfo->Function = (UINT32)Function;

  if (PciConfigHeader.Hdr.ClassCode[0] & BIT0) {
    HddInfo->CommandBaseAddress = (UINT16)(PciConfigHeader.Device.Bar[0] & 0xfffc);
    HddInfo->ControlBaseAddress = (UINT16)((PciConfigHeader.Device.Bar[1] & 0xfffc)+2);
    HddInfo->HddIrq             = PciConfigHeader.Device.InterruptLine;
  } else {
    HddInfo->CommandBaseAddress = 0x1f0;
    HddInfo->ControlBaseAddress = 0x3f6;
    HddInfo->HddIrq             = 14;    
  }
  HddInfo->BusMasterAddress = (UINT16)(PciConfigHeader.Device.Bar[4] & 0xfffc);  
  BusMasterAddress = HddInfo->BusMasterAddress;
  (*ChannelCount)++;
  
  HddInfo++;
  HddInfo->Status  |= HDD_SECONDARY;
  HddInfo->Bus      = (UINT32)Bus;
  HddInfo->Device   = (UINT32)Device;
  HddInfo->Function = (UINT32)Function;
  HddInfo->BusMasterAddress = BusMasterAddress + 8;
  if (PciConfigHeader.Hdr.ClassCode[0] & BIT2) {
    HddInfo->CommandBaseAddress = (UINT16)(PciConfigHeader.Device.Bar[2] & 0xfffc);
    HddInfo->ControlBaseAddress = (UINT16)((PciConfigHeader.Device.Bar[3] & 0xfffc)+2);
    HddInfo->HddIrq             = PciConfigHeader.Device.InterruptLine;
  } else {
    HddInfo->CommandBaseAddress = 0x170;
    HddInfo->ControlBaseAddress = 0x376;
    HddInfo->HddIrq             = 15;    
  }  
  (*ChannelCount)++;
  
ProcExit:
  return Status;                     
}


EFI_STATUS
EFIAPI
GetPlatformHandle (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL  *This,
  IN EFI_GET_PLATFORM_HANDLE_MODE       Mode,
  IN UINT16                             Type,
  OUT EFI_HANDLE                        **HandleBufferIn,
  OUT UINTN                             *HandleCountIn,
  OUT VOID                              **AdditionalData OPTIONAL
)
{
  EFI_STATUS                    Status;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  EFI_HANDLE                    TmpHandle;
  UINTN                         Index;
  UINTN                         BufIdx;
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  HDD_INFO                      *HddInfo;  
  UINTN                         ChannelIndex;
  UINTN                         ChannelCount;
  UINT8                         ClassCode[3];
  BOOLEAN                       VgaExchg;	
  

//DEBUG((EFI_D_INFO, "%a(%d) ", __FUNCTION__, Mode));
  
  HandleBuffer = NULL;
  BufIdx       = 0;
  ChannelIndex = 0;
  VgaExchg     = FALSE;	
  
  Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  &LegacyBios
                  );
  if (EFI_ERROR (Status)) {
    goto ProcExit;
  }  
  
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    &PciIo
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }
    Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET, 3, &ClassCode[0]);
    if (EFI_ERROR (Status)) {
      continue;
    }
    
    switch (Mode) {
      case EfiGetPlatformVgaHandle:
        if(ClassCode[2] == PCI_CLASS_DISPLAY && ClassCode[1] == PCI_CLASS_DISPLAY_VGA){
          if(BufIdx < VGA_HANDLE_BUF_MAX_COUNT){
            gVgaHandles[BufIdx++] = HandleBuffer[Index];
            *HandleBufferIn = gVgaHandles;
          }

          if(gSetupData->VideoDualVga && gSetupData->VideoPrimaryAdapter == 0){
            if(BufIdx > 1 && !VgaExchg){ //more than one VGA adapters
              TmpHandle = gVgaHandles[1];
              gVgaHandles[1] = gVgaHandles[0];
              gVgaHandles[0] = TmpHandle;
              VgaExchg = TRUE;							
            }
          }
        }
        break;
          
      case EfiGetPlatformIdeHandle:
        if(ClassCode[2] == PCI_CLASS_MASS_STORAGE && ClassCode[1] == PCI_CLASS_MASS_STORAGE_IDE){
          if(BufIdx < IDE_HANDLE_BUF_MAX_COUNT){
            gIdeHandles[BufIdx++] = HandleBuffer[Index];
            *HandleBufferIn = gIdeHandles;
            if (AdditionalData != NULL) {
              HddInfo = (HDD_INFO*)*AdditionalData;
              Status  = FillHddInfo(PciIo, &HddInfo[ChannelIndex], &ChannelCount);
              ChannelIndex += ChannelCount;
            }
          }
        }
        break;        

      case EfiGetPlatformIsaBusHandle:
        if(ClassCode[2] == PCI_CLASS_BRIDGE && ClassCode[1] == PCI_CLASS_BRIDGE_ISA){
          if(BufIdx < ISA_HANDLE_BUF_MAX_COUNT){
            gIsaHandles[BufIdx++] = HandleBuffer[Index];
            *HandleBufferIn = gIsaHandles;
          }
        }
        break;
      
      default:
        Status = EFI_UNSUPPORTED;
        goto ProcExit;
    }
  }

  *HandleCountIn = BufIdx;
  if(BufIdx==0){
    Status = EFI_NOT_FOUND;
  }

ProcExit:
  if(HandleBuffer != NULL){
    gBS->FreePool(HandleBuffer);
  }
  DEBUG((EFI_D_INFO, "%r count:%d\n", Status, BufIdx));  
  return Status;
}

#define SMM_PnP_BIOS_CALL   0x47

EFI_STATUS
EFIAPI
SmmInit (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *This,
  IN  VOID                               *EfiToLegacy16BootTable
)
{
  EFI_STATUS                      Status;
  SMM_TABLE                       *SoftIntData;
  SMM_ENTRY                       *SmmEntry;
  UINTN                           TableSize;
  EFI_SWSMI_VALUE_POLICY_PROTOCOL *SwSmiPolicy;
  UINT8                           Index;
  
  
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  Status = gBS->LocateProtocol(&gSwSmiValuePolicyGuid, NULL, &SwSmiPolicy);
  if (EFI_ERROR(Status)){
    SwSmiPolicy->NumSmmEntries = 0;
  }  
  
  TableSize = 0x100;
  Status    = gBS->AllocatePool(EfiBootServicesData, TableSize, &SoftIntData);
  ASSERT_EFI_ERROR (Status);
  ASSERT((UINT64)(UINTN)SoftIntData < SIZE_4GB);  
  ZeroMem(SoftIntData, TableSize);
  ((EFI_TO_COMPATIBILITY16_BOOT_TABLE*)EfiToLegacy16BootTable)->SmmTable = (UINT32)(UINTN)SoftIntData;

  SoftIntData->NumSmmEntries = SwSmiPolicy->NumSmmEntries;
  SmmEntry = &SoftIntData->SmmEntry;

  for(Index = 0; Index < SwSmiPolicy->NumSmmEntries; Index++) {
    SmmEntry->SmmAttributes.Type = SwSmiPolicy->SwSmiEntry[Index].Type;
    SmmEntry->SmmAttributes.PortGranularity = PORT_SIZE_16;
    SmmEntry->SmmAttributes.DataGranularity = DATA_SIZE_8;
    SmmEntry->SmmPort = SB_SWSMI_PORT_REG;
    SmmEntry->SmmData = SwSmiPolicy->SwSmiEntry[Index].Value;
    SmmEntry++;
  }

  SmmEntry->SmmAttributes.Type            = 0;
  SmmEntry->SmmAttributes.PortGranularity = PORT_SIZE_16;
  SmmEntry->SmmAttributes.DataGranularity = DATA_SIZE_8;
  SmmEntry->SmmFunction.Function          = DMI_PNP_50_57;
  SmmEntry->SmmPort                       = SB_SWSMI_PORT_REG;
  SmmEntry->SmmData                       = SMM_PnP_BIOS_CALL;
  SmmEntry ++;
  SoftIntData->NumSmmEntries ++;

  return EFI_SUCCESS;
}





BOOLEAN
CheckVbeSupport (
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios
  )
{
  EFI_IA32_REGISTER_SET                   Regs;
  EFI_STATUS                              Status;
  EFI_PHYSICAL_ADDRESS                    AddressBelow1MB;
  static BOOLEAN                          HasExecute    = FALSE;
  static BOOLEAN                          mVbeSupport   = FALSE;
  VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK  *VbeInformationBlock;
  
  if (HasExecute) {
    return mVbeSupport;
  }
  
  //
  // Test to see if the Video Adapter is compliant with VBE 3.0
  //
  AddressBelow1MB = 0x00100000 - 1;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiBootServicesData,
                  EFI_SIZE_TO_PAGES (sizeof (VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK)),
                  &AddressBelow1MB
                  );
  ASSERT_EFI_ERROR (Status);

  ZeroMem (&Regs, sizeof (EFI_IA32_REGISTER_SET));
  Regs.X.AX = VESA_BIOS_EXTENSIONS_RETURN_CONTROLLER_INFORMATION;
  VbeInformationBlock = (VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK *)(UINTN)AddressBelow1MB;
  VbeInformationBlock->VESASignature  = VESA_BIOS_EXTENSIONS_VBE2_SIGNATURE;
  Regs.X.ES = EFI_SEGMENT ((UINTN) VbeInformationBlock);
  Regs.X.DI = EFI_OFFSET ((UINTN) VbeInformationBlock);

  LegacyBios->Int86 (LegacyBios, 0x10, &Regs);
  
  HasExecute = TRUE;
  if ((Regs.X.AX != VESA_BIOS_EXTENSIONS_STATUS_SUCCESS) ||
      (VbeInformationBlock->VESASignature != VESA_BIOS_EXTENSIONS_VESA_SIGNATURE) ||
      (VbeInformationBlock->VESAVersion < VESA_BIOS_EXTENSIONS_VERSION_2_0)) {
    mVbeSupport = FALSE;
  } else {
    mVbeSupport = TRUE;
  }
  
  gBS->FreePages (AddressBelow1MB, EFI_SIZE_TO_PAGES (sizeof (VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK)));
  return mVbeSupport;
}

VOID
EFIAPI
PrepareToScanRomHookVideo (
  IN EFI_HANDLE  DeviceHandle,
  IN EFI_HANDLE  VgaHandle
  )
{
  EFI_IA32_REGISTER_SET         Regs;
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  EFI_STATUS                    Status;

//DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__)); 
  
  mVideoModeSave = FALSE;
  if ((VgaHandle != NULL) && (DeviceHandle != VgaHandle)) {
    //
    // Get mode settings since PrepareToScanRom may change mode
    //
    Status = gBS->LocateProtocol (
                    &gEfiLegacyBiosProtocolGuid,
                    NULL,
                    &LegacyBios
                    );
    ASSERT_EFI_ERROR (Status);

    if (!CheckVbeSupport (LegacyBios)) {
      DEBUG((EFI_D_INFO, "CheckVbeSupport:No\n"));    
      return;
    }
    
    ZeroMem (&Regs, sizeof (EFI_IA32_REGISTER_SET));
    Regs.X.AX = 0x4F03;
    LegacyBios->Int86 (LegacyBios, 0x10, &Regs);
//  DEBUG((EFI_D_INFO, "(L%d)AX:%04X, BX:%04X\n", __LINE__, Regs.X.AX, Regs.X.BX));     
    mVideoMode = Regs.X.BX;   
    mVideoModeSave = TRUE;
  }
}

VOID
EFIAPI
AfterRomInitHookVideo (
  IN EFI_HANDLE  DeviceHandle,
  IN EFI_HANDLE  VgaHandle
  )
{
  EFI_IA32_REGISTER_SET         Regs;
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  EFI_STATUS                    Status;
  
//DEBUG((EFI_D_INFO, "%a() mVideoModeSave:%d, mVideoMode:%X\n", __FUNCTION__, mVideoModeSave, mVideoMode));  
  
  if (!mVideoModeSave) {
    return;
  }
  Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  &LegacyBios
                  );
  ASSERT_EFI_ERROR (Status);
  
  ZeroMem (&Regs, sizeof(EFI_IA32_REGISTER_SET));

  //
  // Restore video mode after Oprom starts
  //
  Regs.X.AX = 0x4F02;
  Regs.X.BX = mVideoMode;
  LegacyBios->Int86(LegacyBios, 0x10, &Regs);
//DEBUG((EFI_D_INFO, "AX:%04X\n", Regs.X.AX)); 

  mVideoModeSave = FALSE;
}


EFI_STATUS
FindNextPnpExpansionHeader (
  IN BOOLEAN                           FirstHeader,
  IN OUT LEGACY_PNP_EXPANSION_HEADER   **PnpPtr

)
{
  UINTN                       TempData;
  LEGACY_PNP_EXPANSION_HEADER *LocalPnpPtr;
  LocalPnpPtr = *PnpPtr;
  if (FirstHeader == FIRST_INSTANCE) {
    mBasePnpPtr     = LocalPnpPtr;
    mBbsRomSegment  = (UINT16) ((UINTN) mBasePnpPtr >> 4);
    //
    // Offset 0x1a gives offset to PnP expansion header for the first
    // instance, there after the structure gives the offset to the next
    // structure
    //
    LocalPnpPtr = (LEGACY_PNP_EXPANSION_HEADER *) ((UINT8 *) LocalPnpPtr + 0x1a);
    TempData    = (*((UINT16 *) LocalPnpPtr));
  } else {
    TempData = (UINT16) LocalPnpPtr->NextHeader;
  }

  LocalPnpPtr = (LEGACY_PNP_EXPANSION_HEADER *) (((UINT8 *) mBasePnpPtr + TempData));

  //
  // Search for PnP table in Shadowed ROM
  //
  *PnpPtr = LocalPnpPtr;
  if (*(UINT32 *) LocalPnpPtr == SIGNATURE_32 ('$', 'P', 'n', 'P')) {
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

VOID
SaveOprom (
  UINT8   *Rom,
  UINT8   *RtDataAddr,
  UINT16  RtDataSize
)
{
  UINT16 Index;
  UINTN  RomSize = ((EFI_LEGACY_EXPANSION_ROM_HEADER*)Rom)->Size512 << 9;

  if (RomSize == 0) {
    DEBUG ((EFI_D_ERROR, "SaveOprom return abnormally!\n"));
    return;
  }
  //
  // If it has been saved, then just return
  //
  for (Index = 0; Index < mSavedCount; Index ++) {
    if (mSaveRomEntry[Index].Address == Rom){
      return;
    }
  }
  mSaveRomEntry[mSavedCount].Data = AllocateZeroPool(RomSize);
  CopyMem(mSaveRomEntry[mSavedCount].Data, Rom, RomSize);
  mSaveRomEntry[mSavedCount].Address = Rom;
  //
  // Save runtime data associated with this ROM
  //
  if (RtDataSize) {
    ASSERT(RtDataAddr); // if size is not zero, address must not be zero
    mSaveRomEntry[mSavedCount].RtDataAddr = RtDataAddr;
    mSaveRomEntry[mSavedCount].RtDataSize = RtDataSize;
    mSaveRomEntry[mSavedCount].RtData = AllocateZeroPool(RtDataSize);
    CopyMem(mSaveRomEntry[mSavedCount].RtData, RtDataAddr, RtDataSize);
  }

  mSavedCount++;  // Points to zero address/data now.

  return;
}

EFI_STATUS
CallBcvHookInt13 (
  IN  EFI_LEGACY_BIOS_PROTOCOL          *LegacyBios,
  IN  EFI_COMPATIBILITY16_TABLE         *Compatibility16Table,
  IN  EFI_LEGACY_EXPANSION_ROM_HEADER   *RomStart,
  UINT8                                 *RtData,
  UINT32                                RtDataSize
)
{
  VOID                            *RomEnd;
  EFI_LEGACY_EXPANSION_ROM_HEADER *PciPtr;
  LEGACY_PNP_EXPANSION_HEADER     *PnpPtr;
  BOOLEAN                         Instance;
  EFI_STATUS                      Status;
  EFI_IA32_REGISTER_SET           Regs;

  //
  // if it doesn't match signature, then we should not hook int13 either.
  //
  if (*(UINT16 *)(UINTN)RomStart != 0xAA55)
    return EFI_SUCCESS;

  ASSERT ((LegacyBios != NULL) && (Compatibility16Table != NULL));

  PnpPtr    = (LEGACY_PNP_EXPANSION_HEADER *) RomStart;
  PciPtr    = (EFI_LEGACY_EXPANSION_ROM_HEADER *) RomStart;

  RomEnd    = (VOID *) (PciPtr->Size512 * 512 + (UINTN) PciPtr);
  Instance  = FIRST_INSTANCE;
  //
  // Save the context before calling BCV in order to restore it during legacy boot
  //
  DEBUG((EFI_D_ERROR,"into CallBcvHookInt13() \n"));
  ///
  SaveOprom ((UINT8 *)RomStart, RtData, (UINT16)RtDataSize);
  if (PciPtr >= (EFI_LEGACY_EXPANSION_ROM_HEADER *) ((UINTN) 0xc8000)) {
    while (TRUE) {
      Status    = FindNextPnpExpansionHeader (Instance, &PnpPtr);
      Instance  = NOT_FIRST_INSTANCE;
      if (EFI_ERROR (Status)) {
        break;
      }

      //
      // Have seen case where PXE base code have PnP expansion ROM
      // header but no Bcv or Bev vectors.
      //
      if (PnpPtr->Bcv != 0) {
        ZeroMem (&Regs, sizeof(Regs));
        Regs.X.ES = Compatibility16Table->PnPInstallationCheckSegment;
        Regs.X.DI = Compatibility16Table->PnPInstallationCheckOffset;
	 DEBUG((EFI_D_ERROR,"bf LegacyBios->FarCall86() \n"));	
        Status = LegacyBios->FarCall86 (
                               LegacyBios,
                               mBbsRomSegment,
                               PnpPtr->Bcv,
                               &Regs,
                               NULL,
                               0
                               );
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }

      if ((PnpPtr == (LEGACY_PNP_EXPANSION_HEADER *) PciPtr) || (PnpPtr > (LEGACY_PNP_EXPANSION_HEADER *) RomEnd)) {
        break;
      }
    }
  }

  return EFI_SUCCESS;
}

UINT8 *
FindCompatibility16Table (
  VOID
)
/*++
Routine Description:
  Find the EFI_COMPATIBILITY16_TABLE in F segment.

Returns:
  The found EFI_COMPATIBILITY16_TABLE or NULL if not found.
--*/
{
  UINT8  *Table;

  for (Table = (UINT8 *) (UINTN) 0xF0000;
       Table < (UINT8 *) (UINTN) 0x100000;
       Table = (UINT8 *) Table + 0x10
      ) {
    if (*(UINT32 *) Table == EFI_COMPATIBILITY16_TABLE_SIGNATURE) {
      return Table;
    }
  }

  return NULL;
}


VOID
UpdateEbdaMap(UINT32 AddrChange)
{
  UINTN         Index;

  DEBUG ((EFI_D_ERROR, "LegacyBiosPlatform.c : UpdateEbdaMap!\n"));

  //
  // for every SAVED_PCI_ROM update ebdaAddr
  //
  for (Index = 0; Index < mSavedCount; Index++) {
    if (mSaveRomEntry[Index].RtDataAddr != NULL) {
      DEBUG ((EFI_D_INFO, "Original RtDataAddr = 0x%x\n", mSaveRomEntry[Index].RtDataAddr));
      mSaveRomEntry[Index].RtDataAddr -= AddrChange;
      DEBUG ((EFI_D_INFO, "New RtDataAddr = 0x%x\n", mSaveRomEntry[Index].RtDataAddr));
    }
  }
  ///
  DEBUG ((EFI_D_ERROR, "Exit from UpdateEbdaMap()\n"));
  ///
}



UINT32
EFIAPI
AhciReadReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT32              Offset
)
{
  UINT32                  Data;

  ASSERT (PciIo != NULL);
  Data = 0;
  PciIo->Mem.Read (
    PciIo,
    EfiPciIoWidthUint32,
    EFI_AHCI_BAR_INDEX,
    (UINT64) Offset,
    1,
    &Data
  );
  return Data;
}

VOID
EFIAPI
AhciWriteReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Data
)
{
  ASSERT (PciIo != NULL);

  PciIo->Mem.Write (
    PciIo,
    EfiPciIoWidthUint32,
    EFI_AHCI_BAR_INDEX,
    (UINT64) Offset,
    1,
    &Data
  );
}



EFI_STATUS
EFIAPI
PlatformHooks (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL              *This,
  IN EFI_GET_PLATFORM_HOOK_MODE                     Mode,
  IN UINT16                                         Type,
  IN EFI_HANDLE                                     DeviceHandle,          OPTIONAL
  IN OUT UINTN                                      *Shadowaddress,        OPTIONAL
  IN EFI_COMPATIBILITY16_TABLE                      *Compatibility16Table, OPTIONAL
  OUT VOID                                          **AdditionalData       OPTIONAL
)
{
  EFI_STATUS                      Status;
  EFI_PCI_IO_PROTOCOL             *PciIo = NULL; 
  BOOLEAN                         IsLegacyAhciRom = FALSE;
  EFI_LEGACY_BIOS_PROTOCOL        *LegacyBios; 
  EFI_IA32_REGISTER_SET           Regs;  
  UINT16                          EbdaSegNew;
  UINT32                          RtDataSize;
  UINTN                           RuntimeAddress;
  UINT8                           *RtData = NULL;
  EFI_ATA_PASS_THRU_PROTOCOL      *AtaPassThru;
  UINTN                           Port;
  UINT32                          Offset;
  DATA_64                         Data64;  
  UINT32                          PciId;
  UINT8                           ClassCode[3];
  AHCI_FIS_CMDLIST_SAVE           *AhciSave = NULL;
  EFI_HANDLE                      Handle;
  LEGACY_OPROM_INFO               *OpromInfo;
  UINTN                           Segment, Bus, Dev, Func;

  
  DEBUG((EFI_D_ERROR, "%a(%d,0x%X)\n", __FUNCTION__, Mode, Shadowaddress==NULL?0:*Shadowaddress));

  Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  &LegacyBios
                  );
  ASSERT_EFI_ERROR(Status);

  SetMem(ClassCode, sizeof(ClassCode), 0xFF);
  Segment = 0xFF;
  Bus     = 0xFF;
  Dev     = 0xFF;
  Func    = 0xFF;
  if (DeviceHandle != 0) {
    Status = gBS->HandleProtocol (
                    DeviceHandle,
                    &gEfiPciIoProtocolGuid,
                    &PciIo
                    );
    if (!EFI_ERROR (Status)) {
      PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET, 3, ClassCode);
      PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, PCI_VENDOR_ID_OFFSET, 1, &PciId);
      DEBUG((EFI_D_INFO, "%08X\n", PciId));
      PciIo->GetLocation(PciIo, &Segment, &Bus, &Dev, &Func);
    }      
  }
  if(ClassCode[2] == PCI_BCC_STORAGE && ClassCode[1] == PCI_SCC_AHCI){
    Status = gBS->HandleProtocol (
                    DeviceHandle,
                    &gEfiAtaPassThruProtocolGuid,
                    &AtaPassThru
                    );
    if(!EFI_ERROR (Status)){
      IsLegacyAhciRom = TRUE;
      AhciSave = &gAhciAddrSaveArray[gAhciAddrSaveCurCount];
    }  
  }  


  Status = EFI_SUCCESS;
  
  switch (Mode) {
    case EfiPlatformHookPrepareToScanRom:
      
      if (ClassCode[2] == PCI_CLASS_NETWORK && !gSetupData->LegacyPxeBoot) {
         DEBUG((EFI_D_ERROR, "Prohibit Pxe OptionRom\n"));
         Status = EFI_UNSUPPORTED;
         break;
      }
    
      if (IsLegacyAhciRom) {
        AhciSave->PciIo = PciIo;
        AhciSave->PortCount = (AhciReadReg(PciIo, 0) & 0x1F) + 1;
        for (Port = 0; Port < AhciSave->PortCount; Port++) {
          Offset = 0x100 + (UINT32)Port * 0x80;         // base
          Data64.Uint32.Lower32 = AhciReadReg(PciIo, Offset+0x08);
          Data64.Uint32.Upper32 = AhciReadReg(PciIo, Offset+0x0C);
          AhciSave->NativeFis[Port] = Data64.Uint64;
          Data64.Uint32.Lower32 = AhciReadReg(PciIo, Offset+0x00);
          Data64.Uint32.Upper32 = AhciReadReg(PciIo, Offset+0x04);
          AhciSave->NativeCmdList[Port] = Data64.Uint64;
        }
      }
      if (PcdGet8 (PcdDisplayOptionRomMessage)) {
        PrepareToScanRomHookVideo(DeviceHandle, gVgaHandles[0]);
        Regs.H.AH = 0x00;
        Regs.H.AL = 0x03;
        LegacyBios->Int86(LegacyBios, 0x10, &Regs);  // Set the 80x25 Text VGA Mode
      }

      if (!mInt13VectorSaved) {
        mInt13Vector = *((UINT32*)(UINTN)(0x4c));
        mInt13VectorSaved = TRUE;
      }
      mEbdaSegOrg = *(UINT16*)(UINTN)0x40e;
      DEBUG ((EFI_D_INFO, "mEbdaSegOrg = 0x%x\n", mEbdaSegOrg));
      mEbdaSizeOrg = *(UINT8*)((UINTN)mEbdaSegOrg<<4);
      DEBUG ((EFI_D_INFO, "mEbdaSizeOrg = 0x%x\n", mEbdaSizeOrg));
      ASSERT(mEbdaSizeOrg);

      Handle = NULL;
      OpromInfo = AllocatePool(sizeof(LEGACY_OPROM_INFO));
      ASSERT(OpromInfo != NULL);
      OpromInfo->Bus  = (UINT8)Bus;
      OpromInfo->Dev  = (UINT8)Dev;      
      OpromInfo->Func = (UINT8)Func;
      CopyMem(OpromInfo->ClassCode, ClassCode, sizeof(ClassCode));
      OpromInfo->OpromDataSize = (((UINT8*)(*Shadowaddress))[2])<<9;
      OpromInfo->OpromData = AllocatePool(OpromInfo->OpromDataSize);
      ASSERT(OpromInfo->OpromData!=NULL);
      CopyMem(OpromInfo->OpromData, (VOID*)(*Shadowaddress), OpromInfo->OpromDataSize);
      gBS->InstallProtocolInterface(
             &Handle,
             &gLegacyOptionRomInfoProtocolGuid,
             EFI_NATIVE_INTERFACE,
             OpromInfo
             );
      break;

    case EfiPlatformHookShadowServiceRoms:
      break;

    case EfiPlatformHookAfterRomInit:
      DEBUG((EFI_D_ERROR,"EfiPlatformHookAfterRomInit : \n"));	
      ///	  
      AfterRomInitHookVideo (DeviceHandle, gVgaHandles[0]);
      if (IsLegacyAhciRom) {
        for (Port = 0; Port < AhciSave->PortCount; Port++) {
          Offset = 0x100 + (UINT32)Port * 0x80;         // base
          Data64.Uint32.Lower32 = AhciReadReg(PciIo, Offset+0x08);
          Data64.Uint32.Upper32 = AhciReadReg(PciIo, Offset+0x0C);
          AhciSave->LegacyFis[Port] = Data64.Uint64;
          Data64.Uint32.Lower32 = AhciReadReg(PciIo, Offset+0x00);
          Data64.Uint32.Upper32 = AhciReadReg(PciIo, Offset+0x04);
          AhciSave->LegacyCmdList[Port] = Data64.Uint64;

          Data64.Uint64 = AhciSave->NativeFis[Port];
          AhciWriteReg(PciIo, Offset+0x08, Data64.Uint32.Lower32);
          AhciWriteReg(PciIo, Offset+0x0C, Data64.Uint32.Upper32);
          Data64.Uint64 = AhciSave->NativeCmdList[Port];
          AhciWriteReg(PciIo, Offset+0x00, Data64.Uint32.Lower32);
          AhciWriteReg(PciIo, Offset+0x04, Data64.Uint32.Upper32);
        }

        if(gAhciAddrSaveCurCount+1 < AHCI_ADDR_SAVE_MAX_HOST_COUNT){
          gAhciAddrSaveCurCount++;
        }  
      }
      EbdaSegNew = *(UINT16*)(UINTN)0x40e;                    // Update EBDA map
      DEBUG ((EFI_D_INFO, "EbdaSegNew = 0x%x\n", EbdaSegNew));
      DEBUG ((EFI_D_INFO, "EbdaSegNew size = 0x%x\n", *(UINT8*)((UINTN)EbdaSegNew<<4)));
      ASSERT(EbdaSegNew <= mEbdaSegOrg);
      RtDataSize = (UINT32)(mEbdaSegOrg - EbdaSegNew) << 4;   // #of bytes taken by this OpROM
      DEBUG ((EFI_D_INFO, "RtDataSize = 0x%x\n", RtDataSize));

      if (RtDataSize) {   // EBDA was allocated and used by new OPROM, update the EBDA address
        UpdateEbdaMap(RtDataSize);
        RtData = (UINT8*)(((UINTN)EbdaSegNew<<4) + ((UINTN)mEbdaSizeOrg<<10));
        DEBUG ((EFI_D_INFO, "RtData = 0x%x\n", RtData));
      }
      RuntimeAddress = *Shadowaddress;

      //
      // Locate Compatibility16 Table if it is NULL when passed
      //
      if (Compatibility16Table == NULL) {
        Compatibility16Table = (EFI_COMPATIBILITY16_TABLE *) FindCompatibility16Table ();
        ASSERT (Compatibility16Table != NULL);
      }
      
      ///
      DEBUG ((EFI_D_ERROR, "bf CallBcvHookInt13() \n"));
      ///
      CallBcvHookInt13 (
        LegacyBios,
        Compatibility16Table,
        (EFI_LEGACY_EXPANSION_ROM_HEADER *) RuntimeAddress,
        RtData,
        RtDataSize
      );
      break;    
    
    default:
      Status = EFI_UNSUPPORTED;
      break;    
  }

  return Status;
}


///MTN-20161010-S
VOID UpdatePIRTable (VOID)
{
	UINT16 BrigdgeIndex = 0;
	UINT16 SlotIndex = 0;
	UINT16 Len = 0;
	UINT16 Count = 0;
	UINT8 *pPirTable = NULL;
	UINT8 *pBackupPtr = NULL;

	pPirTable = AllocatePool(sizeof(PIR_DATA) * PCI_BRIDGES_NUM);   
	pBackupPtr = pPirTable;
	ZeroMem(pPirTable, sizeof(PIR_DATA) * PCI_BRIDGES_NUM);

	//Len = sizeof(EFI_LEGACY_PIRQ_TABLE_HEADER)+ sizeof(EFI_LEGACY_IRQ_ROUTING_ENTRY) * FIXED_ENTRY_NUM;
	///CopyMem((UINT8*)pPirTable, (UINT8*)&gPirData, Len);
	///(UINT8*)pPirTable =  (UINT8*)(pPirTable + Len);
	
	Len = sizeof(EFI_LEGACY_IRQ_ROUTING_ENTRY);

	for (SlotIndex = 1; SlotIndex <= SLOT_NUM; SlotIndex++)
	{	    
	    for (BrigdgeIndex = 0; BrigdgeIndex < PCI_BRIDGES_NUM; BrigdgeIndex++)
	    {
		if (gPlatformPciBridgesInfo[BrigdgeIndex].SlotNum == SlotIndex)
		{
		     if (gPlatformPciBridgesInfo[BrigdgeIndex].Enabled == 1)
		     {
		         gPirData.Entry[FIXED_ENTRY_NUM+SlotIndex-1].Bus = gPlatformPciBridgesInfo[BrigdgeIndex].SecBus;
			 CopyMem((UINT8*)pPirTable, (UINT8*)&gPirData.Entry[FIXED_ENTRY_NUM+SlotIndex-1], Len);
			 (UINT8*)pPirTable =  (UINT8*)(pPirTable + Len);
			 Count++;
		      }
		}
	    }
	}

	gTotalEntryNum = FIXED_ENTRY_NUM + Count;
	pPirTable = pBackupPtr;

	gPirqTableSize= (sizeof(EFI_LEGACY_PIRQ_TABLE_HEADER)+ sizeof(EFI_LEGACY_IRQ_ROUTING_ENTRY)*gTotalEntryNum);

	CopyMem((UINT8*)&gPirData.Entry[FIXED_ENTRY_NUM], (UINT8*)pPirTable, sizeof(EFI_LEGACY_IRQ_ROUTING_ENTRY)* Count);

	gPirData.Hdr.CompatibleVid = MmioRead16(LPC_PCI_REG(PCI_VID_REG));
	gPirData.Hdr.CompatibleDid = MmioRead16(LPC_PCI_REG(PCI_DID_REG));		
	gPirData.Hdr.Checksum = CalculateCheckSum8((UINT8*)&gPirData, gPirqTableSize);

#if 0
	DEBUG((EFI_D_ERROR, "\n************************************\n"));
	pBackupPtr = (UINT8*)&gPirData;
	for(Count=0; Count<512; Count++) {
		if(Count%16==0 && Count !=0) {
			DEBUG((EFI_D_ERROR, "\n"));
		}
		DEBUG((EFI_D_ERROR,"0x%02x ", *pBackupPtr++));
	}  
	DEBUG((EFI_D_ERROR, "\n************************************\n"));	
#endif

	DEBUG((EFI_D_ERROR,"[MTN-DBG]: gTotalEntryNum=0x%x \n", gTotalEntryNum));
	DEBUG((EFI_D_ERROR,"[MTN-DBG]: gPirqTableSize=0x%x \n", gPirqTableSize));
	DEBUG((EFI_D_ERROR,"[MTN-DBG]: Checksum=0x%x \n", gPirData.Hdr.Checksum));

	if (pPirTable != NULL)
		FreePool(pPirTable);

}
///MTN-20161010-E


EFI_STATUS
EFIAPI
GetRoutingTable (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL            * This,
  OUT VOID                                        **RoutingTable,
  OUT UINTN                                       *RoutingTableEntries,
  OUT VOID                                        **LocalPirqTable, OPTIONAL
  OUT UINTN                                       *PirqTableSize, OPTIONAL
  OUT VOID                                        **LocalIrqPriorityTable, OPTIONAL
  OUT UINTN                                       *IrqPriorityTableEntries OPTIONAL
)
{

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
/*MTN-2016110201 -S
   if (gTotalEntryNum == 0)
   {
	CollectPCIBusInfo();  
	UpdatePIRTable();
   }
*///MTN-2016110201 -E  
  *RoutingTable = &gPirData.Entry[0];
  *RoutingTableEntries = IRQ_ROUTING_COUNT;	//MTN-2016110201
  
  if(LocalPirqTable!=NULL){
    *LocalPirqTable = &gPirData.Hdr;
  }

  
  if(PirqTableSize!=NULL){
    *PirqTableSize = sizeof(gPirData); //MTN-2016110201
  }

  
  if(LocalIrqPriorityTable!=NULL){
    *LocalIrqPriorityTable = NULL;
  }
  if(IrqPriorityTableEntries!=NULL){
    *IrqPriorityTableEntries = 0;
  }

  return EFI_SUCCESS;
}


typedef struct {
  UINT8  Bus;
  UINT8  Dev;
  UINT8  Func;
  UINT8  IntLine;
} PCI_DEV_IRQ;

#define INT_LINE_A    0
#define INT_LINE_B    1
#define INT_LINE_C    2
#define INT_LINE_D    3
#define INT_LINE_E    4
#define INT_LINE_F    5
#define INT_LINE_G    6
#define INT_LINE_H    7

STATIC UINT8  gIntLineIrq[8] = {
  0xA, 0x5, 0x7, 0xB, 0xFF, 0xFF, 0xFF, 0xB
};

//Yankui-remov-s
/*
STATIC PCI_DEV_IRQ gPciDevIrqList[] = {
  {0, 1,    0, INT_LINE_H},
  {0, 1,    1, INT_LINE_H},
  {0, 2,    0, INT_LINE_H},
  {0, 3,    0, INT_LINE_H},
  {0, 4,    0, INT_LINE_H},
  {0, 5,    0, INT_LINE_H},
  {0, 6,    0, INT_LINE_H},
  {0, 7,    0, INT_LINE_H},
  {0, 9,    0, INT_LINE_H},
  {0, 0xA,  0, INT_LINE_A},
  {0, 0xA,  1, INT_LINE_B}, 
  {0, 0xA,  2, INT_LINE_C}, 
  {0, 0xA,  3, INT_LINE_D},
  {0, 0xC,  0, INT_LINE_A},
  
  {0, 0xF,  0, INT_LINE_A},
  {0, 0xF,  1, INT_LINE_A},
  {0, 0xF,  2, INT_LINE_A},

  {0, 0x10, 0, INT_LINE_A},
  {0, 0x10, 1, INT_LINE_B}, 
  {0, 0x10, 2, INT_LINE_C}, 
  {0, 0x10, 7, INT_LINE_D},
  {0, 0x12, 0, INT_LINE_A},  
  {0, 0x14, 0, INT_LINE_B},
  {0, 0x15, 0, INT_LINE_A},  
};
*/

EFI_STATUS
EFIAPI
TranslatePirq (
  IN EFI_LEGACY_BIOS_PLATFORM_PROTOCOL            *This,
  IN UINTN                                        PciBus,
  IN UINTN                                        PciDevice,
  IN UINTN                                        PciFunction,
  IN OUT UINT8                                    *Pirq,
  OUT UINT8                                       *PciIrq
)
{
#if 0
  EFI_STATUS               Status;
  UINTN                    Index;
  UINTN                    Count;
  EFI_HANDLE               *HandleBuffer;
  UINTN                    HandleCount;  
  EFI_PCI_IO_PROTOCOL      *ptPciIo;
  UINT8                    ClassCode[3];
  UINT8                    SubBus[2];
  UINTN                    Segment, Bus, Dev, Func;  
  UINT8                    IntLine;
  
  HandleBuffer = NULL;  
  PciDevice >>= 3;
  DEBUG((EFI_D_INFO, "%a(%02X,%02X,%02X) %02X %02X\n", __FUNCTION__, PciBus, PciDevice, PciFunction, *Pirq, *PciIrq));
  
  Count = sizeof(gPciDevIrqList)/sizeof(gPciDevIrqList[0]);
  for(Index=0;Index<Count;Index++){
    if((UINT8)PciBus      == gPciDevIrqList[Index].Bus &&
       (UINT8)PciDevice   == gPciDevIrqList[Index].Dev &&
       (UINT8)PciFunction == gPciDevIrqList[Index].Func) {
      IntLine = gPciDevIrqList[Index].IntLine;
      *Pirq   = IntLine;
      *PciIrq = gIntLineIrq[IntLine];
      Status = EFI_SUCCESS;
      goto ProcExit;
    }
  }
  
  if(PciBus == 0){
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }
DEBUG((EFI_D_ERROR,"Warning: Cannot find IRQ for device behind bridge\n"));
// Find IRQ on Bridge.  
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  for(Index=0; Index<HandleCount; Index++){
    Status = gBS->HandleProtocol(
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    &ptPciIo
                    );
    ASSERT(!EFI_ERROR(Status));
    Status = ptPciIo->Pci.Read(ptPciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET, 3, &ClassCode[0]);
    if(EFI_ERROR(Status)){
      goto ProcExit;
    }
    if(ClassCode[2] != PCI_CLASS_BRIDGE || 
       ClassCode[1] != PCI_CLASS_BRIDGE_P2P ||
       ClassCode[0] != PCI_IF_BRIDGE_P2P){
      continue;   
    }
    Status = ptPciIo->Pci.Read(ptPciIo, EfiPciIoWidthUint8, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET, 2, &SubBus[0]);
    if(EFI_ERROR(Status)){
      goto ProcExit;
    }
    if(PciBus < SubBus[0] || PciBus > SubBus[1]){
      continue;
    }
    Status = ptPciIo->GetLocation(ptPciIo, &Segment, &Bus, &Dev, &Func);
    if(EFI_ERROR(Status)){
      goto ProcExit;
    }    
    for(Index=0;Index<Count;Index++){
      if((UINT8)Bus  == gPciDevIrqList[Index].Bus &&
         (UINT8)Dev  == gPciDevIrqList[Index].Dev &&
         (UINT8)Func == gPciDevIrqList[Index].Func) {
        IntLine = gPciDevIrqList[Index].IntLine;
        *Pirq   = IntLine;
        *PciIrq = gIntLineIrq[IntLine];
        Status = EFI_SUCCESS;
        goto ProcExit;
      }
    }
    break;
  }  

  Status = EFI_UNSUPPORTED;
  
ProcExit:  
  if(EFI_ERROR(Status)){
    IntLine = INT_LINE_A;
    *Pirq   = IntLine;
    *PciIrq = gIntLineIrq[IntLine];
    Status  = EFI_SUCCESS;
    DEBUG((EFI_D_ERROR, "This Dev IntLine cannot be found at table! Use default\n"));   
  }   
  if(HandleBuffer!=NULL){
    gBS->FreePool(HandleBuffer);
  }
  return Status;
#endif
    UINTN               Index;
    UINT8               Link=0;

	DEBUG((EFI_D_INFO, "%a(%02X,%02X,%02X) %02X\n", __FUNCTION__, PciBus, (PciDevice>>3), PciFunction, *Pirq));

    if(*Pirq >= 4) return EFI_INVALID_PARAMETER;
    for(Index = 0; Index < IRQ_ROUTING_COUNT; Index++) { //MTN-2016110201
      if((PciBus == gPirData.Entry[Index].Bus) &&
  		(PciDevice == gPirData.Entry[Index].Device)) {
  	    Link = gPirData.Entry[Index].PirqEntry[*Pirq].Pirq;
  	    break;
      }
    }

	if(Index == IRQ_ROUTING_COUNT) { //MTN-2016110201
    	//If not found in $PIR table..
    	//TODO: We need a sophisticated algorithm to get the proper IRQs.
    	//      Ask BYO to update the kernel.
    	//      Currently, we just return an error hoping no bridges on buses with non-zero bus#.
        DEBUG((EFI_D_ERROR,"ERROR: Cannot find IRQ routing info for device (%02X,%02X,%02X)!!!\n",
              PciBus, (PciDevice>>3), PciFunction));
    	return EFI_NOT_FOUND;
    }
	ASSERT(Link != 0);
	ASSERT(Link <= 8);
	*Pirq = Link - 1;
	*PciIrq = gIntLineIrq[*Pirq];
	DEBUG((EFI_D_ERROR,"XLAT Result=%x, %x,%x\n",Link,*Pirq,*PciIrq));
	
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PrepareToBoot (
  IN  EFI_LEGACY_BIOS_PLATFORM_PROTOCOL           *This,
  IN  BBS_BBS_DEVICE_PATH                         *BbsDevicePath,
  IN  VOID                                        *BbsTable,
  IN  UINT32                                      LoadOptionsSize,
  IN  VOID                                        *LoadOptions,
  IN  EFI_TO_COMPATIBILITY16_BOOT_TABLE           *EfiToLegacy16BootTable
)
{
  DEVICE_PRODUCER_DATA_HEADER   *SioPtr;
  SAVED_PCI_ROM                 *Rom;  
  UINTN                         Index;
  
  
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  
  SioPtr                  = &EfiToLegacy16BootTable->SioData;
  SioPtr->Flags.A20Kybd   = 1;
  SioPtr->Flags.A20Port90 = 1;

// Restore ENV(ROM space and EBDA) for RAID boot
  for (Index = 0; Index < mSavedCount; Index++) {
    Rom = &mSaveRomEntry[Index];
    CopyMem (
      Rom->Address,
      Rom->Data,
      ((EFI_LEGACY_EXPANSION_ROM_HEADER*)Rom->Data)->Size512 * 0x200
    );
    if (Rom->RtDataAddr) {
      CopyMem (
        Rom->RtDataAddr,
        Rom->RtData,
        Rom->RtDataSize
      );
    }
  }

// Reset number of ATA drives to let CSM16 PrepareToBoot function connect the drives
  *(UINT8*)(UINTN)0x475 = 0;
  
  if(mInt13VectorSaved){
    *((UINT32*)(UINTN)0x4C) = mInt13Vector;
  }
  return EFI_SUCCESS;  
}


VOID
LegacyBootEvent (
  IN EFI_EVENT           Event,
  IN VOID                *Context
)
{
  UINTN                  Port;
  UINT32                 Offset;
  DATA_64                Data64;
  AHCI_FIS_CMDLIST_SAVE  *AhciSave;
  UINTN                  Index;

  if (gAhciAddrSaveCurCount) {
    for(Index = 0; Index < gAhciAddrSaveCurCount; Index++){
      AhciSave = &gAhciAddrSaveArray[Index];
      for (Port = 0; Port < AhciSave->PortCount; Port++) {
        Offset = 0x100 + (UINT32)Port * 0x80;      // base
        Data64.Uint64 = AhciSave->LegacyFis[Port];
        AhciWriteReg(AhciSave->PciIo, Offset+0x08, Data64.Uint32.Lower32);
        AhciWriteReg(AhciSave->PciIo, Offset+0x0C, Data64.Uint32.Upper32);
        Data64.Uint64 = AhciSave->LegacyCmdList[Port];
        AhciWriteReg(AhciSave->PciIo, Offset+0x00, Data64.Uint32.Lower32);
        AhciWriteReg(AhciSave->PciIo, Offset+0x04, Data64.Uint32.Upper32);
      }
    }  
  }
}



STATIC EFI_LEGACY_BIOS_PLATFORM_PROTOCOL  gLegacyBiosPlatform = {
  GetPlatformInfo,
  GetPlatformHandle,
  SmmInit,
  PlatformHooks,
  GetRoutingTable,
  TranslatePirq,
  PrepareToBoot
};

VOID
SetSmmPort (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_STATUS                Status;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  UINT16                    EBdaSeg;
  UINT16                    *SmmPortPtr;

  Status = gBS->LocateProtocol(
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  (VOID**)&LegacyBios
                  );
  if (EFI_ERROR (Status)) {
    Status = gBS->LocateProtocol(
                    &gEfiVirtLegacyBiosProtocolGuid,
                    NULL,
                    (VOID**)&LegacyBios
                    );
    if (EFI_ERROR (Status)) {
      return;
    }
  }
  gBS->CloseEvent(Event);

  EBdaSeg = *(UINT16 *)(UINTN)0x40E;
  SmmPortPtr = (UINT16 *)(UINTN)(((UINT32)EBdaSeg << 4) + 0x1D0); // Offset 0x1D0
  *SmmPortPtr = SB_SWSMI_PORT_REG;
}


EFI_STATUS
LegacyBiosPlatformInstall (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  VOID        *Registration;
//MTN-2016110201 -S

  gPirData.Hdr.CompatibleVid = MmioRead16(LPC_PCI_REG(PCI_VID_REG));
  gPirData.Hdr.CompatibleDid = MmioRead16(LPC_PCI_REG(PCI_DID_REG));  
  gPirData.Hdr.Checksum = CalculateCheckSum8((UINT8*)&gPirData, sizeof(gPirData));
//MTN-2016110201 -E
  gAhciAddrSaveArray = AllocatePool(sizeof(AHCI_FIS_CMDLIST_SAVE) * AHCI_ADDR_SAVE_MAX_HOST_COUNT);
  ASSERT(gAhciAddrSaveArray!=NULL);
  
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiLegacyBiosPlatformProtocolGuid,
                  &gLegacyBiosPlatform,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);

  Status = EfiCreateEventLegacyBootEx (
             TPL_CALLBACK,
             LegacyBootEvent,
             (VOID *)&ImageHandle,
             &Event
             );  
  EfiCreateProtocolNotifyEvent (
    &gEfiLegacyBiosProtocolGuid,
    TPL_CALLBACK,
    SetSmmPort,
    NULL,
    &Registration
    );
  EfiCreateProtocolNotifyEvent (
    &gEfiVirtLegacyBiosProtocolGuid,
    TPL_CALLBACK,
    SetSmmPort,
    NULL,
    &Registration
    ); 

  return Status;
}

