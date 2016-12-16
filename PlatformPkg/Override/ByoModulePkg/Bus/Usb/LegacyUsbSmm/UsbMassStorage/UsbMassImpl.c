/** @file
  USB Mass Storage Driver that manages USB Mass Storage Device and produces Block I/O Protocol.

Copyright (c) 2007 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UsbMassImpl.h"
#include "../UsbLib/UsbUtility.h"
#include <library/DevicePathLib.h>


LIST_ENTRY  mUsbMassStorageList = INITIALIZE_LIST_HEAD_VARIABLE (mUsbMassStorageList);
UINT8 mListInitialized = FALSE;

//
// The underlying transport protocol. CBI support isn't included
// in the current build. It is being obseleted by the standard
// body. If you want to enable it, remove the if directive here,
// then add the UsbMassCbi.c/.h to the driver's inf file.
//
STATIC
USB_MASS_TRANSPORT *mUsbMassTransport[] = {
  &mUsbCbi0Transport,
  &mUsbCbi1Transport,
  &mUsbBotTransport,
  NULL
};

UINTN mUsbMscInfo  = EFI_D_ERROR;
UINTN mUsbMscError = EFI_D_ERROR;

/**
  Initialize the media parameter data for EFI_BLOCK_IO_MEDIA of Block I/O Protocol.

  @param  UsbMass                The USB mass storage device

  @retval EFI_SUCCESS            The media parameters are updated successfully.
  @retval Others                 Failed to get the media parameters.

**/
EFI_STATUS
UsbMassInitMedia (
  IN USB_MASS_DEVICE          *UsbMass
  )
{
  EFI_BLOCK_IO_MEDIA          *Media;
  EFI_STATUS                  Status;
  UINTN                       Index;

  Media = &UsbMass->BlockIoMedia;

  //
  // Initialize the MediaPrsent/ReadOnly and others to the default.
  // We are not forced to get it right at this time, check UEFI2.0
  // spec for more information:
  //
  // MediaPresent: This field shows the media present status as
  //               of the most recent ReadBlocks or WriteBlocks call.
  //
  // ReadOnly    : This field shows the read-only status as of the
  //               recent WriteBlocks call.
  //
  // but remember to update MediaId/MediaPresent/ReadOnly status
  // after ReadBlocks and WriteBlocks
  //
  Media->MediaPresent     = FALSE;
  Media->LogicalPartition = FALSE;
  Media->ReadOnly         = FALSE;
  Media->WriteCaching     = FALSE;
  Media->IoAlign          = 0;
  Media->MediaId          = 1;

  //
  // Some device may spend several seconds before it is ready.
  // Try several times before giving up. Wait 5s at most.
  //
  Status = EFI_SUCCESS;

  for (Index = 0; Index < USB_BOOT_INIT_MEDIA_RETRY; Index++) {
    Status = UsbBootGetParams (UsbMass);
    if ((Status != EFI_MEDIA_CHANGED) && (Status != EFI_NOT_READY) && 
       (Status != EFI_TIMEOUT)) {
      break;
    }

    Status = UsbBootIsUnitReady (UsbMass);
    if (EFI_ERROR (Status)) {
      Stall (USB_BOOT_RETRY_UNIT_READY_STALL * (Index + 1));
    }
  }

  return Status;
}

/**
  Reset the block device.

  This function implements EFI_BLOCK_IO_PROTOCOL.Reset(). 
  It resets the block device hardware.
  ExtendedVerification is ignored in this implementation.

  @param  This                   Indicates a pointer to the calling context.
  @param  ExtendedVerification   Indicates that the driver may perform a more exhaustive
                                 verification operation of the device during reset.

  @retval EFI_SUCCESS            The block device was reset.
  @retval EFI_DEVICE_ERROR       The block device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
UsbMassReset (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN BOOLEAN                  ExtendedVerification
  )
{
  USB_MASS_DEVICE *UsbMass;
  EFI_STATUS      Status;

  UsbMass = USB_MASS_DEVICE_FROM_BLOCKIO (This);
  Status  = UsbMass->Transport->Reset (UsbMass->Context, ExtendedVerification);

  return Status;
}

/**
  Reads the requested number of blocks from the device.

  This function implements EFI_BLOCK_IO_PROTOCOL.ReadBlocks(). 
  It reads the requested number of blocks from the device.
  All the blocks are read, or an error is returned.

  @param  This                   Indicates a pointer to the calling context.
  @param  MediaId                The media ID that the read request is for.
  @param  Lba                    The starting logical block address to read from on the device.
  @param  BufferSize             The size of the Buffer in bytes.
                                 This must be a multiple of the intrinsic block size of the device.
  @param  Buffer                 A pointer to the destination buffer for the data. The caller is
                                 responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS            The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to perform the read operation.
  @retval EFI_NO_MEDIA           There is no media in the device.
  @retval EFI_MEDIA_CHANGED      The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE    The BufferSize parameter is not a multiple of the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER  The read request contains LBAs that are not valid,
                                 or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
UsbMassReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  OUT VOID                    *Buffer
  )
{
  USB_MASS_DEVICE     *UsbMass;
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;
  UINTN               TotalBlock;

  UsbMass = USB_MASS_DEVICE_FROM_BLOCKIO (This);
  Media   = &UsbMass->BlockIoMedia;

  
  //
  // If it is a removable media, such as CD-Rom or Usb-Floppy,
  // need to detect the media before each rw. While some of
  // Usb-Flash is marked as removable media.
  //
  //
  if (Media->RemovableMedia) {
    Status = UsbBootDetectMedia (UsbMass);
    if (EFI_ERROR (Status)) {
      USB_DEBUG((EFI_D_ERROR, "UsbMassReadBlocks: UsbBootDetectMedia (%r)\n", Status));
      goto ON_EXIT;
    }
  }

  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto ON_EXIT;
  }

  if (MediaId != Media->MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto ON_EXIT;
  }

  if (BufferSize == 0) {
    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }

  if (Buffer == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  //
  // Make sure BlockSize and LBA is consistent with BufferSize
  //
  if ((BufferSize % Media->BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto ON_EXIT;
  }

  TotalBlock = BufferSize / Media->BlockSize;

  //
  // Make sure the range to read is valid.
  //
  if (Lba + TotalBlock - 1 > Media->LastBlock) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Status = UsbBootReadBlocks (UsbMass, (UINT32) Lba, TotalBlock, Buffer);
  if (EFI_ERROR (Status)) {
    USB_DEBUG((EFI_D_ERROR, "UsbMassReadBlocks: UsbBootReadBlocks (%r) -> Reset\n", Status));
    UsbMassReset (This, TRUE);
  }

ON_EXIT:
    return Status;
}


/**
  Writes a specified number of blocks to the device.

  This function implements EFI_BLOCK_IO_PROTOCOL.WriteBlocks(). 
  It writes a specified number of blocks to the device.
  All blocks are written, or an error is returned.

  @param  This                   Indicates a pointer to the calling context.
  @param  MediaId                The media ID that the write request is for.
  @param  Lba                    The starting logical block address to be written.
  @param  BufferSize             The size of the Buffer in bytes.
                                 This must be a multiple of the intrinsic block size of the device.
  @param  Buffer                 Pointer to the source buffer for the data.

  @retval EFI_SUCCESS            The data were written correctly to the device.
  @retval EFI_WRITE_PROTECTED    The device cannot be written to.
  @retval EFI_NO_MEDIA           There is no media in the device.
  @retval EFI_MEDIA_CHANGED      The MediaId is not for the current media.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to perform the write operation.
  @retval EFI_BAD_BUFFER_SIZE    The BufferSize parameter is not a multiple of the intrinsic
                                 block size of the device.
  @retval EFI_INVALID_PARAMETER  The write request contains LBAs that are not valid,
                                 or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
UsbMassWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer
  )
{
    USB_MASS_DEVICE     *UsbMass;
    EFI_BLOCK_IO_MEDIA  *Media;
    EFI_STATUS          Status;
    UINTN               TotalBlock;

    UsbMass = USB_MASS_DEVICE_FROM_BLOCKIO (This);
    Media   = &UsbMass->BlockIoMedia;

    //
    // If it is a removable media, such as CD-Rom or Usb-Floppy,
    // need to detect the media before each rw. While some of
    // Usb-Flash is marked as removable media.
    //
    //
    if (Media->RemovableMedia) {
        Status = UsbBootDetectMedia (UsbMass);
        if (EFI_ERROR (Status)) {
            USB_DEBUG((EFI_D_ERROR, "UsbMassWriteBlocks: UsbBootDetectMedia (%r)\n", Status));
            goto ON_EXIT;
        }
    }

    if (!(Media->MediaPresent)) {
      Status = EFI_NO_MEDIA;
      goto ON_EXIT;
    }

    if (MediaId != Media->MediaId) {
      Status = EFI_MEDIA_CHANGED;
      goto ON_EXIT;
    }

    if (BufferSize == 0) {
      Status = EFI_SUCCESS;
      goto ON_EXIT;
    }

    if (Buffer == NULL) {
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    //
    // Make sure BlockSize and LBA is consistent with BufferSize
    //
    if ((BufferSize % Media->BlockSize) != 0) {
        Status = EFI_BAD_BUFFER_SIZE;
        goto ON_EXIT;
    }

    TotalBlock = BufferSize / Media->BlockSize;

    //
    // Make sure the range to write is valid.
    //
    if (Lba + TotalBlock - 1 > Media->LastBlock) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
    }

    //
    // Try to write the data even the device is marked as ReadOnly,
    // and clear the status should the write succeed.
    //
    Status = UsbBootWriteBlocks (UsbMass, (UINT32) Lba, TotalBlock, Buffer);
    if (EFI_ERROR (Status)) {
        USB_DEBUG((EFI_D_ERROR, "UsbMassWriteBlocks: UsbBootWriteBlocks (%r) -> Reset\n", Status));
        UsbMassReset (This, TRUE);
    }

ON_EXIT:
    return Status;
}

EFI_STATUS
EFIAPI
UsbMassVerifyBlocks(
    IN EFI_BLOCK_IO_PROTOCOL    *This,
    IN UINT32                   MediaId,
    IN EFI_LBA                  Lba,
    IN UINTN                   VerSize
)
{
    USB_MASS_DEVICE     *UsbMass;
    EFI_BLOCK_IO_MEDIA  *Media;
    EFI_STATUS          Status;
    UINTN               TotalBlock;

    UsbMass = USB_MASS_DEVICE_FROM_BLOCKIO (This);
    Media   = &UsbMass->BlockIoMedia;
    TotalBlock = VerSize /Media->BlockSize;
    //
    // If it is a removable media, such as CD-Rom or Usb-Floppy,
    // need to detect the media before each rw. While some of
    // Usb-Flash is marked as removable media.
    //
    //
    if (Media->RemovableMedia == TRUE) {
        Status = UsbBootDetectMedia (UsbMass);
        if (EFI_ERROR (Status)) {
            USB_DEBUG((EFI_D_ERROR, "UsbMassVerifyBlocks: UsbBootDetectMedia (%r)\n", Status));
            goto ON_EXIT;
        }
    }

    //
    // Try to verify the the sectors .
    //
    Status = UsbBootVerifyBlocks (UsbMass, (UINT32) Lba, TotalBlock);
    if (EFI_ERROR (Status)) {
        USB_DEBUG((EFI_D_ERROR, "UsbMassVerifyBlocks: UsbBootVerifyBlocks (%r) -> Reset\n", Status));
        UsbMassReset (This, TRUE);
    }

ON_EXIT:
    return Status;

}

/**
  Flushes all modified data to a physical block device.

  This function implements EFI_BLOCK_IO_PROTOCOL.FlushBlocks().
  USB mass storage device doesn't support write cache,
  so return EFI_SUCCESS directly.

  @param  This                   Indicates a pointer to the calling context.

  @retval EFI_SUCCESS            All outstanding data were written correctly to the device.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to write data.
  @retval EFI_NO_MEDIA           There is no media in the device.

**/
EFI_STATUS
EFIAPI
UsbMassFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
    return EFI_SUCCESS;
}

EFI_STATUS
UsbMassInitNonLun (
    IN  EFI_USB_IO_PROTOCOL *UsbIo,
    IN  USB_MASS_TRANSPORT  *Transport,
    OUT USB_MASS_DEVICE     **UsbMassPtr,
    IN  VOID *Context
)
{
    EFI_STATUS        Status;
    USB_INTERFACE     *UsbIf;
    USB_MASS_DEVICE   *UsbMass;
    VOID              *TestBuffer;

    TestBuffer = NULL;

    UsbIf = USB_INTERFACE_FROM_USBIO (UsbIo);
    UsbMass = AllocateZeroPool (sizeof (USB_MASS_DEVICE));

    if (UsbMass == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }
    InsertTailList (&mUsbMassStorageList, &UsbMass->Link);

    UsbMass->Signature = USB_MASS_SIGNATURE;
    UsbMass->UsbIo = UsbIo;
    UsbMass->BlockIo.Media = &UsbMass->BlockIoMedia;
    UsbMass->BlockIo.Reset = UsbMassReset;
    UsbMass->BlockIo.ReadBlocks = UsbMassReadBlocks;
    UsbMass->BlockIo.WriteBlocks = UsbMassWriteBlocks;
    UsbMass->BlockIo.FlushBlocks = UsbMassFlushBlocks;
    UsbMass->BlockIo.VerifyBlocks = UsbMassVerifyBlocks;
    UsbMass->OpticalStorage = FALSE;
    UsbMass->Transport = Transport;
    UsbMass->Context = Context;
    UsbMass->DevicePath = DuplicateDevicePath (UsbIf->DevicePath);

    //
    // Initialize the media parameter data for EFI_BLOCK_IO_MEDIA of Block I/O Protocol.
    //
    Status = UsbMassInitMedia (UsbMass);
    if (!EFI_ERROR (Status)) {
        //
        // According to USB Mass Storage Specification for Bootability, only following
        // 4 Peripheral Device Types are in spec.
        //
        if ((UsbMass->Pdt != USB_PDT_DIRECT_ACCESS) &&
                (UsbMass->Pdt != USB_PDT_CDROM) &&
                (UsbMass->Pdt != USB_PDT_OPTICAL) &&
                (UsbMass->Pdt != USB_PDT_SIMPLE_DIRECT)) {
            Status = EFI_NOT_READY;
            goto ON_ERROR;
        } else if (UsbMass->Pdt == USB_PDT_DIRECT_ACCESS) {

            //
            // Make ReadBlock test for some disk can not read. just read 512 bytes.
            //
            TestBuffer = AllocateZeroPool (512);
            if (TestBuffer == NULL)
                return EFI_OUT_OF_RESOURCES;

            Status = UsbMassReadBlocks(
                         &UsbMass->BlockIo,
                         UsbMass->BlockIoMedia.MediaId,
                         0,
                         512,
                         TestBuffer
                     );
            //
            // If Status is EFI_INVALID_PARAMETER,the media can not readable,skip it.
            //
            FreePool (TestBuffer);
            if (Status == EFI_INVALID_PARAMETER) {
                USB_DEBUG((EFI_D_ERROR, "UsbMassInitNonLun: ReadBlock Test Fail \n"));
                goto ON_ERROR;
            }
        }
    } else if (Status != EFI_NO_MEDIA) {
        USB_DEBUG((EFI_D_ERROR, "UsbMassInitNonLun: UsbMassInitMedia (%r)\n", Status));
        goto ON_ERROR;
    }

    *UsbMassPtr = UsbMass;
    UsbIf->DriverContext[UsbIf->LunCount] = UsbMass;
    UsbIf->LunCount++;

    return EFI_SUCCESS;

ON_ERROR:
    RemoveEntryList(&UsbMass->Link);
    FreePool (UsbMass);
    return Status;
}

STATIC
EFI_STATUS
UsbMassInitMultiLun (
    IN  EFI_USB_IO_PROTOCOL *UsbIo,
    IN  USB_MASS_TRANSPORT *Transport,
    OUT USB_MASS_DEVICE **UsbMassPtr,
    IN  VOID *Context,
    IN  UINT8 MaxLun
)
{
    UINT8           Index;
    UINT8           MediaCount;
    EFI_STATUS      Status;
    USB_INTERFACE   *UsbIf;
    USB_MASS_DEVICE *UsbMass;
    DEVICE_LOGICAL_UNIT_DEVICE_PATH LunNode;

    MediaCount = 0;
    UsbIf = USB_INTERFACE_FROM_USBIO (UsbIo);

    for (Index = 0; Index <= MaxLun; Index++) {
        USB_DEBUG((EFI_D_INFO, "UsbMassInitMultiLun: Start to initialize No.%d logic unit\n", Index));
        UsbMass = AllocateZeroPool (sizeof (USB_MASS_DEVICE));
        if (UsbMass == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
            goto ON_ERROR;
        }
        UsbMass->Signature = USB_MASS_SIGNATURE;
        UsbMass->UsbIo = UsbIo;
        UsbMass->BlockIo.Media = &UsbMass->BlockIoMedia;
        UsbMass->BlockIo.Reset = UsbMassReset;
        UsbMass->BlockIo.ReadBlocks = UsbMassReadBlocks;
        UsbMass->BlockIo.WriteBlocks = UsbMassWriteBlocks;
        UsbMass->BlockIo.FlushBlocks = UsbMassFlushBlocks;
        UsbMass->BlockIo.VerifyBlocks = UsbMassVerifyBlocks;
        UsbMass->OpticalStorage = FALSE;
        UsbMass->Transport = Transport;
        UsbMass->Context = Context;
        UsbMass->Lun = Index;

        //
        // Initialize the media parameter data for EFI_BLOCK_IO_MEDIA of Block I/O Protocol.
        //
        Status = UsbMassInitMedia (UsbMass);
        if (!EFI_ERROR (Status)) {
            //
            // According to USB Mass Storage Specification for Bootability, only following
            // 4 Peripheral Device Types are in spec.
            //
            if ((UsbMass->Pdt != USB_PDT_DIRECT_ACCESS) &&
                    (UsbMass->Pdt != USB_PDT_CDROM) &&
                    (UsbMass->Pdt != USB_PDT_OPTICAL) &&
                    (UsbMass->Pdt != USB_PDT_SIMPLE_DIRECT)) {
                USB_DEBUG((EFI_D_ERROR, "UsbMassInitMultiLun: Found an unsupported peripheral type[%d]\n", UsbMass->Pdt));
                Status = EFI_NOT_READY;
                goto ON_ERROR;
            }
        } else {
            USB_DEBUG((EFI_D_ERROR, "UsbMassInitMultiLun: UsbMassInitMedia (%r)\n", Status));
            FreePool(UsbMass);
            continue;
        }

        //
        // Create a device path node of device logic unit, and append it
        //
        LunNode.Header.Type = MESSAGING_DEVICE_PATH;
        LunNode.Header.SubType = MSG_DEVICE_LOGICAL_UNIT_DP;
        LunNode.Lun = UsbMass->Lun;

        SetDevicePathNodeLength (&LunNode.Header, sizeof (LunNode));
        UsbMass->DevicePath = AppendDevicePathNode (UsbIf->DevicePath, &LunNode.Header);
  
       if (UsbMass->DevicePath == NULL) {
           USB_DEBUG ((EFI_D_ERROR, "UsbMassInitMultiLun: failed to create device logic unit device path\n"));
           Status = EFI_OUT_OF_RESOURCES;
           goto ON_ERROR;
       }

        UsbIf->DriverContext[UsbIf->LunCount] = UsbMass;
        UsbIf->LunCount++;
        InsertTailList (&mUsbMassStorageList, &UsbMass->Link);
        USB_DEBUG((EFI_D_INFO, "UsbMassInitMultiLun: Success to initialize No.%d logic unit\n", Index));
        MediaCount++;
    }

    return EFI_SUCCESS;

ON_ERROR:
    if (UsbMass != NULL) {
        if (UsbMass->DevicePath != NULL) {
            FreePool (UsbMass->DevicePath);
        }
        FreePool (UsbMass);
    }

    //
    // If only success to initialize one lun, return success, or else return error
    //
    if (MediaCount > 0) {
        return EFI_SUCCESS;
    } else {
        return Status;
    }
}

EFI_STATUS
UsbConnectMassDriver(
    USB_INTERFACE        *UsbIf
)
/*++

Routine Description:

  Start the USB mass storage device on the controller. It will
  install a BLOCK_IO protocol on the device if everything is OK.

Arguments:

  This                - The USB mass storage driver binding.
  RemainingDevicePath - The remaining device path.

Returns:

  EFI_SUCCESS          - The driver has started on the device.
  EFI_OUT_OF_RESOURCES - Failed to allocate memory
  Others               - Failed to start the driver on the device.

--*/
{
    UINTN                        Index;
    UINT8                        MaxLun;
    VOID                         *Context;
    EFI_STATUS                   Status;
    USB_MASS_DEVICE              *UsbMass;
    EFI_USB_IO_PROTOCOL          *UsbIo;
    USB_MASS_TRANSPORT           *Transport;
    EFI_USB_INTERFACE_DESCRIPTOR Interface;

    Transport = NULL;
    Context = NULL;
    MaxLun = 0;

    if (!mListInitialized) {
        InitializeListHead(&mUsbMassStorageList);
        mListInitialized = TRUE;
    }

    UsbIo = &UsbIf->UsbIo;

    //
    // Initialize the transport protocols
    //

    Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &Interface);
    if (EFI_ERROR (Status)) {
        USB_DEBUG((EFI_D_ERROR, "USBConnectMassDriver: UsbIo->UsbGetInterfaceDescriptor (%r)\n", Status));
        goto ON_ERROR;
    }

    Status = EFI_UNSUPPORTED;
    for (Index = 0; mUsbMassTransport[Index] != NULL; Index++) {
        Transport = mUsbMassTransport[Index];
        if (Interface.InterfaceProtocol == Transport->Protocol) {
            Status = Transport->Init (UsbIo, &Context);
            break;
        }
    }

    if (EFI_ERROR (Status)) {
        USB_DEBUG((EFI_D_ERROR, "USBConnectMassDriver: Transport->Init (%r)\n", Status));
        goto ON_ERROR;
    }

    //
    // For bot device, try to get max lun.
    // If maxlun=0, then non-lun device, else multi-lun device.
    //
    if (Transport->Protocol == USB_MASS_STORE_BOT) {
        Transport->GetMaxLun (Context, &MaxLun);
        USB_DEBUG((EFI_D_INFO, "UsbMassInitTransport: GetMaxLun = %d\n", MaxLun));
    }
    //
    // Logical Unit Numbers on the device shall be numbered contiguously starting from LUN
    // 0 to a maximum LUN of 15 (Fh). So if MaxLun more than 15, we should set it to 0.
    //
    if (MaxLun > 0x0F) {
      MaxLun = 0;
    }
    if (MaxLun == 0) {
        //
        // Initialize No/Unsupported LUN device
        //
        Status = UsbMassInitNonLun (UsbIo, Transport, &UsbMass, Context);
        if (EFI_ERROR (Status)) {
            USB_DEBUG((EFI_D_ERROR, "USBMassDriverBindingStart: UsbMassInitNonLun (%r)\n", Status));
            return EFI_NOT_READY;
        }
    } else {
        //
        // Try best to initialize all LUNs, and return success only if one of LUNs successed to initialized.
        //
        Status = UsbMassInitMultiLun (
                     UsbIo,
                     Transport,
                     &UsbMass,
                     Context,
                     MaxLun
                 );

        if (EFI_ERROR (Status)) {
            USB_DEBUG((EFI_D_ERROR, "USBConnectMassDriver: UsbMassInitMultiLun (%r)\n", Status));
            return EFI_NOT_READY;
        }
    }

    return EFI_SUCCESS;

ON_ERROR:
    return Status;
}

EFI_STATUS
UsbDisconnectMassDriver (
    USB_INTERFACE        *UsbIf
)
/*++

Routine Description:

  Stop controlling the device.

Arguments:

  This              - The USB mass storage driver binding
  NumberOfChildren  - The number of children of this device
  ChildHandleBuffer - The buffer of children handle.

Returns:

  EFI_SUCCESS - The driver stopped from controlling the device.
  Others      - Failed to stop the driver

--*/
{
    USB_MASS_DEVICE  *UsbMass;
    UINT8            LunIndex;

    for (LunIndex = 0; LunIndex < UsbIf->LunCount; LunIndex++) {
        UsbMass = (USB_MASS_DEVICE *)UsbIf->DriverContext[LunIndex];
        if (LunIndex == 0) {
            UsbMass->Transport->Fini (UsbMass->Context);
        }
        RemoveEntryList(&UsbMass->Link);
        FreePool (UsbMass->DevicePath);
        FreePool (UsbMass);
    }
    return EFI_SUCCESS;
}
