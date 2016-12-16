

#include "PlatformSecLib.h"


/**
  Perform those platform specific operations that are requried 
  to be executed as early as possibile.
  @return TRUE always return true.
**/
EFI_STATUS
EFIAPI
PlatformSecLibConstructor (
  )
{
  //
  // Init Apic Timer for Performance collection.
  // Use EXCEPT_IA32_BOUND as interrupte type.
  //
  PERF_CODE (
    InitializeApicTimer (0, (UINT32) -1, TRUE, 5);
    DisableApicTimerInterrupt ();
  );

  return EFI_SUCCESS;
}




/**
  A developer supplied function to perform platform specific operations.

  It's a developer supplied function to perform any operations appropriate to a
  given platform. It's invoked just before passing control to PEI core by SEC
  core. Platform developer may modify the SecCoreData and PPI list that is
  passed to PEI Core. 

  @param  SecCoreData           The same parameter as passing to PEI core. It
                                could be overridden by this function.

  @return The platform specific PPI list to be passed to PEI core or
          NULL if there is no need of such platform specific PPI list.

**/
EFI_PEI_PPI_DESCRIPTOR *
EFIAPI
SecPlatformMain (
  IN OUT   EFI_SEC_PEI_HAND_OFF        *SecCoreData
  )
{
  return NULL;
}

