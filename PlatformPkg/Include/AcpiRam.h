
#ifndef __ACPI_RAM_H__
#define __ACPI_RAM_H__

#include <Base.h>

#pragma pack(1)

typedef struct {
  UINT64      Signature;
  UINT32      FlashSize;
  UINT32      PciBase;
  UINT32      PciLength;
  UINT16      OSVersion;
  UINT16      IoStates;
  UINT8       AcpiWakeState;
  UINT8       IsRtcWake;
} EFI_ACPI_RAM_DATA;

#pragma pack()


#define ACPI_RAM_DATA_SIGNATURE  SIGNATURE_64('$', 'A', 'C', 'P', 'I', 'R', 'A', 'M')

#define IO_STATE_PS2_MS_PRESENT        BIT14
#define IO_STATE_PS2_KB_PRESENT        BIT10


#endif

