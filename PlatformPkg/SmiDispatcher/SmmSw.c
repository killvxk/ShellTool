
#include "SmmHelpers.h"
#include <Protocol/SmmCpu.h>

EFI_SMM_CPU_PROTOCOL  *mSwSmmCpu = NULL;

const SB_SMM_SOURCE_DESC SW_SOURCE_DESC = {
  SB_SMM_NO_FLAGS,
  {
    {
      {
        ACPI_ADDR_TYPE,
        PMIO_GBLEN_REG
      },
      2,
      6
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
      6
    }
  }
};

VOID
SwGetContext (
  IN  DATABASE_RECORD    *Record,
  OUT SB_SMM_CONTEXT     *Context
  )
/*++

Routine Description:

  Get the software SMI Context by read the SMI IO port

Arguments:

  Record    Not use
  Context   The software SMI Context

Returns:

  None

--*/
{
  UINT8 ApmCnt;
  ApmCnt = IoRead8(PMIO_REG(PMIO_SWSMI_CMD_REG));
  Context->Sw.SwSmiInputValue = ApmCnt;
}

BOOLEAN
SwCmpContext (
  IN SB_SMM_CONTEXT     *Context1,
  IN SB_SMM_CONTEXT     *Context2
  )
/*++

Routine Description:

  Check whether Software SMI Context of two contexts match

Arguments:

  Context1                Context 1 that includes software Context 1
  Context2                Context 2 that includes software Context 2

Returns:

  FALSE                   Software SMI Context match
  TRUE                    Software SMI Context don't match

--*/
{
  return (BOOLEAN) (Context1->Sw.SwSmiInputValue == Context2->Sw.SwSmiInputValue);
}

VOID
EFIAPI
SwGetBuffer (
  IN  DATABASE_RECORD     * Record
  )
{
  EFI_STATUS                 Status;
  UINTN                      Index;
  UINTN                      CpuIndex;
  EFI_SMM_SAVE_STATE_IO_INFO IoState;

  //
  // Locate SMM CPU protocol to retrive the CPU save state
  //
  if (mSwSmmCpu == NULL) {
    Status = gSmst->SmmLocateProtocol (&gEfiSmmCpuProtocolGuid, NULL, (VOID **) &mSwSmmCpu);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Find the CPU which generated the software SMI
  //
  CpuIndex = 0;
  for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
    Status = mSwSmmCpu->ReadSaveState (
                        mSwSmmCpu,
                        sizeof (EFI_SMM_SAVE_STATE_IO_INFO),
                        EFI_SMM_SAVE_STATE_REGISTER_IO,
                        Index,
                        &IoState
                        );
    if (!EFI_ERROR (Status) && (IoState.IoPort == SB_SWSMI_PORT_REG)) {
      CpuIndex = Index;
      break;
    }
  }

  Record->CommBuffer.Sw.SwSmiCpuIndex = CpuIndex;
  Record->CommBuffer.Sw.CommandPort   = IoRead8(SB_SWSMI_PORT_REG);
  Record->CommBuffer.Sw.DataPort      = IoRead8(SB_SWSMI_DATA_REG);
	
}

