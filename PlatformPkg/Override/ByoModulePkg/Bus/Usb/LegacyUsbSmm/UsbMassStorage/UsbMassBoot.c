/** @file
  Implementation of the command set of USB Mass Storage Specification
  for Bootability, Revision 1.0.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UsbMassImpl.h"

#include <library/UefiBootServicesTableLib.h>

STATIC
UINT32
UsbBootGetUint32 (
    IN UINT8                  *Buf
)
/*++

Routine Description:

  Read an UINT32 from the buffer to avoid byte alignment problems, then
  convert that to the little endia. The USB mass storage bootability spec
  use big endia

Arguments:

  Buf - The buffer contains the first byte of the UINT32 in big endia.

Returns:

  The UINT32 value read from the buffer in little endia.

--*/
{
    UINT32                    Value;

    CopyMem (&Value, Buf, sizeof (UINT32));
    return USB_BOOT_SWAP32 (Value);
}

STATIC
VOID
UsbBootPutUint32 (
    IN UINT8                  *Buf,
    IN UINT32                 Data32
)
/*++

Routine Description:

  Put an UINT32 in little endia to the buffer. The data is converted to
  big endia before writing.

Arguments:

  Buf     - The buffer to write data to
  Data32  - The data to write.

Returns:

  None

--*/
{
    Data32 = USB_BOOT_SWAP32 (Data32);
    CopyMem (Buf, &Data32, sizeof (UINT32));
}

STATIC
UINT16
UsbBootGetUint16 (
    IN UINT8                  *Buf
)
/*++

Routine Description:

  Read an UINT32 from the buffer to avoid byte alignment problems, then
  convert that to the little endia. The USB mass storage bootability spec
  use big endia

Arguments:

  Buf - The buffer contains the first byte of the UINT32 in big endia.

Returns:

  The UINT32 value read from the buffer in little endia.

--*/
{
    UINT16                    Value;

    CopyMem (&Value, Buf, sizeof (UINT16));
    return USB_BOOT_SWAP16 (Value);
}

STATIC
VOID
UsbBootPutUint16 (
    IN UINT8                   *Buf,
    IN UINT16                  Data16
)
/*++

Routine Description:

  Put an UINT16 in little endia to the buffer. The data is converted to
  big endia before writing.

Arguments:

  Buf     - The buffer to write data to
  Data16  - The data to write

Returns:

  None

--*/
{
    Data16 = USB_BOOT_SWAP16 (Data16);
    CopyMem (Buf, &Data16, sizeof (UINT16));
}

/**
  Execute REQUEST SENSE Command to retrieve sense data from device.

  @param  UsbMass                The device whose sense data is requested.

  @retval EFI_SUCCESS            The command is excuted successfully.
  @retval EFI_DEVICE_ERROR       Failed to request sense.
  @retval EFI_NO_RESPONSE        The device media doesn't response this request.
  @retval EFI_INVALID_PARAMETER  The command has some invalid parameters.
  @retval EFI_WRITE_PROTECTED    The device is write protected.
  @retval EFI_MEDIA_CHANGED      The device media has been changed.

**/
EFI_STATUS
UsbBootRequestSense (
    IN USB_MASS_DEVICE          *UsbMass,
    USB_BOOT_REQUEST_SENSE_DATA *SenseData
)
{
    USB_BOOT_REQUEST_SENSE_CMD  SenseCmd;
    EFI_BLOCK_IO_MEDIA          *Media;
    USB_MASS_TRANSPORT          *Transport;
    EFI_STATUS                  Status;
    UINT32                      CmdResult;

    Transport = UsbMass->Transport;

    //
    // Request the sense data from the device if command failed
    //
    ZeroMem (&SenseCmd, sizeof (USB_BOOT_REQUEST_SENSE_CMD));
    ZeroMem (SenseData, sizeof (USB_BOOT_REQUEST_SENSE_DATA));

    SenseCmd.OpCode   = USB_BOOT_REQUEST_SENSE_OPCODE;
    SenseCmd.Lun      = (UINT8) (USB_BOOT_LUN (UsbMass->Lun));
    SenseCmd.AllocLen = (UINT8) sizeof (USB_BOOT_REQUEST_SENSE_DATA);

    Status = Transport->ExecCommand (
                 UsbMass->Context,
                 &SenseCmd,
                 sizeof (USB_BOOT_REQUEST_SENSE_CMD),
                 EfiUsbDataIn,
                 SenseData,
                 sizeof (USB_BOOT_REQUEST_SENSE_DATA),
                 UsbMass->Lun,
                 USB_BOOT_GENERAL_CMD_TIMEOUT,
                 &CmdResult
             );
    if (EFI_ERROR (Status) || CmdResult != USB_MASS_CMD_SUCCESS) {
        USB_DEBUG((EFI_D_ERROR, "UsbBootRequestSense: (%r) CmdResult=0x%x\n", Status, CmdResult));
        if (!EFI_ERROR (Status)) {
          Status = EFI_DEVICE_ERROR;
        }
        return Status;
    }

  //
  // If sense data is retrieved successfully, interpret the sense data
  // and update the media status if necessary.
  //
    Media = &UsbMass->BlockIoMedia;

    switch (USB_BOOT_SENSE_KEY (SenseData->SenseKey)) {

    case USB_BOOT_SENSE_NO_SENSE:
        Status = EFI_NO_RESPONSE;
        break;

    case USB_BOOT_SENSE_RECOVERED:
        //
        // Suppose hardware can handle this case, and recover later by itself
        //
        Status = EFI_NOT_READY;
        break;

    case USB_BOOT_SENSE_NOT_READY:
        Status = EFI_DEVICE_ERROR;
        if (SenseData->Asc == USB_BOOT_ASC_NO_MEDIA) {
            Media->MediaPresent = FALSE;
            Status              = EFI_NO_MEDIA;
        } else if (SenseData->Asc == USB_BOOT_ASC_NOT_READY) {
            Status              = EFI_NOT_READY;
        }
        break;

    case USB_BOOT_SNESE_MEDIUM_ERROR:
        //
        // If the media has Error, means the media is not present.
        //
        Status = EFI_NO_MEDIA;
        Media->MediaPresent = FALSE;
        break;

    case USB_BOOT_SENSE_ILLEGAL_REQUEST:
        Status = EFI_INVALID_PARAMETER;
        break;

    case USB_BOOT_SENSE_UNIT_ATTENTION:
        Status = EFI_DEVICE_ERROR;
        if (SenseData->Asc == USB_BOOT_ASC_MEDIA_CHANGE) {
            //
            // If MediaChange, reset ReadOnly and new MediId
            //
            Status          = EFI_MEDIA_CHANGED;
            Media->ReadOnly = FALSE;
            Media->MediaId++;
        }
        break;

    case USB_BOOT_SENSE_DATA_PROTECT:
        Status          = EFI_WRITE_PROTECTED;
        Media->ReadOnly = TRUE;
        break;

    default:
        Status = EFI_DEVICE_ERROR;
        break;
    }

    USB_DEBUG((EFI_D_INFO, "UsbBootRequestSense: (%r) with sense key %x/%x/%x\n",
            Status,
            USB_BOOT_SENSE_KEY (SenseData->SenseKey),
            SenseData->Asc,
            SenseData->Ascq
           ));

    return Status;
}


/**
  Execute the USB mass storage bootability commands.

  This function executes the USB mass storage bootability commands.
  If execution failed, retrieve the error by REQUEST_SENSE, then
  update the device's status, such as ReadyOnly.

  @param  UsbMass                The device to issue commands to
  @param  Cmd                    The command to execute
  @param  CmdLen                 The length of the command
  @param  DataDir                The direction of data transfer
  @param  Data                   The buffer to hold the data
  @param  DataLen                The length of expected data
  @param  Timeout                The timeout used to transfer

  @retval EFI_SUCCESS            Command is excuted successfully
  @retval Others                 Command execution failed.

**/
EFI_STATUS
UsbBootExecCmd (
    IN USB_MASS_DEVICE            *UsbMass,
    IN VOID                       *Cmd,
    IN UINT8                      CmdLen,
    IN EFI_USB_DATA_DIRECTION     DataDir,
    IN VOID                       *Data,
    IN UINT32                     DataLen,
    IN UINT32                     Timeout
)
{
    USB_MASS_TRANSPORT          *Transport;
    USB_BOOT_REQUEST_SENSE_DATA *SenseData;
    EFI_STATUS                  Status;
    UINT32                      CmdResult;

    SenseData = (USB_BOOT_REQUEST_SENSE_DATA*)UsbAllocatePool(sizeof(USB_BOOT_REQUEST_SENSE_DATA));
    Transport = UsbMass->Transport;
    Status    = Transport->ExecCommand (
                    UsbMass->Context,
                    Cmd,
                    CmdLen,
                    DataDir,
                    Data,
                    DataLen,
                    UsbMass->Lun,
                    Timeout,
                    &CmdResult
                );

    if (Status == EFI_TIMEOUT) {
      DEBUG ((EFI_D_ERROR, "UsbBootExecCmd: Timeout to Exec 0x%x Cmd\n", *(UINT8 *)Cmd));
      return EFI_TIMEOUT;
    }

    //
    // ExecCommand return success and get the right CmdResult means
    // the commnad transfer is OK.
    //
    if ((CmdResult == USB_MASS_CMD_SUCCESS) && !EFI_ERROR(Status)) {
        UsbFreePool(SenseData, sizeof(USB_BOOT_REQUEST_SENSE_DATA));
        return EFI_SUCCESS;
    }

    USB_DEBUG((EFI_D_INFO, "UsbBootExecCmd: Fail to Exec 0x%x Cmd /w %r\n",
            *(UINT8 *)Cmd ,Status));

    Status = UsbBootRequestSense (UsbMass, SenseData);
    UsbFreePool(SenseData, sizeof(USB_BOOT_REQUEST_SENSE_DATA));
    return Status;
}


/**
  Execute the USB mass storage bootability commands with retrial.

  This function executes USB mass storage bootability commands.
  If the device isn't ready, wait for it. If the device is ready
  and error occurs, retry the command again until it exceeds the
  limit of retrial times.
  
  @param  UsbMass                The device to issue commands to
  @param  Cmd                    The command to execute
  @param  CmdLen                 The length of the command
  @param  DataDir                The direction of data transfer
  @param  Data                   The buffer to hold the data
  @param  DataLen                The length of expected data
  @param  Timeout                The timeout used to transfer

  @retval EFI_SUCCESS            The command is executed successfully.
  @retval EFI_MEDIA_CHANGED      The device media has been changed.
  @retval Others                 Command execution failed after retrial.

**/
EFI_STATUS
UsbBootExecCmdWithRetry (
    IN USB_MASS_DEVICE          *UsbMass,
    IN VOID                     *Cmd,
    IN UINT8                    CmdLen,
    IN EFI_USB_DATA_DIRECTION   DataDir,
    IN VOID                     *Data,
    IN UINT32                   DataLen,
    IN UINT32                   Timeout
)
{
    EFI_STATUS                  Status;
    INT16                       Index;

    //
    // If the device isn't ready, wait some time. If the device is ready,
    // retry the command again.
    //
    Status  = EFI_SUCCESS;

    for (Index = 0; Index < USB_BOOT_COMMAND_RETRY; Index++) {
        //
        // Execute the command with an increasingly larger timeout value.
        //
        Status = UsbBootExecCmd (
                     UsbMass,
                     Cmd,
                     CmdLen,
                     DataDir,
                     Data,
                     DataLen,
                     Timeout
                 );
        if (Status == EFI_SUCCESS || Status == EFI_MEDIA_CHANGED || Status == EFI_NO_MEDIA) {
          break;
        }
        //
        // Need retry once more, so reset index
        //
        if (Status == EFI_NOT_READY) {
            Index = 0;
        }
    }

    return Status;
}


/**
  Execute TEST UNIT READY command to check if the device is ready.

  @param  UsbMass                The device to test

  @retval EFI_SUCCESS            The device is ready.
  @retval Others                 Device not ready.

**/
EFI_STATUS
UsbBootIsUnitReady (
    IN USB_MASS_DEVICE            *UsbMass
  )
{
    USB_BOOT_TEST_UNIT_READY_CMD  TestCmd;

    ZeroMem (&TestCmd, sizeof (USB_BOOT_TEST_UNIT_READY_CMD));

    TestCmd.OpCode  = USB_BOOT_TEST_UNIT_READY_OPCODE;
    TestCmd.Lun     = (UINT8) (USB_BOOT_LUN (UsbMass->Lun));

    return UsbBootExecCmdWithRetry (
               UsbMass,
               &TestCmd,
               (UINT8) sizeof (USB_BOOT_TEST_UNIT_READY_CMD),
               EfiUsbNoData,
               NULL,
               0,
               USB_BOOT_GENERAL_CMD_TIMEOUT
           );
}


/**
  Execute INQUIRY Command to request information regarding parameters of
  the device be sent to the host computer.

  @param  UsbMass                The device to inquire.

  @retval EFI_SUCCESS            INQUIRY Command is executed successfully.
  @retval Others                 INQUIRY Command is not executed successfully.

**/
EFI_STATUS
UsbBootInquiry (
  IN USB_MASS_DEVICE            *UsbMass
  )
{
    USB_BOOT_INQUIRY_CMD        InquiryCmd;
    USB_BOOT_INQUIRY_DATA       *InquiryData;
    EFI_BLOCK_IO_MEDIA          *Media;
    EFI_STATUS                  Status;

    Media = &(UsbMass->BlockIoMedia);

    InquiryData = UsbAllocatePool (sizeof(USB_BOOT_INQUIRY_DATA));
    if (InquiryData == NULL)
        return EFI_OUT_OF_RESOURCES;

    //
    // Use the Inquiry command to get the RemovableMedia setting.
    //
    ZeroMem (&InquiryCmd, sizeof (USB_BOOT_INQUIRY_CMD));
    ZeroMem (InquiryData, sizeof (USB_BOOT_INQUIRY_DATA));

    InquiryCmd.OpCode   = USB_BOOT_INQUIRY_OPCODE;
    InquiryCmd.Lun      = (UINT8) (USB_BOOT_LUN (UsbMass->Lun));
    InquiryCmd.AllocLen = (UINT8) sizeof (USB_BOOT_INQUIRY_DATA);

    Status = UsbBootExecCmdWithRetry (
                 UsbMass,
                 &InquiryCmd,
                (UINT8) sizeof (USB_BOOT_INQUIRY_CMD),
                 EfiUsbDataIn,
                 InquiryData,
                 sizeof (USB_BOOT_INQUIRY_DATA),
                 USB_BOOT_GENERAL_CMD_TIMEOUT
             );
    if (EFI_ERROR (Status)) {
        UsbFreePool (InquiryData, sizeof (USB_BOOT_INQUIRY_DATA));
        return Status;
    }

  //
  // Get information from PDT (Peripheral Device Type) field and Removable Medium Bit
  // from the inquiry data.
  //
    UsbMass->Pdt          = (UINT8) (USB_BOOT_PDT (InquiryData->Pdt));
    CopyMem (UsbMass->ProductID, InquiryData->ProductID, 16);
    CopyMem (UsbMass->VendorID, InquiryData->VendorID, 8);
    Media->RemovableMedia =  (BOOLEAN) (USB_BOOT_REMOVABLE (InquiryData->Removable));

    //
    // Default value 512 Bytes, in case no media present at first time
    //
    Media->BlockSize      = 0x0200;

    UsbFreePool (InquiryData, sizeof (USB_BOOT_INQUIRY_DATA));
    return Status;
}


/**
  Execute READ CAPACITY command to request information regarding
  the capacity of the installed medium of the device.

  This function executes READ CAPACITY command to get the capacity
  of the USB mass storage media, including the presence, block size,
  and last block number.

  @param  UsbMass                The device to retireve disk gemotric.

  @retval EFI_SUCCESS            The disk geometry is successfully retrieved.
  @retval EFI_NOT_READY          The returned block size is zero.
  @retval Other                  READ CAPACITY command execution failed.
 
**/
EFI_STATUS
UsbBootReadCapacity (
  IN USB_MASS_DEVICE          *UsbMass
  )
{
    USB_BOOT_READ_CAPACITY_CMD  CapacityCmd;
    USB_BOOT_READ_CAPACITY_DATA *CapacityData;
    EFI_BLOCK_IO_MEDIA          *Media;
    EFI_STATUS                  Status;

    Media   = &UsbMass->BlockIoMedia;

    CapacityData = UsbAllocatePool (sizeof(USB_BOOT_READ_CAPACITY_DATA));
    if (CapacityData == NULL)
        return EFI_OUT_OF_RESOURCES;

    //
    // Use the READ CAPACITY command to get the block length and last blockno
    //
    ZeroMem (&CapacityCmd, sizeof (USB_BOOT_READ_CAPACITY_CMD));
    ZeroMem (CapacityData, sizeof (USB_BOOT_READ_CAPACITY_DATA));

    CapacityCmd.OpCode = USB_BOOT_READ_CAPACITY_OPCODE;
    CapacityCmd.Lun    = (UINT8) (USB_BOOT_LUN (UsbMass->Lun));

    Status = UsbBootExecCmdWithRetry (
                 UsbMass,
                 &CapacityCmd,
                 (UINT8) sizeof (USB_BOOT_READ_CAPACITY_CMD),
                 EfiUsbDataIn,
                 CapacityData,
                 sizeof (USB_BOOT_READ_CAPACITY_DATA),
                 USB_BOOT_GENERAL_CMD_TIMEOUT
             );
    if (EFI_ERROR (Status)) {
        UsbFreePool (CapacityData, sizeof (USB_BOOT_READ_CAPACITY_DATA));
        return Status;
    }

    //
    // Get the information on media presence, block size, and last block number
    // from READ CAPACITY data.
    //
    Media->MediaPresent = TRUE;
    Media->LastBlock    = UsbBootGetUint32 (CapacityData->LastLba);
    Media->BlockSize    = UsbBootGetUint32 (CapacityData->BlockLen);

    if (Media->LastBlock == 0) {
        if (Media->BlockSize == 0) {
            Media->BlockSize = 512;
        }
        Media->MediaPresent = FALSE;
        return EFI_NOT_READY;
    }
    if (Media->BlockSize == 0) {
        Media->BlockSize = 512;
        return EFI_NOT_READY;
    }

    UsbFreePool (CapacityData, sizeof (USB_BOOT_READ_CAPACITY_DATA));
    return Status;
}

/**
  Retrieves SCSI mode sense information via MODE SENSE(6) command.

  @param  UsbMass                The device whose sense data is requested.

  @retval EFI_SUCCESS            SCSI mode sense information retrieved successfully.
  @retval Other                  Command execution failed.

**/
EFI_STATUS
UsbScsiModeSense (
    IN USB_MASS_DEVICE          *UsbMass
)
{
    EFI_STATUS                       Status;
    USB_SCSI_MODE_SENSE6_CMD         ModeSenseCmd;
    USB_SCSI_MODE_SENSE6_PARA_HEADER *ModeParaHeader;
    EFI_BLOCK_IO_MEDIA               *Media;

    Media   = &UsbMass->BlockIoMedia;

    ModeParaHeader = UsbAllocatePool (sizeof(USB_SCSI_MODE_SENSE6_PARA_HEADER));
    if (ModeParaHeader == NULL)
        return EFI_OUT_OF_RESOURCES;

    ZeroMem (&ModeSenseCmd, sizeof (USB_SCSI_MODE_SENSE6_CMD));
    ZeroMem (ModeParaHeader, sizeof (USB_SCSI_MODE_SENSE6_PARA_HEADER));

    //
    // ModeSense6 command is defined in [SCSI2Spec-Page151]
    //
    ModeSenseCmd.OpCode         = USB_SCSI_MODE_SENSE6_OPCODE;
    ModeSenseCmd.Lun            = (UINT8) USB_BOOT_LUN (UsbMass->Lun);
    ModeSenseCmd.PageCode       = 0x3F;
    ModeSenseCmd.AllocateLen    = (UINT8) sizeof (USB_SCSI_MODE_SENSE6_PARA_HEADER);

    Status = UsbBootExecCmdWithRetry (
                 UsbMass,
                 &ModeSenseCmd,
                 (UINT8) sizeof (USB_SCSI_MODE_SENSE6_CMD),
                 EfiUsbDataIn,
                 ModeParaHeader,
                 sizeof (USB_SCSI_MODE_SENSE6_PARA_HEADER),
                 USB_BOOT_GENERAL_CMD_TIMEOUT
             );

    //
    // Format of device-specific parameter byte of the mode parameter header is defined in
    // Section 8.2.10 of SCSI-2 Spec.
    // BIT7 of this byte is indicates whether the medium is write protected.
    //
    if (!EFI_ERROR (Status)) {
      Media->ReadOnly = (BOOLEAN) ((ModeParaHeader->DevicePara & BIT7) != 0);
    }

    UsbFreePool (ModeParaHeader, sizeof (USB_SCSI_MODE_SENSE6_PARA_HEADER));
    return Status;
}

EFI_STATUS
UsbBootModeSense (
    IN USB_MASS_DEVICE                 *UsbMass,
    IN USB_BOOT_MODE_PARALIST_PAGE5*   ModeParaListPage5
)
/*++

  Routine Description:
    Retrieves mode sense information via sending Mode Sense
    Packet Command.

  Arguments:
    UsbMass          - The USB_FLOPPY_DEV instance.

  Returns:
    EFI_DEVICE_ERROR - Hardware error
    EFI_SUCCESS      - Success

--*/
{
    EFI_STATUS                    Status;
    USB_BOOT_MODE_SENSE10_CMD     ModeSenseCmd;

    ZeroMem (&ModeSenseCmd, sizeof (USB_BOOT_MODE_SENSE10_CMD));
    ZeroMem (ModeParaListPage5, sizeof (USB_BOOT_MODE_PARALIST_PAGE5));

    ModeSenseCmd.OpCode         = USB_BOOT_MODE_SENSE10_OPCODE;
    ModeSenseCmd.PageCode       = 0x05;
    ModeSenseCmd.ParaListLenLsb = (UINT8) sizeof (USB_BOOT_MODE_PARALIST_PAGE5);

    Status = UsbBootExecCmdWithRetry (
                 UsbMass,
                 &ModeSenseCmd,
                 (UINT8) sizeof (USB_BOOT_MODE_SENSE10_CMD),
                 EfiUsbDataIn,
                 ModeParaListPage5,
                 sizeof (USB_BOOT_MODE_PARALIST_PAGE5),
                 USB_BOOT_GENERAL_CMD_TIMEOUT
             );

    if (!EFI_ERROR (Status))
    {
        UsbMass->BlockIoMedia.BlockSize = UsbBootGetUint16((UINT8*)&ModeParaListPage5->BytesPerSector);
        UsbMass->BlockIoMedia.LastBlock  = ModeParaListPage5->HeaderNumber * ModeParaListPage5->SectorNumber \
                                           * UsbBootGetUint16((UINT8*)&ModeParaListPage5->CylinderNumber) - 1;
    }

    return Status;
}

/**
  Get the parameters for the USB mass storage media.

  This function get the parameters for the USB mass storage media,
  It is used both to initialize the media during the Start() phase
  of Driver Binding Protocol and to re-initialize it when the media is
  changed. Althought the RemoveableMedia is unlikely to change,
  it is also included here.

  @param  UsbMass                The device to retrieve disk gemotric.

  @retval EFI_SUCCESS            The disk gemotric is successfully retrieved.
  @retval Other                  Failed to get the parameters.

**/
EFI_STATUS
UsbBootGetParams (
    IN USB_MASS_DEVICE          *UsbMass
)
{
  EFI_BLOCK_IO_MEDIA          *Media;
  EFI_STATUS                  Status;

  Media  = &(UsbMass->BlockIoMedia);

    Status = UsbBootInquiry (UsbMass);
    if (EFI_ERROR (Status)) {
        USB_DEBUG((EFI_D_ERROR, "UsbBootGetParams: UsbBootInquiry (%r)\n", Status));
        return Status;
    }

    //
    // Don't use the Removable bit in inquirydata to test whether the media
    // is removable because many flash disks wrongly set this bit.
    //
    if ((UsbMass->Pdt == USB_PDT_CDROM) || (UsbMass->Pdt == USB_PDT_OPTICAL)) {
        //
        // CD-Rom device and Non-CD optical device
        //
        UsbMass->OpticalStorage = TRUE;
        //
        // Default value 2048 Bytes, in case no media present at first time
        //
        Media->BlockSize        = 0x0800;
    }

    Status = UsbBootDetectMedia (UsbMass);

    return Status;
}


/**
  Detect whether the removable media is present and whether it has changed.

  @param  UsbMass                The device to check.

  @retval EFI_SUCCESS            The media status is successfully checked.
  @retval Other                  Failed to detect media.

**/
EFI_STATUS
UsbBootDetectMedia (
    IN  USB_MASS_DEVICE       *UsbMass
  )
{
    EFI_BLOCK_IO_MEDIA        OldMedia;
    EFI_BLOCK_IO_MEDIA        *Media;
    UINT8                     CmdSet;
    EFI_STATUS                Status;

    Media    = &UsbMass->BlockIoMedia;

    CopyMem (&OldMedia, &(UsbMass->BlockIoMedia), sizeof (EFI_BLOCK_IO_MEDIA));
    
    CmdSet = ((EFI_USB_INTERFACE_DESCRIPTOR *) (UsbMass->Context))->InterfaceSubClass;

    Status = UsbBootIsUnitReady (UsbMass);
    if (EFI_ERROR (Status) && (Status != EFI_MEDIA_CHANGED)) {
        USB_DEBUG((EFI_D_ERROR, "UsbBootDetectMedia: UsbBootIsUnitReady (%r)\n", Status));
        goto ON_ERROR;
    }

    if ((UsbMass->Pdt != USB_PDT_CDROM) && (CmdSet == USB_MASS_STORE_SCSI)) {
        //
        // ModeSense is required for the device with PDT of 0x00/0x07/0x0E,
        // which is from [MassStorageBootabilitySpec-Page7].
        // ModeSense(10) is useless here, while ModeSense(6) defined in SCSI
        // could get the information of WriteProtected.
        // Since not all device support this command, so skip if fail.
        //
        UsbScsiModeSense (UsbMass);
    }

    Status = UsbBootReadCapacity (UsbMass);
    if (EFI_ERROR (Status)) {
        USB_DEBUG((EFI_D_ERROR, "UsbBootDetectMedia: UsbBootReadCapacity (%r)\n", Status));
        goto ON_ERROR;
    }

    return EFI_SUCCESS;

ON_ERROR:
#if 0

    //
    // Detect whether it is necessary to reinstall the BlockIO
    //
    // MediaId may change in RequestSense for MediaChanged
    // MediaPresent may change in RequestSense for NoMedia
    // MediaReadOnly may change in RequestSense for WriteProtected or MediaChanged
    // MediaPresent/BlockSize/LastBlock may change in ReadCapacity
    //
    if ((Media->MediaId != OldMedia.MediaId) ||
            (Media->MediaPresent != OldMedia.MediaPresent) ||
            (Media->ReadOnly != OldMedia.ReadOnly) ||
            (Media->BlockSize != OldMedia.BlockSize) ||
            (Media->LastBlock != OldMedia.LastBlock)) {

        gBS->ReinstallProtocolInterface (
            UsbMass->Controller,
            &gEfiBlockIoProtocolGuid,
            &UsbMass->BlockIo,
            &UsbMass->BlockIo
        );
        //
        // Update MediaId after reinstall BLOCK_IO_PROTOCOL
        //
        if (Media->MediaPresent != OldMedia.MediaPresent) {
            if (Media->MediaPresent) {
                Media->MediaId = 1;
            } else {
                Media->MediaId = 0;
            }
        }

        if ((Media->ReadOnly != OldMedia.ReadOnly) ||
                (Media->BlockSize != OldMedia.BlockSize) ||
                (Media->LastBlock != OldMedia.LastBlock)) {
            Media->MediaId++;
        }
    }
#endif

    return Status;
}


/**
  Read some blocks from the device.

  @param  UsbMass                The USB mass storage device to read from
  @param  Lba                    The start block number
  @param  TotalBlock             Total block number to read
  @param  Buffer                 The buffer to read to

  @retval EFI_SUCCESS            Data are read into the buffer
  @retval Others                 Failed to read all the data

**/
EFI_STATUS
UsbBootReadBlocks (
    IN  USB_MASS_DEVICE       *UsbMass,
    IN  UINT32                Lba,
    IN  UINTN                 TotalBlock,
    OUT UINT8                 *Buffer
)
{
    USB_BOOT_READ10_CMD       ReadCmd;
    EFI_STATUS                Status;
    UINT16                    Count;
    UINT32                    BlockSize;
    UINT32                    ByteSize;
    UINT32                    Timeout;

    BlockSize = UsbMass->BlockIoMedia.BlockSize;
    Status    = EFI_SUCCESS;

    while (TotalBlock > 0) {
        //
        // Split the total blocks into smaller pieces to ease the pressure
        // on the device. We must split the total block because the READ10
        // command only has 16 bit transfer length (in the unit of block).
        //
        Count     = (UINT16)((TotalBlock < USB_BOOT_IO_BLOCKS) ? TotalBlock : USB_BOOT_IO_BLOCKS);
        ByteSize  = (UINT32)Count * BlockSize;

        //
        // USB command's upper limit timeout is 5s. [USB2.0-9.2.6.1]
        //
        Timeout = (UINT32) USB_BOOT_GENERAL_CMD_TIMEOUT;

        //
        // Fill in the command then execute
        //
        ZeroMem (&ReadCmd, sizeof (USB_BOOT_READ10_CMD));

        ReadCmd.OpCode  = USB_BOOT_READ10_OPCODE;
        ReadCmd.Lun     = (UINT8) (USB_BOOT_LUN (UsbMass->Lun));
        UsbBootPutUint32 (ReadCmd.Lba, Lba);
        UsbBootPutUint16 (ReadCmd.TransferLen, Count);

        Status = UsbBootExecCmdWithRetry (
                     UsbMass,
                     &ReadCmd,
                     (UINT8) sizeof (USB_BOOT_READ10_CMD),
                     EfiUsbDataIn,
                     Buffer,
                     ByteSize,
                     Timeout
                 );
        if (EFI_ERROR (Status)) {
            return Status;
        }
        DEBUG ((EFI_D_BLKIO, "UsbBootReadBlocks: LBA (0x%x), Blk (0x%x)\n", Lba, TotalBlock));
        Lba        += Count;
        Buffer     += Count * BlockSize;
        TotalBlock -= Count;
    }

    return Status;
}


/**
  Write some blocks to the device.

  @param  UsbMass                The USB mass storage device to write to
  @param  Lba                    The start block number
  @param  TotalBlock             Total block number to write
  @param  Buffer                 Pointer to the source buffer for the data.

  @retval EFI_SUCCESS            Data are written into the buffer
  @retval Others                 Failed to write all the data

**/
EFI_STATUS
UsbBootWriteBlocks (
    IN  USB_MASS_DEVICE         *UsbMass,
    IN  UINT32                  Lba,
    IN  UINTN                   TotalBlock,
    OUT UINT8                   *Buffer
    )
{
    USB_BOOT_WRITE10_CMD  WriteCmd;
    EFI_STATUS            Status;
    UINT16                Count;
    UINT32                BlockSize;
    UINT32                ByteSize;
    UINT32                Timeout;

    BlockSize = UsbMass->BlockIoMedia.BlockSize;
    Status    = EFI_SUCCESS;

    while (TotalBlock > 0) {
        //
        // Split the total blocks into smaller pieces to ease the pressure
        // on the device. We must split the total block because the WRITE10
        // command only has 16 bit transfer length (in the unit of block).
        //
        Count     = (UINT16)((TotalBlock < USB_BOOT_IO_BLOCKS) ? TotalBlock : USB_BOOT_IO_BLOCKS);
        ByteSize  = (UINT32)Count * BlockSize;

        //
        // USB command's upper limit timeout is 5s. [USB2.0-9.2.6.1]
        //
        Timeout = (UINT32) USB_BOOT_GENERAL_CMD_TIMEOUT;

        //
        // Fill in the write10 command block
        //
        ZeroMem (&WriteCmd, sizeof (USB_BOOT_WRITE10_CMD));

        WriteCmd.OpCode = USB_BOOT_WRITE10_OPCODE;
        WriteCmd.Lun    = (UINT8) (USB_BOOT_LUN (UsbMass->Lun));
        UsbBootPutUint32 (WriteCmd.Lba, Lba);
        UsbBootPutUint16 (WriteCmd.TransferLen, Count);

        Status = UsbBootExecCmdWithRetry (
                     UsbMass,
                     &WriteCmd,
                     (UINT8) sizeof (USB_BOOT_WRITE10_CMD),
                     EfiUsbDataOut,
                     Buffer,
                     ByteSize,
                     Timeout
                 );
        if (EFI_ERROR (Status)) {
            return Status;
        }
        DEBUG ((EFI_D_BLKIO, "UsbBootWriteBlocks: LBA (0x%x), Blk (0x%x)\n", Lba, TotalBlock));
        Lba        += Count;
        Buffer     += Count * BlockSize;
        TotalBlock -= Count;
    }

    return Status;
}

EFI_STATUS
UsbBootVerifyBlocks(
    IN  USB_MASS_DEVICE         *UsbMass,
    IN  UINT32                  Lba,
    IN  UINTN                   TotalBlock
)
{
    USB_BOOT_VERIFY_COMMAND  VerifyCmd;
    EFI_STATUS            Status;
    UINT32                Timeout;
    UINT16                Count;

    Status    = EFI_SUCCESS;
    while (TotalBlock > 0) {
        //
        // Split the total blocks into smaller pieces to ease the pressure
        // on the device. We must split the total block because the WRITE10
        // command only has 16 bit transfer length (in the unit of block).
        //
        Count     = (UINT16)((TotalBlock < USB_BOOT_IO_BLOCKS) ? TotalBlock : USB_BOOT_IO_BLOCKS);
        //
        // USB command's upper limit timeout is 5s. [USB2.0-9.2.6.1]
        //
        Timeout = (UINT32) USB_BOOT_GENERAL_CMD_TIMEOUT;

        //
        // Fill in the verify command block
        //
        ZeroMem (&VerifyCmd, sizeof (USB_BOOT_VERIFY_COMMAND));

        VerifyCmd.OpCode = USB_BOOT_VERIFY_CMD;
        VerifyCmd.Lun    = USB_BOOT_LUN (UsbMass->Lun);
        UsbBootPutUint32 (VerifyCmd.Lba, Lba);
        UsbBootPutUint16 (VerifyCmd.TransferLen, Count);

        Status = UsbBootExecCmdWithRetry (
                     UsbMass,
                     &VerifyCmd,
                     sizeof (USB_BOOT_VERIFY_COMMAND),
                     EfiUsbNoData,
                     NULL,
                     0,
                     Timeout
                 );
        Lba        += Count;
        TotalBlock -= Count;
        if (EFI_ERROR (Status)) {
            return Status;
        }
    }

    return Status;

}

/**
  Use the USB clear feature control transfer to clear the endpoint stall condition.

  @param  UsbIo                  The USB I/O Protocol instance
  @param  EndpointAddr           The endpoint to clear stall for

  @retval EFI_SUCCESS            The endpoint stall condition is cleared.
  @retval Others                 Failed to clear the endpoint stall condition.

**/
EFI_STATUS
UsbClearEndpointStall (
    IN EFI_USB_IO_PROTOCOL    *UsbIo,
    IN UINT8                  EndpointAddr
)
{
    EFI_STATUS                Status;
    EFI_USB_DEVICE_REQUEST    *Request;
    UINT32                    CmdResult;
    UINT32                    Timeout;

    Request = UsbAllocatePool (sizeof(EFI_USB_DEVICE_REQUEST));
    if (Request == NULL)
        return EFI_OUT_OF_RESOURCES;

    Request->RequestType = 0x02;
    Request->Request     = USB_REQ_CLEAR_FEATURE;
    Request->Value       = USB_FEATURE_ENDPOINT_HALT;
    Request->Index       = EndpointAddr;
    Request->Length      = 0;
    Timeout              = USB_BOOT_GENERAL_CMD_TIMEOUT / USB_MASS_1_MILLISECOND;

    Status = UsbIo->UsbControlTransfer (
                 UsbIo,
                 Request,
                 EfiUsbNoData,
                 Timeout,
                 NULL,
                 0,
                 &CmdResult
             );
    UsbFreePool (Request, sizeof(EFI_USB_DEVICE_REQUEST));
    return Status;
}
