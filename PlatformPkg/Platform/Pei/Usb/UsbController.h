/*++

Module Name:
  
  MultiUsbController.h

Abstract:

  Header file for the EHCI PPI

--*/
#ifndef _PEI_USB_CONTROLLER_H_
#define _PEI_USB_CONTROLLER_H_

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Ppi/UsbController.h>
#include <Ppi/ReadOnlyVariable2.h>

#define USB_CONTROLLER_MAX 7

#define PEI_USB_CONTROLLER_SIGNATURE  SIGNATURE_32 ('M', 'U', 'S', 'B')

typedef struct {
  UINTN                       Signature;
  PEI_USB_CONTROLLER_PPI      UsbControllerPpi;
  EFI_PEI_PPI_DESCRIPTOR      PpiList;
  UINTN                       EhciControllersNum;
  UINTN                       XhciControllersNum;
  UINTN                       UhciControllersNum;
  UINTN                       OhciControllersNum;
  UINTN                       TotalUsbControllers;
  UINTN                       MemBase[USB_CONTROLLER_MAX];
} USB_CONTROLLER_INFO;

#define USB_CONTROLLER_INFO_FROM_THIS(a)  CR (a, USB_CONTROLLER_INFO, UsbControllerPpi, PEI_USB_CONTROLLER_SIGNATURE)

EFI_STATUS
InitUsbControl (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  USB_CONTROLLER_INFO           *PeiUsbControllerInfo
  )
;

EFI_STATUS
HandleXhciFwForRecovery();

#endif
