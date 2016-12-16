
#include "BdsPlatform.h"
#include <PlatformDefinition.h>


//
// Platform specific keyboard device path
//
PLATFORM_KEYBOARD_DEVICE_PATH     gKeyboardDevicePath = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0,
    0x11
  },
  {
    ACPI_DEVICE_PATH,
    ACPI_DP,
    (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),
    (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8),
    EISA_PNP_ID(0x0303),
    0
  },
  gEndEntire
};

//
// Platform specific serial device path
//
PLATFORM_ISA_SERIAL_DEVICE_PATH   gSerialDevicePath = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0,
    0x11
  },
  {
    ACPI_DEVICE_PATH,
    ACPI_DP,
    (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),
    (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8),
    EISA_PNP_ID(0x0501),
    0
  },
  {
    MESSAGING_DEVICE_PATH,
    MSG_UART_DP,
    (UINT8) (sizeof (UART_DEVICE_PATH)),
    (UINT8) ((sizeof (UART_DEVICE_PATH)) >> 8),
    0,
    115200,
    8,
    1,
    1
  },
  {
    MESSAGING_DEVICE_PATH,
    MSG_VENDOR_DP,
    (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
    (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8),
    DEVICE_PATH_MESSAGING_PC_ANSI
  },
  gEndEntire
};

USB_CLASS_FORMAT_DEVICE_PATH gUsbClassKeyboardDevicePath = {
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_USB_CLASS_DP,
      (UINT8) (sizeof (USB_CLASS_DEVICE_PATH)),
      (UINT8) ((sizeof (USB_CLASS_DEVICE_PATH)) >> 8)
    },
    0xffff,           // VendorId 
    0xffff,           // ProductId 
    CLASS_HID,        // DeviceClass 
    SUBCLASS_BOOT,    // DeviceSubClass
    PROTOCOL_KEYBOARD // DeviceProtocol
  },

  { 
    END_DEVICE_PATH_TYPE, 
    END_ENTIRE_DEVICE_PATH_SUBTYPE, 
    END_DEVICE_PATH_LENGTH, 
    0
  }
};

//
// Onboard VGA controller device path
//
PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformOnboardVgaDevice = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x1
  },
  gEndEntire
};


PLATFORM_PEG_GOP_DEVICE_PATH  gPlatformPegVgaDevice = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x2
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },
  gEndEntire  
};	



PLATFORM_GOP_DEVICE_PATH         gPlatformOnboardVgaDeviceGop = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x1
  },
  {
    ACPI_DEVICE_PATH,
    ACPI_ADR_DP,
    (UINT8) (sizeof (ACPI_ADR_DEVICE_PATH)),
    (UINT8) ((sizeof (ACPI_ADR_DEVICE_PATH)) >> 8),
    ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, 0, 0)
  },
  gEndEntire
};

PLATFORM_PEG_GOP_DEVICE_PATH         gPlatformPEGDeviceGop = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x2
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },
  {
    ACPI_DEVICE_PATH,
    ACPI_ADR_DP,
    (UINT8) (sizeof (ACPI_ADR_DEVICE_PATH)),
    (UINT8) ((sizeof (ACPI_ADR_DEVICE_PATH)) >> 8),
    ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, 0, 0)
  },
  gEndEntire
};


//
// Predefined platform default console device path
//
BDS_CONSOLE_CONNECT_ENTRY         gPlatformConsole[] = {
  //
  // Match PCCG current bios, remove console from ConIn&ConOut
  //
  // {
  //   (EFI_DEVICE_PATH_PROTOCOL *) &gSerialDevicePath,
  //   (CONSOLE_OUT | CONSOLE_IN)
  // },
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformOnboardVgaDeviceGop,
    CONSOLE_OUT
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gKeyboardDevicePath,
    CONSOLE_IN
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gUsbClassKeyboardDevicePath, 
    CONSOLE_IN
  },
  {
    NULL,
    0
  }
};

//
// Onboard NIC
//
PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformOnboardNicDevice = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    ONBOARD_LAN_BRIDGE_DEV_NUM
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};


PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformPcie2Device = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    2
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};

PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformPcie3Device = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    3
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};

PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformPcie5Device = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    5
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};

PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformPcie6Device = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    6
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};

PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH2  gPlatformPcie7Device = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    7
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x0
  },  
  gEndEntire
};




//
// Onboard SATA controller device path
//
PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformOnboardSataAhciDevice = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0xF
  },
  gEndEntire
};

PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformOnboardSata1AhciDevice = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x1,
    0xF
  },
  gEndEntire
};

PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformOnboardSata2AhciDevice = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x2,
    0xF
  },
  gEndEntire
};

PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformOnboardSata3AhciDevice = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x0,
    0x15
  },
  gEndEntire
};



//
// Onboard SDIO controller device path
//
PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH  gPlatformOnboardSdioDevice = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0x00,
    0x0C
  },
  gEndEntire
};



//
// Predefined platform connect sequence
//
EFI_DEVICE_PATH_PROTOCOL  *gPlatformConnectSequence[] = {
// Please Keep SATA at the first position of this table!
  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformOnboardSataAhciDevice,
  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformOnboardSata1AhciDevice,
  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformOnboardSata2AhciDevice,
  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformOnboardSata3AhciDevice,  
  
  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformOnboardSdioDevice,
  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformOnboardNicDevice,

  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformPcie2Device,
  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformPcie3Device,
  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformPcie5Device,
  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformPcie6Device,
  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformPcie7Device,
  NULL
};




