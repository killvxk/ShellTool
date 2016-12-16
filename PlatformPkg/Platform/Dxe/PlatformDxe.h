
#ifndef __PLATFORM_DXE_H__
#define __PLATFORM_DXE_H__

#include <uefi.h>
#include <PiDxe.h>
#include <IndustryStandard/Smbios.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <PlatformDefinition.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/PciIo.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/Smbios.h>
#include <Protocol/DevicePath.h>
#include <AsiaCpuProtocol.h>
#include <AsiaNbProtocol.h>
#include <AsiaSbProtocol.h>
#include <CHX001Cfg.h>
#include <SetupVariable.h>
#include <Library/PlatformCommLib.h>
#include <PlatS3Record.h>



extern EFI_ASIA_CPU_PROTOCOL *gPtAsiaCpu;
extern EFI_ASIA_SB_PROTOCOL  *gPtAsiaSb;
extern EFI_ASIA_NB_PROTOCOL  *gPtAsiaNb;
extern ASIA_NB_CONFIGURATION *gAsiaNbCfg;
extern ASIA_SB_CONFIGURATION *gAsiaSbCfg;
extern CONST SETUP_DATA      *gSetupData;

EFI_STATUS
LegacyInterruptInstall (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
  
EFI_STATUS
LegacyBiosPlatformInstall (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
  
EFI_STATUS
SmmAccess2Install (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

EFI_STATUS
EFIAPI
LegacyRegion2Install (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );  

EFI_STATUS
PciPlatformInstall (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

VOID
PlatformDebugAtEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

VOID ShowCpuInfo();
  

EFI_STATUS
CpuDxeConfig (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
  
EFI_STATUS
EFIAPI
IncompatiblePciDeviceSupportEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

EFI_STATUS
EFIAPI
SataControllerInstall (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

EFI_STATUS MiscConfigDxe(VOID);

EFI_STATUS 
ReadFileFromFv(
  IN  CONST EFI_GUID                *NameGuid,
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         SectionInstance,
  OUT VOID                          **FileData,
  OUT UINTN                         *FileDataSize
    );

VOID UpdatePs2State();

EFI_STATUS InstallAcpiTableDmar();

VOID *
AllocateAcpiNvsZeroMemoryBelow4G (
  IN UINTN  Size
  );

EFI_STATUS
EFIAPI
PciHotPlugEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );


#define gPciRootBridge \
  { \
    ACPI_DEVICE_PATH, ACPI_DP, (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), (UINT8) \
      ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8), EISA_PNP_ID (0x0A03), 0 \
  }

#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, END_DEVICE_PATH_LENGTH, 0 \
  }  
  
typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           Pci0Device;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH;


  
#endif
  
  
  
  
  
  
  
  