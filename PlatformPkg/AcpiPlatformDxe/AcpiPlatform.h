
#ifndef __ACPI_PLATFORM_H__
#define __ACPI_PLATFORM_H__

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <SetupVariable.h>
#include <PlatformDefinition.h>


typedef struct {
  UINT8   ApicId;
  UINT32  Flags;
} CPU_APIC_ID_INFO;


extern SETUP_DATA  *gSetupData;

VOID
BuildAcpiSpcrTable (
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
	);

VOID
AcpiPlatformChecksum (
  IN EFI_ACPI_COMMON_HEADER  *Hdr
  );

EFI_STATUS
UpdateVpssPackage (
  IN UINT8  *VpssName
);

EFI_STATUS
UpdateNpssPackage (
  IN UINT8  *NpssName
);

EFI_STATUS
PlatformUpdateTables (
  IN OUT EFI_ACPI_COMMON_HEADER   *Table
  );
  
EFI_STATUS
GetCpuLocalApicInfo (
    CPU_APIC_ID_INFO      **CpuApicIdTables,
    UINTN                 *CpuCount
  );  
  
#endif

