
#include "SmmHelpers.h"


const SB_SMM_SOURCE_DESC SX_SOURCE_DESC = {
  SB_SMM_NO_FLAGS,
  {
    {
      {
        ACPI_ADDR_TYPE,
        PMIO_GBLEN_REG
      },
      2,
      10
    },
    NULL_BIT_DESC_INITIALIZER
  },
  {
    {
      {
        ACPI_ADDR_TYPE,
        PMIO_GBLSTS_REG
      },
      2,
      10
    }
  }
};

VOID
SxGetContext (
  IN  DATABASE_RECORD    *Record,
  OUT SB_SMM_CONTEXT     *Context
  )
/*++

Routine Description:

  Get the Sleep type

Arguments:

  Record                  No use
  Context                 The context that includes SLP_TYP bits to be filled

Returns:

  None

--*/
{
  UINT16  Pm1Cnt;

  Pm1Cnt = IoRead16(mAcpiBaseAddr + PMIO_PM1_CNT_REG);
  Context->Sx.Phase = SxEntry;

  switch (Pm1Cnt & PMIO_PM1_CNT_SLP_TYP) {
    case PMIO_PM1_CNT_S0:
      Context->Sx.Type = SxS0;
      break;

    case PMIO_PM1_CNT_S1:
      Context->Sx.Type = SxS1;
      break;

    case PMIO_PM1_CNT_S3:
      Context->Sx.Type = SxS3;
      break;

    case PMIO_PM1_CNT_S4:
      Context->Sx.Type = SxS4;
      break;

    case PMIO_PM1_CNT_S5:
      Context->Sx.Type = SxS5;
      break;

    default:
      ASSERT(FALSE);
      break;
  }
}

BOOLEAN
SxCmpContext (
  IN SB_SMM_CONTEXT     *Context1,
  IN SB_SMM_CONTEXT     *Context2
  )
/*++

Routine Description:

  Check whether sleep type of two contexts match

Arguments:

  Context1                Context 1 that includes sleep type 1
  Context2                Context 2 that includes sleep type 2

Returns:

  FALSE                   Sleep types match
  TRUE                    Sleep types don't match

--*/
{
  return (BOOLEAN) (Context1->Sx.Type == Context2->Sx.Type);
}



VOID
SbSmmSxGoToSleep (
  VOID
  )
/*++

Routine Description:

  When we get an SMI that indicates that we are transitioning to a sleep state,
  we need to actually transition to that state.  We do this by disabling the 
  "SMI on sleep enable" feature, which generates an SMI when the operating system
  tries to put the system to sleep, and then physically putting the system to sleep.

Arguments:

  None

Returns:

  None.

--*/
{
  UINT16  Pm1Cnt;

// Flush cache into memory before we go to sleep. It is necessary for S3 sleep
// because we may update memory in SMM Sx sleep handlers -- the updates are in cache now
  AsmWbinvd();

  //
  // Disable SMIs
  //
  SbSmmClearSource (&SX_SOURCE_DESC);
  SbSmmDisableSource (&SX_SOURCE_DESC);

  //
  // Get Power Management 1 Control Register Value
  //
  Pm1Cnt = IoRead16 (mAcpiBaseAddr + PMIO_PM1_CNT_REG);
  
// Now that SMIs are disabled, write to the SLP_EN bit again to trigger the sleep
  Pm1Cnt |= PMIO_PM1_CNT_SLP_EN;

  IoWrite16(mAcpiBaseAddr + PMIO_PM1_CNT_REG, Pm1Cnt);

  //
  // Should only proceed if wake event is generated.
  //
  if ((Pm1Cnt & PMIO_PM1_CNT_SLP_TYP) == PMIO_PM1_CNT_S1) {
    while (((IoRead16 ((UINTN) (mAcpiBaseAddr + PMIO_STS_REG))) & PMIO_STS_WAK) == 0x0){};
  } else {
    CpuDeadLoop();
  }

  //
  // The system just went to sleep. If the sleep state was S1, then code execution will resume
  // here when the system wakes up.
  //
  Pm1Cnt = IoRead16 (mAcpiBaseAddr + PMIO_PM1_CNT_REG);

  if ((Pm1Cnt & PMIO_PM1_CNT_SCI_EN) == 0) {
    //
    // An ACPI OS isn't present, clear the sleep information
    //
    Pm1Cnt &= ~PMIO_PM1_CNT_SLP_TYP;
    Pm1Cnt |= PMIO_PM1_CNT_S0;

    IoWrite16(mAcpiBaseAddr + PMIO_PM1_CNT_REG, Pm1Cnt);
  }

  SbSmmClearSource (&SX_SOURCE_DESC);
  SbSmmEnableSource (&SX_SOURCE_DESC);
}



