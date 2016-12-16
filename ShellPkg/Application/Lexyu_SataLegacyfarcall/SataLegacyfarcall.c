#include <Uefi.h>
#include <Uefi/UefiSpec.h>




#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>


#include <Protocol/LegacyBios.h>


#include <LegacyBiosInterface.h>




#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/SortLib.h>
#include <Library/HiiLib.h>
#include <Library/FileHandleLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/HandleParsingLib.h>
#include <Library/ReportStatusCodeLib.h>

#include <IndustryStandard/Atapi.h>



#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/PciIo.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellParameters.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/DriverDiagnostics2.h>
#include <Protocol/DriverDiagnostics.h>
#include <Protocol/PlatformDriverOverride.h>
#include <Protocol/BusSpecificDriverOverride.h>
#include <Protocol/PlatformToDriverConfiguration.h>
#include <Protocol/DriverSupportedEfiVersion.h>
#include <Protocol/DriverFamilyOverride.h>
#include <Protocol/DriverHealth.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/StorageSecurityCommand.h>




#include <Guid/GlobalVariable.h>
#include <Guid/ConsoleInDevice.h>
#include <Guid/ConsoleOutDevice.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/ShellLibHiiGuid.h>




#define EFI_LEGACY_BIOS_PROTOCOL_GUID \
  { \
    0xdb9a1e3d, 0x45cb, 0x4abb, {0x85, 0x3b, 0xe5, 0x38, 0x7f, 0xdb, 0x2e, 0x2d } \
  }

EFI_GUID  gEfiLegacyBiosProtocolGuid = EFI_LEGACY_BIOS_PROTOCOL_GUID;




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

//  EFI_STATUS  							           Status;
//  EFI_LEGACY_BIOS_PROTOCOL                         *LegacyBios;

//  EFI_IA32_REGISTER_SET                            Regs;
//  UINTN                                            CarryFlag;
//  UINT8                                            i;


  // EFI_DEVICE_PATH_PROTOCOL                         *FilePath; 
  // UINT8                                            *OptionalData;

  // FilePath->Type=5;
  // FilePath->SubType=1;
  // FilePath->Length[0]=36;
  // FilePath->Length[1]=0;
  // OptionalData=(UINT8 *)0xCDD2B118;

  // //
  // // See if the Legacy BIOS Protocol is available
  // //
  // Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **) &LegacyBios);
  


  // Status = LegacyBios->LegacyBoot (
  //                                LegacyBios,
  //                                (BBS_BBS_DEVICE_PATH *) FilePath,
  //                                 2,
  //                                 OptionalData
  //                                 );


   // Regs.H.AH = 0x00;
   // Regs.H.AL = 0x03;
   // Status    = LegacyBios->Int86(LegacyBios, 0x10, &Regs);  // Set the 80x25 Text VGA Mode

//      Regs.H.AH = 0x00;
//      Regs.H.DL = 128;
//      CarryFlag = LegacyBios->Int86 (LegacyBios, 0x13, &Regs);
//      Print(L"%d\n",CarryFlag);

  
  return EFI_SUCCESS;
}



// #include "Bds.h"
// #include "Language.h"
// #include "HwErrRecSupport.h"
// #include <Guid/ByoGlobalVariable.h>
// #include <Guid/SetupPassword.h>
// #include <Protocol/ByoFormSetManager.h>
// #include <SetupVariable.h>
// // #include <Library/BaseCryptLib.h>
// // #include <Library/LogoLib.h>
// // #include <Library/HobLib.h>
// #include <SetupVariable.h>




// EFI_STATUS
// EFIAPI
// UefiMain (
//   IN EFI_HANDLE        ImageHandle,
//   IN EFI_SYSTEM_TABLE  *SystemTable
//   )
// {
//     EFI_STATUS                          Status;
//     UINTN                         BootOptionCount;
//     EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;


//     BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);   // first run
//     Status=BootAllBootOptions(BootOptions, BootOptionCount);
//     return Status;
// }