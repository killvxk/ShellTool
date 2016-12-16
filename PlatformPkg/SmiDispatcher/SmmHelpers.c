
#include "SmmHelpers.h"

//
// #define BIT_ZERO 0x00000001
//
const UINT32  BIT_ZERO = 0x00000001;

//
// /////////////////////////////////////////////////////////////////////////////
// SUPPORT / HELPER FUNCTIONS (PCH version-independent)
//
BOOLEAN
CompareEnables (
  const IN SB_SMM_SOURCE_DESC *Src1,
  const IN SB_SMM_SOURCE_DESC *Src2
  )
/*++

Routine Description:

  Compare 2 SMM source descriptors' enable settings.

Arguments:

  Src1                    Pointer to the PCH SMI source description table 1
  Src2                    Pointer to the PCH SMI source description table 2

Returns:

  TRUE                    The enable settings of the 2 SMM source descriptors are identical.
  FALSE                   The enable settings of the 2 SMM source descriptors are not identical.

--*/
{
  BOOLEAN IsEqual;
  UINTN   loopvar;

  IsEqual = TRUE;
  for (loopvar = 0; loopvar < NUM_EN_BITS; loopvar++) {
    //
    // It's okay to compare a NULL bit description to a non-NULL bit description.
    // They are unequal and these tests will generate the correct result.
    //
    if (Src1->En[loopvar].Bit != Src2->En[loopvar].Bit ||
        Src1->En[loopvar].Reg.Type != Src2->En[loopvar].Reg.Type ||
        Src1->En[loopvar].Reg.Data.raw != Src2->En[loopvar].Reg.Data.raw
        ) {
      IsEqual = FALSE;
      break;
      //
      // out of for loop
      //
    }
  }

  return IsEqual;
}

BOOLEAN
CompareStatuses (
  const IN SB_SMM_SOURCE_DESC *Src1,
  const IN SB_SMM_SOURCE_DESC *Src2
  )
/*++

Routine Description:

  Compare 2 SMM source descriptors' statuses.

Arguments:

  Src1                    Pointer to the PCH SMI source description table 1
  Src2                    Pointer to the PCH SMI source description table 2

Returns:

  TRUE                    The statuses of the 2 SMM source descriptors are identical.
  FALSE                   The statuses of the 2 SMM source descriptors are not identical.

--*/
{
  BOOLEAN IsEqual;
  UINTN   loopvar;

  IsEqual = TRUE;

  for (loopvar = 0; loopvar < NUM_STS_BITS; loopvar++) {
    //
    // It's okay to compare a NULL bit description to a non-NULL bit description.
    // They are unequal and these tests will generate the correct result.
    //
    if (Src1->Sts[loopvar].Bit != Src2->Sts[loopvar].Bit ||
        Src1->Sts[loopvar].Reg.Type != Src2->Sts[loopvar].Reg.Type ||
        Src1->Sts[loopvar].Reg.Data.raw != Src2->Sts[loopvar].Reg.Data.raw
        ) {
      IsEqual = FALSE;
      break;
      //
      // out of for loop
      //
    }
  }

  return IsEqual;
}

BOOLEAN
CompareSources (
  const IN SB_SMM_SOURCE_DESC *Src1,
  const IN SB_SMM_SOURCE_DESC *Src2
  )
/*++

Routine Description:

  Compare 2 SMM source descriptors, based on Enable settings and Status settings of them.

Arguments:

  Src1                    Pointer to the PCH SMI source description table 1
  Src2                    Pointer to the PCH SMI source description table 2

Returns:

  TRUE                    The 2 SMM source descriptors are identical.
  FALSE                   The 2 SMM source descriptors are not identical.

--*/
{
  return (BOOLEAN) (CompareEnables (Src1, Src2) && CompareStatuses (Src1, Src2));
}

BOOLEAN
SourceIsActive (
  const IN SB_SMM_SOURCE_DESC *Src
  )
/*++

Routine Description:

  Check if an SMM source is active.

Arguments:

  Src                     Pointer to the PCH SMI source description table

Returns:

  TRUE  - It is active.
  FALSE - It is inactive.

--*/
{
  BOOLEAN IsActive;
  UINTN   loopvar;

  BOOLEAN SciEn;

  IsActive  = TRUE;

  SciEn     = SbSmmGetSciEn ();

  if ((Src->Flags & SB_SMM_SCI_EN_DEPENDENT) && (SciEn)) {
    //
    // This source is dependent on SciEn, and SciEn == 1.  An ACPI OS is present,
    // so we shouldn't do anything w/ this source until SciEn == 0.
    //
    IsActive = FALSE;

  } else {
    //
    // Read each bit desc from hardware and make sure it's a one
    //
    for (loopvar = 0; loopvar < NUM_EN_BITS; loopvar++) {

      if (!IS_BIT_DESC_NULL (Src->En[loopvar])) {

        if (ReadBitDesc (&Src->En[loopvar]) == 0) {
          IsActive = FALSE;
          break;
          //
          // out of for loop
          //
        }

      }
    }

    if (IsActive) {
      //
      // Read each bit desc from hardware and make sure it's a one
      //
      for (loopvar = 0; loopvar < NUM_STS_BITS; loopvar++) {

        if (!IS_BIT_DESC_NULL (Src->Sts[loopvar])) {

          if (ReadBitDesc (&Src->Sts[loopvar]) == 0) {
            IsActive = FALSE;
            break;
            //
            // out of for loop
            //
          }

        }
      }
    }
  }

  return IsActive;
}

VOID
SbSmmEnableSource (
  const SB_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  Enable the SMI source event by set the SMI enable bit, this function would also clear SMI status bit to make initial state is correct

Arguments:

  SrcDesc                 Pointer to the PCH SMI source description table

Returns:

  None

--*/
{
  UINTN loopvar;

  //
  // Set enables to 1 by writing a 1
  //
  for (loopvar = 0; loopvar < NUM_EN_BITS; loopvar++) {
    if (!IS_BIT_DESC_NULL (SrcDesc->En[loopvar])) {
      WriteBitDesc (&SrcDesc->En[loopvar], 1, FALSE);
    }
  }
  //
  // Clear statuses to 0 by writing a 1
  //
  for (loopvar = 0; loopvar < NUM_STS_BITS; loopvar++) {
    if (!IS_BIT_DESC_NULL (SrcDesc->Sts[loopvar])) {
      WriteBitDesc (&SrcDesc->Sts[loopvar], 1, TRUE);
    }
  }
}

VOID
SbSmmDisableSource (
  const SB_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  Disable the SMI source event by clear the SMI enable bit

Arguments:

  SrcDesc                 Pointer to the PCH SMI source description table
  
Returns:
  
  None

--*/
{
  UINTN loopvar;

  for (loopvar = 0; loopvar < NUM_EN_BITS; loopvar++) {
    if (!IS_BIT_DESC_NULL (SrcDesc->En[loopvar])) {
      WriteBitDesc (&SrcDesc->En[loopvar], 0, FALSE);
    }
  }
}

VOID
SbSmmClearSource (
  const SB_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  Clear the SMI status bit by set the source bit of SMI status register

Arguments:

  SrcDesc                 Pointer to the PCH SMI source description table

Returns:

  None

--*/
{
  UINTN loopvar;

  for (loopvar = 0; loopvar < NUM_STS_BITS; loopvar++) {
    if (!IS_BIT_DESC_NULL (SrcDesc->Sts[loopvar])) {
      WriteBitDesc (&SrcDesc->Sts[loopvar], 1, TRUE);
    }
  }
}

VOID
SbSmmClearSourceAndBlock (
  const SB_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  Sets the source to a 1 and then waits for it to clear.
  Be very careful when calling this function -- it will not 
  ASSERT.  An acceptable case to call the function is when 
  waiting for the NEWCENTURY_STS bit to clear (which takes
  3 RTCCLKs).

Arguments:

  SrcDesc                 Pointer to the PCH SMI source description table

Returns:

  None

--*/
{
  UINTN   loopvar;
  BOOLEAN IsSet;

  for (loopvar = 0; loopvar < NUM_STS_BITS; loopvar++) {

    if (!IS_BIT_DESC_NULL (SrcDesc->Sts[loopvar])) {
      //
      // Write the bit
      //
      WriteBitDesc (&SrcDesc->Sts[loopvar], 1, TRUE);

      //
      // Don't return until the bit actually clears.
      //
      IsSet = TRUE;
      while (IsSet) {
        IsSet = ReadBitDesc (&SrcDesc->Sts[loopvar]);
        //
        // IsSet will eventually clear -- or else we'll have
        // an infinite loop.
        //
      }
    }
  }
}
