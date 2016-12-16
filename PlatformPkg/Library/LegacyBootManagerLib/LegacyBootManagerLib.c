
#include <PiDxe.h>
#include <Pi/PiBootMode.h>      // HobLib.h +
#include <Pi/PiHob.h>           // HobLib.h +
#include <Guid/LegacyDevOrder.h>
#include <Guid/GlobalVariable.h>
#include <IndustryStandard/Pci.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciIo.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/PerformanceLib.h>
#include <Library/HobLib.h>
#include <IndustryStandard/Atapi.h>
#include <Guid/MemoryTypeInformation.h>


#pragma pack(1)
typedef struct {
  UINT16     BbsIndex;
} LEGACY_BOOT_OPTION_BBS_DATA;
#pragma pack()


VOID
EFIAPI
LegacyBoot (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION           *BootOption
  );

VOID
EFIAPI
LegacyBootManagerRefreshAllBootOption (
  VOID
  );



#define  BOOT_DESCRIPTION_LENGTH  32

// {FF635C81-A114-4bfc-B342-3EED05A29E06}
EFI_GUID gLastBootOrderVariableGuid = { 0xff635c81, 0xa114, 0x4bfc, { 0xb3, 0x42, 0x3e, 0xed, 0x5, 0xa2, 0x9e, 0x6 } };

VOID DebugDempMem8(VOID *Address, UINTN Size)
{
  UINT8  *Data8;
  UINTN  Index;
  
  Data8 = (UINT8*)Address;
  for(Index=0; Index<Size; Index++){
    DEBUG((EFI_D_INFO, "%02X ", Data8[Index]));
    if(((Index+1)%16)==0){
      DEBUG((EFI_D_INFO, "\n"));
    }
  }
  DEBUG((EFI_D_INFO, "\n"));
}


EFI_STATUS
EFIAPI
LegacyBootManagerConstructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
)
{
  EfiBootManagerRegisterLegacyBootSupport (
    LegacyBootManagerRefreshAllBootOption,
    LegacyBoot
    );
  return EFI_SUCCESS;
}

UINT16
LegacyDeviceType (
  EFI_DEVICE_PATH_PROTOCOL *DevicePath
  )
{
  ASSERT ((DevicePathType (DevicePath) == BBS_DEVICE_PATH) &&
          (DevicePathSubType (DevicePath) == BBS_BBS_DP));
  return ((BBS_BBS_DEVICE_PATH *) DevicePath)->DeviceType;
}

UINT16
GetDevcieTypeInBBS (
  IN CONST BBS_TABLE *BbsEntry
)
{
  UINT16  DevType;

  DevType = BbsEntry->DeviceType;
  if(BbsEntry->DeviceType == BBS_BEV_DEVICE){
    if(BbsEntry->Class == PCI_CLASS_MASS_STORAGE){
      DevType = BBS_CDROM;
    }else if(BbsEntry->Class == PCI_CLASS_NETWORK){
      DevType = BBS_EMBED_NETWORK;
    }
  }

  return DevType;
}

BOOLEAN
ValidBbsEntry (
  IN BBS_TABLE   *BbsEntry
  )
{
  switch (BbsEntry->BootPriority) {
    case BBS_IGNORE_ENTRY:
    case BBS_DO_NOT_BOOT_FROM:
    case BBS_LOWEST_PRIORITY:
      return FALSE;
    default:
      return TRUE;
  }
}


STATIC CHAR16 *TrimStr16(CHAR16 *Str)
{
  UINTN    StringLength;
  CHAR16   *NewString;
  BOOLEAN  HasLeading;
  UINTN    SrcIndex;
  UINTN    TarIndex;
  
  NewString = NULL;

  StringLength = StrLen(Str);
  if(StringLength==0){
    return Str; 
  }

  NewString = AllocatePool((StringLength + 1)*sizeof(Str[0]));
  if(NewString==NULL){
    return Str; 
  }

	HasLeading = TRUE;
	TarIndex   = 0;
	SrcIndex   = 0;
	for(;SrcIndex<StringLength;SrcIndex++){
		if(HasLeading){
			if(Str[SrcIndex] == L' '){
				continue;
			}else{
				HasLeading = FALSE;
			}
		}
		if(TarIndex > 0){
			if(Str[SrcIndex-1] == Str[SrcIndex] && Str[SrcIndex] == L' '){
				continue;
			}
		}
		NewString[TarIndex++] = Str[SrcIndex];
	}

  if(TarIndex && NewString[TarIndex-1] == L' '){
    TarIndex--;
  }
  NewString[TarIndex] = 0;
  ASSERT(TarIndex <= StringLength);
  CopyMem(Str, NewString, (TarIndex+1)*sizeof(Str[0]));
  FreePool(NewString);
  return Str;
}





VOID
InvokeGetBbsInfo (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  UINT16                    HddCount;
  HDD_INFO                  *HddInfo;
  UINT16                    BbsCount;
  BBS_TABLE                 *BbsTable;

  Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  &LegacyBios
                  );
  if (!EFI_ERROR(Status)) {                
    Status = LegacyBios->GetBbsInfo (
                          LegacyBios,
                          &HddCount,
                          &HddInfo,
                          &BbsCount,
                          &BbsTable
                          );
  }
}




/**
  Build Legacy Device Name String according.

  @param CurBBSEntry     BBS Table.
  @param Index           Index.
  @param BufSize         The buffer size.
  @param BootString      The output string.

**/
VOID
BuildLegacyDevNameString (
  IN  BBS_TABLE                 *CurBBSEntry,
  IN  UINTN                     Index,
  IN  UINTN                     BufSize,
  OUT CHAR16                    *BootString
  )
{
  CHAR8                     *StringDesc;
  CHAR8                     *Type;	
  UINTN                     CharIndex;	
  UINTN                     Count;	


// BbsTable[0]     - Floppy
// BbsTable[16:1]  - IDE
// BbsTable[??:17] - BCV/BEV

  switch(GetDevcieTypeInBBS(CurBBSEntry)) {

    case BBS_HARDDISK:
    case BBS_CDROM:
      Type = "SATA";
      break;

    case BBS_USB:
      Type = "USB";
      break;

    case BBS_EMBED_NETWORK:
      Type = "NET";
      break;

    case BBS_BEV_DEVICE:
      Type = "BEV";
      break;

    case BBS_UNKNOWN:
    default:
      Type = "?";
      break;
  }

  if((CurBBSEntry->Class ==PCI_CLASS_SERIAL) && (CurBBSEntry->SubClass == PCI_CLASS_SERIAL_USB)){
    Type = "USB";
  }
  if(CurBBSEntry->Class == 8 && CurBBSEntry->SubClass == 5){ // SD device
    Type = "SD";
  }

  StringDesc = (UINT8*)(UINTN)((CurBBSEntry->DescStringSegment << 4) + CurBBSEntry->DescStringOffset);
  if (StringDesc != NULL) {
    Count = BufSize;		
    if(Count > BOOT_DESCRIPTION_LENGTH){Count = BOOT_DESCRIPTION_LENGTH;}
		if(AsciiStrnCmp(StringDesc, Type, AsciiStrLen(Type))){     // Prefix bad
		  AsciiStrToUnicodeStr(Type, BootString);
      CharIndex = AsciiStrLen(Type);
      BootString[CharIndex]   = L':';
      BootString[CharIndex+1] = L' ';
      CharIndex += 2;
      for(;CharIndex < Count; CharIndex++){
				BootString[CharIndex] = (CHAR16)*(StringDesc++);
      }				
      BootString[CharIndex] = 0;
    } else {
      for(CharIndex=0; CharIndex < Count; CharIndex++){
				BootString[CharIndex] = (CHAR16)*(StringDesc++);
      }				
      BootString[CharIndex] = 0;		
    }
    goto ProcExit;
  }

  AsciiStrToUnicodeStr(Type, BootString);

ProcExit:
  TrimStr16(BootString);
}


UINT16
FuzzyMatch (
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption,
  BBS_TABLE                     *BbsTable,
  UINT16                        BbsCount
  )
{
  UINT16                        Index;
  LEGACY_BOOT_OPTION_BBS_DATA   *BbsData;
  UINT16                        BootPriority;	
  CHAR16                        Description[BOOT_DESCRIPTION_LENGTH + 1];


  BbsData = (LEGACY_BOOT_OPTION_BBS_DATA *) BootOption->OptionalData;

  //
  // Directly check the BBS index stored in BootOption
  //
  if ((BbsData->BbsIndex < BbsCount) &&
      (LegacyDeviceType (BootOption->FilePath) == GetDevcieTypeInBBS(&BbsTable[BbsData->BbsIndex]))) {
    BuildLegacyDevNameString (
      &BbsTable[BbsData->BbsIndex],
      BbsData->BbsIndex,
      sizeof (Description),
      Description
      );
    if (StrCmp (Description, BootOption->Description) == 0) {
      return BbsData->BbsIndex;
    }
  }

  //
  // BBS table could be changed (entry removed/moved)
  // find the correct BBS index
  //
  for(Index=0;Index<BbsCount;Index++){
  	BootPriority = BbsTable[Index].BootPriority;
  	if(BootPriority == BBS_IGNORE_ENTRY || BootPriority == BBS_DO_NOT_BOOT_FROM){
  	  continue;
  	}
    if(LegacyDeviceType(BootOption->FilePath) != GetDevcieTypeInBBS(&BbsTable[Index])){
      continue;
    }
    BuildLegacyDevNameString (
      &BbsTable[Index],
      Index,
      sizeof(Description),
      Description
      );
    if (StrCmp (Description, BootOption->Description) == 0) {
      return Index;
    }		
		
  }
  
  return BbsCount;
}

/*
VOID
UpdateBbsIndex (
  LEGACY_DEV_ORDER_ENTRY   *LegacyDevOrder,
  UINTN                    LegacyDevOrderSize,
  UINT16                   DeviceType,
  UINT16                   OldBbsIndex,
  UINT16                   NewBbsIndex
  )
{
  LEGACY_DEV_ORDER_ENTRY   *Entry;
  UINTN                    Index;

  for (Entry = LegacyDevOrder; 
       Entry < (LEGACY_DEV_ORDER_ENTRY *) ((UINT8 *) LegacyDevOrder + LegacyDevOrderSize);
       Entry = (LEGACY_DEV_ORDER_ENTRY *) ((UINT8 *) Entry + sizeof (BBS_TYPE) + Entry->Length)
       ) {
    if (Entry->BbsType == DeviceType) {
      for (Index = 0; Index < Entry->Length / sizeof (UINT16) - 1; Index++) {
        if (Entry->Data[Index] == OldBbsIndex) {
          Entry->Data[Index] = NewBbsIndex;
          break;
        }
      }
      break;
    }
  }
}
*/


/**
  Delete all the legacy boot options.

  @retval EFI_SUCCESS            All legacy boot options are deleted.
**/
EFI_STATUS
DeleteAllLegacyBootOptions (
  VOID
  )
{
  UINTN                         Index;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  UINTN                         BootOptionCount;
  EFI_STATUS                    Status;	

  BootOption = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
  for (Index = 0; Index < BootOptionCount; Index++) {
    if ((DevicePathType (BootOption[Index].FilePath) == BBS_DEVICE_PATH) &&
        (DevicePathSubType (BootOption[Index].FilePath) == BBS_BBS_DP)) {
      EfiBootManagerDeleteLoadOptionVariable (BootOption[Index].OptionNumber, BootOption[Index].OptionType);
    }
  }

  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &gEfiLegacyDevOrderVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  0,
                  NULL
                  );
//ASSERT(!EFI_ERROR(Status));
	
  return EFI_SUCCESS;
}



// xx xx xx vv xx xx
// 0  1  2  3  4  5
// 0  1  2     3  4

BOOLEAN
RemoveInvalidDataInLegacyDevOrder (
  IN LEGACY_DEV_ORDER_ENTRY        *LegacyDevOrder,
  IN UINTN                         *LegacyDevOrderSize,
  IN UINT16                        InvalidBbsIndex
  )
{
  LEGACY_DEV_ORDER_ENTRY  *Entry;
  UINTN                   DataSize;
  UINTN                   EntrySize;
  UINTN                   Count;
  UINTN                   TotalCount;
  UINTN                   Index;
  UINT8                   *Data8;
  UINT8                   *LastData;
  BOOLEAN                 HasFound;


  DataSize   = *LegacyDevOrderSize;
  Entry      = LegacyDevOrder;
  TotalCount = 0;
  HasFound   = FALSE;

  if(LegacyDevOrder==NULL || LegacyDevOrderSize == NULL || *LegacyDevOrderSize==0){
    goto ProcExit;
  }
	
  while(TotalCount < DataSize){
    Count = (Entry->Length - sizeof(Entry->Length))/sizeof(Entry->Data[0]);
    for(Index=0;Index<Count;Index++){
      if(Entry->Data[Index]==InvalidBbsIndex){
        HasFound = TRUE;
        break;
      }
    }
    if(HasFound){
      Entry->Length       -= sizeof(Entry->Data[0]);
      *LegacyDevOrderSize -= sizeof(Entry->Data[0]);
      Data8    = (UINT8*)&Entry->Data[Index];
      LastData = (UINT8*)LegacyDevOrder + DataSize - 1;
      while(Data8+2 <= LastData){
        *Data8 = *(Data8+2);
        Data8++;
      }
      break;
    }
    EntrySize = sizeof(LEGACY_DEV_ORDER_ENTRY) + (Count-1)*sizeof(Entry->Data[0]);
    TotalCount += EntrySize;
    Entry = (LEGACY_DEV_ORDER_ENTRY*)((UINT8*)Entry + EntrySize);
  }

ProcExit:
	return HasFound;
}

BOOLEAN
UpdateBbsIndexInLegacyDevOrder (
  IN LEGACY_DEV_ORDER_ENTRY        *LegacyDevOrder,
  IN UINTN                         LegacyDevOrderSize,
  IN UINT16                        OldBbsIndex,  
  IN UINT16                        NewBbsIndex
  )
{
  UINTN                   TotalCount;
  UINTN                   DataSize;
  LEGACY_DEV_ORDER_ENTRY  *Entry;  
  UINTN                   Count;
  UINTN                   Index;
  UINTN                   EntrySize;
  UINT16                  *TargetIndex;
  UINT16                  *TargetIndex2;
  UINT16                  Data16;
  BOOLEAN                 OldIndexFound;
  BOOLEAN                 OldIndexFound2;  
  BOOLEAN                 NewIndexFound;  
  
  
  TotalCount     = 0;
  DataSize       = LegacyDevOrderSize;
  Entry          = LegacyDevOrder;
  TargetIndex    = NULL;
  TargetIndex2   = NULL;
  OldIndexFound  = FALSE;
  OldIndexFound2 = FALSE;
  NewIndexFound  = FALSE;
  
  while(TotalCount < DataSize){
    Count = (Entry->Length - sizeof(Entry->Length))/sizeof(Entry->Data[0]);
    
    for(Index=0;Index<Count;Index++){
      Data16 = Entry->Data[Index];
      if(!OldIndexFound && Data16 == OldBbsIndex){
        TargetIndex = &Entry->Data[Index];
        DEBUG((EFI_D_INFO, "TargetIndex:%X\n", Index));
        OldIndexFound = TRUE;
        continue;
      }
      if(!OldIndexFound2 && Data16 == ((OldBbsIndex<<8)|0xFF)){
        TargetIndex2 = &Entry->Data[Index];
        DEBUG((EFI_D_INFO, "TargetIndex2:%X\n", Index));
        OldIndexFound2 = TRUE;
        continue;        
      }
      
      if(!NewIndexFound && Data16 == NewBbsIndex){
// set data replaced to as 0xXXFF, XX is old data.
// UpdateLegacyDevOrder() will igonre bad index data.
        Entry->Data[Index] = (UINT16)((NewBbsIndex<<8)+0xFF);
        NewIndexFound = TRUE;
        DEBUG((EFI_D_INFO, "Update: %04X -> %04X\n", Data16, Entry->Data[Index]));
        continue;
      }
      
      if(NewIndexFound && OldIndexFound && OldIndexFound2){
        goto ProcExit;
      }
    }
 
    EntrySize = sizeof(LEGACY_DEV_ORDER_ENTRY) + (Count-1)*sizeof(Entry->Data[0]);
    TotalCount += EntrySize;
    Entry = (LEGACY_DEV_ORDER_ENTRY*)((UINT8*)Entry + EntrySize);
  }
  
ProcExit:
  if(TargetIndex2 != NULL){
    *TargetIndex2 = NewBbsIndex;
    return TRUE;
  }
  if(TargetIndex != NULL){
    *TargetIndex = NewBbsIndex;
    return TRUE;
  }
  
  return FALSE;
}  


/**
  Delete all the invalid legacy boot options.

  @retval EFI_SUCCESS             All invalide legacy boot options are deleted.
  @retval EFI_OUT_OF_RESOURCES    Fail to allocate necessary memory.
  @retval EFI_NOT_FOUND           Fail to retrive variable of boot order.
**/
EFI_STATUS
DeleteAllInvalidLegacyBootOptions (
  VOID
  )
{
  EFI_STATUS                    Status;
  UINT16                        HddCount;
  UINT16                        BbsCount;
  HDD_INFO                      *HddInfo;
  BBS_TABLE                     *BbsTable;
  UINT16                        BbsIndex;
  UINT16                        OldBbsIndex;
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  UINTN                         Index;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  UINTN                         BootOptionCount;
  LEGACY_DEV_ORDER_ENTRY        *LegacyDevOrder;
  UINTN                         LegacyDevOrderSize;
  BOOLEAN                       VarNeedUpdate;
  EFI_DEVICE_PATH_PROTOCOL      *Dp;


  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));  
  
  LegacyDevOrder = NULL;
  VarNeedUpdate  = FALSE;

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = LegacyBios->GetBbsInfo (
                         LegacyBios,
                         &HddCount,
                         &HddInfo,
                         &BbsCount,
                         &BbsTable
                         );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "(%d)GetBbsInfo:%r\n", __LINE__, Status));
    return Status;
  }

  LegacyDevOrder = EfiBootManagerGetVariableAndSize (
                     VAR_LEGACY_DEV_ORDER,
                     &gEfiLegacyDevOrderVariableGuid,
                     &LegacyDevOrderSize
                     );

  BootOption = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
  for (Index = 0; Index < BootOptionCount; Index++) {
    Dp = BootOption[Index].FilePath;
    if(DevicePathType(Dp) != BBS_DEVICE_PATH || DevicePathSubType(Dp) != BBS_BBS_DP) {
      continue;
    }

    BbsIndex = FuzzyMatch(&BootOption[Index], BbsTable, BbsCount);
    OldBbsIndex = ((LEGACY_BOOT_OPTION_BBS_DATA*)BootOption[Index].OptionalData)->BbsIndex;
    if (BbsIndex == BbsCount) {			// not found
      DEBUG ((EFI_D_INFO, "[LegacyBds] Delete BOOT%04X: %s\n", (UINTN) BootOption[Index].OptionNumber, BootOption[Index].Description));
      EfiBootManagerDeleteLoadOptionVariable (BootOption[Index].OptionNumber, BootOption[Index].OptionType);
      if(RemoveInvalidDataInLegacyDevOrder(LegacyDevOrder, &LegacyDevOrderSize, OldBbsIndex)){
        DEBUG ((EFI_D_INFO, "[LegacyDevOrder] Remove BBS Index %d\n", OldBbsIndex));
        VarNeedUpdate = TRUE;
      }
    } else {
      DEBUG((EFI_D_INFO, "[BBS] 0x%X -> 0x%X\n", OldBbsIndex, BbsIndex));
      if (OldBbsIndex != BbsIndex) {	// Index changed, Update related variable.
        if(UpdateBbsIndexInLegacyDevOrder(LegacyDevOrder, LegacyDevOrderSize, OldBbsIndex, BbsIndex)){
          VarNeedUpdate = TRUE;
        }
        ((LEGACY_BOOT_OPTION_BBS_DATA*)BootOption[Index].OptionalData)->BbsIndex = BbsIndex;
        EfiBootManagerLoadOptionToVariable(&BootOption[Index]);
      }
    }
  }

  EfiBootManagerFreeLoadOptions (BootOption, BootOptionCount);

  if (LegacyDevOrder != NULL) {
    if(VarNeedUpdate){
      Status = gRT->SetVariable (
                      VAR_LEGACY_DEV_ORDER,
                      &gEfiLegacyDevOrderVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      LegacyDevOrderSize,
                      LegacyDevOrder
                      );
    }
    FreePool(LegacyDevOrder);
  }

  return Status;
}

/**
  Find all legacy boot option by device type.

  @param DevType         Device type.
  @param DevName         Device name.
  @param Attribute       The boot option attribute.
  @param BbsIndex        The BBS table index.
  @param OptionNumber    The boot option index.

  @retval TRUE           The Legacy boot option is found.
  @retval FALSE          The legacy boot option is not found.

**/
INTN
FindLegacyBootOption (
  IN EFI_BOOT_MANAGER_LOAD_OPTION  *Key,
  IN EFI_BOOT_MANAGER_LOAD_OPTION  *Array,
  IN UINTN                         Count
  )
{
  UINTN                         Index;
  
  for (Index = 0; Index < Count; Index++) {
    if ((DevicePathType (Array[Index].FilePath) == BBS_DEVICE_PATH) &&
        (DevicePathSubType (Array[Index].FilePath) == BBS_BBS_DP)) {
      //
      // Just compare the FilePath and the Description field
      //
      if ((LegacyDeviceType (Array[Index].FilePath) == LegacyDeviceType (Key->FilePath)) &&
          (StrCmp (Array[Index].Description, Key->Description) == 0)) {
        //
        // The BbsIndex field should match because it was updated in the LegacyBootManagerDeleteAllInvalidBootOptions
        //
        ASSERT (
          ((LEGACY_BOOT_OPTION_BBS_DATA *) Array[Index].OptionalData)->BbsIndex ==
          ((LEGACY_BOOT_OPTION_BBS_DATA *) Key->OptionalData)->BbsIndex
          );
        return (INTN) Index;
      }
    }
  }

  return -1;
}

EFI_STATUS
CreateLegacyBootOption (
  IN OUT EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption,
  IN BBS_TABLE                         *BbsEntry,
  IN UINT16                            BbsIndex
  )
{
  EFI_STATUS                   Status;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath;
  CHAR16                       Description[BOOT_DESCRIPTION_LENGTH + 1];
  CHAR8                        HelpString[BOOT_DESCRIPTION_LENGTH + 1];
  UINTN                        StringLen;
  LEGACY_BOOT_OPTION_BBS_DATA  *OptionalData;
  BBS_BBS_DEVICE_PATH          *BbsNode;

  if ((BootOption == NULL) || (BbsEntry == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  BuildLegacyDevNameString (BbsEntry, BbsIndex, sizeof (Description), Description);

  //
  // Create the BBS device path with description string
  //
  UnicodeStrToAsciiStr (Description, HelpString);
  StringLen = AsciiStrLen (HelpString);
  DevicePath = AllocatePool (sizeof (BBS_BBS_DEVICE_PATH) + StringLen + END_DEVICE_PATH_LENGTH);
  ASSERT (DevicePath != NULL);

  BbsNode = (BBS_BBS_DEVICE_PATH *) DevicePath;
  SetDevicePathNodeLength (BbsNode, sizeof (BBS_BBS_DEVICE_PATH) + StringLen);
  BbsNode->Header.Type    = BBS_DEVICE_PATH;
  BbsNode->Header.SubType = BBS_BBS_DP;
  BbsNode->DeviceType     = GetDevcieTypeInBBS(BbsEntry);
  CopyMem (&BbsNode->StatusFlag, &BbsEntry->StatusFlags, sizeof (BBS_STATUS_FLAGS));
  CopyMem (BbsNode->String, HelpString, StringLen + 1);

  SetDevicePathEndNode (NextDevicePathNode (BbsNode));
  
  //
  // Create the OptionalData
  //
  OptionalData = AllocatePool (sizeof (LEGACY_BOOT_OPTION_BBS_DATA));
  ASSERT (OptionalData != NULL);
  OptionalData->BbsIndex = BbsIndex;
  DEBUG((EFI_D_INFO, "LegacyBootOption BBS:%X T:%X %s\n", BbsIndex, BbsNode->DeviceType, Description));

  //
  // Create the BootOption
  //
  Status = EfiBootManagerInitializeLoadOption (
             BootOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             LOAD_OPTION_ACTIVE,
             Description,
             DevicePath,
             (UINT8 *) OptionalData,
             sizeof (LEGACY_BOOT_OPTION_BBS_DATA)
             );
  FreePool (DevicePath);
  FreePool (OptionalData);
  
  return Status;
}

/**
  Fill the device order buffer.

  @param BbsTable        The BBS table.
  @param BbsType         The BBS Type.
  @param BbsCount        The BBS Count.
  @param Buf             device order buffer.

  @return The device order buffer.

**/
UINT16 *
FillDevOrderBuf (
  IN BBS_TABLE                    *BbsTable,
  IN BBS_TYPE                     BbsType,
  IN UINTN                        BbsCount,
  OUT UINT16                      *Buf
  )
{
  UINTN Index;

  for (Index = 0; Index < BbsCount; Index++) {
    if (!ValidBbsEntry (&BbsTable[Index])) {
      continue;
    }

    if (GetDevcieTypeInBBS(&BbsTable[Index]) != BbsType) {
      continue;
    }

    *Buf = (UINT16) (Index & 0xFF);
    Buf++;
  }

  return Buf;
}

/**
  Create the device order buffer.

  @param BbsTable        The BBS table.
  @param BbsCount        The BBS Count.

  @retval EFI_SUCCES             The buffer is created and the EFI variable named 
                                 VAR_LEGACY_DEV_ORDER and EfiLegacyDevOrderGuid is
                                 set correctly.
  @retval EFI_OUT_OF_RESOURCES   Memmory or storage is not enough.
  @retval EFI_DEVICE_ERROR       Fail to add the device order into EFI variable fail
                                 because of hardware error.
**/
EFI_STATUS
CreateDevOrder (
  IN BBS_TABLE                  *BbsTable,
  IN UINT16                     BbsCount
  )
{
  UINTN                       Index;
  UINTN                       FDCount;
  UINTN                       HDCount;
  UINTN                       CDCount;
  UINTN                       NETCount;
  UINTN                       BEVCount;
  UINTN                       TotalSize;
  UINTN                       HeaderSize;
  LEGACY_DEV_ORDER_ENTRY      *DevOrder;
  LEGACY_DEV_ORDER_ENTRY      *DevOrderPtr;
  EFI_STATUS                  Status;

  FDCount     = 0;
  HDCount     = 0;
  CDCount     = 0;
  NETCount    = 0;
  BEVCount    = 0;
  TotalSize   = 0;
  HeaderSize  = sizeof (BBS_TYPE) + sizeof (UINT16);
  DevOrder    = NULL;
  Status      = EFI_SUCCESS;

  //
  // Count all boot devices
  //
  for (Index = 0; Index < BbsCount; Index++) {
    if (!ValidBbsEntry (&BbsTable[Index])) {
      continue;
    }

    switch (GetDevcieTypeInBBS(&BbsTable[Index])) {
      case BBS_FLOPPY:
        FDCount++;
        break;

      case BBS_HARDDISK:
        HDCount++;
        break;

      case BBS_CDROM:
        CDCount++;
        break;

      case BBS_EMBED_NETWORK:
        NETCount++;
        break;

      case BBS_BEV_DEVICE:
        BEVCount++;
        break;

      default:
        break;
    }
  }

  TotalSize += (HeaderSize + sizeof (UINT16) * FDCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * HDCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * CDCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * NETCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * BEVCount);

  //
  // Create buffer to hold all boot device order
  //
  DevOrder = AllocateZeroPool (TotalSize);
  if (NULL == DevOrder) {
    return EFI_OUT_OF_RESOURCES;
  }
  DevOrderPtr          = DevOrder;

  DevOrderPtr->BbsType = BBS_FLOPPY;
  DevOrderPtr->Length  = (UINT16) (sizeof (DevOrderPtr->Length) + FDCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *) FillDevOrderBuf (BbsTable, BBS_FLOPPY, BbsCount, DevOrderPtr->Data);

  DevOrderPtr->BbsType = BBS_HARDDISK;
  DevOrderPtr->Length  = (UINT16) (sizeof (UINT16) + HDCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *) FillDevOrderBuf (BbsTable, BBS_HARDDISK, BbsCount, DevOrderPtr->Data);
  
  DevOrderPtr->BbsType = BBS_CDROM;
  DevOrderPtr->Length  = (UINT16) (sizeof (UINT16) + CDCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *) FillDevOrderBuf (BbsTable, BBS_CDROM, BbsCount, DevOrderPtr->Data);
  
  DevOrderPtr->BbsType = BBS_EMBED_NETWORK;
  DevOrderPtr->Length  = (UINT16) (sizeof (UINT16) + NETCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *) FillDevOrderBuf (BbsTable, BBS_EMBED_NETWORK, BbsCount, DevOrderPtr->Data);

  DevOrderPtr->BbsType = BBS_BEV_DEVICE;
  DevOrderPtr->Length  = (UINT16) (sizeof (UINT16) + BEVCount * sizeof (UINT16));
  DevOrderPtr          = (LEGACY_DEV_ORDER_ENTRY *) FillDevOrderBuf (BbsTable, BBS_BEV_DEVICE, BbsCount, DevOrderPtr->Data);

  ASSERT (TotalSize == (UINTN) ((UINT8 *) DevOrderPtr - (UINT8 *) DevOrder));

  //
  // Save device order for legacy boot device to variable.
  //
  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &gEfiLegacyDevOrderVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  TotalSize,
                  DevOrder
                  );
  FreePool (DevOrder);

  return Status;
}

/**
  Add the legacy boot devices from BBS table into 
  the legacy device boot order.

  @retval EFI_SUCCESS           The boot devices are added successfully.
  @retval EFI_NOT_FOUND         The legacy boot devices are not found.
  @retval EFI_OUT_OF_RESOURCES  Memmory or storage is not enough.
  @retval EFI_DEVICE_ERROR      Fail to add the legacy device boot order into EFI variable
                                because of hardware error.
**/
EFI_STATUS
UpdateLegacyDevOrder (
  VOID
  )
{
  LEGACY_DEV_ORDER_ENTRY      *DevOrder;
  LEGACY_DEV_ORDER_ENTRY      *NewDevOrder;
  LEGACY_DEV_ORDER_ENTRY      *Ptr;
  LEGACY_DEV_ORDER_ENTRY      *NewPtr;
  UINTN                       DevOrderSize;
  EFI_LEGACY_BIOS_PROTOCOL    *LegacyBios;
  EFI_STATUS                  Status;
  UINT16                      HddCount;
  UINT16                      BbsCount;
  HDD_INFO                    *LocalHddInfo;
  BBS_TABLE                   *LocalBbsTable;
  UINTN                       Index;
  UINTN                       Index2;
  UINTN                       *Idx;
  UINTN                       FDCount;
  UINTN                       HDCount;
  UINTN                       CDCount;
  UINTN                       NETCount;
  UINTN                       BEVCount;
  UINTN                       TotalSize;
  UINTN                       HeaderSize;
  UINT16                      *NewFDPtr;
  UINT16                      *NewHDPtr;
  UINT16                      *NewCDPtr;
  UINT16                      *NewNETPtr;
  UINT16                      *NewBEVPtr;
  UINT16                      *NewDevPtr;
  UINTN                       FDIndex;
  UINTN                       HDIndex;
  UINTN                       CDIndex;
  UINTN                       NETIndex;
  UINTN                       BEVIndex;

  
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  
  Idx           = NULL;
  FDCount       = 0;
  HDCount       = 0;
  CDCount       = 0;
  NETCount      = 0;
  BEVCount      = 0;
  TotalSize     = 0;
  HeaderSize    = sizeof (BBS_TYPE) + sizeof (UINT16);
  FDIndex       = 0;
  HDIndex       = 0;
  CDIndex       = 0;
  NETIndex      = 0;
  BEVIndex      = 0;
  NewDevPtr     = NULL;

  Status        = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = LegacyBios->GetBbsInfo (
                         LegacyBios,
                         &HddCount,
                         &LocalHddInfo,
                         &BbsCount,
                         &LocalBbsTable
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DevOrder = EfiBootManagerGetVariableAndSize (
               VAR_LEGACY_DEV_ORDER,
               &gEfiLegacyDevOrderVariableGuid,
               &DevOrderSize
               );
  if (NULL == DevOrder) {
    return CreateDevOrder (LocalBbsTable, BbsCount);
  }


  DebugDempMem8(DevOrder, DevOrderSize);
  
  //
  // First we figure out how many boot devices with same device type respectively
  //
  for (Index = 0; Index < BbsCount; Index++) {
    if (!ValidBbsEntry (&LocalBbsTable[Index])) {
      continue;
    }

    switch (GetDevcieTypeInBBS(&LocalBbsTable[Index])) {
      case BBS_FLOPPY:
        FDCount++;
        break;

      case BBS_HARDDISK:
        HDCount++;
        break;

      case BBS_CDROM:
        CDCount++;
        break;

      case BBS_EMBED_NETWORK:
        NETCount++;
        break;

      case BBS_BEV_DEVICE:
        BEVCount++;
        break;

      default:
        break;
      }
  }

  TotalSize += (HeaderSize + FDCount * sizeof (UINT16));
  TotalSize += (HeaderSize + HDCount * sizeof (UINT16));
  TotalSize += (HeaderSize + CDCount * sizeof (UINT16));
  TotalSize += (HeaderSize + NETCount * sizeof (UINT16));
  TotalSize += (HeaderSize + BEVCount * sizeof (UINT16));

  NewDevOrder = AllocateZeroPool (TotalSize);
  if (NULL == NewDevOrder) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // copy FD
  //
  Ptr             = DevOrder;
  NewPtr          = NewDevOrder;
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16) (sizeof (UINT16) + FDCount * sizeof (UINT16));
  for (Index = 0; Index < Ptr->Length / sizeof (UINT16) - 1; Index++) {
    if (!ValidBbsEntry (&LocalBbsTable[Ptr->Data[Index] & 0xFF]) ||
        GetDevcieTypeInBBS(&LocalBbsTable[Ptr->Data[Index] & 0xFF]) != BBS_FLOPPY) {
      continue;
    }

    NewPtr->Data[FDIndex] = Ptr->Data[Index];
    FDIndex++;
  }
  NewFDPtr = NewPtr->Data;

  //
  // copy HD
  //
  Ptr             = (LEGACY_DEV_ORDER_ENTRY *) (&Ptr->Data[Ptr->Length / sizeof (UINT16) - 1]);
  NewPtr          = (LEGACY_DEV_ORDER_ENTRY *) (&NewPtr->Data[NewPtr->Length / sizeof (UINT16) -1]);
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16) (sizeof (UINT16) + HDCount * sizeof (UINT16));
  for (Index = 0; Index < Ptr->Length / sizeof (UINT16) - 1; Index++) {
    if (!ValidBbsEntry (&LocalBbsTable[Ptr->Data[Index] & 0xFF]) ||
        GetDevcieTypeInBBS(&LocalBbsTable[Ptr->Data[Index] & 0xFF]) != BBS_HARDDISK) {
      continue;
    }

    NewPtr->Data[HDIndex] = Ptr->Data[Index];
    HDIndex++;
  }
  NewHDPtr = NewPtr->Data;

  //
  // copy CD
  //
  Ptr    = (LEGACY_DEV_ORDER_ENTRY *) (&Ptr->Data[Ptr->Length / sizeof (UINT16) - 1]);
  NewPtr = (LEGACY_DEV_ORDER_ENTRY *) (&NewPtr->Data[NewPtr->Length / sizeof (UINT16) -1]);
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16) (sizeof (UINT16) + CDCount * sizeof (UINT16));
  for (Index = 0; Index < Ptr->Length / sizeof (UINT16) - 1; Index++) {
    if (!ValidBbsEntry (&LocalBbsTable[Ptr->Data[Index] & 0xFF]) ||
        GetDevcieTypeInBBS(&LocalBbsTable[Ptr->Data[Index] & 0xFF]) != BBS_CDROM) {
      continue;
    }

    NewPtr->Data[CDIndex] = Ptr->Data[Index];
    CDIndex++;
  }
  NewCDPtr = NewPtr->Data;

  //
  // copy NET
  //
  Ptr    = (LEGACY_DEV_ORDER_ENTRY *) (&Ptr->Data[Ptr->Length / sizeof (UINT16) - 1]);
  NewPtr = (LEGACY_DEV_ORDER_ENTRY *) (&NewPtr->Data[NewPtr->Length / sizeof (UINT16) -1]);
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16) (sizeof (UINT16) + NETCount * sizeof (UINT16));
  for (Index = 0; Index < Ptr->Length / sizeof (UINT16) - 1; Index++) {
    if (!ValidBbsEntry (&LocalBbsTable[Ptr->Data[Index] & 0xFF]) ||
        GetDevcieTypeInBBS(&LocalBbsTable[Ptr->Data[Index] & 0xFF]) != BBS_EMBED_NETWORK) {
      continue;
    }

    NewPtr->Data[NETIndex] = Ptr->Data[Index];
    NETIndex++;
  }
  NewNETPtr = NewPtr->Data;
  
  //
  // copy BEV
  //
  Ptr    = (LEGACY_DEV_ORDER_ENTRY *) (&Ptr->Data[Ptr->Length / sizeof (UINT16) - 1]);
  NewPtr = (LEGACY_DEV_ORDER_ENTRY *) (&NewPtr->Data[NewPtr->Length / sizeof (UINT16) -1]);
  NewPtr->BbsType = Ptr->BbsType;
  NewPtr->Length  = (UINT16) (sizeof (UINT16) + BEVCount * sizeof (UINT16));
  for (Index = 0; Index < Ptr->Length / sizeof (UINT16) - 1; Index++) {
    if (!ValidBbsEntry (&LocalBbsTable[Ptr->Data[Index] & 0xFF]) ||
        GetDevcieTypeInBBS(&LocalBbsTable[Ptr->Data[Index] & 0xFF]) != BBS_BEV_DEVICE) {
      continue;
    }

    NewPtr->Data[BEVIndex] = Ptr->Data[Index];
    BEVIndex++;
  }
  NewBEVPtr = NewPtr->Data;

  for (Index = 0; Index < BbsCount; Index++) {
    if (!ValidBbsEntry (&LocalBbsTable[Index])) {
      continue;
    }

    switch (GetDevcieTypeInBBS(&LocalBbsTable[Index])) {
    case BBS_FLOPPY:
      Idx       = &FDIndex;
      NewDevPtr = NewFDPtr;
      break;

    case BBS_HARDDISK:
      Idx       = &HDIndex;
      NewDevPtr = NewHDPtr;
      break;

    case BBS_CDROM:
      Idx       = &CDIndex;
      NewDevPtr = NewCDPtr;
      break;

    case BBS_EMBED_NETWORK:
      Idx       = &NETIndex;
      NewDevPtr = NewNETPtr;
      break;

    case BBS_BEV_DEVICE:
      Idx       = &BEVIndex;
      NewDevPtr = NewBEVPtr;
      break;

    default:
      Idx = NULL;
      break;
    }
    //
    // at this point we have copied those valid indexes to new buffer
    // and we should check if there is any new appeared boot device
    //
    if (Idx != NULL) {
      for (Index2 = 0; Index2 < *Idx; Index2++) {
        if ((NewDevPtr[Index2] & 0xFF) == (UINT16) Index) {
          break;
        }
      }

      if (Index2 == *Idx) {
        //
        // Index2 == *Idx means we didn't find Index
        // so Index is a new appeared device's index in BBS table
        // insert it before disabled indexes.
        //
        for (Index2 = 0; Index2 < *Idx; Index2++) {
          if ((NewDevPtr[Index2] & 0xFF00) == 0xFF00) {
            break;
          }
        }
        CopyMem (&NewDevPtr[Index2 + 1], &NewDevPtr[Index2], (*Idx - Index2) * sizeof (UINT16));
        NewDevPtr[Index2] = (UINT16) (Index & 0xFF);
        (*Idx)++;
      }
    }
  }

  FreePool (DevOrder);

  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &gEfiLegacyDevOrderVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  TotalSize,
                  NewDevOrder
                  );
  DebugDempMem8(NewDevOrder, TotalSize);                
  FreePool (NewDevOrder);

  return Status;
}

/**
  Set Boot Priority for specified device type.

  @param DeviceType      The device type.
  @param BbsIndex        The BBS index to set the highest priority. Ignore when -1.
  @param LocalBbsTable   The BBS table.
  @param Priority        The prority table.

  @retval EFI_SUCCESS           The function completes successfully.
  @retval EFI_NOT_FOUND         Failed to find device.
  @retval EFI_OUT_OF_RESOURCES  Failed to get the efi variable of device order.

**/
EFI_STATUS
SetBootPriority4SameTypeDev (
  IN UINT16          DeviceType,
  IN UINTN           BbsIndex,
  IN OUT BBS_TABLE   *LocalBbsTable,
  IN OUT UINT16      *Priority
  )
{
  LEGACY_DEV_ORDER_ENTRY      *DevOrder;
  LEGACY_DEV_ORDER_ENTRY      *DevOrderPtr;
  UINTN                       DevOrderSize;
  UINTN                       Index;

  DevOrder = EfiBootManagerGetVariableAndSize (
               VAR_LEGACY_DEV_ORDER,
               &gEfiLegacyDevOrderVariableGuid,
               &DevOrderSize
               );
  if (NULL == DevOrder) {
    return EFI_OUT_OF_RESOURCES;
  }

  DevOrderPtr = DevOrder;
  while ((UINT8 *) DevOrderPtr < (UINT8 *) DevOrder + DevOrderSize) {
    if (DevOrderPtr->BbsType == DeviceType) {
      break;
    }

    DevOrderPtr = (LEGACY_DEV_ORDER_ENTRY *) ((UINT8 *) DevOrderPtr + sizeof (BBS_TYPE) + DevOrderPtr->Length);
  }

  if ((UINT8 *) DevOrderPtr >= (UINT8 *) DevOrder + DevOrderSize) {
    FreePool (DevOrder);
    return EFI_NOT_FOUND;
  }

  if (BbsIndex != (UINTN) -1) {
    //
    // In case the BBS entry isn't valid because devices were plugged or removed.
    //
    if (!ValidBbsEntry (&LocalBbsTable[BbsIndex]) || (GetDevcieTypeInBBS(&LocalBbsTable[BbsIndex]) != DeviceType)) {
      FreePool (DevOrder);
      return EFI_NOT_FOUND;
    }
    LocalBbsTable[BbsIndex].BootPriority = *Priority;
    (*Priority)++;
  }
  //
  // If the high byte of the DevIndex is 0xFF, it indicates that this device has been disabled.
  //
  for (Index = 0; Index < DevOrderPtr->Length / sizeof (UINT16) - 1; Index++) {
    if ((DevOrderPtr->Data[Index] & 0xFF00) == 0xFF00) {
      //
      // LocalBbsTable[DevIndex[Index] & 0xFF].BootPriority = BBS_DISABLED_ENTRY;
      //
    } else if (DevOrderPtr->Data[Index] != BbsIndex) {
      LocalBbsTable[DevOrderPtr->Data[Index]].BootPriority = *Priority;
      (*Priority)++;
    }
  }

  FreePool (DevOrder);
  return EFI_SUCCESS;
}

/**
  Print the BBS Table.

  @param LocalBbsTable   The BBS table.
  @param BbsCount        The count of entry in BBS table.
**/
VOID
PrintBbsTable (
  IN BBS_TABLE  *LocalBbsTable,
  IN UINT16     BbsCount
  )
{
  UINT16  Index;

  DEBUG ((DEBUG_INFO, "\n"));
  DEBUG ((DEBUG_INFO, " NO  Prio bb/dd/ff cl/sc Type Stat segm:offs\n"));
  DEBUG ((DEBUG_INFO, "=============================================\n"));
  for (Index = 0; Index < BbsCount; Index++) {
    if (!ValidBbsEntry (&LocalBbsTable[Index])) {
      continue;
    }

    DEBUG (
      (DEBUG_INFO,
      " %02x: %04x %02x/%02x/%02x %02x/%02x %04x %04x %04x:%04x\n",
      (UINTN) Index,
      (UINTN) LocalBbsTable[Index].BootPriority,
      (UINTN) LocalBbsTable[Index].Bus,
      (UINTN) LocalBbsTable[Index].Device,
      (UINTN) LocalBbsTable[Index].Function,
      (UINTN) LocalBbsTable[Index].Class,
      (UINTN) LocalBbsTable[Index].SubClass,
      (UINTN) LocalBbsTable[Index].DeviceType,
      (UINTN) * (UINT16 *) &LocalBbsTable[Index].StatusFlags,
      (UINTN) LocalBbsTable[Index].BootHandlerSegment,
      (UINTN) LocalBbsTable[Index].BootHandlerOffset,
      (UINTN) ((LocalBbsTable[Index].MfgStringSegment << 4) + LocalBbsTable[Index].MfgStringOffset),
      (UINTN) ((LocalBbsTable[Index].DescStringSegment << 4) + LocalBbsTable[Index].DescStringOffset))
      );
  }

  DEBUG ((DEBUG_INFO, "\n"));
}







BOOLEAN 
IsThisLegacyBootDevChanged (
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  EFI_STATUS                    Status;
  UINT16                        BbsIndex;
  UINT16                        HddCount;
  UINT16                        BbsCount;
  HDD_INFO                      *LocalHddInfo;
  BBS_TABLE                     *LocalBbsTable;
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  CHAR8                         *BbsDevStr;
  CHAR8                         *DpDevStr;
  CHAR16                        *NewBbsDevStr = NULL;
  CHAR16                        *NewDpDevStr  = NULL;
  BOOLEAN                       Rc;	
  UINTN                         BbsStrLen;
  UINTN                         DpStrLen;	
  UINTN                         StrStart;


  Rc = FALSE;
  if (DevicePathType(BootOption->FilePath) != BBS_DEVICE_PATH || 
      DevicePathSubType(BootOption->FilePath) != BBS_BBS_DP){
    goto ProcExit;
  }

  Status = gBS->LocateProtocol(&gEfiLegacyBiosProtocolGuid, NULL, (VOID**)&LegacyBios);
  if (EFI_ERROR (Status)) {
    goto ProcExit;
  }
  Status = LegacyBios->GetBbsInfo (
                         LegacyBios,
                         &HddCount,
                         &LocalHddInfo,
                         &BbsCount,
                         &LocalBbsTable
                         );
  if (EFI_ERROR (Status)) {
    goto ProcExit;
  }

  BbsIndex = ((LEGACY_BOOT_OPTION_BBS_DATA*)BootOption->OptionalData)->BbsIndex;
  if(GetDevcieTypeInBBS(&LocalBbsTable[BbsIndex]) != LegacyDeviceType(BootOption->FilePath)){
    Rc = TRUE;		
    goto ProcExit;
  }

  BbsDevStr = (CHAR8*)(UINTN)((LocalBbsTable[BbsIndex].DescStringSegment << 4) + 
		                           LocalBbsTable[BbsIndex].DescStringOffset);
  if(BbsDevStr == (CHAR8*)0){
    BbsDevStr = "";
  }		
  DpDevStr = ((BBS_BBS_DEVICE_PATH*)(BootOption->FilePath))->String;
  if(DpDevStr == NULL){
    DpDevStr = "";
  }

  NewBbsDevStr = AllocatePool(sizeof(CHAR16)*AsciiStrSize(BbsDevStr));
  ASSERT(NewBbsDevStr!=NULL);	
  NewDpDevStr = AllocatePool(sizeof(CHAR16)*AsciiStrSize(DpDevStr));
  ASSERT(NewDpDevStr!=NULL);

  AsciiStrToUnicodeStr(BbsDevStr, NewBbsDevStr);
  AsciiStrToUnicodeStr(DpDevStr,  NewDpDevStr);
  TrimStr16(NewBbsDevStr);
  TrimStr16(NewDpDevStr);	


  if(!StrCmp(NewBbsDevStr, NewDpDevStr)){
    goto ProcExit;
  }


  Rc = TRUE;
  
// some type devices, such as network, usb, these orignal name strings do not have
// type prefix name "NET","USB","SATA", so here we should skip it.
    DpStrLen  = StrLen(NewDpDevStr);
  for(StrStart=0;StrStart < DpStrLen;StrStart++){
    if(NewDpDevStr[StrStart] == ':' && NewDpDevStr[StrStart+1] == ' '){
      StrStart += 2;
        break;
      }
    }
  if(StrStart >= DpStrLen){
      goto ProcExit;
    }	
  
  DpStrLen  = DpStrLen - StrStart;
  BbsStrLen = StrLen(NewBbsDevStr);
  if(StrnCmp(NewDpDevStr+StrStart, NewBbsDevStr, MIN(BbsStrLen, DpStrLen))){
    goto ProcExit;
  }	

  Rc = FALSE;		
	

ProcExit:
  if(NewBbsDevStr!=NULL){FreePool(NewBbsDevStr);}
  if(NewDpDevStr!=NULL) {FreePool(NewDpDevStr);}	
	return Rc;
}









/**
  Set the boot priority for BBS entries based on boot option entry and boot order.

  @param  BootOption            The boot option is to be checked for refresh BBS table.
  
  @retval EFI_SUCCESS           The boot priority for BBS entries is refreshed successfully.
  @retval EFI_NOT_FOUND         BBS entries can't be found.
  @retval EFI_OUT_OF_RESOURCES  Failed to get the legacy device boot order.
**/
EFI_STATUS
RefreshBbsTableForBoot (
  IN EFI_BOOT_MANAGER_LOAD_OPTION        *BootOption
  )
{
  EFI_STATUS                    Status;
  UINT16                        BbsIndex;
  UINT16                        HddCount;
  UINT16                        BbsCount;
  HDD_INFO                      *LocalHddInfo;
  BBS_TABLE                     *LocalBbsTable;
  UINT16                        DevType;
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  UINTN                         Index;
  UINT16                        Priority;
  UINT16                        *DeviceType;
  UINTN                         DeviceTypeCount;
  UINTN                         DeviceTypeIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION  *Option;
  UINTN                         OptionCount;

  HddCount      = 0;
  BbsCount      = 0;
  LocalHddInfo  = NULL;
  LocalBbsTable = NULL;
  DevType       = BBS_UNKNOWN;

  Status        = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = LegacyBios->GetBbsInfo (
                         LegacyBios,
                         &HddCount,
                         &LocalHddInfo,
                         &BbsCount,
                         &LocalBbsTable
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // First, set all the present devices' boot priority to BBS_UNPRIORITIZED_ENTRY
  // We will set them according to the settings setup by user
  //
  for (Index = 0; Index < BbsCount; Index++) {
    if (ValidBbsEntry (&LocalBbsTable[Index])) {
      LocalBbsTable[Index].BootPriority = BBS_UNPRIORITIZED_ENTRY;
    }
  }
  //
  // boot priority always starts at 0
  //
  Priority = 0;  
  if ((DevicePathType (BootOption->FilePath) == BBS_DEVICE_PATH) &&
      (DevicePathSubType (BootOption->FilePath) == BBS_BBS_DP)) {
    //
    // If BootOption stands for a legacy boot option, we prioritize the devices with the same type first.
    //
    DevType  = LegacyDeviceType (BootOption->FilePath);
    BbsIndex = ((LEGACY_BOOT_OPTION_BBS_DATA *) BootOption->OptionalData)->BbsIndex;
    Status = SetBootPriority4SameTypeDev (
               DevType,
               BbsIndex,
               LocalBbsTable,
               &Priority
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  //
  // we have to set the boot priority for other BBS entries with different device types
  //
  Option          = EfiBootManagerGetLoadOptions (&OptionCount, LoadOptionTypeBoot);
  DeviceType      = AllocatePool (sizeof (UINT16) * OptionCount);
  DeviceType[0]   = DevType;
  DeviceTypeCount = 1;
  for (Index = 0; Index < OptionCount; Index++) {
    if ((DevicePathType (Option[Index].FilePath) != BBS_DEVICE_PATH) ||
        (DevicePathSubType (Option[Index].FilePath) != BBS_BBS_DP)) {
      continue;
    }
    
    DevType = LegacyDeviceType (Option[Index].FilePath);
    for (DeviceTypeIndex = 0; DeviceTypeIndex < DeviceTypeCount; DeviceTypeIndex++) {
      if (DeviceType[DeviceTypeIndex] == DevType) {
        break;
      }
    }
    if (DeviceTypeIndex < DeviceTypeCount) {
      //
      // We don't want to process twice for a device type
      //
      continue;
    }

    DeviceType[DeviceTypeCount] = DevType;
    DeviceTypeCount++;

    Status = SetBootPriority4SameTypeDev (
               DevType,
               (UINTN) -1,
               LocalBbsTable,
               &Priority
               );
  }
  EfiBootManagerFreeLoadOptions (Option, OptionCount);

  DEBUG_CODE_BEGIN();
    PrintBbsTable (LocalBbsTable, BbsCount);
  DEBUG_CODE_END();
  
  return Status;
}


/**
  Boot the legacy system with the boot option.

  @param  BootOption The legacy boot option which have BBS device path
                     On return, BootOption->Status contains the boot status.
                     EFI_UNSUPPORTED    There is no legacybios protocol, do not support
                                        legacy boot.
                     EFI_STATUS         The status of LegacyBios->LegacyBoot ().
**/
VOID
EFIAPI
LegacyBoot (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION           *BootOption
  )
{
  EFI_STATUS                Status;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    //
    // If no LegacyBios protocol we do not support legacy boot
    //
    BootOption->Status = EFI_UNSUPPORTED;
    return;
  }
  //
  // Notes: if we separate the int 19, then we don't need to refresh BBS
  //
  Status = RefreshBbsTableForBoot (BootOption);
  if (EFI_ERROR (Status)) {
    BootOption->Status = Status;
    return;
  }

  BootOption->Status = LegacyBios->LegacyBoot (
                                     LegacyBios,
                                     (BBS_BBS_DEVICE_PATH *) BootOption->FilePath,
                                     BootOption->OptionalDataSize,
                                     BootOption->OptionalData
                                     );
}

/**
  This function enumerates all the legacy boot options.
**/
EFI_BOOT_MANAGER_LOAD_OPTION *
EnumerateLegacyBootOptions (
  UINTN                         *BootOptionCount
  )
{
  EFI_STATUS                    Status;
  UINT16                        HddCount;
  UINT16                        BbsCount;
  HDD_INFO                      *HddInfo;
  BBS_TABLE                     *BbsTable;
  EFI_LEGACY_BIOS_PROTOCOL      *LegacyBios;
  UINT16                        Index;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINT32                        GrpDefPriority;  
  UINT8                         CurPri;
  UINTN                         CurLoop;
  UINTN                         TotalPri;
  UINT16                        PriType;
  UINT8                         *CheckedList;
  BOOLEAN                       CheckRemain;
  UINTN                         VarBufferSize = 0;
  VOID                          *VarBuffer    = NULL;	
  

  ASSERT (BootOptionCount != NULL);
  BootOptions      = NULL;
  *BootOptionCount = 0;
  BbsCount         = 0;
  GrpDefPriority   = PcdGet32(PcdBootDeviceGroupDefaultPriority);
  TotalPri         = PcdGet32(PcdBootDeviceGroupDefaultCount);
  CheckedList      = NULL;
  PriType          = 0;
  
  DEBUG((EFI_D_INFO, "%a() P:%X\n", __FUNCTION__, GrpDefPriority));
  
  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = LegacyBios->GetBbsInfo (
                         LegacyBios,
                         &HddCount,
                         &HddInfo,
                         &BbsCount,
                         &BbsTable
                         );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  CheckedList = AllocateZeroPool(BbsCount);
  if(CheckedList == NULL){
    return NULL;
  }

  Status = gRT->GetVariable(
                  EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME, 
                  &gEfiMemoryTypeInformationGuid, 
                  NULL, 
                  &VarBufferSize, 
                  VarBuffer
                  );
  DEBUG((EFI_D_INFO, "GetVar(%s):%r\n", EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME, Status));	
  if (Status == EFI_BUFFER_TOO_SMALL) {
    CheckRemain = TRUE;                         // just follow BBS order.		
  }	else {
    CheckRemain = FALSE;                        // first boot, set default priority
  }    
	
  CurLoop = 0;
  do {
    if(CurLoop < TotalPri && !CheckRemain){
      CurPri = (UINT8)((GrpDefPriority >> (CurLoop*4))&0xF);
      
//#HDD        : 0
//#CD-ROM     : 1
//#NETWORK    : 2
//#BEV        : 3
//#FLOPPY     : 4      
      switch(CurPri){
        case 0:
          PriType = BBS_HARDDISK;
          break;
        case 1:
          PriType = BBS_CDROM;
          break;
        case 2:
          PriType = BBS_EMBED_NETWORK;
          break;
        case 3:
          PriType = BBS_BEV_DEVICE;
          break;
        case 4:
          PriType = BBS_FLOPPY;
          break;
        default:
          PriType = BBS_UNKNOWN;
          break;      
      }
      DEBUG((EFI_D_INFO, "CurLoop:%d CurPri:0x%X, Type:0x%X\n", CurLoop, CurPri, PriType));
    }else{
      CheckRemain = TRUE;
    }
    
    for (Index = 0; Index < BbsCount; Index++) {
      if(CheckedList[Index]){
        continue;
      }
      if (!ValidBbsEntry (&BbsTable[Index])) {
        CheckedList[Index] = 1;
        continue;
      }
      if(PriType != GetDevcieTypeInBBS(&BbsTable[Index]) && !CheckRemain){
        continue;
      }
      BootOptions = ReallocatePool (
                      sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                      sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                      BootOptions
                      );
      ASSERT (BootOptions != NULL);

      Status = CreateLegacyBootOption (&BootOptions[(*BootOptionCount)++], &BbsTable[Index], Index);
      ASSERT_EFI_ERROR (Status);
      
      CheckedList[Index] = 1;
    }
    
    CurLoop++;
  } while(!CheckRemain);
  
  FreePool(CheckedList);
  return BootOptions;
}

/**
  Return the index of the boot option in the boot option array.

  The function compares the Description, FilePath, OptionalData.
**/
INTN
LegacyBootManagerFindBootOption (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Key,
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Array,
  IN UINTN                              Count
  )
{
  UINTN                             Index;

  for (Index = 0; Index < Count; Index++) {
    if ((StrCmp (Key->Description, Array[Index].Description) == 0) &&
        (CompareMem (Key->FilePath, Array[Index].FilePath, GetDevicePathSize (Key->FilePath)) == 0) &&
        (Key->OptionalDataSize == Array[Index].OptionalDataSize) &&
        (CompareMem (Key->OptionalData, Array[Index].OptionalData, Key->OptionalDataSize) == 0)) {
      return (INTN) Index;
    }
  }

  return -1;
}

/**
  Return the legacy device position in the bootorder.
**/
VOID
EFIAPI
ByoFindTheRightPosition (
  IN     EFI_BOOT_MANAGER_LOAD_OPTION *Key,
  IN     CHAR16                       *BootVar,
  IN     EFI_GUID                     *BootVarGuid,
  IN OUT UINTN                        *Position
  )
{
  EFI_STATUS                   Status;
  UINT16                       *OptionOrder;
  UINTN                        OptionOrderSize;
  UINTN                        Index;
  CHAR16                       OptionName[sizeof ("Driver####")];
  EFI_BOOT_MANAGER_LOAD_OPTION Option;
  UINT16                       InputDeviceType;
  UINTN                        HdInsertPosition  = 0;
  UINTN                        CdInsertPosition  = 0;
  UINTN                        BevInsertPosition = 0;
  UINT16                       CurrentDeviceType;

  
  InputDeviceType = LegacyDeviceType (Key->FilePath);
  
  switch (InputDeviceType) {
    case BBS_HARDDISK:
    case BBS_CDROM:
    case BBS_BEV_DEVICE:
      break;

    default:
      return;
      break;
  }  

  //
  // Read the BootOrder, or DriverOrder variable.
  //
  OptionOrder = EfiBootManagerGetVariableAndSize (
                  BootVar,
                  BootVarGuid,
                  &OptionOrderSize
                  );
  if (OptionOrder == NULL) {
    return;
  }

  for (Index = 0; Index < OptionOrderSize/sizeof(UINT16); Index++) {
    ZeroMem(&Option, sizeof(EFI_BOOT_MANAGER_LOAD_OPTION));
    
    UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", OptionOrder[Index]);

    Status = EfiBootManagerVariableToLoadOption (OptionName, &Option);
    if (EFI_ERROR (Status)) {
      continue;
    } else {   
      if ((DevicePathType (Option.FilePath) == BBS_DEVICE_PATH) &&
        (DevicePathSubType (Option.FilePath) == BBS_BBS_DP)) {
        CurrentDeviceType = LegacyDeviceType (Option.FilePath);

        switch (CurrentDeviceType) {
          case BBS_HARDDISK:
            HdInsertPosition = Index + 1;
            break;

          case BBS_CDROM:
            CdInsertPosition = Index + 1;
            break;

          case BBS_BEV_DEVICE:
            BevInsertPosition = Index + 1;
            break;

          default:
            break;
        }
      }
    }
    
    EfiBootManagerFreeLoadOption(&Option);
  }  

  if(OptionOrder!=NULL){
    FreePool(OptionOrder);
    OptionOrder = NULL;
  }

  switch (InputDeviceType) {
    case BBS_HARDDISK:
      if(HdInsertPosition != 0) {
        *Position = HdInsertPosition;
      }
      break;
      
    case BBS_CDROM:
      if(CdInsertPosition != 0) {
        *Position = CdInsertPosition;
      }
      else {
        if(BevInsertPosition != 0) {
          *Position = BevInsertPosition; 
        }
      }
      break;
      
    case BBS_BEV_DEVICE:
      if(BevInsertPosition != 0) {
        *Position = BevInsertPosition;
      }
      else {
        if(CdInsertPosition != 0) {
          *Position = CdInsertPosition; 
        }
      }
      break;

    default:
      break;
  }
  
  DEBUG((EFI_D_INFO, "Position:%d\n", *Position));  
  return;
}


VOID
EFIAPI
LegacyBootManagerRefreshAllBootOption (
  VOID
  )
{
  EFI_STATUS                                 Status;
  EFI_LEGACY_BIOS_PROTOCOL                   *LegacyBios;
  UINTN                                      RootBridgeHandleCount;
  EFI_HANDLE                                 *RootBridgeHandleBuffer;
  UINTN                                      HandleCount;
  EFI_HANDLE                                 *HandleBuffer;
  UINTN                                      RootBridgeIndex;
  UINTN                                      Index;
  UINTN                                      Flags;
  EFI_BOOT_MANAGER_LOAD_OPTION               *BootOptions;
  UINTN                                      BootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION               *ExistingBootOptions;
  UINTN                                      ExistingBootOptionCount;
  UINTN                                      Position;
  UINT8                                      *BootOrderVar;
  UINTN                                      BootOrderVarSize;
  INTN                                       VarIndex;
  
  
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
  
  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status) || PcdGet8(PcdBiosBootModeType) == 1) {
    DeleteAllLegacyBootOptions ();
    return;
  }
  PERF_START (NULL, "LegacyBootOptionEnum", "BDS", 0);

  //
  // Before enumerating the legacy boot option, we need to dispatch all the legacy option roms 
  // to ensure the GetBbsInfo() counts all the legacy devices.
  //
  RootBridgeHandleBuffer = NULL;
  HandleBuffer           = NULL;
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiPciRootBridgeIoProtocolGuid,
         NULL,
         &RootBridgeHandleCount,
         &RootBridgeHandleBuffer
         );
  for (RootBridgeIndex = 0; RootBridgeIndex < RootBridgeHandleCount; RootBridgeIndex++) {
    gBS->ConnectController(RootBridgeHandleBuffer[RootBridgeIndex], NULL, NULL, FALSE);
    gBS->LocateHandleBuffer (
           ByProtocol,
           &gEfiPciIoProtocolGuid,
           NULL,
           &HandleCount,
           &HandleBuffer
           );
    for (Index = 0; Index < HandleCount; Index++) {
      //
      // Start the thunk driver so that the legacy option rom gets dispatched.
      // Note: We don't directly call InstallPciRom because some thunk drivers 
      // (e.g. BlockIo thunk driver) depend on the immediate result after dispatching
      //
      Status = LegacyBios->CheckPciRom (
                             LegacyBios,
                             HandleBuffer[Index],
                             NULL,
                             NULL,
                             &Flags
                             );
      if (!EFI_ERROR (Status)) {
        gBS->ConnectController (HandleBuffer[Index], NULL, NULL, FALSE);
      }
    }
    FreePool(HandleBuffer);
    HandleBuffer = NULL;
  }
  if(RootBridgeHandleBuffer!=NULL){
    FreePool(RootBridgeHandleBuffer);
    RootBridgeHandleBuffer = NULL;
  }
  DEBUG((EFI_D_INFO, "ConnectPciController End\n"));  

  //
  // Same algorithm pattern as the EfiBootManagerRefreshAllBootOption
  // Firstly delete the invalid legacy boot options,
  // then enumreate and save the newly appeared legacy boot options
  // the last step is legacy boot option special action to refresh the LegacyDevOrder variable
  //

  BootOrderVar = EfiBootManagerGetVariableAndSize (
                   L"BootOrder",
                   &gEfiGlobalVariableGuid,
                   &BootOrderVarSize
                   );
  if(BootOrderVar!=NULL && BootOrderVarSize!=0){
    gRT->SetVariable(
           L"LastBootOrder",
           &gLastBootOrderVariableGuid,
           EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
           BootOrderVarSize,
           BootOrderVar
           );
    FreePool(BootOrderVar);
  }
  
  DeleteAllInvalidLegacyBootOptions();

  ExistingBootOptions = EfiBootManagerGetLoadOptions(&ExistingBootOptionCount, LoadOptionTypeBoot);
  BootOptions         = EnumerateLegacyBootOptions(&BootOptionCount);

  for (Index = 0; Index < BootOptionCount; Index++) {
    VarIndex = LegacyBootManagerFindBootOption (
                 &BootOptions[Index], 
                 ExistingBootOptions, 
                 ExistingBootOptionCount
                 );
    DEBUG((EFI_D_INFO, "BootOption[%d] at Var[%d]\n", Index, VarIndex));
    if (VarIndex == -1) {
      Position = (UINTN) -1;
      ByoFindTheRightPosition(&BootOptions[Index], L"BootOrder", &gEfiGlobalVariableGuid, &Position);
      if(Position == -1){
        ByoFindTheRightPosition(&BootOptions[Index], L"LastBootOrder", &gLastBootOrderVariableGuid, &Position);
        if(Position != -1){
          // control goes here, means type of new device cannot be found at current BootOrder(already deleted),
          // but could be found at Last BootOrder. So just replace it, not be after it. 
          Position--;
        }
      }
      Status = EfiBootManagerAddLoadOptionVariable (&BootOptions[Index], Position);
      DEBUG ((
        EFI_D_INFO, "[LegacyBds] New Boot Option: Boot%04x Bbs0x%04x %s %r\n",
        (UINTN) BootOptions[Index].OptionNumber,
        (UINTN) ((LEGACY_BOOT_OPTION_BBS_DATA *) BootOptions[Index].OptionalData)->BbsIndex,
        BootOptions[Index].Description,
        Status
        ));
      ASSERT_EFI_ERROR (Status);
    } else {

    }
  }

  EfiBootManagerFreeLoadOptions (ExistingBootOptions, ExistingBootOptionCount);
  EfiBootManagerFreeLoadOptions (BootOptions,         BootOptionCount);

  UpdateLegacyDevOrder ();

  PERF_END(NULL, "LegacyBootOptionEnum", "BDS", 0);
}


