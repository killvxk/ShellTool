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
  Uhci.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#ifndef _UHCI_H
#define _UHCI_H

#include <PiSmm.h>
#include <IndustryStandard/pci22.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/PciIo.h>

#include "../Include/UsbHostController.h"
#include "../UsbLib/SmmPciIo.h"
#include "../UsbVirtualKbc/UsbVirtualKbc.h"

typedef struct _USB_HC_DEV     USB_HC_DEV;

#include "UhciQueue.h"
#include "UhciReg.h"
#include "UhciSched.h"
#include "UhciDebug.h"
BOOLEAN                 mUhciOSRequestOwnership;
LIST_ENTRY              mUhcList;

#endif
