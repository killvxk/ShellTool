
// This Header File is used to share info between asl and c.

#ifndef __PLATFORM_DEFINITION2_H__
#define __PLATFORM_DEFINITION2_H__

#define EFI_ACPI_ENABLE_SW_SMI         0xF0
#define EFI_ACPI_DISABLE_SW_SMI        0xF1
#define EFI_ACPI_S3_PEI_END_SW_SMI     0xF2

//#define PCAL6416A_PCIE_HOTPLUG_SUPPORT    0

#define SMBUS_BASE_ADDRESS             0x400
#define SMBUS_IO_LENGTH                0x20

#define EFI_ACPI_CSTATE_ON_SW_SMI      0x00   // Notify BIOS that OS will take owner of CState.
#define EFI_ACPI_PSTATE_ON_SW_SMI      0x00   // Notify BIOS that OS will take owner of PState.


#endif

