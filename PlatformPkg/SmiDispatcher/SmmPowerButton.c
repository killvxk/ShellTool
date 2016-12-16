
#include "SmmHelpers.h"

const SB_SMM_SOURCE_DESC POWER_BUTTON_SOURCE_DESC = {
  SB_SMM_SCI_EN_DEPENDENT,
  {
    {
      {
        ACPI_ADDR_TYPE,
        PMIO_PM_EN
      },
      2,
      8
    },
    NULL_BIT_DESC_INITIALIZER
  },
  {
    {
      {
        ACPI_ADDR_TYPE,
        PMIO_STS_REG
      },
      2,
      8
    }
  }
};

VOID
PowerButtonGetContext (
  IN  DATABASE_RECORD   *Record,
  OUT SB_SMM_CONTEXT    *Context
  )
/*++

Routine Description:

  Get the power button status.

Arguments:

  Record  - Linked list of the SMI handlers.
  Context - Calling context from the hardware, will be updated with the current power button status.

Returns:

  None.
  
--*/
{
  Context->PowerButton.Phase = EfiPowerButtonEntry;
}

BOOLEAN
PowerButtonCmpContext (
  IN SB_SMM_CONTEXT     *Context1,
  IN SB_SMM_CONTEXT     *Context2
  )
/*++

Routine Description:

  Check whether Power Button status of two contexts match

Arguments:

  Context1                Context 1 that includes Power Button status 1
  Context2                Context 2 that includes Power Button status 2

Returns:

  FALSE                   Power Button status match
  TRUE                    Power Button status don't match

--*/
{
  return (BOOLEAN) (Context1->PowerButton.Phase == Context2->PowerButton.Phase);
}
