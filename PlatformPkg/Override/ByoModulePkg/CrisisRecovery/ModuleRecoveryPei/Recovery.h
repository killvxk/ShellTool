#ifndef __RECOVERY_H__
#define __RECOVERY_H__

#include <Library\BiosIdLib.h>

#define MAX_CAPUSLE_NUMBER 8

typedef struct {
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  UINT64                  Length;
  BIOS_ID_IMAGE           BiosIdImage;
  EFI_GUID                DeviceId;
  BOOLEAN                 Actived;
} CAPSULE_INFO;

typedef struct {
  CAPSULE_INFO        CapsuleInfo[MAX_CAPUSLE_NUMBER];
  UINT8               CapsuleCount;
} CAPSULE_RECORD;

extern EFI_GUID gRecoveryCapsuleRecordGuid;

#endif
