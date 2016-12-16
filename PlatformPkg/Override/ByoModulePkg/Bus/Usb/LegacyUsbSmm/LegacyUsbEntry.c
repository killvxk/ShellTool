/*++
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2009 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================
Module Name:
  LegacyUsbEntry.c

Abstract:
  USB Module file.

Revision History:

  Bug 1989:   Changed to use dynamic software SMI value instead of hard coding.
  TIME:       2011-6-15
  $AUTHOR:    Peng Xianbing
  $REVIEWERS:
  $SCOPE:     Define SwSmi value range build a PolicyData table for csm16 to
              get SwSMI value.
  $TECHNICAL:
  $END-------------------------------------------------------------------------
--*/
#include <Protocol/SmmSwDispatch2.h>
#include <protocol/SmmStatusCode.h>
#include <protocol/SwSmiValuePolicyData.h>

#include "LegacyUsb.h"
#include "../UsbBoot/UsbBoot.h"


char IbvPointer[] = "Copyright (C) 2006-2010, BYOSOFT, Inc. All Rights Reserved.";
char UsbVersion[] = "BU2001 06/01/2010";

EFI_SMM_CPU_PROTOCOL                *mSmmCpu = NULL;
EFI_HANDLE                          mLegacyUsbHandle = NULL;
EFI_SMM_SYSTEM_TABLE                *mFrameworkSmst  = NULL;
EFI_SMM_BASE_HELPER_READY_PROTOCOL  *mSmmBaseHelperReady;
volatile USB_SMMCALL_COMM           *mSmmCallComm;
SMMCALL_ENTRY                       *mSmmCallTablePtr;

SMMCALL_ENTRY UsbSmmCallTable[__ID_LastSmmCall];

extern BOOLEAN mIfLegacyUsbStarted;

EFI_STATUS
SyncLedState(
    IN  EFI_HANDLE                    DispatchHandle,
    IN CONST VOID                    *DispatchContext,
    IN OUT VOID                       *CommBuffer,
    IN OUT UINTN                      *CommBufferSize
);

EFI_STATUS
DoSmmCallEntry(
    IN  EFI_HANDLE                    DispatchHandle,
    IN CONST VOID                    *Context,
    IN OUT VOID                       *CommBuffer,
    IN OUT UINTN                      *CommBufferSize
)
{
    SMMCALL_ENTRY SmmCallFunc;

    EFI_SMM_SW_REGISTER_CONTEXT   *DispatchContext;
    DispatchContext = (EFI_SMM_SW_REGISTER_CONTEXT *)Context;

    if (DispatchContext->SwSmiInputValue == SMMCALL_SMI_VALUE) {
        SmmCallFunc = UsbSmmCallTable[mSmmCallComm->CallId];

        switch (mSmmCallComm->Argc) {

        case 0:
            *(mSmmCallComm->Return) = SmmCallFunc();
            break;
        case 1:
            *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0]);
            break;
        case 2:
            *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0], mSmmCallComm->Argv[1]);
            break;
        case 3:
            *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0], mSmmCallComm->Argv[1],
                                                  mSmmCallComm->Argv[2]);
            break;
        case 4:
            *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0], mSmmCallComm->Argv[1],
                                                  mSmmCallComm->Argv[2], mSmmCallComm->Argv[3]);
            break;
        case 5:
            *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0], mSmmCallComm->Argv[1],
                                                  mSmmCallComm->Argv[2], mSmmCallComm->Argv[3], mSmmCallComm->Argv[4]);
            break;
        case 6:
            *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0], mSmmCallComm->Argv[1],
                                                  mSmmCallComm->Argv[2], mSmmCallComm->Argv[3], mSmmCallComm->Argv[4], mSmmCallComm->Argv[5]);
            break;
        case 7:
            *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0], mSmmCallComm->Argv[1],
                                                  mSmmCallComm->Argv[2], mSmmCallComm->Argv[3], mSmmCallComm->Argv[4], mSmmCallComm->Argv[5],
                                                  mSmmCallComm->Argv[6]);
            break;
        case 8:
            *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0], mSmmCallComm->Argv[1],
                                                  mSmmCallComm->Argv[2], mSmmCallComm->Argv[3], mSmmCallComm->Argv[4], mSmmCallComm->Argv[5],
                                                  mSmmCallComm->Argv[6], mSmmCallComm->Argv[7]);
            break;
        case 9:
            *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0], mSmmCallComm->Argv[1],
                                                  mSmmCallComm->Argv[2], mSmmCallComm->Argv[3], mSmmCallComm->Argv[4], mSmmCallComm->Argv[5],
                                                  mSmmCallComm->Argv[6], mSmmCallComm->Argv[7], mSmmCallComm->Argv[8]);
            break;

        case 10:
            *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0], mSmmCallComm->Argv[1],
                                                  mSmmCallComm->Argv[2], mSmmCallComm->Argv[3], mSmmCallComm->Argv[4], mSmmCallComm->Argv[5],
                                                  mSmmCallComm->Argv[6], mSmmCallComm->Argv[7], mSmmCallComm->Argv[8], mSmmCallComm->Argv[9]);
            break;

        case 11:
            *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0], mSmmCallComm->Argv[1],
                                                  mSmmCallComm->Argv[2], mSmmCallComm->Argv[3], mSmmCallComm->Argv[4], mSmmCallComm->Argv[5],
                                                  mSmmCallComm->Argv[6], mSmmCallComm->Argv[7], mSmmCallComm->Argv[8], mSmmCallComm->Argv[9],
                                                  mSmmCallComm->Argv[10]);
            break;

        case 12:
            *(mSmmCallComm->Return) = SmmCallFunc(mSmmCallComm->Argv[0], mSmmCallComm->Argv[1],
                                                  mSmmCallComm->Argv[2], mSmmCallComm->Argv[3], mSmmCallComm->Argv[4], mSmmCallComm->Argv[5],
                                                  mSmmCallComm->Argv[6], mSmmCallComm->Argv[7], mSmmCallComm->Argv[8], mSmmCallComm->Argv[9],
                                                  mSmmCallComm->Argv[10], mSmmCallComm->Argv[11]);
            break;

        default:
            *(mSmmCallComm->Return) = EFI_UNSUPPORTED;
        }
    }

    return EFI_SUCCESS;
}

CSM16_SMMCALL_COMM *
PointCommBuf (
    IN    VOID
)
/*++
--*/
{
    CSM16_SMMCALL_COMM    *CommBuf;
    UINT16                *pSegOfEbda;
    UINT32                mToEbda;

    pSegOfEbda = (UINT16 *)(UINTN)0x40E;
    mToEbda    = (UINT32)(((UINTN)(*pSegOfEbda) << 4) + 0x90);
    CommBuf    = (CSM16_SMMCALL_COMM *)(UINTN)mToEbda;

    return CommBuf;
}

EFI_STATUS
DoCsm16SmmCall(
    IN  EFI_HANDLE                    DispatchHandle,
    IN CONST VOID                    *DispatchContext,
    IN OUT VOID                       *CommBuffer,
    IN OUT UINTN                      *CommBufferSize
)
{
    CSM16_SMMCALL_COMM                        *CommBuf;
    UINT64                                    Handle;
    EFI_LBA                                   Lba;

    CommBuf = PointCommBuf();
    if (CSM16_DISABLE_USB_DEVICE == CommBuf->CallId) {
        if (mIfLegacyUsbStarted) {
            EndLegacyUsb();
        }
        return EFI_SUCCESS;
    }
    if (mIfLegacyUsbStarted == FALSE) {
        return EFI_DEVICE_ERROR;
#if 0
        if (StartLegacyUsb() != EFI_SUCCESS) {
            CommBuf->StatusCode = 0xffffffff;
            return EFI_DEVICE_ERROR;
        }
#endif
    }

    switch (CommBuf->CallId) {
    case CSM16_GET_USB_BOOT_DEVICE_INFO:
        Csm16GetUsbBootDeviceInfo();
        CommBuf->StatusCode = 0;
        break;

    case CSM16_READ_USB_BOOT_DEVICE:
        Handle = CommBuf->Arguments[1];
        Handle = (LShiftU64(Handle,32) & 0xffffffff00000000) + CommBuf->Arguments[0];
        Lba = CommBuf->Arguments[3];
        Lba = (LShiftU64(Lba,32) & 0xffffffff00000000) + CommBuf->Arguments[2];
        CommBuf->StatusCode = Csm16ReadUsbBootDevice(
                                  Handle, Lba, CommBuf->Arguments[4], CommBuf->Arguments[5]);
        break;

    case CSM16_WRITE_USB_BOOT_DEVICE:
        Handle = CommBuf->Arguments[1];
        Handle = (LShiftU64(Handle, 32) & 0xffffffff00000000) + CommBuf->Arguments[0];
        Lba = CommBuf->Arguments[3];
        Lba = (LShiftU64(Lba, 32) & 0xffffffff00000000) + CommBuf->Arguments[2];
        CommBuf->StatusCode = Csm16WriteUsbBootDevice(
                                  Handle, Lba, CommBuf->Arguments[4], CommBuf->Arguments[5]);
        break;

    case CSM16_VERIFY_USB_BOOT_DEVICE:
        Handle = CommBuf->Arguments[1];
        Handle = (LShiftU64(Handle,32) & 0xffffffff00000000) + CommBuf->Arguments[0];
        Lba = CommBuf->Arguments[3];
        Lba = (LShiftU64(Lba,32) & 0xffffffff00000000) + CommBuf->Arguments[2];
        CommBuf->StatusCode = Csm16VerifyUsbBootDevice(
                                  Handle, Lba, CommBuf->Arguments[4]);

    default:
        break;
    }

    return EFI_SUCCESS;
}

VOID ConstructSmmCallTable(VOID)
{
    INIT_SMMCALL(StartLegacyUsb);
    INIT_SMMCALL(EndLegacyUsb);

    INIT_SMMCALL(GetOhcInfo);
    INIT_SMMCALL(OhciReset);
    INIT_SMMCALL(OhciGetState);
    INIT_SMMCALL(OhciSetState);
    INIT_SMMCALL(OhciControlTransfer);
    INIT_SMMCALL(OhciBulkTransfer);
    INIT_SMMCALL(OhciAsyncInterruptTransfer);
    INIT_SMMCALL(OhciSyncInterruptTransfer);
    INIT_SMMCALL(OhciIsochronousTransfer);
    INIT_SMMCALL(OhciAsyncIsochronousTransfer);
    INIT_SMMCALL(OhciGetRootHubPortNumber);
    INIT_SMMCALL(OhciGetRootHubPortStatus);
    INIT_SMMCALL(OhciSetRootHubPortFeature);
    INIT_SMMCALL(OhciClearRootHubPortFeature);

    INIT_SMMCALL(Ohci2GetCapability);
    INIT_SMMCALL(Ohci2Reset);
    INIT_SMMCALL(Ohci2GetState);
    INIT_SMMCALL(Ohci2SetState);
    INIT_SMMCALL(Ohci2ControlTransfer);
    INIT_SMMCALL(Ohci2BulkTransfer);
    INIT_SMMCALL(Ohci2AsyncInterruptTransfer);
    INIT_SMMCALL(Ohci2SyncInterruptTransfer);
    INIT_SMMCALL(Ohci2IsochronousTransfer);
    INIT_SMMCALL(Ohci2AsyncIsochronousTransfer);
    INIT_SMMCALL(Ohci2GetRootHubPortStatus);
    INIT_SMMCALL(Ohci2SetRootHubPortFeature);
    INIT_SMMCALL(Ohci2ClearRootHubPortFeature);

    INIT_SMMCALL(GetUhcInfo);
    INIT_SMMCALL(UhciReset);
    INIT_SMMCALL(UhciGetState);
    INIT_SMMCALL(UhciSetState);
    INIT_SMMCALL(UhciControlTransfer);
    INIT_SMMCALL(UhciBulkTransfer);
    INIT_SMMCALL(UhciAsyncInterruptTransfer);
    INIT_SMMCALL(UhciSyncInterruptTransfer);
    INIT_SMMCALL(UhciIsochronousTransfer);
    INIT_SMMCALL(UhciAsyncIsochronousTransfer);
    INIT_SMMCALL(UhciGetRootHubPortNumber);
    INIT_SMMCALL(UhciGetRootHubPortStatus);
    INIT_SMMCALL(UhciSetRootHubPortFeature);
    INIT_SMMCALL(UhciClearRootHubPortFeature);

    INIT_SMMCALL(Uhci2GetCapability);
    INIT_SMMCALL(Uhci2Reset);
    INIT_SMMCALL(Uhci2GetState);
    INIT_SMMCALL(Uhci2SetState);
    INIT_SMMCALL(Uhci2ControlTransfer);
    INIT_SMMCALL(Uhci2BulkTransfer);
    INIT_SMMCALL(Uhci2AsyncInterruptTransfer);
    INIT_SMMCALL(Uhci2SyncInterruptTransfer);
    INIT_SMMCALL(Uhci2IsochronousTransfer);
    INIT_SMMCALL(Uhci2AsyncIsochronousTransfer);
    INIT_SMMCALL(Uhci2GetRootHubPortStatus);
    INIT_SMMCALL(Uhci2SetRootHubPortFeature);
    INIT_SMMCALL(Uhci2ClearRootHubPortFeature);

    INIT_SMMCALL(GetEhcInfo);
    INIT_SMMCALL(EhcGetCapability);
    INIT_SMMCALL(EhcReset);
    INIT_SMMCALL(EhcGetState);
    INIT_SMMCALL(EhcSetState);
    INIT_SMMCALL(EhcControlTransfer);
    INIT_SMMCALL(EhcBulkTransfer);
    INIT_SMMCALL(EhcAsyncInterruptTransfer);
    INIT_SMMCALL(EhcSyncInterruptTransfer);
    INIT_SMMCALL(EhcIsochronousTransfer);
    INIT_SMMCALL(EhcAsyncIsochronousTransfer);
    INIT_SMMCALL(EhcGetRootHubPortStatus);
    INIT_SMMCALL(EhcSetRootHubPortFeature);
    INIT_SMMCALL(EhcClearRootHubPortFeature);

    INIT_SMMCALL(GetXhcInfo);
    INIT_SMMCALL(XhcGetCapability);
    INIT_SMMCALL(XhcReset);
    INIT_SMMCALL(XhcGetState);
    INIT_SMMCALL(XhcSetState);
    INIT_SMMCALL(XhcControlTransfer);
    INIT_SMMCALL(XhcBulkTransfer);
    INIT_SMMCALL(XhcAsyncInterruptTransfer);
    INIT_SMMCALL(XhcSyncInterruptTransfer);
    INIT_SMMCALL(XhcIsochronousTransfer);
    INIT_SMMCALL(XhcAsyncIsochronousTransfer);
    INIT_SMMCALL(XhcGetRootHubPortStatus);
    INIT_SMMCALL(XhcSetRootHubPortFeature);
    INIT_SMMCALL(XhcClearRootHubPortFeature);

    INIT_SMMCALL(ScanUsbBus);
    INIT_SMMCALL(UsbIoControlTransfer);
    INIT_SMMCALL(UsbIoBulkTransfer);
    INIT_SMMCALL(UsbIoAsyncInterruptTransfer);
    INIT_SMMCALL(UsbIoSyncInterruptTransfer);
    INIT_SMMCALL(UsbIoIsochronousTransfer);
    INIT_SMMCALL(UsbIoAsyncIsochronousTransfer);
    INIT_SMMCALL(UsbIoGetDeviceDescriptor);
    INIT_SMMCALL(UsbIoGetActiveConfigDescriptor);
    INIT_SMMCALL(UsbIoGetInterfaceDescriptor);
    INIT_SMMCALL(UsbIoGetEndpointDescriptor);
    INIT_SMMCALL(UsbIoGetStringDescriptor);
    INIT_SMMCALL(UsbIoGetSupportedLanguages);
    INIT_SMMCALL(UsbIoPortReset);
}

EFI_STATUS
USBLegacyDisable(
    IN  EFI_HANDLE                    DispatchHandle,
    IN CONST VOID                     *DispatchContext,
    IN OUT VOID                       *CommBuffer,
    IN OUT UINTN                      *CommBufferSize
)
{
    EndLegacyUsb();
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LegacyUsbEntryPoint (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
)
{
    EFI_STATUS                         Status;
    EFI_SMM_SW_DISPATCH2_PROTOCOL      *SwDispatch;
    EFI_SMM_SW_REGISTER_CONTEXT        SwContext;
    EFI_HANDLE                         SwHandle;
    EFI_LEGACY_USB_INF_PROTOCOL        *LegacyUsbInf;
    EFI_PHYSICAL_ADDRESS               MemAddress;
    EFI_SWSMI_VALUE_POLICY_PROTOCOL    *SwSmiPolicy;
    EFI_HANDLE                         SwSmiValuePolicyHandle = NULL;

    //
    // Get SMM CPU protocol
    //
    Status = gSmst->SmmLocateProtocol (
                      &gEfiSmmCpuProtocolGuid,
                      NULL,
                      (VOID **)&mSmmCpu
                      );
    ASSERT_EFI_ERROR (Status);

    //
    // Get EFI SMM Base Helper Protocol
    //
    Status = gBS->LocateProtocol (
                 &gEfiSmmBaseHelperReadyProtocolGuid,
                 NULL,
                 (VOID **)&mSmmBaseHelperReady
             );
    if (!EFI_ERROR (Status)) {
        mFrameworkSmst = mSmmBaseHelperReady->FrameworkSmst;
    }

    Status = gBS->AllocatePool (
                 EfiBootServicesData,
                 sizeof (EFI_LEGACY_USB_INF_PROTOCOL),
                 &LegacyUsbInf
             );
    if (EFI_ERROR (Status)) {
        return EFI_OUT_OF_RESOURCES;
    }

    Status = gBS->AllocatePages (
                     AllocateAnyPages,
                     EfiReservedMemoryType,
                     1,
                     &MemAddress
                     );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    } else {
      DEBUG((EFI_D_ERROR, "USB.SmmCallCommBuf:%lX, 1\n", MemAddress));
    }		

    LegacyUsbInf->SmmCallCommBuf = (USB_SMMCALL_COMM *)(UINTN)MemAddress;

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mLegacyUsbHandle,
                    &gEfiLegacyUsbInfProtocolGuid,
                    LegacyUsbInf,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
    SwHandle = NULL;
    Status = gSmst->SmmInstallProtocolInterface (
                      &SwHandle,
                      &gEfiLegacyUsbInfProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      LegacyUsbInf
                      );
    SwHandle = NULL;

    ConstructSmmCallTable();
    mSmmCallComm     = LegacyUsbInf->SmmCallCommBuf;
    mSmmCallTablePtr = LegacyUsbInf->SmmCallTablePtr = UsbSmmCallTable;

    Status = gSmst->SmmLocateProtocol(&gEfiSmmSwDispatch2ProtocolGuid, NULL, &SwDispatch);
    if (EFI_ERROR(Status)) {
        return EFI_UNSUPPORTED;
    }

    SwContext.SwSmiInputValue = SMMCALL_SMI_VALUE;
    Status = SwDispatch->Register (
                 SwDispatch,
                 DoSmmCallEntry,
                 &SwContext,
                 &SwHandle
             );
    if (EFI_ERROR(Status)) {
        return EFI_UNSUPPORTED;
    }

    SwContext.SwSmiInputValue = USBSyncLED_SMI;
    Status = SwDispatch->Register (
                 SwDispatch,
                 SyncLedState,
                 &SwContext,
                 &SwHandle
             );
    if (EFI_ERROR(Status)) {
        return EFI_UNSUPPORTED;
    }

    SwContext.SwSmiInputValue = CSM16_SMMCALL_SMI_VALUE;
    Status = SwDispatch->Register (
                 SwDispatch,
                 DoCsm16SmmCall,
                 &SwContext,
                 &SwHandle
             );
    if (EFI_ERROR(Status)) {
        return EFI_UNSUPPORTED;
    }

    Status = gBS->LocateProtocol (&gSwSmiValuePolicyGuid, NULL, &SwSmiPolicy);
    if (EFI_ERROR(Status)) {
        Status = gBS->AllocatePool (
                     EfiBootServicesData,
                     sizeof (EFI_SWSMI_VALUE_POLICY_PROTOCOL),
                     &SwSmiPolicy
                 );
        if (EFI_ERROR (Status)) {
            return EFI_OUT_OF_RESOURCES;
        }
        ZeroMem (SwSmiPolicy, sizeof (EFI_SWSMI_VALUE_POLICY_PROTOCOL));
        SwSmiPolicy->NumSmmEntries = 1;       // first node
        SwSmiPolicy->SwSmiEntry[0].Type = 1;  // for USB use
        SwSmiPolicy->SwSmiEntry[0].Value = SwContext.SwSmiInputValue;
        Status = gBS->InstallProtocolInterface (
                     &SwSmiValuePolicyHandle,
                     &gSwSmiValuePolicyGuid,
                     EFI_NATIVE_INTERFACE,
                     (VOID *) SwSmiPolicy
                 );
    } else {
        SwSmiPolicy->SwSmiEntry[SwSmiPolicy->NumSmmEntries].Type = 1; // for USB use
        SwSmiPolicy->SwSmiEntry[SwSmiPolicy->NumSmmEntries].Value = SwContext.SwSmiInputValue;
        SwSmiPolicy->NumSmmEntries ++;
    }

    DEBUG((EFI_D_ERROR, "LegacyUsbEntry Exit\n"));

    return EFI_SUCCESS;
}



