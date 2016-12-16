/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  MpTable.c

Abstract: 
  MPS table builder functions implementation.

Revision History:

**/

#include "MpTable.h"
#include <SetupVariable.h>
#include <Library/IoLib.h>
#include <PlatformDefinition.h>




UINT8                *MpsTablePointer          = NULL;
UINT8                *MpsTableCurrentPointer   = NULL;
UINT8                *MpsExTablePointer        = NULL;
VOID                 *MpsExTableCurrentPointer = NULL;
UINT16               BaseTableLength          = 0;
UINT16               BaseTableEntryCount      = 0;
UINT16               ExtendedTableLength      = 0;
UINT8                OemId[8] = {"OEM"};
UINT8                ProductId[12] = {"8614C"};
BUS_INFO             BusEntry[MP_TABLE_MAX_BUS_ENTRIES];
UINT8                MaxBusId        = 0;
INT8                 CurrentBusEntry = 0;
UINT8                IsaBusId        = 0;
UINT8                *BusTypeString[] = {
  "CBUS  ", "CBUSII", "EISA  ", "FUTURE", "INTERN", "ISA   ",
  "MBI   ", "MBII  ", "MCA   ", "MPI   ", "MPSA  ", "NUBUS ",
  "PCI   ", "PCMCIA", "TC    ", "VL    ", "VME   ", "XPRESS"
  };

//
// number in BusTypeString array, i.e. bridge subclass 0 - PCI host has number
// 12 in array, subclass 1 - ISA bridge -number 5 etc.
//
UINT8                BridgeBusTypes[] = {
  12, 5, 2, 8, 12, 13, 11 
  }; 

IO_APIC_INFO         IoApicEntry[MP_TABLE_MAX_IO_APIC_ENTRIES];
UINT8                CurrentIoApicEntry = 0;
EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo = NULL;
CPU_INFO *gCpuInfoEntry = NULL;



 SETUP_DATA      *gSetupData;







EFI_STATUS
MpsTableAddHeader (
  VOID
  )
{
  EFI_STATUS                    Status;
  MP_CONFIGURATION_TABLE_HEADER *HeaderPointer;
  VOID                          *LegacyAddr;
  EFI_LEGACY_BIOS_PROTOCOL	    *LegacyBios;

  HeaderPointer = (MP_CONFIGURATION_TABLE_HEADER*)MpsTablePointer;

  HeaderPointer->Signature       = MP_CONFIGURATION_TABLE_SIGNATURE;
  HeaderPointer->BaseTableLength = BaseTableLength;
  HeaderPointer->VersionNumber   = MP_TABLE_VERSION_1_4;
  HeaderPointer->CheckSum        = 0;

  CopyMem (HeaderPointer->OemId, OemId, 8);
  CopyMem (HeaderPointer->ProductId, ProductId, 12);

  HeaderPointer->OemTablePointer     = 0;
  HeaderPointer->OemTableSize        = 0;
  HeaderPointer->BaseTableEntryCount = BaseTableEntryCount;
  HeaderPointer->LocalApicAddress    = LOCAL_APIC_BASE;
  HeaderPointer->ExtendedTableLength = ExtendedTableLength;
  HeaderPointer->ExtendedCheckSum    = 0;
  HeaderPointer->Reserved            = 0;

  HeaderPointer->ExtendedCheckSum = CalculateCheckSum8 (MpsExTablePointer, ExtendedTableLength);
  HeaderPointer->CheckSum = CalculateCheckSum8 (MpsTablePointer, BaseTableLength);
	
  Status = gBS->LocateProtocol (
		              &gEfiLegacyBiosProtocolGuid, 
		              NULL, 
		              (VOID**)&LegacyBios
		              );
  ASSERT_EFI_ERROR (Status);

  Status = LegacyBios->GetLegacyRegion (
                          LegacyBios,
                          (UINTN)(BaseTableLength + ExtendedTableLength),
                          0,  // E or F segment
                          16, // 16-byte aligned
                          &LegacyAddr
                          );
  ASSERT_EFI_ERROR (Status);
  ASSERT (((UINTN)LegacyAddr & 0xF) == 0);

  DEBUG((EFI_D_ERROR,"MP Table, LegacyAddr = %x \n",(UINTN)LegacyAddr));
  Status = LegacyBios->CopyLegacyRegion (
                          LegacyBios,
                          (UINTN)(BaseTableLength + ExtendedTableLength),
                          LegacyAddr,
                          MpsTablePointer
                          );
  ASSERT_EFI_ERROR (Status);

  gBS->FreePages((EFI_PHYSICAL_ADDRESS)(UINTN)MpsTablePointer, 1);
  MpsTablePointer = LegacyAddr;
  
	return EFI_SUCCESS;
}




UINT16
MpsTableAddCpuEntry (
  VOID
  )
{
  MP_TABLE_CPU_ENTRY *EntryPointer;
  UINT16             EntryCount;
  EFI_STATUS         Status;
  UINTN              ProcessorIndex;
  UINTN              NumberOfProcessors;
  UINTN              NumberOfThreadsPerCore;

  EntryCount = 0;
  EntryPointer = (MP_TABLE_CPU_ENTRY*)MpsTableCurrentPointer;
  
  Status = CollectCpuInfo (
             &NumberOfProcessors, 
             &NumberOfThreadsPerCore
             );
  ASSERT_EFI_ERROR (Status);
  
  for (ProcessorIndex = 0; ProcessorIndex < NumberOfProcessors;) {
    EntryPointer->EntryType        = MP_TABLE_CPU_ENTRY_TYPE;
    EntryPointer->LocalApicId      = (UINT8) gCpuInfoEntry[ProcessorIndex].ApicId;
    EntryPointer->LocalApicVersion = gCpuInfoEntry[ProcessorIndex].ApicVersion;
    if ((gCpuInfoEntry[ProcessorIndex].StatusFlag & PROCESSOR_AS_BSP_BIT) == PROCESSOR_AS_BSP_BIT)
      EntryPointer->CpuFlags.CpuBootStrap    = 1;
    else 
      EntryPointer->CpuFlags.CpuBootStrap    = 0;
		
    if ((gCpuInfoEntry[ProcessorIndex].StatusFlag & PROCESSOR_ENABLED_BIT) == PROCESSOR_ENABLED_BIT)
      EntryPointer->CpuFlags.CpuEnable     = 1;
    else 
      EntryPointer->CpuFlags.CpuEnable     = 0;
		
    EntryPointer->CpuFlags.Reserved        = 0;
    EntryPointer->CpuSignature = gCpuInfoEntry[ProcessorIndex].Signature;
    EntryPointer->FeatureFlags = gCpuInfoEntry[ProcessorIndex].FeatureFlags;
    EntryPointer->Reserved[0]  = 0;
    EntryPointer->Reserved[1]  = 0;

    EntryPointer++;
    EntryCount++;

    // 
    // According to non ACPI requirements, only needs to report the 1st thread of each core.
    //
    ProcessorIndex += NumberOfThreadsPerCore;
  }

  if(gCpuInfoEntry!=NULL){FreePool(gCpuInfoEntry);gCpuInfoEntry = NULL;}
	
  return EntryCount;
}

UINT16
MpsTableAddBusEntry (
  VOID
  )
{
  MP_TABLE_BUS_ENTRY *EntryPointer;
  UINT16             EntryCount;
  UINT16             i;

  EntryCount   = 0;
  EntryPointer = (MP_TABLE_BUS_ENTRY *) MpsTableCurrentPointer;

  QueryBusInfo();

  for (i = 0; i < (UINT16)CurrentBusEntry; i++) {
    EntryPointer->EntryType = MP_TABLE_BUS_ENTRY_TYPE;
    EntryPointer->BusId     = BusEntry[i].BusId;
    CopyMem (EntryPointer->BusTypeString, BusTypeString[(BusEntry[i].BusType)], 6);
    EntryPointer++;
    EntryCount++;
  }
  return EntryCount;
}

UINT16
MpsTableAddIoApicEntry (
  VOID
  )
{
  MP_TABLE_IO_APIC_ENTRY *EntryPointer;
  UINT16                 EntryCount;
  EFI_PHYSICAL_ADDRESS   SearchAddress;
  UINT8                  i;

  EntryCount   = 0;
  EntryPointer = (MP_TABLE_IO_APIC_ENTRY *) MpsTableCurrentPointer;

  //
  // first we detect IO APICS mapped into default address range
  //
  for (SearchAddress = IO_APIC_BASE_ADDRESS_BOTTOM;
       SearchAddress < IO_APIC_BASE_ADDRESS_TOP;
       SearchAddress += IO_APIC_SEARCH_STEP) {
    if (*((UINT8 *) (UINTN) SearchAddress) != 0xFF)
      CollectIoApicInfo (NULL, (UINT32) SearchAddress);
  }

  // 
  // Second we arrange IO APIC in their ID ascending order (if there more than one)
  //
  if (CurrentIoApicEntry > 1)
    ArrangeIoApicEntries();

  //
  // now we are ready to create MPS table IO APIC entries
  //
  for (i = 0; i < CurrentIoApicEntry; i++) {
    EntryPointer->EntryType     = MP_TABLE_IO_APIC_ENTRY_TYPE;
    EntryPointer->IoApicId      = IoApicEntry[i].IoApicId;
    EntryPointer->IoApicVersion = IoApicEntry[i].IoApicVersion;
    EntryPointer->Flags         = IoApicEntry[i].IoApicFlags;
    EntryPointer->IoApicAddress = IoApicEntry[i].IoApicAddress;
    EntryPointer++;
    EntryCount++;
  }
  return EntryCount;
}



UINT16
MpsTableAddLocalIntAssignEntry (
  VOID
  )
{
  MP_TABLE_LOCAL_INT_ASSIGN_ENTRY *EntryPointer;
  UINT16                          EntryCount;

  EntryCount = 0;
  EntryPointer = (MP_TABLE_LOCAL_INT_ASSIGN_ENTRY *) MpsTableCurrentPointer;

  EntryPointer->EntryType = MP_TABLE_LOCAL_INT_ASSIGN_ENTRY_TYPE;
  EntryPointer->InterruptType = INT_TYPE_EXT_INT;
  EntryPointer->InterruptSignal.Polarity = POLARITY_CONFORM_SPEC;
  EntryPointer->InterruptSignal.TriggerMode = TRIGGER_MODE_CONFORM_SPEC;
  EntryPointer->InterruptSignal.Reserved = 0;
  EntryPointer->SourceBusId = 0;
  EntryPointer->SourceBusIrq = 0;
  EntryPointer->DestLocalApicId = 0xFF;
  EntryPointer->DestLocalApicItin = 0;

  EntryPointer++;
  EntryCount++;

  EntryPointer->EntryType = MP_TABLE_LOCAL_INT_ASSIGN_ENTRY_TYPE;
  EntryPointer->InterruptType = INT_TYPE_NMI;
  EntryPointer->InterruptSignal.Polarity = POLARITY_CONFORM_SPEC;
  EntryPointer->InterruptSignal.TriggerMode = TRIGGER_MODE_CONFORM_SPEC;
  EntryPointer->InterruptSignal.Reserved = 0;
  EntryPointer->SourceBusId = 0;
  EntryPointer->SourceBusIrq = 0;
  EntryPointer->DestLocalApicId = 0xFF;
  EntryPointer->DestLocalApicItin = 1;

  EntryPointer++;
  EntryCount++;

  return EntryCount;
}



VOID
MpsExTableAddSysAddressMapEntry (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *pPciRootBridgeIoProtocol,
  IN UINT8                           BusId,
  IN UINT8                           BusAttributes
  )
{
  MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY *EntryPointer;
  ACPI_ADDRESS_SPACE_DESC           *Resources;
  BOOLEAN                           VgaMemoryAdded;

  VgaMemoryAdded = FALSE;
  EntryPointer   = (MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY *) MpsExTableCurrentPointer;

  pPciRootBridgeIoProtocol->Configuration (pPciRootBridgeIoProtocol, &Resources);

  //
  // Add entries to ISA decode
  //
  if ((BusAttributes & 0x3) != 0) {
    //
    // Supports ISA I/O space
    //
    EntryPointer->EntryType   = MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY_TYPE;
    EntryPointer->EntryLength = sizeof (MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY);
    EntryPointer->BusId       = BusId;
    EntryPointer->AddressType = 0;
    EntryPointer->AddressBase = 0;
    EntryPointer->AddressLength = 0x1000;
    EntryPointer++;
    ExtendedTableLength        += sizeof (MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY);
  }

  while (Resources->Signature == 0x8A) {
    if (Resources->ResourceType != 2  && Resources->AddressLength) {
      if (Resources->ResourceType == 1)
        EntryPointer->AddressType = 0;
      else if (Resources->TypeSpecificFlags & 6)
        EntryPointer->AddressType = 2;          
      else
        EntryPointer->AddressType = 1;          

      //
      // VGA region should be at the beginning of Memory range
      //
      if (EntryPointer->AddressType == 1 && (BusAttributes & 0x1C) != 0 && !VgaMemoryAdded) {
        EntryPointer->EntryType   = MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY_TYPE;
        EntryPointer->EntryLength = sizeof (MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY);
        EntryPointer->BusId       = BusId;
        EntryPointer->AddressType = 1; 
        EntryPointer->AddressBase = 0xA0000;
        EntryPointer->AddressLength = 0x20000;
        EntryPointer++;
        ExtendedTableLength += sizeof (MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY);
        VgaMemoryAdded      = TRUE;
        EntryPointer->AddressType = 1;
      }

      EntryPointer->EntryType = MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY_TYPE;
      EntryPointer->EntryLength = sizeof (MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY);
      EntryPointer->BusId = BusId;
      EntryPointer->AddressBase = Resources->AddressRangeMin;
      EntryPointer->AddressLength = Resources->AddressLength;

      EntryPointer++;
      ExtendedTableLength += sizeof (MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY);
    }
    Resources++;
  }

  MpsExTableCurrentPointer = (VOID *) EntryPointer;
}




VOID
MpsExTableAddBusHierarchyEntry (
  VOID
  )
{
  MP_EX_TABLE_BUS_HIERARCHY_ENTRY *EntryPointer;
  UINT8                           i;

  EntryPointer = (MP_EX_TABLE_BUS_HIERARCHY_ENTRY *) MpsExTableCurrentPointer;

  for (i = 0; i < CurrentBusEntry; i++) {
    //
    // Skips root bridge and P2P bridge
    //
    if (BusEntry[i].ParentBusId != 0xFF && BusEntry[i].BusType != 12) {
      EntryPointer->EntryType   = MP_EX_TABLE_BUS_HIERARCHY_ENTRY_TYPE;
      EntryPointer->EntryLength = sizeof (MP_EX_TABLE_BUS_HIERARCHY_ENTRY);
      EntryPointer->BusId       = BusEntry[i].BusId;
      EntryPointer->BusInfo     = BusEntry[i].Flags;
      EntryPointer->ParentBusId = BusEntry[i].ParentBusId;
      EntryPointer->Reserved[0] = 0;
      EntryPointer->Reserved[1] = 0;
      EntryPointer->Reserved[2] = 0;

      EntryPointer++;
      ExtendedTableLength += sizeof (MP_EX_TABLE_BUS_HIERARCHY_ENTRY);
    }
  }
  MpsExTableCurrentPointer = (VOID *) EntryPointer;
}




VOID 
MpsExTableAddCompatBusAddressModifierEntry (
  VOID
  )
{
  MP_EX_TABLE_COMPAT_BUS_ADDRESS_MODIFIER_ENTRY *EntryPointer;
  UINT8                                          i;

  EntryPointer = (MP_EX_TABLE_COMPAT_BUS_ADDRESS_MODIFIER_ENTRY *) MpsExTableCurrentPointer;

  for (i = 0; i < CurrentBusEntry; i++) {
    if (BusEntry[i].ParentBusId == 0xFF) {
      //
      // Add subtract ISA predefined range
      //
      EntryPointer->EntryType   = MP_EX_TABLE_COMPAT_BUS_ADDRESS_MODIFIER_ENTRY_TYPE;
      EntryPointer->EntryLength = sizeof (MP_EX_TABLE_COMPAT_BUS_ADDRESS_MODIFIER_ENTRY);
      EntryPointer->BusId       = BusEntry[i].BusId;
      EntryPointer->RangeList   = 0;
      EntryPointer->AddressModifier = (BusEntry[i].Flags & 0x3) ? 0 : 1;

      EntryPointer++;
      ExtendedTableLength += sizeof (MP_EX_TABLE_COMPAT_BUS_ADDRESS_MODIFIER_ENTRY);

      //
      // Add/subtract VGA predefined range
      //
      EntryPointer->EntryType   = MP_EX_TABLE_COMPAT_BUS_ADDRESS_MODIFIER_ENTRY_TYPE;
      EntryPointer->EntryLength = sizeof (MP_EX_TABLE_COMPAT_BUS_ADDRESS_MODIFIER_ENTRY);
      EntryPointer->BusId       = BusEntry[i].BusId;
      EntryPointer->RangeList   = 1;
      EntryPointer->AddressModifier = (BusEntry[i].Flags & 0x1C) ? 0 : 1;

      EntryPointer++;
      ExtendedTableLength += sizeof (MP_EX_TABLE_COMPAT_BUS_ADDRESS_MODIFIER_ENTRY);
    }
  }
  MpsExTableCurrentPointer = (VOID *) EntryPointer;
}

//YKN_20160309 +S
typedef  struct {
    UINT8 Seg;
    UINT8 Bus;
	UINT8 Dev;
	UINT8 Fun;
	UINT8 PriBus;
	UINT8 SecBus;
	UINT8 Enabled;
	UINT8 Rsved;
} PCI_BRIDGE;

PCI_BRIDGE gPlatformPciBridges[] = {
		{0, 0, 2, 0, 0, 0, 0, 0},
		{0, 0, 3, 0, 0, 0, 0, 0},
		{0, 0, 3, 1, 0, 0, 0, 0},
		{0, 0, 3, 2, 0, 0, 0, 0},
		{0, 0, 3, 3, 0, 0, 0, 0},
		{0, 0, 4, 0, 0, 0, 0, 0},
		{0, 0, 4, 1, 0, 0, 0, 0},
		{0, 0, 5, 0, 0, 0, 0, 0},
		{0, 0, 5, 1, 0, 0, 0, 0}
};
#define NUM_PCI_BRIDGES 9
//YKN_20160309 +E

VOID
QueryBusInfo (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_HANDLE *pHandleBuffer;
  UINTN      NumberOfHandles;
  UINTN      i;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *pPciRootBridgeIoProtocol;
  ACPI_ADDRESS_SPACE_DESC          *pAddressDescriptor;
  EFI_PCI_IO_PROTOCOL *pPciIoProtocol;
  UINT8   PciData[4];
  UINTN   BusNumber;
  UINTN   dummy[3];
  UINT64  SupportedAttributes;

  //Yankui-add-s
  UINTN  Index;
  UINTN  Seg, Bus, Dev, Fun;
  //Yankui-add-e
  //
  // First detect and retreive bus Id for PCI host (root) buses
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol, 
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  &NumberOfHandles, 
                  &pHandleBuffer
                  );
  if (EFI_ERROR (Status))
    return;

  for (i = 0; i < NumberOfHandles; i++) {
    Status = gBS->HandleProtocol (
                    pHandleBuffer[i], 
                    &gEfiPciRootBridgeIoProtocolGuid,
                    (VOID**) & pPciRootBridgeIoProtocol
                    );
    Status = pPciRootBridgeIoProtocol->Configuration (
                                         pPciRootBridgeIoProtocol,
                                         &pAddressDescriptor
                                         );
    Status = pPciRootBridgeIoProtocol->GetAttributes (
                                         pPciRootBridgeIoProtocol,
                                         &SupportedAttributes,
                                         NULL
                                         );

    // 
    // Find bus range decoding - the min will be host PCI bus Id
    //
    while (pAddressDescriptor->Signature == 0x8A && pAddressDescriptor->ResourceType != 2)
      pAddressDescriptor++;

    BusEntry[CurrentBusEntry].BusId       = (UINT8) pAddressDescriptor->AddressRangeMin;
    BusEntry[CurrentBusEntry].ParentBusId = 0xFF;
    BusEntry[CurrentBusEntry].BusType     = BridgeBusTypes[0];
    BusEntry[CurrentBusEntry].Flags       = (UINT8) SupportedAttributes;

    MaxBusId = (MaxBusId > BusEntry[CurrentBusEntry].BusId) ? \
               MaxBusId : BusEntry[CurrentBusEntry].BusId;

    MpsExTableAddSysAddressMapEntry (
      pPciRootBridgeIoProtocol,
      BusEntry[CurrentBusEntry].BusId,
      BusEntry[CurrentBusEntry].Flags
      );
    CurrentBusEntry++;
  }
  gBS->FreePool (pHandleBuffer);

  //
  // Second detect all other buses via PCI bridges
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol, 
                  &gEfiPciIoProtocolGuid,
                  NULL, 
                  &NumberOfHandles, 
                  &pHandleBuffer
                  );
  if (EFI_ERROR (Status))
    return;

  for (i = 0; i < NumberOfHandles; i++) {
    Status = gBS->HandleProtocol (
                    pHandleBuffer[i], 
                    &gEfiPciIoProtocolGuid,
                    (VOID**) &pPciIoProtocol
                    );
    if (EFI_ERROR (Status))
      continue;

    //
    // Read class code information at 0x8 offset in PCI header
    //
    Status = pPciIoProtocol->Pci.Read (
                                   pPciIoProtocol, 
                                   EfiPciIoWidthUint32,
                                   0x8, 
                                   1, 
                                   (VOID*) PciData
                                   );
    if (EFI_ERROR (Status))
      continue;

    //
    // If IO APIC device collect info, we will need it later
    //
    //if ((PciData[3] == 0x8) && (PciData[2] == 0) && (PciData[2] >= 0x10)) {
	if ((PciData[3] == 0x8) && (PciData[2] == 0) && (PciData[1] >= 0x10)) {
      CollectIoApicInfo (pPciIoProtocol, 0);
      continue;
    }

    //
    // not bridge device
    //
    if (PciData[3] != 0x6)
      continue;

    //
    // PCI host bridge we already know, so skip it or unknown bridge - skip it also
    //
    if ((PciData[2] == 0) || (PciData[2] > 6)) 
      continue;                                

    BusEntry[CurrentBusEntry].BusType = BridgeBusTypes[(PciData[2])];

    //
    // PCI to PCI bridge
    //
    if (PciData[2] == 4) {
      BusEntry[CurrentBusEntry].Flags = PciData[1];
      //
      // Read primary and secondary bus numbers
      //
      Status = pPciIoProtocol->Pci.Read (
                                     pPciIoProtocol, 
                                     EfiPciIoWidthUint32,
                                     0x18, 
                                     1, 
                                     (VOID*) PciData
                                     );
      //
      // Bus Id prodused by this bridge
      //
      BusEntry[CurrentBusEntry].BusId       = PciData[1];
      BusEntry[CurrentBusEntry].ParentBusId = PciData[0];
      MaxBusId = (MaxBusId > BusEntry[CurrentBusEntry].BusId) ? \
                 MaxBusId : BusEntry[CurrentBusEntry].BusId;

     //YKN_20160309 +S
	  pPciIoProtocol->GetLocation(pPciIoProtocol, &Seg, &Bus, &Dev, &Fun);
	 ///DEBUG((EFI_D_ERROR,"[MTN-DBG]:Seg=0x%x Bus=0x%x Dev=0x%x Fun=0x%x \n",Seg,Bus,Dev,Fun));
	  for(Index=0;Index < NUM_PCI_BRIDGES; Index++) {
	  	if((Bus == gPlatformPciBridges[Index].Bus) &&
			(Dev == gPlatformPciBridges[Index].Dev) &&(Fun == gPlatformPciBridges[Index].Fun)) {
            gPlatformPciBridges[Index].Enabled = 1;
			gPlatformPciBridges[Index].PriBus = PciData[0];
			gPlatformPciBridges[Index].SecBus = PciData[1];
			break;
		}
	  }
	  ASSERT(Index < NUM_PCI_BRIDGES);
	  //YKN_20160309 +E

    } else {     
      //
      // Any other bridge.  
      //

      //
      // all other buses has this feature
      //
      BusEntry[CurrentBusEntry].Flags = 1; 

      //
      // We will assign Id after enumerating PCI buses
      //
      BusEntry[CurrentBusEntry].BusId = 0xFF;

      //
      // Get bridge location to determine parent bus Id
      //
      Status = pPciIoProtocol->GetLocation (
                                 pPciIoProtocol, 
                                 &dummy[0], 
                                 &BusNumber,
                                 &dummy[1], 
                                 &dummy[2]
                                 );
      BusEntry[CurrentBusEntry].ParentBusId = (UINT8) BusNumber;
    }
    CurrentBusEntry++;
  }
  gBS->FreePool (pHandleBuffer);

  //
  // Third enumerate all other buses
  //
  for (i = 0; i < (UINTN)CurrentBusEntry; i++) {
    if (BusEntry[i].BusId == 0xFF) {
      BusEntry[i].BusId = MaxBusId + 1;
      MaxBusId++;
      if (BusEntry[i].BusType == BUS_TYPE_ISA)
        //
        // Save ISA bus Id for interrupt assign procedure
        //
        IsaBusId = MaxBusId;
    }
  }
  
  MpsExTableAddBusHierarchyEntry ();
  MpsExTableAddCompatBusAddressModifierEntry ();
}

VOID 
CollectIoApicInfo (
  IN EFI_PCI_IO_PROTOCOL *pPciIoProtocol OPTIONAL,
  IN UINT32              BaseAddress OPTIONAL
  )
/*++

Routine Description:

  Retreive system info about IO APIC

Arguments:

  pPciIoProtocol  - Pointer to PciIo Protocol
  BaseAddress     - Base Address

Returns:

  VOID

--*/
{
  UINT32           IoApicAddress;
  EFI_STATUS       Status;
  UINT32 volatile *IoApicMemory32;
  UINT8  volatile *IoApicMemory8;
  UINT32          data;

  if (BaseAddress == 0) { 
    //
    // IO APIC visible as PCI device
    //
    Status = pPciIoProtocol->Pci.Read (
                                   pPciIoProtocol, 
                                   EfiPciIoWidthUint32,
                                   0x10, 
                                   1, 
                                   (VOID*) & IoApicAddress
                                   );
    if (EFI_ERROR (Status) || (IoApicAddress == 0)) {
      //
      // Problem or mapped to default address range
      //
      return;
    }
  } else {
    //
    // IO APIC mapped to default range
    //
    IoApicAddress = BaseAddress;
  }
  IoApicMemory32 = (UINT32 *) (UINTN) IoApicAddress;
  IoApicMemory8  = (UINT8 *) (UINTN) IoApicAddress;

  IoApicMemory8[IO_APIC_INDEX_REG] = IO_APIC_MAP_ID_REG;
  data = IoApicMemory32[IO_APIC_DATA_REG];

  IoApicEntry[CurrentIoApicEntry].IoApicId = (UINT8) ((data & 0x0F000000) >> 24);

  IoApicMemory8[IO_APIC_INDEX_REG] = IO_APIC_MAP_VERSION_REG;
  data  = IoApicMemory32[IO_APIC_DATA_REG];

  IoApicEntry[CurrentIoApicEntry].IoApicVersion = (UINT8) (data & 0xFF);
  IoApicEntry[CurrentIoApicEntry].MaxRedirectionEntries = (UINT8) (((data & 0x00FF0000) >> 16) + 1);
  IoApicEntry[CurrentIoApicEntry].IoApicFlags = 1;
  IoApicEntry[CurrentIoApicEntry].IoApicAddress = IoApicAddress;

  CurrentIoApicEntry++;
}




VOID
ArrangeIoApicEntries (
  VOID
  )
{
  UINT8        i;
  BOOLEAN      Swap;
  IO_APIC_INFO Temp;

  Swap = TRUE;
  while (Swap) {
    Swap = FALSE;
    for (i = 0; i < CurrentIoApicEntry - 1; i++) {
      if (IoApicEntry[i].IoApicId > IoApicEntry[i+1].IoApicId) {
        Swap = TRUE;
        CopyMem (&Temp, &IoApicEntry[i], sizeof (IO_APIC_INFO));
        CopyMem (&IoApicEntry[i], &IoApicEntry[i+1], sizeof (IO_APIC_INFO));
        CopyMem (&IoApicEntry[i+1], &Temp, sizeof (IO_APIC_INFO));
      }
    }
  }
}



VOID 
GetIoApicId (
  IN UINT16   SysVectorNumber,
  OUT UINT8 *IoApicId,
  OUT UINT8 *IoApicItin
  )
{
  UINT8 i;

  for (i = 0; i < CurrentIoApicEntry; i++) {
    if (SysVectorNumber < IoApicEntry[i].MaxRedirectionEntries) {
      *IoApicId   = IoApicEntry[i].IoApicId;
      *IoApicItin = (UINT8) SysVectorNumber;
      return;
    }
    SysVectorNumber = SysVectorNumber - IoApicEntry[i].MaxRedirectionEntries;
  }
}



VOID
ApGetCpuInfo (
  IN  VOID  *Buffer
  )
{
  UINT32                    RegEax;
  UINT32                    RegEdx;
  UINTN                     ProcessorIndex;

  ProcessorIndex = (UINTN)Buffer;

  if (((AsmReadMsr64 (MSR_XAPIC_BASE)) & (BIT11 + BIT10)) == (BIT11 + BIT10)) {
    gCpuInfoEntry[ProcessorIndex].ApicVersion = (UINT8)AsmReadMsr64 (MSR_EXT_XAPIC_VERSION);
  } else {
    gCpuInfoEntry[ProcessorIndex].ApicVersion = (UINT8)(*(volatile UINT32*)(UINTN)(LOCAL_APIC_BASE + APIC_VERSION_REGISTER));
  }

  AsmCpuid (1, &RegEax, NULL, NULL, &RegEdx);
  gCpuInfoEntry[ProcessorIndex].Signature    = RegEax;
  gCpuInfoEntry[ProcessorIndex].FeatureFlags = RegEdx;

}


EFI_STATUS 
CollectCpuInfo (
  IN OUT UINTN     *NumberOfProcessors,
  IN OUT UINTN     *NumberOfThreadsPerCore
  )
{
  EFI_STATUS                Status;
  EFI_MP_SERVICES_PROTOCOL  *MpService;
  EFI_PROCESSOR_INFORMATION ProcessorInfoBuffer;
  UINTN                     ProcessorIndex;
  UINTN                     NumberOfEnabledProcessors;
  BOOLEAN                   Finished;

  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL,(VOID**)&MpService);
  ASSERT_EFI_ERROR (Status);

  Status = MpService->GetNumberOfProcessors (
                        MpService, 
                        NumberOfProcessors, 
                        &NumberOfEnabledProcessors
                        );
  ASSERT_EFI_ERROR(Status);

  if(gCpuInfoEntry==NULL){
    gCpuInfoEntry = AllocatePool(sizeof(CPU_INFO) * (*NumberOfProcessors));
    ASSERT(gCpuInfoEntry!=NULL);
    ZeroMem(gCpuInfoEntry, sizeof(CPU_INFO) * (*NumberOfProcessors));
  }
	
  *NumberOfThreadsPerCore = 0;
  
  for (ProcessorIndex = 0; ProcessorIndex < *NumberOfProcessors; ProcessorIndex++) {
    Status = MpService->GetProcessorInfo (
                          MpService,
                          ProcessorIndex,
                          &ProcessorInfoBuffer
                          );
    ASSERT_EFI_ERROR (Status);
    
    //
    // Both of NumberOfThreads and NumberOfCores are 0 based
    //
    if (*NumberOfThreadsPerCore < ProcessorInfoBuffer.Location.Thread + 1) {
      *NumberOfThreadsPerCore = ProcessorInfoBuffer.Location.Thread + 1;
    }

    gCpuInfoEntry[ProcessorIndex].StatusFlag = ProcessorInfoBuffer.StatusFlag;
    gCpuInfoEntry[ProcessorIndex].ApicId     = ProcessorInfoBuffer.ProcessorId;

    if (ProcessorInfoBuffer.StatusFlag & PROCESSOR_AS_BSP_BIT) {
      ApGetCpuInfo ((VOID *)ProcessorIndex);
    } else {
      Status = MpService->StartupThisAP (
                            MpService, 
                            ApGetCpuInfo, 
                            ProcessorIndex, 
                            NULL, 
                            0, 
                            (VOID *)ProcessorIndex,
                            &Finished
                            );
      ASSERT_EFI_ERROR(Status);
    }
  }

  return EFI_SUCCESS;
}




STATIC MP_TABLE_INT_ASSIGN_ENTRY gType3List[] = {
// 	            EntryTYPE              IntType     Flag  SrcBusId  SrcBusIrq   DestApicId   DestApicIn
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     10,        0,           9,           2},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     10,        1,           9,           1}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT, 0xF,     10,        3,           9,           3}, 	
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT, 0xF,     10,        4,           9,           4},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     10,        8,           9,           8},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT, 0xF,     10,        9,           9,           9}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     10,      0xC,           9,         0xC}, 	
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     10,      0xD,           9,         0xD},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     10,      0xE,           9,         0xE}, 	
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     10,      0xF,           9,         0xF},

  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (2<<2)|0,       0xA,        0xB}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (3<<2)|0,       0xA,          7},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (3<<2)|1,       0xA,         0xB},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (3<<2)|2,       0xA,         0xF},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (3<<2)|3,       0xA,         0x13},

  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (4<<2)|0,       0xA,         0x3},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (4<<2)|1,       0xA,         0x17},
  	
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (5<<2)|0,       0xA,         0x3}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (5<<2)|1,       0xA,         0x7}, 

  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (1<<2)|0,       0xA,        0x11}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (1<<2)|1,       0xA,        0x12}, 

  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (8<<2)|0,       0xA,        0x10}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (9<<2)|0,       0xA,        0x0C}, 

  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (0xA<<2)|0,     0x9,        0x10}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (0xA<<2)|1,     0x9,        0x11}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (0xA<<2)|2,     0x9,        0x12}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (0xA<<2)|3,     0x9,        0x13}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (0xB<<2)|0,     0x9,        0x15}, ///MTN_20160325
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (0xC<<2)|0,     0x9,        0x16}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     0,     (0x14<<2)|0,    0x9,        0x11}, 
  	
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     1,       (0<<2)|0,     0xA,        8}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     1,       (0<<2)|1,     0xA,        9},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     1,       (0<<2)|2,     0xA,        0xA},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     1,       (0<<2)|3,     0xA,        0xB}, 

  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     2,       (0<<2)|0,     0xA,        4}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     2,       (0<<2)|1,     0xA,        5},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     2,       (0<<2)|2,     0xA,        6},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     2,       (0<<2)|3,     0xA,        7},

  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     3,       (0<<2)|0,     0xA,        8}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     3,       (0<<2)|1,     0xA,        9},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     3,       (0<<2)|2,     0xA,        0xA},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     3,       (0<<2)|3,     0xA,        0xB}, 

  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     4,       (0<<2)|0,     0xA,        0xC}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     4,       (0<<2)|1,     0xA,        0xD},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     4,       (0<<2)|2,     0xA,        0xE},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     4,       (0<<2)|3,     0xA,        0xF}, 

  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     5,       (0<<2)|0,     0xA,        0x10}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     5,       (0<<2)|1,     0xA,        0x11},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     5,       (0<<2)|2,     0xA,        0x12},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     5,       (0<<2)|3,     0xA,        0x13}, 

  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     6,       (0<<2)|0,     0xA,        0}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     6,       (0<<2)|1,     0xA,        1},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     6,       (0<<2)|2,     0xA,        2},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     6,       (0<<2)|3,     0xA,        3},  

  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     7,       (0<<2)|0,     0xA,        0x14}, ///MTN_20160325
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     7,       (0<<2)|1,     0xA,        0x15}, ///MTN_20160325
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     7,       (0<<2)|2,     0xA,        0x16}, ///MTN_20160325
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     7,       (0<<2)|3,     0xA,        0x17}, ///MTN_20160325

  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     8,       (0<<2)|0,     0xA,        0}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     8,       (0<<2)|1,     0xA,        1},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     8,       (0<<2)|2,     0xA,        2},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     8,       (0<<2)|3,     0xA,        3},  

  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     9,       (0<<2)|0,     0xA,        4}, 
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     9,       (0<<2)|1,     0xA,        5},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     9,       (0<<2)|2,     0xA,        6},
  	{MP_TABLE_INT_ASSIGN_ENTRY_TYPE, INT_TYPE_INT,   0,     9,       (0<<2)|3,     0xA,        7},  

};	

//YKN_20160309 +S

#define NUM_INTERNAL_DEVICE 11
#define ISA_IRQ_START_INDEX 0
#define ISA_IRQ_END_INDEX 9

#define BRIDGE_IRQ_START_INDEX 10
#define BRIDGE_IRQ_END_INDEX (BRIDGE_IRQ_START_INDEX + NUM_PCI_BRIDGES - 1)

#define INTNAL_DEVICE_IRQ_START_INDEX (BRIDGE_IRQ_END_INDEX + 1)
#define INTNAL_DEVICE_IRQ_END_INDEX  (INTNAL_DEVICE_IRQ_START_INDEX + NUM_INTERNAL_DEVICE -1 )

#define SLOT0_IRQ_START_INDEX (INTNAL_DEVICE_IRQ_END_INDEX + 1)
#define SLOT1_IRQ_START_INDEX (SLOT0_IRQ_START_INDEX + 4)


UINT16
MpsTableAddIntAssignEntry (
  VOID
  )
{
  UINTN  Index, Len, Index2;
  UINT16 Count=0;
  UINT8  *Ptr=MpsTableCurrentPointer;
  MP_TABLE_INT_ASSIGN_ENTRY *StartPtr, *BackupPtr;

  UINT8   PEGIoApicItinIBuf[4];
  UINT8   PEG2IoApicItinIBuf[4];
  UINT8   PEG3IoApicItinIBuf[4];
  UINT8   RintrApicPEG = 0;
  UINT8   RintrApicPEG2 = 0;
  UINT8   RintrApicPEG3 = 0;

  
///MTN-20161022 Start
   RintrApicPEG = MmioRead8(HC_PCI_REG(D0F0_PCIE_PORT_G_INTR_ROUTING_SEL)) >> 4;
   RintrApicPEG2 = MmioRead8(HC_PCI_REG(D0F0_PCIE_PORT_G2_INTR_ROUTING_SEL)) >> 4;
   RintrApicPEG3 = MmioRead8(HC_PCI_REG(D0F0_PCIE_PORT_G3_INTR_ROUTING_SEL)) >> 4;

  DEBUG((EFI_D_ERROR,"RintrApicPEG=0x%x\n",RintrApicPEG));
  DEBUG((EFI_D_ERROR,"RintrApicPEG2=0x%x\n",RintrApicPEG2));
  DEBUG((EFI_D_ERROR,"RintrApicPEG3=0x%x\n",RintrApicPEG3));

  DEBUG((EFI_D_ERROR,"%a()\n",__FUNCTION__));
  //DEBUG((EFI_D_ERROR,"%d,%d,%d,%d,%d,%d\n",BRIDGE_IRQ_START_INDEX,BRIDGE_IRQ_END_INDEX,INTNAL_DEVICE_IRQ_START_INDEX,
  //	INTNAL_DEVICE_IRQ_END_INDEX,SLOT0_IRQ_START_INDEX,SLOT1_IRQ_START_INDEX));
 gSetupData = GetSetupDataHobData();
  for(Index = ISA_IRQ_START_INDEX; Index <= ISA_IRQ_END_INDEX; Index++) {
  	gType3List[Index].SourceBusId = IsaBusId;
  }
  StartPtr = &gType3List[ISA_IRQ_START_INDEX];
  BackupPtr = &gType3List[ISA_IRQ_END_INDEX];
  
  Len = (UINT8*)(BackupPtr + 1) - (UINT8*)StartPtr;
  CopyMem(Ptr, StartPtr, Len);
  Ptr += Len;
  Count = Count + ISA_IRQ_END_INDEX-ISA_IRQ_START_INDEX+1;

  StartPtr = &gType3List[BRIDGE_IRQ_START_INDEX];
  Len = sizeof(gType3List[0]);
  for(Index = 0; Index <= NUM_PCI_BRIDGES; Index++) {
    if(gPlatformPciBridges[Index].Enabled) {
		CopyMem(Ptr, StartPtr, Len);
		Ptr += Len;
		Count++;
    }
	StartPtr++;
  }

  StartPtr = &gType3List[INTNAL_DEVICE_IRQ_START_INDEX];
  BackupPtr = &gType3List[INTNAL_DEVICE_IRQ_END_INDEX];
  Len = (UINT8*)(BackupPtr + 1) - (UINT8*)StartPtr;
  CopyMem(Ptr, StartPtr, Len);
  Ptr += Len;
  Count = Count + INTNAL_DEVICE_IRQ_END_INDEX - INTNAL_DEVICE_IRQ_START_INDEX + 1;
  	
  StartPtr = &gType3List[SLOT0_IRQ_START_INDEX];
  Len = 4*sizeof(MP_TABLE_INT_ASSIGN_ENTRY);
  for(Index = 0; Index < NUM_PCI_BRIDGES; Index++) {
  	if(gPlatformPciBridges[Index].Enabled) {
		BackupPtr = StartPtr; 

	if(gPlatformPciBridges[Index].Dev == 2) {///D2F0 PEG	

	
	switch(RintrApicPEG){	 
	  case 0:						 
		  PEGIoApicItinIBuf[0] = 8;
		  PEGIoApicItinIBuf[1] = 9;
		  PEGIoApicItinIBuf[2]= 10;
		  PEGIoApicItinIBuf[3]= 11;
		break;
		
	  case 1:						  
		PEGIoApicItinIBuf[0] = 9; 
		PEGIoApicItinIBuf[1] = 10;
		PEGIoApicItinIBuf[2]= 11;
		PEGIoApicItinIBuf[3]= 8;
		break;
	
	  case 2:						  
		PEGIoApicItinIBuf[0] = 10;
		PEGIoApicItinIBuf[1] = 11;
		PEGIoApicItinIBuf[2]= 8;
		PEGIoApicItinIBuf[3]= 9;
		break;
		
	  case 3:						
		PEGIoApicItinIBuf[0] = 11; 
		PEGIoApicItinIBuf[1] = 8;
		PEGIoApicItinIBuf[2]= 9;
		PEGIoApicItinIBuf[3]= 10;
		break;		
	}
      }

	if((gPlatformPciBridges[Index].Dev == 5) &&((gPlatformPciBridges[Index].Fun == 0))) {///D5F0 PEG2	
	switch(RintrApicPEG2){	 
	  case 0:						 
		  PEG2IoApicItinIBuf[0] = 0;
		  PEG2IoApicItinIBuf[1] = 1;
		  PEG2IoApicItinIBuf[2]= 2;
		  PEG2IoApicItinIBuf[3]= 3;
		break;
		
	  case 1:						  
		PEG2IoApicItinIBuf[0] = 1; 
		PEG2IoApicItinIBuf[1] = 2;
		PEG2IoApicItinIBuf[2]= 3;
		PEG2IoApicItinIBuf[3]= 0;
		break;
	
	  case 2:						  
		PEG2IoApicItinIBuf[0] = 2;
		PEG2IoApicItinIBuf[1] = 3;
		PEG2IoApicItinIBuf[2]= 0;
		PEG2IoApicItinIBuf[3]= 1;
		break;
		
	  case 3:						
		PEG2IoApicItinIBuf[0] = 3; 
		PEG2IoApicItinIBuf[1] = 0;
		PEG2IoApicItinIBuf[2]= 1;
		PEG2IoApicItinIBuf[3]= 2;
		break;		
	}
     }

	if((gPlatformPciBridges[Index].Dev == 5) &&((gPlatformPciBridges[Index].Fun == 1))) {///D5F1 PEG3	
	switch(RintrApicPEG3){	 
	  case 0:						 
		  PEG3IoApicItinIBuf[0] = 4;
		  PEG3IoApicItinIBuf[1] = 5;
		  PEG3IoApicItinIBuf[2]= 6;
		  PEG3IoApicItinIBuf[3]= 7;
		break;
		
	  case 1:						  
		PEG3IoApicItinIBuf[0] = 5; 
		PEG3IoApicItinIBuf[1] = 6;
		PEG3IoApicItinIBuf[2]= 7;
		PEG3IoApicItinIBuf[3]= 4;
		break;
	
	  case 2:						  
		PEG3IoApicItinIBuf[0] = 6;
		PEG3IoApicItinIBuf[1] = 7;
		PEG3IoApicItinIBuf[2]= 4;
		PEG3IoApicItinIBuf[3]= 5;
		break;
		
	  case 3:						
		PEG3IoApicItinIBuf[0] = 7; 
		PEG3IoApicItinIBuf[1] = 4;
		PEG3IoApicItinIBuf[2]= 5;
		PEG3IoApicItinIBuf[3]= 6;
		break;		
	}
     }
	
		//Update Bus#
	  for(Index2 = 0; Index2 < 4; Index2++) {
		
	  	BackupPtr->SourceBusId = gPlatformPciBridges[Index].SecBus;
		
		if(gPlatformPciBridges[Index].Dev == 2) {///D2F0 PEG	
		  	BackupPtr->DestIoApicItin = PEGIoApicItinIBuf[Index2];
		}

		if((gPlatformPciBridges[Index].Dev == 5) &&((gPlatformPciBridges[Index].Fun == 0))) {///D5F0 PEG2	
			BackupPtr->DestIoApicItin = PEG2IoApicItinIBuf[Index2];
		}
		
		if((gPlatformPciBridges[Index].Dev == 5) &&((gPlatformPciBridges[Index].Fun == 1))) {///D5F1 PEG3	
			BackupPtr->DestIoApicItin = PEG3IoApicItinIBuf[Index2];
		}
		BackupPtr++;
	  }
	  CopyMem(Ptr, StartPtr, Len);
	  Ptr += Len;
	  Count += 4;
  	}
	StartPtr += 4;
  }
//  CopyMem(MpsTableCurrentPointer, gType3List, sizeof(gType3List));
//  return sizeof(gType3List)/sizeof(gType3List[0]);
//DEBUG((EFI_D_ERROR,"Count=%x\n",Count));
//Ptr=MpsTableCurrentPointer;
//StartPtr=(MP_TABLE_INT_ASSIGN_ENTRY *)MpsTableCurrentPointer;
//for(Index=0;Index<Count;Index++) {
//	Ptr = (UINT8*)StartPtr;
//    for(Index2=0;Index2<sizeof(MP_TABLE_INT_ASSIGN_ENTRY);Index2++) {
//		DEBUG((EFI_D_ERROR,"%02x  ",*Ptr));
//		Ptr++;
//    }
//	DEBUG((EFI_D_ERROR,"\n"));
//	StartPtr++;
//}
return Count;
}

//YKN_20160309 +E






EFI_STATUS
MpsTableCreate (
  VOID
  )
{
  EFI_STATUS            Status;
  UINT16                EntryCount;
  EFI_PHYSICAL_ADDRESS  Address;


  Address = 0xffffffff;
  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiBootServicesData,
                   1,
                   &Address
                   );
  ASSERT_EFI_ERROR(Status);

  MpsTablePointer          = (UINT8*)(UINTN)Address;
  MpsTableCurrentPointer   = MpsTablePointer;
  MpsExTablePointer        = MpsTablePointer + 2048;
  MpsExTableCurrentPointer = MpsExTablePointer;

  MpsTableCurrentPointer += sizeof(MP_CONFIGURATION_TABLE_HEADER);
  BaseTableLength        += sizeof(MP_CONFIGURATION_TABLE_HEADER);

  EntryCount = MpsTableAddCpuEntry();
  MpsTableCurrentPointer += sizeof (MP_TABLE_CPU_ENTRY) * EntryCount;
  BaseTableLength        += sizeof (MP_TABLE_CPU_ENTRY) * EntryCount;
  BaseTableEntryCount     = BaseTableEntryCount + EntryCount;

  EntryCount = MpsTableAddBusEntry();
  MpsTableCurrentPointer += sizeof (MP_TABLE_BUS_ENTRY) * EntryCount;
  BaseTableLength        += sizeof (MP_TABLE_BUS_ENTRY) * EntryCount;
  BaseTableEntryCount     = BaseTableEntryCount + EntryCount;

  EntryCount = MpsTableAddIoApicEntry();
  MpsTableCurrentPointer += sizeof (MP_TABLE_IO_APIC_ENTRY) * EntryCount;
  BaseTableLength        += sizeof (MP_TABLE_IO_APIC_ENTRY) * EntryCount;
  BaseTableEntryCount    = BaseTableEntryCount + EntryCount;

  EntryCount = MpsTableAddIntAssignEntry();
  MpsTableCurrentPointer += sizeof (MP_TABLE_INT_ASSIGN_ENTRY) * EntryCount;
  BaseTableLength        += sizeof (MP_TABLE_INT_ASSIGN_ENTRY) * EntryCount;
  BaseTableEntryCount     = BaseTableEntryCount + EntryCount;

  EntryCount = MpsTableAddLocalIntAssignEntry();
  MpsTableCurrentPointer += sizeof (MP_TABLE_LOCAL_INT_ASSIGN_ENTRY) * EntryCount;
  BaseTableLength        += sizeof (MP_TABLE_LOCAL_INT_ASSIGN_ENTRY) * EntryCount;
  BaseTableEntryCount     = BaseTableEntryCount + EntryCount;

  CopyMem (
    MpsTableCurrentPointer, 
    MpsExTablePointer, 
    ExtendedTableLength);
	
  MpsExTablePointer = MpsTableCurrentPointer;

  Status = MpsTableAddHeader();

  return Status;
}




EFI_STATUS
MpsTableFloatPointerModify (
  VOID
  )
{
  EFI_STATUS                  Status;
  VOID                        *MpsTable;
  EFI_LEGACY_BIOS_PROTOCOL	  *LegacyBios;
  MP_FLOATING_POINTER         MpsTableFloatingPointer;

	
  Status = gBS->LocateProtocol (
		              &gEfiLegacyBiosProtocolGuid, 
		              NULL, 
		              (VOID**)&LegacyBios
		              );
  ASSERT_EFI_ERROR (Status);

  MpsTableFloatingPointer.Signature = MP_FLOATING_POINTER_SIGNATURE;
  MpsTableFloatingPointer.PhysicalAddresPointer = (UINT32)(UINTN)MpsTablePointer;
  MpsTableFloatingPointer.Length = sizeof (MP_FLOATING_POINTER) / 16;
  MpsTableFloatingPointer.VersionNumber = MP_TABLE_VERSION_1_4;
  MpsTableFloatingPointer.CheckSum    = 0;
  MpsTableFloatingPointer.DefaultConfiguration = 0;
  MpsTableFloatingPointer.IMCRflag    = 0;
  MpsTableFloatingPointer.Reserved[0] = 0;
  MpsTableFloatingPointer.Reserved[1] = 0;
  MpsTableFloatingPointer.Reserved[2] = 0;
  MpsTableFloatingPointer.CheckSum = CalculateCheckSum8((UINT8*)&MpsTableFloatingPointer, sizeof(MP_FLOATING_POINTER));

  Status = LegacyBios->GetLegacyRegion(LegacyBios, sizeof(MpsTableFloatingPointer), BIT0, 16, &MpsTable);
  ASSERT_EFI_ERROR (Status);
  Status = LegacyBios->CopyLegacyRegion(LegacyBios, sizeof(MpsTableFloatingPointer), MpsTable, &MpsTableFloatingPointer);
  ASSERT_EFI_ERROR (Status);
  DEBUG((EFI_D_INFO, "MpsTable:%X\n", MpsTable));

  return EFI_SUCCESS;
}


VOID 
MpsTableReadyToBootNotify (
  IN EFI_EVENT Event,
  IN VOID     *Context
  )
{
  EFI_STATUS  Status;

  gBS->CloseEvent (Event);

  DEBUG((EFI_D_ERROR,"\n\n ********* \n Begin to create MPS Table \n"));
  Status = MpsTableCreate();
  if (EFI_ERROR(Status)){
    return;
  }
  Status = MpsTableFloatPointerModify();
  if (EFI_ERROR(Status)){
    return;
  }		
}




EFI_STATUS 
MpsTableBuilderInit (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_EVENT  Event;

  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             MpsTableReadyToBootNotify,
             NULL,
             &Event
             );
  return Status;
}


