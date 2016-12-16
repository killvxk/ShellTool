

#ifndef _AHCI_INTERRUPT_H_
#define _AHCI_INTERRUPT_H_

#include <Uefi.h>
#include <ShellBase.h>
#include <Library/ShellCEntryLib.h>



#include <Protocol/AtaPassThru.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/StorageSecurityCommand.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/ReportStatusCodeLib.h>


#include <IndustryStandard/Atapi.h>


#include <Register/LocalApic.h>
#include <Register/IoApic.h>
#include <Protocol/Legacy8259.h>
#include <Library/LocalApicLib.h>
#include <Library/IoApicLib.h>
#include <Protocol/Cpu.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Protocol/PciIo.h>
#include <Library/BaseLib.h>
#include <Library/PciLib.h>
#include <IndustryStandard/Pci.h>
#include <Protocol/AcpiTable.h>
#include <IndustryStandard/Acpi.h>
#include <Protocol/AcpiSupport.h>

//#include <Protocol/DevicePathToText.h>
//
// ATA bus data structure for ATA controller
//
typedef struct {
  EFI_ATA_PASS_THRU_PROTOCOL  *AtaPassThru;
  EFI_HANDLE                  Controller;
  EFI_DEVICE_PATH_PROTOCOL    *ParentDevicePath;
  EFI_HANDLE                  DriverBindingHandle;
} ATA_BUS_DRIVER_DATA;

//
// ATA device data structure for each child device
//
typedef struct {
  UINT32                                Signature;

  EFI_HANDLE                            Handle;
  EFI_BLOCK_IO_PROTOCOL                 BlockIo;
  EFI_BLOCK_IO2_PROTOCOL                BlockIo2;
  EFI_BLOCK_IO_MEDIA                    BlockMedia;
  EFI_DISK_INFO_PROTOCOL                DiskInfo;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL StorageSecurity;

  ATA_BUS_DRIVER_DATA                   *AtaBusDriverData;
  UINT16                                Port;
  UINT16                                PortMultiplierPort;

  //
  // Buffer for the execution of ATA pass through protocol
  //
  EFI_ATA_PASS_THRU_COMMAND_PACKET      Packet;
  EFI_ATA_COMMAND_BLOCK                 Acb;
  EFI_ATA_STATUS_BLOCK                  *Asb;

  BOOLEAN                               UdmaValid;
  BOOLEAN                               Lba48Bit;
  BOOLEAN                               Locked;

  //
  // Cached data for ATA identify data
  //
  ATA_IDENTIFY_DATA                     *IdentifyData;

  EFI_UNICODE_STRING_TABLE              *ControllerNameTable;
  CHAR16                                ModelName[41];

  LIST_ENTRY                            AtaTaskList;
} ATA_DEVICE;

#endif
