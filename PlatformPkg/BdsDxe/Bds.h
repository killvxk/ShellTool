
#ifndef _BDS_MODULE_H_
#define _BDS_MODULE_H_

#include <Uefi.h>
#include <Guid/GlobalVariable.h>
#include <Protocol/Bds.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/LoadedImage.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/PerformanceLib.h>

#include <Library/UefiBootManagerLib.h>
#include "PlatformBootManagerLib.h"

//
// Prototypes
//

/**

  Service routine for BdsInstance->Entry(). Devices are connected, the
  consoles are initialized, and the boot options are tried.

  @param This            Protocol Instance structure.

**/
VOID
EFIAPI
BdsEntry (
  IN  EFI_BDS_ARCH_PROTOCOL *This
  );

typedef
VOID
(* BDS_PROCESS_VARIABLE) (
  VOID  **Variable,
  UINTN *VariableSize
  );

#endif
