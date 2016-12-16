
#ifndef _FRONT_PAGE_H_
#define _FRONT_PAGE_H_

#include <PiDxe.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/GlobalVariable.h>
#include <Guid/LegacyDevorder.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Protocol/SetupSaveNotify.h>
#include <Protocol/ByoFormSetManager.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/DevicePath.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/ByoFormBrowserEx.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>




#pragma pack(1)

typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

typedef struct {
  UINT16     BbsIndex;
} LEGACY_BOOT_OPTION_BBS_DATA;
#pragma pack()



extern UINT8  FrontPageVfrBin[];
extern EFI_FORM_BROWSER2_PROTOCOL              *gFormBrowser2;
extern EFI_BYO_FORM_BROWSER_EXTENSION_PROTOCOL *gFormBrowserEx;
extern EFI_GUID gEfiSetupEnterGuid;
extern EFI_GUID gByoGlobalVariableGuid;

  
CHAR16 *
UiDevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  );  

EFI_STATUS
BdsStartBootMaint (
  VOID
  );

VOID
ByoFreeBMPackage (
  VOID
  );
  
#endif // _FRONT_PAGE_H_

