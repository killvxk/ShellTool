/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
  UsbVirtualKbc.c

Abstract:


Revision History:

**/

#include "UsbVirtualKbc.h"

USB_VIRTUAL_KBC_DEV             *mVirtualKBCDev = NULL;
EFI_USB_VIRTUAL_KBC_PROTOCOL    *mVirtualKbc = NULL;
extern EFI_SMM_SYSTEM_TABLE     *mFrameworkSmst;
extern EFI_SMM_CPU_PROTOCOL     *mSmmCpu;
extern LIST_ENTRY               mUsbBusList;
UINT8                           KeepTrap6064hEnable = 0;
extern BOOLEAN                  mLegacyFreeSupport;
extern  BOOLEAN                 mIfLegacyUsbStarted;

BOOLEAN
KbcIBFEmpty (VOID)
{
    UINT8   KbdCmd;
    UINT16  TimeOut;

    for (TimeOut = 0; TimeOut < KBC_INBUFFER_COUNTER; TimeOut++) {
        KbdCmd = SmmIoRead8 (KBC_CMD_PORT);
        if ((KbdCmd & KBC_STS_IBF) == 0) {
            return TRUE;
        }
        Stall(50);
    }
    return FALSE;
}

BOOLEAN
KbcOBFull (VOID)
{
    UINT8   KbdCmd;
    UINT16  TimeOut;

    for (TimeOut = 0; TimeOut < KBC_INBUFFER_COUNTER; TimeOut++) {
        KbdCmd = SmmIoRead8 (KBC_CMD_PORT);
        if ((KbdCmd & KBC_STS_OBF) == KBC_STS_OBF) {
            return TRUE;
        }
        Stall(50);
    }
    return FALSE;
}

UINT8
SmmKbcIoRead8(IN UINT64 Address)
{
  UINT8   Data;

  Data = 0;
  if (mVirtualKBCDev != NULL) {
    if (mVirtualKBCDev->IsKbcPresent) {
      Data = SmmIoRead8 (Address);
    } else {
      if (Address == KBC_CMD_PORT) {
        UsbDevSendData (KBC_PORT64_READ, &Data);
        KBC_DEBUG((EFI_D_ERROR, "read64 64 = %x\n",mVirtualKBCDev->VirtualKbcData.VirtualKBC64Port));
      }
      if (Address == KBC_DATA_PORT) {
        UsbDevSendData (KBC_PORT60_READ, &Data);
        KBC_DEBUG((EFI_D_ERROR, "read60 64 = %x\n",mVirtualKBCDev->VirtualKbcData.VirtualKBC64Port));
      }
    }
  } else {
    Data = SmmIoRead8 (Address);
  }
  return Data;
}

UINT8
SmmKbcIoWrite8(
    IN UINT64 Address,
    IN UINT8  Data
)
{
  UINT8   InData;

  InData = Data;
  if (mVirtualKBCDev != NULL) {
    if (mVirtualKBCDev->IsKbcPresent) {
      KbcIBFEmpty ();
      SmmIoWrite8 (Address, Data);
    } else {
      if (Address == KBC_CMD_PORT) {
        UsbDevSendData (KBC_PORT64_WRITE, &InData);
        KBC_DEBUG((EFI_D_ERROR, "write64 64 = %x\n",mVirtualKBCDev->VirtualKbcData.VirtualKBC64Port));
      }
      if (Address == KBC_DATA_PORT) {
        UsbDevSendData (KBC_PORT60_WRITE, &InData);
        KBC_DEBUG((EFI_D_ERROR, "write60 64 = %x\n",mVirtualKBCDev->VirtualKbcData.VirtualKBC64Port));
      }
    }
  } else {
    KbcIBFEmpty ();
    SmmIoWrite8 (Address, Data);
  }

  return InData;
}

BOOLEAN
UsbKbcSendDataForUhci (
    IN KBC_DATA_TYPE  Type,
    IN UINT8          Data
)
{

    UINT8   KbcSts;

    KbcSts = SmmIoRead8 (KBC_CMD_PORT);

    if ((KbcSts & KBC_STS_OBF) == KBC_STS_OBF)
        goto ON_EXIT;

    if (Type == KbcKB) {
        KbcIBFEmpty ();
        SmmIoWrite8 (KBC_CMD_PORT, KBC_WRITKBB_BYTE);
    }

    if (Type == KbcMS) {
        KbcIBFEmpty ();
        SmmIoWrite8 (KBC_CMD_PORT, KBC_WRITMSB_BYTE);
    }

    if (KbcIBFEmpty ()) {
        SmmIoWrite8 (KBC_DATA_PORT, Data);
        return  TRUE;
    }
ON_EXIT:
    return  FALSE;

}
BOOLEAN
UsbKbcSendDataForOhci (
    IN KBC_DATA_TYPE Type,
    IN UINT8 Data
)
{
    UINT8 KbcSts;
    KBC_DATA_TYPE KbcType;

    KbcType = Kbc;
    mVirtualKBCDev->VirtualKBCApi->VirtualStatus (&KbcSts, FALSE);
    if ((KbcSts & KBC_STS_OBF) == KBC_STS_OBF)
        goto ON_EXIT;

    mVirtualKBCDev->VirtualKBCApi->VirtualSetData (Data);
    KbcSts |= KBC_STS_OBF;

    if (Type == KbcKB) {
        if ((mVirtualKBCDev->VirtualKbcData.KBCCmdReg & KBC_CMD_INT) == KBC_CMD_INT)
            KbcType = KbcKB;
    }

    if (Type == KbcMS) {
        KbcSts |= KBC_STS_MOBF;
        if ((mVirtualKBCDev->VirtualKbcData.KBCCmdReg & KBC_CMD_INT2) == KBC_CMD_INT2)
            KbcType = KbcMS;
    }

    mVirtualKBCDev->VirtualKBCApi->VirtualIRQ (KbcType);
    mVirtualKBCDev->VirtualKBCApi->VirtualStatus (&KbcSts, TRUE);

    return  TRUE;

ON_EXIT:
    return  FALSE;
}

VOID
UsbDevSendData (
    IN UINT8 Type,
    IN UINT8 *Data
)
{

    USB_VIRTUAL_KBC_DATA  *Ptr;
    KBC_DATA_TYPE   KbcType;
    UINT8           KbcSts;
    BOOT_CFG        *BootCfg;

    Ptr = &mVirtualKBCDev->VirtualKbcData;
    KbcType = Kbc;
    KbcSts = Ptr->VirtualKBC64Port;

    switch (Type) {

    case KBC_PORT64_WRITE:
        Ptr->KBCSendCmd = *Data;
        break;

    case KBC_PORT64_READ:
        if (mVirtualKBCDev->BusType == USB_OHCI) {
            mVirtualKBCDev->VirtualKBCApi->VirtualStatus (Data, FALSE);
        } else {
            *Data = Ptr->VirtualKBC64Port;
        }
        *Data |= Ptr->KBCSendStatus;
        break;

    case KBC_PORT60_WRITE:
        Ptr->VirtualKBC60Port = *Data;
        if (mVirtualKBCDev->BusType == USB_OHCI) {
            mVirtualKBCDev->VirtualKBCApi->VirtualSetData (*Data);
            mVirtualKBCDev->VirtualKBCApi->VirtualStatus (&KbcSts, FALSE);
        }

        if ((KbcSts & KBC_STS_OBF) != KBC_STS_OBF) {
            KbcSts &= ~(KBC_STS_OBF | KBC_STS_MOBF);
            Ptr->VirtualKBC64Port = KbcSts;

            if (Ptr->KBCSendCmd == KBC_WRITKBB_BYTE) {
                Ptr->VirtualKBC64Port |= KBC_STS_OBF;
                KBC_DEBUG((EFI_D_ERROR, "0xD2 64 = %x\n",mVirtualKBCDev->VirtualKbcData.VirtualKBC64Port));
                KbcType = KbcKB;
            }
            if (Ptr->KBCSendCmd == KBC_WRITMSB_BYTE) {
              GET_BOOT_CFG(BootCfg);
              if (BootCfg->BootStart) {
                Ptr->VirtualKBC64Port |= KBC_STS_OBF | KBC_STS_MOBF;
                KBC_DEBUG((EFI_D_ERROR, "0xD3 64 = %x\n",mVirtualKBCDev->VirtualKbcData.VirtualKBC64Port));
                KbcType = KbcMS;
              }
            }
            mVirtualKBCDev->VirtualKBCApi->VirtualStatus (&Ptr->VirtualKBC64Port, TRUE);
            mVirtualKBCDev->VirtualKBCApi->VirtualIRQ (KbcType);
        }
        break;

    case KBC_PORT60_READ:
        *Data = Ptr->VirtualKBC60Port;
        break;

    default:
        break;
    }

}

VOID
UsbKbcStoreData (
    IN USB_VIRTUAL_KBC_DATA *Ptr,
    IN KBC_DATA_TYPE  Type,
    IN UINT8          Data
)
{
    switch (Type) {
    case Kbc:
        if (!(mVirtualKBCDev->IsKbcPresent)) {
            Ptr->KBCdata [Ptr->KBCdataTail] = Data;
            ++Ptr->KBCdataTail;
            UsbVirtualKbcTimer (TRUE);
        }
        break;

    case KbcKB:
        if (!(mVirtualKBCDev->IsKBPresent)) {
            Ptr->KBdata [Ptr->KBdataTail] = Data;
            ++Ptr->KBdataTail;
            UsbVirtualKbcTimer (TRUE);
        }
        break;

    case KbcMS:
        if (!(mVirtualKBCDev->IsMSPresent)) {
            Ptr->MSdata [Ptr->MSdataTail] = Data;
            ++Ptr->MSdataTail;
            KBC_DEBUG((EFI_D_ERROR, "MSdataTail = %x\n",Ptr->MSdataTail));
            UsbVirtualKbcTimer (TRUE);
        }
        break;

    default:
        break;
    }

}

VOID
UsbVirtualKbcStoreData (
    IN USB_VIRTUAL_KBC_DATA *Ptr,
    IN KBC_DATA_TYPE  Type,
    IN UINT8          Data
)
{
    switch (Type) {
    case Kbc:
        Ptr->KBCdata [Ptr->KBCdataTail] = Data;
        ++Ptr->KBCdataTail;
        UsbVirtualKbcTimer (TRUE);
        break;

    case KbcKB:
        Ptr->KBdata[Ptr->KBdataTail] = Data;
        ++Ptr->KBdataTail;
        UsbVirtualKbcTimer (TRUE);
        break;

    case KbcMS:
        Ptr->MSdata[Ptr->MSdataTail] = Data;
        ++Ptr->MSdataTail;
        UsbVirtualKbcTimer (TRUE);
        break;

    default:
        break;
    }

}

EFI_STATUS
UsbKbcTimerCallBack (
    IN VOID   *Data,
    IN UINTN  DataLength,
    IN VOID   *Context,
    IN UINT32 Status
)
{
    USB_VIRTUAL_KBC_DATA  *Ptr;
    BOOLEAN               TimerFlag;
    BOOLEAN               SendCodeFlag;

    SendCodeFlag = FALSE;
    TimerFlag    = FALSE;
    Ptr          = &mVirtualKBCDev->VirtualKbcData;
    KBC_DEBUG((EFI_D_ERROR, "KBCH=%x,KBCT=%x,", Ptr->KBCdataHead,Ptr->KBCdataTail));
    KBC_DEBUG((EFI_D_ERROR, "KBH=%x,KBT=%x,", Ptr->KBdataHead,Ptr->KBdataTail));
    KBC_DEBUG((EFI_D_ERROR, "MSH=%x,MST=%x\n", Ptr->MSdataHead,Ptr->MSdataTail));
    UsbVirtualKbcTimer (FALSE);

    if (Ptr->KBCdataHead != Ptr->KBCdataTail) {
        if (!(mVirtualKBCDev->IsKbcPresent)) {
            Ptr->VirtualKBC60Port  = Ptr->KBCdata [Ptr->KBCdataHead];
            Ptr->VirtualKBC64Port |= KBC_STS_OBF;
            KBC_DEBUG((EFI_D_ERROR, "KBC data 64 = %x\n",mVirtualKBCDev->VirtualKbcData.VirtualKBC64Port));
            SendCodeFlag      = TRUE;
        }

        if (mVirtualKBCDev->IsKbcPresent) {
            if (mVirtualKBCDev->BusType == USB_UHCI || mVirtualKBCDev->BusType == USB_EHCI)
                SendCodeFlag = UsbKbcSendDataForUhci (Kbc, Ptr->KBCdata [Ptr->KBCdataHead]);
        }

        if (mVirtualKBCDev->BusType == USB_OHCI)
            SendCodeFlag = UsbKbcSendDataForOhci (Kbc, Ptr->KBCdata [Ptr->KBCdataHead]);

        if (SendCodeFlag) {
            ++Ptr->KBCdataHead;
        }
        goto ON_EXIT;
    }

    if (Ptr->KBdataHead != Ptr->KBdataTail) {
        if (!(mVirtualKBCDev->IsKbcPresent)) {
            Ptr->VirtualKBC60Port  = Ptr->KBdata [Ptr->KBdataHead];
            Ptr->VirtualKBC64Port |= KBC_STS_OBF;
            KBC_DEBUG((EFI_D_ERROR, "KB data 64 = %x\n",mVirtualKBCDev->VirtualKbcData.VirtualKBC64Port));
            SendCodeFlag      = TRUE;
            mVirtualKBCDev->VirtualKBCApi->VirtualIRQ (KbcKB);
        }

        if (!(mVirtualKBCDev->IsKBPresent) && mVirtualKBCDev->IsKbcPresent) {
            if (mVirtualKBCDev->BusType == USB_UHCI || mVirtualKBCDev->BusType == USB_EHCI) {
                SendCodeFlag = UsbKbcSendDataForUhci (KbcKB, Ptr->KBdata [Ptr->KBdataHead]);
            }
        }

        if (mVirtualKBCDev->BusType == USB_OHCI) {
            SendCodeFlag = UsbKbcSendDataForOhci (KbcKB, Ptr->KBdata[Ptr->KBdataHead]);
        }

        if (SendCodeFlag) {
            ++Ptr->KBdataHead;
        }
        goto ON_EXIT;
    }

    if (Ptr->MSdataHead != Ptr->MSdataTail) {
        if (!(mVirtualKBCDev->IsKbcPresent)) {
            if (!((Ptr->VirtualKBC64Port & KBC_STS_OBF) == KBC_STS_OBF)) {
                Ptr->VirtualKBC60Port  = Ptr->MSdata [Ptr->MSdataHead];
                Ptr->VirtualKBC64Port |= KBC_STS_OBF | KBC_STS_MOBF;
                KBC_DEBUG((EFI_D_ERROR, "MS data 64 = %x\n",mVirtualKBCDev->VirtualKbcData.VirtualKBC64Port));
                SendCodeFlag      = TRUE;
                mVirtualKBCDev->VirtualKBCApi->VirtualIRQ (KbcMS);
            }
        }
        if (!(mVirtualKBCDev->IsMSPresent) && mVirtualKBCDev->IsKbcPresent) {
            if (mVirtualKBCDev->BusType == USB_UHCI || mVirtualKBCDev->BusType == USB_EHCI) {
                SendCodeFlag = UsbKbcSendDataForUhci (KbcMS, Ptr->MSdata [Ptr->MSdataHead]);
            }
        }

        if (mVirtualKBCDev->BusType == USB_OHCI) {
            SendCodeFlag = UsbKbcSendDataForOhci (KbcMS, Ptr->MSdata [Ptr->MSdataHead]);
        }

        if (SendCodeFlag) {
            ++Ptr->MSdataHead;
        }
        goto ON_EXIT;
    }

    TimerFlag           = TRUE;
    Ptr->KBCSendStatus &= ~KBC_STS_OBF;
    Ptr->VirtualKBC64Port   &= ~KBC_STS_OBF;
    KBC_DEBUG((EFI_D_ERROR, "CallBack 64 = %x\n",mVirtualKBCDev->VirtualKbcData.VirtualKBC64Port));
ON_EXIT:
    if (!(TimerFlag == TRUE)) {
        UsbVirtualKbcTimer (TRUE);
    }
    return EFI_SUCCESS;
}

VOID
UsbSetLED (VOID)
{
    LIST_ENTRY      *Link;
    USB_BUS         *BusInstance;
    USB_DEVICE      *Device;
    USB_INTERFACE   *UsbIf;
    USB_KB_DEV      *UsbKeyboardDevice;
    UINTN           DeviceIndex;
    UINTN           InterfaceIndex;
    UINTN           LunIndex;

    Link  = mUsbBusList.ForwardLink;

    while (Link != &mUsbBusList) {
        //
        // Traverse current USB bus instance found in system.
        //
        BusInstance  = USB_BUS_FROM_LINK (Link);
        //
        // Check devices found under this bus instance.
        //
        for (DeviceIndex = 1; DeviceIndex < USB_MAX_DEVICES; DeviceIndex++) {
            Device = BusInstance->Devices [DeviceIndex];
            if (Device != NULL) {
                //
                // Check each Interface that device supported.
                //
                for (InterfaceIndex = 0; InterfaceIndex < Device->NumOfInterface; InterfaceIndex++) {
                    UsbIf = Device->Interfaces [InterfaceIndex];
                    //
                    // LED update for each keyboard interface found.
                    //
                    if ((UsbIf != NULL) && (UsbIf->Type == UDT_KEYBOARD)) {
                        for (LunIndex = 0; LunIndex < UsbIf->LunCount ; LunIndex++) {
                            UsbKeyboardDevice = UsbIf->DriverContext [LunIndex];
                            SetKeyLED (UsbKeyboardDevice);
                        }
                    }
                }
            }
        }
        //
        // Advance to next USB bus instance.
        //
        Link = Link->ForwardLink;
    }
}

VOID
UsbVirtualKbcTimer (IN BOOLEAN IsNewTransfer)
{
    EFI_USB2_HC_PROTOCOL  *Usb2Hc;
    USB_VIRTUAL_KBC_DATA        *VirtualKbcData;

    Usb2Hc    = mVirtualKBCDev->Usb2Hc;
    VirtualKbcData = &mVirtualKBCDev->VirtualKbcData;
    if (mVirtualKBCDev->IsTimerPresent) {
        if (!IsNewTransfer) {
            mVirtualKBCDev->IsTimerPresent = IsNewTransfer;
            Usb2Hc->AsyncInterruptOnlyTransfer(
                Usb2Hc,
                IsNewTransfer,
                USB_VIRTUAL_KBC_INTERVAL,
                UsbKbcTimerCallBack,
                VirtualKbcData);
        }
    }

    if (!(mVirtualKBCDev->IsTimerPresent)) {
        if (IsNewTransfer) {
            mVirtualKBCDev->IsTimerPresent = IsNewTransfer;
            Usb2Hc->AsyncInterruptOnlyTransfer(
                Usb2Hc,
                IsNewTransfer,
                USB_VIRTUAL_KBC_INTERVAL,
                UsbKbcTimerCallBack,
                VirtualKbcData);
        }
    }

}

VOID
UsbKbcToKeyboardCommand (
    IN USB_VIRTUAL_KBC_DATA *Ptr,
    IN UINT8          Data
)
{
    UINT8   AckData;
    UINT8   DataType;
    UINT8   *BdaPtr;

    BdaPtr = (UINT8 *)(UINTN)BDA_LED;

    if (Ptr->KbArgNumber == 0) {
        DataType = Data;
        Ptr->KbLastCmd = Data;
    } else {
        DataType = Ptr->KbLastCmd;
    }

    switch (DataType) {
    case KB_SET_LED:
        if (Ptr->KbArgNumber == 0) {
            Ptr->KBdataHead = Ptr->KBdataTail;
            AckData = KBC_ACK_STATUS;
            UsbKbcStoreData (Ptr, KbcKB, AckData);
            ++Ptr->KbArgNumber;
        } else {
            Ptr->KbArgNumber = 0;
            Ptr->KBdataHead = Ptr->KBdataTail;
            AckData = KBC_ACK_STATUS;
            UsbKbcStoreData (Ptr, KbcKB, AckData);

            if (!(mVirtualKBCDev->IsKbcPresent)) {
                *BdaPtr |= ((Data << 4) & 0x70);
            }
            UsbSetLED(); // Update USB LED status now.
        }
        break;

    case KB_SET_TYPEMATIC:
        if (Ptr->KbArgNumber == 0) {
            Ptr->KBdataHead = Ptr->KBdataTail;
            AckData = KBC_ACK_STATUS;
            UsbKbcStoreData (Ptr, KbcKB, AckData);
            ++Ptr->KbArgNumber;
        } else {
            Ptr->KbArgNumber = 0;
            Ptr->KBdataHead = Ptr->KBdataTail;
            AckData = KBC_ACK_STATUS;
            UsbKbcStoreData (Ptr, KbcKB, AckData);
            Ptr->KbTypeRateDelay = Data;
        }
        break;

    case KB_ECHO_RESP:
        Ptr->KBdataHead = Ptr->KBdataTail;
        AckData = KBC_ECHO_STATUS;
        UsbKbcStoreData (Ptr, KbcKB, AckData);
        break;

    case KB_READ_ID:
        Ptr->KBdataHead = Ptr->KBdataTail;
        AckData = KBC_ACK_STATUS;
        UsbKbcStoreData (Ptr, KbcKB, AckData);
        AckData = 0xAB;
        UsbKbcStoreData (Ptr, KbcKB, AckData);
        AckData = 0x83;
        UsbKbcStoreData (Ptr, KbcKB, AckData);
        break;

    case KB_SET_DEFAULT:
        Ptr->KBdataHead = Ptr->KBdataTail;
        AckData = KBC_ACK_STATUS;
        UsbKbcStoreData (Ptr, KbcKB, AckData);
        Ptr->KbScanMode = 0x2;
        Ptr->KbKeyType = 0x34;
        break;

    case KB_SET_SCANCODE:
        if (Ptr->KbArgNumber == 0) {
            Ptr->KBdataHead = Ptr->KBdataTail;
            AckData = KBC_ACK_STATUS;
            UsbKbcStoreData (Ptr, KbcKB, AckData);
            ++Ptr->KbArgNumber;
        } else {
            Ptr->KbArgNumber = 0;
            Ptr->KBdataHead = Ptr->KBdataTail;
            AckData = KBC_ACK_STATUS;
            Ptr->KbScanMode = Data;
        }
        break;

    case KB_RESEND_CMD:
        Ptr->KBdataHead = Ptr->KBdataTail;
        AckData = Ptr->VirtualKBC60Port;
        UsbKbcStoreData (Ptr, KbcKB, AckData);
        break;

    case KB_RESET_CMD:
        Ptr->KBdataHead = Ptr->KBdataTail;
        AckData = KBC_ACK_STATUS;
        UsbKbcStoreData (Ptr, KbcKB, AckData);
        break;

    case KB_SETAKEY_TYPE:
    case KB_SETAKEY_BREAK:
    case KB_SETAKEY_MAKE:
    case KB_SETAKEY_TMB:
    case KB_SETKEY_TYPE:
    case KB_SETKEY_BREAK:
    case KB_SETKEY_MAKE:
    case KB_ENABLE_SCANING:
    case KB_DISABLE_SCANING:
        break;

    default:
        break;
    }

}

VOID
UsbKbcToMouseCommand (
    IN USB_VIRTUAL_KBC_DATA *Ptr,
    IN UINT8          Data
)
{
    UINT8   AckData;
    UINT8   DataType;

    if (Ptr->MsArgNumber == 0) {
        DataType = Data;
        Ptr->MsLastCmd = Data;
    } else {
        DataType = Ptr->MsLastCmd;
    }

    AckData = KBC_ACK_STATUS;
    Ptr->MSdataHead = Ptr->MSdataTail;

    UsbKbcStoreData (Ptr, KbcMS, AckData);
    KBC_DEBUG((EFI_D_ERROR, "MouseCommand DataType = %x\n",DataType));
    switch (DataType) {
    case  MS_READ_DATA:
    case  MS_RESET_WRAPMODE:
    case  MS_SET_WRAPMODE:
    case  MS_RESEND_CMD:
        break;

    case MS_SET_SAMPLERATE:
        if (Ptr->MsArgNumber == 0) {
            ++Ptr->MsArgNumber;
        } else {
            Ptr->MsArgNumber = 0;
            Ptr->MsSampleRate = Data;
        }
        break;

    case MS_SET_RESOLOTION:
        if (Ptr->MsArgNumber == 0) {
            ++Ptr->MsArgNumber;
        } else {
            Ptr->MsArgNumber = 0;
            Ptr->MsResolution = (1 << Data);
        }
        break;

    case MS_SET_SCALING21:
        Ptr->MsMode |= MSMODE_SCAL12;
        break;

    case MS_SET_SCALING11:
        Ptr->MsMode &= ~MSMODE_SCAL12;
        break;

    case MS_READ_STATUS:
        AckData = Ptr->MsMode;
        UsbKbcStoreData (Ptr, KbcMS, AckData);
        AckData = Ptr->MsResolution;
        UsbKbcStoreData (Ptr, KbcMS, AckData);
        AckData = Ptr->MsSampleRate;
        UsbKbcStoreData (Ptr, KbcMS, AckData);
        break;

    case MS_SET_STREAMMODE:
        Ptr->MsMode &= ~MSMODE_REMOTE;
        break;

    case MS_SET_REMOTEMODE:
        Ptr->MsMode |= MSMODE_REMOTE;
        break;

    case MS_READ_ID:
        AckData = (UINT8) Ptr->MsScrollingID;
        if (AckData == 0) {
            AckData = 0x03;
        }
        UsbKbcStoreData (Ptr, KbcMS, AckData);
        break;

    case MS_ENABLE_REPORT:
        Ptr->MsMode |= MSMODE_ENMS;
        break;

    case MS_DISABLE_REPORT:
        Ptr->MsMode &= ~MSMODE_ENMS;
        break;

    case MS_SET_DEFAULT:
        Ptr->MsSampleRate = MS_DEFAULT_SAMPLERATE;
        Ptr->MsResolution = MS_DEFAULT_RESOLUTION;
        Ptr->MsScrolling = MS_DEFAULT_SCROLLING;
        break;

    case MS_RESET_CMD:
        Ptr->MsSampleRate = MS_DEFAULT_SAMPLERATE;
        Ptr->MsResolution = MS_DEFAULT_RESOLUTION;
        Ptr->MsScrolling = MS_DEFAULT_SCROLLING;

        AckData = KBC_ACK1_STATUS;
        UsbKbcStoreData (Ptr, KbcMS, AckData);
        AckData = KBC_ACK2_STATUS;
        UsbKbcStoreData (Ptr, KbcMS, AckData);
        break;

    default:
        break;
    }

}

BOOLEAN
UsbKbcMouseDataReturn (
    IN USB_VIRTUAL_KBC_DATA *Ptr,
    IN UINT8          Data
)
{
    UINT8         AckData;
    UINT8         KbcSts;
    BOOLEAN       Flag;
    KBC_DATA_TYPE Type;

    Flag = TRUE;

    if (!(mVirtualKBCDev->IsMSPresent)) {
        Flag = FALSE;
        goto ON_EXIT;
    }

    KbcIBFEmpty ();
    SmmIoWrite8 (KBC_DATA_PORT, Data);
    KbcIBFEmpty ();

    while (KbcOBFull()) {
        KbcSts  = SmmIoRead8 (KBC_CMD_PORT);
        AckData = SmmIoRead8 (KBC_DATA_PORT);
        if (Data == MS_RESET_CMD) {
            KbcOBFull();
        }
        Type = KbcKB;
        if ((KbcSts & KBC_STS_MOBF) == KBC_STS_MOBF) {
            Type = KbcMS;
        }
        UsbVirtualKbcStoreData (Ptr, Type, AckData);
        Flag = FALSE;
    }

ON_EXIT:
    return Flag;
}


VOID
UsbKbcPort60Read (
    IN USB_VIRTUAL_KBC_DATA *Ptr,
    IN UINT8          *Data
)
{
    BOOLEAN   DevFlag;
    DevFlag = TRUE;
    DevFlag = mVirtualKBCDev->IsKbcPresent ? TRUE : FALSE;

    if (DevFlag) {
        *Data = SmmIoRead8 (KBC_DATA_PORT);
    } else {
        *Data = Ptr->VirtualKBC60Port;
        Ptr->VirtualKBC64Port &= ~(KBC_STS_OBF | KBC_STS_MOBF);
    }
}

VOID
UsbKbcPort64Read (
    IN USB_VIRTUAL_KBC_DATA *Ptr,
    IN UINT8          *Data
)
{
    BOOLEAN   DevFlag;
    DevFlag = TRUE;
    DevFlag = mVirtualKBCDev->IsKbcPresent ? TRUE : FALSE;

    if (DevFlag) {
        *Data = SmmIoRead8 (KBC_CMD_PORT);
    } else {
        *Data = Ptr->VirtualKBC64Port;
    }
    Ptr->VirtualKBC64Port &= ~KBC_STS_IBF;
}

VOID
UsbKbcPort60Write (
    IN USB_VIRTUAL_KBC_DATA *Ptr,
    IN UINT8          Data
)
{
    BOOLEAN DevFlag;
    UINT8 Control;
    DevFlag = TRUE;
    DevFlag = mVirtualKBCDev->IsKbcPresent ? TRUE : FALSE;
    mVirtualKBCDev->VirtualKBCApi->VirtualControl (&Control, FALSE);

    //
    // Process Gate A20 On/Off command if controller type is OHCI.
    //

    if (mVirtualKBCDev->BusType == USB_OHCI) {
        if ((Control & OHceC_GateA20S) == OHceC_GateA20S) {
            SmmKbcIoWrite8 (KBC_CMD_PORT, 0xD1);
            SmmKbcIoWrite8 (KBC_DATA_PORT, Data);
            SmmKbcIoWrite8 (KBC_CMD_PORT, 0xFF);
            return;
        }
    }
    KBC_DEBUG((EFI_D_ERROR, "Ptr->KBCLastCmd = %x, ", Ptr->KBCLastCmd));
    if (Ptr->KBCLastCmd == 0)
        UsbKbcToKeyboardCommand(Ptr, Data);

    switch (Ptr->KBCLastCmd) {

    case  KBC_SETPASS_BYTE:
    case  KBC_WRITKBB_BYTE:
        Ptr->KBCLastCmd = 0;
        break;

    case  KBC_WRITOUT_PORT:
        Ptr->KBCLastCmd = 0;
        Ptr->KBCOutPutPort = Data;
        break;

    case  KBC_WRITMSB_BYTE:
        Ptr->KBCLastCmd = 0;
        DevFlag = mVirtualKBCDev->IsMSPresent ? TRUE : FALSE;
        break;

    case  KBC_WRITCMD_BYTE:
        Ptr->KBCLastCmd = 0;
        Ptr->KBCCmdReg = Data;
        break;

    case  KBC_WRITMSD_BYTE:
        UsbKbcToMouseCommand (Ptr, Data);
        DevFlag = mVirtualKBCDev->IsMSPresent ? TRUE : FALSE;
        if (mVirtualKBCDev->BusType == USB_OHCI) {
            DevFlag = UsbKbcMouseDataReturn (Ptr ,Data);
        }
        break;

    default:
        break;
    }

    if (DevFlag) {
        KbcIBFEmpty ();
        SmmIoWrite8 (KBC_DATA_PORT, Data);
        KbcIBFEmpty ();
    }

}

VOID
UsbKbcPort64Write (
    IN USB_VIRTUAL_KBC_DATA *Ptr,
    IN UINT8          Data
)
{
    BOOLEAN   DevFlag;

    DevFlag = TRUE;
    DevFlag = mVirtualKBCDev->IsKbcPresent ? TRUE : FALSE;

    switch (Data) {
    case  KBC_WRITCMD_BYTE:
    case  KBC_GETVERS_NUMB:
    case  KBC_GETPASS_BYTE:
    case  KBC_SETPASS_BYTE:
    case  KBC_MOUSEIF_TEST:
    case  KBC_CRLSELF_TEST:
    case  KBC_KEYBDIF_TEST:
    case  KBC_READ_VERSION:
    case  KBC_WRITOUT_PORT:
    case  KBC_WRITKBB_BYTE:
    case  KBC_READILS_PORT:
    case  KBC_READIMS_PORT:
    case  KBC_CHKPASS_BYTE:
    case  KBC_READINP_PORT:
    case  KBC_READOUT_PORT:
    case  KBC_READTST_PORT:
    case  KBC_RESET_SYSTEM:
        Ptr->KBCLastCmd = Data;
        break;

    case  KBC_WRITMSD_BYTE:
    case  KBC_WRITMSB_BYTE:
        Ptr->KBCLastCmd = Data;
        Ptr->KBCSendStatus |= KBC_STS_OBF;
        DevFlag = mVirtualKBCDev->IsMSPresent ? TRUE : FALSE;
        break;

    case  KBC_READCMD_BYTE:
        //Ptr->KBCLastCmd = Data;
        KBC_DEBUG((EFI_D_ERROR, "KBC_READCMD_BYTE\n"));
        UsbKbcStoreData (Ptr, Kbc, Ptr->KBCCmdReg);
        break;

    case  KBC_DISMOUSE_INF:
        Ptr->KBCCmdReg |= KBC_CMD_DISMS;
        KBC_DEBUG((EFI_D_ERROR, "disable mouse\n"));
        break;

    case  KBC_ENAMOUSE_INF:
        Ptr->KBCCmdReg &= ~KBC_CMD_DISMS;
        KBC_DEBUG((EFI_D_ERROR, "enable mouse\n"));
        break;

    case  KBC_DISKEYBD_INF:
        Ptr->KBCCmdReg |= KBC_CMD_DISKB;
        break;

    case  KBC_ENAKEYBD_INF:
        Ptr->KBCCmdReg &= ~KBC_CMD_DISKB;
        break;

    default:
        break;
    }

    if (DevFlag) {
        KbcIBFEmpty ();
        SmmIoWrite8 (KBC_CMD_PORT, Data);
        KbcIBFEmpty ();
    }
    if (!DevFlag && (Data == KBC_RESET_SYSTEM)) {
        SmmIoWrite8 (PORT_92, PORT_92_RESET);
    }

}

VOID
UsbKbcKBMSData (IN USB_VIRTUAL_KBC_DATA *Ptr)
{
    UINT8 KbcSts;
    UINT8 Data;

    mVirtualKBCDev->VirtualKBCApi->VirtualStatus (&KbcSts, FALSE);

    if ((KbcSts & KBC_STS_OBF) == KBC_STS_OBF) {

        //
        // Store the PS2 keyboard and mouse data to buffer.
        //

        KbcSts = SmmIoRead8 (KBC_CMD_PORT);
        if (!(KbcSts & KBC_STS_OBF) == KBC_STS_OBF)
            goto ON_EXIT;

        Data = SmmIoRead8 (KBC_DATA_PORT);

        if ((KbcSts & KBC_STS_MOBF) == KBC_STS_MOBF) {
            Ptr->MSdata [Ptr->MSdataTail] = Data;
            ++Ptr->MSdataTail;
        } else {
            Ptr->KBdata [Ptr->KBdataTail] = Data;
            ++Ptr->KBdataTail;
        }
        UsbVirtualKbcTimer (TRUE);
    } else {

        //
        //  Send IRQ and Data immediately.
        //

        KbcSts = SmmIoRead8 (KBC_CMD_PORT);
        if (!(KbcSts & KBC_STS_OBF) == KBC_STS_OBF)
            goto ON_EXIT;

        Data = SmmIoRead8 (KBC_DATA_PORT);

        mVirtualKBCDev->VirtualKBCApi->VirtualSetData (Data);

        mVirtualKBCDev->VirtualKBCApi->VirtualStatus (&KbcSts, TRUE);
        mVirtualKBCDev->VirtualKBCApi->VirtualIRQ (KbcKB);
    }

ON_EXIT:
    return;
}

EFI_STATUS
UsbOhciVirtualOn (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT8                *SmiState
)
{
    if (mVirtualKBCDev == NULL)
        return EFI_SUCCESS;

    mVirtualKBCDev->Usb2Hc->GSetVirtualState (mVirtualKBCDev->Usb2Hc, EfiVirtualOn, SmiState, TRUE);
    return EFI_SUCCESS;
}

EFI_STATUS
UsbOhciVirtualOff (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT8                *SmiState
)
{
    if (mVirtualKBCDev == NULL)
        return EFI_SUCCESS;

    mVirtualKBCDev->Usb2Hc->GSetVirtualState (mVirtualKBCDev->Usb2Hc, EfiVirtualOff, SmiState, FALSE);
    return EFI_SUCCESS;
}

EFI_STATUS
UsbOhciVirtualCheckStatus (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT8                *SmiState
)
{
    if (mVirtualKBCDev == NULL)
        return EFI_SUCCESS;

    mVirtualKBCDev->Usb2Hc->GSetVirtualState (mVirtualKBCDev->Usb2Hc, EfiVirtualStatus, SmiState, TRUE);
    return EFI_SUCCESS;

}

EFI_STATUS
UsbOhciVirtualClearStatus (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT8                *SmiState
)
{
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbOhciControl (
    IN UINT8    *Data,
    IN BOOLEAN  RW
)
{
    mVirtualKBCDev->Usb2Hc->GSetVirtualState (mVirtualKBCDev->Usb2Hc, EfiVirtualControlReg, Data, RW);
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbOhciStatus (
    IN UINT8    *Data,
    IN BOOLEAN  RW
)
{
    mVirtualKBCDev->Usb2Hc->GSetVirtualState (mVirtualKBCDev->Usb2Hc, EfiVirtualStatusReg, Data, RW);
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbOhciSendIRQ (IN KBC_DATA_TYPE Type)
{

    //
    // IRQEn of Ohci will be set always when emulation on.
    // HC generates IRQ1 or IRQ12 when IBF in HceStatus is set.
    //
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbOhciGetData (IN UINT8 *Data)
{
    mVirtualKBCDev->Usb2Hc->GSetVirtualState (mVirtualKBCDev->Usb2Hc, EfiVirtualInputData, Data, FALSE);
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbOhciSetData (IN UINT8 Data)
{
    mVirtualKBCDev->Usb2Hc->GSetVirtualState (mVirtualKBCDev->Usb2Hc, EfiVirtualOuputData, &Data, TRUE);
    return EFI_SUCCESS;
}

EFI_STATUS
USBLPCVirtualOn (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT8                *SmiState
)
{
    EFI_STATUS  Status;

    Status = EFI_SUCCESS;

    if (mVirtualKBCDev == NULL) {
        return Status;
    }

    Status = mVirtualKbc->UsbVirtualKbcControl (
                 mVirtualKbc,
                 VirtualKbcOn,
                 SmiState);

    return Status;
}

EFI_STATUS
USBLPCVirtualOff (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT8                *SmiState
)
{
    EFI_STATUS  Status;

    Status = EFI_SUCCESS;

    if (mVirtualKBCDev == NULL) {
        return Status;
    }

    Status = mVirtualKbc->UsbVirtualKbcControl (
                 mVirtualKbc,
                 VirtualKbcOff,
                 SmiState);

    return Status;

}

EFI_STATUS
USBLPCVirtualCheckStatus (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT8                *SmiState
)
{
    EFI_STATUS  Status;

    Status = EFI_SUCCESS;

    if (mVirtualKBCDev == NULL) {
        return Status;
    }

    Status = mVirtualKbc->UsbVirtualKbcControl (
                 mVirtualKbc,
                 VirtualKbcCheckStatus,
                 SmiState);

    return Status;
}

EFI_STATUS
USBLPCVirtualClearStatus (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT8                *SmiState
)
{
    EFI_STATUS  Status;

    Status = EFI_SUCCESS;

    if (mVirtualKBCDev == NULL) {
        return Status;
    }

    Status = mVirtualKbc->UsbVirtualKbcControl (
                 mVirtualKbc,
                 VirtualKbcClearStatus,
                 SmiState);
    return Status;
}

EFI_STATUS
EFIAPI
USBLPCVirtualSendIRQ (IN KBC_DATA_TYPE Type)
{
    EFI_STATUS  Status;

    Status = EFI_SUCCESS;

    if (Type == KbcKB)
        Status = mVirtualKbc->UsbVirtualKbcIrq (
                     mVirtualKbc,
                     VirtualKbcIrq1);

    if (Type == KbcMS)
        Status = mVirtualKbc->UsbVirtualKbcIrq (
                     mVirtualKbc,
                     VirtualKbcIrq12);
    return Status;
}

EFI_STATUS
EFIAPI
USBLPCVirtualGetData (IN UINT8 *Data)
{
    EFI_STATUS  Status;

    Status = EFI_SUCCESS;

    if (mVirtualKBCDev == NULL) {
        return Status;
    }

    Status = mVirtualKbc->UsbVirtualKbcData (
                 mVirtualKbc,
                 VirtualKbcReadData,
                 Data);
    return Status;
}

EFI_STATUS
EFIAPI
USBLPCVirtualSetData (IN UINT8 Data)
{
    EFI_STATUS  Status;

    Status = EFI_SUCCESS;

    if (mVirtualKBCDev == NULL) {
        return Status;
    }

    Status = mVirtualKbc->UsbVirtualKbcData (
                 mVirtualKbc,
                 VirtualKbcWriteData,
                 &Data);

    return Status;
}

EFI_STATUS
UsbUhciVirtualOn (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT8                *SmiState
)
{
    if (mVirtualKBCDev == NULL)
        return EFI_SUCCESS;

    mVirtualKBCDev->Usb2Hc->GSetVirtualState (mVirtualKBCDev->Usb2Hc, EfiVirtualOn, SmiState, TRUE);
    return EFI_SUCCESS;
}

EFI_STATUS
UsbUhciVirtualOff (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT8                *SmiState
)
{
    if (mVirtualKBCDev == NULL)
        return EFI_SUCCESS;

    mVirtualKBCDev->Usb2Hc->GSetVirtualState (mVirtualKBCDev->Usb2Hc, EfiVirtualOff, SmiState, FALSE);
    return EFI_SUCCESS;
}

EFI_STATUS
UsbUhciVirtualCheckStatus (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT8                *SmiState
)
{
    if (mVirtualKBCDev == NULL)
        return EFI_SUCCESS;

    mVirtualKBCDev->Usb2Hc->GSetVirtualState (mVirtualKBCDev->Usb2Hc, EfiVirtualStatus, SmiState, TRUE);
    return EFI_SUCCESS;

}

EFI_STATUS
UsbUhciVirtualClearStatus (
    IN EFI_USB2_HC_PROTOCOL *This,
    IN UINT8                *SmiState
)
{
    USB_BUS               *BusInstance;
    EFI_USB2_HC_PROTOCOL  *Usb2Hc;
    LIST_ENTRY            *Link;

    if (IsListEmpty (&mUsbBusList))
        return EFI_SUCCESS;

    if (mVirtualKBCDev == NULL)
        return EFI_SUCCESS;

    //
    // Find the asynchronous transfer to this device/endpoint pair.
    //
    Link  = mUsbBusList.ForwardLink;
    do {
        BusInstance  = USB_BUS_FROM_LINK (Link);
        Usb2Hc = BusInstance->Usb2Hc;
        if (BusInstance->HostControllerType == USB_UHCI) {
            mVirtualKBCDev->Usb2Hc->GSetVirtualState (Usb2Hc, EfiVirtualClearStatus, SmiState, TRUE);
        }
        Link = Link->ForwardLink;
    } while (Link != &mUsbBusList);

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbUhciControl (
    IN UINT8    *Data,
    IN BOOLEAN  RW
)
{
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbUhciStatus (
    IN UINT8    *Data,
    IN BOOLEAN  RW
)
{
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbUhciSendIRQ (IN KBC_DATA_TYPE Type)
{
    UINT16  Value;
    UINT16  TCOBase;
    UINT32  PciAddress;
    UINT8   Data;

    if (Type == Kbc)
        goto ON_EXIT;

    //
    // Used Chipset to issue IRQ 1 or 12.
    //
    PciAddress = SMM_PCI_ADDRESS (0, 0x1F, 0, 0);
    SmmPciCfgRead (
        gSmst,
        SmmPciWidthUint16,
        (SMM_PCI_IO_ADDRESS *) &PciAddress,
        &Value
    );

    Value &= 0xFFFF;
    if (Value == 0x8086) {
        PciAddress = SMM_PCI_ADDRESS (0, 0x1F, 0, 0x40);
        SmmPciCfgRead (
            gSmst,
            SmmPciWidthUint16,
            (SMM_PCI_IO_ADDRESS *) &PciAddress,
            &Value
        );

        Value &= ~BIT0;
        TCOBase = (UINT16) Value;
        TCOBase += 0x70;
        Data = SmmIoRead8 (TCOBase);

        if (Type == KbcKB) {
            SmmIoWrite8 (TCOBase, (Data & 0xFE));
        }

        if (Type == KbcMS) {
            SmmIoWrite8 (TCOBase, (Data & 0xFD));
        }

        SmmIoWrite8 (TCOBase, Data);
    }

ON_EXIT:
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbUhciGetData (IN UINT8 *Data)
{
    EFI_STATUS                 Status;
    UINT32                     DataEAX;
    UINT32                     Index;
    UINT32                     CpuIndex;
    EFI_SMM_SAVE_STATE_IO_INFO IoState;

    //
    // Find out which CPU triggered the SW SMI
    //
    CpuIndex = 0;
    for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
        Status = mSmmCpu->ReadSaveState (
                     mSmmCpu,
                     sizeof (EFI_SMM_SAVE_STATE_IO_INFO),
                     EFI_SMM_SAVE_STATE_REGISTER_IO,
                     Index,
                     &IoState
                 );
        if (!EFI_ERROR (Status) && (IoState.IoPort == 0x60 || IoState.IoPort == 0x64)) {
            CpuIndex = Index;
            KBC_DEBUG((EFI_D_ERROR, "P = %x, ", CpuIndex));
            KBC_DEBUG((EFI_D_ERROR, "IoPort = %x, ", IoState.IoPort));
            break;
        }
    }

    DataEAX = 0;
    Status  = mSmmCpu->ReadSaveState (
                  mSmmCpu,
                  sizeof (UINT32),
                  EFI_SMM_SAVE_STATE_REGISTER_RAX,
                  CpuIndex,
                  &DataEAX
              );
    ASSERT_EFI_ERROR (Status);

    *Data = (UINT8)DataEAX;
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbUhciSetData (IN UINT8 Data)
{
    EFI_STATUS                 Status;
    UINT32                     DataEAX;
    UINT32                     Index;
    UINT32                     CpuIndex;
    EFI_SMM_SAVE_STATE_IO_INFO IoState;
    EFI_SMI_CPU_SAVE_STATE     *SaveState;
    //
    // Find out which CPU triggered the SW SMI
    //
    CpuIndex = 0;
    for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
        Status = mSmmCpu->ReadSaveState (
                     mSmmCpu,
                     sizeof (EFI_SMM_SAVE_STATE_IO_INFO),
                     EFI_SMM_SAVE_STATE_REGISTER_IO,
                     Index,
                     &IoState
                 );
        if (!EFI_ERROR (Status) && (IoState.IoPort == 0x60 || IoState.IoPort == 0x64)) {
            CpuIndex = Index;
            KBC_DEBUG((EFI_D_ERROR, "P = %x, ", CpuIndex));
            KBC_DEBUG((EFI_D_ERROR, "IoPort = %x, ", IoState.IoPort));
            break;
        }
    }
    DataEAX = 0;
    Status  = mSmmCpu->ReadSaveState (
                  mSmmCpu,
                  sizeof (UINT32),
                  EFI_SMM_SAVE_STATE_REGISTER_RAX,
                  CpuIndex,
                  &DataEAX
              );
    ASSERT_EFI_ERROR (Status);

    DataEAX = DataEAX & 0xFFFFFF00;
    DataEAX |= (UINT32)Data;
    Status   = mSmmCpu->WriteSaveState (
                   mSmmCpu,
                   sizeof (UINT32),
                   EFI_SMM_SAVE_STATE_REGISTER_RAX,
                   CpuIndex,
                   &DataEAX
               );
    ASSERT_EFI_ERROR (Status);
    if (mFrameworkSmst != NULL) {
      SaveState = &mFrameworkSmst->CpuSaveState[CpuIndex].Ia32SaveState;
      SaveState->EAX = DataEAX;
    }

    return EFI_SUCCESS;
}


USB_VIRTUAL_KBC_API
mUsbLPCAPI = {
    USBLPCVirtualOn,
    USBLPCVirtualOff,
    USBLPCVirtualCheckStatus,
    USBLPCVirtualClearStatus,
    UsbUhciControl,
    UsbUhciStatus,
    USBLPCVirtualSendIRQ,
    USBLPCVirtualGetData,
    USBLPCVirtualSetData
};

USB_VIRTUAL_KBC_API
mUsbUhciAPI = {
    UsbUhciVirtualOn,
    UsbUhciVirtualOff,
    UsbUhciVirtualCheckStatus,
    UsbUhciVirtualClearStatus,
    UsbUhciControl,
    UsbUhciStatus,
    UsbUhciSendIRQ,
    UsbUhciGetData,
    UsbUhciSetData
};

USB_VIRTUAL_KBC_API
mUsbOhciAPI = {
    UsbOhciVirtualOn,
    UsbOhciVirtualOff,
    UsbOhciVirtualCheckStatus,
    UsbOhciVirtualClearStatus,
    UsbOhciControl,
    UsbOhciStatus,
    UsbOhciSendIRQ,
    UsbOhciGetData,
    UsbOhciSetData
};

BOOLEAN
UsbVirtualKbcCheckKbcPresent (VOID)
{
    UINT8   KbcSts;

    KbcSts = SmmIoRead8 (KBC_CMD_PORT);

    if (KbcSts == KBC_STS_ERR0) {
        return FALSE;
    }

    if (KbcSts == KBC_STS_ERR1) {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
UsbVirtualKbcCheckKbPresent (VOID)
{
    UINT8   KbcSts;
    BOOLEAN KeyboardFlag;

    KeyboardFlag = FALSE;
    KbcSts = SmmIoRead8 (KBC_CMD_PORT);

    if ((KbcSts & KBC_STS_OBF) == KBC_STS_OBF) {
        KbcSts = SmmIoRead8 (KBC_DATA_PORT);
    }

    //
    // Disable keyboard interface of Kbc.
    //
    KbcIBFEmpty ();
    SmmIoWrite8 (KBC_CMD_PORT, KBC_DISKEYBD_INF);

    //
    // Send the keyboard enable scaning command.
    //
    KbcIBFEmpty ();
    SmmIoWrite8 (KBC_DATA_PORT, KB_ENABLE_SCANING);

    if (KbcOBFull()) {
        KbcSts = SmmIoRead8 (KBC_DATA_PORT);
        if (KbcSts == KBC_ACK_STATUS) {
            KeyboardFlag = TRUE;
        }
    }

    KbcIBFEmpty ();
    SmmIoWrite8 (KBC_CMD_PORT, KBC_ENAKEYBD_INF);
    KbcIBFEmpty ();
    return KeyboardFlag;
}

BOOLEAN
UsbVirtualKbcCheckMsPresent (VOID)
{
    UINT8   KbcSts;
    BOOLEAN MouseFlag;

    MouseFlag = FALSE;

    KbcSts = SmmIoRead8 (KBC_CMD_PORT);
    if ((KbcSts & KBC_STS_OBF) == KBC_STS_OBF) {
        KbcSts = SmmIoRead8 (KBC_DATA_PORT);
    }

    //
    // Disable keyboard interface of Kbc.
    //
    KbcIBFEmpty ();
    SmmIoWrite8 (KBC_CMD_PORT, KBC_DISKEYBD_INF);

    //
    // Send the mouse disable report command.
    //
    KbcIBFEmpty ();
    SmmIoWrite8 (KBC_CMD_PORT, KBC_WRITMSD_BYTE);
    KbcIBFEmpty ();
    SmmIoWrite8 (KBC_DATA_PORT, MS_DISABLE_REPORT);
    KbcIBFEmpty ();

    if (KbcOBFull()) {
        KbcSts = SmmIoRead8 (KBC_DATA_PORT);
        if (KbcSts == KBC_ACK_STATUS) {
            MouseFlag = TRUE;
        }
    }
    KbcIBFEmpty ();
    SmmIoWrite8 (KBC_CMD_PORT, KBC_ENAKEYBD_INF);
    KbcIBFEmpty ();
    return MouseFlag;
}


EFI_STATUS
UsbVirtualkbcInitialize (
    EFI_USB2_HC_PROTOCOL *This,
    UINT8                HostControllerType
)
{
    EFI_STATUS          Status;
    UINT8               SmiState;
    USB_DEBUG((EFI_D_ERROR, "PvtUsbVirtualkbcInitialize start\n"));
    if (mVirtualKBCDev == NULL) {
      Status = gBS->AllocatePool (
                      EfiReservedMemoryType,
                      sizeof (USB_VIRTUAL_KBC_DEV),
                      (VOID **) &mVirtualKBCDev
                      );
      ASSERT_EFI_ERROR (Status);
      if (EFI_ERROR(Status)) {
        return EFI_OUT_OF_RESOURCES;
      } else {
        DEBUG((EFI_D_ERROR, "UsbVirtualkbcInitialize.mVirtualKBCDev:%lX,S:%X\n",
                             (UINT64)(UINTN)mVirtualKBCDev,
                             sizeof(USB_VIRTUAL_KBC_DEV)
                             ));
      }
      ZeroMem (mVirtualKBCDev, sizeof (USB_VIRTUAL_KBC_DEV));
    }

    mVirtualKBCDev->Usb2Hc  = This;
    mVirtualKBCDev->BusType = HostControllerType;
    mVirtualKBCDev->VirtualKbcData.VirtualKBC64Port = KBC_STS_INH | KBC_STS_A2 | KBC_STS_SYS;
    mVirtualKBCDev->VirtualKbcData.KBCCmdReg  |= KBC_CMD_INT | KBC_CMD_INT2 | KBC_CMD_SYS;

    mVirtualKBCDev->VirtualFlag = KBC_PORT64_WRITE | KBC_PORT64_READ |
                        KBC_PORT60_WRITE | KBC_PORT60_READ;

    mVirtualKBCDev->IsKbcPresent = UsbVirtualKbcCheckKbcPresent ();
    USB_DEBUG((EFI_D_ERROR, "mVirtualKBCDev->IsKbcPresent = %x\n", mVirtualKBCDev->IsKbcPresent));
    if (mVirtualKBCDev->IsKbcPresent) {
        mVirtualKBCDev->IsKBPresent = UsbVirtualKbcCheckKbPresent();
        mVirtualKBCDev->IsMSPresent = UsbVirtualKbcCheckMsPresent();
        mVirtualKBCDev->VirtualFlag = KBC_PORT60_WRITE;//KBC_PORT64_WRITE | KBC_PORT60_WRITE | KBC_PORT60_READ;
    }
    USB_DEBUG((EFI_D_ERROR, "mVirtualKBCDev->VirtualFlag = %x\n", mVirtualKBCDev->VirtualFlag));
    if (mVirtualKBCDev->BusType == USB_OHCI) {
        mVirtualKBCDev->VirtualKBCApi = &mUsbOhciAPI;
        if (!IsLegacySupport(This) && !mVirtualKBCDev->IsKbcPresent) {
          FreePool (mVirtualKBCDev);
          mVirtualKBCDev     = NULL;
          mLegacyFreeSupport = TRUE;
          USB_DEBUG((EFI_D_ERROR, "OHCI not support LegacySupport!\n"));
          return EFI_SUCCESS;
        }
    }
    if (mVirtualKBCDev->BusType == USB_UHCI) {
        mVirtualKBCDev->VirtualKBCApi = &mUsbUhciAPI;
    }
    if (mVirtualKBCDev->BusType == USB_EHCI) {
        mVirtualKBCDev->VirtualKBCApi = &mUsbLPCAPI;
    }
    mVirtualKBCDev->VirtualKBCApi->VirtualClearStatus(This, &SmiState);
    USB_DEBUG((EFI_D_ERROR, "&mVirtualKBCDev->VirtualKbcData = %x\n",&mVirtualKBCDev->VirtualKbcData));
    mVirtualKBCDev->VirtualKBCApi->VirtualEnable(This, &mVirtualKBCDev->VirtualFlag);
    USB_DEBUG((EFI_D_ERROR, "PvtUsbVirtualkbcInitialize end\n"));
    return EFI_SUCCESS;

}

VOID
UsbVirtualKbcDispatcher (IN BOOLEAN Flag)
{
    EFI_USB2_HC_PROTOCOL  *Usb2Hc;
    USB_VIRTUAL_KBC_DATA        *VirtualKbcData;
    UINT8                 SmiState;
    UINT8                 KbcSts;

    SmiState = 0;
    KbcSts   = 0;

    if (mVirtualKBCDev == NULL)
        goto ON_EXIT;

    Usb2Hc = mVirtualKBCDev->Usb2Hc;
    VirtualKbcData = &mVirtualKBCDev->VirtualKbcData;

    if (Flag == TRUE) {
        //
        // Enable USB emulation.
        //
        mVirtualKBCDev->VirtualKBCApi->VirtualClearStatus (Usb2Hc, &SmiState);
        mVirtualKBCDev->VirtualKBCApi->VirtualEnable (Usb2Hc, &mVirtualKBCDev->VirtualFlag);
    } else {
        //
        // Disable USB emulation and check USB emulation event.
        //
        SmiState = mVirtualKBCDev->VirtualFlag;
        mVirtualKBCDev->VirtualKBCApi->VirtualCheckStatus (Usb2Hc, &SmiState);
        mVirtualKBCDev->VirtualKBCApi->VirtualDisable (Usb2Hc, &mVirtualKBCDev->VirtualFlag);

        if (SmiState == KBC_NO_EMULATION)
            goto ON_EXIT;
        KBC_DEBUG((EFI_D_ERROR, "Op = %x, ", SmiState));
        switch (SmiState) {
        case KBC_PORT64_WRITE:
            mVirtualKBCDev->VirtualKBCApi->VirtualGetData (&KbcSts);
            UsbKbcPort64Write (VirtualKbcData, KbcSts);
            break;

        case KBC_PORT64_READ:
            UsbKbcPort64Read (VirtualKbcData, &KbcSts);
            mVirtualKBCDev->VirtualKBCApi->VirtualSetData (KbcSts);
            break;

        case KBC_PORT60_WRITE:
            mVirtualKBCDev->VirtualKBCApi->VirtualGetData (&KbcSts);
            UsbKbcPort60Write (VirtualKbcData, KbcSts);
            break;

        case KBC_PORT60_READ:
            UsbKbcPort60Read (VirtualKbcData, &KbcSts);
            mVirtualKBCDev->VirtualKBCApi->VirtualSetData (KbcSts);
            break;

        case KBC_KBMS_DATA:
            UsbKbcKBMSData (VirtualKbcData);
            break;

        default:
            break;
        }
        KBC_DEBUG((EFI_D_ERROR, "Data = %x, ", KbcSts));
        KBC_DEBUG((EFI_D_ERROR, "60 = %x, ",mVirtualKBCDev->VirtualKbcData.VirtualKBC60Port));
        if (mVirtualKBCDev->IsKbcPresent) {
            KBC_DEBUG((EFI_D_ERROR, "64 = %x\n",SmmIoRead8 (0x64)));
        } else {
            KBC_DEBUG((EFI_D_ERROR, "64 = %x\n",mVirtualKBCDev->VirtualKbcData.VirtualKBC64Port));
        }
        mVirtualKBCDev->VirtualKBCApi->VirtualClearStatus (Usb2Hc, &SmiState);
    }

ON_EXIT:
    return;
}

EFI_STATUS
UsbConnectVirtualKbc (VOID)
{
    EFI_STATUS            Status;
    LIST_ENTRY            *Link;
    USB_BUS               *BusInstance;
    EFI_USB2_HC_PROTOCOL  *Usb2Hc;
    USB_DEBUG((EFI_D_ERROR, "UsbConnectVirtualKbc start\n"));
    if (IsListEmpty (&mUsbBusList)) {
        return EFI_SUCCESS;
    }

    Link         = mUsbBusList.ForwardLink;
    BusInstance  = NULL;
    Usb2Hc       = NULL;
    if (mVirtualKbc == NULL) {
        Status = gSmst->SmmLocateProtocol (
                     &gEfiUsbVirtualKbcProtocolGuid,
                     NULL,
                     &mVirtualKbc);
        USB_DEBUG((EFI_D_ERROR, "locate gEfiUsbVirtualKbcProtocolGuid Status = %r\n", Status));
        if (Status != EFI_SUCCESS) {
            mVirtualKbc = NULL;
        }
    }

    if (mVirtualKbc != NULL) {
        mVirtualKbc->UsbVirtualKbcCallBack(
            mVirtualKbc,
            (VOID *) (UINTN) UsbSmiDispatcher);
        USB_DEBUG((EFI_D_ERROR, "UsbConnectVirtualKbc start 1\n"));
    }

    while (Link != &mUsbBusList) {
        BusInstance = USB_BUS_FROM_LINK (Link);
        Usb2Hc = BusInstance->Usb2Hc;
        if ((BusInstance->HostControllerType == USB_EHCI) && (mVirtualKbc != NULL)) {
            USB_DEBUG((EFI_D_ERROR, "EHCI VirtualKBCDriver Init start\n"));
            UsbVirtualkbcInitialize (Usb2Hc, BusInstance->HostControllerType);
            break;
        }
        if (BusInstance->HostControllerType == USB_UHCI || BusInstance->HostControllerType == USB_OHCI) {
            DEBUG ((EFI_D_ERROR, "UHCI or OHCI VirtualKBCDriver Init start\n"));
            UsbVirtualkbcInitialize (Usb2Hc, BusInstance->HostControllerType);
            break;
        }
        Link = Link->ForwardLink;
    }
    USB_DEBUG((EFI_D_ERROR, "Connect PcdKeepTrap64hEnable = %x\n",PcdGet8 (PcdKeepTrap64hEnable)));
    KeepTrap6064hEnable = PcdGet8 (PcdKeepTrap64hEnable);
    USB_DEBUG((EFI_D_ERROR, "mLegacyFreeSupport = %x\n", mLegacyFreeSupport));
    return EFI_SUCCESS;
}

EFI_STATUS
UsbDisconnectVirtualKBCDriver (VOID)
{
    UINT8   SmiState;
    USB_DEBUG((EFI_D_ERROR, "UsbDisconnectVirtualKBCDriver\n"));
    if (mVirtualKBCDev == NULL)
        goto ON_EXIT;

    if (KeepTrap6064hEnable)
        goto ON_EXIT;

    mVirtualKBCDev->VirtualKBCApi->VirtualDisable (mVirtualKBCDev->Usb2Hc, &mVirtualKBCDev->VirtualFlag);
    mVirtualKBCDev->VirtualKBCApi->VirtualClearStatus (mVirtualKBCDev->Usb2Hc, &SmiState);
    // FreePool (mVirtualKBCDev);
    mVirtualKBCDev = NULL;

ON_EXIT:
    return EFI_SUCCESS;
}

VOID
SendLedCommand ()
{
  UINT8         Command;
  UINT8         KbcSts;
  BDA_LED_MAP   *BdaLed;

  BdaLed = (BDA_LED_MAP *)(UINTN)BDA_LED_PTR;
  //
  // Light configuration
  //
  Command = 0;
  if (BdaLed->CapsLock) {
    Command |= 4;
  }
  if (BdaLed->NumLock) {
    Command |= 2;
  }
  if (BdaLed->ScrollLock) {
    Command |= 1;
  }
  if (KbcIBFEmpty()) {
    SmmKbcIoWrite8 (KBC_DATA_PORT, 0xed);
    if (KbcOBFull()) {
      KbcSts = SmmIoRead8 (KBC_DATA_PORT);
      if (KbcIBFEmpty()) {
        SmmKbcIoWrite8 (KBC_DATA_PORT, Command);
        if (KbcOBFull()) {
          KbcSts = SmmIoRead8 (KBC_DATA_PORT);
        }
      }
    }
  }
}

VOID
SetPs2LedState ()
{
  static BOOLEAN   Ps2KbPresent  = FALSE;
  static BOOLEAN   DeteckHasDone = FALSE;

  if (!DeteckHasDone) {
    if (UsbVirtualKbcCheckKbcPresent ()) {
      if (UsbVirtualKbcCheckKbPresent ()) {
        Ps2KbPresent  = TRUE;
      }
    }
    DeteckHasDone = TRUE;
  }

  if (Ps2KbPresent) {
    SendLedCommand ();
  }

  return;
}

EFI_STATUS
SyncLedState(
    IN  EFI_HANDLE                    DispatchHandle,
    IN CONST VOID                     *DispatchContext,
    IN OUT VOID                       *CommBuffer,
    IN OUT UINTN                      *CommBufferSize
)
{
  DEBUG((EFI_D_INFO, __FUNCTION__"()\n"));

  SetPs2LedState ();
  if (mIfLegacyUsbStarted) {
    UsbSetLED ();
  }

  return EFI_SUCCESS;
}

