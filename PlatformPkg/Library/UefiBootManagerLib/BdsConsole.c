
#include "InternalBdsLib.h"
#include <Library/IoLib.h>
#include <Library/LogoLib.h>
#include <Protocol/IsaIo.h>
#include <SetupVariable.h>

#define   PCI_USB_UHCI_CLASS        0x0C030000
#define   PCI_USB_OHCI_CLASS        0x0C031000
#define   PCI_USB_EHCI_CLASS        0x0C032000
#define   PCI_USB_XHCI_CLASS        0x0C033000

CHAR16       *mConVarName[] = {
  L"ConIn",
  L"ConOut",
  L"ErrOut",
  L"ConInDev",
  L"ConOutDev",
  L"ErrOutDev"
};

BOOLEAN   PEGConnect = FALSE;
BOOLEAN	  PCIConnect = FALSE; 
EFI_GUID  gEfiVirtualIsaIoProtocolGuid = { 0x893e441f, 0x5a6a, 0x40d1, { 0x83, 0xcb, 0x8a, 0xa1, 0x6, 0xdd, 0xd7, 0xab } };

/**
  Performs an ISA I/O Read Cycle

  @param[in]  This              A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in]  Width             Specifies the width of the I/O operation.
  @param[in]  Offset            The offset in ISA I/O space to start the I/O operation.
  @param[in]  Count             The number of I/O operations to perform.
  @param[out] Buffer            The destination buffer to store the results

  @retval EFI_SUCCESS           The data was read from the device sucessfully.
  @retval EFI_UNSUPPORTED       The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
IsaIoIoRead (
  IN  EFI_ISA_IO_PROTOCOL        *This,
  IN  EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN  UINT32                     Offset,
  IN  UINTN                      Count,
  OUT VOID                       *Buffer
  )
{
  UINT8  *Data;

  Data  = (UINT8 *)Buffer;
  *Data = IoRead8 ((UINT8)Offset);
  return EFI_SUCCESS;
}

/**
  Performs an ISA I/O Write Cycle

  @param[in] This                A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Width               Specifies the width of the I/O operation.
  @param[in] Offset              The offset in ISA I/O space to start the I/O operation.
  @param[in] Count               The number of I/O operations to perform.
  @param[in] Buffer              The source buffer to write data from

  @retval EFI_SUCCESS            The data was writen to the device sucessfully.
  @retval EFI_UNSUPPORTED        The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
IsaIoIoWrite (
  IN EFI_ISA_IO_PROTOCOL        *This,
  IN EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN UINT32                     Offset,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  )
{
  UINT8  *Data;

  Data = Buffer;
  IoWrite8 ((UINT8)Offset, *Data);
  return EFI_SUCCESS;
}

//
// Module Variables
//
EFI_ISA_IO_PROTOCOL mIsaIoInterface = {
  {
    NULL,
    NULL
  },
  {
    IsaIoIoRead,
    IsaIoIoWrite
  },
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  0,
  NULL
};

EFI_STATUS
CheckConoutDevice (
)
{
  EFI_DEVICE_PATH_PROTOCOL  *StartDevicePath;
  UINTN                     VariableSize;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *CopyOfDevicePath;
  UINTN                     Size;


  PCI_DEVICE_PATH           *PciDevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  CONSOLE_TYPE              ConsoleType = ConOut;
  //
  // Check if the console variable exist
  //
  StartDevicePath = EfiBootManagerGetVariableAndSize (
                      mConVarName[ConsoleType],
                      &gEfiGlobalVariableGuid,
                      &VariableSize
                      );
  if (StartDevicePath == NULL) {
    return EFI_UNSUPPORTED;
  }

  CopyOfDevicePath = StartDevicePath;
  do {
    //
    // Check every instance of the console variable
    //
    Instance  = GetNextDevicePathInstance (&CopyOfDevicePath, &Size);
    if (Instance == NULL) {
      FreePool (StartDevicePath);
      return EFI_UNSUPPORTED;
    }
    DevicePath  = Instance;
    while (!IsDevicePathEnd (DevicePath)) {
      if((DevicePath->Type == HARDWARE_DEVICE_PATH) && (DevicePath->SubType == HW_PCI_DP)) {
        PciDevicePathNode = (PCI_DEVICE_PATH *) DevicePath;
        if((PciDevicePathNode->Device == 0x02) && ( PciDevicePathNode->Function == 0)) {
          PEGConnect = TRUE;
        }
        if((PciDevicePathNode->Device == 0x13) && ( PciDevicePathNode->Function == 0)) {
          PCIConnect = TRUE;
        }
    break;
      }
      PciDevicePathNode    = (PCI_DEVICE_PATH *) DevicePath;	
      DevicePath    = NextDevicePathNode (DevicePath);		
    }
    FreePool(Instance);
  } while (CopyOfDevicePath != NULL);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InstallVirtualIsaIo (
  IN OUT EFI_HANDLE *VirtualController
)
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT32                    PciClassCode;

  //
  //Install the NULL ISA I/O protocol on the Virtual Controller handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  VirtualController,
                  &gEfiVirtualIsaIoProtocolGuid,
                  &mIsaIoInterface,
                  NULL
                  );

  //
  // Here we need to connect USB host controller. Then the USB SMM driver
  // can start off bus enumeration and prepare for legacy service including port 60/64
  // trap. This is necessary to make virtual keyboard work.
  //
  HandleCount = 0;
  HandleBuffer = NULL;
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiPciIoProtocolGuid,
         NULL,
         &HandleCount,
         &HandleBuffer
         );
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    &PciIo
                    );
    if (!EFI_ERROR(Status)) {
      PciIo->Pci.Read (
                   PciIo,
                   EfiPciIoWidthUint32,
                   8,
                   1,
                   &PciClassCode
                   );
      PciClassCode &= 0x0FFFFFF00;
      if ((PciClassCode == PCI_USB_UHCI_CLASS) || (PciClassCode == PCI_USB_OHCI_CLASS) ||
          (PciClassCode == PCI_USB_EHCI_CLASS) || (PciClassCode == PCI_USB_XHCI_CLASS)) {
        gBS->ConnectController (HandleBuffer[Index], NULL, NULL, FALSE);
        break;
      }
    }
  }
  return Status;
}

/**
  Search out the video controller.

  @return  PCI device path of the video controller.
**/
EFI_HANDLE
GetVideoController (
  VOID
  )
{
  EFI_STATUS                Status;
  UINTN                     RootBridgeHandleCount;
  EFI_HANDLE                *RootBridgeHandleBuffer;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     RootBridgeIndex;
  UINTN                     Index;
  EFI_HANDLE                VideoController;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;

  //
  // Make all the PCI_IO protocols show up
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  &RootBridgeHandleCount,
                  &RootBridgeHandleBuffer
                  );
  if (EFI_ERROR (Status) || (RootBridgeHandleCount == 0)) {
    return NULL;
  }

  VideoController = NULL;
  for (RootBridgeIndex = 0; RootBridgeIndex < RootBridgeHandleCount; RootBridgeIndex++) {
    gBS->ConnectController (RootBridgeHandleBuffer[RootBridgeIndex], NULL, NULL, FALSE);

    //
    // Start to check all the pci io to find the first video controller
    //
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiPciIoProtocolGuid,
                    NULL,
                    &HandleCount,
                    &HandleBuffer
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, &PciIo);
      if (!EFI_ERROR (Status)) {
        //
        // Check for all video controller
        //
        Status = PciIo->Pci.Read (
                          PciIo,
                          EfiPciIoWidthUint32,
                          0,
                          sizeof (Pci) / sizeof (UINT32),
                          &Pci
                          );
        if (!EFI_ERROR (Status) && IS_PCI_VGA (&Pci)) {
          // TODO: use IS_PCI_DISPLAY??
          VideoController = HandleBuffer[Index];
          break;
        }
      }
    }
    FreePool (HandleBuffer);

    if (VideoController != NULL) {
      break;
    }
  }
  FreePool (RootBridgeHandleBuffer);
  
  return VideoController;
}

/**
  Query all the children of VideoController and return the device paths of all the 
  children that support GraphicsOutput protocol.

  @param VideoController       PCI handle of video controller.

  @return  Device paths of all the children that support GraphicsOutput protocol.
**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
EfiBootManagerGetGopDevicePath (
  IN      EFI_HANDLE  VideoController,
  IN OUT  EFI_HANDLE  *GopHandle
  )
{
  UINTN                                Index;
  EFI_STATUS                           Status;
  EFI_DEVICE_PATH_PROTOCOL             *ChildDevicePath;
  EFI_DEVICE_PATH_PROTOCOL             *GopPool;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         NumberOfHandles;

  ASSERT (VideoController != NULL);
  GopPool = NULL;
  HandleBuffer = NULL;
  //
  // Locate protocol.
  //
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiGraphicsOutputProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );
  for (Index = 0; Index < NumberOfHandles; Index++) {
    Status = gBS->OpenProtocol (
                    HandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **) &ChildDevicePath,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      GopPool = AppendDevicePathInstance (GopPool, ChildDevicePath);
      *GopHandle = HandleBuffer[Index];
      break;
    }
  }
  if (HandleBuffer) {
    gBS->FreePool (HandleBuffer);
  }
  return GopPool;
}

/**
  Find the platform active active video controller and connect it.

  @retval EFI_NOT_FOUND There is no active video controller.
  @retval EFI_SUCCESS   The video controller is connected.
**/
EFI_STATUS
ConnectVideoController (
  VOID
  )
{
  EFI_HANDLE                 VideoController;
  EFI_DEVICE_PATH_PROTOCOL   *Gop;
  EFI_STATUS                 Status;
  EFI_HANDLE                 GopHandle;
  //
  // Get the platform vga device
  //
  VideoController = GetVideoController ();
  
  if (VideoController == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Try to connect the PCI device path, so that GOP dirver could start on this 
  // device and create child handles with GraphicsOutput Protocol installed
  // on them, then we get device paths of these child handles and select 
  // them as possible console device.
  //
  gBS->ConnectController (VideoController, NULL, NULL, FALSE);

  Gop = EfiBootManagerGetGopDevicePath (VideoController, &GopHandle);
  if (Gop == NULL) {
    //
    // Last chance - just try VGA controller
    // Don't assume all VGA controller support GOP operation
    //
    DEBUG ((EFI_D_ERROR, "[Bds] GOP not found, try VGA device path!\n"));
    Gop = DuplicateDevicePath (DevicePathFromHandle (VideoController));
    if (Gop == NULL) {
      return EFI_NOT_FOUND;
    }
  }

  EfiBootManagerUpdateConsoleVariable (ConOut, Gop, NULL);
  FreePool (Gop);

  //
  // Necessary for ConPlatform and ConSplitter driver to start up again after ConOut is updated.
  //
  Status = gBS->ConnectController (GopHandle, NULL, NULL, TRUE);
  DEBUG ((EFI_D_ERROR, "GOP Status:%r\n", Status));
  return Status;
}

/**
  Check if the video controller is connected.

  @retval TRUE  The video controller is connected.
  @retval FALSE The video controller isn't connected.
**/
BOOLEAN
HasLocalDisplay (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         HandleCount;

  HandleCount  = 0;
  HandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleTextOutProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  while (HandleCount-- != 0) {
    Status = gBS->OpenProtocol (
                    HandleBuffer[HandleCount],
                    &gEfiGraphicsOutputProtocolGuid,
                    NULL,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      Status = gBS->OpenProtocol (
                      HandleBuffer[HandleCount],
                      &gEfiDevicePathProtocolGuid,
                      NULL,
                      NULL,
                      NULL,
                      EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                      );
      if (!EFI_ERROR (Status)) {
        return TRUE;
      }
    }
  }
  
  return FALSE;  
}

/**
  Fill console handle in System Table if there are no valid console handle in.

  Firstly, check the validation of console handle in System Table. If it is invalid,
  update it by the first console device handle from EFI console variable. 

  @param  VarName            The name of the EFI console variable.
  @param  ConsoleGuid        Specified Console protocol GUID.
  @param  ConsoleHandle      On IN,  console handle in System Table to be checked. 
                             On OUT, new console hanlde in system table.
  @param  ProtocolInterface  On IN,  console protocol on console handle in System Table to be checked. 
                             On OUT, new console protocol on new console hanlde in system table.

  @retval TRUE               System Table has been updated.
  @retval FALSE              System Table hasn't been updated.

**/
BOOLEAN 
UpdateSystemTableConsole (
  IN     CHAR16                          *VarName,
  IN     EFI_GUID                        *ConsoleGuid,
  IN OUT EFI_HANDLE                      *ConsoleHandle,
  IN OUT VOID                            **ProtocolInterface
  )
{
  EFI_STATUS                Status;
  UINTN                     DevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *FullDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *VarConsole;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  VOID                      *Interface;
  EFI_HANDLE                NewHandle;

  ASSERT (VarName != NULL);
  ASSERT (ConsoleHandle != NULL);
  ASSERT (ConsoleGuid != NULL);
  ASSERT (ProtocolInterface != NULL);

  if (*ConsoleHandle != NULL) {
    Status = gBS->HandleProtocol (
                   *ConsoleHandle,
                   ConsoleGuid,
                   &Interface
                   );
    if (Status == EFI_SUCCESS && Interface == *ProtocolInterface) {
      //
      // If ConsoleHandle is valid and console protocol on this handle also
      // also matched, just return.
      //
      return FALSE;
    }
  }
  
  //
  // Get all possible consoles device path from EFI variable
  //
  VarConsole = EfiBootManagerGetVariableAndSize (
                 VarName,
                 &gEfiGlobalVariableGuid,
                 &DevicePathSize
                 );
  if (VarConsole == NULL) {
    //
    // If there is no any console device, just return.
    //
    return FALSE;
  }

  FullDevicePath = VarConsole;

  do {
    //
    // Check every instance of the console variable
    //
    Instance  = GetNextDevicePathInstance (&VarConsole, &DevicePathSize);
    if (Instance == NULL) {
      DEBUG ((EFI_D_ERROR, "[Bds] No valid console instance is found for %s!\n", VarName));
      // We should not ASSERT when all the console devices are removed.
      // ASSERT_EFI_ERROR (EFI_NOT_FOUND);
      FreePool (FullDevicePath);
      return FALSE;
    }
    
    //
    // Find console device handle by device path instance
    //
    Status = gBS->LocateDevicePath (
                    ConsoleGuid,
                    &Instance,
                    &NewHandle
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Get the console protocol on this console device handle
      //
      Status = gBS->HandleProtocol (
                      NewHandle,
                      ConsoleGuid,
                      &Interface
                      );
      if (!EFI_ERROR (Status)) {
        //
        // Update new console handle in System Table.
        //
        DEBUG ((EFI_D_ERROR, "UpdateSystemTableConsole, VarName %s!\n", VarName));
        DEBUG ((EFI_D_ERROR, "UpdateSystemTableConsole, Return TURE.\n"));
        
        *ConsoleHandle     = NewHandle;
        *ProtocolInterface = Interface;
        return TRUE;
      }
    }

  } while (Instance != NULL);

  //
  // No any available console devcie found.
  //
  return FALSE;
}

/**
  This function update console variable based on ConVarName, it can
  add or remove one specific console device path from the variable

  @param  ConVarName               Console related variable name, ConIn, ConOut,
                                   ErrOut.
  @param  CustomizedConDevicePath  The console device path which will be added to
                                   the console variable ConVarName, this parameter
                                   can not be multi-instance.
  @param  ExclusiveDevicePath      The console device path which will be removed
                                   from the console variable ConVarName, this
                                   parameter can not be multi-instance.

  @retval EFI_UNSUPPORTED          The added device path is same to the removed one.
  @retval EFI_SUCCESS              Success add or remove the device path from  the
                                   console variable.

**/
EFI_STATUS
EFIAPI
EfiBootManagerUpdateConsoleVariable (
  IN  CONSOLE_TYPE              ConsoleType,
  IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *VarConsole;
  UINTN                     DevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;

  if (ConsoleType >= sizeof (mConVarName) / sizeof (mConVarName[0])) {
    return EFI_INVALID_PARAMETER;
  }

  VarConsole      = NULL;
  DevicePathSize  = 0;

  //
  // Notes: check the device path point, here should check
  // with compare memory
  //
  if (CustomizedConDevicePath == ExclusiveDevicePath) {
    return EFI_UNSUPPORTED;
  }
  //
  // Delete the ExclusiveDevicePath from current default console
  //
  VarConsole = EfiBootManagerGetVariableAndSize (
                 mConVarName[ConsoleType],
                 &gEfiGlobalVariableGuid,
                 &DevicePathSize
                 );

  //
  // Initialize NewDevicePath
  //
  NewDevicePath = VarConsole;

  //
  // If ExclusiveDevicePath is even the part of the instance in VarConsole, delete it.
  // In the end, NewDevicePath is the final device path.
  //
  if (ExclusiveDevicePath != NULL && VarConsole != NULL) {
      NewDevicePath = EfiBootManagerDelPartMatchInstance (VarConsole, ExclusiveDevicePath);
  }
  //
  // Try to append customized device path to NewDevicePath.
  //
  if (CustomizedConDevicePath != NULL) {
    if (!EfiBootManagerMatchDevicePaths (NewDevicePath, CustomizedConDevicePath)) {
      //
      // Check if there is part of CustomizedConDevicePath in NewDevicePath, delete it.
      //
      NewDevicePath = EfiBootManagerDelPartMatchInstance (NewDevicePath, CustomizedConDevicePath);
      //
      // In the first check, the default console variable will be _ModuleEntryPoint,
      // just append current customized device path
      //
      TempNewDevicePath = NewDevicePath;
      NewDevicePath = AppendDevicePathInstance (NewDevicePath, CustomizedConDevicePath);
      if (TempNewDevicePath != NULL) {
        FreePool(TempNewDevicePath);
      }
    }
  }

  //
  // Finally, Update the variable of the default console by NewDevicePath
  //
  gRT->SetVariable (
         mConVarName[ConsoleType],
         &gEfiGlobalVariableGuid,
         EFI_VARIABLE_BOOTSERVICE_ACCESS
         | EFI_VARIABLE_RUNTIME_ACCESS
         | ((ConsoleType < ConInDev) ? EFI_VARIABLE_NON_VOLATILE : 0),
         GetDevicePathSize (NewDevicePath),
         NewDevicePath
         );

  if (VarConsole == NewDevicePath) {
    if (VarConsole != NULL) {
      FreePool(VarConsole);
    }
  } else {
    if (VarConsole != NULL) {
      FreePool(VarConsole);
    }
    if (NewDevicePath != NULL) {
      FreePool(NewDevicePath);
    }
  }

  return EFI_SUCCESS;

}


EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
EfiBootManagerUpdateConsoleDP (
  IN  EFI_DEVICE_PATH_PROTOCOL  *VarConsole,
  IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath2;	


  ASSERT(CustomizedConDevicePath != ExclusiveDevicePath);

  NewDevicePath = DuplicateDevicePath(VarConsole);
	TempNewDevicePath2 = NewDevicePath;
	
  if (NewDevicePath != NULL && ExclusiveDevicePath != NULL) {
    NewDevicePath = EfiBootManagerDelPartMatchInstance(NewDevicePath, ExclusiveDevicePath);
  } 
	
  if (CustomizedConDevicePath != NULL) {
    if (!EfiBootManagerMatchDevicePaths (NewDevicePath, CustomizedConDevicePath)) {
      NewDevicePath = EfiBootManagerDelPartMatchInstance(NewDevicePath, CustomizedConDevicePath);
      TempNewDevicePath = NewDevicePath;
      NewDevicePath = AppendDevicePathInstance (NewDevicePath, CustomizedConDevicePath);
      if (TempNewDevicePath != NULL) { FreePool(TempNewDevicePath); }
    }
  }

  if(VarConsole!=NULL){FreePool(VarConsole);}
  if(NewDevicePath != TempNewDevicePath2 && TempNewDevicePath2!=NULL){FreePool(TempNewDevicePath2);}
  return NewDevicePath;
}



/**
  Connect the console device base on the variable ConVarName, if
  device path of the ConVarName is multi-instance device path, if
  anyone of the instances is connected success, then this function
  will return success.

  @param  ConsoleType              ConIn, ConOut or ErrOut.

  @retval EFI_NOT_FOUND            There is not any console devices connected
                                   success
  @retval EFI_SUCCESS              Success connect any one instance of the console
                                   device path base on the variable ConVarName.

**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectConsoleVariable (
  IN  CONSOLE_TYPE              ConsoleType
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *StartDevicePath;
  UINTN                     VariableSize;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *Next;
  EFI_DEVICE_PATH_PROTOCOL  *CopyOfDevicePath;
  UINTN                     Size;
  BOOLEAN                   DeviceExist;
  PCI_DEVICE_PATH           *PciDevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  BOOLEAN                   PEGDevicePath = FALSE;
  BOOLEAN                   PCIDevicePath = FALSE;
  SETUP_DATA                SystemConfiguration;
  UINTN                     VarSize;
  
  if ((ConsoleType != ConIn) && (ConsoleType != ConOut) && (ConsoleType != ErrOut)) {
    return EFI_INVALID_PARAMETER;
  }
  Status      = EFI_SUCCESS;
  DeviceExist = FALSE;

  if(ConsoleType == ConOut ) CheckConoutDevice();

  VarSize = sizeof (SETUP_DATA);
  Status = gRT->GetVariable (
                  PLATFORM_SETUP_VARIABLE_NAME,
                  &gPlatformSetupVariableGuid,
                  NULL,
                  &VarSize,
                  &SystemConfiguration
                  );
  //
  // Check if the console variable exist
  //
  StartDevicePath = EfiBootManagerGetVariableAndSize (
                      mConVarName[ConsoleType],
                      &gEfiGlobalVariableGuid,
                      &VariableSize
                      );
  if (StartDevicePath == NULL) {
    DEBUG((EFI_D_ERROR, "L\"%s\" not found\n", mConVarName[ConsoleType]));
    return EFI_UNSUPPORTED;
  }
  ShowDevicePath(L"DP:", StartDevicePath, L"\r\n");

  CopyOfDevicePath = StartDevicePath;
  do {
    //
    // Check every instance of the console variable
    //
    PEGDevicePath = FALSE;
    PCIDevicePath = FALSE; 
    Instance  = GetNextDevicePathInstance (&CopyOfDevicePath, &Size);
    if (Instance == NULL) {
      FreePool (StartDevicePath);
      return EFI_UNSUPPORTED;
    }
    if(ConsoleType == ConOut ) {
      DevicePath  = Instance;
      while (!IsDevicePathEnd (DevicePath)) {
        if((DevicePath->Type == 0x01) && (DevicePath->SubType == 0x01)) {
          PciDevicePathNode    = (PCI_DEVICE_PATH *) DevicePath;
          if((PciDevicePathNode->Device == 0x02) && ( PciDevicePathNode->Function == 0)) {
            PEGDevicePath = TRUE;
          }
          if((PciDevicePathNode->Device == 0x13) && ( PciDevicePathNode->Function == 0)) {
            PCIDevicePath = TRUE;
          }
          break;
        }
        DevicePath = NextDevicePathNode (DevicePath);		
      }
    }
    Next = Instance;
    while (!IsDevicePathEndType (Next)) {
      Next = NextDevicePathNode (Next);
    }

    SetDevicePathEndNode (Next);
    //
    // Connect the USB console
    // USB console device path is a short-form device path that 
    //  starts with the first element being a USB WWID
    //  or a USB Class device path
    //
//  ShowDevicePath(L"Instance:", Instance, UNI_CRLF);    
    if ((DevicePathType (Instance) == MESSAGING_DEVICE_PATH) &&
        ((DevicePathSubType (Instance) == MSG_USB_CLASS_DP) || (DevicePathSubType (Instance) == MSG_USB_WWID_DP))
       ) {
      Status = EfiBootManagerConnectUsbShortFormDevicePath (Instance);
      if (!EFI_ERROR (Status)) {
        DeviceExist = TRUE;
      }
    } else {
      //
      // Connect the instance device path
      //
      Status = EfiBootManagerConnectDevicePath (Instance, NULL);
      DEBUG((EFI_D_INFO, "EfiBootManagerConnectDevicePath():%r\n", Status));
      if (!EFI_ERROR (Status)) {
        DeviceExist = TRUE;
      }
    }
    FreePool(Instance);
  } while (CopyOfDevicePath != NULL);

  FreePool (StartDevicePath);
  if ((ConsoleType == ConOut) && !HasLocalDisplay()) {
    //
    // Force to connect local video controller
    // Backward compatible to old platforms which don't insert the GOP device path to ConOut
    //
    DEBUG ((EFI_D_ERROR, "[Bds] Local display isn't found, find & connect it automatically\n"));
    Status = ConnectVideoController ();
    if (!EFI_ERROR (Status)) {
      DeviceExist = TRUE;
    }
  }

  if (!DeviceExist) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}


/**
  This function will search every input/output device in current system,
  and make every input/output device as potential console device.
**/
VOID
EFIAPI
EfiBootManagerConnectAllDefaultConsoles (
  VOID
  )
{
  BOOLEAN                   SystemTableUpdated;
  EFI_STATUS                Status;

  DEBUG((EFI_D_INFO, "ConnectDefConsoles\n"));  
  
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_PC_INIT));
  PERF_START (NULL, "ConnectConOut", "BDS", 0);	
  Status = EfiBootManagerConnectConsoleVariable (ConOut);
  PERF_END   (NULL, "ConnectConOut", "BDS", 0);	
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (EFI_ERROR_CODE, (EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_NOT_DETECTED));  
  } else {
// ConSplitterConOutDriverBindingStart.ConSplitterTextOutAddDevice() 
// will clear screen and enable EnableCursor.
    if(gST->ConOut!=NULL){gST->ConOut->EnableCursor(gST->ConOut, FALSE);}
    if (!PcdGetBool(PcdLogoHasShowed)) {            // show logo here, before connect ConIn
      EnableQuietBoot (PcdGetPtr(PcdLogoFile));
      PcdSetBool(PcdLogoHasShowed, TRUE);
    }
  }

  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_INIT));
  PERF_START (NULL, "ConnectConIn", "BDS", 0);	
  Status = EfiBootManagerConnectConsoleVariable (ConIn);
  PERF_END   (NULL, "ConnectConIn", "BDS", 0);	
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (EFI_ERROR_CODE, (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_NOT_DETECTED));  
  }

  //
  // The _ModuleEntryPoint err out var is legal.
  //
//-EfiBootManagerConnectConsoleVariable (ErrOut);
//-PERF_START (NULL, "ErrOutReady", "BDS", 1);
//-PERF_END   (NULL, "ErrOutReady", "BDS", 0);

  SystemTableUpdated = FALSE;
  //
  // Fill console handles in System Table if no console device assignd.
  //
  if (UpdateSystemTableConsole (L"ConIn", &gEfiSimpleTextInProtocolGuid, &gST->ConsoleInHandle, (VOID **) &gST->ConIn)) {
    SystemTableUpdated = TRUE;
  }

  if (UpdateSystemTableConsole (L"ConOut", &gEfiSimpleTextOutProtocolGuid, &gST->ConsoleOutHandle, (VOID **) &gST->ConOut)) {
    SystemTableUpdated = TRUE;
  }
  

//-  if (UpdateSystemTableConsole (L"ErrOut", &gEfiSimpleTextOutProtocolGuid, &gST->StandardErrorHandle, (VOID **) &gST->StdErr)) {
//-    SystemTableUpdated = TRUE;
//-  }

  if (SystemTableUpdated) {
    //
    // Update the CRC32 in the EFI System Table header
    //
    gST->Hdr.CRC32 = 0;
    gBS->CalculateCrc32 (
          (UINT8 *) &gST->Hdr,
          gST->Hdr.HeaderSize,
          &gST->Hdr.CRC32
          );
  }
  
}
