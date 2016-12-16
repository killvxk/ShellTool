

#include "AcpiPlatform.h"
#include <IndustryStandard/HighPrecisionEventTimerTable.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/PlatformCommLib.h>
#include <Protocol/FirmwareVolume2.h>
#include <AcpiRam.h>



SETUP_DATA           *gSetupData;
EFI_ACPI_RAM_DATA    *gAcpiRam;
PLATFORM_MEMORY_INFO *gMemInfo;

#pragma pack(1)
typedef struct {
    UINT8                           DescriptorType;      // 0x8A
    UINT16                          ResourceLength;      // 0x002B
    UINT8                           ResourceType;        // 0x00
    UINT8                           Flags;               // 0x0C
    UINT8                           SpecificFlags;       // 0x01
    UINT64                          Granularity;         // 0x0000000000000000
    UINT64                          Minimum;             // 0x0000000038584946
    UINT64                          Maximum;             // 0x0000000038584946
    UINT64                          TranslationOffset;   // 0x0000000000000000
    UINT64                          AddressLength;       // 0x0000000000000000
} AML_RESOURCE_ADDRESS64;
#pragma pack()






STATIC UINT8 gGnvsAmlArray[] = {
  0x5B, 0x80,               // OpRegionOp
  'G', 'N', 'V', 'S',       // Name                         +0
  0x00,                     // RegionSpace(SystemMemory)
  0x0C,                     // DWordPrefix
  0x00, 0x00, 0xFF, 0xFF,   // default base                 +6
  0x0B,                     // WordPrefix
  0x55, 0xAA                // default length               +11
};  

STATIC UINT8 gNpssAmlArray[] = {
  0x08,                     // NameOp
  'N', 'P', 'S', 'S',       // Name
  0x12,                     // PackageOp
};  

STATIC UINT8 gVpssAmlArray[] = {
  0x08,                     // NameOp
  'V', 'P', 'S', 'S',       // Name
  0x12,                     // PackageOp
};  

STATIC UINT8 gS1AmlArray[] = {
  0x08,                     // NameOp
  '_', 'S', '1', '_',       // Name
  0x12,                     // PackageOp
};  

STATIC UINT8 gS3AmlArray[] = {
  0x08,                     // NameOp
  '_', 'S', '3', '_',       // Name
  0x12,                     // PackageOp
};  

 

EFI_STATUS
PlatformUpdateTables (
  IN OUT EFI_ACPI_COMMON_HEADER   *Table
  )
{
  EFI_ACPI_DESCRIPTION_HEADER                 *TableHeader;
  UINT8                                       *EndPtr;
  EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE *ApicPtr;
  EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER *HpetTbl; 
  EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE *pzxFadt; // tiger. 2016-06-27
  UINTN                                       CpuApicIndex;
  UINT8                                       *CurrPtr;	
  UINT8                                       *CurrPtrEnd;	  
  UINT8                                       *DsdtPointer;
  BOOLEAN                                     GnvsFound;
  BOOLEAN                                     NpssFound;
  BOOLEAN                                     VpssFound;
  CPU_APIC_ID_INFO                            *CpuApicIdTable;
  UINTN                                       CpuCount;
  BOOLEAN                                     S1Support;
  BOOLEAN                                     S3Support;  
  BOOLEAN                                     S1Found;
  BOOLEAN                                     S3Found;  
  BOOLEAN                                     Pci64Found;
  AML_RESOURCE_ADDRESS64                      *Pci64Res;
  

  TableHeader = (EFI_ACPI_DESCRIPTION_HEADER*)Table;

  if (Table->Signature != EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE) {
    CopyMem(TableHeader->OemId, PcdGetPtr(PcdAcpiDefaultOemId), sizeof(TableHeader->OemId));
    
    // Skip OEM table ID and creator information for DSDT, SSDT and PSDT tables, since these are
    // created by an ASL compiler and the creator information is useful.
    if (Table->Signature != EFI_ACPI_2_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE &&
        Table->Signature != EFI_ACPI_2_0_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE &&
        Table->Signature != EFI_ACPI_2_0_PERSISTENT_SYSTEM_DESCRIPTION_TABLE_SIGNATURE
        ) {
      TableHeader->OemTableId      = PcdGet64(PcdAcpiDefaultOemTableId);
      TableHeader->OemRevision     = PcdGet32(PcdAcpiDefaultOemRevision);
      TableHeader->CreatorId       = PcdGet32(PcdAcpiDefaultCreatorId);
      TableHeader->CreatorRevision = PcdGet32(PcdAcpiDefaultCreatorId);
    }        
  }
  
  switch (Table->Signature) {
    case EFI_ACPI_2_0_MULTIPLE_SAPIC_DESCRIPTION_TABLE_SIGNATURE:
      GetCpuLocalApicInfo(&CpuApicIdTable, &CpuCount);
      ApicPtr = (EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE*)(((EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER*)Table)+1);
      EndPtr  = (UINT8*)Table + Table->Length;
      CpuApicIndex  = 0;
      while ((UINTN)ApicPtr < (UINTN)EndPtr) {
        if (ApicPtr->Type == EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC) {
          if (CpuApicIndex < CpuCount) {
            ApicPtr->Flags  = CpuApicIdTable[CpuApicIndex].Flags;
            ApicPtr->ApicId = CpuApicIdTable[CpuApicIndex].ApicId;
          } else {
            ApicPtr->Flags  = 0;
            ApicPtr->ApicId = 0xFF;
          }
          ApicPtr->AcpiProcessorId = (UINT8)CpuApicIndex + 1;
					CpuApicIndex++;
        }
        ApicPtr = (EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE*)((UINT8*)ApicPtr + ApicPtr->Length);
      }
      FreePool(CpuApicIdTable);
      break;

    case EFI_ACPI_3_0_HIGH_PRECISION_EVENT_TIMER_TABLE_SIGNATURE:
      HpetTbl = (EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER*)Table;
      HpetTbl->EventTimerBlockId = (HpetTbl->EventTimerBlockId & 0xFFFF)|
                                    (MmioRead16(LPC_PCI_REG(PCI_VID_REG))<<16);
      break;
    /// FADT. TGR-2016062702+S
    case EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE:
	DEBUG((EFI_D_ERROR,"tiger 20160627 : find FADT 5.0 \n"));
	pzxFadt= (EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE*)Table;
	DEBUG((EFI_D_ERROR,"Original IaPcBootArch = %x \n",pzxFadt->IaPcBootArch));
	if(gSetupData->MsiSupport)
	{
	    DEBUG((EFI_D_ERROR,"Enable MSI support \n"));
	}
	else
	{
	    DEBUG((EFI_D_ERROR,"Disable MSI support \n"));
   	    pzxFadt->IaPcBootArch = (pzxFadt->IaPcBootArch) | 0x08;
	}
	DEBUG((EFI_D_ERROR,"New       IaPcBootArch = %x \n",pzxFadt->IaPcBootArch));
	break;	
    /// TGR-2016062702+E
    case EFI_ACPI_2_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:  

      GnvsFound   = FALSE;
      NpssFound   = FALSE;
      VpssFound   = FALSE;
      S1Found     = FALSE;
      S3Found     = FALSE;
      Pci64Found  = FALSE;

      switch(gSetupData->AcpiSleepStatus){     
        case 0:                         // Not support
          S1Support = 0;
          S3Support = 0;
          break;
          
        case 1:                         // S1
          S1Support = 1;
          S3Support = 0; 
          break;

        case 3:                         //Both S1 and S3
          S1Support = 1;
          S3Support = 1;
          break;
          
        case 2:                         // S3
        default:
          S1Support = 0;
          S3Support = 1; 
          break;      
      }
      
      CurrPtr    = (UINT8*)Table;
      CurrPtrEnd = CurrPtr + ((EFI_ACPI_COMMON_HEADER *)CurrPtr)->Length;
      for (DsdtPointer = CurrPtr; DsdtPointer <= CurrPtrEnd;DsdtPointer++) {
        switch(*(UINT32*)DsdtPointer) {
        
          case (SIGNATURE_32 ('G', 'N', 'V', 'S')):
            if(!GnvsFound && CompareMem(DsdtPointer-2, gGnvsAmlArray, sizeof(gGnvsAmlArray))==0){
              *(UINT32*)(DsdtPointer + 6) = (UINT32)(UINTN)gAcpiRam;
              *(UINT16*)(DsdtPointer +11) = sizeof(EFI_ACPI_RAM_DATA);
              DEBUG((EFI_D_ERROR, "GNVS Found! -> (%X,%X)\n", (UINT32)(UINTN)gAcpiRam, sizeof(EFI_ACPI_RAM_DATA)));
              GnvsFound = TRUE;
            }
            break;
            
          case (SIGNATURE_32 ('N', 'P', 'S', 'S')):
            if(!NpssFound && CompareMem(DsdtPointer-1, gNpssAmlArray, sizeof(gNpssAmlArray))==0){
              DEBUG((EFI_D_ERROR, "NPSS Found!\n"));
              UpdateNpssPackage(DsdtPointer);
              NpssFound = TRUE;
            }
            break;

          case (SIGNATURE_32 ('V', 'P', 'S', 'S')):
            if(!VpssFound && CompareMem(DsdtPointer-1, gVpssAmlArray, sizeof(gVpssAmlArray))==0){
              DEBUG((EFI_D_ERROR, "VPSS Found!\n"));
              UpdateVpssPackage(DsdtPointer);
              VpssFound = TRUE;
            }
            break;            

          case (SIGNATURE_32 ('_', 'S', '1', '_')):
            if(!S1Found && CompareMem(DsdtPointer-1, gS1AmlArray, sizeof(gS1AmlArray))==0){
              if(!S1Support){
                *DsdtPointer = 'X';
              }  
              S1Found = TRUE;
            }  
            break;

          case (SIGNATURE_32 ('_', 'S', '3', '_')):
            if(!S3Found && CompareMem(DsdtPointer-1, gS3AmlArray, sizeof(gS3AmlArray))==0){
              if(!S3Support){
                *DsdtPointer = 'X';
              }  
              S3Found = TRUE;
            }  
            break;


          case (SIGNATURE_32('F', 'I', 'X', '0')):
            Pci64Res = BASE_CR(DsdtPointer, AML_RESOURCE_ADDRESS64, Minimum);
            if(!Pci64Found &&
              Pci64Res->DescriptorType == AML_CREATE_DWORD_FIELD_OP &&
              (UINT32)Pci64Res->Maximum == SIGNATURE_32('F', 'I', 'X', '0')){
              if(PcdGet32(PcdSystemMiscConfig) & SYS_MISC_CFG_PCI64){
                Pci64Res->Granularity   = 0;
                Pci64Res->Minimum       = gMemInfo->Pci64Base;
                Pci64Res->Maximum       = gMemInfo->Pci64Base + gMemInfo->Pci64Size - 1;
                Pci64Res->AddressLength = gMemInfo->Pci64Size;
              } else {
                Pci64Res->Granularity   = 0;
                Pci64Res->Minimum       = 0;
                Pci64Res->Maximum       = 0;
                Pci64Res->AddressLength = 0;
              }
              Pci64Found = TRUE;
            }  
            break;

            
          default:
            break;
        }                   // switch
        
        if(GnvsFound && NpssFound && VpssFound && S1Found && S3Found && Pci64Found){
          break;
        }
      }                     // for
      break;
      
    default:
      break;
  }

  return EFI_SUCCESS;
}






EFI_STATUS
EFIAPI
AcpiPlatformInit (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_ACPI_TABLE_PROTOCOL       *AcpiTable;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv2;
  INTN                          Instance;
  EFI_ACPI_COMMON_HEADER        *CurrentTable;
  UINTN                         TableKey;
  UINT32                        FvStatus;
  UINTN                         Size;
  UINTN                         FvHandleCount;
  EFI_HANDLE                    *FvHandleBuffer = NULL;  
	CHAR8                         *Sign;
  UINTN                         Index;

  
	
  DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));

  gSetupData = (SETUP_DATA*)GetSetupDataHobData();
  gAcpiRam   = (EFI_ACPI_RAM_DATA*)GetAcpiRam();
  gMemInfo   = (PLATFORM_MEMORY_INFO*)GetPlatformMemInfo(); 

  Status = gBS->LocateProtocol(&gEfiAcpiTableProtocolGuid, NULL, &AcpiTable);
  ASSERT_EFI_ERROR (Status);
	
  BuildAcpiSpcrTable(AcpiTable);
  
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &FvHandleCount,
                  &FvHandleBuffer
                  );
  ASSERT(!EFI_ERROR(Status));
  if(EFI_ERROR(Status) || FvHandleCount==0){
    goto ProcExit;
  }
  
  for (Index = 0; Index < FvHandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    FvHandleBuffer[Index],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID**)&Fv2
                    );
    ASSERT(!EFI_ERROR(Status));

    Instance = 0;
    while(1){
      CurrentTable = NULL;
      Size = 0;
      Status = Fv2->ReadSection (
                      Fv2,
                      (EFI_GUID*)PcdGetPtr(PcdAcpiTableStorageFile),
                      EFI_SECTION_RAW,
                      Instance++,
                      (VOID**)&CurrentTable,
                      &Size,
                      &FvStatus
                      );
      if(EFI_ERROR(Status)){
        break;
      }

      Sign = (CHAR8*)(&CurrentTable->Signature);
      DEBUG((EFI_D_ERROR, "%c%c%c%c\n", Sign[0], Sign[1], Sign[2], Sign[3]));
      ASSERT(Size >= CurrentTable->Length);
      
      PlatformUpdateTables(CurrentTable);
      AcpiTableUpdateChksum(CurrentTable);
      Status = AcpiTable->InstallAcpiTable (
                            AcpiTable,
                            CurrentTable,
                            CurrentTable->Length,
                            &TableKey
                            );
      ASSERT_EFI_ERROR(Status);
      gBS->FreePool(CurrentTable);
      CurrentTable = NULL;      
    }  
  }
  
  if (FvHandleBuffer != NULL) {
    gBS->FreePool(FvHandleBuffer);
  }

ProcExit:         
  return EFI_SUCCESS;
}





