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




#include <Uefi.h>
#include <ShellBase.h>

#include <Guid/GlobalVariable.h>
#include <Guid/ConsoleInDevice.h>
#include <Guid/ConsoleOutDevice.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/ShellLibHiiGuid.h>

#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellParameters.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DriverDiagnostics2.h>
#include <Protocol/DriverDiagnostics.h>
#include <Protocol/PlatformDriverOverride.h>
#include <Protocol/BusSpecificDriverOverride.h>
#include <Protocol/PlatformToDriverConfiguration.h>
#include <Protocol/DriverSupportedEfiVersion.h>
#include <Protocol/DriverFamilyOverride.h>
#include <Protocol/DriverHealth.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/PciRootBridgeIo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/SortLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/FileHandleLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/HandleParsingLib.h>




SHELL_STATUS
EFIAPI
Shellcommandrun (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );





extern EFI_HANDLE gTestHandle;


STATIC CONST CHAR16 mFileName[] = L"ShellCommands-help";
EFI_HANDLE gTestHandle = NULL;






/**
  Gets the debug file name.  This will be used if HII is not working.

  @retval NULL    No file is available.
  @return         The NULL-terminated filename to get help from.
**/
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameSata (
  VOID
  )
{
  return (mFileName);
}





/**
  Constructor for the test Shell Commands library.

  @param ImageHandle    the image handle of the process
  @param SystemTable    the EFI System Table pointer

  @retval EFI_SUCCESS        the shell command handlers were installed sucessfully
  @retval EFI_UNSUPPORTED    the shell level required was not found.
**/
EFI_STATUS
EFIAPI
Test_CommandLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // install the HII stuff.
  //


  /**
  hII: human interface infrastructure
  Registers a list of packages in the HII Database and returns the HII Handle
  associated with that registration.  If an HII Handle has already been registered
  with the same PackageListGuid and DeviceHandle, then NULL is returned.  If there
  are not enough resources to perform the registration, then NULL is returned.
  If an empty list of packages is passed in, then NULL is returned.  If the size of
  the list of package is 0, then NULL is returned.

  The variable arguments are pointers which point to package header that defined 
  by UEFI VFR compiler and StringGather tool.

  #pragma pack (push, 1)
  typedef struct {
    UINT32                  BinaryLength;
    EFI_HII_PACKAGE_HEADER  PackageHeader;
  } EDKII_AUTOGEN_PACKAGES_HEADER;
  #pragma pack (pop)
  
  @param[in]  PackageListGuid  The GUID of the package list.
  @param[in]  DeviceHandle     If not NULL, the Device Handle on which 
                               an instance of DEVICE_PATH_PROTOCOL is installed.
                               This Device Handle uniquely defines the device that 
                               the added packages are associated with.
  @param[in]  ...              The variable argument list that contains pointers 
                               to packages terminated by a NULL.

  @retval NULL   A HII Handle has already been registered in the HII Database with
                 the same PackageListGuid and DeviceHandle.
  @retval NULL   The HII Handle could not be created.
  @retval NULL   An empty list of packages was passed in.
  @retval NULL   All packages are empty.
  @retval Other  The HII Handle associated with the newly registered package list.

**/


  gTestHandle = HiiAddPackages (&gTestGuid, gImageHandle, test_commandStrings, NULL);
  if (gTestHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }

  Print(L"registercommand - Constructor - Pass1\n");

  //
  // install our shell command handlers that are always installed
  //
  
  /**
  Registers handlers of type SHELL_RUN_COMMAND and
  SHELL_GET_MAN_FILENAME for each shell command.

  If the ShellSupportLevel is greater than the value of the
  PcdShellSupportLevel then return RETURN_UNSUPPORTED.

  Registers the handlers specified by GetHelpInfoHandler and CommandHandler
  with the command specified by CommandString. If the command named by
  CommandString has already been registered, then return
  RETURN_ALREADY_STARTED.

  If there are not enough resources available to register the handlers then
  RETURN_OUT_OF_RESOURCES is returned.

  If CommandString is NULL, then ASSERT().
  If GetHelpInfoHandler is NULL, then ASSERT().
  If CommandHandler is NULL, then ASSERT().
  If ProfileName is NULL, then ASSERT().

  @param[in]  CommandString         Pointer to the command name.  This is the
                                    name to look for on the command line in
                                    the shell.
  @param[in]  CommandHandler        Pointer to a function that runs the
                                    specified command.
  @param[in]  GetManFileName        Pointer to a function that provides man
                                    filename.
  @param[in]  ShellMinSupportLevel  minimum Shell Support Level which has this
                                    function.
  @param[in]  ProfileName           profile name to require for support of this
                                    function.
  @param[in]  CanAffectLE           indicates whether this command's return value
                                    can change the LASTERROR environment variable.
  @param[in]  HiiHandle             Handle of this command's HII entry.
  @param[in]  ManFormatHelp         HII locator for the help text.

  @retval  RETURN_SUCCESS           The handlers were registered.
  @retval  RETURN_OUT_OF_RESOURCES  There are not enough resources available to
                                    register the shell command.
  @retval RETURN_UNSUPPORTED        the ShellMinSupportLevel was higher than the
                                    currently allowed support level.
  @retval RETURN_ALREADY_STARTED    The CommandString represents a command that
                                    is already registered.  Only 1 handler set for
                                    a given command is allowed.
  @sa SHELL_GET_MAN_FILENAME
  @sa SHELL_RUN_COMMAND
**/



  ShellCommandRegisterCommandName(L"command_test",Shellcommandrun, ShellCommandGetManFileNameSata, 0, L"Sata", TRUE, gTestHandle, STRING_TOKEN(STR_GET_HELP));
    
  Print(L"registercommand - Constructor - Pass2\n");
  return (EFI_SUCCESS);
}






/**
  Destructor for the library.  free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
Test_CommandsLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gTestHandle != NULL) {
    HiiRemovePackages(gTestHandle);
  }
  Print(L"registercommand - destructor - Pass\n");
  return (EFI_SUCCESS);
}
