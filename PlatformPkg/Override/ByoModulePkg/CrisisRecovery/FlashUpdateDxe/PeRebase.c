
#include <Uefi.h>
#include <Pi/PiFirmwareVolume.h>
#include <Pi/PiFirmwareFile.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/FirmwareFileSystem3.h>
#include <IndustryStandard/PeImage.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>

#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
  ((ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1)))






EFI_STATUS
RebasePeImage (
  IN  OUT UINT8   *PeImage,
  IN      VOID    *FvBBImage,
  IN      UINT32  NewFvBase
)
{
  EFI_STATUS                            Status;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;

  ZeroMem(&ImageContext, sizeof(ImageContext));
  ImageContext.Handle    = PeImage;
  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

  Status = PeCoffLoaderGetImageInfo(&ImageContext);
  if (EFI_ERROR (Status)) {goto ProcExit;}  
  
  ImageContext.DestinationAddress = (UINTN)PeImage - (UINTN)FvBBImage + NewFvBase;
  ImageContext.ImageAddress = (UINTN)PeImage;
  DEBUG((EFI_D_INFO, "%X ->%X\n", ImageContext.ImageAddress, ImageContext.DestinationAddress));  
  Status = PeCoffLoaderRelocateImage(&ImageContext);
  DEBUG((EFI_D_INFO, "PeCoffLoaderRelocateImage:%r, Err:%X\n", Status, ImageContext.ImageError));

ProcExit:	
  return Status;
}






  
EFI_FFS_FILE_STATE
GetFileState(
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
{
  EFI_FFS_FILE_STATE  FileState;
  EFI_FFS_FILE_STATE  HighestBit;

  FileState = FfsHeader->State;

  if (ErasePolarity != 0) {
    FileState = (EFI_FFS_FILE_STATE)~FileState;
  }
  
  //
  // Get file state set by its highest none zero bit.
  //
  HighestBit = 0x80;
  while (HighestBit != 0 && (HighestBit & FileState) == 0) {
    HighestBit >>= 1;
  }

  return HighestBit;
} 



// Assume Ffs header checksum is OK.  
// Assume Ffs Data checksum is OK.
// Assume Peim in FV_IMAGE will run on the memory, no need rebase    
// Assmue only PEIM needs rebase.
EFI_STATUS
ReBaseFvPeims (
  IN  VOID    *FvBBImage,
  IN  UINT32  NewFvBase
  )
{
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  EFI_STATUS                            Status = EFI_SUCCESS;
  EFI_FFS_FILE_HEADER                   *FfsFileHeader;
  UINT32                                FileLength;
  UINT32                                FileOccupiedSize;
  UINT32                                FileOffset;
  UINT64                                FvLength;
  UINT8                                 ErasePolarity;
  UINT8                                 FileState;
  UINT8                                 DataCheckSum;
  BOOLEAN                               IsFfs3Fv;
  EFI_COMMON_SECTION_HEADER             *Section; 
  UINT32                                SectionSize; 
  UINT32                                ParsedLength;
  UINT32                                SectionLength; 
  UINT8                                 *PeImage;
  CHAR16                                *UiName;

  
  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER*)FvBBImage;
  IsFfs3Fv = CompareGuid (&FwVolHeader->FileSystemGuid, &gEfiFirmwareFileSystem3Guid);
  FvLength = FwVolHeader->FvLength;
  if ((FwVolHeader->Attributes & EFI_FVB2_ERASE_POLARITY) != 0) {
    ErasePolarity = 1;
  } else {
    ErasePolarity = 0;
  }
  FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FwVolHeader + FwVolHeader->HeaderLength);
  FileOffset = (UINT32) ((UINT8 *)FfsFileHeader - (UINT8 *)FwVolHeader);
  ASSERT (FileOffset <= 0xFFFFFFFF);

  while (FileOffset < (FvLength - sizeof (EFI_FFS_FILE_HEADER))) {
    FileState = GetFileState (ErasePolarity, FfsFileHeader);
    
    switch (FileState) {

      case EFI_FILE_HEADER_CONSTRUCTION:
      case EFI_FILE_HEADER_INVALID:
        if (IS_FFS_FILE2 (FfsFileHeader)) {
          FileOffset    += sizeof (EFI_FFS_FILE_HEADER2);
          FfsFileHeader =  (EFI_FFS_FILE_HEADER *) ((UINT8 *) FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER2));
        } else {
          FileOffset    += sizeof (EFI_FFS_FILE_HEADER);
          FfsFileHeader =  (EFI_FFS_FILE_HEADER *) ((UINT8 *) FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER));
        }
        break;

      case EFI_FILE_DELETED:
        if (IS_FFS_FILE2 (FfsFileHeader)) {
          FileLength = FFS_FILE2_SIZE (FfsFileHeader);
          ASSERT (FileLength > 0x00FFFFFF);
        } else {
          FileLength = FFS_FILE_SIZE (FfsFileHeader);
        }
        FileOccupiedSize = GET_OCCUPIED_SIZE(FileLength, 8);
        FileOffset      += FileOccupiedSize;
        FfsFileHeader    = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
        break;

      default:
        goto ProcExit;
        break;
        
      case EFI_FILE_DATA_VALID:
      case EFI_FILE_MARKED_FOR_UPDATE:
        if (IS_FFS_FILE2 (FfsFileHeader)) {
          FileLength = FFS_FILE2_SIZE (FfsFileHeader);
          ASSERT (FileLength > 0x00FFFFFF);
          FileOccupiedSize = GET_OCCUPIED_SIZE (FileLength, 8);
          if (!IsFfs3Fv) {
            FileOffset += FileOccupiedSize;
            FfsFileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) FfsFileHeader + FileOccupiedSize);
            break;
          }
        } else {
          FileLength = FFS_FILE_SIZE (FfsFileHeader);
          FileOccupiedSize = GET_OCCUPIED_SIZE (FileLength, 8);
        }
   
        if(FfsFileHeader->Type == EFI_FV_FILETYPE_PEIM || FfsFileHeader->Type == EFI_FV_FILETYPE_PEI_CORE){
          if (IS_FFS_FILE2 (FfsFileHeader)) {
            ASSERT (FFS_FILE2_SIZE (FfsFileHeader) > 0x00FFFFFF);
            Section = (EFI_COMMON_SECTION_HEADER *) ((UINT8 *) FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER2));
            SectionSize = FFS_FILE2_SIZE (FfsFileHeader) - sizeof (EFI_FFS_FILE_HEADER2);
          } else {
            Section = (EFI_COMMON_SECTION_HEADER *) ((UINT8 *) FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER));
            SectionSize = FFS_FILE_SIZE (FfsFileHeader) - sizeof (EFI_FFS_FILE_HEADER);
          }
          
          ParsedLength = 0;
          while (ParsedLength < SectionSize) {
            if (Section->Type == EFI_SECTION_PE32) {
              if (IS_SECTION2 (Section)) {
                PeImage = ((UINT8 *) Section + sizeof (EFI_COMMON_SECTION_HEADER2));
              } else {
                PeImage = ((UINT8 *) Section + sizeof (EFI_COMMON_SECTION_HEADER));
              }
              Status = RebasePeImage(PeImage, FvBBImage, NewFvBase);
              if(EFI_ERROR(Status)){goto ProcExit;}
            } else if(Section->Type == EFI_SECTION_USER_INTERFACE){
              if (IS_SECTION2 (Section)) {
                UiName = (CHAR16*)((UINTN)Section + sizeof (EFI_COMMON_SECTION_HEADER2));
              } else {
                UiName = (CHAR16*)((UINTN)Section + sizeof (EFI_COMMON_SECTION_HEADER));
              }            
              DEBUG((EFI_D_INFO, "UI:%s\n", UiName));
            }
            if (IS_SECTION2 (Section)) {
              SectionLength = SECTION2_SIZE (Section);
            } else {
              SectionLength = SECTION_SIZE (Section);
            } 
            SectionLength = GET_OCCUPIED_SIZE (SectionLength, 4);
            ParsedLength += SectionLength;
            Section = (EFI_COMMON_SECTION_HEADER *)((UINT8 *)Section + SectionLength);            
          }
        }

        DataCheckSum = FFS_FIXED_CHECKSUM;
        if ((FfsFileHeader->Attributes & FFS_ATTRIB_CHECKSUM) == FFS_ATTRIB_CHECKSUM) {
          FfsFileHeader->IntegrityCheck.Checksum.File = 0;
          if (IS_FFS_FILE2 (FfsFileHeader)) {
            DataCheckSum = CalculateCheckSum8 ((CONST UINT8 *) FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER2), FileLength - sizeof(EFI_FFS_FILE_HEADER2));
          } else {
            DataCheckSum = CalculateCheckSum8 ((CONST UINT8 *) FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER), FileLength - sizeof(EFI_FFS_FILE_HEADER));
          }
        }
        FfsFileHeader->IntegrityCheck.Checksum.File = DataCheckSum;
        
        FileOffset   += FileOccupiedSize; 
        FfsFileHeader =  (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
        break;

    } 
  }
  
ProcExit:
  return Status; 
}









