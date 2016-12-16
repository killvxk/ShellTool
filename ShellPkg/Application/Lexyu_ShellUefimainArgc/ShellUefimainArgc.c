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
#include "UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-n", TypeValue},
  {L"-number", TypeValue},
  {L"-h", TypeFlag},
  {L"-help", TypeFlag},
  {NULL, TypeMax}
  };

// STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
//   {L"-p", TypeValue},
//   {L"-d", TypeFlag},
//   {L"-v", TypeFlag},
//   {L"-verbose", TypeFlag},
//   {L"-sfo", TypeFlag},
//   {L"-l", TypeValue},
//   {NULL, TypeMax}
//   };



// EFI_HANDLE gShellDebug1HiiHandle = NULL;
/**
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;

  // CONST CHAR16        *Temp;
  CONST CHAR16        *Temp2;
  CONST CHAR16        *Temp3;
  // CONST CHAR16        *Data;

  // EFI_GUID      *Guid;
  // CONST CHAR16  *VariableName;
  // SHELL_STATUS        ShellStatus;
  // UINTN               ParamCount;


  // EFI_SHELL_INTERFACE           *mEfiShellInterface;
  // EFI_SHELL_PROTOCOL            *gEfiShellProtocol;


  // Status = gBS->OpenProtocol(ImageHandle,
  //                              &gEfiShellInterfaceGuid,
  //                              (VOID **)&mEfiShellInterface,
  //                              ImageHandle,
  //                              NULL,
  //                              EFI_OPEN_PROTOCOL_GET_PROTOCOL
  //                             );



  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);


  Status=ShellCommandLineParse(ParamList,&Package,&ProblemParam,TRUE);
  if(EFI_ERROR(Status))
  {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) 
    {
      // ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, ProblemParam);
      // FreePool(ProblemParam);
      // ShellStatus = SHELL_INVALID_PARAMETER;
    	Print(L"Wrong Parameters! Please check your input!\n");
    }
  }
  else
  {
    Print(L"The number of parameters is %d\n",ShellCommandLineGetCount(Package));
  	if (ShellCommandLineGetCount(Package) != 1 )
  	{
      // ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle);
      // ShellStatus = SHELL_INVALID_PARAMETER;
  		Print(L"Wrong Parameters! Please check your input!\n");
    }
    else
    {
    	// Print(L"The number of parameters is %d\n",ShellCommandLineGetCount(Package));
      Temp3=ShellCommandLineGetRawValue(Package, 0);
    	if (ShellCommandLineGetFlag(Package, L"-h") || ShellCommandLineGetFlag(Package, L"-help")) 
    	{
	    	Print(L"%s -h/-help\n%s  -n  num\n",Temp3,Temp3);
        return EFI_SUCCESS;
		  }

	    // Temp = ShellCommandLineGetRawValue(Package, 1);
	    // Data = ShellCommandLineGetRawValue(Package, 2);
	    // Print(L"%s\n",Temp);
	    // Print(L"%s\n",Data);
      Temp2=ShellCommandLineGetValue(Package, L"-n");
      if(Temp2)
      // if(Temp2 != NULL)
      {
        Print(L"%s\n",Temp2);
      }
      else
      {
        Print(L"%s -h/-help\n%s  -n  num\n",Temp3,Temp3);
      }
	  }
  }
  return EFI_SUCCESS;
}
