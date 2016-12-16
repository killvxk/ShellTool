/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  PeiBiosIdLib.c

Abstract:
  Boot service DXE BIOS ID library implementation.

Revision History:

**/

#include <PiPei.h>
#include <Pi/PiFirmwareVolume.h>
#include <Library/DebugLib.h>
#include <Library/BiosIdLib.h>
#include <Library/BaseMemoryLib.h>


STATIC CONST UINT8 gBiosidSign[8] = {'$', 'I', 'B', 'I', 'O', 'S', 'I', '$'};

EFI_STATUS
GetBiosId (
  OUT BIOS_ID_IMAGE  *BiosIdImage
  )
/*++
Description:

  This function returns BIOS ID by searching HOB or FV.

Arguments:

  BiosIdImage - The BIOS ID got from HOB or FV
  
Returns:

  EFI_SUCCESS - All parameters were valid and BIOS ID has been got.

  EFI_NOT_FOUND - BiosId image is not found, and no parameter will be modified.

  EFI_INVALID_PARAMETER - The parameter is NULL
     
--*/
{
  EFI_STATUS   Status = EFI_NOT_FOUND;
  UINT8        *Data8;
  UINTN        Index;
  UINTN        Count;

  Data8 = (UINT8*)(UINTN)PcdGet32(PcdFlashFvSecBase);
  Count = PcdGet32(PcdFlashFvSecSize);
  
  for(Index=0;Index<Count;Index+=16){
    if(CompareMem(&Data8[Index], gBiosidSign, sizeof(gBiosidSign)) == 0){
      CopyMem(BiosIdImage, &Data8[Index], sizeof(BIOS_ID_IMAGE));
      Status = EFI_SUCCESS;
      break;
    }      
  }

  DEBUG((EFI_D_INFO, __FUNCTION__":%r\n", Status));
  return Status;
}



EFI_STATUS
GetRecoveryCapsuleBiosIdImage (
  IN UINTN                CapsuleBaseAddress,
  IN OUT BIOS_ID_IMAGE    *BiosIdImage
  )
{
  UINT8                        *Data8;
  UINTN                        Index;
  UINTN                        Count;
  EFI_FIRMWARE_VOLUME_HEADER   *FvHdr;
  EFI_STATUS                   Status = EFI_NOT_FOUND;

  Count = PcdGet32(PcdFlashFvSecSize);
  FvHdr = (EFI_FIRMWARE_VOLUME_HEADER*)(CapsuleBaseAddress + PcdGet32(PcdFlashFvSecBase) - PcdGet32(PcdFlashAreaBaseAddress));
  if(FvHdr->Signature != EFI_FVH_SIGNATURE || FvHdr->FvLength != Count){
    goto ProcExit;
  }
  
  FvHdr++;
  Data8 = (UINT8*)ALIGN_POINTER(FvHdr, 16);
  for(Index=0;Index<Count;Index+=16){
    if(CompareMem(&Data8[Index], gBiosidSign, sizeof(gBiosidSign)) == 0){
      CopyMem(BiosIdImage, &Data8[Index], sizeof(BIOS_ID_IMAGE));
      Status = EFI_SUCCESS;
      break;
    }      
  }  

ProcExit:
  if(EFI_ERROR(Status)){	
    DEBUG((EFI_D_INFO, __FUNCTION__":%r\n", Status));
  }		
  return Status;
}






EFI_STATUS
GetBiosVersionDateTime (
  OUT CHAR16    *BiosVersion, 
  OUT CHAR16    *BiosReleaseDate,
  OUT CHAR16    *BiosReleaseTime OPTIONAL
  )
/*++
Description:

  This function returns the Version & Release date and time by getting and converting
  BIOS ID.

Arguments:

  BiosVersion - The Bios Version out of the conversion

  BiosReleaseDate - The Bios Release Date out of the conversion

  BiosReleaseTime - The Bios Release Time out of the conversion
  
Returns:

  EFI_SUCCESS - All parameters were valid and Version & Release Date have been set.

  EFI_NOT_FOUND - BiosId image is not found, and no parameter will be modified.

  EFI_INVALID_PARAMETER - One of the parameters is NULL
     
--*/
{
  EFI_STATUS    Status;
  BIOS_ID_IMAGE BiosIdImage;
  
  if ((BiosVersion == NULL) || (BiosReleaseDate == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetBiosId (&BiosIdImage);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  
  //
  // Fill the BiosVersion data from the BIOS ID.
  //
  StrCpy (BiosVersion, (CHAR16 *) (&(BiosIdImage.BiosIdString)));
  
  //
  // Fill the build timestamp data from the BIOS ID in the "MM/DD/YY" format.
  //
  
  BiosReleaseDate[0] = BiosIdImage.BiosIdString.TimeStamp[2];
  BiosReleaseDate[1] = BiosIdImage.BiosIdString.TimeStamp[3];
  BiosReleaseDate[2] = (CHAR16) ((UINT8) ('/'));

  BiosReleaseDate[3] = BiosIdImage.BiosIdString.TimeStamp[4];
  BiosReleaseDate[4] = BiosIdImage.BiosIdString.TimeStamp[5];
  BiosReleaseDate[5] = (CHAR16) ((UINT8) ('/'));

  //
  // Add 20 for SMBIOS table
  // Current Linux kernel will misjudge 09 as year 0, so using 2009 for SMBIOS table
  //
  BiosReleaseDate[6] = '2';
  BiosReleaseDate[7] = '0';
  BiosReleaseDate[8] = BiosIdImage.BiosIdString.TimeStamp[0];
  BiosReleaseDate[9] = BiosIdImage.BiosIdString.TimeStamp[1];

  BiosReleaseDate[10] = (CHAR16) ((UINT8) ('\0'));

  if (BiosReleaseTime != NULL) {

    //
    // Fill the build timestamp time from the BIOS ID in the "HH:MM" format.
    //
  
    BiosReleaseTime[0] = BiosIdImage.BiosIdString.TimeStamp[6];
    BiosReleaseTime[1] = BiosIdImage.BiosIdString.TimeStamp[7];
    BiosReleaseTime[2] = (CHAR16) ((UINT8) (':'));

    BiosReleaseTime[3] = BiosIdImage.BiosIdString.TimeStamp[8];
    BiosReleaseTime[4] = BiosIdImage.BiosIdString.TimeStamp[9];

    BiosReleaseTime[5] = (CHAR16) ((UINT8) ('\0'));
  }
  
  return  EFI_SUCCESS;
}
