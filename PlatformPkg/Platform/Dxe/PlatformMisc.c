
#include "PlatformDxe.h"
#include <Library/PerformanceLib.h>
#include <Protocol/UsbPolicy.h>
#include <Protocol/Ps2Policy.h>
#include <Protocol/PciRootBridgeIo.h>
#include <AcpiRam.h>



EFI_STATUS
EFIAPI
Ps2InitHardware (
  IN  EFI_HANDLE  Handle
  );

//
// PS/2 Keyboard Controller registers
//
#define KEYBOARD_8042_DATA_REGISTER                  0x60
#define KEYBOARD_8042_STATUS_REGISTER                0x64
#define KEYBOARD_8042_COMMAND_REGISTER               0x64

#define KEYBOARD_STATUS_REGISTER_HAS_OUTPUT_DATA     BIT0   // 0 - Output register has no data; 
                                                            // 1 - Output register has data
#define KEYBOARD_STATUS_REGISTER_HAS_INPUT_DATA      BIT1   // 0 - Input register has no data;  
                                                            // 1 - Input register has data
#define KEYBOARD_STATUS_REGISTER_SYSTEM_FLAG         BIT2   // Set to 0 after power on reset
#define KEYBOARD_STATUS_REGISTER_INPUT_DATA_TYPE     BIT3   // 0 - Data in input register is data; 
                                                            // 1 - Data in input register is command
#define KEYBOARD_STATUS_REGISTER_ENABLE_FLAG         BIT4   // 0 - Keyboard is disable; 
                                                            // 1 - Keyboard is enable
#define KEYBOARD_STATUS_REGISTER_TRANSMIT_TIMEOUT    BIT5   // 0 - Transmit is complete without timeout; 
                                                            // 1 - Transmit is timeout without complete
#define KEYBOARD_STATUS_REGISTER_RECEIVE_TIMEOUT     BIT6   // 0 - Receive is complete without timeout; 
                                                            // 1 - Receive is timeout without complete
#define KEYBOARD_STATUS_REGISTER_PARITY              BIT7   // 0 - Odd parity; 
                                                            // 1 - Even parity

#define KEYBOARD_8048_COMMAND_CLEAR_OUTPUT_DATA      0xF4
#define KEYBOARD_8048_COMMAND_SET_DEFAULT            0xF6
#define KEYBOARD_8048_COMMAND_RESET                  0xFF
#define KEYBOARD_8048_COMMAND_SELECT_SCAN_CODE_SET   0xF0

#define KEYBOARD_MAX_TRY                             256     // 256
#define KEYBOARD_TIMEOUT                             65536   // 0.07s
#define KEYBOARD_WAITFORVALUE_TIMEOUT                1000000 // 1s

#define KEYBOARD_8048_RETURN_8042_ACK                0xFA





STATIC EFI_USB_POLICY_PROTOCOL gUsbPolicy;

EFI_PS2_POLICY_PROTOCOL mPs2PolicyData = {
  EFI_KEYBOARD_NUMLOCK,
  (EFI_PS2_INIT_HARDWARE) Ps2InitHardware
};

EFI_STATUS MiscConfigDxe (VOID)
{
  EFI_STATUS  Status;
  
  ZeroMem(&gUsbPolicy, sizeof(gUsbPolicy));
  gUsbPolicy.Version                     = (UINT8)USB_POLICY_PROTOCOL_REVISION_2;
  gUsbPolicy.UsbMassStorageEmulationType = ATUO_TYPE;     // It seems spell error, but ...
  gUsbPolicy.UsbOperationMode            = HIGH_SPEED;
  gUsbPolicy.LegacyKBEnable              = LEGACY_KB_EN;
  gUsbPolicy.LegacyMSEnable              = LEGACY_MS_EN;
  gUsbPolicy.USBPeriodSupport            = LEGACY_PERIOD_UN_SUPP;
  gUsbPolicy.LegacyFreeSupport           = LEGACY_FREE_UN_SUPP;
  gUsbPolicy.LegacyUsbEnable             = gSetupData->LegacyUsbEnable?LEGACY_USB_EN:LEGACY_USB_DIS;
  gUsbPolicy.XhcSupport                  = LEGACY_XHC_SUPP;
  gUsbPolicy.UsbTimeTue                  = LEGACY_USB_TIME_TUE_ENABLE;
  gUsbPolicy.InternelHubExist            = USB_NOT_HAVE_HUB_INTERNEL;
  gUsbPolicy.EnumWaitPortStableStall     = 100;
  gUsbPolicy.CodeBase                    = ICBD_CODE_BASE;
  
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gImageHandle,
                  &gUsbPolicyGuid, &gUsbPolicy,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);  

  //
  // Initialize the keyboard settings
  //
  mPs2PolicyData.KeyboardLight = 0;
  if (gSetupData->Numlock)
    mPs2PolicyData.KeyboardLight |= EFI_KEYBOARD_NUMLOCK;
  Status = gBS->InstallProtocolInterface (
                  &gImageHandle,
                  &gEfiPs2PolicyProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPs2PolicyData
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;  
}



VOID
UpdatePs2State (
  VOID
  )
{
  BOOLEAN                       Rc;
  EFI_ACPI_RAM_DATA             *AcpiRam;
  

	AcpiRam = (EFI_ACPI_RAM_DATA*)GetAcpiRam();

  if (CheckKbcPresent()) {
    PERF_START (NULL,"CMSP", "MISC", 0);
    Rc = CheckMsPresent();
    PERF_END   (NULL,"CMSP", "MISC", 0);
    if (Rc) {
      AcpiRam->IoStates |= IO_STATE_PS2_MS_PRESENT;
    }
    PERF_START (NULL,"CKBP", "MISC", 0);    
    Rc = CheckKbPresent();
    PERF_END   (NULL,"CKBP", "MISC", 0);    
    if (Rc) {
      AcpiRam->IoStates |= IO_STATE_PS2_KB_PRESENT;
    }		
  }

}


VOID*
AllocateAcpiNvsZeroMemoryBelow4G (
  IN UINTN  Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer;

  Pages = EFI_SIZE_TO_PAGES(Size);
  Address = 0xffffffff;

  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiACPIMemoryNVS,
                   Pages,
                   &Address
                   );
  ASSERT_EFI_ERROR(Status);

  Buffer = (VOID*)(UINTN)Address;
  ZeroMem(Buffer, Size);

  return Buffer;
}

EFI_STATUS
Ps2InitHardware (
  IN  EFI_HANDLE  Handle
  )
/*++

Routine Description:

  Perform additional PS/2 hardware init. 
  - Send a 0xF6 command to set the kbd in the default state. If the PS/2 keyboard driver takes a short-cut that skips the kbd 
    reset command 0xff and the kbd enabling command 0xf4, then the command 0xf6 becomes mandatory to make KBD to work with Emerald Lake 
    CRB KSC FW. To avoid changes in the generic PS/2 keyboard driver, we send 0xF6 command in this function as a platform specific hook.
  - PS/2 Swap feature is not supported.

Arguments:
  
  Handle  - Handle for the protocol instance.
  
Returns:

  EFI_SUCCESS

--*/
{
  EFI_STATUS                      Status;
  UINT8                           Buf8;
  UINT64                          Buf64;
  UINT32                          TimeOut;
  UINT32                          SumTimeOut;
  STATIC  BOOLEAN                 Initialized;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  //
  // Only do once
  //
  if (!Initialized) {
    Initialized = TRUE;
  } else {
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (&gEfiPciRootBridgeIoProtocolGuid, NULL, &PciRootBridgeIo);
  ASSERT_EFI_ERROR (Status);

  //
  // Based on assumption that output buffer clearing is done by Keyboard driver before calling this function,
  // we don't do output buffer clearing here to save boot time.
  // If the assumption does not hold true, we must clear output buffer here.
  //

  //
  // Wait for input buffer empty
  //
  Status = PciRootBridgeIo->PollIo (
                               PciRootBridgeIo, 
                               EfiPciWidthUint8,
                               KEYBOARD_8042_STATUS_REGISTER,
                               KEYBOARD_STATUS_REGISTER_HAS_INPUT_DATA,
                               0,
                               KEYBOARD_TIMEOUT * 10,
                               &Buf64
                               );
  if (EFI_SUCCESS != Status) {
    goto Done;
  }

  //
  // Write the 0xF6 command
  //
  Buf8 = KEYBOARD_8048_COMMAND_SET_DEFAULT;
  Status = PciRootBridgeIo->Io.Write (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint8,
                                  KEYBOARD_8042_DATA_REGISTER,
                                  1,
                                  &Buf8
                                  );
  //
  // Wait for ACK
  //
  TimeOut = 0;
  SumTimeOut = 0;
  while (1) {
    //
    // Read from data port
    //
    for (TimeOut = 0; TimeOut < KEYBOARD_TIMEOUT; TimeOut += 30) {
      Status = PciRootBridgeIo->Io.Read(
                                      PciRootBridgeIo,
                                      EfiPciWidthUint8,
                                      KEYBOARD_8042_STATUS_REGISTER,
                                      1,
                                      &Buf8
                                      );
      ASSERT_EFI_ERROR (Status);
      if (Buf8 & KEYBOARD_STATUS_REGISTER_HAS_OUTPUT_DATA) {
        Status = PciRootBridgeIo->Io.Read(
                                       PciRootBridgeIo,
                                       EfiPciWidthUint8,
                                       KEYBOARD_8042_DATA_REGISTER,
                                       1,
                                       &Buf8
                                       );
        ASSERT_EFI_ERROR (Status);
        break;
      }
      gBS->Stall (30);
    }

    //
    // Is it an ACK?
    //
    if (TimeOut < KEYBOARD_TIMEOUT && Buf8 == KEYBOARD_8048_RETURN_8042_ACK) {
      Status = EFI_SUCCESS;
      break;
    }

    SumTimeOut += TimeOut;
    if (SumTimeOut >= KEYBOARD_TIMEOUT * 2) {
      Status = EFI_DEVICE_ERROR;
      break;
    }
  }

Done:
  return  Status;
}

