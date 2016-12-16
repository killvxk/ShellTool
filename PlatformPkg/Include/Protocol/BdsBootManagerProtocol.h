
#ifndef __BDS_BOOT_MANAGER_PROTOCOL_H__
#define __BDS_BOOT_MANAGER_PROTOCOL_H__

#include <UefiBootManagerLoadOption.h>

typedef struct _EFI_BDS_BOOT_MANAGER_PROTOCOL EFI_BDS_BOOT_MANAGER_PROTOCOL;


typedef  
VOID
(EFIAPI *EFI_BOOT_MANAGER_CONNECT_ALL) (
    VOID
  );

typedef  
VOID
(EFIAPI *EFI_BOOT_MANAGER_REFRESH_ALL_BOOT_OPTION) (
    VOID
  );  

typedef   
EFI_STATUS
(EFIAPI *EFI_BOOT_MANAGER_GET_LOAD_OPTIONS) (
  OUT EFI_BOOT_MANAGER_LOAD_OPTION   **Option,
  OUT UINTN                          *OptionCount
  );

typedef   
EFI_STATUS
(EFIAPI *EFI_BOOT_MANAGER_FREE_LOAD_OPTIONS) (
  IN EFI_BOOT_MANAGER_LOAD_OPTION   *Option,
  IN UINTN                          OptionCount
  );
  
  
  
typedef  
EFI_STATUS
(EFIAPI *EFI_BOOT_MANAGER_CREATE_FV_BOOT_OPTION) (
  EFI_GUID                       *FileGuid,
  CHAR16                         *Description,
  EFI_BOOT_MANAGER_LOAD_OPTION   *BootOption,
  BOOLEAN                        IsBootCategory,
  UINT8                          *OptionalData,    OPTIONAL
  UINT32                         OptionalDataSize       
  );


typedef
EFI_STATUS
(EFIAPI *EFI_BOOT_MANAGER_CREATE_SETUP_BOOT_OPTION) (
  OUT EFI_BOOT_MANAGER_LOAD_OPTION   *SetupOption
);


typedef  
VOID
(EFIAPI *EFI_BOOT_MANAGER_BOOT)(
  IN  EFI_BOOT_MANAGER_LOAD_OPTION   *BootOption
  );



struct _EFI_BDS_BOOT_MANAGER_PROTOCOL {
  EFI_BOOT_MANAGER_CONNECT_ALL              ConnectAll;
  EFI_BOOT_MANAGER_REFRESH_ALL_BOOT_OPTION  RefreshOption;
  EFI_BOOT_MANAGER_GET_LOAD_OPTIONS         GetOption;
  EFI_BOOT_MANAGER_FREE_LOAD_OPTIONS        FreeOption;
  EFI_BOOT_MANAGER_CREATE_FV_BOOT_OPTION    CreateFvOption;
  EFI_BOOT_MANAGER_CREATE_SETUP_BOOT_OPTION CreateSetupOption;
  EFI_BOOT_MANAGER_BOOT                     Boot;
};  
  
extern EFI_GUID gEfiBootManagerProtocolGuid;

#endif


