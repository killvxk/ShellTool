
#include "InstallOptionRom.h"
#include <Protocol/AtaPassThru.h>
#include <Protocol/ScsiPassThruExt.h>

ADDITIONAL_ROM_TABLE  gRomTable[] = {
  {REALTEK_VENDOR_ID, 0x8168,             LAN_OPROM_FILE_GUID,  NULL},
  {0,                 0,                  AHCI_OPROM_FILE_GUID, OnBoardAhciOpRomCheck},
};


BOOLEAN 
OnBoardAhciOpRomCheck(
  EFI_HANDLE           Handle,
  EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  UINT8        ClassCode[3];
  EFI_STATUS   Status;
  UINTN        Index;
  EFI_DEVICE_PATH_PROTOCOL    *Dp;
  UINTN                       DpSize;
  EFI_ATA_PASS_THRU_PROTOCOL      *AtaPassThru;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *ExtScsiPT;  
  UINT16                      SataPort;
  UINT16                      SataPortMp;
  UINT64                      Lun;  
  UINT8                       Target[TARGET_MAX_BYTES];
  UINT8                       *TargetId;  
  BOOLEAN                     Rc = FALSE;
    

  Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, PCI_CC_PI_REG, 3, ClassCode);
  if(ClassCode[2] != PCI_BCC_STORAGE || ClassCode[1] != PCI_SCC_AHCI){
    goto ProcExit;
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  &Dp
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  

  DpSize = GetDevicePathSize(Dp);
  for(Index = 0; Index < 4; Index++){
    if(CompareMem(Dp, gPlatformConnectSequence[Index], DpSize)){
      continue;
    }  
    
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiAtaPassThruProtocolGuid,
                    &AtaPassThru
                    );
    if(EFI_ERROR(Status)){
      goto TryScsi;
    }
    SataPort = 0xFFFF;
    while (TRUE) {
      Status = AtaPassThru->GetNextPort(AtaPassThru, &SataPort);
      if (EFI_ERROR (Status)) {
        break;
      } 
      SataPortMp = 0xFFFF;
      while (TRUE) {
        Status = AtaPassThru->GetNextDevice(AtaPassThru, SataPort, &SataPortMp);
        if (EFI_ERROR (Status)) {
          break;
        }
        Rc = TRUE;
        goto ProcExit;
      }
    }
  }

TryScsi:
  Status = gBS->HandleProtocol(
                  Handle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  &ExtScsiPT
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }
  Lun = 0;
  TargetId = &Target[0];
  SetMem(Target, sizeof(Target), 0xFF);
  Status = ExtScsiPT->GetNextTargetLun(ExtScsiPT, &TargetId, &Lun);
  if(!EFI_ERROR(Status)){
    Rc = TRUE;
  }
  
ProcExit:
  return Rc;
}




VOID
InstallAdditionalOpRom (
  VOID
  )
{
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  EFI_STATUS                    Status;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         Index;
  UINTN                         TableIndex;
  UINTN                         TableCount;	
  UINTN                         Flags;
  VOID                          *LocalRomImage;
  UINTN                         LocalRomSize;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  UINT16                        VendorId;
  UINT16                        DeviceId;
  BOOLEAN                       RunCheck;
  UINT8                         ClassCode[3];


  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  HandleBuffer = NULL;

  Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  &LegacyBios
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "LegacyBios Not found! Skip\n"));
    goto ProcExit;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  ASSERT(!EFI_ERROR(Status));

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    &PciIo
                    );
    ASSERT(!EFI_ERROR(Status));

    PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_VENDOR_ID_OFFSET, 1, &VendorId);
    PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_DEVICE_ID_OFFSET, 1, &DeviceId);

    TableCount = sizeof(gRomTable)/sizeof(gRomTable[0]);
    for (TableIndex = 0; TableIndex < TableCount; TableIndex++) {
      if(gRomTable[TableIndex].RunCheck == NULL){
        RunCheck = (gRomTable[TableIndex].VendorId == VendorId && gRomTable[TableIndex].DeviceId == DeviceId);
      } else {
        RunCheck = gRomTable[TableIndex].RunCheck(HandleBuffer[Index], PciIo);
      }
      if(RunCheck){
        Status = GetSectionFromAnyFv (
                   &gRomTable[TableIndex].RomImageGuid,
                   EFI_SECTION_RAW,
                   0,
                   &LocalRomImage,
                   &LocalRomSize
                   );
        if (!EFI_ERROR (Status) && LocalRomImage != NULL && LocalRomSize != 0) {
          Status = LegacyBios->InstallPciRom (
                                 LegacyBios,
                                 HandleBuffer[Index],
                                 &LocalRomImage,
                                 &Flags,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL
                                 );
          break;
        }
      }
    }
    if(TableIndex < TableCount){
      continue;
    }			
  

// Check if we have oprom
//  DEBUG((EFI_D_INFO, "OpRom(%X,%lX) %04X%04X\n", PciIo->RomImage, PciIo->RomSize, DeviceId, VendorId));
    if (PciIo->RomImage == NULL || PciIo->RomSize == 0) {
      continue;
    }
    PciIo->Pci.Read (
                 PciIo,
                 EfiPciIoWidthUint8,
                 PCI_CLASSCODE_OFFSET,
                 sizeof(ClassCode),
                 &ClassCode[0]
                 );
//  DEBUG ((EFI_D_INFO,"ClassCode(%X,%X,%X)\n", ClassCode[2], ClassCode[1], ClassCode[0]));
    if (((ClassCode[2] == PCI_CLASS_DISPLAY) && (ClassCode[1] == PCI_CLASS_DISPLAY_VGA)) ||
        ((ClassCode[2] == PCI_CLASS_OLD) && (ClassCode[1] == PCI_CLASS_OLD_VGA))) {
      continue;
    }
    //
    // connect external OpRom
    //
    Status = gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
  }


ProcExit:
  if(HandleBuffer != NULL){
    gBS->FreePool(HandleBuffer);
  }
  return;
}


