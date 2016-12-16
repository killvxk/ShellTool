/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  FvbInfo.c

Abstract: 

Revision History:

**/
/*++
  This file contains an 'Intel Peripheral Driver' and is
  licensed for Intel CPUs and chipsets under the terms of your
  license agreement with Intel or your vendor.  This file may
  be modified by the user, subject to additional terms of the
  license agreement
--*/
/*++

Copyright (c) 2010, Intel Corporation. All rights reserved. <BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  FvbInfo.c

Abstract:

  Defines data structure that is the volume header found.
  These data is intent to decouple FVB driver with FV header.

--*/

#include <PiSmm.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/SystemNvDataGuid.h>


#define FVB_MEDIA_BLOCK_SIZE        0x10000
#define NV_STORAGE_BASE_ADDRESS     FixedPcdGet32(PcdFlashNvStorageVariableBase)
#define SYSTEM_NV_BLOCK_NUM         (FixedPcdGet32(PcdFlashNvStorageVariableSize) / FVB_MEDIA_BLOCK_SIZE)




typedef struct {
    EFI_PHYSICAL_ADDRESS        BaseAddress;
    EFI_FIRMWARE_VOLUME_HEADER  FvbInfo;
    //
    //EFI_FV_BLOCK_MAP_ENTRY    ExtraBlockMap[n];//n=0
    //
    EFI_FV_BLOCK_MAP_ENTRY      End[1];
} EFI_FVB2_MEDIA_INFO;


EFI_FVB2_MEDIA_INFO mPlatformFvbMediaInfo[] = {
    //
    // Systen NvStorage FVB
    //
    {
        NV_STORAGE_BASE_ADDRESS,
        {
            {0,}, //ZeroVector[16]
            EFI_SYSTEM_NV_DATA_FV_GUID,
            FVB_MEDIA_BLOCK_SIZE * SYSTEM_NV_BLOCK_NUM,
            EFI_FVH_SIGNATURE,
            0x0004feff, // check MdePkg/Include/Pi/PiFirmwareVolume.h for details on EFI_FVB_ATTRIBUTES_2
            sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
            0x0936,    //CheckSum
            0,    //ExtHeaderOffset
            {0,}, //Reserved[1]
            2,    //Revision
            {
                {
                    SYSTEM_NV_BLOCK_NUM,
                    FVB_MEDIA_BLOCK_SIZE,
                }
            }
        },
        {
            {
                0,
                0
            }
        }
    }
};

EFI_STATUS
GetFvbInfo (
    IN  EFI_PHYSICAL_ADDRESS         FvBaseAddress,
    OUT EFI_FIRMWARE_VOLUME_HEADER   **FvbInfo
)
{
    UINTN   Index;

    for (Index=0; Index < sizeof (mPlatformFvbMediaInfo)/sizeof (mPlatformFvbMediaInfo[0]); Index += 1) {
        if (mPlatformFvbMediaInfo[Index].BaseAddress == FvBaseAddress) {
            *FvbInfo =  &mPlatformFvbMediaInfo[Index].FvbInfo;

            DEBUG ((EFI_D_INFO, "\nBaseAddr: 0x%lx \n", FvBaseAddress));
            DEBUG ((EFI_D_INFO, "FvLength: 0x%lx \n", (*FvbInfo)->FvLength));
            DEBUG ((EFI_D_INFO, "HeaderLength: 0x%x \n", (*FvbInfo)->HeaderLength));
            DEBUG ((EFI_D_INFO, "FvBlockMap[0].NumBlocks: 0x%x \n", (*FvbInfo)->BlockMap[0].NumBlocks));
            DEBUG ((EFI_D_INFO, "FvBlockMap[0].BlockLength: 0x%x \n", (*FvbInfo)->BlockMap[0].Length));
            DEBUG ((EFI_D_INFO, "FvBlockMap[1].NumBlocks: 0x%x \n",   (*FvbInfo)->BlockMap[1].NumBlocks));
            DEBUG ((EFI_D_INFO, "FvBlockMap[1].BlockLength: 0x%x \n\n", (*FvbInfo)->BlockMap[1].Length));

            return EFI_SUCCESS;
        }
    }
    return EFI_NOT_FOUND;
}
