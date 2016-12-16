
#include "PlatformDxe.h"
#include <Guid/GlobalVariable.h>


UINTN
EFIAPI
SerialPortWrite (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
);


VOID
PlatformDebugAtEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{

#ifndef MDEPKG_NDEBUG	
  SerialPortWrite("Flush", 0);  // below will fill registers related to COM
#endif 

}


