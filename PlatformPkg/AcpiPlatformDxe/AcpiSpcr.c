

#include "AcpiPlatform.h"
#include <IndustryStandard/SerialPortConsoleRedirectionTable.h>
#include <Library/PlatformCommLib.h>


//
// terminal type
//
#define VT100TYPE                 		 0
#define VT100PLUSTYPE             		 1
#define VTUTF8TYPE                		 2
#define PCANSITYPE                		 3


EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE  gAcpiSpcrTable = {
  {
    EFI_ACPI_3_0_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,
    sizeof(gAcpiSpcrTable),
    0x01,                                                 // Revision
    0,                                                    // checksum
    {'B', 'Y', 'O', 'S', 'O','F'},                        // OemId
    SIGNATURE_64('B','Y','O','S','O','F','T','\0'),       // OemTableId
    1,                                                    // OemRevision
    SIGNATURE_32('B','Y','O','\0'),                       // CreatorId
    0x01000001                                            // CreatorRevision
  },
  0,                                                      // InterfaceType
  {0, 0, 0},                                              // Reserved1[3]
  {0x01,                                                  // AddressSpaceId
   0x08,                                                  // RegisterBitWidth
   0,                                                     // RegisterBitOffset
   0,                                                     // Reserved
   0                                                      // Address
  },
  0,                                                      // InterruptType
  0,                                                      // Irq
  0,                                                      // GlobalSystemInterrupt
  0,                                                      // BaudRate
  0,                                                      // Parity
  0,                                                      // StopBits
  0,                                                      // FlowControl
  0,                                                      // TerminalType
  0,                                                      // Language
  0xFFFF,                                                 // PciDeviceId
  0xFFFF,                                                 // PciVendorId
  0,                                                      // PciBusNumber
  0,                                                      // PciDeviceNumber
  0,                                                      // PciFunctionNumber
  0,                                                      // PciFlags
  0,                                                      // PciSegment
  0                                                       // Reserved2
};


VOID
BuildAcpiSpcrTable (
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
	)
{
  EFI_STATUS                Status;
  UINT16                    ComPort;
  UINT8                     Baudrate;
  UINT8                     TerminalType;
  UINT8                     IrqNumber;
  UINTN                     TableKey;


  IrqNumber    = 0;
  ComPort      = 0;
  Baudrate     = 0;
  TerminalType = 0;
  
  if (!gSetupData->UCREnable) {
    return;
  }
  
  switch (gSetupData->SerialPortSelect) {
    case 0:
      ComPort   = 0x3F8;
      IrqNumber = 4;
      break;
			
    case 1:
      ComPort   = 0x2F8;
      IrqNumber = 3;
      break;
	case 3:
	  ComPort   = 0x2E8;
      IrqNumber = 3;
      break;
    default:
      ComPort   = 0x3F8;
      IrqNumber = 4;
      break;
  }
  switch (gSetupData->SerialBaudrate) {
    case 0: // 9600
      Baudrate = 3;
      break;
    case 1: // 19200
      Baudrate = 4;
      break;
    case 3: // 57600
      Baudrate = 6;
      break;
    case 4: // 115200
      Baudrate = 7;
      break;
    default: // 115200
      Baudrate = 7;
      break;
  }
  switch(gSetupData->TerminalType) {
    case 0:
      TerminalType = PCANSITYPE;
      break;
    case 1:
      TerminalType = VT100TYPE;
      break;
    case 2:
      TerminalType = VT100PLUSTYPE;
      break;
    case 3:
      TerminalType = VTUTF8TYPE;
      break;
    default:
      TerminalType = VT100TYPE;
      break;
  }

  gAcpiSpcrTable.BaseAddress.Address  = (UINT64)ComPort;
  gAcpiSpcrTable.InterruptType				= 0x05;//bit0:dual-8259 bit1:I/O APIC bit2:I/O SAPIC
  gAcpiSpcrTable.Irq									= IrqNumber;
  gAcpiSpcrTable.BaudRate							= Baudrate;
  gAcpiSpcrTable.StopBits							= 1;
  gAcpiSpcrTable.FlowControl					= 0;
  gAcpiSpcrTable.TerminalType					= TerminalType;

  PlatformUpdateTables((EFI_ACPI_COMMON_HEADER*)&gAcpiSpcrTable);  
  AcpiTableUpdateChksum(&gAcpiSpcrTable);
  
  Status = AcpiTable->InstallAcpiTable(
                        AcpiTable,
                        &gAcpiSpcrTable,
                        sizeof(gAcpiSpcrTable),
                        &TableKey
                        );

}


