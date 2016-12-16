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
  SmmPciIo.c

Abstract:
  USB Module file.

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

#include "SmmPciIo.h"

EFI_STATUS
SmmPciCfgRead (
    IN     EFI_SMM_SYSTEM_TABLE2   *Smst,
    IN     SMM_PCI_IO_WIDTH       Width,
    IN     SMM_PCI_IO_ADDRESS     *Address,
    IN OUT VOID                   *Buffer
)
/*++

Routine Description:

  Read value from the specified PCI config space register.

Arguments:

  Smst     -  The EFI runtime system table.
  Width    -  The width (8, 16 or 32 bits) of accessed pci config space register.
  Address  -  The address of the accessed pci register (bus, dev, func, offset).
  Buffer   -  The returned value.

Returns:

  EFI_SUCCESS            -  All operations successfully.
  EFI_INVALID_PARAMETER  -  Width is not valid or dosn't match register address.
  Other                  -  If any error occured when calling libiary functions.

--*/
{
    EFI_SMM_CPU_IO2_PROTOCOL  *SmmCpuIo;

    SmmCpuIo = &(Smst->SmmIo);

    return SmmSingleSegmentPciAccess (SmmCpuIo, FALSE, Width, Address, Buffer);
}

EFI_STATUS
SmmPciCfgWrite (
    IN     EFI_SMM_SYSTEM_TABLE2   *Smst,
    IN     SMM_PCI_IO_WIDTH       Width,
    IN     SMM_PCI_IO_ADDRESS     *Address,
    IN OUT VOID                   *Buffer
)
/*++

Routine Description:

  Write value into the specified PCI config space register.

Arguments:

  Smst     -  The EFI runtime system table.
  Width    -  The width (8, 16 or 32 bits) of accessed pci config space register.
  Address  -  The address of the accessed pci register (bus, dev, func, offset).
  Buffer   -  The returned value.

Returns:

  EFI_SUCCESS            -  All operations successfully.
  EFI_INVALID_PARAMETER  -  Width is not valid or dosn't match register address.
  Other                  -  If any error occured when calling libiary functions.

--*/
{
    EFI_SMM_CPU_IO2_PROTOCOL  *SmmCpuIo;

    SmmCpuIo = &(Smst->SmmIo);

    return SmmSingleSegmentPciAccess (SmmCpuIo, TRUE, Width, Address, Buffer);
}

EFI_STATUS
EFIAPI
SmmPciCfgModify (
    IN     EFI_SMM_SYSTEM_TABLE2   *Smst,
    IN     SMM_PCI_IO_WIDTH       Width,
    IN     SMM_PCI_IO_ADDRESS     *Address,
    IN       VOID                 *SetBits,
    IN       VOID                 *ClearBits
)
/*++

Routine Description:

  Modifies athe contents of PCI Configuration register.

Arguments:

  Width    -  The width (8, 16 or 32 bits) of accessed pci config space register.
  Address  -  The address of the accessed pci register (bus, dev, func, offset).
  SetBits     - Bit locations to set
  ClearBits   - Bit locations to clear

Returns:

  EFI_SUCCESS
  EFI_INVALID_PARAMETER if Width is not defined in PEI_PCI_CFG_PPI_WIDTH

--*/
{
    UINTN           Buffer;
    UINTN           SetBit;
    UINTN           ClearBit;
    EFI_SMM_CPU_IO2_PROTOCOL  *SmmCpuIo;

    SmmCpuIo = &(Smst->SmmIo);

    SetBit =  *(UINTN*)SetBits;
    ClearBit = *(UINTN*)ClearBits;

    SmmSingleSegmentPciAccess (SmmCpuIo, FALSE, Width, Address, &Buffer);
    Buffer = ((Buffer & (~ClearBit)) | SetBit);

    return SmmSingleSegmentPciAccess (SmmCpuIo, TRUE, Width, Address, &Buffer);
}

EFI_STATUS
SmmSingleSegmentPciAccess (
    IN     EFI_SMM_CPU_IO2_PROTOCOL    *CpuIo,
    IN     BOOLEAN                     IsWrite,
    IN     SMM_PCI_IO_WIDTH            Width,
    IN     SMM_PCI_IO_ADDRESS          *Address,
    IN OUT VOID                        *Buffer
)
/*++

Routine Description:

  Access a PCI config space address, including read and write.

Arguments:

  CpuIo    -  The cpu I/O accessing interface provided by EFI runtime sys table.
  IsWrite  -  Indicates whether this operation is a write access or read.
  Width    -  The width (8, 16 or 32 bits) of accessed pci config space register.
  Address  -  The address of the accessed pci register (bus, dev, func, offset).
  Buffer   -  The returned value when this is a reading operation or the data.
              to be written when this is a writing one.

Returns:

  EFI_SUCCESS            -  All operations successfully.
  EFI_INVALID_PARAMETER  -  Width is not valid or dosn't match register address.
  Other                  -  If any error occured when calling libiary functions.

--*/
{
    EFI_STATUS            Status;
    PCI_CONFIG_ACCESS_CF8 PciCf8Data;
    UINT64                PciDataReg;
    UINT32                AddressPort;

    Status = CpuIo->Io.Read (CpuIo, SmmPciWidthUint32, 0xcf8, 1, &AddressPort);
    if (EFI_ERROR (Status)) {
        return Status;
    }

    //
    // PCI Config access are all 32-bit alligned, but by accessing the
    // CONFIG_DATA_REGISTER (0xcfc) with different widths more cycle types
    // are possible on PCI.
    //
    // To read a byte of PCI config space you load 0xcf8 and
    // read 0xcfc, 0xcfd, 0xcfe, 0xcff
    //
    // The validation of passed in arguments "Address" will be checked in the
    // CPU IO functions, so we don't check them here
    //
    if (Width >= SmmPciWidthMaximum) {
        return EFI_INVALID_PARAMETER;
    }

    PciCf8Data.Bits.Reg      = Address->Register & 0xfc;
    PciCf8Data.Bits.Func     = Address->Function;
    PciCf8Data.Bits.Dev      = Address->Device;
    PciCf8Data.Bits.Bus      = Address->Bus;
    PciCf8Data.Bits.Reserved = 0;
    PciCf8Data.Bits.Enable   = 1;

    Status = CpuIo->Io.Write (CpuIo, SmmPciWidthUint32, 0xcf8, 1, &PciCf8Data);
    if (EFI_ERROR (Status)) {
        return Status;
    }

    PciDataReg = 0xcfc + (Address->Register & 0x03);

    if (IsWrite) {
        //
        // This is a Pci write operation, write data into (0xcfc + offset)
        //
        Status = CpuIo->Io.Write (CpuIo, Width, PciDataReg, 1, Buffer);
        if (EFI_ERROR (Status)) {
            return Status;
        }
    } else {
        //
        // This is a Pci Read operation, read returned data from (0xcfc + offset)
        //
        Status = CpuIo->Io.Read (CpuIo, Width, PciDataReg, 1, Buffer);
        if (EFI_ERROR (Status)) {
            return Status;
        }
    }

    Status = CpuIo->Io.Write (CpuIo, SmmPciWidthUint32, 0xcf8, 1, &AddressPort);
    if (EFI_ERROR (Status)) {
        return Status;
    }

    return EFI_SUCCESS;
}
