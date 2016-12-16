/** @file
  This is a simple shell application

  Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>


VOID
myEventNotify(
  IN EFI_EVENT    Event,
  IN VOID        *Context
  )
{
	static  UINTN   times = 0;
    Print(L"%s\n myEventNotify signal %d\n",Context, times);
    DEBUG((EFI_D_ERROR,"%s\nmyEventNotify signal %d\n",Context, times));
    times++;
}


/**
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS   Status;
  EFI_EVENT    myEvent;
  UINT8        i=0;

  Status = gBS->CreateEvent(EVT_TIMER|EVT_NOTIFY_SIGNAL, TPL_CALLBACK,(EFI_EVENT_NOTIFY)myEventNotify, (VOID*)L"Hello, Time out!", &myEvent);
  Status = gBS->SetTimer(myEvent, TimerPeriodic, 1*1000*1000); // Do timer counter until Close this event.
  while(i<1000000000000)
  {
  	Print(L"Out here!\n");
    DEBUG((EFI_D_ERROR,"Out here!\n"));
    i++;
  }
  Status = gBS->CloseEvent(myEvent);
  return EFI_SUCCESS;
}
