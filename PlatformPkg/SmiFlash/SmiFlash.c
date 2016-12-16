/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SmiFlash.c

Abstract:
  Provides Access to flash backup Services through SMI

Revision History:

**/

#include "SmiFlash.h"
#include <Framework/SmmCis.h>
#include <Protocol/SmmBaseHelperReady.h>

//---------------------------------------------------------------------------
NV_MEDIA_ACCESS_PROTOCOL              *mMediaAccess;
EFI_SMM_CPU_PROTOCOL                  *mSmmCpu;

UINT32                                mFDBaseAddress;
UINT32                                mFDSize;
UINT8                                 *BlockBuffer;
EFI_SMM_SYSTEM_TABLE                  *mFrameworkSmst;
EFI_SMM_BASE_HELPER_READY_PROTOCOL    *mSmmBaseHelperReady;

SMBIOS_TABLE_ENTRY_POINT        *mSmbiosLegacyEntry = NULL;
SMBIOS_TABLE_ENTRY_POINT        *mSmbiosEfiEntry    = NULL;
UINT8                           *mSmbiosBufferPtr   = NULL;
UINT8                           *mSmbiosScratchPtr  = NULL;
LIST_ENTRY                      mPnp52HeadNode;

EFI_STATUS
InsertRecord(
  PNP_52_DATA_BUFFER *pDataBufferPtr
)
{
  EFI_STATUS          Status;
  UINT16              RecordLen;
  PNP_52_RECORD       *Record;
  PNP_52_DATA_BUFFER  *pData;
  UINTN               Length;
  LIST_ENTRY          *Link;

  if (pDataBufferPtr == NULL)
    return EFI_INVALID_PARAMETER;

  if (pDataBufferPtr->Command == DeleteChanged) {
    Link = GetFirstNode (&mPnp52HeadNode);
    while(!IsNull (&mPnp52HeadNode, Link)) {
      Record = CR (Link, PNP_52_RECORD, Link, EFI_PNP_52_SIGNATURE);
      if ((Record->pRecord->Command == AddChanged) &&
          (Record->pRecord->StructureHeader.Type == pDataBufferPtr->StructureHeader.Type) &&
          (Record->pRecord->StructureHeader.Handle == pDataBufferPtr->StructureHeader.Handle)) {
        RemoveEntryList (&Record->Link);
        gSmst->SmmFreePool(Record);
        return EFI_SUCCESS;
      }
      Link = GetNextNode(
               &mPnp52HeadNode,
               &Record->Link);
    }
  }

  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    sizeof (PNP_52_RECORD),
                    (VOID **)&Record
                    );
  if (EFI_ERROR (Status)) {
    return Status ;
  }

  ZeroMem (Record, sizeof (PNP_52_RECORD));

  InsertTailList(&mPnp52HeadNode, &Record->Link);

  Record->Signature = EFI_PNP_52_SIGNATURE;

  Record->header.Signature  = SMBIOS_REC_SIGNATURE;

  RecordLen = sizeof (SMBIOS_REC_HEADER) + sizeof (PNP_52_DATA_BUFFER) - 1;
  RecordLen = RecordLen + pDataBufferPtr->DataLength;
  Record->header.RecordLen  = RecordLen;

  Length  = sizeof (PNP_52_DATA_BUFFER) - 1;
  Length += pDataBufferPtr->DataLength;

  Status  = gSmst->SmmAllocatePool (
                     EfiRuntimeServicesData,
                     Length,
                     (VOID **)&pData
                     );
  if (EFI_ERROR (Status)) {
    return Status ;
  }

  CopyMem (pData, (UINT8 *) pDataBufferPtr, Length);

  Record->pRecord = pData;

  return Status;
}

EFI_STATUS
DeleteRecord(
  PNP_52_DATA_BUFFER *pDataBufferPtr
)
{
  LIST_ENTRY      *Link;
  PNP_52_RECORD   *Record = NULL;

  if (IsListEmpty (&mPnp52HeadNode)) {
    return EFI_UNSUPPORTED;
  } else {
    Link = GetFirstNode (&mPnp52HeadNode);
  }

  while(!IsNull (&mPnp52HeadNode, Link)) {
    Record = CR (Link, PNP_52_RECORD, Link, EFI_PNP_52_SIGNATURE);
    if ((Record->pRecord->Command == pDataBufferPtr->Command) &&
        (Record->pRecord->FieldOffset == pDataBufferPtr->FieldOffset) &&
        (Record->pRecord->StructureHeader.Type == pDataBufferPtr->StructureHeader.Type) &&
        (Record->pRecord->StructureHeader.Handle == pDataBufferPtr->StructureHeader.Handle)) {
      RemoveEntryList (&Record->Link);
      gSmst->SmmFreePool(Record);
      break;
    }
    Link = GetNextNode(
             &mPnp52HeadNode,
             &Record->Link);
  }
  return EFI_SUCCESS;
}

EFI_STATUS
Mem2RecordList (
  UINT8   *pBase,
  UINTN   size
)
{
  EFI_STATUS    Status;
  UINT8         *RecordPtr;
  UINT8         *RecordEndPtr;
  UINTN         Length;

  PNP_52_DATA_BUFFER *DataBufferInRec;

  RecordPtr = pBase;
  RecordEndPtr =  (UINT8 *)(pBase + size);

  while (RecordPtr < RecordEndPtr) {
    if (((SMBIOS_REC_HEADER *)RecordPtr)->Signature != SMBIOS_REC_SIGNATURE)
      break;

    DataBufferInRec = (PNP_52_DATA_BUFFER *)(RecordPtr + sizeof (SMBIOS_REC_HEADER));
    Length  = sizeof (PNP_52_DATA_BUFFER) - 1;
    Length += DataBufferInRec->DataLength;

    Status  = InsertRecord (DataBufferInRec);
    if (EFI_ERROR (Status)) {
      return Status ;
    }
    RecordPtr = RecordPtr + Length + sizeof (SMBIOS_REC_HEADER);
  }

  return EFI_SUCCESS;
}

UINTN
RecordList2Mem (
  UINT8 *pBase
)
{
  LIST_ENTRY        *Link;
  PNP_52_RECORD     *Record;
  UINTN             Length = 0;

  if (IsListEmpty (&mPnp52HeadNode)) {
    return Length;
  } else {
    Link = GetFirstNode (&mPnp52HeadNode);
  }

  while(!IsNull (&mPnp52HeadNode, Link)) {
    Record  = CR (Link, PNP_52_RECORD, Link, EFI_PNP_52_SIGNATURE);
    CopyMem (pBase, (UINT8 *)&Record->header, sizeof (SMBIOS_REC_HEADER));
    pBase  += sizeof(SMBIOS_REC_HEADER);
    CopyMem (pBase, Record->pRecord, (Record->header.RecordLen - sizeof (SMBIOS_REC_HEADER)));
    pBase  += (Record->header.RecordLen - sizeof (SMBIOS_REC_HEADER));
    Length += Record->header.RecordLen;
    Link    = GetNextNode(
                &mPnp52HeadNode,
                &Record->Link);
  }

  return Length;
}

UINTN
PnpStringDataLen (
  IN UINT8 *StringData
)
{
  UINT8 *StringPtr = StringData;

  for (; *StringPtr != 0; StringPtr ++);

  return (UINTN)(StringPtr - StringData);
}

UINTN
PnpGetSmbiosStructureSize (
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  UINT8 *SmbiosPtr;

  SmbiosPtr = StrucPtr->Raw + StrucPtr->Hdr->Length;

  //
  // Find two NULL string
  //
  for (; *(UINT16 *)SmbiosPtr != 0; SmbiosPtr ++);

  SmbiosPtr += 2;

  //
  // Return the correct size
  //
  return (UINTN)(SmbiosPtr - StrucPtr->Raw);
}


UINTN
PnpGetSmbiosStructuresSize (
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  SMBIOS_STRUCTURE_POINTER  Smbios;
  UINTN                     TototalSize = 0;
  UINTN                     StrucSize   = 0;

  Smbios.Raw = StrucPtr->Raw;

  //
  // Scan the smbios table to the end and caculate the total size
  //
  while (Smbios.Hdr->Type != 127) {
    StrucSize = PnpGetSmbiosStructureSize(&Smbios);
    TototalSize += StrucSize;
    Smbios.Raw =  Smbios.Raw + StrucSize;
  }

  StrucSize = PnpGetSmbiosStructureSize(&Smbios);
  TototalSize += StrucSize;

  return TototalSize;
}

UINTN
PnpGetMaxSmbiosStructureSize (
  VOID
)
{
  SMBIOS_STRUCTURE_POINTER  Smbios;
  SMBIOS_STRUCTURE_POINTER  SmbiosEnd;
  UINTN                     MaxSize   = 0;
  UINTN                     StrucSize = 0;

  Smbios.Raw    = (UINT8 *)(UINTN)(mSmbiosEfiEntry->TableAddress);
  SmbiosEnd.Raw = Smbios.Raw + mSmbiosEfiEntry->TableLength;

  //
  // Scan the smbios table to the end and caculate the total size
  //
  while (Smbios.Raw < SmbiosEnd.Raw) {
    StrucSize = PnpGetSmbiosStructureSize (&Smbios);
    if (StrucSize > MaxSize)
      MaxSize = StrucSize;
    Smbios.Raw = Smbios.Raw + StrucSize;
  }
  return MaxSize;
}

VOID
PnpUpdateSmbiosEntryInMem (
  VOID
)
{
  SMBIOS_STRUCTURE_POINTER  StrucTemp;
  UINT8                     CheckSum;
  UINTN                     Index;
  UINT8                     FsegValue8;
  UINT8                     EsegValue8;

  StrucTemp.Raw = (UINT8 *)(UINTN)mSmbiosEfiEntry->TableAddress;

  //
  // Update the EFI entry
  //

  //
  // Update some fields in entry
  //
  mSmbiosEfiEntry->TableLength = (UINT16)PnpGetSmbiosStructuresSize(&StrucTemp);
  mSmbiosEfiEntry->MaxStructureSize = (UINT16)PnpGetMaxSmbiosStructureSize();

  CheckSum = 0;
  mSmbiosEfiEntry->IntermediateChecksum = 0;
  for (Index = 0x10; Index < mSmbiosEfiEntry->EntryPointLength; Index++) {
    CheckSum = (UINT8) (CheckSum + ((UINT8 *)(mSmbiosEfiEntry))[Index]);
  }
  mSmbiosEfiEntry->IntermediateChecksum = (UINT8)(0 - CheckSum);

  CheckSum = 0;
  mSmbiosEfiEntry->EntryPointStructureChecksum = 0;
  for (Index = 0x0; Index < mSmbiosEfiEntry->EntryPointLength; Index++) {
    CheckSum = (UINT8) (CheckSum + ((UINT8 *)(mSmbiosEfiEntry))[Index]);
  }
  mSmbiosEfiEntry->EntryPointStructureChecksum = (UINT8)(0 - CheckSum);
  //
  // Enable E & F segment write
  //
  EnableMemAccess(&FsegValue8, &EsegValue8);
  //
  // Update the legacy entry by copy
  //
  if (mSmbiosLegacyEntry != NULL) {
    CopyMem (
      ((UINT8 *) (UINTN)mSmbiosLegacyEntry->TableAddress),
      ((UINT8 *) (UINTN)mSmbiosEfiEntry->TableAddress),
      mSmbiosEfiEntry->TableLength
    );
    mSmbiosLegacyEntry->TableLength              = mSmbiosEfiEntry->TableLength;
    mSmbiosLegacyEntry->MaxStructureSize         = mSmbiosEfiEntry->MaxStructureSize;
    mSmbiosLegacyEntry->NumberOfSmbiosStructures = mSmbiosEfiEntry->NumberOfSmbiosStructures;

    CheckSum = 0;
    mSmbiosLegacyEntry->IntermediateChecksum = 0;
    for (Index = 0x10; Index < mSmbiosLegacyEntry->EntryPointLength; Index++) {
      CheckSum = (UINT8) (CheckSum + ((UINT8 *)(mSmbiosLegacyEntry))[Index]);
    }
    mSmbiosLegacyEntry->IntermediateChecksum = (UINT8)(0 - CheckSum);

    CheckSum = 0;
    mSmbiosLegacyEntry->EntryPointStructureChecksum = 0;
    for (Index = 0x0; Index < mSmbiosLegacyEntry->EntryPointLength; Index++) {
      CheckSum = (UINT8) (CheckSum + ((UINT8 *)(mSmbiosLegacyEntry))[Index]);
    }
    mSmbiosLegacyEntry->EntryPointStructureChecksum = (UINT8)(0 - CheckSum);
  }
  //
  // Recover E & F segment write
  //
  DisableMemAccess (FsegValue8, EsegValue8);
}

EFI_STATUS
PnpUpdateStringDataInMem (
  IN PNP_52_DATA_BUFFER       *DataBufferPtr,
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  SMBIOS_STRUCTURE_POINTER  StrucTemp;
  UINT8                     StringIndex;
  UINT8                     Index;
  UINT8                     *UpdateBufPtr   = NULL;
  UINT8                     *StringDataPtr  = NULL;
  UINT8                     *StrucEndPtr    = NULL;
  UINTN                     CopySize        = 0;

  //
  // Check whether the smbios update buffer can contain the new smbios table
  //
  if ((mSmbiosEfiEntry->TableLength + DataBufferPtr->DataLength > SMBIOS_BUFFER_SIZE) ||
      (DataBufferPtr->DataLength == 1)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Clean out the smbios update buffer
  //
  ZeroMem (mSmbiosBufferPtr, SMBIOS_BUFFER_SIZE);

  //
  // Copy the structure into update buffer except all the string data belonged
  //
  UpdateBufPtr = mSmbiosBufferPtr;
  CopyMem (UpdateBufPtr, StrucPtr->Raw, StrucPtr->Hdr->Length);
  UpdateBufPtr += StrucPtr->Hdr->Length;

  //
  // Get the string number, 1 based
  //
  StringIndex = *(UINT8 *)(StrucPtr->Raw + DataBufferPtr->FieldOffset);
  DEBUG((EFI_D_ERROR,"StringIndex = %x\n", StringIndex));

  //
  // if no string belonged, return error
  //
  if (StringIndex == 0)
    return EFI_INVALID_PARAMETER;

  StringDataPtr = StrucPtr->Raw + StrucPtr->Hdr->Length;
  StrucEndPtr   = StrucPtr->Raw + PnpGetSmbiosStructureSize (StrucPtr) - 1;
  Index         = 1;

  while (StringDataPtr < StrucEndPtr) {
    if (Index != StringIndex) {
      CopySize = PnpStringDataLen (StringDataPtr) + 1;
      CopyMem (UpdateBufPtr, StringDataPtr, CopySize);
      UpdateBufPtr += CopySize;
      StringDataPtr += CopySize;
    } else {
      CopySize = DataBufferPtr->DataLength;
      CopyMem(UpdateBufPtr, DataBufferPtr->StructureData, CopySize);
      UpdateBufPtr += CopySize;
      CopySize = PnpStringDataLen(StringDataPtr) + 1;
      StringDataPtr += CopySize;
    }
    Index++;
  }

  //
  // Add the NULL byte at the end of the string set
  //
  *UpdateBufPtr = 0;
  UpdateBufPtr ++;
  StringDataPtr ++;

  //
  // Now StringDataPtr point to the next sequential structure,
  // Copy the remained structures content into update buffer
  //
  StrucTemp.Raw = StringDataPtr;
  CopySize = PnpGetSmbiosStructuresSize(&StrucTemp);
  CopyMem (UpdateBufPtr, StringDataPtr, CopySize);

  //
  // Copy the update buffer content into smbios table
  //
  StrucTemp.Raw = mSmbiosBufferPtr;
  CopySize = PnpGetSmbiosStructuresSize(&StrucTemp);
  CopyMem (StrucPtr->Raw,  mSmbiosBufferPtr, CopySize);

  //
  // Update the smbios entry in mem because of the changed table length
  //
  PnpUpdateSmbiosEntryInMem();

  return EFI_SUCCESS;
}


EFI_STATUS
UpdateToMem (
  IN PNP_52_DATA_BUFFER *DataBufferPtr,
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  EFI_STATUS  Status = EFI_UNSUPPORTED;
  UINT32      OffsetOfSmbios;
  UINT32      Value32;
  UINT8       FsegValue8;
  UINT8       EsegValue8;

  DEBUG((EFI_D_ERROR,"smbios Command = %x\n", DataBufferPtr->Command));
  if (DataBufferPtr->Command == DoubleWordChanged) {
    Value32  = *(UINT8 *)(StrucPtr->Raw + DataBufferPtr->FieldOffset);
    Value32 &= DataBufferPtr->ChangeMask;
    Value32 |= DataBufferPtr->ChangeValue;
    *(UINT32 *)(StrucPtr->Raw + DataBufferPtr->FieldOffset) = Value32;
    //
    // update legacy smbios table
    //
    if (mSmbiosLegacyEntry && mSmbiosEfiEntry) {
      //
      // Enable E & F segment write
      //
      EnableMemAccess(&FsegValue8, &EsegValue8);
      OffsetOfSmbios = (UINT32)(UINTN)(StrucPtr->Raw + DataBufferPtr->FieldOffset) - mSmbiosEfiEntry->TableAddress;
      *(UINT32 *)(UINTN)(mSmbiosLegacyEntry->TableAddress + OffsetOfSmbios) = Value32;
      //
      // Restore E & F segment write
      //
      DisableMemAccess (FsegValue8, EsegValue8);
    }
    Status = EFI_SUCCESS;
  } else if (DataBufferPtr->Command == StringChanged) {
    Status =  PnpUpdateStringDataInMem (DataBufferPtr, StrucPtr);
    Status = EFI_SUCCESS;
  }
  DeleteRecord (DataBufferPtr);
  InsertRecord (DataBufferPtr);

  return Status;
}

VOID
GetSmbiosDataFromFlash ()
{
  EFI_STATUS  Status;
  UINTN       Size = EFI_PAGE_SIZE;
  UINTN       SmbiosRecBase = FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE;

  InitializeListHead (&mPnp52HeadNode);
  Status = Mem2RecordList((UINT8*)SmbiosRecBase, Size);
  ASSERT_EFI_ERROR (Status);
}

VOID
Write2Flash (
  UINT8     *SourceAddress,
  UINTN     FlashAddress,
  UINTN     Size
)
{
  if (mMediaAccess != NULL && Size > 0) {
    mMediaAccess->Erase (
                    mMediaAccess,
                    FlashAddress,
                    SIZE_4KB,
                    SPI_MEDIA_TYPE
                    );
    mMediaAccess->Write (
                    mMediaAccess,
                    FlashAddress,
                    SourceAddress,
                    Size,
                    SPI_MEDIA_TYPE
                    );
  }
}

VOID
WriteSmbiosData2Flash ()
{
  UINTN       TotalLen;

  SetMem (mSmbiosScratchPtr, EFI_PAGE_SIZE, 0xff);
  TotalLen = RecordList2Mem (mSmbiosScratchPtr);

  Write2Flash (mSmbiosScratchPtr, FLASH_REGION_NVSTORAGE_SUBREGION_NV_SMBIOS_STORE_BASE, TotalLen);

}

EFI_STATUS
GetValueFromAsccii (
  IN OUT UINT8  *BinBuffer,
  IN OUT UINT32 *BinLength,
  IN     CHAR8  *HexStr
  )
{
  UINTN   Index;
  UINTN   Length;
  UINT8   Digit;
  CHAR8   TemStr[2];

  ZeroMem (TemStr, sizeof (TemStr));

  //
  // Find out how many hex characters the string has.
  //
  if ((HexStr[0] == '0') && ((HexStr[1] == 'x') || (HexStr[1] == 'X'))) {
    HexStr += 2;
  }

  Length = AsciiStrLen (HexStr);

  for (Index = 0; Index < Length; Index ++) {
    TemStr[0] = HexStr[Index];
    Digit = (UINT8) AsciiStrHexToUint64 (TemStr);
    if (Digit == 0 && TemStr[0] != '0') {
      //
      // Invalid Lun Char
      //
      break;
    }
    if ((Index & 1) == 0) {
      BinBuffer [Index/2] = Digit;
    } else {
      BinBuffer [Index/2] = (UINT8) ((BinBuffer [Index/2] << 4) + Digit);
    }
  }

  *BinLength = (UINT32) ((Index + 1)/2);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetSystemConfigurationTable (
  IN EFI_GUID *TableGuid,
  IN OUT VOID **Table
  )
{
  UINTN Index;
  ASSERT (Table != NULL);

  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (TableGuid, &(gST->ConfigurationTable[Index].VendorGuid))) {
      *Table = gST->ConfigurationTable[Index].VendorTable;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


VOID
GetSmbiosEntry (
  VOID
)
{
  UINT8 *Ptr;
  if (mSmbiosLegacyEntry == NULL) {
    Ptr = (UINT8 *) (UINTN) 0xf0000;
    for ( ; Ptr < (UINT8 *) (UINTN) 0x100000; Ptr += 0x10) {
      if (*(UINT32 *) Ptr == SIGNATURE_32('_','S','M','_')) {
        mSmbiosLegacyEntry = (SMBIOS_TABLE_ENTRY_POINT *)Ptr;
      }
    }
  }
  //
  // Find the efi smbios table entry
  //
  if (mSmbiosEfiEntry == NULL) {
    GetSystemConfigurationTable (
      &gEfiSmbiosTableGuid,
      (VOID **)&mSmbiosEfiEntry
    );
    ASSERT (mSmbiosEfiEntry != NULL);
  }

  return;
}

BOOLEAN
GetSmbiosStructureByType (
  IN UINT8                    StrucType,
  IN SMBIOS_STRUCTURE_POINTER *StrucPtr
)
{
  SMBIOS_STRUCTURE_POINTER  Smbios;
  SMBIOS_STRUCTURE_POINTER  SmbiosEnd;

  if (mSmbiosEfiEntry == NULL || mSmbiosLegacyEntry == NULL)
    GetSmbiosEntry ();

  //
  // By default, we use the efi entry table
  //
  Smbios.Raw = (UINT8 *)(UINTN)(mSmbiosEfiEntry->TableAddress);
  SmbiosEnd.Raw = Smbios.Raw + mSmbiosEfiEntry->TableLength;

  //
  // Scan the smbios table and find the matched smbios structure having the
  // Same handle number as given
  //
  while (Smbios.Raw < SmbiosEnd.Raw) {
    if (Smbios.Hdr->Type== StrucType) {
      StrucPtr->Raw = Smbios.Raw;
      return TRUE;
    }
    Smbios.Raw = Smbios.Raw + PnpGetSmbiosStructureSize (&Smbios);
  }

  //
  // if no matched structure is found, return null
  //
  StrucPtr->Raw = NULL;
  return FALSE;
}

EFI_STATUS
FlashInterface (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  )
/*++

Routine Description:

 Calling the SMI Interface
 The caller will write AL (the value 0xef) to the SMI Command Port as
 defined in the ACPI FADT.
 The SMI handler will update the callers' buffer(s) and return.
 Notes:
 The function will clear the carry bit if it is successful (CF = 0).
 If the function is unsuccessful, it will set the carry bit and set the
 error code in the AH register as indicated by the error table below.
 The function returns the following data in the provided parameter block.

Arguments:

  DispatchHandle  - Handle of Disaptcher
  DispatchContext - Context when the Disaptcher calls the function

Returns:

  EFI_STATUS

--*/
{
    EFI_STATUS                 Status;
    UINTN                      Index;
    UINTN                      CpuIndex;
    UINT8                      SubFunction;
    FLASH_BLOCK_INFO           *FlashBlockInfo;
    BLOCK_READ_PARAMETER       *BlockPara;
    MEDIA_BLOCK_MAP            *MapInfo;
    FD_AREA_INFO               *FdArea;
    UINTN                      BufferSize;
    UINT32                     RegEax;
    UINT32                     RegEbx;
    UINT32                     RegEflags;
    EFI_SMM_SAVE_STATE_IO_INFO IoState;
    EFI_SMI_CPU_SAVE_STATE     *SaveState;
    //BIOS_ID_IMAGE              CurrentBiosId;
    //BIOS_ID_IMAGE              CapsuleBiosId;
    //FLASH_BUFFER               *FlashBuf;
    //UINTN                      FvMainOffset;
    //UINTN                      CompareLength;
    //
    // Find out which CPU triggered the SW SMI
    //
    CpuIndex = 0;
    for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
        Status = mSmmCpu->ReadSaveState (
                            mSmmCpu,
                            sizeof (EFI_SMM_SAVE_STATE_IO_INFO),
                            EFI_SMM_SAVE_STATE_REGISTER_IO,
                            Index,
                            &IoState
                            );
        if (!EFI_ERROR (Status) && (IoState.IoData == SW_SMI_FLASH_SERVICES)) {
            CpuIndex = Index;
            break;
        }
    }

    if (Index >= gSmst->NumberOfCpus) {
        CpuDeadLoop ();
    }

    //
    // Ready save state for register
    //
    Status = mSmmCpu->ReadSaveState (
                        mSmmCpu,
                        sizeof (UINT32),
                        EFI_SMM_SAVE_STATE_REGISTER_RAX,
                        CpuIndex,
                        &RegEax
                        );
    ASSERT_EFI_ERROR (Status);

    Status = mSmmCpu->ReadSaveState (
                        mSmmCpu,
                        sizeof (UINT32),
                        EFI_SMM_SAVE_STATE_REGISTER_RBX,
                        CpuIndex,
                        &RegEbx
                        );
    ASSERT_EFI_ERROR (Status);

    Status = mSmmCpu->ReadSaveState (
                        mSmmCpu,
                        sizeof (UINT32),
                        EFI_SMM_SAVE_STATE_REGISTER_RFLAGS,
                        CpuIndex,
                        &RegEflags
                        );
    ASSERT_EFI_ERROR (Status);

    //
    // Extract the sub function number from AH,
    // and process the requested sub function.
    //
    SubFunction = (UINT8) ((RegEax & 0xFF00) >> 8);

    switch (SubFunction) {
    case GET_FLASH_INFO_SUBFUNCTION:
        //
        // EBX = address of the FLASH_BLOCK_INFO parameter block
        //
        FlashBlockInfo = (FLASH_BLOCK_INFO *) (UINTN) RegEbx;

        Status = mMediaAccess->Info (
            mMediaAccess,
            &MapInfo,
            SPI_MEDIA_TYPE
            );

        if(FlashBlockInfo->BlockCount < 1 ||
            FlashBlockInfo->BlockSize < (FlashBlockInfo->BlockCount + 1) * sizeof(FlashBlockInfo)
        ) {
            FlashBlockInfo->BlockCount = 0;
            FlashBlockInfo->BlockSize  = 0;

            while(MapInfo->Count) {
                FlashBlockInfo->BlockCount++;
                MapInfo++;
            }

            FlashBlockInfo->BlockSize = (FlashBlockInfo->BlockCount + 1) * sizeof(FlashBlockInfo);
        } else {
            Index = 1;
            while(MapInfo->Count && (Index - 1) < FlashBlockInfo->BlockCount) {
                FlashBlockInfo[Index].BlockSize  = (UINT16)MapInfo->Size;
                FlashBlockInfo[Index].BlockCount = (UINT16)MapInfo->Count;

                Index++;
                MapInfo++;
            }

            if((Index - 1) < FlashBlockInfo->BlockCount) {
                FlashBlockInfo->BlockCount = (UINT16)(Index - 1);
                FlashBlockInfo->BlockSize = (FlashBlockInfo->BlockCount + 1) * sizeof(FlashBlockInfo);
            }
        }
        break;

    case GET_FD_AREA_SUBFUNCTION:
        FdArea = (FD_AREA_INFO *) (UINTN) RegEbx;
        Status = RETURN_SUCCESS;

        switch(FdArea->Type) {

        case FD_AREA_TYPE_FD:
            FdArea->Offset = mFDBaseAddress;
            FdArea->Size   = mFDSize;
            break;

        case FD_AREA_TYPE_FVMAIN_COMPACT:
            FdArea->Offset = PcdGet32(PcdFlashFvMainBase) - mFDBaseAddress;
            FdArea->Size   = PcdGet32(PcdFlashFvMainSize);
            break;

        case FD_AREA_TYPE_NVSTORAGE_VARIABLE:
            FdArea->Offset = PcdGet32(PcdFlashNvStorageVariableBase) - mFDBaseAddress;
            FdArea->Size   = PcdGet32(PcdFlashNvStorageVariableSize);
            break;

        case FD_AREA_TYPE_NVSTORAGE_MICROCODE:
            FdArea->Offset = PcdGet32(PcdFlashNvStorageMicrocodeBase) - mFDBaseAddress;
            FdArea->Size   = PcdGet32(PcdFlashNvStorageMicrocodeSize);
            break;

        case FD_AREA_TYPE_FVRECOVERY:
            FdArea->Offset = PcdGet32(PcdFlashFvRecoveryBase) - mFDBaseAddress;
            FdArea->Size   = PcdGet32(PcdFlashFvRecoverySize);
            break;

        default:
            Status = RETURN_INVALID_PARAMETER;
            break;
        }
        break;

    case READ_FLASH_BLOCK_SUBFUNCTION:
        //
        // EBX = address of the BLOCK_READ_PARAMETER parameter block
        //
        BlockPara   = (BLOCK_READ_PARAMETER *) (UINTN) RegEbx;
        Status      = EFI_SUCCESS;

        if (BlockPara->Offset >= mFDSize) {
          Status    = RETURN_INVALID_PARAMETER;
          break;
        }

        BufferSize  = (UINTN)BlockPara->BufferSize;
        if (BlockPara->Offset + BufferSize > mFDSize) {
          BufferSize = mFDSize - BlockPara->Offset;
        }

        /*
        Status = mMediaAccess->Read (
                                 mMediaAccess,
                                 mFDBaseAddress + BlockPara->Offset,
                                 (UINT8*)(UINTN)BlockPara->Buffer,
                                 &BufferSize,
                                 SPI_MEDIA_TYPE
                                 );
        */
        CopyMem ((UINT8 *)(UINTN)(BlockPara->Buffer), (UINT8 *)(UINTN)(mFDBaseAddress + BlockPara->Offset), BufferSize);
        BlockPara->BufferSize   = (UINT32)BufferSize;
        break;
/*
    case COMPARE_BIOS_ID_SUBFUNCTION:
        Status = GetBiosId ((UINT8 *)(UINTN)PcdGet32(PcdFlashFvMainBase),
                            (UINT8 *)(UINTN)(PcdGet32(PcdFlashFvMainBase) + PcdGet32(PcdFlashFvMainSize)),
                            &CurrentBiosId
                           );
        DEBUG ((EFI_D_ERROR,"Get CurrentBiosId Status:%r\n", Status));
        //
        // EBX = address of the parameter block
        //
        FlashBuf = (FLASH_BUFFER *) (UINTN) RegEbx;
        FvMainOffset = PcdGet32(PcdFlashFvMainBase) - mFDBaseAddress;
        Status = GetBiosId ((UINT8 *)(UINTN)(FlashBuf->Buffer + FvMainOffset),
                            (UINT8 *)(UINTN)(FlashBuf->Buffer + FvMainOffset + PcdGet32(PcdFlashFvMainSize)),
                            &CapsuleBiosId
                            );
        DEBUG ((EFI_D_ERROR,"Get CapsuleBiosId Status:%r\n", Status));
        Status = RETURN_NOT_FOUND;
        // just care Signature, BoardId & OemId
        CompareLength = STRUCT_FIELD_SIZE (BIOS_ID_IMAGE, Signature) +
                        STRUCT_FIELD_SIZE (BIOS_ID_STRING, BoardId) +
                        STRUCT_FIELD_SIZE (BIOS_ID_STRING, OemId);
        if (!CompareMem(&CurrentBiosId, &CapsuleBiosId, CompareLength)) {
          Status = EFI_SUCCESS;
        }
        break;
*/
    case UPDATE_SMBIOS_DATA:
      {
        UPDATE_SMBIOS_PARAMETER  *SmbiosPtr;
        SMBIOS_STRUCTURE_POINTER StrucPtr;
        PNP_52_DATA_BUFFER       *Parameter;
        UINT8                    Index;
        UINT32                   DataLength;
        UINT32                   Data32[4];
        BOOLEAN                  NeedUpdate  = FALSE;
        BOOLEAN                  DataIsValid = TRUE;

        //
        // EBX = address of the parameter block
        //
        SmbiosPtr = (UPDATE_SMBIOS_PARAMETER *)(UINTN) RegEbx;
        Parameter = &SmbiosPtr->Parameter;
        DEBUG ((DEBUG_INFO, "SubFun:%x\n",SmbiosPtr->SubFun));
        Parameter->DataLength = (UINT16)AsciiStrLen (Parameter->StructureData) + 1;
        DEBUG ((DEBUG_INFO, "Parameter->DataLength:%x\n",Parameter->DataLength));
        if (Parameter->DataLength == 1) {
          Status = RETURN_INVALID_PARAMETER;
          break;
        }
        if (SmbiosPtr->SubFun == UPDATE_UUID ||
            SmbiosPtr->SubFun == UPDATE_SERIAL_NUMBER ||
            SmbiosPtr->SubFun == UPDATE_MODEL_NUMBER ||
            SmbiosPtr->SubFun == UPDATE_BRAND_ID) {
          Parameter->StructureHeader.Type = 1;
        } else if (SmbiosPtr->SubFun == UPDATE_BASE_BOARD_SERIAL_NUMBER ||
                   SmbiosPtr->SubFun == UPDATE_BASE_BOARD_ASSET_TAG) {
          Parameter->StructureHeader.Type = 2;
        } else if (SmbiosPtr->SubFun == UPDATE_ASSET_TAG) {
          Parameter->StructureHeader.Type = 3;
        }
        if (GetSmbiosStructureByType (Parameter->StructureHeader.Type, &StrucPtr)) {
          Parameter->StructureHeader.Length = StrucPtr.Hdr->Length;
          Parameter->StructureHeader.Handle = StrucPtr.Hdr->Handle;
          Parameter->Command                = StringChanged;  // default command
          NeedUpdate = TRUE;
        }
        switch(SmbiosPtr->SubFun) {
          case UPDATE_UUID:
            //
            // check string is valid
            // a~f:0x61~0x66, A~F:0x41~0x46, 0~9:0x30~0x39
            //
            for (Index = 0; Index < 0x20; Index ++) {
              if (Parameter->StructureData[Index] > 'f') {
                DataIsValid = FALSE;
              } else if (Parameter->StructureData[Index] < 'a' && Parameter->StructureData[Index] > 'F') {
                DataIsValid = FALSE;
              } else if (Parameter->StructureData[Index] < 'A' && Parameter->StructureData[Index] > '9') {
                DataIsValid = FALSE;
              } else if (Parameter->StructureData[Index] < '0') {
                DataIsValid = FALSE;
              }
              if (DataIsValid == FALSE) {
                NeedUpdate = FALSE;
                Status     = RETURN_INVALID_PARAMETER;
                break;
              }
            }
            if (DataIsValid) {
              GetValueFromAsccii ((UINT8*)(UINTN)Data32, &DataLength, Parameter->StructureData);
              Parameter->Command     = DoubleWordChanged;
              Parameter->ChangeMask  = 0;
              Parameter->DataLength  = 0;
              Parameter->FieldOffset = 0x08; // UUID offset of Type1
              for (Index = 0; Index < (DataLength / 4); Index ++) {
                Parameter->ChangeValue = Data32[Index];
                UpdateToMem (Parameter, &StrucPtr);
                Parameter->FieldOffset += 4;
              }
            }
            break;
          case UPDATE_SERIAL_NUMBER:
          case UPDATE_BASE_BOARD_SERIAL_NUMBER:
            Parameter->FieldOffset = 0x07;
            break;
          case UPDATE_MODEL_NUMBER:
            Parameter->FieldOffset = 0x05;
            break;
          case UPDATE_BRAND_ID:
            Parameter->FieldOffset = 0x06;
            break;
          case UPDATE_ASSET_TAG:
          case UPDATE_BASE_BOARD_ASSET_TAG:
            Parameter->FieldOffset = 0x08;
            break;
          default:
              Status = RETURN_INVALID_PARAMETER;
              break;
        }
        if (NeedUpdate) {
          if (Parameter->Command == StringChanged) {
            if (Parameter->DataLength > MAX_STRING_LENGTH) {
              Parameter->StructureData[MAX_STRING_LENGTH] = '\0';
              Parameter->DataLength = MAX_STRING_LENGTH + 1;
            }
            UpdateToMem (Parameter, &StrucPtr);
            //
            // update Type3 serial number
            //
            if (SmbiosPtr->SubFun == UPDATE_SERIAL_NUMBER) {
              Parameter->StructureHeader.Type = 3;
              if (GetSmbiosStructureByType (Parameter->StructureHeader.Type, &StrucPtr)) {
                Parameter->StructureHeader.Length = StrucPtr.Hdr->Length;
                Parameter->StructureHeader.Handle = StrucPtr.Hdr->Handle;
                UpdateToMem (Parameter, &StrucPtr);
              }
            }
          }
          WriteSmbiosData2Flash ();
        }
      }
      break;
/*
    case BIOS_VERIFY:
      {
        int                         err;
        static EVP_MD_CTX           md_ctx;
        UINT8                       *PubDerKey;
        UINT8                       PubkeyHash1[SHA256_DIGEST_LENGTH];
        UINT8                       *PubkeyHash2 = NULL;
        EVP_PKEY                    *EvpPubKey = NULL;
        RSA                         *PubRsa;
        UINT32                      DataSize;
        UINT8                       *Data;
        BIOS_VERIFY_PARAMETER       *BiosVerifyPtr;
        BIOS_VERIFY_CAPSULE_BUFFER  *CapsuleBuffer;
        EFI_GUID                     SignGuid = gSignCapsuleHeaderGuild;

        BiosVerifyPtr = (BIOS_VERIFY_PARAMETER *)(UINTN) RegEbx;
        Status = EFI_SUCCESS;
        switch(BiosVerifyPtr->SubFun) {
          case VERIFY_INIT:
            err = 0xFF;
            CapsuleBuffer = (BIOS_VERIFY_CAPSULE_BUFFER *)(UINTN) (BiosVerifyPtr->Buffer);
            if (CompareGuid(&SignGuid, &(CapsuleBuffer->Header.CapsuleGuid))) {
              PubDerKey     = CapsuleBuffer->Data;
              DataSize      = CapsuleBuffer->PubkeySize;
              SHA256 (PubDerKey, DataSize, PubkeyHash1);
              Status = GetPubkey (
                         (UINT8 *)(UINTN)PcdGet32(PcdFlashFvRecoveryBase),
                         (UINT8 *)(UINTN)((PcdGet32(PcdFlashFvRecoveryBase) - 16 + PcdGet32(PcdFlashFvRecoverySize))),
                         &PubkeyHash2
                         );
              ASSERT (Status == EFI_SUCCESS);
              if (!CompareMem(PubkeyHash2, PubkeyHash1, SHA256_DIGEST_LENGTH)) {
                err = EVP_VerifyInit(&md_ctx, EVP_sha256());
              }
            }
            DEBUG ((EFI_D_ERROR, "init Err:%d\n", err));
            break;
          case VERIFY_UPDATE:
            Data = (UINT8 *)(UINTN)BiosVerifyPtr->Buffer;
            err  = EVP_VerifyUpdate(&md_ctx, Data, BiosVerifyPtr->Size);
            DEBUG ((EFI_D_ERROR, "update Err:%d\n", err));
            break;
          case VERIFY_FINAL:
            CapsuleBuffer = (BIOS_VERIFY_CAPSULE_BUFFER *)(UINTN) (BiosVerifyPtr->Buffer);
            PubDerKey     = CapsuleBuffer->Data;
            DataSize      = CapsuleBuffer->PubkeySize;
            PubRsa = d2i_RSAPublicKey(NULL, &PubDerKey, DataSize); 
            if((EvpPubKey = EVP_PKEY_new())==NULL){
              err = 3;
              goto ErrorExit;
            }
            if(EVP_PKEY_assign_RSA(EvpPubKey, PubRsa)!=1){
              err = 4;
              goto ErrorExit;
            }
            DataSize      = ALIGN_VALUE(DataSize, 4);
            Data          = &CapsuleBuffer->Data[DataSize];
            err = EVP_VerifyFinal(&md_ctx, Data, CapsuleBuffer->SignSize, EvpPubKey);
          ErrorExit:
            DEBUG ((EFI_D_ERROR, "final Err:%d\n", err));
            if (EvpPubKey != NULL) {
              EVP_PKEY_free (EvpPubKey);
            }
            if (err == 1) {
              EVP_MD_CTX_cleanup(&md_ctx);
            }
            break;
 
          default:
            err = 0xff;
            break;
        }
        if (err != 1) {
          EVP_MD_CTX_cleanup(&md_ctx);
          Status = RETURN_ABORTED;
        }
      }
      break;
*/
    default:
        Status = RETURN_INVALID_PARAMETER;
        break;
    }
    //
    // Set CF in case of error, clear on success
    //
    if (EFI_ERROR (Status)) {
        RegEflags |= CARRY_FLAG;
    } else {
        RegEflags &= ~CARRY_FLAG;
    }
    //
    // Put the error code in AH
    //
    RegEax &= 0xFFFF00FF;
    RegEax |= ((Status & 0xFF)   << 8);

    Status = mSmmCpu->WriteSaveState (
                        mSmmCpu,
                        sizeof (UINT32),
                        EFI_SMM_SAVE_STATE_REGISTER_RAX,
                        CpuIndex,
                        &RegEax
                        );
    ASSERT_EFI_ERROR (Status);
    if (mFrameworkSmst != NULL) {
      SaveState = &mFrameworkSmst->CpuSaveState[CpuIndex].Ia32SaveState;
      SaveState->EAX = RegEax;
    }
    return Status;
}

EFI_STATUS
AllocDataBuffer ()
{
  EFI_STATUS  Status;

  //
  // Allocate buffer for updating smbios table in memory
  //
  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    EFI_PAGE_SIZE,
                    (VOID **)&mSmbiosBufferPtr
                    );
  if (EFI_ERROR(Status))
    return Status;

  //
  // Allocate buffer for updating smbios table in flash
  //
  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    EFI_PAGE_SIZE,
                    (VOID **)&mSmbiosScratchPtr
                    );
  GetSmbiosDataFromFlash ();
  if (mSmbiosEfiEntry == NULL || mSmbiosLegacyEntry == NULL)
    GetSmbiosEntry ();

  return Status;
}

EFI_STATUS
SmiFlashEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
/*++

Routine Description:

  Entry point of the SMI flash DXE driver

Arguments:

  ImageHandle - ImageHandle
  SystemTable - Pointer to System Table

Returns:

  EFI_STATUS
--*/
{

  EFI_STATUS            Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL  *SwDispatch = NULL;
  EFI_SMM_SW_REGISTER_CONTEXT   SwContext;
  EFI_HANDLE                    Handle;

  //
  // Get EFI SMM Base Helper Protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmmBaseHelperReadyProtocolGuid,
                  NULL,
                  (VOID **)&mSmmBaseHelperReady
                  );

  if (!EFI_ERROR (Status)) {
    mFrameworkSmst = mSmmBaseHelperReady->FrameworkSmst;
  }

  //
  // Get SMM CPU protocol
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmCpuProtocolGuid,
                    NULL,
                    (VOID **)&mSmmCpu
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    &SwDispatch
                    );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
      return Status;
  }

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmNvMediaAccessProtocolGuid,
                    NULL,
                    &mMediaAccess
                    );
  if (EFI_ERROR (Status)) {
    Status = gBS->LocateProtocol (&gEfiSmmNvMediaAccessProtocolGuid, NULL, &mMediaAccess);
    ASSERT_EFI_ERROR (Status);
    if(EFI_ERROR(Status)) {
      return Status;
    }
  }

  SwContext.SwSmiInputValue = SW_SMI_FLASH_SERVICES;
  Status = SwDispatch->Register (
                         SwDispatch,
                         FlashInterface,
                         &SwContext,
                         &Handle
                         );
  ASSERT_EFI_ERROR (Status);

  mFDBaseAddress = PcdGet32(PcdFlashAreaBaseAddress);
  mFDSize        = PcdGet32(PcdFlashAreaSize);
  
  Status = AllocDataBuffer();
  ASSERT_EFI_ERROR (Status);

  return Status;
}
