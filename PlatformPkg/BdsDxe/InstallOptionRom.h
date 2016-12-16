
#ifndef _INSTALL_OPTION_ROM_H_
#define _INSTALL_OPTION_ROM_H_

#include "BdsPlatform.h"
#include <Library/PerformanceLib.h>
#include <IndustryStandard/Atapi.h>
#include <PlatformDefinition.h>


//
// The structure for the light version of AHCI ROM
//
#define SATA_CONTROLLER_STRUC_SIGNATURE       0x41325324
#define SATA_CONTROLLER_STRUC_FIELDS_VALID    0xF
#define SATA_CONTROLLER_IMPLEMENTED_PORTS     0x0000003F

typedef enum {
  EfiAhciHdd,                  // HDD
  EfiAhciCD,                   // CDROM or DVD ROM
  EfiAhciOtherDevice           // Other device is unsupported yet
} EFI_AHCI_DEVICE_DETECT_TYPE;

#define  ATA_ATAPI_DEVICE_TYPE_MASK                 0x1F00
#define  ATA_ATAPI_DEVICE_ATAPI_REMOVEABLE_MASK     0x8080
#define  ATA_ATAPI_DEVICE_TYPE_CDR                  5
#define  ATA_ATAPI_DEVICE_TYPE(Sig) \
  (((UINT16) ((Sig) & ATA_ATAPI_DEVICE_TYPE_MASK)) >> 8)
  
#pragma pack(1)
typedef	struct	{
  UINT32	dSignature;			    // Signature, ASCII string
  UINT8	  bLength;				    // Structure length in bytes
  UINT8	  bChecksum;			    // Checksum
  UINT8	  bVersion;			      // Version Number
  UINT8	  bReserved;			    // Reserved for future use
  UINT32	dPtrSataController; // SATA_CONTROLLER_STRUC address
  UINT32	dReserved;			    // Reserved for future use
} S2A_INSTALLATION_STRUC;

typedef	struct	{
  UINT32	dPtrSataController;	  // Next SATA_CONTROLLER_STRUC address
  UINT8	  bAttribute;				    // Attribute
  UINT8	  bBus;				          // Bus# of SATA controller
  UINT8	  bDevice;				      // Device# of SATA controller
  UINT8	  bFunction;				    // Function# of SATA controller
  UINT32	dImplementedPort;	    // Implemented SATA Ports
  UINT32	dDevicePresentOnPort; // Device present on SATA Ports
  UINT32	dDeviceTypeOnPort;	  // Device Type on SATA Ports
  UINT32	dPtrSataDevice;		    // SATA_DEVICE_STRUC address
} SATA_CONTROLLER_STRUC;

typedef	struct	{
  UINT32	dPtrSataDevice;	              // Next SATA_DEVICE_STRUC address
  UINT8	  bAttribute;		                // Attribute
  UINT8	  bPort;				                // SATA Port# (0-based) where device is present
  UINT8	  bIdentifyDeviceData[0x200];   // 512-bytes of identify device data
} SATA_DEVICE_STRUC;
#pragma pack()

typedef
BOOLEAN
(EFIAPI *LEGACY_OPROM_RUN_CHECK)(
  IN EFI_HANDLE           Handle,
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  );

typedef struct {
  UINT16                  VendorId;
  UINT16                  DeviceId;
  EFI_GUID                RomImageGuid;
  LEGACY_OPROM_RUN_CHECK  RunCheck;
} ADDITIONAL_ROM_TABLE;

VOID
InstallAdditionalOpRom (
  VOID
  );

VOID
InstallAdditionalExternalOpRom (
  IN BOOLEAN InstallStorageRom,
  IN BOOLEAN InstallOtherRom
  );

BOOLEAN 
OnBoardAhciOpRomCheck(
  IN EFI_HANDLE           Handle,
  IN EFI_PCI_IO_PROTOCOL  *PciIo
);    
  
#endif

