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
#include <UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.h>







/**
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
SHELL_STATUS
EFIAPI
Shellcommandrun (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS      Status;
  LIST_ENTRY      *Package;
  CHAR16          *ProblemParam;
  // SHELL_STATUS    ShellStatus;

  Status=ShellCommandLineParse(EmptyParamList,&Package,&ProblemParam,TRUE);
  if(EFI_ERROR(Status))
  {
/*    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) 
    {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    }*/
    Print(L"ShellExecute - Pass - 1\n");
    Print(L"ShellExecute - Pass - by lexyu1\n");
  }
  else
  {
    // Print(L"The number of parameters is %d\n",ShellCommandLineGetCount(Package));
    Print(L"ShellExecute - Pass - 2\n");
    Print(L"ShellExecute - Pass - by lexyu2\n");
  }


  Print(L"ShellExecute - Pass - 3\n");
  Print(L"ShellExecute - Pass - by lexyu3\n");
  return EFI_SUCCESS;
}

