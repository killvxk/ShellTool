/*++
  This file contains an 'Intel Pre-EFI Module' and is licensed
  for Intel CPUs and Chipsets under the terms of your license
  agreement with Intel or your vendor.  This file may be
  modified by the user, subject to additional terms of the
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
  
   PiSmmCommunicationPei.c

Abstract:

  PiSmmCommunication PEI Driver.

**/

#include <PiPei.h>
#include <PiDxe.h>
#include <PiSmm.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Protocol/SmmCommunication.h>
#include <Ppi/SmmCommunication.h>
#include <Ppi/SmmAccess.h>
#include <Ppi/SmmControl.h>
#include <Guid/AcpiS3Context.h>
#include "PiSmmCommunicationPrivate.h"
#include <Library/PlatformCommLib.h>

/**
  the whole picture is below:

  +----------------------------------+
  | ACPI_VARIABLE_HOB                |
  |   SmramDescriptor                | <- DRAM
  |     CpuStart                     |
  +----------------------------------+---
                                     |
  +----------------------------------+<--
  | SMM_S3_RESUME_STATE              |
  |   Signature                      | <- SMRAM
  |   Smst                           |
  +----------------------------------+---
                                     |
  +----------------------------------+<--
  | EFI_SMM_SYSTEM_TABLE2            |
  |   NumberOfTableEntries           | <- SMRAM
  |   SmmConfigurationTable          |
  +----------------------------------+---
                                     |
  +----------------------------------+<--
  | EFI_SMM_COMMUNICATION_CONTEXT    |
  |   SwSmiNumber                    | <- SMRAM
  |   AcpiTableAddress               |
  +----------------------------------+---
                                     |
  +----------------------------------+<--
  | EFI_SMM_COMMUNICATION_ACPI_TABLE |
  |   SwSmiNumber                    | <- AcpiNvs
  |   BufferPtrAddress               |
  +----------------------------------+---
                                     |
  +----------------------------------+<--
  | EFI_SMM_COMMUNICATE_HEADER       |
  |   HeaderGuid                     | <- DRAM
  |   MessageLength                  |
  +----------------------------------+

**/


/**
  Communicates with a registered handler.
  
  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_SMM_COMMUNICATION_PPI instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into SMRAM.
  @param[in, out] CommSize       The size of the data buffer being passed in.On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_NOT_STARTED        The service is NOT started.
**/
EFI_STATUS
EFIAPI
Communicate (
  IN CONST EFI_PEI_SMM_COMMUNICATION_PPI   *This,
  IN OUT VOID                              *CommBuffer,
  IN OUT UINTN                             *CommSize
  );

EFI_PEI_SMM_COMMUNICATION_PPI      mSmmCommunicationPpi = { Communicate };

EFI_PEI_PPI_DESCRIPTOR mPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiSmmCommunicationPpiGuid,
  &mSmmCommunicationPpi
};




/**
  Communicates with a registered handler.
  
  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_SMM_COMMUNICATION_PPI instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into SMRAM.
  @param[in, out] CommSize       The size of the data buffer being passed in.On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_NOT_STARTED        The service is NOT started.
**/
EFI_STATUS
EFIAPI
Communicate (
  IN CONST EFI_PEI_SMM_COMMUNICATION_PPI   *This,
  IN OUT VOID                              *CommBuffer,
  IN OUT UINTN                             *CommSize
  )
{
  EFI_STATUS                       Status;
  PEI_SMM_CONTROL_PPI              *SmmControl;
  UINT8                            SmiCommand;
  UINTN                            Size;
  EFI_SMM_COMMUNICATION_ACPI_TABLE *SmmCommunicationAcpiTable;


//DEBUG ((EFI_D_ERROR, "SmmComm\n"));

  if (CommBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get needed resource
  //
  Status = PeiServicesLocatePpi (
             &gPeiSmmControlPpiGuid,
             0,
             NULL,
             (VOID **)&SmmControl
             );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_STARTED;
  }

  SmmCommunicationAcpiTable = (EFI_SMM_COMMUNICATION_ACPI_TABLE*)GetAcpiTableScat();
  SmmCommunicationAcpiTable->BufferPtrAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)CommBuffer;
  SmiCommand = (UINT8)SmmCommunicationAcpiTable->SwSmiNumber;
  Size = sizeof(SmiCommand);
  Status = SmmControl->Trigger (
                         (EFI_PEI_SERVICES **)GetPeiServicesTablePointer(),
                         SmmControl,
                         (INT8 *)&SmiCommand,
                         &Size,
                         FALSE,
                         0
                         );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Entry Point for PI SMM communication PEIM.

  @param  FileHandle              Handle of the file being invoked.
  @param  PeiServices             Pointer to PEI Services table.

  @retval EFI_SUCEESS     
  @return Others          Some error occurs.
**/
EFI_STATUS
EFIAPI
PiSmmCommunicationPeiEntryPoint (
  IN EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  EFI_STATUS                      Status;
  PEI_SMM_ACCESS_PPI              *SmmAccess;
  EFI_BOOT_MODE                   BootMode;
  UINTN                           Index;

  BootMode = GetBootModeHob();
  if (BootMode != BOOT_ON_S3_RESUME) {
    return EFI_UNSUPPORTED;
  }

  Status = PeiServicesLocatePpi (
             &gPeiSmmAccessPpiGuid,
             0,
             NULL,
             (VOID **)&SmmAccess
             );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "SmmAccessPpi Not Found\n"));
    return EFI_NOT_STARTED;
  }
  if (SmmAccess->LockState) {
    DEBUG((EFI_D_INFO, "SMRAM Locked\n"));
  } else {
    for (Index = 0; !EFI_ERROR (Status); Index++) {
      Status = SmmAccess->Open((EFI_PEI_SERVICES **)GetPeiServicesTablePointer(), SmmAccess, Index);
    }  
  }

  PeiServicesInstallPpi(&mPpiList);
  return RETURN_SUCCESS;
}
