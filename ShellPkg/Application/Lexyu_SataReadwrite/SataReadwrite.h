#include <Uefi.h>
#include <ShellBase.h>

#include <Guid/GlobalVariable.h>
#include <Guid/ConsoleInDevice.h>
#include <Guid/ConsoleOutDevice.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/ShellLibHiiGuid.h>


#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellParameters.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DriverDiagnostics2.h>
#include <Protocol/DriverDiagnostics.h>
#include <Protocol/PlatformDriverOverride.h>
#include <Protocol/BusSpecificDriverOverride.h>
#include <Protocol/PlatformToDriverConfiguration.h>
#include <Protocol/DriverSupportedEfiVersion.h>
#include <Protocol/DriverFamilyOverride.h>
#include <Protocol/DriverHealth.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/PciRootBridgeIo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/SortLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/FileHandleLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/HandleParsingLib.h>

#define EFI_DISK_INFO_PROTOCOL_GUID \
	  { \
		0xd432a67f, 0x14dc, 0x484b, {0xb3, 0xbb, 0x3f, 0x2, 0x91, 0x84, 0x93, 0x27 } \
	  }
EFI_GUID gEfiDiskInfoProtocolGuid = EFI_DISK_INFO_PROTOCOL_GUID;

VOID *
my_memset (
  OUT VOID    *Dest,
  IN  UINTN   Char,
  IN  UINTN   Count
  );
VOID *
my_memcpy (
  OUT VOID        *Dest,
  IN  const VOID  *Src,
  IN  UINTN       Count
  );
VOID Display_Percent(UINT64 Lba,UINT32 i,UINT8 Read,UINT8 Status);
VOID Display_Percent2(UINT64 Lba,UINT32 i,UINT64 Count);





VOID *
my_memset (
  OUT VOID    *Dest,
  IN  UINTN   Char,
  IN  UINTN   Count
  )
{
  volatile UINT8  *Ptr;
  
  for (Ptr = Dest; Count > 0; Count--, Ptr++) {
    *Ptr = (UINT8) Char;
  }

  return Dest;
}







VOID *
my_memcpy (
  OUT VOID        *Dest,
  IN  const VOID  *Src,
  IN  UINTN       Count
  )
{
  volatile UINT8  *Ptr;
  const    UINT8  *Source;
  
  for (Ptr = Dest, Source = Src; Count > 0; Count--, Source++, Ptr++) {
    *Ptr = *Source;
  }

  return Dest;
}







VOID Display_Percent(UINT64 Lba,UINT32 i,UINT8 Read,UINT8 Status)
{
	if(Status==0)
    {
		if(Read==0)
		{
			DEBUG((EFI_D_ERROR,"\rReading at 0x%llx Success\n",(Lba+i)));
		}
		else
		{
			DEBUG((EFI_D_ERROR,"\rWriting at 0x%llx Success\n",(Lba+i)));
		}
    }
    else
    {
		if(Read==0)
		{
			DEBUG((EFI_D_ERROR,"\rReading at 0x%llx Error\n",(Lba+i)));
		}
		else
		{
			DEBUG((EFI_D_ERROR,"\rWriting at 0x%llx Error\n",(Lba+i)));
		}
    }
}


VOID Display_Percent2(UINT64 Lba,UINT32 i,UINT64 Count)
{
	UINT64 n = (i+1) * 100ULL;
	INT32 percent;	
	percent=(INT32)(n/Count);
	DEBUG((EFI_D_ERROR,"\rTotal Read/Write -- %3d%% Complete\n.",percent));
}


