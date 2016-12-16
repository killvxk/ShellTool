/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  SmiVariableSmm.c

Abstract:

 Provides Access to EFI Variable Services through SMI
--*/

#include <SmiVariable.h>
#include <Framework/SmmCis.h>
#include <Protocol/SmmBaseHelperReady.h>

EFI_SMM_CPU_PROTOCOL                  *mSmmCpu;

EFI_SMM_VARIABLE_PROTOCOL             *mSmmVariable;
EFI_SMM_SYSTEM_TABLE                  *mFrameworkSmst;
EFI_SMM_BASE_HELPER_READY_PROTOCOL    *mSmmBaseHelperReady;

EFI_STATUS
VariableInterfaceInit (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
)
{
  UINTN       VariableSize;
  EFI_STATUS  Status;

  VariableSize = sizeof (mHiiDbAddress);
  Status = mSmmVariable->SmmGetVariable (
                           mEfiExportDatabaseName,
                           &mEfiExportDatabaseGuid,
                           NULL,
                           &VariableSize,
                           &mHiiDbAddress
                           );
  DEBUG ((DEBUG_INFO, " SmiVariableInterfaceInit(): HII Database exported at 0x%X\n", mHiiDbAddress));
	return Status;
}

EFI_STATUS
VariableInterface (
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

  VOID

--*/
{
  EFI_STATUS                 Status;
  UINTN                      Index;
  UINTN                      CpuIndex;
  VARIABLE_BLOCK             *VariableBlock;
  GET_NEXT_VARIABLE_BLOCK    *GetNextVariableBlock;
  COPY_MEMORY_BLOCK          *CopyMemoryBlock;
  UINT8                      SubFunction;
  UINTN                      VariableSize;
  VOID                       *HiiDbAddress;
  EFI_SMM_SAVE_STATE_IO_INFO IoState;
  UINT32                     RegEax;
  UINT32                     RegEbx;
  UINT32                     RegEdx;
  UINT32                     RegEflags;
  EFI_SMI_CPU_SAVE_STATE     *SaveState;
  CHECK_BIOS_PASSWORD_BLOCK  *CheckBiosPsw;

  VariableBlock         = NULL;
  GetNextVariableBlock  = NULL;
  CopyMemoryBlock       = NULL;

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
    if (!EFI_ERROR (Status) && (IoState.IoData == SW_SMI_VARIABLE_SERVICES)) {
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
                      EFI_SMM_SAVE_STATE_REGISTER_RDX,
                      CpuIndex,
                      &RegEdx
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
  // Extract the subfunction number from AH,
  // and process the requested subfunction.
  //
  SubFunction = (UINT8) ((RegEax & 0xFF00) >> 8);

  switch (SubFunction) {
    case GET_VARIABLE_SUBFUNCTION:
      //
      // EBX = address of the VARIABLE_BLOCK parameter block
      //
      VariableBlock = (VARIABLE_BLOCK *) (UINTN) RegEbx;
      Status = mSmmVariable->SmmGetVariable (
                               (CHAR16 *) (UINTN) VariableBlock->VariableName,
                               &VariableBlock->VendorGuid,
                               &VariableBlock->Attributes,
                               (UINTN *) &VariableBlock->DataSize,
                               (VOID *) (UINTN) VariableBlock->Data
                               );
      break;

    case GET_NEXT_VARIABLE_SUBFUNCTION:
      //
      // EBX = address of the GET_NEXT_VARIABLE_BLOCK parameter block
      //
      GetNextVariableBlock = (GET_NEXT_VARIABLE_BLOCK *) (UINTN) RegEbx;
      Status = mSmmVariable->SmmGetNextVariableName (
                               (UINTN *) &GetNextVariableBlock->VariableNameSize,
                               (CHAR16 *) (UINTN) GetNextVariableBlock->VariableName,
                               &GetNextVariableBlock->VendorGuid
                               );
      break;

    case SET_VARIABLE_SUBFUNCTION:
      //
      // EBX = address of the VARIABLE_BLOCK parameter block
      //
      VariableBlock = (VARIABLE_BLOCK *) (UINTN) RegEbx;
      Status = mSmmVariable->SmmSetVariable (
                               (CHAR16 *) (UINTN) VariableBlock->VariableName,
                               &VariableBlock->VendorGuid,
                               VariableBlock->Attributes,
                               (UINTN) VariableBlock->DataSize,
                               (VOID *) (UINTN) VariableBlock->Data
                               );
      break;

    case EXPORT_HII_DB_SUBFUNCTION:
      //
      // Read the HII Database address from the variable.
      //
      if (mHiiDbAddress == NULL) {
        VariableSize = sizeof (HiiDbAddress);
        Status = mSmmVariable->SmmGetVariable (
                                 mEfiExportDatabaseName,
                                 &mEfiExportDatabaseGuid,
                                 NULL,
                                 &VariableSize,
                                 &HiiDbAddress
                                 );
      } else {
        HiiDbAddress = mHiiDbAddress;
        Status = EFI_SUCCESS;
      }
      //
      // Return the HII Database address in DX:BX
      //
      RegEdx &= 0xFFFF0000;
      RegEdx |= (((UINT32) (UINTN) HiiDbAddress) >> 16) & 0xFFFF;
      RegEbx &= 0xFFFF0000;
      RegEbx |= ((UINT32) (UINTN) HiiDbAddress) & 0xFFFF;

      break;

    case COPY_MEMORY_SUBFUNCTION:
      //
      // EBX = address of the COPY_MEMORY_BLOCK parameter block
      //
      CopyMemoryBlock = (COPY_MEMORY_BLOCK *) (UINTN) RegEbx;

      CopyMem (
        (VOID *) (UINTN) CopyMemoryBlock->Destination,
        (VOID *) (UINTN) CopyMemoryBlock->Source,
        CopyMemoryBlock->Size
        );

      Status = EFI_SUCCESS;
      break;

    case CHECK_BIOS_PASSWORD_SUBFUNCTION:
    {
      SETUP_DATA  SetupData;
      UINTN       VarSize;
      TSESETUP    Tse;

      VarSize = sizeof (TSESETUP);
      Status = mSmmVariable->SmmGetVariable (
                      L"TSESetup",
                      &gSetupPasswordVariableTseGuid,
                      NULL,
                      &VarSize,
                      &Tse
                      );
      ASSERT(!EFI_ERROR(Status));
      CheckBiosPsw = (CHECK_BIOS_PASSWORD_BLOCK *) (UINTN) RegEbx;
      switch(CheckBiosPsw->SubFun) {
        case REQUIRE_PASSWORD_ITEM:
          VarSize = sizeof (SETUP_DATA);
          Status = mSmmVariable->SmmGetVariable (
                          L"Setup",
                          &gPlatformSetupVariableGuid,
                          NULL,
                          &VarSize,
                          &SetupData
                          );
          ASSERT(!EFI_ERROR(Status));
          CheckBiosPsw->RequireAdminPsw = 0;
          break;
/*					
        case CHECK_BIOS_PASSWORD:
          DEBUG((EFI_D_ERROR, "Tse.AdminPassword:%s\n",Tse.AdminPassword));
          DEBUG((EFI_D_ERROR, "Password:%s\n",CheckBiosPsw->Password));
          if (StrCmp(CheckBiosPsw->Password, Tse.AdminPassword)) {
            Status = RETURN_INVALID_PARAMETER;
          }
          DEBUG((EFI_D_ERROR, "Status:%r\n",Status));
          break;
*/          
        default:
          Status = RETURN_INVALID_PARAMETER;
          break;
      }
      break;
    }
    default:
      Status = RETURN_UNSUPPORTED;
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
SmiVariableSmmEntryPoint (
	IN EFI_HANDLE       ImageHandle,
	IN EFI_SYSTEM_TABLE *SystemTable
	)
/*++

Routine Description:

  Entry point of the SMI variable DXE driver

Arguments:

  ImageHandle - ImageHandle
  SystemTable - Pointer to System Table

Returns:

  EFI_STATUS
--*/
{
	EFI_STATUS                     Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL  *SwDispatch = NULL;
  EFI_SMM_SW_REGISTER_CONTEXT    SwContext;
  EFI_HANDLE                     Handle;

  //
  // Get EFI SMM Base Helper Protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmmBaseHelperReadyProtocolGuid,
                  NULL,
                  (VOID **)&mSmmBaseHelperReady
                  );
  DEBUG ((EFI_D_ERROR, "Locate gEfiSmmBaseHelperReadyProtocolGuid status = %r\n", Status));
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

  Status = gSmst->SmmLocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, &mSmmVariable);
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmLocateProtocol (&gEfiSmmSwDispatch2ProtocolGuid, NULL, &SwDispatch);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SwContext.SwSmiInputValue = SW_SMI_VARIABLE_SERVICES;
  Status = SwDispatch->Register (SwDispatch, VariableInterface, &SwContext, &Handle);
  ASSERT_EFI_ERROR (Status);

  SwContext.SwSmiInputValue = SW_SMI_VARIABLE_SERVICES_INIT;
  Status = SwDispatch->Register (SwDispatch, VariableInterfaceInit, &SwContext, &Handle);
  ASSERT_EFI_ERROR (Status);

	return Status;
}
