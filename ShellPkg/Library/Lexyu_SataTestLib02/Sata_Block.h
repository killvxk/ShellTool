#include "Sata_test_CommandsLib.h"
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
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
VOID Display_Percent(UINT64 Lba,UINT32 i,UINT64 Count,UINT8 Read);





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







VOID Display_Percent(UINT64 Lba,UINT32 i,UINT64 Count,UINT8 Read)
{
			UINT64 n = (i+1) * 100ULL;
			INT32 percent;	
			percent=(INT32)(n/Count);
			if(Read)
			{
				DEBUG((EFI_D_ERROR,"\rReading at 0x%llx -- %3d%% complete.",
					   (Lba+i), percent));
			}
			else
			{
				DEBUG((EFI_D_ERROR,"\rWriting at 0x%llx -- %3d%% complete.",
					   (Lba+i), percent));
			}

}

