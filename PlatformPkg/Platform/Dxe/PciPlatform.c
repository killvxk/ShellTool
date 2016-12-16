
#include "PlatformDxe.h"
#include <IndustryStandard/Pci.h>
#include <Protocol/PciPlatform.h>
#include <Protocol/PciIo.h>



STATIC
EFI_STATUS
PhaseNotify (
  IN  EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN  EFI_HANDLE                                     HostBridge,
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE  Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE                ChipsetPhase
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PlatformPrepController (
  IN  EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN  EFI_HANDLE                                     HostBridge,
  IN  EFI_HANDLE                                     RootBridge,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS    PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE   Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE                ChipsetPhase
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
GetPlatformPolicy (
  IN  CONST EFI_PCI_PLATFORM_PROTOCOL   *This,
  OUT       EFI_PCI_PLATFORM_POLICY     *PciPolicy
  )
{
  return EFI_UNSUPPORTED;
}




STATIC PCI_OPTION_ROM_TABLE gOpRomTable[] = {
  {IGD_PCI_BUS, IGD_PCI_DEV, IGD_PCI_FUNC, 0xFFFF, 0xFFFF, IGD_VBIOS_FILE_GUID},
};

STATIC
EFI_STATUS
GetPciRom (
  IN  CONST EFI_PCI_PLATFORM_PROTOCOL   *This,
  IN        EFI_HANDLE                  PciHandle,
  OUT       VOID                        **RomImage,
  OUT       UINTN                       *RomSize
  )
{
  EFI_STATUS                    Status;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  UINTN                         Segment;
  UINTN                         Bus;
  UINTN                         Device;
  UINTN                         Function;
  UINT16                        VendorId;
  UINT16                        DeviceId;
  UINTN                         Index;
  UINTN                         Count;
  BOOLEAN                       HasFound;
  UINT8                         CacheLineSize;
  PCI_OPTION_ROM_TABLE          *RomTable;
  

  HasFound = FALSE;
  Status = gBS->HandleProtocol (
                  PciHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID**)&PciIo
                  );
  if (EFI_ERROR (Status)) {
    goto ProcExit;
  }
  
  Status = PciIo->GetLocation (
                    PciIo,
                    &Segment,
                    &Bus,
                    &Device,
                    &Function
                    );
  Status = PciIo->Pci.Read (
                    PciIo,
                    EfiPciIoWidthUint16,
                    0,
                    1,
                    &VendorId
                    );
  Status = PciIo->Pci.Read (
                    PciIo,
                    EfiPciIoWidthUint16,
                    2,
                    1,
                    &DeviceId
                    );
  
  CacheLineSize = PCI_CACHE_LINE_SIZE;
  Status = PciIo->Pci.Write(PciIo, EfiPciIoWidthUint8, PCI_CACHELINE_SIZE_OFFSET, 1, &CacheLineSize);
                       
  Status = gPtAsiaSb->PostPciInit(gPtAsiaSb, PciIo);
  ASSERT_EFI_ERROR(Status);   
  Status = gPtAsiaNb->PostPciInit(gPtAsiaNb, PciIo);
  ASSERT_EFI_ERROR(Status);                       
  
  Count = sizeof(gOpRomTable)/sizeof(gOpRomTable[0]);
  RomTable = gOpRomTable;
  for(Index=0; Index<Count; Index++) {
    if(RomTable[Index].BusNo == 0xFFFF){             // use id
      if(VendorId != RomTable[Index].VendorId || 
         DeviceId != RomTable[Index].DeviceId){
        continue;
      }
    } else {
      if((UINT8)RomTable[Index].BusNo != (UINT8)Bus ||
                RomTable[Index].DevNo != (UINT8)Device ||
                RomTable[Index].FuncNo != (UINT8)Function){
        continue;
      }          
    }
    
    Status = GetSectionFromFv (
               &gOpRomTable[Index].FileName,
               EFI_SECTION_RAW,
               0,
               RomImage,
               RomSize
               );
    if(EFI_ERROR(Status)){
      DEBUG((EFI_D_ERROR, "GetSectionFromFv(%g):%r\n", &gOpRomTable[Index].FileName, Status));                          
      continue;
    }else{
      HasFound = TRUE;
      break;
    }
  }
  
ProcExit:
  if(HasFound){
    Status = EFI_SUCCESS;
  }else{
    Status = EFI_NOT_FOUND;
  }
  return Status;
}


STATIC EFI_PCI_PLATFORM_PROTOCOL gPciPlatform = {
  PhaseNotify,
  PlatformPrepController,
  GetPlatformPolicy,
  GetPciRom
};
  
EFI_STATUS
PciPlatformInstall (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiPciPlatformProtocolGuid);  
  
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiPciPlatformProtocolGuid,
                  &gPciPlatform,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);
  
  return Status;
}  