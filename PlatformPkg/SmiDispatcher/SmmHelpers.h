
#ifndef __SB_SMM_HELPERS_H__
#define __SB_SMM_HELPERS_H__

#include "SbSmm.h"
#include "SmmHelpers2.h"

VOID
SbSmmPublishDispatchProtocols (
  VOID
  );


BOOLEAN
CompareEnables (
  const IN SB_SMM_SOURCE_DESC *Src1,
  const IN SB_SMM_SOURCE_DESC *Src2
  )
/*++

Routine Description:

  Compare 2 SMM source descriptors' enable settings.

Arguments:

  Src1  - an SMM source descriptor
  Src2  - an SMM source descriptor

Returns:

  TRUE  - The enable settings of the 2 SMM source descriptors are identical.
  FALSE - The enable settings of the 2 SMM source descriptors are not identical.

--*/
;

BOOLEAN
CompareStatuses (
  const IN SB_SMM_SOURCE_DESC *Src1,
  const IN SB_SMM_SOURCE_DESC *Src2
  )
/*++

Routine Description:

  Compare 2 SMM source descriptors' statuses.

Arguments:

  Src1  - an SMM source descriptor
  Src2  - an SMM source descriptor

Returns:

  TRUE  - The statuses of the 2 SMM source descriptors are identical.
  FALSE - The statuses of the 2 SMM source descriptors are not identical.

--*/
;

BOOLEAN
CompareSources (
  const IN SB_SMM_SOURCE_DESC *Src1,
  const IN SB_SMM_SOURCE_DESC *Src2
  )
/*++

Routine Description:

  Compare 2 SMM source descriptors, based on Enable settings and Status settings of them.

Arguments:

  Src1  - an SMM source descriptor
  Src2  - an SMM source descriptor

Returns:

  TRUE  - The 2 SMM source descriptors are identical.
  FALSE - The 2 SMM source descriptors are not identical.

--*/
;

BOOLEAN
SourceIsActive (
  const IN SB_SMM_SOURCE_DESC *Src
  )
/*++

Routine Description:

  Check if an SMM source is active.

Arguments:

  Src - An SMM source descriptor

Returns:

  TRUE  - It is active.
  FALSE - It is inactive.

--*/
;

VOID
SbSmmEnableSource (
  const SB_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  Enable the SMI source event by set the SMI enable bit, this function would also clear SMI status bit to make initial state is correct

Arguments:

  SrcDesc                 Pointer to the SB SMI source description table

Returns:

  None

--*/
;

VOID
SbSmmDisableSource (
  const SB_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  Disable the SMI source event by clear the SMI enable bit

Arguments:

  SrcDesc                 Pointer to the SB SMI source description table
  
Returns:
  
  None

--*/
;

VOID
SbSmmClearSource (
  const SB_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  Clear the SMI status bit by set the source bit of SMI status register

Arguments:

  SrcDesc                 Pointer to the SB SMI source description table

Returns:

  None

--*/
;

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

  SrcDesc                 Pointer to the SB SMI source description table

Returns:

  None

--*/
;

#endif
