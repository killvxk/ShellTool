
#ifndef _BDS_PLATFORM_H
#define _BDS_PLATFORM_H

#include <PiDxe.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/LoadFile.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/PciIo.h>
#include <Protocol/CpuIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/OEMBadging.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/GenericMemoryTest.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/FirmwareVolume2.h>

#include <Guid/CapsuleVendor.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/GlobalVariable.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootManagerLib.h>
#include "PlatformBootManagerLib.h"
#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <IndustryStandard/Pci30.h>
#include <SetupVariable.h>




///
/// ConnectType
///
#define CONSOLE_OUT 0x00000001
#define STD_ERROR   0x00000002
#define CONSOLE_IN  0x00000004
#define CONSOLE_ALL (CONSOLE_OUT | CONSOLE_IN | STD_ERROR)

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     ConnectType;
} BDS_CONSOLE_CONNECT_ENTRY;

extern BDS_CONSOLE_CONNECT_ENTRY gPlatformConsole [];
extern EFI_DEVICE_PATH_PROTOCOL  *gPlatformConnectSequence [];
#define gPciRootBridge \
  { \
    ACPI_DEVICE_PATH, ACPI_DP, (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), (UINT8) \
      ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8), EISA_PNP_ID (0x0A03), 0 \
  }

#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, END_DEVICE_PATH_LENGTH, 0 \
  }

//
// Platform Root Bridge
//
typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ROOT_BRIDGE_DEVICE_PATH;

//
// Below is the platform console device path
//
typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           IsaBridge;
  ACPI_HID_DEVICE_PATH      Keyboard;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_KEYBOARD_DEVICE_PATH;

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           IsaBridge;
  ACPI_HID_DEVICE_PATH      IsaSerial;
  UART_DEVICE_PATH          Uart;
  VENDOR_DEVICE_PATH        TerminalType;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ISA_SERIAL_DEVICE_PATH;

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           Pci0Device;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH;

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           Pci0Device;
  PCI_DEVICE_PATH           Pci1Device;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2;

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           PciDevice;
  ACPI_ADR_DEVICE_PATH      AcpiDeviceNode;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_GOP_DEVICE_PATH;

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           Pci0Device;
  PCI_DEVICE_PATH           Pci1Device;
  ACPI_ADR_DEVICE_PATH      AcpiDeviceNode;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_PEG_GOP_DEVICE_PATH;

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           PciBridge;
  PCI_DEVICE_PATH           PciDevice;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_PCI_CONTROLLER_DEVICE_PATH;

//
// Below is the boot option device path
//
typedef struct {
  BBS_BBS_DEVICE_PATH       LegacyHD;
  EFI_DEVICE_PATH_PROTOCOL  End;
} LEGACY_HD_DEVICE_PATH;

#define CLASS_HID           3
#define SUBCLASS_BOOT       1
#define PROTOCOL_KEYBOARD   1

typedef struct {
  USB_CLASS_DEVICE_PATH           UsbClass;
  EFI_DEVICE_PATH_PROTOCOL        End;
} USB_CLASS_FORMAT_DEVICE_PATH;


extern BDS_CONSOLE_CONNECT_ENTRY     gPlatformConsole[];
extern EFI_DEVICE_PATH_PROTOCOL      *gPlatformConnectSequence[];
extern PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformOnboardSataAhciDevice;
extern PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformOnboardSdioDevice;
extern PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2 gPlatformOnboardNicDevice;
extern PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformOnboardVgaDevice;
extern PLATFORM_GOP_DEVICE_PATH                 gPlatformOnboardVgaDeviceGop;
extern PLATFORM_PEG_GOP_DEVICE_PATH             gPlatformPegVgaDevice;
extern PLATFORM_PEG_GOP_DEVICE_PATH             gPlatformPEGDeviceGop;
extern PLATFORM_KEYBOARD_DEVICE_PATH            gKeyboardDevicePath;
extern USB_CLASS_FORMAT_DEVICE_PATH             gUsbClassKeyboardDevicePath;
//
// Platform BDS Functions
//

typedef
VOID
(*PROCESS_VARIABLE) (
  VOID  **Variable,
  UINTN *VariableSize
  );
VOID
UpdateEfiGlobalVariable (
  CHAR16           *VariableName,
  EFI_GUID         *AgentGuid,
  PROCESS_VARIABLE ProcessVariable
  )
/*++

Routine Description:

  Generic function to update the console variable.
  Please refer to FastBootSupport.c for how to use it.

Arguments:

  VariableName    - The name of the variable to be updated
  AgentGuid       - The Agent GUID
  ProcessVariable - The function pointer to update the variable
                    NULL means to restore to the original value

--*/
;

/**
  Perform the memory test base on the memory test intensive level,
  and update the memory resource.

  @param  Level         The memory test intensive level.

  @retval EFI_STATUS    Success test all the system memory and update
                        the memory resource

**/
EFI_STATUS
MemoryTest (
  IN EXTENDMEM_COVERAGE_LEVEL Level
  );

EFI_STATUS
PlatformBdsShowProgress (
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleForeground,
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleBackground,
  CHAR16                        *Title,
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL ProgressColor,
  UINTN                         Progress,
  UINTN                         PreviousValue
  )
/*++

Routine Description:
  
  Show progress bar with title above it. It only works in UGA mode.

Arguments:
  
  TitleForeground  -  Foreground color for Title.
  TitleBackground  -  Background color for Title.
  Title            -  Title above progress bar.
  ProgressColor    -  Progress bar color.
  Progress         -  Progress (0-100)
  PreviousValue    -  Previous value of progress.
  
Returns: 
  
  EFI_STATUS  -  Success update the progress bar.

--*/
;

VOID
ConnectSequence (
  VOID
  )
/*++

Routine Description:

  Connect with predeined platform connect sequence, 
  the OEM/IBV can customize with their own connect sequence.
  
Arguments:

  None.
 
Returns:

  None.
  
--*/
;

/**
  This function locks platform flash that is not allowed to be updated during normal boot path.
  The flash layout is platform specific.

  **/
VOID
LockNonUpdatableFlash (
  VOID
  );

BOOLEAN
EFIAPI
CompareBootOption (
  CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Left,
  CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Right
  );

VOID 
PrintBootPrompt (
  VOID
  );

EFI_STATUS
EFIAPI
ProcessCapsules (
  EFI_BOOT_MODE BootMode
  );

UINT8
GetDisplayBootMode (
  VOID
  );
  
#endif
