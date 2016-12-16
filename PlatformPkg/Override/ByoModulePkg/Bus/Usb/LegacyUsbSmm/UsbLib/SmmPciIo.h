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
  SmmPciIo.h

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/


#ifndef _SMM_PCI_IO_UTILITY_H
#define _SMM_PCI_IO_UTILITY_H

#include <PiSmm.h>

#include <Protocol/SmmCpuIo2.h>
#include <IndustryStandard/Pci22.h>
//
// Utility consumed protocols
//

#include <Protocol/Smmbase2.h>

#pragma pack(1)
typedef struct {
    UINT16   Function:3;
    UINT16   Device:5;
    UINT16   Bus:8;
} EFI_PCI_FUNCTION_ADDRESS;

#pragma pack()

//
// Pci I/O related data structure deifinition
//
typedef enum {
    SmmPciWidthUint8  = 0,
    SmmPciWidthUint16 = 1,
    SmmPciWidthUint32 = 2,
    SmmPciWidthUint64 = 3,
    SmmPciWidthMaximum
} SMM_PCI_IO_WIDTH;

#define SMM_PCI_ADDRESS(bus, dev, func, reg) \
    ((UINT64) ((((UINTN) bus) << 24) + (((UINTN) dev) << 16) + (((UINTN) func) << 8) + ((UINTN) reg)))

typedef struct {
    UINT8   Register;
    UINT8   Function;
    UINT8   Device;
    UINT8   Bus;
    UINT32  ExtendedRegister;
} SMM_PCI_IO_ADDRESS;

//
// Pci Configuration Space access functions definition
//
EFI_STATUS
SmmPciCfgRead (
    IN     EFI_SMM_SYSTEM_TABLE2   *Smst,
    IN     SMM_PCI_IO_WIDTH       Width,
    IN     SMM_PCI_IO_ADDRESS     *Address,
    IN OUT VOID                   *Buffer
);

EFI_STATUS
SmmPciCfgWrite (
    IN     EFI_SMM_SYSTEM_TABLE2   *Smst,
    IN     SMM_PCI_IO_WIDTH       Width,
    IN     SMM_PCI_IO_ADDRESS     *Address,
    IN OUT VOID                   *Buffer
);

EFI_STATUS
EFIAPI
SmmPciCfgModify (
    IN     EFI_SMM_SYSTEM_TABLE2   *Smst,
    IN     SMM_PCI_IO_WIDTH       Width,
    IN     SMM_PCI_IO_ADDRESS     *Address,
    IN       VOID                 *SetBits,
    IN       VOID                 *ClearBits
);

EFI_STATUS
SmmSingleSegmentPciAccess (
    IN     EFI_SMM_CPU_IO2_PROTOCOL    *CpuIo,
    IN     BOOLEAN                     IsWrite,
    IN     SMM_PCI_IO_WIDTH            Width,
    IN     SMM_PCI_IO_ADDRESS          *Address,
    IN OUT VOID                        *Buffer
);

#endif
