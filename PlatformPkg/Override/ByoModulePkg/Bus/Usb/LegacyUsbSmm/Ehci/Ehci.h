/*++
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2009 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================
Module Name:
  Ehci.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _EHCI_H_
#define _EHCI_H_

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmServicesTableLib.h>
#include <IndustryStandard/pci22.h>
#include <Protocol/PciIo.h>
#include "../UsbLib/SmmPciIo.h"
#include "../Include/UsbHostController.h"

#define PCI_CLASSC_PI_EHCI  0x20
#define BIT(a)  (1 << (a))

typedef struct _USB2_HC_DEV     USB2_HC_DEV;

#include "EhciReg.h"
#include "EhciUrb.h"
#include "EhciSched.h"
#include "EhciDebug.h"

BOOLEAN             mEhciOSRequestOwnership;
LIST_ENTRY          mEhcList;

//
// EHCI debug port control status register bit definition
//
#define USB_DEBUG_PORT_IN_USE        BIT10
#define USB_DEBUG_PORT_ENABLE        BIT28
#define USB_DEBUG_PORT_OWNER         BIT30

#endif
