

#include "PlatformDxe.h"
#include <SetupVariable.h>
#include <AutoBuildTime.h>
#include <BiosVersion.h>



typedef struct {
  CHAR8  *IRD_Str;
  UINT8  IntConnType;
  CHAR8  *ERD_Str;
  UINT8  ExtConnType;
  UINT8  PortType;
} SMBIOS_TYPE8_CONTENT;

typedef struct {
  CHAR8   *SlotDesigStr;
  UINT8   SlotType;
  UINT8   SlotbusWidth;
  UINT8   SlotUsage;
  UINT8   SlotLen;
  UINT16  SlotId;
  UINT8   SlotChars;
  UINT8   SlotChars2;
  UINT8   BusNo;
  UINT8   DevFuncNo;
} SMBIOS_TYPE9_CONTENT;

typedef struct {
  CHAR8   *Name;
  UINT8   DeviceType;
  UINT8   DeviceTypeInstance;
  UINT16  SegmentGroupNum;
  UINT8   BusNum;
  UINT8   DevFuncNum;
} SMBIOS_TYPE41_CONTENT;

#define DMI_TYPE41_DEVTYPE_DEV_EN    BIT7



//----------------------------------------------------------------
STATIC SMBIOS_TYPE8_CONTENT gType8ContentList[] = {
  {"J1",  PortConnectorTypeNone, "PS2 Mouse",    PortConnectorTypePS2,        PortTypeMouse},
  {"J2",  PortConnectorTypeNone, "PS2 Keyboard", PortConnectorTypePS2,        PortTypeKeyboard},

  {"J3",  PortConnectorTypeNone, "VGA",          PortConnectorTypeDB15Female, PortTypeVideoPort},
  {"J4",  PortConnectorTypeNone, "HDMI",         PortConnectorTypeOther,      PortTypeVideoPort},

  {"J5",  PortConnectorTypeNone, "USB 1",        PortConnectorTypeUsb,        PortTypeUsb},
  {"J6",  PortConnectorTypeNone, "USB 2",        PortConnectorTypeUsb,        PortTypeUsb},
  {"J7",  PortConnectorTypeNone, "USB 3",        PortConnectorTypeUsb,        PortTypeUsb},
  {"J8",  PortConnectorTypeNone, "USB 4",        PortConnectorTypeUsb,        PortTypeUsb},
  {"J9",  PortConnectorTypeNone, "USB 5",        PortConnectorTypeUsb,        PortTypeUsb},

  {"J10", PortConnectorTypeNone, "Ethernet",     PortConnectorTypeRJ45,       PortTypeNetworkPort},
};

//     Dev                   SlotNo
//J22   3    PE0        1x     2
//J27   7    PEG1       2x     6
//J32   2    PEG0       8x     1
//J38   5    PE2        1x     4
//J42   6    PE3        1x     5
//Lan   4    PE1        1x     3
SMBIOS_TYPE9_CONTENT  gSmbiosType9ContentList[] = {
  {"J22", SlotTypePciExpress, SlotDataBusWidth1X, SlotUsageAvailable, SlotLengthUnknown, 2, 0x0c, 0x01, 0, 3 << 3},
  {"J27", SlotTypePciExpress, SlotDataBusWidth2X, SlotUsageAvailable, SlotLengthUnknown, 6, 0x0c, 0x01, 0, 7 << 3},
  {"J32", SlotTypePciExpress, SlotDataBusWidth8X, SlotUsageAvailable, SlotLengthUnknown, 1, 0x0c, 0x01, 0, 2 << 3},
  {"J38", SlotTypePciExpress, SlotDataBusWidth1X, SlotUsageAvailable, SlotLengthUnknown, 4, 0x0c, 0x01, 0, 5 << 3},  
  {"J42", SlotTypePciExpress, SlotDataBusWidth1X, SlotUsageAvailable, SlotLengthUnknown, 5, 0x0c, 0x01, 0, 6 << 3},
};	

SMBIOS_TYPE41_CONTENT  gSmbiosType41ContentList[] = {
  //{"SATA",  OnBoardDeviceExtendedTypeSATAController, 1, 0, 0, (0xF << 3)},
  {"Audio", OnBoardDeviceExtendedTypeSound,          1, 0, 0, (0x14 << 3)},    
};






//----------------------------------------------------------------
STATIC
EFI_STATUS
AddSmbiosRecord (
  IN EFI_SMBIOS_PROTOCOL        *Smbios,
  OUT EFI_SMBIOS_HANDLE         *SmbiosHandle,
  IN EFI_SMBIOS_TABLE_HEADER    *Record
  )
{
  *SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  return Smbios->Add (
                   Smbios,
                   NULL,
                   SmbiosHandle,
                   Record
                   );
}


STATIC EFI_STATUS AddSmbiosType2(EFI_SMBIOS_PROTOCOL *Smbios)
{
  CHAR8                           *Str8;
  UINTN                           ManuStrLen;
  UINTN                           ProductStrLen;
  UINTN                           VerStrLen;
  UINTN                           AssertTagStrLen;
  UINTN                           SerialNumStrLen;
  UINTN                           ChassisStrLen;
  EFI_STATUS                      Status;
  CHAR8                           *Manufacturer;
  CHAR8                           *Product;
  CHAR8                           *Version;
  CHAR8                           *SerialNumber;
  CHAR8                           *AssertTag;
  CHAR8                           *Chassis;
  EFI_SMBIOS_HANDLE               SmbiosHandle;
  SMBIOS_TABLE_TYPE2              *SmbiosRecord;
  UINTN                           TypeSize;


  Manufacturer = TKN2STR(DMI_MANUFACTURER);
  ManuStrLen = AsciiStrLen(Manufacturer);
  ASSERT (ManuStrLen <= SMBIOS_STRING_MAX_LENGTH);

  Product = TKN2STR(TKN_BOARD_ID);
  ProductStrLen = AsciiStrLen(Product);
  ASSERT (ProductStrLen <= SMBIOS_STRING_MAX_LENGTH);

  Version = "1";
  VerStrLen = AsciiStrLen(Version);
  ASSERT (VerStrLen <= SMBIOS_STRING_MAX_LENGTH);

  SerialNumber = TKN2STR(DMI_NOT_SET_STR);
  SerialNumStrLen = AsciiStrLen(SerialNumber);
  ASSERT (SerialNumStrLen <= SMBIOS_STRING_MAX_LENGTH);

  AssertTag = TKN2STR(DMI_NOT_SET_STR);
  AssertTagStrLen = AsciiStrLen(AssertTag);
  ASSERT (AssertTagStrLen <= SMBIOS_STRING_MAX_LENGTH);

  Chassis = "Chassis Location";
  ChassisStrLen = AsciiStrLen(Chassis);
  ASSERT(ChassisStrLen <= SMBIOS_STRING_MAX_LENGTH);

  TypeSize = sizeof(SMBIOS_TABLE_TYPE2) + ManuStrLen + 1
                                        + ProductStrLen + 1
                                        + VerStrLen + 1
                                        + SerialNumStrLen + 1
                                        + AssertTagStrLen + 1
                                        + ChassisStrLen + 1
                                        + 1;
  SmbiosRecord = AllocatePool(TypeSize);
  ZeroMem(SmbiosRecord, TypeSize);

  SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof(SMBIOS_TABLE_TYPE2) - sizeof(SmbiosRecord->ContainedObjectHandles);
  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;
  SmbiosRecord->Manufacturer      = 1;
  SmbiosRecord->ProductName       = 2;
  SmbiosRecord->Version           = 3;
  SmbiosRecord->SerialNumber      = 4;
  SmbiosRecord->AssetTag          = 5;
  SmbiosRecord->LocationInChassis = 6;
  SmbiosRecord->FeatureFlag.Motherboard        = 1;
  SmbiosRecord->FeatureFlag.Replaceable        = 1;	
  SmbiosRecord->ChassisHandle                  = 0;
  SmbiosRecord->BoardType                      = BaseBoardTypeMotherBoard;
  SmbiosRecord->NumberOfContainedObjectHandles = 0;

  Str8  = (CHAR8*)(SmbiosRecord + 1) - 2;   // "ContainedObjectHandles" not existed.
  AsciiStrCpy(Str8, Manufacturer);
  Str8 += ManuStrLen + 1;
  AsciiStrCpy(Str8, Product);
  Str8 += ProductStrLen + 1;
  AsciiStrCpy(Str8, Version);
  Str8 += VerStrLen	+ 1;
  AsciiStrCpy(Str8, SerialNumber);
  Str8 += SerialNumStrLen	+ 1;
  AsciiStrCpy(Str8, AssertTag);
  Str8 += AssertTagStrLen	+ 1;
  AsciiStrCpy(Str8, Chassis);

  Status = AddSmbiosRecord(Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*)SmbiosRecord);
  FreePool(SmbiosRecord);
  return Status;
}



STATIC EFI_STATUS AddSmbiosType0(EFI_SMBIOS_PROTOCOL *Smbios)
{
  CHAR8                 *Str8;
  UINTN                 VendorStrLen;
  UINTN                 VerStrLen;
  UINTN                 DateStrLen;
  CHAR8                 *Version;
  CHAR8                 *ReleaseDate;
  EFI_STATUS            Status;
  CHAR8                 *BiosVerdorStr = NULL;
  SMBIOS_TABLE_TYPE0    *SmbiosRecord = NULL;
  EFI_SMBIOS_HANDLE     SmbiosHandle;
  CHAR16                *FirmwareVendor;
  UINTN                 FwVdrLen;


  FirmwareVendor = (CHAR16*)PcdGetPtr(PcdFirmwareVendor);
  FwVdrLen = StrLen(FirmwareVendor);
  if(FwVdrLen == 0){
    FirmwareVendor = L"ByoSoft";
    FwVdrLen = StrLen(FirmwareVendor);    
  }  
  BiosVerdorStr = AllocatePool(FwVdrLen+1);
  if(BiosVerdorStr == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }  
  UnicodeStrToAsciiStr(FirmwareVendor, BiosVerdorStr);

  VendorStrLen = AsciiStrLen(BiosVerdorStr);
  if (VendorStrLen > SMBIOS_STRING_MAX_LENGTH) {
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }

  Version = TKN2VERSTR(TKN_BIOS_MAJOR_VER, TKN_BIOS_MINOR_VER);
  VerStrLen = AsciiStrLen(Version);
  if (VerStrLen > SMBIOS_STRING_MAX_LENGTH) {
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }

  ReleaseDate = TKN_BUILD_RLSDATE_STR;
  DateStrLen = AsciiStrLen(ReleaseDate);
  if (DateStrLen > SMBIOS_STRING_MAX_LENGTH) {
    Status = EFI_UNSUPPORTED;
    goto ProcExit;
  }

  SmbiosRecord = AllocateZeroPool(sizeof (SMBIOS_TABLE_TYPE0) + VendorStrLen + 1 + VerStrLen + 1 + DateStrLen + 1 + 1);
  if(SmbiosRecord == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }  

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_BIOS_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE0);

  SmbiosRecord->Hdr.Handle  = 0;
  SmbiosRecord->Vendor      = 1;  
  SmbiosRecord->BiosVersion = 2;
  SmbiosRecord->BiosSegment = 0xF000;
  SmbiosRecord->BiosReleaseDate = 3;
  SmbiosRecord->BiosSize = (UINT8)((PcdGet32(PcdFlashAreaSize)>>16)-1);

// Bit7	  PCI is supported - 1 (Yes)	
// Bit11	BIOS is Upgradeable (Flash) - 1 (Yes)
// Bit12	BIOS shadowing is allowed - 1 (Yes)
// Bit15	Boot from CD is supported - 1 (Yes)
// Bit16	Selectable Boot is supported - 1 (Yes)
// Bit17	BIOS ROM is socketed - 1 (Yes)
// Bit19	EDD (Enhanced Disk Drive) Specification is supported - 1 (Yes)
// Bit26	Int 5h, Print Screen Service is supported - 1 (Yes)
// Bit27	Int 9h, 8042 Keyboard services are supported - 1 (Yes)
// Bit28	Int 14h, Serial Services are supported - 1 (Yes)
  SmbiosRecord->BiosCharacteristics.PciIsSupported              = 1;
  SmbiosRecord->BiosCharacteristics.BiosIsUpgradable            = 1;
  SmbiosRecord->BiosCharacteristics.BiosShadowingAllowed        = 1;
  SmbiosRecord->BiosCharacteristics.BootFromCdIsSupported       = 1;
  SmbiosRecord->BiosCharacteristics.SelectableBootIsSupported   = 1;
  SmbiosRecord->BiosCharacteristics.RomBiosIsSocketed           = 1;
  SmbiosRecord->BiosCharacteristics.EDDSpecificationIsSupported = 1;
  SmbiosRecord->BiosCharacteristics.PrintScreenIsSupported      = 1;
  SmbiosRecord->BiosCharacteristics.Keyboard8042IsSupported     = 1;
  SmbiosRecord->BiosCharacteristics.SerialIsSupported           = 1;

//Bit0	ACPI supported - 1 (Yes)
//Bit1	USB Legacy is supported - 1 (Yes)
  SmbiosRecord->BIOSCharacteristicsExtensionBytes[0] = 3;

//Bit0	BIOS Boot Specification supported - 1 (Yes)
//Bit1	Function key-initiated Network Service boot supported - 0 (No)
//Bit2	Enable Targeted Content Distribution - 0 (No)
//Bit3	UEFI Specification is supported - 0 (No)
//Bit4	SMBIOS table describes a virtual machine - 0 (No)
  SmbiosRecord->BIOSCharacteristicsExtensionBytes[1] = 0xD;

  SmbiosRecord->SystemBiosMajorRelease = (UINT8)AsciiStrDecimalToUintn(TKN2STR(TKN_BIOS_MAJOR_VER));
  SmbiosRecord->SystemBiosMinorRelease = (UINT8)AsciiStrDecimalToUintn(TKN2STR(TKN_BIOS_MINOR_VER));
  SmbiosRecord->EmbeddedControllerFirmwareMajorRelease = 0xFF;
  SmbiosRecord->EmbeddedControllerFirmwareMinorRelease = 0xFF;

  Str8 = (CHAR8 *)(SmbiosRecord + 1);
  CopyMem(Str8, BiosVerdorStr, VendorStrLen);
  Str8 += VendorStrLen + 1;
  CopyMem(Str8, Version, VerStrLen);
  Str8 += VerStrLen + 1;
  CopyMem(Str8, ReleaseDate, DateStrLen);

  Status = AddSmbiosRecord(Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*)SmbiosRecord);

ProcExit:
  if(SmbiosRecord!=NULL){FreePool(SmbiosRecord);}
  if(BiosVerdorStr!=NULL){FreePool(BiosVerdorStr);}
  return Status;
}



STATIC EFI_STATUS AddSmbiosType32(EFI_SMBIOS_PROTOCOL *Smbios)
{
  EFI_STATUS                         Status;
  EFI_SMBIOS_HANDLE                  SmbiosHandle;
  SMBIOS_TABLE_TYPE32                *SmbiosRecord;

  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE32) + 2);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE32) + 2);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE32);

  SmbiosRecord->Hdr.Handle = 0;  
  SmbiosRecord->BootStatus = 0;

  Status = AddSmbiosRecord(Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);

  FreePool(SmbiosRecord);
  return Status;
}




STATIC EFI_STATUS AddSmbiosType3(EFI_SMBIOS_PROTOCOL *Smbios)
{
  EFI_STATUS                      Status;
  CHAR8                           *Str8;
  UINTN                           ManuStrLen;
  UINTN                           VerStrLen;
  UINTN                           AssertTagStrLen;
  UINTN                           SerialNumStrLen;
  CHAR8                           *Manufacturer;
  CHAR8                           *Version;
  CHAR8                           *SerialNumber;
  CHAR8                           *AssertTag;
  EFI_SMBIOS_HANDLE               SmbiosHandle;
  SMBIOS_TABLE_TYPE3              *SmbiosRecord;


  Manufacturer = "Chassis Manufacturer";
  ManuStrLen = AsciiStrLen(Manufacturer);
  ASSERT(ManuStrLen <= SMBIOS_STRING_MAX_LENGTH);

  Version = "A1";
  VerStrLen = AsciiStrLen(Version);
  ASSERT(VerStrLen <= SMBIOS_STRING_MAX_LENGTH);

  SerialNumber = "                    ";
  SerialNumStrLen = AsciiStrLen(SerialNumber);
  ASSERT(SerialNumStrLen <= SMBIOS_STRING_MAX_LENGTH);

  AssertTag = TKN2STR(DMI_NOT_SET_STR);
  AssertTagStrLen = AsciiStrLen(AssertTag);
  ASSERT(AssertTagStrLen <= SMBIOS_STRING_MAX_LENGTH);

  SmbiosRecord = AllocateZeroPool(sizeof(SMBIOS_TABLE_TYPE3) + ManuStrLen + 1  + VerStrLen + 1 + SerialNumStrLen + 1 + AssertTagStrLen + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE3);

  SmbiosRecord->Hdr.Handle   = 0;  
  SmbiosRecord->Manufacturer = 1;  
  SmbiosRecord->Type         = MiscChassisTypeDeskTop;
  SmbiosRecord->Version      = 2;  
  SmbiosRecord->SerialNumber = 3;  
  SmbiosRecord->AssetTag     = 4;
	
  SmbiosRecord->BootupState      = ChassisStateOther;
  SmbiosRecord->PowerSupplyState = ChassisStateOther;
  SmbiosRecord->ThermalState     = ChassisStateOther;
  SmbiosRecord->SecurityStatus   = ChassisSecurityStatusOther;

  Str8 = (CHAR8 *)(SmbiosRecord + 1);
  AsciiStrCpy(Str8, Manufacturer);
  Str8 += ManuStrLen + 1;
  AsciiStrCpy(Str8, Version);
  Str8 += VerStrLen + 1;
  AsciiStrCpy(Str8, SerialNumber);
  Str8 += SerialNumStrLen + 1;
  AsciiStrCpy(Str8, AssertTag);

  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);

  FreePool(SmbiosRecord);
  return Status;
}



STATIC EFI_STATUS AddSmbiosType11(EFI_SMBIOS_PROTOCOL *Smbios)
{
  UINTN                    OemStrLen;
  CHAR8                    *OptionalStrStart;
  EFI_STATUS               Status;
  CHAR8                    *OemStr;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  SMBIOS_TABLE_TYPE11      *SmbiosRecord;


  OemStr = "System Language String";
  OemStrLen = AsciiStrLen(OemStr);
  ASSERT (OemStrLen <= SMBIOS_STRING_MAX_LENGTH);

  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE11) + OemStrLen + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE11) + OemStrLen + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_OEM_STRINGS;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE11);

  SmbiosRecord->Hdr.Handle = 0;  
  SmbiosRecord->StringCount = 1;
  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  AsciiStrCpy(OptionalStrStart, OemStr);

  Status = AddSmbiosRecord(Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*)SmbiosRecord);

  FreePool(SmbiosRecord);
  return Status;
}







EFI_STATUS AddSmbiosType8(EFI_SMBIOS_PROTOCOL *Smbios)
{
  CHAR8                *OptionalStrStart;
  UINTN                InternalRefStrLen;
  UINTN                ExternalRefStrLen;  
  EFI_STATUS           Status = EFI_SUCCESS;
  SMBIOS_TABLE_TYPE8   *SmbiosRecord;
  EFI_SMBIOS_HANDLE    SmbiosHandle;
  UINTN                Index;

  
  for(Index=0;Index<sizeof(gType8ContentList)/sizeof(gType8ContentList[0]);Index++){
    InternalRefStrLen = AsciiStrLen(gType8ContentList[Index].IRD_Str);
    ASSERT (InternalRefStrLen <= SMBIOS_STRING_MAX_LENGTH);

    ExternalRefStrLen = AsciiStrLen(gType8ContentList[Index].ERD_Str);
    ASSERT (InternalRefStrLen <= SMBIOS_STRING_MAX_LENGTH);

    SmbiosRecord = AllocateZeroPool(sizeof (SMBIOS_TABLE_TYPE8) + InternalRefStrLen + 1 + ExternalRefStrLen + 1 + 1);

    SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE8);

    SmbiosRecord->Hdr.Handle = 0;  
    SmbiosRecord->InternalReferenceDesignator = 1;
    SmbiosRecord->InternalConnectorType = gType8ContentList[Index].IntConnType;
    SmbiosRecord->ExternalReferenceDesignator = 2;
    SmbiosRecord->ExternalConnectorType = gType8ContentList[Index].ExtConnType;
    SmbiosRecord->PortType = gType8ContentList[Index].PortType;
    
    OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
    AsciiStrCpy(OptionalStrStart, gType8ContentList[Index].IRD_Str);
    AsciiStrCpy(OptionalStrStart + InternalRefStrLen + 1, gType8ContentList[Index].ERD_Str);

    Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    FreePool(SmbiosRecord);
  }
	
  return Status;
}




STATIC EFI_STATUS AddSmbiosType23(EFI_SMBIOS_PROTOCOL *Smbios)
{
  EFI_STATUS               Status;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  SMBIOS_TABLE_TYPE23      *SmbiosRecord;

  SmbiosRecord = AllocateZeroPool(sizeof (SMBIOS_TABLE_TYPE23) + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_SYSTEM_RESET;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE23);

  SmbiosRecord->Hdr.Handle    = 0;  
  SmbiosRecord->Capabilities  = 0;
  SmbiosRecord->ResetCount    = 0;
  SmbiosRecord->ResetLimit    = 0;  
  SmbiosRecord->TimerInterval = 0;
  SmbiosRecord->Timeout       = 0;

  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);

  FreePool(SmbiosRecord);
  return Status;
}


STATIC EFI_STATUS AddSmbiosType13(EFI_SMBIOS_PROTOCOL *Smbios)
{
  EFI_STATUS               Status;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  SMBIOS_TABLE_TYPE13      *SmbiosRecord;
  UINTN                    LangStrLeng;
  CHAR8                    *LangStr;

  LangStr = "en|US|iso8859-1";
  LangStrLeng = AsciiStrLen(LangStr);
  ASSERT(LangStrLeng <= SMBIOS_STRING_MAX_LENGTH);

  SmbiosRecord = AllocateZeroPool(sizeof (SMBIOS_TABLE_TYPE13) + LangStrLeng + 1 + 1);
  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE13);

  SmbiosRecord->Hdr.Handle = 0;
  SmbiosRecord->InstallableLanguages = 1;
  SmbiosRecord->Flags = 0;                            // long format
  SmbiosRecord->CurrentLanguages = 1;
  AsciiStrCpy((CHAR8 *)(SmbiosRecord + 1), LangStr);

  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);

  FreePool(SmbiosRecord);
  return Status;
}


STATIC EFI_STATUS AddSmbiosType1(EFI_SMBIOS_PROTOCOL *Smbios)
{
  CHAR8                             *Str8;
  UINTN                             ManuStrLen;
  UINTN                             VerStrLen;
  UINTN                             PdNameStrLen;
  UINTN                             SerialNumStrLen;
  UINTN                             SkuNumStrLen;
  UINTN                             FamilyStrLen;
  UINTN                             TotalSize;
  EFI_STATUS                        Status;
  CHAR8                             *Manufacturer;
  CHAR8                             *ProductName;
  CHAR8                             *Version;
  CHAR8                             *SerialNumber;
  CHAR8                             *SkuNumber;
  CHAR8                             *Family;
  EFI_SMBIOS_HANDLE                 SmbiosHandle;
  SMBIOS_TABLE_TYPE1                *SmbiosRecord;
// {A7199A69-E522-46bf-BE96-3F49BC97F755}  
  STATIC EFI_GUID gMyTestSmbiosType1Guid = { 0xa7199a69, 0xe522, 0x46bf, { 0xbe, 0x96, 0x3f, 0x49, 0xbc, 0x97, 0xf7, 0x55}};


  Manufacturer = TKN2STR(DMI_MANUFACTURER);
  ManuStrLen = AsciiStrLen(Manufacturer);
  ASSERT (ManuStrLen <= SMBIOS_STRING_MAX_LENGTH);

  ProductName = TKN2STR(TKN_BOARD_ID);
  PdNameStrLen = AsciiStrLen(ProductName);
  ASSERT (PdNameStrLen <= SMBIOS_STRING_MAX_LENGTH);

  Version = TKN2STR(TKN_BIOS_VER);
  VerStrLen = AsciiStrLen(Version);
  ASSERT (VerStrLen <= SMBIOS_STRING_MAX_LENGTH);

  SerialNumber = TKN2STR(DMI_NOT_SET_STR);
  SerialNumStrLen = AsciiStrLen(SerialNumber);
  ASSERT (SerialNumStrLen <= SMBIOS_STRING_MAX_LENGTH);

  SkuNumber = TKN2STR(DMI_NOT_SET_STR);
  SkuNumStrLen = AsciiStrLen(SkuNumber);
  ASSERT(SkuNumStrLen <= SMBIOS_STRING_MAX_LENGTH);

  Family = TKN2STR(DMI_NOT_SET_STR);
  FamilyStrLen = AsciiStrLen(Family);
  ASSERT(FamilyStrLen <= SMBIOS_STRING_MAX_LENGTH);

  TotalSize = sizeof (SMBIOS_TABLE_TYPE1) + ManuStrLen + 1 + PdNameStrLen + 1 + VerStrLen + 1 +
              SerialNumStrLen + 1 + SkuNumStrLen + 1 + FamilyStrLen + 1 + 1;
  SmbiosRecord = AllocateZeroPool(TotalSize);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_SYSTEM_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE1);

  SmbiosRecord->Hdr.Handle   = 0;
  SmbiosRecord->Manufacturer = 1;
  SmbiosRecord->ProductName  = 2;
  SmbiosRecord->Version      = 3;
  SmbiosRecord->SerialNumber = 4;
  SmbiosRecord->SKUNumber    = 5;
  SmbiosRecord->Family       = 6;

#ifndef MDEPKG_NDEBUG
  CopyMem(&SmbiosRecord->Uuid, &gMyTestSmbiosType1Guid, sizeof(SmbiosRecord->Uuid));
#else
  SetMem(&SmbiosRecord->Uuid, sizeof(SmbiosRecord->Uuid), 0xFF);
#endif

  SmbiosRecord->WakeUpType = SystemWakeupTypePowerSwitch;

  Str8 = (CHAR8 *)(SmbiosRecord + 1);
  CopyMem(Str8, Manufacturer, ManuStrLen);
  Str8 += ManuStrLen + 1;
  CopyMem(Str8, ProductName, PdNameStrLen);
  Str8 += PdNameStrLen + 1;
  CopyMem(Str8, Version, VerStrLen);
  Str8 += VerStrLen + 1;
  CopyMem(Str8, SerialNumber, SerialNumStrLen);
  Str8 += SerialNumStrLen + 1;
  CopyMem(Str8, SkuNumber, SkuNumStrLen);
  Str8 += SkuNumStrLen + 1;
  CopyMem(Str8, Family, FamilyStrLen);

  Status = AddSmbiosRecord(Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);

  FreePool(SmbiosRecord);
  return Status;
}




STATIC EFI_STATUS AddSmbiosType12(EFI_SMBIOS_PROTOCOL *Smbios)
{
  UINTN                             OptStrLen;
  CHAR8                             *OptionString;
  EFI_STATUS                        Status;
  EFI_SMBIOS_HANDLE                 SmbiosHandle;
  SMBIOS_TABLE_TYPE12               *SmbiosRecord;


  OptionString = "System Option String";
  OptStrLen = AsciiStrLen(OptionString);
  ASSERT(OptStrLen <= SMBIOS_STRING_MAX_LENGTH);
 
  SmbiosRecord = AllocateZeroPool(sizeof (SMBIOS_TABLE_TYPE12) + OptStrLen + 1 + 1);
  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE12);
  SmbiosRecord->Hdr.Handle = 0;  
  SmbiosRecord->StringCount = 1;
  AsciiStrCpy((CHAR8*) (SmbiosRecord + 1), OptionString);

  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);

  FreePool(SmbiosRecord);
  return Status;
}



STATIC EFI_STATUS AddSmbiosType9(EFI_SMBIOS_PROTOCOL *Smbios)
{
  UINTN                              SlotDesigStrLen;
  EFI_STATUS                         Status = EFI_SUCCESS;
  SMBIOS_TABLE_TYPE9                 *SmbiosRecord;
  EFI_SMBIOS_HANDLE                  SmbiosHandle;
  UINTN                              Index;
  SMBIOS_TYPE9_CONTENT               *Ctx;
  UINTN                              CtxCount;


  Ctx = gSmbiosType9ContentList;
  CtxCount = sizeof(gSmbiosType9ContentList)/sizeof(gSmbiosType9ContentList[0]);

  for(Index=0;Index<CtxCount;Index++){	

  SlotDesigStrLen = AsciiStrLen(Ctx[Index].SlotDesigStr);
  SmbiosRecord = AllocateZeroPool(sizeof (SMBIOS_TABLE_TYPE9) + SlotDesigStrLen + 1 + 1);
  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_SYSTEM_SLOTS;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE9);
  SmbiosRecord->Hdr.Handle = 0; 
  SmbiosRecord->SlotDesignation = 1;
  SmbiosRecord->SlotType         = Ctx[Index].SlotType;
  SmbiosRecord->SlotDataBusWidth = Ctx[Index].SlotbusWidth;
  SmbiosRecord->CurrentUsage     = Ctx[Index].SlotUsage;
  SmbiosRecord->SlotLength       = Ctx[Index].SlotLen;
  SmbiosRecord->SlotID           = Ctx[Index].SlotId;

  *(UINT8*)&SmbiosRecord->SlotCharacteristics1 = Ctx[Index].SlotChars;
  *(UINT8*)&SmbiosRecord->SlotCharacteristics2 = Ctx[Index].SlotChars2;

  SmbiosRecord->SegmentGroupNum = 0;
  SmbiosRecord->BusNum          = Ctx[Index].BusNo;
  SmbiosRecord->DevFuncNum      = Ctx[Index].DevFuncNo;

  AsciiStrCpy((CHAR8 *)(SmbiosRecord + 1), Ctx[Index].SlotDesigStr);
  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
  FreePool(SmbiosRecord);

  }
	
  return Status;
}





STATIC EFI_STATUS AddSmbiosType41(EFI_SMBIOS_PROTOCOL *Smbios)
{
  EFI_STATUS                         Status = EFI_SUCCESS;
  SMBIOS_TABLE_TYPE41                *SmbiosRecord;
  EFI_SMBIOS_HANDLE                  SmbiosHandle;
  UINTN                              Index;
  UINTN                              NameLen;
  SMBIOS_TYPE41_CONTENT              *Ctx;
  UINTN                              CtxCount;
  UINTN                              PciBase;
  

  Ctx      = gSmbiosType41ContentList;
  CtxCount = sizeof(gSmbiosType41ContentList)/sizeof(gSmbiosType41ContentList[0]);
  
  for(Index=0;Index<CtxCount;Index++){
    PciBase = PCI_DEV_MMBASE(Ctx[Index].BusNum, (Ctx[Index].DevFuncNum >> 3), (Ctx[Index].DevFuncNum&0x7));
    if(MmioRead16(PciBase) != 0xFFFF){
      Ctx[Index].DeviceType |= DMI_TYPE41_DEVTYPE_DEV_EN;
    }  
    
    NameLen = AsciiStrLen(Ctx[Index].Name);
    SmbiosRecord = AllocateZeroPool(sizeof (SMBIOS_TABLE_TYPE41) + NameLen + 1 + 1);
    SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_ONBOARD_DEVICES_EXTENDED_INFORMATION;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE41);
    SmbiosRecord->ReferenceDesignation = 1;
    SmbiosRecord->DeviceType         = Ctx[Index].DeviceType;
    SmbiosRecord->DeviceTypeInstance = Ctx[Index].DeviceTypeInstance;
    SmbiosRecord->SegmentGroupNum    = Ctx[Index].SegmentGroupNum;
    SmbiosRecord->BusNum             = Ctx[Index].BusNum;    
    SmbiosRecord->DevFuncNum         = Ctx[Index].DevFuncNum;  

    AsciiStrCpy((CHAR8 *)(SmbiosRecord + 1), Ctx[Index].Name);
    Status = AddSmbiosRecord(Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*)SmbiosRecord);
    FreePool(SmbiosRecord);    
  }  
  
  return Status;  
}





STATIC
EFI_STATUS
HandleType16 (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios,
  OUT EFI_SMBIOS_HANDLE    *Handle,
  IN  PLAT_DIMM_INFO       *DimmInfo  
  )
{
  SMBIOS_TABLE_TYPE16   *Type16Record;
  EFI_STATUS            Status = EFI_SUCCESS;
  EFI_SMBIOS_HANDLE     SmbiosHandle;


  Type16Record = AllocateZeroPool(sizeof(SMBIOS_TABLE_TYPE16)+2);
  if(Type16Record == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  Type16Record->Hdr.Type   = EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY;
  Type16Record->Hdr.Length = sizeof(SMBIOS_TABLE_TYPE16);
  Type16Record->Hdr.Handle = 0;
  Type16Record->Location   = MemoryArrayLocationSystemBoard;
  Type16Record->Use        = MemoryArrayUseSystemMemory;
  Type16Record->MemoryErrorCorrection = MemoryErrorCorrectionNone;
  Type16Record->MaximumCapacity = PLATFORM_MEMORY_MAX_CAP_GB << 20;  // unit in K
  Type16Record->MemoryErrorInformationHandle = 0xFFFE;
  Type16Record->NumberOfMemoryDevices = DimmInfo->DimmCount;

  Status = AddSmbiosRecord(Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*)Type16Record);
  if(!EFI_ERROR(Status)){
    *Handle = SmbiosHandle;
  }

ProcExit:
  if(Type16Record!=NULL){
    FreePool(Type16Record);
  }
  return Status;
}


STATIC CHAR8  gStrDevLocator[]   = {"DIMM0"};
STATIC CHAR8  gStrBankLocator[]  = {"BANK0"};
STATIC CHAR8  gStrManufacturer[] = {"Manufacturer0"};
STATIC CHAR8  gStrSerialNumber[] = {"00000000"};
STATIC CHAR8  gStrAssetTag[]     = {"2016Mem0"};
STATIC CHAR8  gPartNumber[]      = {"123456789012345678"};



//Device Locator : String number of the string that identifies the physically-labeled 
//                 socket or board position where the memory device is located  
//Bank Locator   : String number of the string that identifies the physically labeled 
//                 bank where the memory device is located 
//Manufacturer   : String number for the manufacturer of this memory device
//SerialNumber   : String number for the serial number of this memory device  
//Asset Tag      : String number for the asset tag of this memory device 
//Part Number    : String number for the part number of this memory device 

STATIC
EFI_STATUS
HandleType17_20 (
  IN  EFI_SMBIOS_PROTOCOL     *Smbios,
  IN  EFI_SMBIOS_HANDLE       Type16Handle,
  IN  EFI_SMBIOS_HANDLE       Type19Handle,  
  IN  PLAT_DIMM_INFO          *DimmInfo
  )
{
  SMBIOS_TABLE_TYPE17    *Type17Record = NULL;
  SMBIOS_TABLE_TYPE20    *Type20Record = NULL;	
  UINT16                 MemorySize;
  EFI_SMBIOS_HANDLE      Type17Handle;
  EFI_SMBIOS_HANDLE      Type20Handle;	
  EFI_STATUS             Status = EFI_SUCCESS;
  UINT16                 Index;
  UINTN                  Size;
  CHAR8                  *Str;
  UINTN                  StrLen;
  UINT32                 StartingAddress;	


//DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));
	
  Size   = sizeof(SMBIOS_TABLE_TYPE17) +
           sizeof(gStrDevLocator) +
           sizeof(gStrBankLocator) +
           sizeof(gStrManufacturer) +
           sizeof(gStrSerialNumber) +
           sizeof(gStrAssetTag) +
           sizeof(gPartNumber) +
           1;
  Type17Record = AllocateZeroPool(Size);
  if(Type17Record == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  Type20Record = AllocateZeroPool(sizeof(SMBIOS_TABLE_TYPE20) + 2);
  if(Type20Record == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  StartingAddress = 0;
  for(Index=0;Index<DimmInfo->DimmCount;Index++){
    MemorySize = DimmInfo->SpdInfo[Index].DimmSize;
    ZeroMem(Type17Record, Size);
    Type17Record->Hdr.Type   = EFI_SMBIOS_TYPE_MEMORY_DEVICE;
    Type17Record->Hdr.Length = sizeof(SMBIOS_TABLE_TYPE17);
    Type17Record->Hdr.Handle = 0;
    Type17Record->MemoryArrayHandle = Type16Handle;
    Type17Record->MemoryErrorInformationHandle = 0xFFFE;
    Type17Record->TotalWidth = 64;
    Type17Record->DataWidth  = 64;
    Type17Record->DeviceSet  = 0xFF;
    Type17Record->FormFactor = MemoryFormFactorDimm;
    Type17Record->TypeDetail.Synchronous = 1;
    Type17Record->DeviceLocator = 1;
    Type17Record->BankLocator   = 2;
    Type17Record->Manufacturer  = 3;
    Type17Record->SerialNumber  = 4;
    Type17Record->AssetTag      = 5;
    Type17Record->PartNumber    = 6;
    Type17Record->MemoryType = MemoryTypeDdr3;
    if(MemorySize){
      Type17Record->ConfiguredMemoryClockSpeed = DimmInfo->DimmFreq;
      Type17Record->Speed = DimmInfo->SpdInfo[Index].DimmSpeed;
    }
    if(MemorySize < 0x7FFF){	// 32G - 1M
      Type17Record->Size = MemorySize;
    }else{
      Type17Record->Size = 0x7FFF;
      Type17Record->ExtendedSize = MemorySize;
    }

    Str = (CHAR8*)Type17Record + sizeof(SMBIOS_TABLE_TYPE17);
    StrLen = AsciiStrLen(gStrDevLocator);
    AsciiStrCpy(Str, gStrDevLocator);
    Str[StrLen-1] = '0' + (UINT8)Index;
    Str += StrLen+1;

    StrLen = AsciiStrLen(gStrBankLocator);
    AsciiStrCpy(Str, gStrBankLocator);
    Str[StrLen-1] = '0' + (UINT8)Index;
    Str += StrLen+1;

    if(MemorySize == 0){
      Type17Record->Manufacturer  = 0;
      Type17Record->SerialNumber  = 0;
      Type17Record->AssetTag      = 0;
      Type17Record->PartNumber    = 0;
      
    } else {  
      StrLen = AsciiStrLen(gStrManufacturer);
      AsciiStrCpy(Str, gStrManufacturer);
      Str[StrLen-1] = '0' + (UINT8)Index;
      Str += StrLen+1;

      AsciiSPrint(gStrSerialNumber, sizeof(gStrSerialNumber), "%08X", DimmInfo->SpdInfo[Index].Sn);
      StrLen = AsciiStrLen(gStrSerialNumber);
      AsciiStrCpy(Str, gStrSerialNumber);
      Str += StrLen+1;

      StrLen = AsciiStrLen(gStrAssetTag);
      AsciiStrCpy(Str, gStrAssetTag);
      Str[StrLen-1] = '0' + (UINT8)Index;
      Str += StrLen+1;
      
      StrLen = AsciiStrLen(DimmInfo->SpdInfo[Index].PartNo);
      AsciiStrCpy(Str, DimmInfo->SpdInfo[Index].PartNo);
      Str += StrLen+1;

      ASSERT((UINTN)Str+1 <= (UINTN)Type17Record+Size);      
    }

    Status = AddSmbiosRecord(Smbios, &Type17Handle, (EFI_SMBIOS_TABLE_HEADER*)Type17Record);
    ASSERT(!EFI_ERROR(Status));
    if(MemorySize){
      Type20Record->Hdr.Type   = EFI_SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS;
      Type20Record->Hdr.Length = sizeof(SMBIOS_TABLE_TYPE20);
      Type20Record->Hdr.Handle = 0;
      Type20Record->StartingAddress    = StartingAddress;
      Type20Record->EndingAddress      = StartingAddress + (((UINT32)MemorySize)<<10) - 1;	// MB -> KB
      Type20Record->MemoryDeviceHandle = Type17Handle;
      Type20Record->MemoryArrayMappedAddressHandle = Type19Handle;
      Type20Record->PartitionRowPosition = 0xFF;
      Type20Record->InterleavePosition   = 0xFF;
      Type20Record->InterleavedDataDepth = 0xFF;
      Type20Record->ExtendedStartingAddress = 0;
      Type20Record->ExtendedEndingAddress   = 0;
      Status = AddSmbiosRecord(Smbios, &Type20Handle, (EFI_SMBIOS_TABLE_HEADER*)Type20Record);
    }
    StartingAddress += ((UINT32)MemorySize) << 10;		// MB -> KB
  }

ProcExit:
  if(Type17Record!=NULL){
    FreePool(Type17Record);
  }
  if(Type20Record!=NULL){
    FreePool(Type20Record);
  }	
  return Status;
}


STATIC
EFI_STATUS
HandleType19 (
  IN  EFI_SMBIOS_PROTOCOL   *Smbios,
  IN  EFI_SMBIOS_HANDLE     Type16Handle,
  OUT EFI_SMBIOS_HANDLE     *Handle,
  IN  PLAT_DIMM_INFO        *DimmInfo
  )
{
  EFI_STATUS                      Status;
  SMBIOS_TABLE_TYPE19             *Type19Record;
  EFI_SMBIOS_HANDLE               SmbiosHandle;

  Type19Record = AllocateZeroPool(sizeof(SMBIOS_TABLE_TYPE19) + 2);
  if(Type19Record == NULL){
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;
  }

  Type19Record->Hdr.Type   = EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS;
  Type19Record->Hdr.Length = sizeof(SMBIOS_TABLE_TYPE19);
  Type19Record->Hdr.Handle = 0;
  Type19Record->StartingAddress   = 0;
  Type19Record->EndingAddress     = (UINT32)LShiftU64(DimmInfo->DimmTotalSizeMB, 10) - 1;
  Type19Record->MemoryArrayHandle = Type16Handle;
  Type19Record->PartitionWidth    = (UINT8)DimmInfo->DimmCount;
  Status = AddSmbiosRecord(Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*)Type19Record);
  if(!EFI_ERROR(Status)){
    *Handle = SmbiosHandle;
  }
ProcExit:
  if(Type19Record!=NULL){
    FreePool(Type19Record);
  }
  return Status;
}



STATIC UINT8 GetAssociativityValue(UINT8 Associativity)
{
  UINT8  Value;
  
  switch(Associativity){
    case 2:
      Value = 4;
      break;
    case 4:
      Value = 5;
      break;
    case 8:
      Value = 7;
      break;
    case 16:
      Value = 8;
      break;
    case 12:
      Value = 9;
      break;
    case 24:
      Value = 10;
      break;
    case 32:
      Value = 11;
      break;
    case 48:
      Value = 12;
      break;
    case 64:
      Value = 13;
      break;
    case 20:
      Value = 14;
      break;
    default:
      Value = 2;
      break;
  }
  return Value;
}




EFI_STATUS
CpuSmbiosDxe (
  IN EFI_SMBIOS_PROTOCOL *Smbios
  )
{
  EFI_STATUS            Status;
  SMBIOS_TABLE_TYPE7    *Type7;
  SMBIOS_TABLE_TYPE4    *Type4;  
  UINT8                 *TypeData;
  UINTN                 Size;
  CHAR8                 *StrCacheL1;
  CHAR8                 *StrCpuSocket;
  CHAR8                 *StrCpuMft;
  CHAR8                 *Str8;
  ACPU_CACHE_INFO       CacheInfo[ACPU_CACHE_INFO_NUM];
  UINTN                 Index;
  UINT16                CacheSize[3];     // unit in KB
  UINT8                 Associativity[3];
  EFI_SMBIOS_HANDLE     CacheHandle[3];
  UINT8                 CacheType[3];
  EFI_SMBIOS_HANDLE     SmbiosHandle;  
  CHAR8                 CpuBrandString[48+1];
  CHAR8                 *CpuBrandStr;
  UINT32                ProcessorId[2];
  ACPU_SPEED_INFO       SpeedInfo;
  
  
  CacheHandle[0] = 0xFFFF;
  CacheHandle[1] = 0xFFFF;  
  CacheHandle[2] = 0xFFFF;  
  CacheSize[0]   = 0;
  CacheSize[1]   = 0;
  CacheSize[2]   = 0;
  Associativity[0] = 0;
  Associativity[1] = 0;
  Associativity[2] = 0; 
  
  StrCacheL1   = "L1-Cache";
  StrCpuSocket = "CPU 1";
  StrCpuMft    = "Zhaoxin";
  
  gPtAsiaCpu->GetCpuCacheInfo(CacheInfo);
  gPtAsiaCpu->GetCpuBrandString(CpuBrandString);
  gPtAsiaCpu->GetCpuSpeedInfo(&SpeedInfo); 
  
  CpuBrandStr = CpuBrandString;
  Index = 0;
  while(CpuBrandStr[Index]!=0){
    if(CpuBrandStr[Index] == ' '){
      CpuBrandStr++;
    } else {
      break;
    }
  }
  
  for(Index=0;Index<ACPU_CACHE_INFO_NUM;Index++){
    if(CacheInfo[Index].Level == ACPU_CACHE_LEVEL_L1 && CacheInfo[Index].Type == ACPU_CACHE_TYPE_CODE){
      CacheSize[0]     = CacheInfo[Index].CacheSize;
      Associativity[0] = CacheInfo[Index].Associativity;
      CacheType[0]     = CacheInfo[Index].Type;
    } else if(CacheInfo[Index].Level == ACPU_CACHE_LEVEL_L2){
      CacheSize[1]     = CacheInfo[Index].CacheSize;
      Associativity[1] = CacheInfo[Index].Associativity; 
      CacheType[1]     = CacheInfo[Index].Type;      
    } else if(CacheInfo[Index].Level == ACPU_CACHE_LEVEL_L3){
      CacheSize[2]     = CacheInfo[Index].CacheSize;
      Associativity[2] = CacheInfo[Index].Associativity;  
      CacheType[2]     = CacheInfo[Index].Type;      
    }
  }
  
// Type 7  
  Size = sizeof(*Type7) + AsciiStrSize(StrCacheL1) + 1;
  TypeData = AllocatePool(Size);
  ASSERT(TypeData != NULL);
  ZeroMem(TypeData, Size);
  Type7 = (SMBIOS_TABLE_TYPE7*)TypeData;
  for(Index=0; Index<3; Index++){
    if(CacheSize[Index] == 0){
      continue;
    }
    Type7->Hdr.Type = EFI_SMBIOS_TYPE_CACHE_INFORMATION;
    Type7->Hdr.Handle = 0;
    Type7->Hdr.Length = sizeof(SMBIOS_TABLE_TYPE7);    
    Type7->SocketDesignation = 1;                             // first string.
    Type7->CacheConfiguration = BIT8 + BIT7 + (UINT8)Index;   // Write Back, Enabled, Internal, Not Socketed, Cache Level N
    Type7->MaximumCacheSize = CacheSize[Index];
    Type7->InstalledSize    = CacheSize[Index];
    Type7->SupportedSRAMType.Asynchronous = 1;
    Type7->SupportedSRAMType.PipelineBurst = 1;
    Type7->SupportedSRAMType.Burst = 1;
    Type7->SupportedSRAMType.Other = 1;
    CopyMem(&Type7->CurrentSRAMType, &Type7->SupportedSRAMType, sizeof(Type7->SupportedSRAMType));
    Type7->CacheSpeed = 0;      // unknown
    Type7->ErrorCorrectionType = CacheErrorSingleBit;
    Type7->SystemCacheType     = CacheType[Index];
    Type7->Associativity       = GetAssociativityValue(Associativity[Index]);
    Str8 = (CHAR8*)(Type7+1);
    AsciiStrCpy(Str8, StrCacheL1);
    Str8[1] = '1' + (UINT8)Index;
    
    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    Status = Smbios->Add(Smbios, NULL, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*)Type7);
    ASSERT(!EFI_ERROR(Status));
    CacheHandle[Index] = SmbiosHandle;
  }
  
  FreePool(TypeData);
  TypeData = NULL;
  Type7    = NULL;

  
  

  
// Type 4
  AsmCpuid(1, &ProcessorId[0], NULL, NULL, &ProcessorId[1]);
  Size = sizeof(*Type4) + AsciiStrSize(StrCpuSocket) + AsciiStrSize(StrCpuMft)
                        + AsciiStrSize(CpuBrandStr)  + 1;
  TypeData = AllocatePool(Size);
  ASSERT(TypeData != NULL);
  ZeroMem(TypeData, Size);
  Type4 = (SMBIOS_TABLE_TYPE4*)TypeData;
  Type4->Hdr.Type = EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION;
  Type4->Hdr.Handle = 0;  
  Type4->Hdr.Length = sizeof(SMBIOS_TABLE_TYPE4); 
  Type4->Socket = 1;  
  Type4->ProcessorType = CentralProcessor;
  Type4->ProcessorFamily = ProcessorFamilyOther;
  Type4->ProcessorManufacture = 2;
  CopyMem(&Type4->ProcessorId, &ProcessorId[0], sizeof(Type4->ProcessorId));
  Type4->ProcessorVersion = 3;
  *(UINT8*)(&Type4->Voltage) = BIT7 + 12;     // /1.2v
  Type4->ExternalClock = (UINT16)SpeedInfo.CPUBusSpeed;
  Type4->MaxSpeed      = (UINT16)(SpeedInfo.CPUBusSpeed * (SpeedInfo.CurrentBusRatio>>1));
  Type4->CurrentSpeed  = Type4->MaxSpeed;
  Type4->Status        = BIT6 + BIT0;     // CPU Socket Populated, CPU Enabled
  Type4->ProcessorUpgrade = ProcessorUpgradeNone;
  Type4->L1CacheHandle = CacheHandle[0];
  Type4->L2CacheHandle = CacheHandle[1];
  Type4->L3CacheHandle = CacheHandle[2];   
  Type4->SerialNumber  = 0;
  Type4->AssetTag      = 0;
  Type4->PartNumber    = 0;
  Type4->CoreCount        = gPtAsiaCpu->GetCpuCores();
  Type4->EnabledCoreCount = Type4->CoreCount;
  Type4->ThreadCount      = Type4->CoreCount;
// Power/Performance Control, Enhanced Virtualization, Execute Protection, Multi-Core, 64-bit Capable
  Type4->ProcessorCharacteristics = BIT7 + BIT6 + BIT5 + BIT3 + BIT2;
  Type4->ProcessorFamily2 = ProcessorFamilyViaNano;
  
  Str8 = (CHAR8*)(Type4+1);
  AsciiStrCpy(Str8, StrCpuSocket);
  Str8 += AsciiStrLen(StrCpuSocket) + 1;
  AsciiStrCpy(Str8, StrCpuMft);
  Str8 += AsciiStrLen(StrCpuMft) + 1;
  AsciiStrCpy(Str8, CpuBrandStr); 

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add(Smbios, NULL, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*)Type4);
  ASSERT(!EFI_ERROR(Status));  

  FreePool(TypeData);
  TypeData = NULL;
  Type4    = NULL;  
  
  return Status;
}






VOID
SmbiosCallback (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_STATUS             Status;
  EFI_SMBIOS_PROTOCOL    *Smbios;  
  EFI_SMBIOS_HANDLE      Type16Handle;
  EFI_SMBIOS_HANDLE      Type19Handle;
  PLAT_DIMM_INFO         *DimmInfo;

  Status = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID**)&Smbios);
  if (EFI_ERROR(Status)) {
    return;
  }

  gBS->CloseEvent(Event);


  DimmInfo = (PLAT_DIMM_INFO*)GetPlatformDimmInfo();

  Status = AddSmbiosType0(Smbios);  
  ASSERT(!EFI_ERROR(Status));  
  
  Status = AddSmbiosType1(Smbios); 
  ASSERT(!EFI_ERROR(Status));
  Status = AddSmbiosType2(Smbios);
  ASSERT(!EFI_ERROR(Status));  
  Status = AddSmbiosType3(Smbios);
  ASSERT(!EFI_ERROR(Status));  
  Status = AddSmbiosType8(Smbios);  
  ASSERT(!EFI_ERROR(Status));  
  Status = AddSmbiosType9(Smbios);  
  ASSERT(!EFI_ERROR(Status));  
  Status = AddSmbiosType11(Smbios);  
  ASSERT(!EFI_ERROR(Status));  
  Status = AddSmbiosType12(Smbios);  
  ASSERT(!EFI_ERROR(Status));  
  Status = AddSmbiosType13(Smbios);
  ASSERT(!EFI_ERROR(Status));
  Status = AddSmbiosType23(Smbios);  
  ASSERT(!EFI_ERROR(Status));  
  Status = AddSmbiosType32(Smbios);
  ASSERT(!EFI_ERROR(Status));  
  Status = AddSmbiosType41(Smbios);
  ASSERT(!EFI_ERROR(Status));  
  Status = HandleType16(Smbios, &Type16Handle, DimmInfo);
  ASSERT(!EFI_ERROR(Status));
  Status = HandleType19(Smbios, Type16Handle, &Type19Handle, DimmInfo);
  ASSERT(!EFI_ERROR(Status));
  Status = HandleType17_20(Smbios, Type16Handle, Type19Handle, DimmInfo);
  ASSERT(!EFI_ERROR(Status));
  Status = CpuSmbiosDxe(Smbios);
  ASSERT(!EFI_ERROR(Status));
}




