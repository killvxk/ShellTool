
#ifndef _STRING_H_
#define _STRING_H_

#include "BdsPlatform.h"

extern EFI_HII_HANDLE gStringPackHandle;

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
  );

/**
  Initialize HII global accessor for string support.

**/
VOID
InitializeStringSupport (
  VOID
  );

#endif // _STRING_H_
