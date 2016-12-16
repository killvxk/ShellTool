
#include "SmmHelpers.h"



VOID
SbSmmPublishDispatchProtocols (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gSmst->SmmInstallProtocolInterface (
                    &mPrivateData.ProtocolHandle[SxType],
                    &gEfiSmmSxDispatch2ProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mPrivateData.Protocols[SxType].Protocols.Sx
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmInstallProtocolInterface (
                    &mPrivateData.ProtocolHandle[SwType],
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mPrivateData.Protocols[SwType].Protocols.Sw
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmInstallProtocolInterface (
                    &mPrivateData.ProtocolHandle[PowerButtonType],
                    &gEfiSmmPowerButtonDispatch2ProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mPrivateData.Protocols[PowerButtonType].Protocols.PowerButton
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmInstallProtocolInterface (
                    &mPrivateData.ProtocolHandle[PeriodicTimerType],
                    &gEfiSmmPeriodicTimerDispatch2ProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mPrivateData.Protocols[PeriodicTimerType].Protocols.PeriodicTimer
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmInstallProtocolInterface (
                    &mPrivateData.ProtocolHandle[UsbType],
                    &gEfiSmmUsbDispatch2ProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mPrivateData.Protocols[UsbType].Protocols.Usb
                    );
  ASSERT_EFI_ERROR (Status);
}



EFI_STATUS
SbSmmInitHardware (
  VOID
  )
/*++

Routine Description:

  Initialize bits that aren't necessarily related to an SMI source.

Arguments:

  None

Returns:

  EFI_SUCCESS.  
  Asserts, otherwise.

--*/
{
  EFI_STATUS  Status;

  //
  // Clear all SMIs
  //
  SbSmmClearSmi ();

  Status = SbSmmEnableGlobalSmiBit ();
  ASSERT_EFI_ERROR (Status);

  //
  // Be *really* sure to clear all SMIs
  //
  SbSmmClearSmi ();

  return EFI_SUCCESS;
}

EFI_STATUS
SbSmmEnableGlobalSmiBit (
  VOID
  )
/*++

Routine Description:

  Enables the SB to generate SMIs. Note that no SMIs will be generated
  if no SMI sources are enabled. Conversely, no enabled SMI source will
  generate SMIs if SMIs are not globally enabled. This is the main 
  switchbox for SMI generation.

Arguments:

  None

Returns:

  EFI_SUCCESS.  
  Asserts, otherwise.

--*/
{
  IoOr16 (mAcpiBaseAddr + PMIO_GBLCTRL_REG, PMIO_GC_SMIEN);
  return EFI_SUCCESS;
}

EFI_STATUS
SbSmmClearSmi (
  VOID
  )
/*++

Routine Description:

  Clears the SMI after all SMI source have been processed.  
  Note that this function will not work correctly (as it is
  written) unless all SMI sources have been processed.  
  A revision of this function could manually clear all SMI
  status bits to guarantee success.

Arguments:

  None

Returns:

  EFI_SUCCESS.  
  Asserts, otherwise.

--*/
{
  BOOLEAN SmiClear;
  BOOLEAN SciEn;
  UINT32  Pm1Cnt;

  //
  // Determine whether an ACPI OS is present (via the SCI_EN bit)
  //
  Pm1Cnt  = IoRead32 (mAcpiBaseAddr + PMIO_PM1_CNT_REG);
  SciEn   = (BOOLEAN) ((Pm1Cnt & PMIO_PM1_CNT_SCI_EN) == PMIO_PM1_CNT_SCI_EN);
  if (!SciEn) {
    IoWrite16 (mAcpiBaseAddr + PMIO_STS_REG, IoRead16(mAcpiBaseAddr + PMIO_STS_REG));
    IoWrite16 (mAcpiBaseAddr + PMIO_GP_STS,  IoRead16(mAcpiBaseAddr + PMIO_GP_STS));
		IoWrite16(PMIO_REG(PMIO_GBLSTS_REG),  PMIO_GBLSTS_SWSMI);
  }

  //
  // Try to clear the EOS bit. ASSERT on an error
  //
  SmiClear = SbSmiSetInActive();
  ASSERT(SmiClear);

  return EFI_SUCCESS;
}

BOOLEAN
SbSmiSetInActive (
  VOID
  )
{
  IoOr16(mAcpiBaseAddr + PMIO_GBLCTRL_REG, PMIO_GC_SMIACTIVE);
	return (IoRead16(mAcpiBaseAddr + PMIO_GBLCTRL_REG) & PMIO_GC_SMIACTIVE)?FALSE:TRUE;
}

BOOLEAN
SbSmmGetSciEn (
  VOID
  )
/*++

Routine Description:

  Determine whether an ACPI OS is present (via the SCI_EN bit)

Arguments:

  None

Returns:

  TRUE                    ACPI OS is present
  FALSE                   ACPI OS is not present

--*/
{
  BOOLEAN SciEn;
  UINT32  Pm1Cnt;

  //
  // Determine whether an ACPI OS is present (via the SCI_EN bit)
  //
  Pm1Cnt  = IoRead16(mAcpiBaseAddr + PMIO_PM1_CNT_REG);
  SciEn   = (BOOLEAN)((Pm1Cnt & PMIO_PM1_CNT_SCI_EN) == PMIO_PM1_CNT_SCI_EN);

  return SciEn;
}
//
// These may or may not need to change w/ the PCH version; they're highly IA-32 dependent, though.
//
BOOLEAN
ReadBitDesc (
  const SB_SMM_BIT_DESC  *BitDesc
  )
/*++

Routine Description:

  Read a specifying bit with the register

Arguments:

  BitDesc                 The struct that includes register address, size in byte and bit number 

Returns:

  TRUE                    The bit is enabled
  FALSE                   The bit is disabled

--*/
{
  EFI_STATUS  Status;
  UINT64      Register;
  UINT32      PciBus;
  UINT32      PciDev;
  UINT32      PciFun;
  UINT32      PciReg;
  UINTN       RegSize;
  BOOLEAN     BitWasOne;
  UINTN       ShiftCount;
  UINTN       RegisterOffset;

  ASSERT (BitDesc != NULL);
  ASSERT (!IS_BIT_DESC_NULL (*BitDesc));

  RegSize     = 0;
  Register    = 0;
  ShiftCount  = 0;
  BitWasOne   = FALSE;

  switch (BitDesc->Reg.Type) {

  case ACPI_ADDR_TYPE:
    switch (BitDesc->SizeInBytes) {

    case 0:
      //
      // Chances are that this field didn't get initialized.
      // Check your assignments to bit descriptions.
      //
      ASSERT (FALSE);
      break;

    case 1:
      RegSize = SMM_IO_UINT8;
      break;

    case 2:
      RegSize = SMM_IO_UINT16;
      break;

    case 4:
      RegSize = SMM_IO_UINT32;
      break;

    case 8:
      RegSize = SMM_IO_UINT64;
      break;

    default:
      //
      // Unsupported or invalid register size
      //
      ASSERT (FALSE);
      break;
    }
    //
    // Double check that we correctly read in the acpi base address
    //
    ASSERT ((mAcpiBaseAddr != 0x0) && ((mAcpiBaseAddr & 0x1) != 0x1));

    RegisterOffset  = BitDesc->Reg.Data.acpi;
    ShiftCount      = BitDesc->Bit;
    //
    // As current CPU Smm Io can only support at most
    // 32-bit read/write,if Operation is 64 bit,
    // we do a 32 bit operation according to BitDesc->Bit
    //
    if (RegSize == SMM_IO_UINT64) {
      RegSize = SMM_IO_UINT32;
      //
      // If the operation is for high 32 bits
      //
      if (BitDesc->Bit >= 32) {
        RegisterOffset += 4;
        ShiftCount -= 32;
      }
    }

    Status = gSmst->SmmIo.Io.Read (
                              &gSmst->SmmIo,
                              RegSize,
                              mAcpiBaseAddr + RegisterOffset,
                              1,
                              &Register
                              );
    ASSERT_EFI_ERROR (Status);

    if ((Register & (LShiftU64 (BIT0, ShiftCount))) != 0) {
      BitWasOne = TRUE;
    } else {
      BitWasOne = FALSE;
    }
    break;

  case MEMORY_MAPPED_IO_ADDRESS_TYPE:
    //
    // Read the register, and it with the bit to read
    //
    switch (BitDesc->SizeInBytes) {
    case 1:
      Register = (UINT64) MmioRead8 ((UINTN) BitDesc->Reg.Data.Mmio);
      break;

    case 2:
      Register = (UINT64) MmioRead16 ((UINTN) BitDesc->Reg.Data.Mmio);
      break;

    case 4:
      Register = (UINT64) MmioRead32 ((UINTN) BitDesc->Reg.Data.Mmio);
      break;

    case 8:
      Register                      = (UINT64) MmioRead32 ((UINTN) BitDesc->Reg.Data.Mmio);
      *((UINT32 *) (&Register) + 1) = MmioRead32 ((UINTN) BitDesc->Reg.Data.Mmio + 4);
      break;

    default:
      //
      // Unsupported or invalid register size
      //
      ASSERT (FALSE);
      break;
    }

    Register = Register & (LShiftU64 (BIT0, BitDesc->Bit));
    if (Register) {
      BitWasOne = TRUE;
    } else {
      BitWasOne = FALSE;
    }
    break;

  case PCIE_ADDR_TYPE:
    PciBus  = BitDesc->Reg.Data.pcie.Fields.Bus;
    PciDev  = BitDesc->Reg.Data.pcie.Fields.Dev;
    PciFun  = BitDesc->Reg.Data.pcie.Fields.Fnc;
    PciReg  = BitDesc->Reg.Data.pcie.Fields.Reg;
    switch (BitDesc->SizeInBytes) {

    case 0:
      //
      // Chances are that this field didn't get initialized.
      // Check your assignments to bit descriptions.
      //
      ASSERT (FALSE);
      break;

    case 1:
      Register = (UINT64) MmioRead8 (PCI_DEV_MMBASE (PciBus, PciDev, PciFun) + PciReg);
      break;

    case 2:
      Register = (UINT64) MmioRead16 (PCI_DEV_MMBASE (PciBus, PciDev, PciFun) + PciReg);
      break;

    case 4:
      Register = (UINT64) MmioRead32 (PCI_DEV_MMBASE (PciBus, PciDev, PciFun) + PciReg);
      break;

    default:
      //
      // Unsupported or invalid register size
      //
      ASSERT (FALSE);
      break;
    }

    if ((Register & (LShiftU64 (BIT0, BitDesc->Bit))) != 0) {
      BitWasOne = TRUE;
    } else {
      BitWasOne = FALSE;
    }
    break;

  default:
    //
    // This address type is not yet implemented
    //
    ASSERT (FALSE);
    break;
  }

  return BitWasOne;
}

VOID
WriteBitDesc (
  const SB_SMM_BIT_DESC   *BitDesc,
  const BOOLEAN           ValueToWrite,
  const BOOLEAN           WriteClear
  )
/*++

Routine Description:

  Write a specifying bit with the register

Arguments:

  BitDesc                 The struct that includes register address, size in byte and bit number 
  ValueToWrite            The value to be wrote
  WriteClear              If the rest bits of the register is write clear

Returns:

  None

--*/
{
  EFI_STATUS  Status;
  UINT64      Register;
  UINT64      AndVal;
  UINT64      OrVal;
  UINT32      RegSize;
  UINT32      PciBus;
  UINT32      PciDev;
  UINT32      PciFun;
  UINT32      PciReg;
  UINTN       RegisterOffset;

  ASSERT (BitDesc != NULL);
  ASSERT (!IS_BIT_DESC_NULL (*BitDesc));

  RegSize   = 0;
  Register  = 0;

  if (WriteClear) {
    AndVal    = LShiftU64 (BIT0, BitDesc->Bit);
  } else {
    AndVal    = ~(LShiftU64 (BIT0, BitDesc->Bit));
  }
  OrVal     = (LShiftU64 ((UINT32) ValueToWrite, BitDesc->Bit));

  switch (BitDesc->Reg.Type) {

  case ACPI_ADDR_TYPE:
    switch (BitDesc->SizeInBytes) {

    case 0:
      //
      // Chances are that this field didn't get initialized.
      // Check your assignments to bit descriptions.
      //
      ASSERT (FALSE);
      break;

    case 1:
      RegSize = SMM_IO_UINT8;
      break;

    case 2:
      RegSize = SMM_IO_UINT16;
      break;

    case 4:
      RegSize = SMM_IO_UINT32;
      break;

    case 8:
      RegSize = SMM_IO_UINT64;
      break;

    default:
      //
      // Unsupported or invalid register size
      //
      ASSERT (FALSE);
      break;
    }
    //
    // Double check that we correctly read in the acpi base address
    //
    ASSERT ((mAcpiBaseAddr != 0x0) && ((mAcpiBaseAddr & 0x1) != 0x1));

    RegisterOffset = BitDesc->Reg.Data.acpi;
    //
    // As current CPU Smm Io can only support at most
    // 32-bit read/write,if Operation is 64 bit,
    // we do a 32 bit operation according to BitDesc->Bit
    //
    if (RegSize == SMM_IO_UINT64) {
      RegSize = SMM_IO_UINT32;
      //
      // If the operation is for high 32 bits
      //
      if (BitDesc->Bit >= 32) {
        RegisterOffset += 4;

        if (WriteClear) {
          AndVal  = LShiftU64 (BIT0, BitDesc->Bit - 32);
        } else {
          AndVal  = ~(LShiftU64 (BIT0, BitDesc->Bit - 32));
        }
        OrVal   = LShiftU64 ((UINT32) ValueToWrite, BitDesc->Bit - 32);
      }
    }

    Status = gSmst->SmmIo.Io.Read (
                              &gSmst->SmmIo,
                              RegSize,
                              mAcpiBaseAddr + RegisterOffset,
                              1,
                              &Register
                              );
    ASSERT_EFI_ERROR (Status);

    Register &= AndVal;
    Register |= OrVal;

    Status = gSmst->SmmIo.Io.Write (
                              &gSmst->SmmIo,
                              RegSize,
                              mAcpiBaseAddr + RegisterOffset,
                              1,
                              &Register
                              );
    ASSERT_EFI_ERROR (Status);
    break;

  case MEMORY_MAPPED_IO_ADDRESS_TYPE:
    //
    // Read the register, or it with the bit to set, then write it back.
    //
    switch (BitDesc->SizeInBytes) {
    case 1:
      Register = (UINT64) MmioRead8 ((UINTN) BitDesc->Reg.Data.Mmio);
      break;

    case 2:
      Register = (UINT64) MmioRead16 ((UINTN) BitDesc->Reg.Data.Mmio);
      break;

    case 4:
      Register = (UINT64) MmioRead32 ((UINTN) BitDesc->Reg.Data.Mmio);
      break;

    case 8:
      Register = (UINT64) MmioRead32 ((UINTN) BitDesc->Reg.Data.Mmio);
      *((UINT32 *) (&Register) + 1) = MmioRead32 ((UINTN) BitDesc->Reg.Data.Mmio + 4);
      break;

    default:
      //
      // Unsupported or invalid register size
      //
      ASSERT (FALSE);
      break;
    }

    Register &= AndVal;
    Register |= OrVal;
    //
    // Read the register, or it with the bit to set, then write it back.
    //
    switch (BitDesc->SizeInBytes) {
    case 1:
      MmioWrite8 ((UINTN) BitDesc->Reg.Data.Mmio, (UINT8) Register);
      break;

    case 2:
      MmioWrite16 ((UINTN) BitDesc->Reg.Data.Mmio, (UINT16) Register);
      break;

    case 4:
      MmioWrite32 ((UINTN) BitDesc->Reg.Data.Mmio, (UINT32) Register);
      break;

    case 8:
      MmioWrite32 ((UINTN) BitDesc->Reg.Data.Mmio, (UINT32) Register);
      MmioWrite32 ((UINTN) BitDesc->Reg.Data.Mmio + 4, *((UINT32 *) (&Register) + 1));
      break;

    default:
      //
      // Unsupported or invalid register size
      //
      ASSERT (FALSE);
      break;
    }
    break;

  case PCIE_ADDR_TYPE:
    PciBus  = BitDesc->Reg.Data.pcie.Fields.Bus;
    PciDev  = BitDesc->Reg.Data.pcie.Fields.Dev;
    PciFun  = BitDesc->Reg.Data.pcie.Fields.Fnc;
    PciReg  = BitDesc->Reg.Data.pcie.Fields.Reg;
    switch (BitDesc->SizeInBytes) {

    case 0:
      //
      // Chances are that this field didn't get initialized -- check your assignments
      // to bit descriptions.
      //
      ASSERT (FALSE);
      break;

    case 1:
      MmioAndThenOr8 (PCI_DEV_MMBASE (PciBus, PciDev, PciFun) + PciReg, (UINT8) AndVal, (UINT8) OrVal);
      break;

    case 2:
      MmioAndThenOr16 (PCI_DEV_MMBASE (PciBus, PciDev, PciFun) + PciReg, (UINT16) AndVal, (UINT16) OrVal);
      break;

    case 4:
      MmioAndThenOr32 (PCI_DEV_MMBASE (PciBus, PciDev, PciFun) + PciReg, (UINT32) AndVal, (UINT32) OrVal);
      break;

    default:
      //
      // Unsupported or invalid register size
      //
      ASSERT (FALSE);
      break;
    }
    break;

  default:
    //
    // This address type is not yet implemented
    //
    ASSERT (FALSE);
    break;
  }
}
