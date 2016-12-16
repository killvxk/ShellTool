/*++
  This file contains an 'Intel Peripheral Driver' and is
  licensed for Intel CPUs and chipsets under the terms of your
  license agreement with Intel or your vendor.  This file may
  be modified by the user, subject to additional terms of the
  license agreement
--*/
/** @file

Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:
  
   PiSmmCommunicationSmm.c

Abstract:

  PiSmmCommunication SMM Driver.

**/

#include <PiSmm.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmReadyToLock.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/AcpiTable.h>
#include <Ppi/SmmCommunication.h>
#include <Guid/Acpi.h>
#include "PiSmmCommunicationPrivate.h"
#include <Pi/PiBootMode.h>      // HobLib.h +
#include <Pi/PiHob.h>           // HobLib.h +
#include <Library/HobLib.h>
#include <PlatformDefinition.h>


EFI_SMM_COMMUNICATION_CONTEXT  mSmmCommunicationContext = {
  SMM_COMMUNICATION_SIGNATURE
};

EFI_SMM_COMMUNICATION_ACPI_TABLE  mSmmCommunicationAcpiTable = {
  EFI_ACPI_4_0_UEFI_ACPI_DATA_TABLE_SIGNATURE,
  sizeof (EFI_SMM_COMMUNICATION_ACPI_TABLE),
  0x1,   // Revision
  0x0,   // Checksum
  {0x0}, // OemId[6]
  0x0,   // OemTableId
  0x0,   // OemRevision
  0x0,   // CreatorId
  0x0,   // CreatorRevision
  {0x0},                                                 // Identifier
  OFFSET_OF (EFI_SMM_COMMUNICATION_ACPI_TABLE, SwSmiNumber),  // DataOffset
  0x0,                                                   // SwSmiNumber
  0x0                                                    // BufferPtrAddress
};

/**
  Set SMM communication context.
**/
VOID
SetCommunicationContext (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gSmst->SmmInstallConfigurationTable (
                    gSmst,
                    &gEfiPeiSmmCommunicationPpiGuid,
                    &mSmmCommunicationContext,
                    sizeof(mSmmCommunicationContext)
                    );
  ASSERT_EFI_ERROR (Status);
}

/**
  Get SMM communication ACPI table address.

  @return SMM communication ACPI table address.
**/
EFI_PHYSICAL_ADDRESS
GetSmmCommunicationAcpiTableAddress (
  VOID
  )
{
  EFI_ACPI_4_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt;
  EFI_ACPI_DESCRIPTION_HEADER                   *Xsdt;
  UINTN                                         Index;
  UINT32                                        *TableAddress;
  UINT64                                        *XTableAddress;
  UINTN                                         TableCount;
  EFI_SMM_COMMUNICATION_ACPI_TABLE              *SmmAcpi;

  Rsdp = NULL;
  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpi20TableGuid) ||
        CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpi10TableGuid) ||
        CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpiTableGuid)  ) {
      //
      // A match was found.
      //
      Rsdp = (EFI_ACPI_4_0_ROOT_SYSTEM_DESCRIPTION_POINTER *) gST->ConfigurationTable[Index].VendorTable;
      break;
    }
  }

  ASSERT (Rsdp != NULL);
  if (Rsdp == NULL) {
    return 0;
  }

  Rsdt = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN) Rsdp->RsdtAddress;
  TableCount = (Rsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT32);
  TableAddress = (UINT32 *)(Rsdt + 1);
  for (Index = 0; Index < TableCount; Index++) {
    SmmAcpi = (EFI_SMM_COMMUNICATION_ACPI_TABLE *)(UINTN)TableAddress[Index];
    if ((SmmAcpi->UefiAcpiDataTable.Header.Signature == EFI_ACPI_4_0_UEFI_ACPI_DATA_TABLE_SIGNATURE) &&
        (SmmAcpi->UefiAcpiDataTable.Header.Length == sizeof (EFI_SMM_COMMUNICATION_ACPI_TABLE)) &&
        CompareGuid (&(SmmAcpi->UefiAcpiDataTable.Identifier), &gEfiSmmCommunicationProtocolGuid) ) {
      return (EFI_PHYSICAL_ADDRESS)(UINTN)SmmAcpi;
    }
  }

  if (Rsdp->Revision >= 2) {
    Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN) Rsdp->XsdtAddress;
    TableCount = (Rsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT64);
    XTableAddress = (UINT64 *)(Xsdt + 1);
    for (Index = 0; Index < TableCount; Index++) {
      SmmAcpi = (EFI_SMM_COMMUNICATION_ACPI_TABLE *)(UINTN)XTableAddress[Index];
      if ((SmmAcpi->UefiAcpiDataTable.Header.Signature == EFI_ACPI_4_0_UEFI_ACPI_DATA_TABLE_SIGNATURE) &&
          (SmmAcpi->UefiAcpiDataTable.Header.Length == sizeof (EFI_SMM_COMMUNICATION_ACPI_TABLE)) &&
          CompareGuid (&(SmmAcpi->UefiAcpiDataTable.Identifier), &gEfiSmmCommunicationProtocolGuid) ) {
        return (EFI_PHYSICAL_ADDRESS)(UINTN)SmmAcpi;
      }
    }
  }

  ASSERT(FALSE);
  return 0;
}

/**
  Dispatch function for a Software SMI handler.

  @param DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param Context         Points to an optional handler context which was specified when the
                         handler was registered.
  @param CommBuffer      A pointer to a collection of data in memory that will
                         be conveyed from a non-SMM environment into an SMM environment.
  @param CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS Command is handled successfully.

**/
EFI_STATUS
EFIAPI
PiSmmCommunicationHandler (
  IN EFI_HANDLE  DispatchHandle,
  IN CONST VOID  *Context         OPTIONAL,
  IN OUT VOID    *CommBuffer      OPTIONAL,
  IN OUT UINTN   *CommBufferSize  OPTIONAL
  )
{
  EFI_SMM_COMMUNICATION_ACPI_TABLE *SmmCommunicationAcpiTable;
  UINTN                            CommSize;
  EFI_STATUS                       Status;
  EFI_SMM_COMMUNICATE_HEADER       *CommunicateHeader;

  DEBUG ((EFI_D_INFO, "PiSmmCommunicationHandler Enter\n"));

  SmmCommunicationAcpiTable = (EFI_SMM_COMMUNICATION_ACPI_TABLE *)(UINTN)mSmmCommunicationContext.AcpiTableAddress;
  CommunicateHeader = (EFI_SMM_COMMUNICATE_HEADER *)(UINTN)SmmCommunicationAcpiTable->BufferPtrAddress;
  DEBUG ((EFI_D_INFO, "PiSmmCommunicationHandler CommunicateHeader - %x\n", CommunicateHeader));
  CommSize = (UINTN)CommunicateHeader->MessageLength;

  //
  // Call dispatch function
  //
  DEBUG ((EFI_D_INFO, "PiSmmCommunicationHandler Data - %x\n", &CommunicateHeader->Data[0]));
  Status = gSmst->SmiManage (
                    &CommunicateHeader->HeaderGuid,
                    NULL,
                    &CommunicateHeader->Data[0],
                    &CommSize
                    );
  DEBUG ((EFI_D_INFO, "PiSmmCommunicationHandler %r\n", Status));
  DEBUG ((EFI_D_INFO, "PiSmmCommunicationHandler Exit\n"));

  return Status;
}

/**
  Entry Point for PI SMM communication SMM driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCEESS     
  @return Others          Some error occurs.
**/
EFI_STATUS
EFIAPI
PiSmmCommunicationSmmEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL *SmmSwDispatch2;
  EFI_SMM_SW_REGISTER_CONTEXT   SmmSwDispatchContext;
  EFI_HANDLE                    DispatchHandle;
  EFI_ACPI_TABLE_PROTOCOL       *AcpiTableProtocol;
  UINTN                         TableKey;
  UINT64                        OemTableId;


  CopyMem (
    mSmmCommunicationAcpiTable.UefiAcpiDataTable.Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (mSmmCommunicationAcpiTable.UefiAcpiDataTable.Header.OemId)
    );
  OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  CopyMem (&mSmmCommunicationAcpiTable.UefiAcpiDataTable.Header.OemTableId, &OemTableId, sizeof (UINT64));
  mSmmCommunicationAcpiTable.UefiAcpiDataTable.Header.OemRevision      = PcdGet32 (PcdAcpiDefaultOemRevision);
  mSmmCommunicationAcpiTable.UefiAcpiDataTable.Header.CreatorId        = PcdGet32 (PcdAcpiDefaultCreatorId);
  mSmmCommunicationAcpiTable.UefiAcpiDataTable.Header.CreatorRevision  = PcdGet32 (PcdAcpiDefaultCreatorRevision);


  //
  // Register software SMI handler
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    (VOID **)&SmmSwDispatch2
                    );
  ASSERT_EFI_ERROR (Status);

  SmmSwDispatchContext.SwSmiInputValue = (UINTN)-1;
  Status = SmmSwDispatch2->Register (
                             SmmSwDispatch2,
                             PiSmmCommunicationHandler,
                             &SmmSwDispatchContext,
                             &DispatchHandle
                             );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((EFI_D_ERROR, "SmmCommunication SwSmi: %x\n", (UINTN)SmmSwDispatchContext.SwSmiInputValue));

  //
  // Set ACPI table
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **) &AcpiTableProtocol);
  ASSERT_EFI_ERROR (Status);

  mSmmCommunicationAcpiTable.SwSmiNumber = (UINT32)SmmSwDispatchContext.SwSmiInputValue;
  CopyMem (&mSmmCommunicationAcpiTable.UefiAcpiDataTable.Identifier, &gEfiSmmCommunicationProtocolGuid, sizeof(gEfiSmmCommunicationProtocolGuid));

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                &mSmmCommunicationAcpiTable,
                                sizeof(mSmmCommunicationAcpiTable),
                                &TableKey
                                );
  ASSERT_EFI_ERROR (Status);

  //
  // Save context
  //
  mSmmCommunicationContext.SwSmiNumber = (UINT32)SmmSwDispatchContext.SwSmiInputValue;
  mSmmCommunicationContext.AcpiTableAddress = GetSmmCommunicationAcpiTableAddress ();
  DEBUG ((EFI_D_INFO, "SmmCommunicationAcpiTable: %x\n", mSmmCommunicationContext.AcpiTableAddress));
  SetCommunicationContext ();

  
  return Status;
}
