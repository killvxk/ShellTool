
#include "InternalBdsLib.h"
#include <Guid/ByoGlobalVariable.h>
#include <Guid/BlockIoVendor.h> 
#include <SetupVariable.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Library/PlatformCommLib.h>


CONST UINT16 USB_LANG_ID   = 0x0409; // English
CHAR16       mUefiPrefix[] = L"UEFI ";

EFI_BOOT_MANAGER_REFRESH_LEGACY_BOOT_OPTION  mEfiBootManagerRefreshLegacyBootOption = NULL;
EFI_BOOT_MANAGER_LEGACY_BOOT                 mEfiBootManagerLegacyBoot              = NULL;

typedef struct {
  UINT8             Pdt;            ///< Peripheral Device Type (low 5 bits)
  UINT8             Removable;      ///< Removable Media (highest bit)
  UINT8             Reserved0[2];
  UINT8             AddLen;         ///< Additional length
  UINT8             Reserved1[3];
  UINT8             VendorID[8];
  UINT8             ProductID[16];
  UINT8             ProductRevision[4];
} USB_BOOT_INQUIRY_DATA;

///
/// This GUID is used for an EFI Variable that stores the front device pathes
/// for a partial device path that starts with the HD node.
///
EFI_GUID mHdBootVariablePrivateGuid = { 0xfab7e9e1, 0x39dd, 0x4f2b, { 0x84, 0x08, 0xe2, 0x0e, 0x90, 0x6c, 0xb6, 0xde } };
UINT32 gEfiByoBootOptionFlag = SIGNATURE_32('_', 'B', 'Y', 'O');

/**
  Get the image file buffer data and buffer size by its device path. 

  @param FilePath  On input, a pointer to an allocated buffer containing the device
                   path of the file.
                   On output the pointer could be NULL when the function fails to
                   load the boot option, or could point to an allocated buffer containing
                   the device path of the file.
                   It could be updated by either short-form device path expanding,
                   or default boot file path appending.
                   Caller is responsible to free it when it's non-NULL.
  @param FileSize  A pointer to the size of the file buffer.

  @retval NULL   File is NULL, or FileSize is NULL. Or, the file can't be found.
  @retval other  The file buffer. The caller is responsible to free the memory.
**/
VOID *
LoadEfiBootOption (
  IN OUT EFI_DEVICE_PATH_PROTOCOL **FilePath,
  OUT    UINTN                    *FileSize
  );

UINT8
GetEfiNetWorkType (
  IN  EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  OUT CHAR16                     **TypeStr
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *Node;
  EFI_DEVICE_PATH_PROTOCOL      *NextNode;
  UINT8                         Type;
  UINT8                         IpType;

  ASSERT (DevicePath != NULL);

  IpType = 0xFF;
  
  for (Node = DevicePath; !IsDevicePathEndType(Node); Node = NextDevicePathNode(Node)) {
    if(DevicePathType(Node) != MESSAGING_DEVICE_PATH){
      continue;
    }  

    Type = DevicePathSubType(Node);
    if(Type == MSG_IPv4_DP){
      IpType = MSG_IPv4_DP;
    } else if(Type == MSG_IPv6_DP){
      IpType = MSG_IPv6_DP;
    }
    
    NextNode = NextDevicePathNode(Node);    
    if (!IsDevicePathEndType(NextNode)) {
      continue;
    }

    Type = DevicePathSubType(Node);
    switch(Type){
     case MSG_IPv4_DP:
     case MSG_IPv6_DP:
       *TypeStr = L"PXE";
       return IpType;

     case MSG_URI_DP:
       *TypeStr = L"HTTP";
       return IpType;          
    }
  }

  *TypeStr = L"";
  return 0xFF;
}







typedef struct {
  UINT32   DidVid;
  CHAR16   *Str;
} NetWorkTypeStruct;

NetWorkTypeStruct mNetWorkTypeList[] = {
  { 0x15028086, L"Intel 82579LM" },
  { 0x816810EC, L"Realtek 8111G" },
};

CHAR16 *
GetEfiNetWorkString (
  IN  EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                DriverHandle;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT32                    DidVid;
  UINTN                     Index;

  Status = gBS->LocateDevicePath (
                  &gEfiPciIoProtocolGuid,
                  &DevicePath,
                  &DriverHandle
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol(
                    DriverHandle,
                    &gEfiPciIoProtocolGuid,
                    &PciIo
                    );
    if (!EFI_ERROR (Status)) {
      PciIo->Pci.Read (
                   PciIo,
                   EfiPciIoWidthUint32,
                   0,
                   1,
                   &DidVid
                   );
      for (Index = 0; Index < sizeof (mNetWorkTypeList) / sizeof (NetWorkTypeStruct); Index ++) {
        if (DidVid == mNetWorkTypeList[Index].DidVid)
          return mNetWorkTypeList[Index].Str;
      }
    }
  }
  return L"Net";
}


VOID
EFIAPI
EfiBootManagerRegisterLegacyBootSupport (
  EFI_BOOT_MANAGER_REFRESH_LEGACY_BOOT_OPTION   RefreshLegacyBootOption,
  EFI_BOOT_MANAGER_LEGACY_BOOT                  LegacyBoot
  )
{
  mEfiBootManagerRefreshLegacyBootOption = RefreshLegacyBootOption;
  mEfiBootManagerLegacyBoot              = LegacyBoot;
}

VOID
FreeAndSet (
  VOID   **Orig,
  VOID   *New
  )
{
  FreePool (*Orig);
  *Orig = New;
}

/**
  Internal function to check if the input boot option is a valid EFI NV Boot####.

  @param OptionToCheck  Boot option to be checked.

  @retval TRUE      This boot option matches a valid EFI NV Boot####.
  @retval FALSE     If not.

**/
BOOLEAN
BootOptionInVariable (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION             *OptionToCheck
  )
{
  EFI_STATUS                   Status;
  EFI_BOOT_MANAGER_LOAD_OPTION BootOption;
  BOOLEAN                      Valid;
  CHAR16                       OptionName[sizeof ("Boot####")];

  UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", OptionToCheck->OptionNumber);
  Status = EfiBootManagerVariableToLoadOption (OptionName, &BootOption);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // If the Boot Option Number and Device Path matches, OptionToCheck matches a
  // valid EFI NV Boot####.
  //
  Valid = FALSE;
  if ((OptionToCheck->OptionNumber == BootOption.OptionNumber) &&
      (CompareMem (OptionToCheck->FilePath, BootOption.FilePath, GetDevicePathSize (OptionToCheck->FilePath)) == 0))
      {
    Valid = TRUE;
  }

  EfiBootManagerFreeLoadOption (&BootOption);
  return Valid;
}

/**
  According to a file guild, check a Fv file device path is valid. If it is invalid,
  try to return the valid device path.
  FV address maybe changes for memory layout adjust from time to time, use this function
  could promise the Fv file device path is right.

  @param  DevicePath   The Fv file device path to be fixed up.

  @retval DevicePath   The DevicePath ponits to the valid FV file.
  @retval !NULL        The fixed device path.
  @retval NULL         Failed to fix up the DevicePath.
**/
VOID
FixupMemmapFvFilePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL      **DevicePath
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  EFI_DEVICE_PATH_PROTOCOL      *Node;
  EFI_HANDLE                    FvHandle;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  EFI_LOADED_IMAGE_PROTOCOL     *LoadedImage;
  UINTN                         Size;
  EFI_FV_FILETYPE               Type;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINT32                        AuthenticationStatus;
  UINTN                         FvHandleCount;
  EFI_HANDLE                    *FvHandleBuffer;
  EFI_DEVICE_PATH_PROTOCOL      *NewDevicePath;
  
  Node = *DevicePath;
  Status = gBS->LocateDevicePath (&gEfiFirmwareVolume2ProtocolGuid, &Node, &FvHandle);
  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (FvHandle, &gEfiFirmwareVolume2ProtocolGuid, (VOID **) &Fv);
    ASSERT_EFI_ERROR (Status);

    Status = Fv->ReadFile (
                   Fv,
                   EfiGetNameGuidFromFwVolDevicePathNode ((CONST MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) Node),
                   NULL,
                   &Size,
                   &Type,
                   &Attributes,
                   &AuthenticationStatus
                   );
    if (EFI_ERROR (Status)) {
      FreeAndSet (DevicePath, NULL);
    }
    return;
  }

    
  Node = NextDevicePathNode (DevicePath);

  //
  // Firstly find the FV file in current FV
  //
  gBS->HandleProtocol (
         gImageHandle,
         &gEfiLoadedImageProtocolGuid,
         (VOID **) &LoadedImage
         );
  NewDevicePath = AppendDevicePathNode (DevicePathFromHandle (LoadedImage->DeviceHandle), Node);
  FixupMemmapFvFilePath (&NewDevicePath);

  if (NewDevicePath != NULL) {
    FreeAndSet (DevicePath, NewDevicePath);
    return;
  }

  //
  // Secondly find the FV file in all other FVs
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiFirmwareVolume2ProtocolGuid,
         NULL,
         &FvHandleCount,
         &FvHandleBuffer
         );
  for (Index = 0; Index < FvHandleCount; Index++) {
    if (FvHandleBuffer[Index] == LoadedImage->DeviceHandle) {
      //
      // Skip current FV
      //
      continue;
    }
    NewDevicePath = AppendDevicePathNode (DevicePathFromHandle (FvHandleBuffer[Index]), Node);
    FixupMemmapFvFilePath (&NewDevicePath);

    if (NewDevicePath != NULL) {
      FreeAndSet (DevicePath, NewDevicePath);
      return;
    }
  }
}

/**
  Check if it's of Fv file device path type.
  
  The function doesn't garentee the device path points to existing Fv file.

  @retval TRUE   The device path is of Fv file device path type.
  @retval FALSE  The device path isn't of Fv file device path type.
**/
BOOLEAN
IsMemmapFvFilePath (
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL   *FileNode;

  if ((DevicePathType (DevicePath) == HARDWARE_DEVICE_PATH) && (DevicePathSubType (DevicePath) == HW_MEMMAP_DP)) {
    FileNode = NextDevicePathNode (DevicePath);
    if ((DevicePathType (FileNode) == MEDIA_DEVICE_PATH) && (DevicePathSubType (FileNode) == MEDIA_PIWG_FW_FILE_DP)) {
      return IsDevicePathEnd (NextDevicePathNode (FileNode));
    }
  }

  return FALSE;
}

/**
  Check whether a USB device match the specified USB Class device path. This
  function follows "Load Option Processing" behavior in UEFI specification.

  @param UsbIo       USB I/O protocol associated with the USB device.
  @param UsbClass    The USB Class device path to match.

  @retval TRUE       The USB device match the USB Class device path.
  @retval FALSE      The USB device does not match the USB Class device path.

**/
BOOLEAN
MatchUsbClass (
  IN EFI_USB_IO_PROTOCOL        *UsbIo,
  IN USB_CLASS_DEVICE_PATH      *UsbClass
  )
{
  EFI_STATUS                    Status;
  EFI_USB_DEVICE_DESCRIPTOR     DevDesc;
  EFI_USB_INTERFACE_DESCRIPTOR  IfDesc;
  UINT8                         DeviceClass;
  UINT8                         DeviceSubClass;
  UINT8                         DeviceProtocol;

  if ((DevicePathType (UsbClass) != MESSAGING_DEVICE_PATH) ||
      (DevicePathSubType (UsbClass) != MSG_USB_CLASS_DP)){
    return FALSE;
  }

  //
  // Check Vendor Id and Product Id.
  //
  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if ((UsbClass->VendorId != 0xffff) &&
      (UsbClass->VendorId != DevDesc.IdVendor)) {
    return FALSE;
  }

  if ((UsbClass->ProductId != 0xffff) &&
      (UsbClass->ProductId != DevDesc.IdProduct)) {
    return FALSE;
  }

  DeviceClass    = DevDesc.DeviceClass;
  DeviceSubClass = DevDesc.DeviceSubClass;
  DeviceProtocol = DevDesc.DeviceProtocol;
  if (DeviceClass == 0) {
    //
    // If Class in Device Descriptor is set to 0, use the Class, SubClass and
    // Protocol in Interface Descriptor instead.
    //
    Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &IfDesc);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }

    DeviceClass    = IfDesc.InterfaceClass;
    DeviceSubClass = IfDesc.InterfaceSubClass;
    DeviceProtocol = IfDesc.InterfaceProtocol;
  }

  //
  // Check Class, SubClass and Protocol.
  //
  if ((UsbClass->DeviceClass != 0xff) &&
      (UsbClass->DeviceClass != DeviceClass)) {
    return FALSE;
  }

  if ((UsbClass->DeviceSubClass != 0xff) &&
      (UsbClass->DeviceSubClass != DeviceSubClass)) {
    return FALSE;
  }

  if ((UsbClass->DeviceProtocol != 0xff) &&
      (UsbClass->DeviceProtocol != DeviceProtocol)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Eliminate the extra spaces in the Str to one space.
**/
VOID
EliminateExtraSpaces (
  IN CHAR16                    *Str
  )
{
  UINTN                        Index;
  UINTN                        ActualIndex;
  
  for (Index = 0, ActualIndex = 0; Str[Index] != L'\0'; Index++) {
    if ((Str[Index] != L' ') || ((ActualIndex > 0) && (Str[ActualIndex - 1] != L' '))) {
      Str[ActualIndex++] = Str[Index];
    }
  }
  
  Str[ActualIndex] = L'\0';
}



STATIC UINTN GetSataDeviceIndex(EFI_HANDLE Handle)
{
  EFI_STATUS                    Status;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer = NULL;
  UINTN                         DevIndex = 0xFFFF;
  UINTN                         Index;
  EFI_DISK_INFO_PROTOCOL        *DiskInfo;
  EFI_PCI_IO_PROTOCOL           *PciIo;  
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *Dp;
  EFI_DEVICE_PATH_PROTOCOL      *TempDp;  
  EFI_HANDLE                    PciHandle;
  UINT32                        IdeChannel;	
  UINT32                        IdeDevice;
  UINT8                         ClassCode[3];
  UINTN                         PortCount = 0;
  UINT8                         SataMode;
  UINT32                        Cap;
  EFI_ATA_PASS_THRU_PROTOCOL    *AtaPassThru;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *ExtScsiPT;  
  UINT16                        SataPort;
  UINT16                        SataPortMp;
  BOOLEAN                       SataDevFound;
  UINT64                        Lun;  
  UINT8                         Target[TARGET_MAX_BYTES];
  UINT8                         *TargetId;
  

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDiskInfoProtocolGuid,
                  (VOID**)&DiskInfo
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  if(CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoIdeInterfaceGuid)){
    SataMode = 1;
  } else if(CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoAhciInterfaceGuid)){
    SataMode = 6;
  } else {
    goto ProcExit;
  }

  DEBUG((EFI_D_INFO, "SataMode:%d\n", SataMode));

  Status = DiskInfo->WhichIde (
                       DiskInfo,
                       &IdeChannel,
                       &IdeDevice
                       );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }	  
  DEBUG((EFI_D_INFO, "IDE(%d,%d)\n", IdeChannel, IdeDevice));
  
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID**)&DevicePath
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  

  PciHandle = NULL;
  Dp = DuplicateDevicePath(DevicePath);
  TempDp = Dp;
  while (!IsDevicePathEnd (TempDp)) {
    if (DevicePathType(TempDp) == MESSAGING_DEVICE_PATH &&
        (DevicePathSubType(TempDp) == MSG_ATAPI_DP || 
         DevicePathSubType(TempDp) == MSG_SATA_DP)) {
      SetDevicePathEndNode(TempDp);
      TempDp = Dp;
      gBS->LocateDevicePath(&gEfiPciIoProtocolGuid, &TempDp, &PciHandle);
      break;
    }
    TempDp = NextDevicePathNode(TempDp);
  }
  if (PciHandle == NULL) {
    DEBUG((EFI_D_INFO, "DP not found PciIo\n"));
    goto ProcExit;
  }
  FreePool(Dp);
  
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  
  for (Index = 0; Index < HandleCount; Index++) {
    
    SataDevFound = FALSE;   
    
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    &PciIo
                    );
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiAtaPassThruProtocolGuid,
                    (VOID**)&AtaPassThru
                    );
    if(!EFI_ERROR(Status)){
      SataPort = 0xFFFF;
      while (TRUE) {
        Status = AtaPassThru->GetNextPort(AtaPassThru, &SataPort);
        if (EFI_ERROR (Status)) {
          break;
        } 
        SataPortMp = 0xFFFF;
        while (TRUE) {
          Status = AtaPassThru->GetNextDevice(AtaPassThru, SataPort, &SataPortMp);
          if (EFI_ERROR (Status)) {
            break;
          }
          SataDevFound = TRUE;
          goto FoundSataDevice;
        }
      }
    }
    
    Status = gBS->HandleProtocol(
                    HandleBuffer[Index],
                    &gEfiExtScsiPassThruProtocolGuid,
                    &ExtScsiPT
                    );
    if(EFI_ERROR(Status)){
      continue;
    }
    Lun = 0;
    TargetId = &Target[0];
    SetMem(Target, sizeof(Target), 0xFF);
    Status = ExtScsiPT->GetNextTargetLun(ExtScsiPT, &TargetId, &Lun);
    if(!EFI_ERROR(Status)){
      SataDevFound = TRUE;
    }

FoundSataDevice:
    if(!SataDevFound){
      continue;
    }
    
    PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET, 3, &ClassCode[0]);
    if(ClassCode[2] == PCI_CLASS_MASS_STORAGE && ClassCode[1] == SataMode){
      if(PciHandle == HandleBuffer[Index]){
        break;  
      }
      if(SataMode == 1){
        PortCount += 4;
      } else {
        PciIo->Mem.Read (
                     PciIo,
                     EfiPciIoWidthUint32,
                     5,
                     0,
                     1,
                     &Cap
                     );
        PortCount += (Cap & 0x1F) + 1;
      }
    }  
  } 
  
  if(Index == HandleCount){
    goto ProcExit;
  }  

  if(SataMode == 1){
    DevIndex = PortCount + IdeChannel * 2 + IdeDevice;
  } else {             // must AHCI
    DevIndex = PortCount + IdeChannel;
  }

ProcExit:
  if(HandleBuffer != NULL){
    gBS->FreePool(HandleBuffer);
  }    
  return DevIndex;  
}




/**
  Try to get the controller's ATA/ATAPI description.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
GetAtaAtapiDescription (
  IN EFI_HANDLE                Handle
  )
{
  UINTN                        Index;
  EFI_STATUS                   Status;
  EFI_DISK_INFO_PROTOCOL       *DiskInfo;
  UINT32                       BufferSize;
  EFI_ATAPI_IDENTIFY_DATA      *IdentifyData = NULL;
  CHAR16                       *ModelName = NULL;  
  CHAR16                       *NewString = NULL;
  UINTN                        AtaIndex;



  AtaIndex = GetSataDeviceIndex(Handle);
  if(AtaIndex == 0xFFFF){
    DEBUG((EFI_D_INFO, "GetSatadevIndexError\n"));
    goto ProcExit;
  }  

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDiskInfoProtocolGuid,
                  (VOID**)&DiskInfo
                  );
  if (EFI_ERROR (Status)) {
    goto ProcExit;
  }
  IdentifyData = AllocatePool(sizeof(EFI_ATAPI_IDENTIFY_DATA));
  if(IdentifyData == NULL){
    goto ProcExit;
  }  
  ModelName = AllocatePool((40+1)*sizeof(CHAR16));
  if(ModelName == NULL){
    goto ProcExit;
  }    
  
  BufferSize = sizeof(EFI_ATAPI_IDENTIFY_DATA);
  Status = DiskInfo->Identify (
                       DiskInfo,
                       IdentifyData,
                       &BufferSize
                       );
  if (EFI_ERROR (Status)) {
    goto ProcExit;
  }
  for (Index = 0; Index < 40; Index += 2) {
    ModelName[Index]     = IdentifyData->ModelName[Index + 1];
    ModelName[Index + 1] = IdentifyData->ModelName[Index];
  }
  ModelName[Index] = L'\0';
  EliminateExtraSpaces(ModelName);

  NewString = AllocatePool(32*sizeof(CHAR16));
  if (NewString == NULL) {
    goto ProcExit;
  }  
  UnicodeSPrint(NewString, 32*sizeof(CHAR16), L"SATA %d: %s", AtaIndex+1, ModelName);


ProcExit:
  if(IdentifyData!=NULL){FreePool(IdentifyData);}
  if(ModelName!=NULL){FreePool(ModelName);}  
  return NewString;
}




/**
  Try to get the controller's USB description.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
GetUsbDescription (
  IN EFI_HANDLE                Handle
  )
{
  EFI_STATUS                   Status;
  CHAR16                       *Description;
  UINT8                        ModelNumber[50];
  USB_BOOT_INQUIRY_DATA        UsbInquiryData;  
  UINT32                       BufferSize = 0;
  EFI_DISK_INFO_PROTOCOL       *DiskInfo;
  

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDiskInfoProtocolGuid,
                  &DiskInfo
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }             

  if(!CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoUsbInterfaceGuid)){
    return NULL;
  }		
	
  BufferSize = sizeof(USB_BOOT_INQUIRY_DATA);  
  Status = DiskInfo->Inquiry(
                     DiskInfo,
                     &UsbInquiryData,
                     &BufferSize
                     );
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  
  ZeroMem (ModelNumber, sizeof (ModelNumber));
  CopyMem (ModelNumber, UsbInquiryData.VendorID, 8);
  ModelNumber[8] = ' ';

  CopyMem (&ModelNumber[9], UsbInquiryData.ProductID, 16);
  ModelNumber[25] = '\0';

  Description = AllocateZeroPool (150);
  UnicodeSPrint (Description, 150, L"USB: %a", ModelNumber);
  EliminateExtraSpaces (Description);
  
  return Description;
}

/**
  Return the boot description for the controller based on the type.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
GetMiscDescription (
  IN EFI_HANDLE                  Handle
  )
{
  EFI_STATUS                     Status;
  CHAR16                         *Description;
  EFI_BLOCK_IO_PROTOCOL          *BlockIo;
  EFI_DEVICE_PATH_PROTOCOL       *ptDevPath;
  BOOT_TYPE                      BootType;
  EFI_PCI_IO_PROTOCOL            *ptPciIo;
  EFI_HANDLE                     PciHandle;
  UINT8                          ClassCode[3];
  EFI_DEVICE_PATH_PROTOCOL       *ptDP;
  

  ptDevPath = DevicePathFromHandle(Handle);
  BootType  = BootTypeFromDevicePath(ptDevPath);
  
  switch (BootType) {
    case AcpiFloppyBoot:
      Description = L"Floppy";
      break;

    case MessageAtapiBoot:
    case MessageSataBoot:
      Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **) &BlockIo);
      ASSERT_EFI_ERROR (Status);
// Assume a removable SATA device should be the DVD/CD device
      Description = BlockIo->Media->RemovableMedia ? L"DVD/CDROM" : L"Hard Drive";
      break;

    case MessageUsbBoot:
      Description = L"USB Device";
      break;

    case MessageScsiBoot:
      Description = L"SCSI Device";
      break;

    default:
      Description = L"Misc Device";
      break;
  }

  DEBUG((EFI_D_INFO, "BootType:%d\n", BootType));
  if(BootType == UnsupportedBoot){
    ptDP = ptDevPath;
    Status = gBS->LocateDevicePath (
              &gEfiPciIoProtocolGuid,
              &ptDP,
              &PciHandle
              );
    if(!EFI_ERROR(Status)){
      Status = gBS->HandleProtocol(
                      PciHandle,
                      &gEfiPciIoProtocolGuid,
                      &ptPciIo
                      );
      ASSERT(!EFI_ERROR(Status));
      Status = ptPciIo->Pci.Read(
                              ptPciIo, 
                              EfiPciIoWidthUint8,  
                              PCI_CLASSCODE_OFFSET, 
                              3, 
                              ClassCode
                              );
      if(ClassCode[2] == 8 && 
         ClassCode[1] == 5 && 
        (ClassCode[0] == 0 || ClassCode[0] == 1)){      // SD
        Description = L"SD";
      } else if(ClassCode[2] == 1 && ClassCode[1] == 8 && ClassCode[0] == 2){ // MVMe
        Description = L"NVMe";
      }
    }  
  }
  
  return AllocateCopyPool(StrSize(Description), Description);
}

GET_BOOT_DESCRIPTION mGetBootDescription[] = {
  GetUsbDescription,
  GetAtaAtapiDescription,
  GetMiscDescription
};

/**
  Check whether a USB device match the specified USB WWID device path. This
  function follows "Load Option Processing" behavior in UEFI specification.

  @param UsbIo       USB I/O protocol associated with the USB device.
  @param UsbWwid     The USB WWID device path to match.

  @retval TRUE       The USB device match the USB WWID device path.
  @retval FALSE      The USB device does not match the USB WWID device path.

**/
BOOLEAN
MatchUsbWwid (
  IN EFI_USB_IO_PROTOCOL        *UsbIo,
  IN USB_WWID_DEVICE_PATH       *UsbWwid
  )
{
  EFI_STATUS                   Status;
  EFI_USB_DEVICE_DESCRIPTOR    DevDesc;
  EFI_USB_INTERFACE_DESCRIPTOR IfDesc;
  UINT16                       *LangIdTable;
  UINT16                       TableSize;
  UINT16                       Index;
  CHAR16                       *CompareStr;
  UINTN                        CompareLen;
  CHAR16                       *SerialNumberStr;
  UINTN                        Length;

  if ((DevicePathType (UsbWwid) != MESSAGING_DEVICE_PATH) ||
      (DevicePathSubType (UsbWwid) != MSG_USB_WWID_DP)) {
    return FALSE;
  }

  //
  // Check Vendor Id and Product Id.
  //
  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  if ((DevDesc.IdVendor != UsbWwid->VendorId) ||
      (DevDesc.IdProduct != UsbWwid->ProductId)) {
    return FALSE;
  }

  //
  // Check Interface Number.
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &IfDesc);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  if (IfDesc.InterfaceNumber != UsbWwid->InterfaceNumber) {
    return FALSE;
  }

  //
  // Check Serial Number.
  //
  if (DevDesc.StrSerialNumber == 0) {
    return FALSE;
  }

  //
  // Get all supported languages.
  //
  TableSize = 0;
  LangIdTable = NULL;
  Status = UsbIo->UsbGetSupportedLanguages (UsbIo, &LangIdTable, &TableSize);
  if (EFI_ERROR (Status) || (TableSize == 0) || (LangIdTable == NULL)) {
    return FALSE;
  }

  //
  // Serial number in USB WWID device path is the last 64-or-less UTF-16 characters.
  //
  CompareStr = (CHAR16 *) (UINTN) (UsbWwid + 1);
  CompareLen = (DevicePathNodeLength (UsbWwid) - sizeof (USB_WWID_DEVICE_PATH)) / sizeof (CHAR16);
  if (CompareStr[CompareLen - 1] == L'\0') {
    CompareLen--;
  }

  //
  // Compare serial number in each supported language.
  //
  for (Index = 0; Index < TableSize / sizeof (UINT16); Index++) {
    SerialNumberStr = NULL;
    Status = UsbIo->UsbGetStringDescriptor (
                      UsbIo,
                      LangIdTable[Index],
                      DevDesc.StrSerialNumber,
                      &SerialNumberStr
                      );
    if (EFI_ERROR (Status) || (SerialNumberStr == NULL)) {
      continue;
    }

    Length = StrLen (SerialNumberStr);
    if ((Length >= CompareLen) &&
        (CompareMem (SerialNumberStr + Length - CompareLen, CompareStr, CompareLen * sizeof (CHAR16)) == 0)) {
      FreePool (SerialNumberStr);
      return TRUE;
    }

    FreePool (SerialNumberStr);
  }

  return FALSE;
}

VOID
PrintDp (
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath
  )
{
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL    *ToText;
  CHAR16                              *DpStr;

  Status = gBS->LocateProtocol (&gEfiDevicePathToTextProtocolGuid, NULL, (VOID **) &ToText);
  ASSERT_EFI_ERROR (Status);

  DpStr = ToText->ConvertDevicePathToText (DevicePath, FALSE, FALSE);
  DEBUG ((EFI_D_INFO, "%s", DpStr));
  FreePool (DpStr);
}

/**
  Find a USB device which match the specified short-form device path start with 
  USB Class or USB WWID device path. If ParentDevicePath is NULL, this function
  will search in all USB devices of the platform. If ParentDevicePath is not NULL,
  this function will only search in its child devices.

  @param DevicePath           The device path that contains USB Class or USB WWID device path.
  @param ParentDevicePathSize The length of the device path before the USB Class or 
                              USB WWID device path.
  @param UsbIoHandleCount     A pointer to the count of the returned USB IO handles.

  @retval NULL       The matched USB IO handles cannot be found.
  @retval other      The matched USB IO handles.

**/
EFI_HANDLE *
FindUsbDevice (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  UINTN                     ParentDevicePathSize,
  OUT UINTN                     *UsbIoHandleCount
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                *UsbIoHandles;
  EFI_DEVICE_PATH_PROTOCOL  *UsbIoDevicePath;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  UINTN                     Index;
  UINTN                     UsbIoDevicePathSize;
  BOOLEAN                   Matched;

  ASSERT (UsbIoHandleCount != NULL);  

  //
  // Get all UsbIo Handles.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiUsbIoProtocolGuid,
                  NULL,
                  UsbIoHandleCount,
                  &UsbIoHandles
                  );
  if (EFI_ERROR (Status)) {
    *UsbIoHandleCount = 0;
    UsbIoHandles      = NULL;
  }

  for (Index = 0; Index < *UsbIoHandleCount; ) {
    //
    // Get the Usb IO interface.
    //
    Status = gBS->HandleProtocol(
                    UsbIoHandles[Index],
                    &gEfiUsbIoProtocolGuid,
                    (VOID **) &UsbIo
                    );
    UsbIoDevicePath = DevicePathFromHandle (UsbIoHandles[Index]);
    Matched         = FALSE;
    if (!EFI_ERROR (Status) && (UsbIoDevicePath != NULL)) {
      UsbIoDevicePathSize = GetDevicePathSize (UsbIoDevicePath) - END_DEVICE_PATH_LENGTH;

      //
      // Compare starting part of UsbIoHandle's device path with ParentDevicePath.
      //
      if (CompareMem (UsbIoDevicePath, DevicePath, ParentDevicePathSize) == 0) {
        if (MatchUsbClass (UsbIo, (USB_CLASS_DEVICE_PATH *) ((UINTN) DevicePath + ParentDevicePathSize)) ||
            MatchUsbWwid (UsbIo, (USB_WWID_DEVICE_PATH *) ((UINTN) DevicePath + ParentDevicePathSize))) {
          Matched = TRUE;
        }
      }
    }

    if (!Matched) {
      (*UsbIoHandleCount) --;
      CopyMem (&UsbIoHandles[Index], &UsbIoHandles[Index + 1], (*UsbIoHandleCount - Index) * sizeof (EFI_HANDLE));
    } else {
      Index++;
    }
  }

  return UsbIoHandles;
}

/**
  Expand USB Class or USB WWID device path node to be full device path of a USB
  device in platform.

  This function support following 4 cases:
  1) Boot Option device path starts with a USB Class or USB WWID device path,
     and there is no Media FilePath device path in the end.
     In this case, it will follow Removable Media Boot Behavior.
  2) Boot Option device path starts with a USB Class or USB WWID device path,
     and ended with Media FilePath device path.
  3) Boot Option device path starts with a full device path to a USB Host Controller,
     contains a USB Class or USB WWID device path node, while not ended with Media
     FilePath device path. In this case, it will follow Removable Media Boot Behavior.
  4) Boot Option device path starts with a full device path to a USB Host Controller,
     contains a USB Class or USB WWID device path node, and ended with Media
     FilePath device path.

  @param  DevicePath    On input, a pointer to an allocated buffer that contains the 
                        file device path.
                        On output, a pointer to an reallocated buffer that contains 
                        the expanded device path. It would point to NULL if the file
                        cannot be read.

  @param  FileSize      A pointer to the file size.

  @retval !NULL  The file buffer.
  @retval NULL   The input device path doesn't point to a valid file.
**/
VOID *
ExpandUsbShortFormDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath,
  OUT UINTN                        *FileSize
  )
{
  UINTN                             ParentDevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL          *ShortformNode;
  EFI_DEVICE_PATH_PROTOCOL          *RemainingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          *FullDevicePath;
  EFI_HANDLE                        *UsbIoHandles;
  UINTN                             UsbIoHandleCount;
  UINTN                             Index;
  VOID                              *FileBuffer;
  
  //
  // Search for USB Class or USB WWID device path node.
  //
  for ( ShortformNode = *DevicePath
      ; !IsDevicePathEnd (ShortformNode)
      ; ShortformNode = NextDevicePathNode (ShortformNode)
      ) {
    if ((DevicePathType (ShortformNode) == MESSAGING_DEVICE_PATH) &&
        ((DevicePathSubType (ShortformNode) == MSG_USB_CLASS_DP) ||
         (DevicePathSubType (ShortformNode) == MSG_USB_WWID_DP))) {
      break;
    }
  }
  ASSERT (!IsDevicePathEnd (ShortformNode));

  FullDevicePath       = NULL;
  ParentDevicePathSize = (UINTN) ShortformNode - (UINTN) *DevicePath;
  RemainingDevicePath  = NextDevicePathNode (ShortformNode);
  FileBuffer           = NULL;
  UsbIoHandles         = FindUsbDevice (*DevicePath, ParentDevicePathSize, &UsbIoHandleCount);

  for (Index = 0; Index < UsbIoHandleCount; Index++) {
    FullDevicePath = AppendDevicePath (DevicePathFromHandle (UsbIoHandles[Index]), RemainingDevicePath);
    DEBUG ((EFI_D_INFO, "[Bds] FullDp1[%d]:", Index)); DEBUG_CODE (PrintDp (FullDevicePath); ); DEBUG ((EFI_D_INFO, "\n"));
    FileBuffer = LoadEfiBootOption (&FullDevicePath, FileSize);
    if (FileBuffer != NULL) {
      DEBUG ((EFI_D_INFO, "-->")); DEBUG_CODE (PrintDp (FullDevicePath); ); DEBUG ((EFI_D_INFO, FileBuffer != NULL ? " - Found\n" : "\n"));
      break;
    }
  }

  if (UsbIoHandles != NULL) {
    FreePool (UsbIoHandles);
  }

  if (FileBuffer == NULL) {
    //
    // Boot Option device path starts with USB Class or USB WWID device path.
    // For Boot Option device path which doesn't begin with the USB Class or
    // USB WWID device path, it's not needed to connect again here.
    //
    if ((DevicePathType (*DevicePath) == MESSAGING_DEVICE_PATH) &&
        ((DevicePathSubType (*DevicePath) == MSG_USB_CLASS_DP) ||
         (DevicePathSubType (*DevicePath) == MSG_USB_WWID_DP))) {
      EfiBootManagerConnectUsbShortFormDevicePath (*DevicePath);

      UsbIoHandles = FindUsbDevice (*DevicePath, ParentDevicePathSize, &UsbIoHandleCount);
      for (Index = 0; Index < UsbIoHandleCount; Index++) {
        FullDevicePath = AppendDevicePath (DevicePathFromHandle (UsbIoHandles[Index]), RemainingDevicePath);
        DEBUG ((EFI_D_INFO, "[Bds] FullDp2[%d]:", Index)); DEBUG_CODE (PrintDp (FullDevicePath); ); DEBUG ((EFI_D_INFO, "\n"));
        FileBuffer = LoadEfiBootOption (&FullDevicePath, FileSize);
        if (FileBuffer != NULL) {
          DEBUG ((EFI_D_INFO, "-->")); DEBUG_CODE (PrintDp (FullDevicePath); ); DEBUG ((EFI_D_INFO, FileBuffer != NULL ? " - Found\n" : "\n"));
          break;
        }
      }

      if (UsbIoHandles != NULL) {
        FreePool (UsbIoHandles);
      }
    }
  }

  FreeAndSet (DevicePath, FullDevicePath);
  return FileBuffer;
}

/**
  Expand a device path that starts with a hard drive media device path node to be a
  full device path that includes the full hardware path to the device. We need
  to do this so it can be booted. As an optimization the front match (the part point
  to the partition node. E.g. ACPI() /PCI()/ATA()/Partition() ) is saved in a variable
  so a connect all is not required on every boot. All successful history device path
  which point to partition node (the front part) will be saved.

  @param  DevicePath    On input, a pointer to an allocated buffer that contains the 
                        file device path.
                        On output, a pointer to an reallocated buffer that contains 
                        the expanded device path. It would point to NULL if the file
                        cannot be read.

**/
VOID
ExpandPartitionShortFormDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL      **DevicePath
  )
{
  EFI_STATUS                Status;
  UINTN                     BlockIoHandleCount;
  EFI_HANDLE                *BlockIoBuffer;
  EFI_DEVICE_PATH_PROTOCOL  *FullDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *BlockIoDevicePath;
  UINTN                     Index;
  UINTN                     InstanceNum;
  EFI_DEVICE_PATH_PROTOCOL  *CachedDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;
  UINTN                     CachedDevicePathSize;
  BOOLEAN                   DeviceExist;
  BOOLEAN                   NeedAdjust;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  UINTN                     Size;

  FullDevicePath      = NULL;
  //
  // Check if there is prestore 'HDDP' variable.
  // If exist, search the front path which point to partition node in the variable instants.
  // If fail to find or 'HDDP' not exist, reconnect all and search in all system
  //
  CachedDevicePath = EfiBootManagerGetVariableAndSize (
                      L"HDDP",
                      &mHdBootVariablePrivateGuid,
                      &CachedDevicePathSize
                      );

  if (CachedDevicePath != NULL) {
    TempNewDevicePath = CachedDevicePath;
    DeviceExist = FALSE;
    NeedAdjust = FALSE;
    do {
      //
      // Check every instance of the variable
      // First, check whether the instance contain the partition node, which is needed for distinguishing  multi
      // partial partition boot option. Second, check whether the instance could be connected.
      //
      Instance  = GetNextDevicePathInstance (&TempNewDevicePath, &Size);
      if (MatchPartitionDevicePathNode (Instance, (HARDDRIVE_DEVICE_PATH *) *DevicePath)) {
        //
        // Connect the device path instance, the device path point to hard drive media device path node
        // e.g. ACPI() /PCI()/ATA()/Partition()
        //
        Status = EfiBootManagerConnectDevicePath (Instance, NULL);
        if (!EFI_ERROR (Status)) {
          DeviceExist = TRUE;
          break;
        }
      }
      //
      // Come here means the first instance is not matched
      //
      NeedAdjust = TRUE;
      FreePool(Instance);
    } while (TempNewDevicePath != NULL);

    if (DeviceExist) {
      //
      // Find the matched device path.
      // Append the file path information from the boot option and return the fully expanded device path.
      //
      FullDevicePath = AppendDevicePath (Instance, NextDevicePathNode (*DevicePath));

      //
      // Adjust the 'HDDP' instances sequence if the matched one is not first one.
      //
      if (NeedAdjust) {
        //
        // First delete the matched instance.
        //
        TempNewDevicePath = CachedDevicePath;
        CachedDevicePath  = EfiBootManagerDelPartMatchInstance (CachedDevicePath, Instance);
        FreePool (TempNewDevicePath);

        //
        // Second, append the remaining path after the matched instance
        //
        TempNewDevicePath = CachedDevicePath;
        CachedDevicePath = AppendDevicePathInstance (Instance, CachedDevicePath );
        FreePool (TempNewDevicePath);
        //
        // Save the matching Device Path so we don't need to do a connect all next time
        //
        Status = gRT->SetVariable (
                        L"HDDP",
                        &mHdBootVariablePrivateGuid,
                        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                        GetDevicePathSize (CachedDevicePath),
                        CachedDevicePath
                        );
      }

      FreePool (Instance);
      FreePool (CachedDevicePath);
      FreePool (*DevicePath);
      *DevicePath = FullDevicePath;
      return;
    }
  }

  //
  // If we get here we fail to find or 'HDDP' not exist, and now we need
  // to search all devices in the system for a matched partition
  //
  EfiBootManagerConnectAll ();
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &BlockIoHandleCount, &BlockIoBuffer);
  if (EFI_ERROR (Status)) {
    BlockIoHandleCount = 0;
    BlockIoBuffer      = NULL;
  }
  //
  // Loop through all the device handles that support the BLOCK_IO Protocol
  //
  for (Index = 0; Index < BlockIoHandleCount; Index++) {

    Status = gBS->HandleProtocol (BlockIoBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID *) &BlockIoDevicePath);
    if (EFI_ERROR (Status) || BlockIoDevicePath == NULL) {
      continue;
    }

    if (MatchPartitionDevicePathNode (BlockIoDevicePath, (HARDDRIVE_DEVICE_PATH *) *DevicePath)) {
      //
      // Find the matched partition device path
      //
      FullDevicePath = AppendDevicePath (BlockIoDevicePath, NextDevicePathNode (*DevicePath));

      //
      // Save the matched partition device path in 'HDDP' variable
      //
      if (CachedDevicePath != NULL) {
        //
        // Save the matched partition device path as first instance of 'HDDP' variable
        //
        if (EfiBootManagerMatchDevicePaths (CachedDevicePath, BlockIoDevicePath)) {
          TempNewDevicePath = CachedDevicePath;
          CachedDevicePath = EfiBootManagerDelPartMatchInstance (CachedDevicePath, BlockIoDevicePath);
          FreePool(TempNewDevicePath);

          TempNewDevicePath = CachedDevicePath;
          CachedDevicePath = AppendDevicePathInstance (BlockIoDevicePath, CachedDevicePath);
          if (TempNewDevicePath != NULL) {
            FreePool(TempNewDevicePath);
          }
        } else {
          TempNewDevicePath = CachedDevicePath;
          CachedDevicePath = AppendDevicePathInstance (BlockIoDevicePath, CachedDevicePath);
          FreePool(TempNewDevicePath);
        }
        //
        // Here limit the device path instance number to 12, which is max number for a system support 3 IDE controller
        // If the user try to boot many OS in different HDs or partitions, in theory, the 'HDDP' variable maybe become larger and larger.
        //
        InstanceNum = 0;
        ASSERT (CachedDevicePath != NULL);
        TempNewDevicePath = CachedDevicePath;
        while (!IsDevicePathEnd (TempNewDevicePath)) {
          TempNewDevicePath = NextDevicePathNode (TempNewDevicePath);
          //
          // Parse one instance
          //
          while (!IsDevicePathEndType (TempNewDevicePath)) {
            TempNewDevicePath = NextDevicePathNode (TempNewDevicePath);
          }
          InstanceNum++;
          //
          // If the CachedDevicePath variable contain too much instance, only remain 12 instances.
          //
          if (InstanceNum >= 12) {
            SetDevicePathEndNode (TempNewDevicePath);
            break;
          }
        }
      } else {
        CachedDevicePath = DuplicateDevicePath (BlockIoDevicePath);
      }

      //
      // Save the matching Device Path so we don't need to do a connect all next time
      //
      Status = gRT->SetVariable (
                      L"HDDP",
                      &mHdBootVariablePrivateGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      GetDevicePathSize (CachedDevicePath),
                      CachedDevicePath
                      );

      break;
    }
  }

  if (CachedDevicePath != NULL) {
    FreePool (CachedDevicePath);
  }
  if (BlockIoBuffer != NULL) {
    FreePool (BlockIoBuffer);
  }
  FreeAndSet (DevicePath, FullDevicePath);
}



STATIC CHAR16 *gLoaderPathNameList[] = {
  EFI_REMOVABLE_MEDIA_FILE_NAME,
  L"\\EFI\\Ubuntu\\grubx64.efi",
};

/**
  Algorithm follows the UEFI Spec chapter 3.4 Boot Mechanisms.

  @param  DevicePath  Device Path to a  bootable device

  @return  The bootable media handle. If the media on the DevicePath is not bootable, NULL will return.

**/
EFI_HANDLE
GetBootableDeviceHandle (
  IN  EFI_DEVICE_PATH_PROTOCOL        *DevicePath,
  OUT CHAR16                          **OsLoaderPathName
  )
{
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH_PROTOCOL            *UpdatedDevicePath;
  EFI_HANDLE                          Handle;
  EFI_BLOCK_IO_PROTOCOL               *BlockIo;
  VOID                                *Buffer;
  EFI_DEVICE_PATH_PROTOCOL            *TempDevicePath;
  UINTN                               Size;
  UINTN                               TempSize;
  EFI_HANDLE                          ReturnHandle;
  EFI_HANDLE                          *SimpleFileSystemHandles;
  UINTN                               NumberSimpleFileSystemHandles;
  UINTN                               Index;
  UINTN                               LoaderIndex;
  UINTN                               LoaderCount;
  EFI_IMAGE_DOS_HEADER                DosHeader;
  EFI_IMAGE_OPTIONAL_HEADER_UNION     HdrData;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION Hdr;

  ReturnHandle      = NULL;
  UpdatedDevicePath = DevicePath;

  //
  // Check whether the device is connected
  //
  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &UpdatedDevicePath, &Handle);
  if (EFI_ERROR (Status)) {
    //
    // Skip the case that the boot option point to a simple file protocol which does not consume block Io protocol,
    //
    Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &UpdatedDevicePath, &Handle);
    if (EFI_ERROR (Status)) {
      //
      // Fail to find the proper BlockIo and simple file protocol, maybe because device not present,  we need to connect it firstly
      //
      UpdatedDevicePath = DevicePath;
      Status            = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &UpdatedDevicePath, &Handle);
      gBS->ConnectController (Handle, NULL, NULL, TRUE);
    }
  } else {
    //
    // For removable device boot option, its contained device path only point to the removable device handle, 
    // should make sure all its children handles (its child partion or media handles) are created and connected. 
    //
    gBS->ConnectController (Handle, NULL, NULL, TRUE); 
    //
    // Get BlockIo protocol and check removable attribute
    //
    Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
    //
    // Issue a dummy read to the device to check for media change.
    // When the removable media is changed, any Block IO read/write will
    // cause the BlockIo protocol be reinstalled and EFI_MEDIA_CHANGED is
    // returned. After the Block IO protocol is reinstalled, subsequent
    // Block IO read/write will success.
    //
    Buffer = AllocatePool (BlockIo->Media->BlockSize);
    if (Buffer != NULL) {
      BlockIo->ReadBlocks (
               BlockIo,
               BlockIo->Media->MediaId,
               0,
               BlockIo->Media->BlockSize,
               Buffer
               );
      FreePool(Buffer);
    }
  }

  //
  // Detect the the default boot file from removable Media
  //
  Size = GetDevicePathSize(DevicePath) - END_DEVICE_PATH_LENGTH;
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiSimpleFileSystemProtocolGuid,
         NULL,
         &NumberSimpleFileSystemHandles,
         &SimpleFileSystemHandles
         );
  for (Index = 0; Index < NumberSimpleFileSystemHandles; Index++) {
    //
    // Get the device path size of SimpleFileSystem handle
    //
    TempDevicePath = DevicePathFromHandle (SimpleFileSystemHandles[Index]);
    TempSize       = GetDevicePathSize (TempDevicePath)- END_DEVICE_PATH_LENGTH;
    //
    // Check whether the device path of boot option is part of the SimpleFileSystem handle's device path
    //
    if ((Size <= TempSize) && (CompareMem (TempDevicePath, DevicePath, Size) == 0)) {
      //
      // Load the default boot file \EFI\BOOT\boot{machinename}.EFI from removable Media
      //  machinename is ia32, ia64, x64, ...
      //
      Hdr.Union = &HdrData;

      LoaderCount = sizeof(gLoaderPathNameList)/sizeof(gLoaderPathNameList[0]);
      for(LoaderIndex=0;LoaderIndex<LoaderCount;LoaderIndex++){
        Status = GetImageHeader (
                   SimpleFileSystemHandles[Index],
                   gLoaderPathNameList[LoaderIndex],
                   &DosHeader,
                   Hdr
                   );
        if(!EFI_ERROR(Status)){
          break;
        }
      }
      
      if (!EFI_ERROR (Status) && 
          EFI_IMAGE_MACHINE_TYPE_SUPPORTED (Hdr.Pe32->FileHeader.Machine) &&
          Hdr.Pe32->OptionalHeader.Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION) {
        ReturnHandle = SimpleFileSystemHandles[Index];
        *OsLoaderPathName = gLoaderPathNameList[LoaderIndex];
        break;
      }
    }
  }

  if (SimpleFileSystemHandles != NULL) {
    FreePool(SimpleFileSystemHandles);
  }

  return ReturnHandle;
}

/**
  Get the image file buffer data and buffer size by its device path. 

  @param FilePath  On input, a pointer to an allocated buffer that contains the 
                   file device path.
                   On output the device path pointer could be modified to point to
                   a new allocated buffer that contains the full device path.
                   It could be caused by either short-form device path expanding,
                   or default boot file path appending.
  @param FileSize  A pointer to the size of the file buffer.

  @retval NULL   The file can't be found.
  @retval other  The file buffer. The caller is responsible to free memory.
**/
VOID *
LoadEfiBootOption (
  IN OUT EFI_DEVICE_PATH_PROTOCOL **FilePath,
  OUT    UINTN                    *FileSize
  )
{
  EFI_HANDLE                      Handle;
  VOID                            *FileBuffer;
  UINT32                          AuthenticationStatus;
  EFI_DEVICE_PATH_PROTOCOL        *Node;
  CHAR16                          *LoaderPathName;
  

  ASSERT ((FilePath != NULL) && (*FilePath != NULL) && (FileSize != NULL));

  EfiBootManagerConnectDevicePath (*FilePath, NULL);

  *FileSize  = 0;
  FileBuffer = NULL;
  //
  // Expand the short-form device path to full device path
  //
  if (1) {

    if ((DevicePathType (*FilePath) == MEDIA_DEVICE_PATH) &&
        (DevicePathSubType (*FilePath) == MEDIA_HARDDRIVE_DP)) {
      //
      // Expand the Harddrive device path
      //
      ExpandPartitionShortFormDevicePath (FilePath);
      if (*FilePath == NULL) {
        return NULL;
      }

    } else {
      for (Node = *FilePath; !IsDevicePathEnd (Node); Node = NextDevicePathNode (Node)) {
        if ((DevicePathType (Node) == MESSAGING_DEVICE_PATH) &&
            ((DevicePathSubType (Node) == MSG_USB_CLASS_DP) ||
             (DevicePathSubType (Node) == MSG_USB_WWID_DP))) {
          break;
        }
      }

      if (!IsDevicePathEnd (Node)) {
        //
        // Expand the USB WWID/Class device path
        //
        FileBuffer = ExpandUsbShortFormDevicePath (FilePath, FileSize);
        if (FileBuffer == NULL) {
          return NULL;
        }
      }
    }
  }

  //
  // Fix up the boot option path if it points to a FV in memory map style of device path
  //
  if (IsMemmapFvFilePath (*FilePath)) {
    FixupMemmapFvFilePath (FilePath);
    if (*FilePath == NULL) {
      return NULL;
    }
  }

  if (FileBuffer == NULL) {
    FileBuffer = GetFileBufferByFilePath (TRUE, *FilePath, FileSize, &AuthenticationStatus);
  }

  //
  // If we didn't find an image directly, we need to try as if it is a removable device boot option
  // and load the image according to the default boot behavior.
  //
  if (FileBuffer == NULL) {
    //
    // check if there is a bootable media could be found in this device path,
    // and get the bootable media handle
    //
    Handle = GetBootableDeviceHandle (*FilePath, &LoaderPathName);
    if (Handle != NULL) {
      //
      // Load the default boot file \EFI\BOOT\boot{machinename}.EFI from the media
      //  machinename is ia32, ia64, x64, ...
      //
      FreeAndSet (FilePath, FileDevicePath (Handle, LoaderPathName));
      ASSERT (*FilePath != NULL);
      FileBuffer = GetFileBufferByFilePath (TRUE, *FilePath, FileSize, &AuthenticationStatus);
    }
  }

  if (FileBuffer == NULL) {
    FreeAndSet (FilePath, NULL);
  }

  return FileBuffer;
}




/**
  Attempt to boot the EFI boot option. This routine sets L"BootCurent" and
  also signals the EFI ready to boot event. If the device path for the option
  starts with a BBS device path a legacy boot is attempted via the registered 
  gLegacyBoot function. Short form device paths are also supported via this 
  rountine. A device path starting with MEDIA_HARDDRIVE_DP, MSG_USB_WWID_DP,
  MSG_USB_CLASS_DP gets expaned out to find the first device that matches.
  If the BootOption Device Path fails the removable media boot algorithm 
  is attempted (\EFI\BOOTIA32.EFI, \EFI\BOOTX64.EFI,... only one file type 
  is tried per processor type)

  @param  BootOption    Boot Option to try and boot.
                        On return, BootOption->Status contains the boot status.
                        EFI_SUCCESS     BootOption was booted
                        EFI_UNSUPPORTED A BBS device path was found with no valid callback
                                        registered via EfiBootManagerInitialize().
                        EFI_NOT_FOUND   The BootOption was not found on the system
                        !EFI_SUCCESS    BootOption failed with this error status

**/
VOID
EFIAPI
EfiBootManagerBoot (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION             *BootOption
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                ImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL *ImageInfo;
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  VOID                      *FileBuffer;
  UINTN                     FileSize;
  BOOLEAN                   LoadOptionBoot;


  if (BootOption == NULL) {
    return;
  }

  if (BootOption->FilePath == NULL || BootOption->OptionType != LoadOptionTypeBoot) {
    BootOption->Status = EFI_INVALID_PARAMETER;
    return;
  }

  LoadOptionBoot = (BOOLEAN)((BootOption->Attributes & LOAD_OPTION_CATEGORY) == LOAD_OPTION_CATEGORY_BOOT);


  // 1. Create Boot#### for a temporary boot (i.e. a boot by selected a EFI Shell using "Boot From File")
  // 2. Update system variables: remove BootNext, set BootCurrent
  // 2.1. Remove BootNext
  // To prevent loops, the boot manager deletes BootNext before transferring control to the 
  // preselected boot option.
  //
  if (BootOption->BootNext) {
    Status = gRT->SetVariable (
                    L"BootNext",
                    &gEfiGlobalVariableGuid,
                    0,
                    0,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }
  //
  // 2.2. Set BootCurrent
  //
  Status = gRT->SetVariable (
                  L"BootCurrent",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (UINT16),
                  &BootOption->OptionNumber
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // 3. Signal the EVT_SIGNAL_READY_TO_BOOT event when we are about to load and execute
  //    the boot option.
// Fix: BOOT_SERVICE_APPLICATION not the last event to be logged in PCR[04]  
  if (LoadOptionBoot) {
    REPORT_STATUS_CODE (
      EFI_PROGRESS_CODE, 
      (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_READY_TO_BOOT_EVENT)
      );
    EfiSignalEventReadyToBoot();
  }

  //
  // 4. Load EFI boot option to ImageHandle
  //
  ImageHandle = NULL;
  if (DevicePathType (BootOption->FilePath) != BBS_DEVICE_PATH) {
    Status     = EFI_NOT_FOUND;
    FilePath   = DuplicateDevicePath (BootOption->FilePath);
    FileBuffer = LoadEfiBootOption (&FilePath, &FileSize);
    if (FileBuffer != NULL) {

      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, PcdGet32 (PcdProgressCodeOsLoaderLoad));

      Status = gBS->LoadImage (
                      TRUE,
                      gImageHandle,
                      FilePath,
                      FileBuffer,
                      FileSize,
                      &ImageHandle
                      );
      FreePool (FileBuffer);
      FreePool (FilePath);
    }

    if (EFI_ERROR (Status)) {
      REPORT_STATUS_CODE (
        EFI_ERROR_CODE, 
        (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_EC_BOOT_OPTION_LOAD_ERROR)
        );
      BootOption->Status = Status;
      return;
    }
  }

  //
  // 5. Adjust the different type memory page number just before booting
  //    and save the updated info into the variable for next boot to use
  //
  SetMemoryTypeInformationVariable(LoadOptionBoot);

  //
  // Record the performance data for End of BDS
  //
  PERF_END(NULL, "BDS", NULL, 0);

  PERF_CODE (
    WriteBootToOsPerformanceData ();
  );

  DEBUG_CODE_BEGIN();
    if (BootOption->Description == NULL) {
      DEBUG ((DEBUG_INFO | DEBUG_LOAD, "[Bds]Booting from unknown device path\n"));
    } else {
      DEBUG ((DEBUG_INFO | DEBUG_LOAD, "[Bds]Booting %s\n", BootOption->Description));
    }
  DEBUG_CODE_END();

  //
  // Check to see if we should legacy BOOT. If yes then do the legacy boot
  // Write boot to OS performance data for Legacy boot
  //
  if ((DevicePathType (BootOption->FilePath) == BBS_DEVICE_PATH) && (DevicePathSubType (BootOption->FilePath) == BBS_BBS_DP)) {
    if (mEfiBootManagerLegacyBoot != NULL) {
      mEfiBootManagerLegacyBoot (BootOption);
    } else {
      BootOption->Status = EFI_UNSUPPORTED;
    }

    return;
  }
 
  //
  // Provide the image with its load options
  //
  Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &ImageInfo);
  ASSERT_EFI_ERROR (Status);

  ImageInfo->LoadOptionsSize  = BootOption->OptionalDataSize;
  ImageInfo->LoadOptions      = BootOption->OptionalData;
  //
  // Before calling the image, enable the Watchdog Timer for 5 minutes period
  //
  gBS->SetWatchdogTimer (5 * 60, 0x0000, 0x00, NULL);

  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, PcdGet32 (PcdProgressCodeOsLoaderStart));

  Status = gBS->StartImage (ImageHandle, &BootOption->ExitDataSize, &BootOption->ExitData);
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Image Return Status = %r\n", Status));

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE, 
      (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_EC_BOOT_OPTION_FAILED)
      );
  }

  //
  // Clear the Watchdog Timer after the image returns
  //
  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);

  //
  // Clear Boot Current
  //
  gRT->SetVariable (
        L"BootCurrent",
        &gEfiGlobalVariableGuid,
        0,
        0,
        NULL
        );

  BootOption->Status = Status;
}


/**
  Check whether there is a instance in BlockIoDevicePath, which contain multi device path
  instances, has the same partition node with HardDriveDevicePath device path

  @param  BlockIoDevicePath      Multi device path instances which need to check
  @param  HardDriveDevicePath    A device path which starts with a hard drive media
                                 device path.

  @retval TRUE                   There is a matched device path instance.
  @retval FALSE                  There is no matched device path instance.

**/
BOOLEAN
MatchPartitionDevicePathNode (
  IN  EFI_DEVICE_PATH_PROTOCOL   *BlockIoDevicePath,
  IN  HARDDRIVE_DEVICE_PATH      *HardDriveDevicePath
  )
{
  HARDDRIVE_DEVICE_PATH     *TmpHdPath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  BOOLEAN                   Match;
  EFI_DEVICE_PATH_PROTOCOL  *BlockIoHdDevicePathNode;

  if ((BlockIoDevicePath == NULL) || (HardDriveDevicePath == NULL)) {
    return FALSE;
  }

  //
  // Make PreviousDevicePath == the device path node before the end node
  //
  DevicePath              = BlockIoDevicePath;
  BlockIoHdDevicePathNode = NULL;

  //
  // find the partition device path node
  //
  while (!IsDevicePathEnd (DevicePath)) {
    if ((DevicePathType (DevicePath) == MEDIA_DEVICE_PATH) &&
        (DevicePathSubType (DevicePath) == MEDIA_HARDDRIVE_DP)
        ) {
      BlockIoHdDevicePathNode = DevicePath;
      break;
    }

    DevicePath = NextDevicePathNode (DevicePath);
  }

  if (BlockIoHdDevicePathNode == NULL) {
    return FALSE;
  }
  //
  // See if the harddrive device path in blockio matches the orig Hard Drive Node
  //
  TmpHdPath = (HARDDRIVE_DEVICE_PATH *) BlockIoHdDevicePathNode;
  Match = FALSE;

  //
  // Check for the match
  //
  if ((TmpHdPath->MBRType == HardDriveDevicePath->MBRType) &&
      (TmpHdPath->SignatureType == HardDriveDevicePath->SignatureType)) {
    switch (TmpHdPath->SignatureType) {
    case SIGNATURE_TYPE_GUID:
      Match = CompareGuid ((EFI_GUID *)TmpHdPath->Signature, (EFI_GUID *)HardDriveDevicePath->Signature);
      break;
    case SIGNATURE_TYPE_MBR:
      Match = (BOOLEAN) (*((UINT32 *) (&(TmpHdPath->Signature[0]))) == ReadUnaligned32((UINT32 *)(&(HardDriveDevicePath->Signature[0]))));
      break;
    default:
      Match = FALSE;
      break;
    }
  }

  return Match;
}



EFI_STATUS
GetInternalShellDevPath (
    EFI_DEVICE_PATH_PROTOCOL **DevicePath
  )
{
  EFI_STATUS                         Status;
  EFI_DEVICE_PATH_PROTOCOL           *ShellDP;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;
  EFI_FIRMWARE_VOLUME2_PROTOCOL      *Fv;
  UINT32                             AuthenticationStatus;
  VOID                               *Buffer;
  UINTN                              Size;
	EFI_GUID                           *ShellGuid;


  if(DevicePath == NULL) {return EFI_INVALID_PARAMETER;}

  ShellGuid = (EFI_GUID*)PcdGetPtr(PcdShellFile);
  EfiInitializeFwVolDevicepathNode(&FileNode, ShellGuid);
  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID**)&LoadedImage
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (
                    LoadedImage->DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID**)&Fv
                    );
    if (!EFI_ERROR (Status)) {
      Buffer  = NULL;
      Size    = 0;
      Status  = Fv->ReadSection (
                      Fv,
                      ShellGuid,
                      EFI_SECTION_PE32,
                      0,
                      &Buffer,
                      &Size,
                      &AuthenticationStatus
                      );
      if (Buffer != NULL) {
        FreePool(Buffer);
      }
    }
  }
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  ShellDP = AppendDevicePathNode (
              DevicePathFromHandle(LoadedImage->DeviceHandle),
              (EFI_DEVICE_PATH_PROTOCOL*)&FileNode
              );
  if(ShellDP==NULL){
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    *DevicePath = ShellDP;
  }

  return Status;
}







/**
  Emuerate all possible bootable medias in the following order:
  1. Removable BlockIo            - The boot option only points to the removable media
                                    device, like USB key, DVD, Floppy etc.
  2. Fixed BlockIo                - The boot option only points to a Fixed blockIo device,
                                    like HardDisk.
  3. Non-BlockIo SimpleFileSystem - The boot option points to a device supporting
                                    SimpleFileSystem Protocol, but not supporting BlockIo
                                    protocol.
  4. LoadFile                     - The boot option points to the media supporting 
                                    LoadFile protocol.
  Reference: UEFI Spec chapter 3.3 Boot Option Variables Default Boot Behavior

  TODO: Do we need to add EfiBootManagerConnectAll() in this function? Impact to SLE platform is big.
**/
EFI_BOOT_MANAGER_LOAD_OPTION *
EFIAPI
EfiBootManagerEnumerateBootOptions (
  UINTN                                 *BootOptionCount
  )
{
  EFI_STATUS                            Status;
  EFI_BOOT_MANAGER_LOAD_OPTION          *BootOptions;
  UINT16                                NonBlockNumber;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *Handles;
  EFI_BLOCK_IO_PROTOCOL                 *BlkIo;
  UINTN                                 Removable;
  UINTN                                 Index;
  UINTN                                 FunctionIndex;
  CHAR16                                *Temp;
  CHAR16                                *DescriptionPtr;
  CHAR16                                Description[60];
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
  CHAR16                                *DevStr;
  CHAR16                                *BootTypeStr;
  CHAR16                                *IpTypeStr;
  UINT8                                 IpType;
  SETUP_DATA                            *SetupData;


  ASSERT (BootOptionCount != NULL);

  SetupData = (SETUP_DATA*)GetSetupDataHobData();

  *BootOptionCount = 0;
  BootOptions      = NULL;

  //
  // Parse removable block io followed by fixed block io
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiBlockIoProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );

  for (Removable = 0; Removable < 2; Removable++) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      Handles[Index],
                      &gEfiBlockIoProtocolGuid,
                      (VOID **) &BlkIo
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Skip the logical partitions
      //
      if (BlkIo->Media->LogicalPartition) {
        continue;
      }

      //
      // Skip the fixed block io then the removable block io
      //
      if (BlkIo->Media->RemovableMedia == ((Removable == 0) ? FALSE : TRUE)) {
        continue;
      }

      DescriptionPtr = NULL;
      for (FunctionIndex = 0; FunctionIndex < sizeof (mGetBootDescription) / sizeof (mGetBootDescription[0]); FunctionIndex++) {
        DescriptionPtr = mGetBootDescription[FunctionIndex] (Handles[Index]);
        if (DescriptionPtr != NULL) {
          break;
        }
      }

      if (DescriptionPtr == NULL) {
        continue;
      }

      //
      // Avoid description confusion between UEFI & Legacy boot option by adding "UEFI " prefix
      //
      Temp = StrCat (
               StrCpy (
                 AllocatePool (StrSize (DescriptionPtr) + sizeof (mUefiPrefix)),
                 mUefiPrefix
                 ),
               DescriptionPtr
               );
      FreePool (DescriptionPtr);
      DescriptionPtr = Temp;

      BootOptions = ReallocatePool (
                      sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                      sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                      BootOptions
                      );
      ASSERT (BootOptions != NULL);

      Status = EfiBootManagerInitializeLoadOption (
                 &BootOptions[(*BootOptionCount)++],
                 LoadOptionNumberUnassigned,
                 LoadOptionTypeBoot,
                 LOAD_OPTION_ACTIVE,
                 DescriptionPtr,
                 DevicePathFromHandle (Handles[Index]),
                 NULL,
                 0
                 );
      ASSERT_EFI_ERROR (Status);

      FreePool (DescriptionPtr);
    }
  }

  if (HandleCount != 0) {
    FreePool (Handles);
  }

  //
  // Parse simple file system not based on block io
  //
  NonBlockNumber = 0;
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiSimpleFileSystemProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlkIo
                    );
     if (!EFI_ERROR (Status)) {
      //
      //  Skip if the file system handle supports a BlkIo protocol, which we've handled in above
      //
      continue;
    }
    UnicodeSPrint (Description, sizeof (Description), NonBlockNumber > 0 ? L"%s %d" : L"%s", L"UEFI Non-Block Boot Device", NonBlockNumber);
    
    BootOptions = ReallocatePool (
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                    BootOptions
                    );
    ASSERT (BootOptions != NULL);

    Status = EfiBootManagerInitializeLoadOption (
               &BootOptions[(*BootOptionCount)++],
               LoadOptionNumberUnassigned,
               LoadOptionTypeBoot,
               LOAD_OPTION_ACTIVE,
               Description,
               DevicePathFromHandle (Handles[Index]),
               NULL,
               0
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (HandleCount != 0) {
    FreePool (Handles);
  }

  //
  // Parse load file, assuming UEFI Network boot option
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiLoadFileProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );
  
  for (Index = 0; Index < HandleCount; Index++) {
    IpType      = 0xFF;
    DevStr      = L"";
    BootTypeStr = L"";
    Status = gBS->HandleProtocol(
                    Handles[Index],
                    &gEfiDevicePathProtocolGuid,
                    &DevicePath
                    );
    if (!EFI_ERROR (Status)) {
      IpType = GetEfiNetWorkType(DevicePath, &BootTypeStr);
      DevStr = GetEfiNetWorkString(DevicePath);
    }

    if ((!SetupData->UefiPxeIPV4 && IpType == MSG_IPv4_DP) || 
        (!SetupData->UefiPxeIPV6 && IpType == MSG_IPv6_DP) ||
        IpType == 0xFF) {
      continue;
    }

    switch(IpType){
      case MSG_IPv4_DP:
        IpTypeStr = L"IPv4";
        break;
      case MSG_IPv6_DP:
        IpTypeStr = L"IPv6";
        break;
      default:
        IpTypeStr = L"";
        break;
    }    

    UnicodeSPrint(
      Description, 
      sizeof(Description), 
      L"UEFI %s %s %s", 
      DevStr, 
      BootTypeStr,
      IpTypeStr
      );
    
    BootOptions = ReallocatePool (
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                    BootOptions
                    );
    ASSERT (BootOptions != NULL);

    Status = EfiBootManagerInitializeLoadOption (
               &BootOptions[(*BootOptionCount)++],
               LoadOptionNumberUnassigned,
               LoadOptionTypeBoot,
               LOAD_OPTION_ACTIVE,
               Description,
               DevicePathFromHandle (Handles[Index]),
               NULL,
               0
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (HandleCount != 0) {
    FreePool (Handles);
  }


// Shell
  //if(PcdGet8(PcdInternalShellEnable))
  {
    Status = GetInternalShellDevPath(&DevicePath);
    if(!EFI_ERROR(Status)){
      BootOptions = ReallocatePool (
                      sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                      sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                      BootOptions
                      );
      ASSERT (BootOptions != NULL);

      Status = EfiBootManagerInitializeLoadOption (
                 &BootOptions[(*BootOptionCount)++],
                 LoadOptionNumberUnassigned,
                 LoadOptionTypeBoot,
                 LOAD_OPTION_ACTIVE,
                 L"Internal EDK Shell",
                 DevicePath,
                 NULL,
                 0
                 );
      ASSERT_EFI_ERROR (Status);
  		
      FreePool(DevicePath);
    }		
  }




// Mark the boot option as added by bds by setting OptionalData to a special GUID
  for (Index = 0; Index < *BootOptionCount; Index++) {
    BootOptions[Index].OptionalData     = AllocateCopyPool(sizeof(gEfiByoBootOptionFlag), &gEfiByoBootOptionFlag);
    BootOptions[Index].OptionalDataSize = sizeof(gEfiByoBootOptionFlag);
  }

  return BootOptions;
}

/**
  The function enumerates all boot options, creates them and registers them in the BootOrder variable.
**/
VOID
EFIAPI
EfiBootManagerRefreshAllBootOption (
  VOID
  )
{
  EFI_BOOT_MANAGER_LOAD_OPTION  *NvBootOptions;
  UINTN                         NvBootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         BootOptionCount;
  UINTN                         Index;


  if(PcdGetBool(PcdRefreshAllBootOptionOccurred)){
    return;
  }
  PcdSetBool(PcdRefreshAllBootOptionOccurred, TRUE);

  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));  

// Legacy
  if (mEfiBootManagerRefreshLegacyBootOption != NULL) {
    mEfiBootManagerRefreshLegacyBootOption();
  }

// Uefi
  if(PcdGet8(PcdBiosBootModeType) == 2){  // legacy only
    EfiBootManagerDeleteAllUefiBootOption();
    return;		
  }		

  BootOptions   = EfiBootManagerEnumerateBootOptions (&BootOptionCount);
  NvBootOptions = EfiBootManagerGetLoadOptions (&NvBootOptionCount, LoadOptionTypeBoot);

  //
  // Remove invalid EFI boot options from NV
  //
  for (Index = 0; Index < NvBootOptionCount; Index++) {
    if (DevicePathType(NvBootOptions[Index].FilePath) == BBS_DEVICE_PATH && 
        DevicePathSubType(NvBootOptions[Index].FilePath) == BBS_BBS_DP) {
      continue;
    }				
    if(NvBootOptions[Index].OptionalDataSize == sizeof(gEfiByoBootOptionFlag) &&
      CompareMem(NvBootOptions[Index].OptionalData, &gEfiByoBootOptionFlag, sizeof(gEfiByoBootOptionFlag))==0){
// Only check those added by platform bds
// so that the boot options added by end-user or OS installer won't be deleted
      if (EfiBootManagerFindLoadOption (&NvBootOptions[Index], BootOptions, BootOptionCount) == (UINTN) -1) {
        EfiBootManagerDeleteLoadOptionVariable (NvBootOptions[Index].OptionNumber, LoadOptionTypeBoot);
      }
    }
  }

  //
  // Add new EFI boot options to NV
  //
  for (Index = 0; Index < BootOptionCount; Index++) {
    if (EfiBootManagerFindLoadOption (&BootOptions[Index], NvBootOptions, NvBootOptionCount) == (UINTN) -1) {
      EfiBootManagerAddLoadOptionVariable (&BootOptions[Index], (UINTN) -1);
    }
  }

  EfiBootManagerFreeLoadOptions (BootOptions,   BootOptionCount);
  EfiBootManagerFreeLoadOptions (NvBootOptions, NvBootOptionCount);
}


/**
  For a bootable Device path, return its boot type.

  @param  DevicePath                   The bootable device Path to check

  @retval AcpiFloppyBoot               If given device path contains ACPI_DEVICE_PATH type device path node
                                       which HID is floppy device.
  @retval MessageAtapiBoot             If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_ATAPI_DP.
  @retval MessageSataBoot              If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_SATA_DP.
  @retval MessageScsiBoot              If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_SCSI_DP.
  @retval MessageUsbBoot               If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_USB_DP.
  @retval MessageNetworkBoot           If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_MAC_ADDR_DP, MSG_VLAN_DP,
                                       MSG_IPv4_DP or MSG_IPv6_DP.
  @retval UnsupportedBoot              If tiven device path doesn't match the above condition, it's not supported.

**/
BOOT_TYPE
BootTypeFromDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *Node;
  EFI_DEVICE_PATH_PROTOCOL      *LastDeviceNode;

  ASSERT (DevicePath != NULL);

  for (Node = DevicePath; !IsDevicePathEndType (Node); Node = NextDevicePathNode (Node)) {
    switch (DevicePathType (Node)) {

      case ACPI_DEVICE_PATH:
        if (EISA_ID_TO_NUM (((ACPI_HID_DEVICE_PATH *) Node)->HID) == 0x0604) {
          return AcpiFloppyBoot;
        }
        break;

      case MESSAGING_DEVICE_PATH:
        //
        // Skip LUN device node
        //
        LastDeviceNode=Node;
        while (DevicePathSubType(LastDeviceNode) == MSG_DEVICE_LOGICAL_UNIT_DP){
                LastDeviceNode = NextDevicePathNode (LastDeviceNode); 
		}

        //
        // if the device path not only point to driver device, it is not a messaging device path,
        //
        if (!IsDevicePathEndType (LastDeviceNode)) {
          break;
        }

        switch (DevicePathSubType (Node)) {
        case MSG_ATAPI_DP:
          return MessageAtapiBoot;
          break;

        case MSG_SATA_DP:
          return MessageSataBoot;
          break;

        case MSG_USB_DP:
          return MessageUsbBoot;
          break;

        case MSG_SCSI_DP:
          return MessageScsiBoot;
          break;

        case MSG_MAC_ADDR_DP:
        case MSG_VLAN_DP:
        case MSG_IPv4_DP:
        case MSG_IPv6_DP:
          return MessageNetworkBoot;
          break;
        }
    }
  }

  return UnsupportedBoot;
}


