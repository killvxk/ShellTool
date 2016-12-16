
#include "PlatformDxe.h"
#include <IndustryStandard/Acpi.h>
#include <Protocol/PciHotPlugInit.h>


#define EFI_PCI_HOTPLUG_DRIVER_PRIVATE_SIGNATURE   SIGNATURE_32('_', 'H', 'P', '_')

typedef struct {
  UINTN                           Signature;
  EFI_HANDLE                      Handle;
  EFI_PCI_HOT_PLUG_INIT_PROTOCOL  HotPlugInit;
} PCI_HOT_PLUG_INSTANCE;






EFI_STATUS
EFIAPI
InitializeRootHpc (
  IN  EFI_PCI_HOT_PLUG_INIT_PROTOCOL     *This,
  IN  EFI_DEVICE_PATH_PROTOCOL           *PhpcDevicePath,
  IN  UINT64                             PhpcPciAddress,
  IN  EFI_EVENT                          Event, OPTIONAL
  OUT EFI_HPC_STATE                      *PhpcState
  )
{
  if (Event) {
    gBS->SignalEvent (Event);
  }

  *PhpcState = EFI_HPC_STATE_INITIALIZED;
  return EFI_SUCCESS;
}


STATIC PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformRootPortDp = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x2
  },
  gEndEntire
};

STATIC EFI_HPC_LOCATION gPciSlotList[] = {
  {
    (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformRootPortDp,
    (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformRootPortDp
  },
};


EFI_STATUS
EFIAPI
GetRootHpcList (
  IN  EFI_PCI_HOT_PLUG_INIT_PROTOCOL  *This,
  OUT UINTN                           *HpcCount,
  OUT EFI_HPC_LOCATION                **HpcList
  )
{
  EFI_HPC_LOCATION  *Buffer;
  EFI_STATUS        Status = EFI_SUCCESS;

  Buffer = (EFI_HPC_LOCATION*)AllocatePool(sizeof(gPciSlotList));
  if(Buffer == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  CopyMem(Buffer, gPciSlotList, sizeof(gPciSlotList));

  *HpcCount = sizeof(gPciSlotList)/sizeof(gPciSlotList[0]);
  *HpcList  = Buffer;

ProcExit:
  return Status;
}


EFI_STATUS
EFIAPI
GetResourcePadding (
  IN  EFI_PCI_HOT_PLUG_INIT_PROTOCOL      *This,
  IN  EFI_DEVICE_PATH_PROTOCOL            *PhpcDevicePath,
  IN  UINT64                              PhpcPciAddress,
  OUT EFI_HPC_STATE                       *PhpcState,
  OUT VOID                                **Padding,
  OUT EFI_HPC_PADDING_ATTRIBUTES          *Attributes
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *PaddingRes;
  EFI_STATUS                        Status = EFI_SUCCESS;
  UINTN                             Size;

  
  Size = sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR)*4 + sizeof(EFI_ACPI_END_TAG_DESCRIPTOR);
  PaddingRes = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR*)AllocateZeroPool(Size);
  if(PaddingRes == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  *Padding    = (VOID*)PaddingRes;
  *Attributes = EfiPaddingPciBus;
  *PhpcState  = EFI_HPC_STATE_INITIALIZED | EFI_HPC_STATE_ENABLED;

  
  PaddingRes->Desc                 = 0x8A;
  PaddingRes->Len                  = sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
  PaddingRes->ResType              = ACPI_ADDRESS_SPACE_TYPE_BUS;
  PaddingRes->GenFlag              = 0x0;
  PaddingRes->SpecificFlag         = 0;
  PaddingRes->AddrSpaceGranularity = 0;
  PaddingRes->AddrRangeMin         = 0;
  PaddingRes->AddrRangeMax         = 1;
  PaddingRes->AddrLen              = 1;

  PaddingRes++;
  PaddingRes->Desc                 = 0x8A;
  PaddingRes->Len                  = sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
  PaddingRes->ResType              = ACPI_ADDRESS_SPACE_TYPE_MEM;
  PaddingRes->GenFlag              = 0x0;
  PaddingRes->AddrSpaceGranularity = 32;
  PaddingRes->SpecificFlag         = 0;
  PaddingRes->AddrRangeMin         = 0;
  PaddingRes->AddrRangeMax         = 0x1000000 - 1;  
  PaddingRes->AddrLen              = 0x1000000;

  PaddingRes++;
  PaddingRes->Desc                 = 0x8A;
  PaddingRes->Len                  = sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
  PaddingRes->ResType              = ACPI_ADDRESS_SPACE_TYPE_MEM;
  PaddingRes->GenFlag              = 0;
  PaddingRes->SpecificFlag         = 6;
  PaddingRes->AddrSpaceGranularity = 32;  
  PaddingRes->AddrRangeMin         = 0;
  PaddingRes->AddrRangeMax         = 0x1000000 - 1;  
  PaddingRes->AddrLen              = 0x1000000;

  PaddingRes++;
  PaddingRes->Desc                 = 0x8A;
  PaddingRes->Len                  = sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
  PaddingRes->ResType              = ACPI_ADDRESS_SPACE_TYPE_IO;
  PaddingRes->GenFlag              = 0;
  PaddingRes->SpecificFlag         = 0;
  PaddingRes->AddrSpaceGranularity = 0;  
  PaddingRes->AddrRangeMin         = 0;
  PaddingRes->AddrRangeMax         = 0x1000 - 1;  
  PaddingRes->AddrLen              = 0x1000;

  PaddingRes++;
  ((EFI_ACPI_END_TAG_DESCRIPTOR*)PaddingRes)->Desc     = ACPI_END_TAG_DESCRIPTOR;
  ((EFI_ACPI_END_TAG_DESCRIPTOR*)PaddingRes)->Checksum = 0x0;

ProcExit:
  return Status;
}






STATIC PCI_HOT_PLUG_INSTANCE  gPciHotPlug = { 
  EFI_PCI_HOTPLUG_DRIVER_PRIVATE_SIGNATURE, 
  NULL,
  {
    GetRootHpcList,
    InitializeRootHpc,
    GetResourcePadding
  }
};

EFI_STATUS
EFIAPI
PciHotPlugEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallProtocolInterface (
                  &gPciHotPlug.Handle,
                  &gEfiPciHotPlugInitProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gPciHotPlug.HotPlugInit
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}


