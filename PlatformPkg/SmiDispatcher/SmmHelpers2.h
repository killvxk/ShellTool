#ifndef __SMM_HELPERS2_H__
#define __SMM_HELPERS2_H__

#include "SbSmm.h"
#include <Library/DevicePathLib.h>
#include <Protocol/DevicePathToText.h>

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
;

EFI_STATUS
SbSmmEnableGlobalSmiBit (
  VOID
  )
/*++

Routine Description:

  Enables the PCH to generate SMIs. Note that no SMIs will be generated
  if no SMI sources are enabled. Conversely, no enabled SMI source will
  generate SMIs if SMIs are not globally enabled. This is the main 
  switchbox for SMI generation.

Arguments:

  None

Returns:

  EFI_SUCCESS.  
  Asserts, otherwise.

--*/
;

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
;

BOOLEAN
SbSmiSetInActive (
  VOID
  )
/*++

Routine Description:

  Set the SMI EOS bit after all SMI source have been processed.

Arguments:

  None

Returns:

  FALSE                   SMI still active
  TRUE                    SMI is inactive for next trigger.

--*/
;

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
;

BOOLEAN
ReadBitDesc (
  const SB_SMM_BIT_DESC *BitDesc
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
;

VOID
WriteBitDesc (
  const SB_SMM_BIT_DESC  *BitDesc,
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
;

#endif



