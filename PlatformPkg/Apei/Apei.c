

#include <Uefi.h>
#include <IndustryStandard/Acpi.h>
#include <Library/IoLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/SmmSwDispatch2.h>
#include "Apei.h"



/*
  ACPI Platform Error Interfaces (APEI)
    Error Record Serialization Table (ERST)           <--- 
      Common Platform Error Record (CPER) 
    Boot Error Record Table (BERT)
    Hardware Error Source Table (HEST)
    Error Injection Table (EINJ)
*/


#define MAX_ERROR_LOG_RANGE_SIZE    0x2000





 


#pragma pack(1)

typedef struct {
  UINT64      Command;
  UINT64      RecordID;
  UINT64      NextRecordID;
  UINT32      LogOffset;
  UINT32      Padding0;
  UINT64      Status;
  UINT64      BusyFlag;
  UINT64      RecordCount;
  UINT64      ErrorLogAddressRange;
  UINT64      ErrorLogAddressLength;
  UINT64      ErrorLogAddressAttributes;
  UINT64      Dummy;
  UINT64	    CurRecordID;
} ERST_PARAMETER_REGION;

#pragma pack()

typedef struct {
  ERST_PARAMETER_REGION  *ErstParam;
} APEI_CTX;  

extern EFI_ACPI_5_0_ERST_SERIALIZATION_INSTRUCTION_ENTRY gSerialInsEntry[];
extern UINTN gSerialInsEntrySize;

APEI_CTX gApeiCtx;

  
VOID
ErstRead (
  EFI_PHYSICAL_ADDRESS  RecordBuffer,
  UINT64                RecordID,
  UINT64                *Status
  )
{

}

VOID
ErstWrite (
  EFI_PHYSICAL_ADDRESS  NewRecord,
  UINT64                *Status
  )
{

}

VOID
ErstClear (
  UINT64                RecordID,
  UINT64                *Status
  )
{

}


EFI_STATUS
ERSTLogSwSmiCallback (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  ERST_PARAMETER_REGION  *ErstParam;

  ErstParam = gApeiCtx.ErstParam;
  ErstParam->BusyFlag = 1;

  switch (ErstParam->Command) {
    case ERST_ERROR_SET_RECORD:
      ErstWrite(ErstParam->ErrorLogAddressRange + ErstParam->LogOffset,
                &ErstParam->Status);
      break;
    case ERST_ERROR_GET_RECORD:
      ErstRead(ErstParam->ErrorLogAddressRange + ErstParam->LogOffset,
               ErstParam->RecordID,
               &ErstParam->Status);
      break;
    case ERST_ERROR_CLEAR_RECORD:
      ErstClear(ErstParam->RecordID, &ErstParam->Status);
      break;

    default: // Unknown Serialization Command 
      break;
  }

  ErstParam->BusyFlag = 0;

  return EFI_SUCCESS;
}




EFI_STATUS
EFIAPI
ApeiSmmEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_ACPI_TABLE_PROTOCOL   *AcpiTable;  
  UINTN                     TableKey;
  UINTN                     TableSize;
  EFI_ACPI_5_0_ERROR_RECORD_SERIALIZATION_TABLE_HEADER  *Erst;
  UINT8                     *LogRange;
  UINTN                     LogRangeSize;
  ERST_PARAMETER_REGION     *ParamBlock;
  UINT8                     *RecBlock;
  UINTN                     RecBlockSize;
  EFI_SMM_SW_DISPATCH2_PROTOCOL   *SwDispatch;
  EFI_SMM_SW_REGISTER_CONTEXT     SwContext;
  EFI_HANDLE                      SwHandle;

  
  TableSize = sizeof(EFI_ACPI_5_0_ERROR_RECORD_SERIALIZATION_TABLE_HEADER) +
              sizeof(EFI_ACPI_5_0_ERST_SERIALIZATION_INSTRUCTION_ENTRY) * 16;
              
  Erst = (EFI_ACPI_5_0_ERROR_RECORD_SERIALIZATION_TABLE_HEADER*)AllocateZeroPool(TableSize);
  ASSERT(Erst != NULL);
  
  LogRangeSize = MAX_ERROR_LOG_RANGE_SIZE;
  Status = gBS->AllocatePool (
                  EfiReservedMemoryType, 
                  LogRangeSize,
                  &LogRange
                  );
  ASSERT_EFI_ERROR(Status);                  
  ZeroMem(LogRange, LogRangeSize);                  
  
  Erst->Header.Signature = EFI_ACPI_5_0_ERROR_RECORD_SERIALIZATION_TABLE_SIGNATURE;
  CopyMem(Erst->Header.OemId, PcdGetPtr(PcdAcpiDefaultOemId), sizeof(Erst->Header.OemId));
  Erst->Header.OemTableId      = PcdGet64(PcdAcpiDefaultOemTableId);
  Erst->Header.OemRevision     = PcdGet32(PcdAcpiDefaultOemRevision);;
  Erst->Header.CreatorId       = PcdGet32(PcdAcpiDefaultCreatorId);
  Erst->Header.CreatorRevision = PcdGet32(PcdAcpiDefaultCreatorId);  
  Erst->Header.Revision        = 1;
  Erst->Header.Length          = (UINT32)TableSize;  
  
  Erst->SerializationHeaderSize = sizeof(EFI_ACPI_5_0_ERROR_RECORD_SERIALIZATION_TABLE_HEADER);    
//Erst->InstructionEntryCount   = 0;
  
  ParamBlock   = (ERST_PARAMETER_REGION*)LogRange;
  RecBlock     = LogRange + sizeof(ERST_PARAMETER_REGION);
  RecBlockSize = LogRangeSize - sizeof(ERST_PARAMETER_REGION);

  gApeiCtx.ErstParam = ParamBlock;

  ParamBlock->ErrorLogAddressRange      = (UINTN)RecBlock;
  ParamBlock->ErrorLogAddressLength     = RecBlockSize;
  ParamBlock->ErrorLogAddressAttributes = 0;
  ParamBlock->CurRecordID               = (UINTN)-1;
  ParamBlock->RecordID                  = 0;
  ParamBlock->RecordCount               = 0;

  gSerialInsEntry[0].RegisterRegion.Address   = (UINTN)&ParamBlock->Command;	      // BEGIN_WRITE_OPERATION
  gSerialInsEntry[1].RegisterRegion.Address   = (UINTN)&ParamBlock->Command;	      // BEGIN_READ_OPERATION
  gSerialInsEntry[2].RegisterRegion.Address   = (UINTN)&ParamBlock->Command;	      // BEGIN_CLEAR_OPERATION
  gSerialInsEntry[3].RegisterRegion.Address   = (UINTN)&ParamBlock->Command;	      // END_OPERATION
  gSerialInsEntry[4].RegisterRegion.Address   = (UINTN)&ParamBlock->LogOffset;	    // SET_RECORD_OFFSET
  gSerialInsEntry[5].RegisterRegion.Address   = PcdGet16(PcdSwSmiCmdPort); 
  gSerialInsEntry[6].RegisterRegion.Address   = (UINTN)&ParamBlock->BusyFlag;	      // CHECK_BUSY_STATUS
  gSerialInsEntry[7].RegisterRegion.Address   = (UINTN)&ParamBlock->Status;	        // GET_OPERATION_STATUS
  gSerialInsEntry[8].RegisterRegion.Address   = (UINTN)&ParamBlock->CurRecordID;	  // GET_RECORD_IDENTIFIER
  gSerialInsEntry[9].RegisterRegion.Address   = (UINTN)&ParamBlock->RecordID;	      // SET_RECORD_IDENTIFIER
  gSerialInsEntry[10].RegisterRegion.Address  = (UINTN)&ParamBlock->RecordCount;    // GET_RECORD_COUNT
  gSerialInsEntry[11].RegisterRegion.Address  = (UINTN)&ParamBlock->Command;	      // BEGIN_DUMMY_WRITE_OPERATION
  gSerialInsEntry[12].RegisterRegion.Address  = (UINTN)&ParamBlock->NextRecordID;	  // GET_NEXT_RECORDID / RESERVED
  gSerialInsEntry[13].RegisterRegion.Address  = (UINTN)&ParamBlock->ErrorLogAddressRange;	    // GET_ERROR_LOG_ADDRESS_RANGE
  gSerialInsEntry[14].RegisterRegion.Address  = (UINTN)&ParamBlock->ErrorLogAddressLength;	// GET_ERROR_LOG_ADDRESS_RANGE_LENGTH
  gSerialInsEntry[15].RegisterRegion.Address  = (UINTN)&ParamBlock->ErrorLogAddressAttributes;// GET_ERROR_LOG_ADDRESS_RANGE_ATTRIBUTES
  
  CopyMem((UINT8*)Erst + Erst->SerializationHeaderSize, gSerialInsEntry, gSerialInsEntrySize);


  Status = gSmst->SmmLocateProtocol(&gEfiSmmSwDispatch2ProtocolGuid, NULL, &SwDispatch);
  ASSERT_EFI_ERROR (Status);

  SwHandle = NULL;
  SwContext.SwSmiInputValue = ERST_ERROR_RECORD_SWSMI;
  Status = SwDispatch->Register (
                         SwDispatch,
                         ERSTLogSwSmiCallback,
                         &SwContext,
                         &SwHandle
                         );
  ASSERT_EFI_ERROR (Status);

  
  Status = gBS->LocateProtocol(&gEfiAcpiTableProtocolGuid, NULL, &AcpiTable);
  ASSERT_EFI_ERROR(Status);

  Status = AcpiTable->InstallAcpiTable(      // Protocol will checksum it.
                        AcpiTable,
                        Erst,
                        TableSize,
                        &TableKey
                        );  
  ASSERT_EFI_ERROR(Status);

  FreePool(Erst);
  
  return Status;
}



