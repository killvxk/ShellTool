/** @file
  This is a simple shell application

  Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/PciLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/PciIo.h>
#include <Protocol/LegacyBios.h>


#define MAX_BBS_ENTRIES                  0x100
#define PCI_INT_LINE_OFFSET              0x3C ///< Interrupt Line Register




/**
  Print the PCI Interrupt Line and Interrupt Pin registers.
**/
VOID
PrintPciInterruptRegister (
  VOID
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index;
  EFI_HANDLE                  *Handles;
  UINTN                       HandleNum;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  UINT8                       Interrupt[2];
  UINTN                       Segment;
  UINTN                       Bus;
  UINTN                       Device;
  UINTN                       Function;

  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiPciIoProtocolGuid,
         NULL,
         &HandleNum,
         &Handles
         );

  Bus      = 0;
  Device   = 0;
  Function = 0;

  Print(L"\n");
  Print(L" bb/dd/ff interrupt line interrupt pin\n");
  Print(L"======================================\n");
  for (Index = 0; Index < HandleNum; Index++) {
    Status = gBS->HandleProtocol (Handles[Index], &gEfiPciIoProtocolGuid, (VOID **) &PciIo);
    if (!EFI_ERROR (Status)) {
      Status = PciIo->Pci.Read (
                            PciIo,
                            EfiPciIoWidthUint8,
                            PCI_INT_LINE_OFFSET,
                            2,
                            Interrupt
                            );
    }
    if (!EFI_ERROR (Status)) {
      Status = PciIo->GetLocation (
                        PciIo,
                        &Segment,
                        &Bus,
                        &Device,
                        &Function
                        );
    }
    if (!EFI_ERROR (Status)) {
      Print(L" %02x/%02x/%02x 0x%02x           0x%02x\n",
              Bus, Device, Function, Interrupt[0], Interrupt[1]);
    }
  }
  Print(L"\n");

  if (Handles != NULL) {
    FreePool (Handles);
  }
}



/**
  Print the BBS Table.

  @param BbsTable   The BBS table.
**/
VOID
PrintBbsTable (
  IN BBS_TABLE *BbsTable
  )
{
  UINT16 Index;
  UINT16 SubIndex;
  CHAR8  *String;

  Print(L"\n");
  Print(L" NO  Prio bb/dd/ff cl/sc Type Stat segm:offs mfgs:mfgo dess:deso\n");
  Print(L"=================================================================\n");
  for (Index = 0; Index < MAX_BBS_ENTRIES; Index++) {
    //
    // Filter
    //
    if (BbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) {
      continue;
    }

    Print(
      L" %02x: %04x %02x/%02x/%02x %02x/%02x %04x %04x",
      (UINTN) Index,
      (UINTN) BbsTable[Index].BootPriority,
      (UINTN) BbsTable[Index].Bus,
      (UINTN) BbsTable[Index].Device,
      (UINTN) BbsTable[Index].Function,
      (UINTN) BbsTable[Index].Class,
      (UINTN) BbsTable[Index].SubClass,
      (UINTN) BbsTable[Index].DeviceType,
      (UINTN) * (UINT16 *) &BbsTable[Index].StatusFlags
      );
    Print(
      L" %04x:%04x %04x:%04x %04x:%04x",
      (UINTN) BbsTable[Index].BootHandlerSegment,
      (UINTN) BbsTable[Index].BootHandlerOffset,
      (UINTN) BbsTable[Index].MfgStringSegment,
      (UINTN) BbsTable[Index].MfgStringOffset,
      (UINTN) BbsTable[Index].DescStringSegment,
      (UINTN) BbsTable[Index].DescStringOffset
      );

    //
    // Print DescString
    //
    String = (CHAR8 *)(UINTN)((BbsTable[Index].DescStringSegment << 4) + BbsTable[Index].DescStringOffset);
    if (String != NULL) {
      Print(L" (");
      for (SubIndex = 0; String[SubIndex] != 0; SubIndex++) {
        Print(L"%c", String[SubIndex]);
      }
      Print(L")");
    }
    Print(L"\n");
  }

  Print(L"\n");

  return ;
}





/**
  Print the BBS Table.

  @param HddInfo   The HddInfo table.


**/
VOID
PrintHddInfo (
  IN HDD_INFO *HddInfo
  )
{
  UINTN Index;

  Print ((L"\n"));
  for (Index = 0; Index < MAX_IDE_CONTROLLER; Index++) {
    Print(L"Index - %04x\n", Index);
    Print(L"  Status    - %04x\n", (UINTN)HddInfo[Index].Status);
    Print(L"  B/D/F     - %02x/%02x/%02x\n", (UINTN)HddInfo[Index].Bus, (UINTN)HddInfo[Index].Device, (UINTN)HddInfo[Index].Function);
    Print(L"  Command   - %04x\n", HddInfo[Index].CommandBaseAddress);
    Print(L"  Control   - %04x\n", HddInfo[Index].ControlBaseAddress);
    Print(L"  BusMaster - %04x\n", HddInfo[Index].BusMasterAddress);
    Print(L"  HddIrq    - %02x\n", HddInfo[Index].HddIrq);
    Print(L"  IdentifyDrive[0].Raw[0] - %x\n", HddInfo[Index].IdentifyDrive[0].Raw[0]);
    Print(L"  IdentifyDrive[1].Raw[0] - %x\n", HddInfo[Index].IdentifyDrive[1].Raw[0]);
  }

  Print(L"\n");

  return ;
}



/**
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/


EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE                  ImageHandle,
  IN EFI_SYSTEM_TABLE            *SystemTable
  )
{
	EFI_STATUS                        Status;
    EFI_LEGACY_BIOS_PROTOCOL         *LegacyBios;
	UINT16                            HddCount        = 0;
    UINT16                            BbsCount        = 0;
	HDD_INFO                         *LocalHddInfo    = NULL;
	BBS_TABLE                        *LocalBbsTable   = NULL;


    //
    // See if the Legacy BIOS Protocol is available
    //
    Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **) &LegacyBios);
    if (EFI_ERROR (Status)) {
      return EFI_ERROR(1);
    }

	//
	// Let platform code know the boot options
	//
	LegacyBios->GetBbsInfo(
	  LegacyBios,
	  &HddCount,
	  &LocalHddInfo,
	  &BbsCount,
	  &LocalBbsTable
	  );

	PrintPciInterruptRegister();
	PrintBbsTable(LocalBbsTable);
	PrintHddInfo(LocalHddInfo);
 
    return EFI_SUCCESS;
}
