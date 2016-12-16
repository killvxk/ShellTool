
#include "BdsPlatform.h"
#include "String.h"
#include <Guid/SetupPassword.h>
#include <Protocol/ExitPmAuth.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/AcpiS3Save.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/LegacyBiosPlatform.h>
#include <Protocol/PciRootBridgeIo.h>
#include "Protocol/UpdateFlash.h"
#include <Library/ReportStatusCodeLib.h>
#include <Library/TcgPhysicalPresenceLib.h>
#include "InstallOptionRom.h"
#include <ByoCapsuleExt.h>


VOID
InvokeGetBbsInfo (
  VOID
  );


extern EFI_GUID gBdsAllDriversConnectedProtocolGuid;


extern USB_CLASS_FORMAT_DEVICE_PATH    gUsbClassKeyboardDevicePath;
extern PLATFORM_ISA_SERIAL_DEVICE_PATH gSerialDevicePath;
extern SETUP_DATA                      *gSetupData;



EFI_STATUS PlatformVgaStatus(BOOLEAN *PegVgaPresent, BOOLEAN *IgdPresent)
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_HANDLE                Handle;
	EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT8                     ClassCode[3];


	*IgdPresent    = FALSE;
  *PegVgaPresent = FALSE;	

  DevPath = (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformOnboardVgaDevice;
  Status  = gBS->LocateDevicePath(&gEfiPciIoProtocolGuid, &DevPath, &Handle);
  if(!EFI_ERROR(Status)){
    Status = gBS->HandleProtocol(
                    Handle,
                    &gEfiPciIoProtocolGuid, 
                    &PciIo
                    );
    ASSERT(!EFI_ERROR(Status));
	  Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET, 3, &ClassCode[0]);
    if(ClassCode[2] == PCI_CLASS_DISPLAY && ClassCode[1] == PCI_CLASS_DISPLAY_VGA){
      *IgdPresent = TRUE;
    }			
  }	

  DevPath = (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformPegVgaDevice;
  Status  = gBS->LocateDevicePath(&gEfiPciIoProtocolGuid, &DevPath, &Handle);
  if(!EFI_ERROR(Status)){
    Status = gBS->HandleProtocol(
                    Handle,
                    &gEfiPciIoProtocolGuid, 
                    &PciIo
                    );
    ASSERT(!EFI_ERROR(Status));
	  Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET, 3, &ClassCode[0]);
    if(ClassCode[2] == PCI_CLASS_DISPLAY && ClassCode[1] == PCI_CLASS_DISPLAY_VGA){
      *PegVgaPresent = TRUE;
    }			
  }

  return Status;	
}






/**
  This procedure is used to process flash update under
  reocver mode or flash update mode

  @param  BootMode              Indicate which boot mode system is in

  @return  EFI_NOT_FOUND        Not find flash update proccess procedure
  @return  EFI_SUCCESS          Flash update is proccessed successfully

**/
EFI_STATUS
ProccessFlashUpdate (
  EFI_BOOT_MODE   BootMode
  )
{
  EFI_STATUS               Status;
  UPDATE_FLASH_PROTOCOL    *UpdateFlash;
  EFI_INPUT_KEY            Key;


  Status = gBS->LocateProtocol(&gUpdateFlashProtocolGuid, NULL, &UpdateFlash);
  if (EFI_ERROR(Status)) {
    return EFI_NOT_FOUND;
  }

  Status = UpdateFlash->ProcessFlash(BootMode);

// recovery fail
  if(BootMode == BOOT_IN_RECOVERY_MODE){
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_RED);	
    Print (L"\rSign BIOS verify failed, please reload the new BIOS image\n");
    gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);    
    Print (L"\rPress Any Key to reboot...\n");
    while (gST->ConIn != NULL) {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);    	
      if (!EFI_ERROR (Status)) {      
        gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
      }
      gBS->Stall (1000 * 100);
    }
  }
  
  return Status;
}

/**
  This function converts an input device structure to a Unicode string.

  @param DevPath                  A pointer to the device path structure.

  @return A new allocated Unicode string that represents the device path.

**/
CHAR16 *
DevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  )
{
  EFI_STATUS                       Status;
  CHAR16                           *ToText;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *DevPathToText;

  if (DevPath == NULL) {
    return NULL;
  }

  Status = gBS->LocateProtocol (
                  &gEfiDevicePathToTextProtocolGuid,
                  NULL,
                  (VOID **) &DevPathToText
                  );
  ASSERT_EFI_ERROR (Status);
  ToText = DevPathToText->ConvertDevicePathToText (
                            DevPath,
                            FALSE,
                            TRUE
                            );
  ASSERT (ToText != NULL);
  return ToText;
}

VOID
UpdateEfiGlobalVariable (
  CHAR16           *VariableName,
  EFI_GUID         *AgentGuid,
  PROCESS_VARIABLE ProcessVariable
  )
/*++

Routine Description:

  Generic function to update the console variable.
  Please refer to FastBootSupport.c for how to use it.

Arguments:

  VariableName    - The name of the variable to be updated
  AgentGuid       - The Agent GUID
  ProcessVariable - The function pointer to update the variable
                    NULL means to restore to the original value

--*/
{
  EFI_STATUS  Status;
  CHAR16      BackupVariableName[20];
  CHAR16      FlagVariableName[20];
  VOID        *Variable;
  VOID        *BackupVariable;
  VOID        *NewVariable;
  UINTN       VariableSize;
  UINTN       BackupVariableSize;
  UINTN       NewVariableSize;
  BOOLEAN     Flag;
  BOOLEAN     *FlagVariable;
  UINTN       FlagSize;
  CHAR16      *Str;

  ASSERT (StrLen (VariableName) <= 13);
  UnicodeSPrint (BackupVariableName, sizeof (BackupVariableName), L"%sBackup", VariableName);
  UnicodeSPrint (FlagVariableName, sizeof (FlagVariableName), L"%sModify", VariableName);

  Variable       = EfiBootManagerGetVariableAndSize (VariableName, &gEfiGlobalVariableGuid, &VariableSize);
  BackupVariable = EfiBootManagerGetVariableAndSize (BackupVariableName, AgentGuid, &BackupVariableSize);
  FlagVariable   = EfiBootManagerGetVariableAndSize (FlagVariableName, AgentGuid, &FlagSize);
  if (ProcessVariable != NULL) {
    if (FlagVariable == NULL) {
      //
      // Last boot is normal boot
      // Set flag
      // BackupVariable <- Variable
      // Variable       <- ProcessVariable (Variable)
      //
      Flag   = TRUE;
      Status = gRT->SetVariable (
                      FlagVariableName,
                      AgentGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      sizeof (Flag),
                      &Flag
                      );
      ASSERT_EFI_ERROR (Status);

      Status = gRT->SetVariable (
                      BackupVariableName,
                      AgentGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      VariableSize,
                      Variable
                      );
      ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));


      NewVariable     = Variable;
      NewVariableSize = VariableSize;
      ProcessVariable (&NewVariable, &NewVariableSize);
      DEBUG ((EFI_D_ERROR, "============================%s============================\n", VariableName));
      Str = DevicePathToStr ((EFI_DEVICE_PATH_PROTOCOL *) Variable);    DEBUG ((EFI_D_ERROR, "O:%s\n", Str)); gBS->FreePool (Str);
      Str = DevicePathToStr ((EFI_DEVICE_PATH_PROTOCOL *) NewVariable); DEBUG ((EFI_D_ERROR, "N:%s\n", Str)); gBS->FreePool (Str);
      
      Status = gRT->SetVariable (
                      VariableName,
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      NewVariableSize,
                      NewVariable
                      );
      ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));

      if (NewVariable != NULL) {
        FreePool (NewVariable);
      }
    } else { // LastBootIsModifiedPtr != NULL
      //
      // Last Boot is modified boot
      //
    }
  } else {
    if (FlagVariable != NULL) {
      //
      // Last boot is modified boot
      // Clear LastBootIsModified flag
      // Variable       <- BackupVariable
      // BackupVariable <- NULL
      //
      Status = gRT->SetVariable (
                      FlagVariableName,
                      AgentGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      0,
                      NULL
                      );
      ASSERT_EFI_ERROR (Status);

      Status = gRT->SetVariable (
                      VariableName,
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      BackupVariableSize,
                      BackupVariable
                      );
      ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));

      Status = gRT->SetVariable (
                      BackupVariableName,
                      AgentGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      0,
                      NULL
                      );
      ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));
    } else { // LastBootIsModifiedPtr == NULL
      //
      // Last boot is normal boot
      //
    }
  }

  if (Variable != NULL) {
    FreePool (Variable);
  }

  if (BackupVariable != NULL) {
    FreePool (BackupVariable);
  }

  if (FlagVariable != NULL) {
    FreePool (FlagVariable);
  }

}

VOID
SignalAllDriversConnected (
  VOID
  )
{
  EFI_HANDLE                 Handle;
  EFI_STATUS                 Status;

  //
  // Inform other code that all drivers have been connected.
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gBdsAllDriversConnectedProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
}

VOID
ExitPmAuth (
  VOID
  )
{
  EFI_HANDLE                 Handle;
  EFI_STATUS                 Status;
  EFI_ACPI_S3_SAVE_PROTOCOL  *AcpiS3Save;

  
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));  
  //
  // Prepare S3 information, this MUST be done before ExitPmAuth
  //
  Status = gBS->LocateProtocol (&gEfiAcpiS3SaveProtocolGuid, NULL, (VOID **)&AcpiS3Save);
  if (!EFI_ERROR (Status)) {
    AcpiS3Save->S3Save (AcpiS3Save, NULL);
  }

  //
  // Inform the SMM infrastructure that we're entering BDS and may run 3rd party code hereafter 
  // NOTE: We can NOT put it to PlatformBdsInit, because many boot script touch PCI BAR. :-(
  //       We have to connect PCI root bridge, allocate resource, then ExitPmAuth().
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gExitPmAuthProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  
  //
  // NOTE: We need install DxeSmmReadyToLock directly here because many boot script is added via ExitPmAuth callback.
  // If we install them at same callback, these boot script will be rejected because BootScript Driver runs first to lock them done.
  // So we seperate them to be 2 different events, ExitPmAuth is last chance to let platform add boot script. DxeSmmReadyToLock will
  // make boot script save driver lock down the interface.
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiDxeSmmReadyToLockProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
}


VOID
ConnectRootBridge (
  VOID
  )
{
  UINTN                            RootBridgeHandleCount;
  EFI_HANDLE                       *RootBridgeHandleBuffer = NULL;
  UINTN                            RootBridgeIndex;

  RootBridgeHandleCount = 0;
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiPciRootBridgeIoProtocolGuid,
         NULL,
         &RootBridgeHandleCount,
         &RootBridgeHandleBuffer
         );
  for (RootBridgeIndex = 0; RootBridgeIndex < RootBridgeHandleCount; RootBridgeIndex++) {
    gBS->ConnectController (RootBridgeHandleBuffer[RootBridgeIndex], NULL, NULL, FALSE);
  }

  if(RootBridgeHandleBuffer!=NULL){FreePool(RootBridgeHandleBuffer);}	
}


BOOLEAN
IsGopDevicePath (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  while (!IsDevicePathEndType (DevicePath)) {
    if (DevicePathType (DevicePath) == ACPI_DEVICE_PATH &&
        DevicePathSubType (DevicePath) == ACPI_ADR_DP) {
      return TRUE;
    }
    DevicePath = NextDevicePathNode (DevicePath);
  }
  return FALSE;
}


//
// BDS Platform Functions
//

/**
  Connect the USB short form device path.

  @param DevicePath   USB short form device path

  @retval EFI_SUCCESS           Successfully connected the USB device
  @retval EFI_NOT_FOUND         Cannot connect the USB device
  @retval EFI_INVALID_PARAMETER The device path is invalid.
**/
EFI_STATUS
ConnectUsbShortFormDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  EFI_STATUS                            Status;
  EFI_HANDLE                            *Handles;
  UINTN                                 HandleCount;
  UINTN                                 Index;
  EFI_PCI_IO_PROTOCOL                   *PciIo;
  UINT8                                 Class[3];
  BOOLEAN                               AtLeastOneConnected;

  //
  // Check the passed in parameters
  //
  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DevicePathType (DevicePath) != MESSAGING_DEVICE_PATH) ||
      ((DevicePathSubType (DevicePath) != MSG_USB_CLASS_DP) && (DevicePathSubType (DevicePath) != MSG_USB_WWID_DP))
     ) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find the usb host controller firstly, then connect with the remaining device path
  //
  AtLeastOneConnected = FALSE;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **) &PciIo
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Check whether the Pci device is the wanted usb host controller
      //
      Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x09, 3, &Class);
      if (!EFI_ERROR (Status) &&
          ((PCI_CLASS_SERIAL == Class[2]) && (PCI_CLASS_SERIAL_USB == Class[1]))
         ) {
        Status = gBS->ConnectController (
                        Handles[Index],
                        NULL,
                        DevicePath,
                        FALSE
                        );
        if (!EFI_ERROR(Status)) {
          AtLeastOneConnected = TRUE;
        }
      }
    }
  }

  return AtLeastOneConnected ? EFI_SUCCESS : EFI_NOT_FOUND;
}

CHAR16                       *mConsoleVar[] = {L"ConIn", L"ConOut"};

/**
  Return whether the device is trusted console.
  
  @param Device  The device to be tested.

  @retval TRUE   The device can be trusted.
  @retval FALSE  The device cannot be trusted.
**/
BOOLEAN
IsTrustedConsole (
  EFI_DEVICE_PATH_PROTOCOL  *Device
  )
{
  if (IsGopDevicePath (Device)) {
    return TRUE;
  }

  if (CompareMem (Device, &gKeyboardDevicePath, GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &gKeyboardDevicePath) - END_DEVICE_PATH_LENGTH) == 0) {
    return TRUE;
  }

  if (CompareMem (Device, &gUsbClassKeyboardDevicePath, GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &gUsbClassKeyboardDevicePath) - END_DEVICE_PATH_LENGTH) == 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  The function connects the trusted consoles and then call the PP processing library interface.
**/
VOID
ProcessTcgPp (
  VOID
  )
{
  EFI_DEVICE_PATH_PROTOCOL     *Consoles;
  EFI_DEVICE_PATH_PROTOCOL     *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL     *Instance;
  UINTN                        Size;
  UINTN                        Index;

  for (Index = 0; Index < sizeof (mConsoleVar) / sizeof (mConsoleVar[0]); Index++) {
    Consoles = GetEfiGlobalVariable (mConsoleVar[Index]);

    TempDevicePath = Consoles;
    do {
      Instance = GetNextDevicePathInstance (&TempDevicePath, &Size);
      if (Instance == NULL) {
        break;
      }
      if (IsTrustedConsole (Instance)) {
        if ((DevicePathType (Instance) == MESSAGING_DEVICE_PATH) &&
            ((DevicePathSubType (Instance) == MSG_USB_CLASS_DP) || (DevicePathSubType (Instance) == MSG_USB_WWID_DP))
           ) {
          ConnectUsbShortFormDevicePath (Instance);
        } else {
          EfiBootManagerConnectDevicePath (Instance, NULL);
        }
      }
      FreePool (Instance);
    } while (TempDevicePath != NULL);

    if (Consoles != NULL) {
      FreePool (Consoles);
    }
  }
  
  //
  // Process TCG Physical Presence request just after the trusted console is ready
  //
//why  TcgPhysicalPresenceLibProcessRequest ();
}


BOOLEAN
UpdateSerialConsoleRedirection (
  IN OUT EFI_DEVICE_PATH_PROTOCOL *PlatformConsole,
  IN UART_DEVICE_PATH             *UartDevicePath,
  IN VENDOR_DEVICE_PATH           *VendorDevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINT16                     *pSegOfEbda;
  UINT32                     *mToEbda;
  EFI_GUID                   PcAnsiGuid   = DEVICE_PATH_MESSAGING_PC_ANSI;
  EFI_GUID                   Vt100Guid    = DEVICE_PATH_MESSAGING_VT_100;
  EFI_GUID                   Vt100PlusGuid= EFI_VT_100_PLUS_GUID;
  EFI_GUID                   VtUtf8Guid   = DEVICE_PATH_MESSAGING_VT_UTF8;
  UINT8                      EnableConRed = 1;
  UINT16                     ComPort      = 0;
  UINT32                     UID          = 0;
  UINT64                     Baudrate     = 115200;
  BOOLEAN                    NeedUpdate   = FALSE;
  DevicePath = PlatformConsole;

  pSegOfEbda = (UINT16 *)(UINTN)0x40E;
  mToEbda    = (UINT32*)(((UINTN)(*pSegOfEbda) << 4) + 0x1C2); //flag for CSM support

  while (!IsDevicePathEndType (DevicePath)) {
    DevicePath = NextDevicePathNode (DevicePath);
    if ((DevicePathType (DevicePath) == ACPI_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == ACPI_DP)) {
       //
       // Update GUID according to setting
       //
      if (((ACPI_HID_DEVICE_PATH *)DevicePath)->HID == EISA_PNP_ID(0x0501)){
        switch (gSetupData->SerialPortSelect) {
          case 0:
            EnableConRed = 0x01;
            ComPort      = 0x3F8;
            ((ACPI_HID_DEVICE_PATH *)DevicePath)->UID = UID;
            break;
						
          case 1:
            UID = 1;
            EnableConRed = 0x03;
            ComPort      = 0x2F8;
            ((ACPI_HID_DEVICE_PATH *)DevicePath)->UID = UID;
            break;
		  case 2:
		  	UID = 2;
            EnableConRed = 0x01;
            ComPort      = 0x3E8;
            ((ACPI_HID_DEVICE_PATH *)DevicePath)->UID = UID;
			break;
		  case 3:
		    UID = 3;
            EnableConRed = 0x01;
            ComPort      = 0x2E8;
            ((ACPI_HID_DEVICE_PATH *)DevicePath)->UID = UID;
			break;
          default:
            EnableConRed = 0x01;
            ComPort      = 0x3F8;
            ((ACPI_HID_DEVICE_PATH *)DevicePath)->UID = UID;
            break;
        }

        *((volatile UINT8*)mToEbda) = EnableConRed;
        mToEbda    = (UINT32*)(((UINTN)(*pSegOfEbda) << 4) + 0x1C3);
        *((volatile UINT16*)mToEbda) = ComPort;
      }
      continue;
    }
    //
    // Get the VENDOR defined node
    //
    if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_UART_DP)) {
      //
      // Update GUID according to setting
      //
      switch (gSetupData->SerialBaudrate) {
        case 0: // 9600
          Baudrate = 9600;
          ((UART_DEVICE_PATH *)DevicePath)->BaudRate = Baudrate;
          break;
        case 1: // 19200
          Baudrate = 19200;
          ((UART_DEVICE_PATH *)DevicePath)->BaudRate = Baudrate;
          break;
        case 2: // 38400
          Baudrate = 38400;
          ((UART_DEVICE_PATH *)DevicePath)->BaudRate = Baudrate;
          break;
        case 3: // 57600
          Baudrate = 57600;
          ((UART_DEVICE_PATH *)DevicePath)->BaudRate = Baudrate;
          break;
        case 4: // 115200
          Baudrate = 115200;
          ((UART_DEVICE_PATH *)DevicePath)->BaudRate = Baudrate;
          break;
        default: // 115200
          Baudrate = 115200;
          ((UART_DEVICE_PATH *)DevicePath)->BaudRate = Baudrate;
          break;
      }
      continue;
    }
    if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_VENDOR_DP)) {
      //
      // Update GUID according to setting
      //
      switch(gSetupData->TerminalType) {
        case 0:
          CopyMem (&((VENDOR_DEVICE_PATH *)DevicePath)->Guid, &PcAnsiGuid, sizeof(EFI_GUID));
          break;
        case 1:
          CopyMem (&((VENDOR_DEVICE_PATH *)DevicePath)->Guid, &Vt100Guid, sizeof(EFI_GUID));
          break;
        case 2:
          CopyMem (&((VENDOR_DEVICE_PATH *)DevicePath)->Guid, &Vt100PlusGuid, sizeof(EFI_GUID));
          break;
        case 3:
          CopyMem (&((VENDOR_DEVICE_PATH *)DevicePath)->Guid, &VtUtf8Guid, sizeof(EFI_GUID));
          break;
        default:
          CopyMem (&((VENDOR_DEVICE_PATH *)DevicePath)->Guid, &Vt100Guid, sizeof(EFI_GUID));
          break;
      }
      continue;
    }
  }
  if (UartDevicePath != NULL) {
    if ((Baudrate != UartDevicePath->BaudRate) || CompareMem(&VendorDevicePath->Guid, &((VENDOR_DEVICE_PATH *)DevicePath)->Guid, sizeof(EFI_GUID))) {
      NeedUpdate = TRUE;
    }
  }
  return NeedUpdate;
}

EFI_STATUS
UpdateSerialConsoleVariable () {
  EFI_DEVICE_PATH_PROTOCOL  *VarConsole;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL  *SerialDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *Next;
  UART_DEVICE_PATH          *UartDevicePath;
  VENDOR_DEVICE_PATH        *VendorDevicePath;
  UINTN                     Size;
  BOOLEAN                   Found = FALSE;
  BOOLEAN                   NeedUpdateVariable = FALSE;
  Size             = 0;
  VarConsole       = NULL;
  DevicePath       = NULL;
  UartDevicePath   = NULL;
  VendorDevicePath = NULL;
  DevicePathNode   = NULL;
  SerialDevicePath = (EFI_DEVICE_PATH_PROTOCOL*)&gSerialDevicePath;
  VarConsole = EfiBootManagerGetVariableAndSize (
                L"ConOut",
                &gEfiGlobalVariableGuid,
                &Size
                );
  DevicePath = VarConsole;
  do {
    //
    // The outer loop handles multi instance device paths.
    //
    // After this call DevicePath points to the next Instance
    //
    Instance  = GetNextDevicePathInstance (&DevicePath, &Size);

    Next = Instance;
    while (!IsDevicePathEndType (Next)) {
      //
      // Early break when it's a serial device path
      // Note: Check EISA_PNP_ID (0x501) instead of UART node because we only want to disable the local serial but not the SOL
      //       SOL device path doesn't contain the EISA_PNP_ID (0x501)
      //
      if ((Next->Type == ACPI_DEVICE_PATH) && (Next->SubType == ACPI_DP) &&
          (((ACPI_HID_DEVICE_PATH *) Next)->HID == EISA_PNP_ID (0x501))
          ) {
        Found = TRUE;
        break;
      }
      Next = NextDevicePathNode (Next);
    }
  } while (DevicePath != NULL && Found == FALSE);
  if (Found) {
    DevicePathNode = Instance;
    while (!IsDevicePathEndType (DevicePathNode)) {
      DevicePathNode = NextDevicePathNode (DevicePathNode);
      if ((DevicePathType (DevicePathNode) == MESSAGING_DEVICE_PATH) &&
        (DevicePathSubType (DevicePathNode) == MSG_UART_DP)) {
        UartDevicePath   = (UART_DEVICE_PATH *)DevicePathNode;
        continue;
      }
      if ((DevicePathType (DevicePathNode) == MESSAGING_DEVICE_PATH) &&
        (DevicePathSubType (DevicePathNode) == MSG_VENDOR_DP)) {
        VendorDevicePath = (VENDOR_DEVICE_PATH *)DevicePathNode;
        continue;
      }
    }
    if (!gSetupData->UCREnable) {
      //delete it
      EfiBootManagerUpdateConsoleVariable (ConIn, NULL, Instance);
      EfiBootManagerUpdateConsoleVariable (ConOut, NULL, Instance);
    }
  }
  if (gSetupData->UCREnable) {
    NeedUpdateVariable = UpdateSerialConsoleRedirection( SerialDevicePath,
                                                         UartDevicePath,
                                                         VendorDevicePath
                                                         );
    if (NeedUpdateVariable && Found) {
      //delete it first
      EfiBootManagerUpdateConsoleVariable (ConIn, NULL, Instance);
      EfiBootManagerUpdateConsoleVariable (ConOut, NULL, Instance);
    }
    EfiBootManagerUpdateConsoleVariable (ConIn, SerialDevicePath, NULL);
    EfiBootManagerUpdateConsoleVariable (ConOut, SerialDevicePath, NULL);
  }
  return EFI_SUCCESS;
}



VOID
EFIAPI
PlatformBootManagerBeforeConsole (
  VOID
  )
/*++

Routine Description:

  Platform Bds init. Incude the platform firmware vendor, revision
  and so crc check.

Arguments:

Returns:

  None.

--*/
{
  EFI_STATUS                          Status = EFI_SUCCESS;
  UINTN                               Index;
  EFI_DEVICE_PATH_PROTOCOL            *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL            *TempDevicePath2;
  EFI_LEGACY_BIOS_PLATFORM_PROTOCOL   *LegacyBiosPlatform = NULL;
  EFI_HANDLE                          *HandleBuffer;
  UINTN                               HandleCount;
  EFI_DEVICE_PATH_PROTOCOL            *PciDevicePath = NULL;
  EFI_DEVICE_PATH_PROTOCOL            *ConsoleOut;
  UINTN                               Size;
  EFI_DEVICE_PATH_PROTOCOL            *Instance;
  UINT8                               PriVideo;
  BOOLEAN                             PegVgaPresent;
  BOOLEAN                             IgdPresent;
  EFI_DEVICE_PATH_PROTOCOL            *IgdVgaDp;
  EFI_DEVICE_PATH_PROTOCOL	          *PegVgaDp;
  EFI_LEGACY_BIOS_PROTOCOL            *LegacyBios;
	

  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_BEGIN_CONNECTING_DRIVERS)
    );

  ConnectRootBridge();
  DEBUG((EFI_D_INFO, "RootBrigdeConnected\n"));

  Status = gBS->LocateProtocol(&gEfiLegacyBiosProtocolGuid, NULL, (VOID**)&LegacyBios);	
  if(!EFI_ERROR(Status)){	
    Status = gBS->LocateProtocol(&gEfiLegacyBiosPlatformProtocolGuid, NULL, (VOID **) &LegacyBiosPlatform);
  }
  if (!EFI_ERROR (Status)) {
    Status = LegacyBiosPlatform->GetPlatformHandle (
                                   LegacyBiosPlatform,
                                   EfiGetPlatformVgaHandle,
                                   0,
                                   &HandleBuffer,
                                   &HandleCount,
                                   NULL
                                   );
    if (!EFI_ERROR (Status)) {
      PciDevicePath = DevicePathFromHandle(HandleBuffer[0]);
      DEBUG((EFI_D_INFO, "VGA_DP:%X\n", PciDevicePath));
    }

    // Remove all GOP and insert the current one.
    ConsoleOut = GetEfiGlobalVariable(L"ConOut");

    TempDevicePath  = ConsoleOut;
    TempDevicePath2	= DuplicateDevicePath(ConsoleOut);
    do {
      Instance = GetNextDevicePathInstance(&TempDevicePath, &Size);
      if (Instance == NULL) {
        break;
      }
      if (IsGopDevicePath (Instance)) {
        TempDevicePath2 = EfiBootManagerUpdateConsoleDP(TempDevicePath2, NULL, Instance);
      }
      FreePool (Instance);
    } while (TempDevicePath != NULL);

    Instance = AppendDevicePath(PciDevicePath, (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformOnboardVgaDeviceGop.AcpiDeviceNode);
    TempDevicePath2 = EfiBootManagerUpdateConsoleDP(TempDevicePath2, Instance, NULL);

    gRT->SetVariable (
           L"ConOut",
           &gEfiGlobalVariableGuid,
           EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
           GetDevicePathSize(TempDevicePath2),
           TempDevicePath2
           );
    if(TempDevicePath2!=NULL){FreePool(TempDevicePath2);}
    if(ConsoleOut != NULL){FreePool (ConsoleOut);}
  } else {

    IgdVgaDp = (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformOnboardVgaDeviceGop;
    PegVgaDp = (EFI_DEVICE_PATH_PROTOCOL *)&gPlatformPEGDeviceGop;
				
    PriVideo = gSetupData->VideoPrimaryAdapter;
    PlatformVgaStatus(&PegVgaPresent, &IgdPresent);
    DEBUG((EFI_D_INFO, "PriVideo:%d, PegVgaP:%d, IgdP:%d\n", PriVideo, PegVgaPresent, IgdPresent));
		
    if(PriVideo == DISPLAY_PRIMARY_PCIE && PegVgaPresent){
      EfiBootManagerUpdateConsoleVariable (ConOut, PegVgaDp, IgdVgaDp);
    } else {
      EfiBootManagerUpdateConsoleVariable (ConOut, IgdVgaDp, PegVgaDp);
    }		
  }
	
  InitializeStringSupport();

  for (Index = 0; gPlatformConsole[Index].DevicePath != NULL; Index++) {
    if (gPlatformConsole[Index].ConnectType & CONSOLE_IN) {
      Status = EfiBootManagerUpdateConsoleVariable (ConIn, gPlatformConsole[Index].DevicePath, NULL);
      DEBUG((EFI_D_INFO, "UpdateVar_ConIn:%r\n", Status));
    }
  }
  
  UpdateSerialConsoleVariable();

  gRT->SetVariable (
         EFI_CAPSULE_VARIABLE_NAME,
         &gEfiCapsuleVendorGuid,
         EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
         0,
         NULL
         );
  gRT->SetVariable (
         EFI_CAPSULE_EXT_VARIABLE_NAME,
         &gEfiCapsuleVendorGuid,
         EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
         0,
         NULL
         );


//RegisterDefaultBootOption();
//RegisterStaticHotkey ();
    
//ProcessTcgPp();
//ExitPmAuth();
}

VOID
ConnectSequence (
  VOID
  )
{
  UINTN                    Index;
  EFI_HANDLE               DeviceHandle;
  EFI_STATUS               Status;
  EFI_PCI_IO_PROTOCOL      *PciIo;
  UINT32                   Data32;
  
  
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  
  for (Index = 0; gPlatformConnectSequence[Index] != NULL; Index++) {
    Status = EfiBootManagerConnectDevicePath(gPlatformConnectSequence[Index], &DeviceHandle);
    DEBUG((EFI_D_INFO, "CD[%d]:%r DH:%X\n", Index, Status, DeviceHandle));
    if (!EFI_ERROR (Status)) {
      Status = gBS->HandleProtocol(DeviceHandle, &gEfiPciIoProtocolGuid, &PciIo);
      if(!EFI_ERROR(Status)){
        PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, 0, 1, &Data32);
        DEBUG((EFI_D_INFO, "ID:%08X\n", Data32)); 
      }  
      Status = gBS->ConnectController(DeviceHandle, NULL, NULL, TRUE);
      DEBUG((EFI_D_INFO, "CC:%r\n", Status));      
    } 
  }
  
// Dispatch again since Switchable Graphics driver depends on PCI_IO protocol
  gDS->Dispatch();
}



VOID
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  )
/*++

Routine Description:

  The function will excute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.
  
Arguments:
  DriverOptionList - The header of the driver option link list
  BootOptionList   - The header of the boot option link list
  ProcessCapsules  - A pointer to ProcessCapsules()
  BaseMemoryTest   - A pointer to BaseMemoryTest()
 
Returns:
  None.
  
--*/
{
  EFI_BOOT_MODE                 BootMode;
  BOOLEAN                       BootState;


  BootState = PcdGetBool(PcdBootState);
  if (BootState) {
    PcdSetBool(PcdBootState, FALSE);
  }

  TcgPhysicalPresenceLibProcessRequest();

  BootMode = GetBootModeHob();
  DEBUG((EFI_D_ERROR, "(AfterConsole) BootState:%d, BootMode:%d\n", BootState, BootMode));  
  
  switch (BootMode) {

    case BOOT_ASSUMING_NO_CONFIGURATION_CHANGES:
    case BOOT_WITH_MINIMAL_CONFIGURATION:
    case BOOT_ON_S4_RESUME:
      ConnectSequence();
      InstallAdditionalOpRom();
      SignalAllDriversConnected();
      InvokeGetBbsInfo();		
      break;

    case BOOT_ON_FLASH_UPDATE:
      ProccessFlashUpdate(BOOT_ON_FLASH_UPDATE);
      break;

    case BOOT_IN_RECOVERY_MODE:
      ProccessFlashUpdate(BOOT_IN_RECOVERY_MODE);
      break;

    case BOOT_WITH_FULL_CONFIGURATION:
    case BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS:
    case BOOT_WITH_DEFAULT_SETTINGS:
    default:
      ConnectSequence();
      EfiBootManagerConnectAll();
      InstallAdditionalOpRom();
      SignalAllDriversConnected();
      EfiBootManagerRefreshAllBootOption ();
      break;
  }
	
  LockNonUpdatableFlash();
  ExitPmAuth();

}

VOID
LockNonUpdatableFlash (
  VOID
  )
{
  UINT32      Base;
  UINT32      Size;
  UINT32      LpcMmioBar;
  UINT32      SpiBar;
  UINT32      Value;
  
  LpcMmioBar = (MmioRead32(LPC_PCI_REG(LPC_MMIO_BAR_REG)) >> 4) << 12;
  SpiBar     = MmioRead32(LpcMmioBar) & ~0xFF;
  if ((MmioRead16(SpiBar + SPI0_SPIS_REG) & SPI0_SPIS_LOCKDOWN) != 0) {
    return;
  }

  Base  = (PcdGet32 (PcdFlashFvMainBase) - PcdGet32 (PcdFlashAreaBaseAddress))>> 12;
  Size  = PcdGet32 (PcdFlashFvMainSize) >> 12;
  Value = BIT31 | Base | ((Base + Size - 1) << 12);
  if (Base >= 0 && Size > 0) {
    MmioWrite32(SpiBar + SPI0_PROTECTED_BIOS_RANGE0, Value);
  }

  Base  = (PcdGet32 (PcdFlashFvRecoveryBase) - PcdGet32 (PcdFlashAreaBaseAddress)) >> 12;
  Size  = PcdGet32 (PcdFlashFvRecoverySize) >> 12;
  Value = BIT31 | Base | ((Base + Size - 1) << 12);
  if (Base >= 0 && Size > 0) {
    MmioWrite32(SpiBar + SPI0_PROTECTED_BIOS_RANGE1, Value);
  }
}


