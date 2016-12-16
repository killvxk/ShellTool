/*++

Copyright (c) 2006-2012, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Description:

  Utility routines used by boot maintenance modules.

**/

#include "BootMaint.h"

/**

  Find the first instance of this Protocol
  in the system and return it's interface.


  @param ProtocolGuid    Provides the protocol to search for
  @param Interface       On return, a pointer to the first interface
                         that matches ProtocolGuid

  @retval  EFI_SUCCESS      A protocol instance matching ProtocolGuid was found
  @retval  EFI_NOT_FOUND    No protocol instances were found that match ProtocolGuid

**/
EFI_STATUS
EfiLibLocateProtocol (
  IN  EFI_GUID    *ProtocolGuid,
  OUT VOID        **Interface
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
                  ProtocolGuid,
                  NULL,
                  (VOID **) Interface
                  );
  return Status;
}




/**

  Helper function called as part of the code needed
  to allocate the proper sized buffer for various
  EFI interfaces.


  @param Status          Current status
  @param Buffer          Current allocated buffer, or NULL
  @param BufferSize      Current buffer size needed

  @retval  TRUE  if the buffer was reallocated and the caller
                 should try the API again.
  @retval  FALSE The caller should not call this function again.

**/
BOOLEAN
EfiGrowBuffer (
  IN OUT EFI_STATUS   *Status,
  IN OUT VOID         **Buffer,
  IN UINTN            BufferSize
  )
{
  BOOLEAN TryAgain;

  //
  // If this is an initial request, buffer will be null with a new buffer size
  //
  if ((*Buffer == NULL) && (BufferSize != 0)) {
    *Status = EFI_BUFFER_TOO_SMALL;
  }
  //
  // If the status code is "buffer too small", resize the buffer
  //
  TryAgain = FALSE;
  if (*Status == EFI_BUFFER_TOO_SMALL) {

    if (*Buffer != NULL) {
      FreePool (*Buffer);
    }

    *Buffer = AllocateZeroPool (BufferSize);

    if (*Buffer != NULL) {
      TryAgain = TRUE;
    } else {
      *Status = EFI_OUT_OF_RESOURCES;
    }
  }
  //
  // If there's an error, free the buffer
  //
  if (!TryAgain && EFI_ERROR (*Status) && (*Buffer != NULL)) {
    FreePool (*Buffer);
    *Buffer = NULL;
  }

  return TryAgain;
}


/**
  Function deletes the variable specified by VarName and VarGuid.

  @param VarName           A Null-terminated Unicode string that is
                           the name of the vendor's variable.
                         
  @param VarGuid           A unique identifier for the vendor.

  @retval  EFI_SUCCESS           The variable was found and removed
  @retval  EFI_UNSUPPORTED       The variable store was inaccessible
  @retval  EFI_NOT_FOUND         The variable was not found

**/
EFI_STATUS
EfiLibDeleteVariable (
  IN CHAR16   *VarName,
  IN EFI_GUID *VarGuid
  )
{
  return gRT->SetVariable (
                VarName,
                VarGuid,
                0,
                0,
                NULL
                );
}




/**
  Duplicate a string.

  @param Src             The source.

  @return A new string which is duplicated copy of the source.
  @retval NULL If there is not enough memory.

**/
CHAR16 *
EfiStrDuplicate (
  IN CHAR16   *Src
  )
{
  CHAR16  *Dest;
  UINTN   Size;

  Size  = StrSize (Src);
  Dest  = AllocateZeroPool (Size);
  ASSERT (Dest != NULL);
  if (Dest != NULL) {
    CopyMem (Dest, Src, Size);
  }

  return Dest;
}



/**
  Function is used to determine the number of device path instances
  that exist in a device path.


  @param DevicePath      A pointer to a device path data structure.

  @return This function counts and returns the number of device path instances
          in DevicePath.

**/
UINTN
EfiDevicePathInstanceCount (
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  )
{
  UINTN Count;
  UINTN Size;

  Count = 0;
  while (GetNextDevicePathInstance (&DevicePath, &Size) != NULL) {
    Count += 1;
  }

  return Count;
}

/**
  Adjusts the size of a previously allocated buffer.


  @param OldPool         - A pointer to the buffer whose size is being adjusted.
  @param OldSize         - The size of the current buffer.
  @param NewSize         - The size of the new buffer.

  @return   The newly allocated buffer.
  @retval   NULL  Allocation failed.

**/
VOID *
EfiReallocatePool (
  IN VOID                 *OldPool,
  IN UINTN                OldSize,
  IN UINTN                NewSize
  )
{
  VOID  *NewPool;

  NewPool = NULL;
  if (NewSize != 0) {
    NewPool = AllocateZeroPool (NewSize);
  }

  if (OldPool != NULL) {
    if (NewPool != NULL) {
      CopyMem (NewPool, OldPool, OldSize < NewSize ? OldSize : NewSize);
    }

    FreePool (OldPool);
  }

  return NewPool;
}




