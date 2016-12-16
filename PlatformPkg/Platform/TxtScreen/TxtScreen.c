

#include <uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/PciIo.h>
#include <PlatformDefinition.h>
#include <SetupVariable.h>
#include <Library/PlatformCommLib.h>



STATIC BOOLEAN IsOnBoardLanPresent()
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_HANDLE                Handle;
	EFI_PCI_IO_PROTOCOL       *PciIo; 
  UINT8                     ClassCode[3];
  BOOLEAN                   Rc = FALSE;

  
  DevPath = (EFI_DEVICE_PATH_PROTOCOL*)gObLanDp;
  Status  = gBS->LocateDevicePath(&gEfiPciIoProtocolGuid, &DevPath, &Handle);
  if(EFI_ERROR(Status)){goto ProcExit;}  
  
  Status = gBS->HandleProtocol(
                  Handle,
                  &gEfiPciIoProtocolGuid, 
                  &PciIo
                  );
  ASSERT(!EFI_ERROR(Status));

  Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, 9, sizeof(ClassCode), ClassCode);
  if(ClassCode[2] != 2){
    goto ProcExit;
  }
 
  Rc = TRUE;
  
ProcExit:
  return Rc;
}


STATIC EFI_INPUT_KEY GetInputKey()
{
	EFI_INPUT_KEY  Key;
	UINTN          Index;
	
// flush keyboard buffer
	while(gST->ConIn->ReadKeyStroke(gST->ConIn,&Key) == EFI_SUCCESS){};
	
	gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &Index);
	gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
  
	return Key;
}









EFI_STATUS
EFIAPI
TxtScreenEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_INPUT_KEY  Key;  
  BOOLEAN        CursorVisible;
  
  if(!IsOnBoardLanPresent()){
    BltSaveAndRetore(gBS, TRUE);
    CursorVisible = gST->ConOut->Mode->CursorVisible;
    gST->ConOut->ClearScreen(gST->ConOut);
    gST->ConOut->EnableCursor(gST->ConOut, FALSE);
    Print(L"\n\n");
    Print(L"--------------------------------------------------------\n");
    Print(L"[ERROR] Onboard Lan is absent! Press [F1] to continue...\n");
    Print(L"--------------------------------------------------------\n");    
    while(1){
      Key = GetInputKey();
      if(Key.ScanCode == SCAN_F1){
        break;
      }  
    }
    gST->ConOut->EnableCursor(gST->ConOut, CursorVisible);
    BltSaveAndRetore(gBS, FALSE);    
  }

  return EFI_SUCCESS;
}

