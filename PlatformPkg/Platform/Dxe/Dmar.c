
#include "PlatformDxe.h"
#include <IndustryStandard/Acpi.h>
#include <Protocol/AcpiTable.h>


#define EFI_ACPI_DMA_REMAPPING_TABLE_SIGNATURE  SIGNATURE_32('D', 'M', 'A', 'R')
#define EFI_ACPI_DMA_REMAPPING_TABLE_REVISION   1

#define DMAR_FLAG_INTR_REMAP          BIT0
#define DMAR_FLAG_X2APIC_OPT_OUT      BIT1

#define DMAR_DRHD_FLAG_INC_PCI_ALL    BIT0

#define DMAR_DEV_SCOPE_TYPE_IOAPIC    3

#define  REMAPPING_TYPE_DRHD		0
#define  REMAPPING_TYPE_RMRR		1
#define  REMAPPING_TYPE_ATSR		2
#define  REMAPPING_TYPE_RHSA		3

#pragma pack(1)

typedef struct {
  UINT8     Type;
  UINT8     Length;
  UINT16    Reserved;
  UINT8     EnumID;
  UINT8     StartBusNo;
  UINT8     Path[2];
} DMAR_DEVICE_SCOPE;

typedef struct {
  UINT16              Type;
  UINT16              Length;
  UINT8               Flags;
  UINT8               Reserved;
  UINT16              SegmentNo;
  UINT64              BaseAddr;
  DMAR_DEVICE_SCOPE   DevScope[2];
} DMAR_DRHD_TABLE;

typedef struct {
	EFI_ACPI_DESCRIPTION_HEADER 	Header;
	UINT8                         HostAddrWidth;        // +36
  UINT8                         Flags;                // +37
  UINT8                         Reserved[10];         // +38
  DMAR_DRHD_TABLE               DrhdTbl;
}ACPI_TABLE_DMAR;

#pragma pack()


STATIC ACPI_TABLE_DMAR gDmarTable = {
  {
    EFI_ACPI_DMA_REMAPPING_TABLE_SIGNATURE,
    sizeof(gDmarTable),
    EFI_ACPI_DMA_REMAPPING_TABLE_REVISION,
    0,
    {0,},
    0,
    0,
    0,
    0,
  },
  0x27,
  DMAR_FLAG_INTR_REMAP,
  {0,},
  {
    REMAPPING_TYPE_DRHD,
    sizeof(DMAR_DRHD_TABLE),
    DMAR_DRHD_FLAG_INC_PCI_ALL,
    0,
    0,
    0,
    {
      {
        DMAR_DEV_SCOPE_TYPE_IOAPIC,
        sizeof(DMAR_DEVICE_SCOPE),
        0,
        0,
        0,
        {0,0},
      },
      {
        DMAR_DEV_SCOPE_TYPE_IOAPIC,
        sizeof(DMAR_DEVICE_SCOPE),
        0,
        0,
        0,
        {0,0},        
      },      
    }
  },
};  


EFI_STATUS InstallAcpiTableDmar()
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;
  EFI_STATUS               Status;
  ACPI_TABLE_DMAR          *Dmar;
  UINT64                   OemTableId;
  UINTN                    TableKey;
  
  
  if(!gSetupData->IOVEnable){
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }  

  Status = gBS->LocateProtocol(&gEfiAcpiTableProtocolGuid, NULL, (VOID**)&AcpiTable);
  if (EFI_ERROR (Status)) {
    goto ProcExit;
  }  

  Dmar = &gDmarTable;
  CopyMem(Dmar->Header.OemId, PcdGetPtr(PcdAcpiDefaultOemId), sizeof(Dmar->Header.OemId));
  OemTableId = PcdGet64(PcdAcpiDefaultOemTableId);
  CopyMem(&Dmar->Header.OemTableId, &OemTableId, sizeof(UINT64));
  Dmar->Header.OemRevision     = PcdGet32(PcdAcpiDefaultOemRevision);
  Dmar->Header.CreatorId       = PcdGet32(PcdAcpiDefaultCreatorId);
  Dmar->Header.CreatorRevision = PcdGet32(PcdAcpiDefaultCreatorRevision);

  Dmar->DrhdTbl.BaseAddr  = gAsiaNbCfg->RcrbvBar;
  Dmar->DrhdTbl.DevScope[0].EnumID = gAsiaSbCfg->SbApicID;
  Dmar->DrhdTbl.DevScope[0].Path[0] = 0x11;                 // dev
  Dmar->DrhdTbl.DevScope[0].Path[1] = 0x00;                 // func
  Dmar->DrhdTbl.DevScope[1].EnumID = gAsiaNbCfg->NbApicID;
  Dmar->DrhdTbl.DevScope[1].Path[0] = 0x00;                 // dev
  Dmar->DrhdTbl.DevScope[1].Path[1] = 0x05;                 // func

  AcpiTableUpdateChksum(Dmar);
 
  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        Dmar,
                        Dmar->Header.Length,
                        &TableKey
                        ); 
  
ProcExit:
  return Status;  
}




