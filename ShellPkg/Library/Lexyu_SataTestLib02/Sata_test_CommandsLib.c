/** @file
  Main file for NULL named library for debug1 profile shell command functions.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Sata_test_CommandsLib.h"





STATIC CONST CHAR16 mFileName[] = L"ShellCommands";
EFI_HANDLE gShellSataHiiHandle = NULL;

/**
  Gets the debug file name.  This will be used if HII is not working.

  @retval NULL    No file is available.
  @return         The NULL-terminated filename to get help from.
**/
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameSatamike (
  VOID
  )
{
  return (mFileName);
}




/**
  Constructor for the Shell Debug1 Commands library.

  @param ImageHandle    the image handle of the process
  @param SystemTable    the EFI System Table pointer

  @retval EFI_SUCCESS        the shell command handlers were installed sucessfully
  @retval EFI_UNSUPPORTED    the shell level required was not found.
**/
EFI_STATUS
EFIAPI
Sata_test_CommandLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // install the HII stuff.
  //
  gShellSataHiiHandle = HiiAddPackages (&gShellSataHiiGuid, gImageHandle, Sata_test_CommandsLibStrings, NULL);
  if (gShellSataHiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }

  //
  // install our shell command handlers that are always installed
  //
  ShellCommandRegisterCommandName(L"sata_test",     ShellCommandRunSata               , ShellCommandGetManFileNameSatamike, 0, L"Sata", TRUE, gShellSataHiiHandle, STRING_TOKEN(STR_GET_HELP_SATA)         );

  return (EFI_SUCCESS);
}





/**
  Destructor for the library.  free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
Sata_test_CommandsLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gShellSataHiiHandle != NULL) {
    HiiRemovePackages(gShellSataHiiHandle);
  }
  return (EFI_SUCCESS);
}
