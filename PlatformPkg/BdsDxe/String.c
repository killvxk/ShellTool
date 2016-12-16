
#include "BdsPlatform.h"

EFI_HII_HANDLE gStringPackHandle;

/**
  Initialize HII global accessor for string support.

**/
VOID
InitializeStringSupport (
  VOID
  )
{
  gStringPackHandle = HiiAddPackages (
                        &gEfiCallerIdGuid,
                        gImageHandle,
                        BdsDxeStrings,
                        NULL
                        );
  ASSERT (gStringPackHandle != NULL);
}

/**
  Get string by string id from HII Interface
  FeaturePcd cannot defeat Macro in this case

  @param Id              String ID.

  @retval  CHAR16 *  String from ID.
  @retval  NULL      If error occurs.

**/
CHAR16 *
GetStringById (
  UINT16   Id
  )
{
  return HiiGetString (gStringPackHandle, Id, NULL);
}
