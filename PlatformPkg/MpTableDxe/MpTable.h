/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  MpTable.h

Abstract: 
  Header file for MP table builder.

Revision History:

**/

#ifndef __MPTABLE__H__
#define __MPTABLE__H__

#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/MpService.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/LegacyRegion2.h>
#include <Protocol/Cpu.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PlatformCommLib.h>


#define PCI_CFG_ADDR(bus,dev,func,reg) \
  ((VOID*)(UINTN) (PCIEX_BASE_ADDRESS + ((bus) << 20) + ((dev) << 15) + ((func) << 12) + reg))



//
// Type declarations
//

#define MSR_EXT_XAPIC_LOGICAL_APIC_ID   0x802
#define MSR_EXT_XAPIC_VERSION           0x803
#define MSR_XAPIC_BASE                  0x1B
#define APIC_VERSION_REGISTER           0x30

#define MAX_CPU_NUMBER                  4

#define MP_TABLE_MAX_BUS_ENTRIES        0xFF // From 0 to 0xFE, FF reserved means no parent bus
#define MP_TABLE_MAX_IO_APIC_ENTRIES    0x10

#define MP_FLOATING_POINTER_SIGNATURE    0x5F504D5F  // _MP_
#define MP_CONFIGURATION_TABLE_SIGNATURE 0x504D4350  // PCMP

#define MP_TABLE_VERSION_1_1    1
#define MP_TABLE_VERSION_1_4    4

#define MP_TABLE_CPU_ENTRY_TYPE                  0
#define MP_TABLE_BUS_ENTRY_TYPE                  1
#define MP_TABLE_IO_APIC_ENTRY_TYPE              2
#define MP_TABLE_INT_ASSIGN_ENTRY_TYPE           3
#define MP_TABLE_LOCAL_INT_ASSIGN_ENTRY_TYPE     4

#define MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY_TYPE              128
#define MP_EX_TABLE_BUS_HIERARCHY_ENTRY_TYPE                129
#define MP_EX_TABLE_COMPAT_BUS_ADDRESS_MODIFIER_ENTRY_TYPE  130

#define LOCAL_APIC_BASE             0xFEE00000
#define IO_APIC_BASE_ADDRESS_BOTTOM 0xFEC00000
#define IO_APIC_BASE_ADDRESS_TOP    0xFED00000
#define IO_APIC_SEARCH_STEP         0x1000

#define IO_APIC_INDEX_REG           0
#define IO_APIC_DATA_REG            4

#define IO_APIC_MAP_ID_REG          0
#define IO_APIC_MAP_VERSION_REG     1

#define INT_TYPE_INT                0
#define INT_TYPE_NMI                1
#define INT_TYPE_SMI                2
#define INT_TYPE_EXT_INT            3

#define POLARITY_CONFORM_SPEC       0
#define POLARITY_ACTIVE_HIGH        1
#define POLARITY_ACTIVE_LOW         3

#define TRIGGER_MODE_CONFORM_SPEC    0
#define TRIGGER_MODE_EDGE_TRIGGERED  1
#define TRIGGER_MODE_LEVEL_TRIGGERED 3

#define BUS_TYPE_ISA                5

#define HEADER_TYPE_PCI_TO_PCI_BRIDGE 0x01
#define HEADER_LAYOUT_CODE            0x7f
#define PCI_HEADER_TYPE_OFFSET                      0x0E
#define PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET    0x19


#if defined(_MSC_EXTENSIONS)
#define GLOBAL_REMOVE_IF_UNREFERENCED __declspec(selectany)
#else
#define GLOBAL_REMOVE_IF_UNREFERENCED
#endif


#pragma pack(1)

typedef struct _MP_FOATING_POINTER {
  UINT32  Signature;
  UINT32  PhysicalAddresPointer;
  UINT8   Length;
  UINT8   VersionNumber;
  UINT8   CheckSum;
  UINT8   DefaultConfiguration;
  UINT8   IMCRflag;
  UINT8   Reserved[3];
} MP_FLOATING_POINTER;

typedef struct _MP_CONFIGURATION_TABLE_HEADER {
  UINT32  Signature;
  UINT16  BaseTableLength;
  UINT8   VersionNumber;
  UINT8   CheckSum;
  UINT8   OemId[8];
  UINT8   ProductId[12];
  UINT32  OemTablePointer;
  UINT16  OemTableSize;
  UINT16  BaseTableEntryCount;
  UINT32  LocalApicAddress;
  UINT16  ExtendedTableLength;
  UINT8   ExtendedCheckSum;
  UINT8   Reserved;
} MP_CONFIGURATION_TABLE_HEADER;

typedef struct _MP_TABLE_CPU_ENTRY {
  UINT8   EntryType;
  UINT8   LocalApicId;
  UINT8   LocalApicVersion;
  struct {
    UINT8   CpuEnable    : 1;
    UINT8   CpuBootStrap : 1;
    UINT8   Reserved     : 6;
  } CpuFlags;
  UINT32  CpuSignature;
  UINT32  FeatureFlags;
  UINT32  Reserved[2];
} MP_TABLE_CPU_ENTRY;

typedef struct _MP_TABLE_BUS_ENTRY {
  UINT8   EntryType;
  UINT8   BusId;
  UINT8   BusTypeString[6];
} MP_TABLE_BUS_ENTRY;

typedef struct _MP_TABLE_IO_APIC_ENTRY {
  UINT8   EntryType;
  UINT8   IoApicId;
  UINT8   IoApicVersion;
  UINT8   Flags;

  UINT32  IoApicAddress;
} MP_TABLE_IO_APIC_ENTRY;

typedef union {
  UINT8   IsaBusIrq;
  struct {
    UINT8   PciIntSignal    : 2;
    UINT8   PciDeviceNumber : 5;
    UINT8   Reserved        : 1;
  } PciBusIrq;
} SOURCE_IRQ;

typedef struct _MP_TABLE_INT_ASSIGN_ENTRY {
  UINT8   EntryType;
  UINT8   InterruptType;
  UINT16  Flags;
  UINT8   SourceBusId;
  SOURCE_IRQ   SourceBusIrq;
  UINT8   DestIoApicId;
  UINT8   DestIoApicItin;
} MP_TABLE_INT_ASSIGN_ENTRY;

typedef struct _MP_TABLE_LOCAL_INT_ASSIGN_ENTRY {
  UINT8   EntryType;
  UINT8   InterruptType;
  struct {
    UINT16  Polarity    : 2;
    UINT16  TriggerMode : 2;
    UINT16  Reserved    : 12;
  } InterruptSignal;
  UINT8   SourceBusId;
  UINT8   SourceBusIrq;
  UINT8   DestLocalApicId;
  UINT8   DestLocalApicItin;
} MP_TABLE_LOCAL_INT_ASSIGN_ENTRY;

typedef struct _MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY {
  UINT8   EntryType;
  UINT8   EntryLength;
  UINT8   BusId;
  UINT8   AddressType;
  UINT64  AddressBase;
  UINT64  AddressLength;
} MP_EX_TABLE_SYS_ADDRESS_MAP_ENTRY;

typedef struct _MP_EX_TABLE_BUS_HIERARCHY_ENTRY {
  UINT8   EntryType;
  UINT8   EntryLength;
  UINT8   BusId;
  UINT8   BusInfo;
  UINT8   ParentBusId;
  UINT8   Reserved[3];
} MP_EX_TABLE_BUS_HIERARCHY_ENTRY;

typedef struct _MP_EX_TABLE_COMPAT_BUS_ADDRESS_MODIFIER_ENTRY {
  UINT8   EntryType;
  UINT8   EntryLength;
  UINT8   BusId;
  UINT8   AddressModifier;
  UINT32  RangeList;
} MP_EX_TABLE_COMPAT_BUS_ADDRESS_MODIFIER_ENTRY;

//
// Additional structures
//
typedef struct _BUS_INFO {
  UINT8   BusId;
  UINT8   ParentBusId;
  UINT8   BusType;
  UINT8   Flags;
} BUS_INFO;

typedef struct _ACPI_ADDRESS_SPACE_DESC {
  UINT8   Signature;
  UINT16  DescLength;
  UINT8   ResourceType;
  UINT8   GeneralFlags;
  UINT8   TypeSpecificFlags;
  UINT64  AddressSpaceGranularity;
  UINT64  AddressRangeMin;
  UINT64  AddressRangeMax;
  UINT64  AddressTranslationOffset;
  UINT64  AddressLength;
} ACPI_ADDRESS_SPACE_DESC;


typedef struct _IO_APIC_INFO {
  UINT8   IoApicId;
  UINT8   IoApicVersion;
  UINT8   IoApicFlags;
  UINT8   MaxRedirectionEntries;
  UINT32  IoApicAddress;
} IO_APIC_INFO;

typedef struct _ISO_PARAMETER_TABLE {
  UINT8   PicIrq;
  UINT8   Flags; 
  UINT16  ApicInt;
} ISO_PARAMETER_TABLE;

typedef struct _CPU_INFO {
  UINT32      StatusFlag;
  UINT32      Signature;
  UINT32      FeatureFlags;
  UINT64      ApicId;
  UINT8       ApicVersion;
} CPU_INFO;



#pragma pack()

//
// Functions prototypes
//

EFI_STATUS
MpsTableBuilderInit (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  );

EFI_STATUS
MpsTableFloatPointerModify (
  VOID
  );

EFI_STATUS
MpsTableCreate (
  VOID
  );

VOID 
MpsTableReadyToBootNotify (
  IN EFI_EVENT Event,
  IN VOID *Context
  );

EFI_STATUS
MpsTableAddHeader (
  VOID
  );

UINT16 
MpsTableAddCpuEntry (
  VOID
  );

UINT16
MpsTableAddBusEntry (
  VOID
  );

UINT16
MpsTableAddIoApicEntry (
  VOID
  );

UINT16
MpsTableAddIntAssignEntry (
  VOID
  );

UINT16
MpsTableAddLocalIntAssignEntry (
  VOID
  );

VOID
MpsTableAddExtendedTable (
  VOID
  );

VOID
MpsExTableAddSysAddressMapEntry (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *pPciRootBridgeIoProtocol,
  IN UINT8 BusId,
  IN UINT8 BusAttributes
  );

VOID 
MpsExTableAddBusHierarchyEntry (
  VOID
  );

VOID 
MpsExTableAddCompatBusAddressModifierEntry (
  VOID
  );


//----------------------- Additional functions -------------------------

VOID
QueryBusInfo (
  VOID
  );

VOID
CollectIoApicInfo (
  IN EFI_PCI_IO_PROTOCOL *pPciIoProtocol OPTIONAL,
  IN UINT32 BaseAddress OPTIONAL
  );

VOID 
ArrangeIoApicEntries (
  VOID
  );

VOID 
GetIoApicId (
  IN UINT16   SysVectorNumber,
  OUT UINT8 *IoApicId,
  OUT UINT8 *IoApicItin
  );

EFI_STATUS 
CollectCpuInfo (
  IN OUT UINTN     *NumberOfCpus,
  IN OUT UINTN     *NumberOfThreadsPerCore
  );

  
  
  
  

//
// Customized section - begin
//
#define IRQ_00_OVERRIDE_ENABLE  1
#define IRQ_00_APIC_INT         2
#define IRQ_00_POLARITY         0
#define IRQ_00_TRIGGER_MODE     0
#define IRQ_01_OVERRIDE_ENABLE  0
#define IRQ_03_OVERRIDE_ENABLE  0
#define IRQ_04_OVERRIDE_ENABLE  0
#define IRQ_05_OVERRIDE_ENABLE  0
#define IRQ_06_OVERRIDE_ENABLE  0
#define IRQ_07_OVERRIDE_ENABLE  0
#define IRQ_08_OVERRIDE_ENABLE  0
#define IRQ_09_OVERRIDE_ENABLE  1
#define IRQ_09_APIC_INT         9
#define IRQ_09_POLARITY         1
#define IRQ_09_TRIGGER_MODE     3
#define IRQ_10_OVERRIDE_ENABLE  0
#define IRQ_11_OVERRIDE_ENABLE  0
#define IRQ_12_OVERRIDE_ENABLE  0
#define IRQ_13_OVERRIDE_ENABLE  0
#define IRQ_14_OVERRIDE_ENABLE  0
#define IRQ_15_OVERRIDE_ENABLE  0

#define T_ACPI_OEM_ID INTEL
#define T_ACPI_OEM_TBL_ID SNB-CPT

//
// Platform Base Address definitions
//
#define PCIEX_BASE_ADDRESS          EDKII_GLUE_PciExpressBaseAddress  // Pci Express Configuration Space Base Address  
 
  
  
#endif
