

#include "FrontPage.h"
#include <SetupVariable.h>
#include <Protocol/BootLogo.h>
#include <Guid/TcmSetupCfgGuid.h>
#include <Guid/TcSetupCfgGuid.h>


extern EFI_GUID gEfiMainByoFormsetGuid;
extern EFI_GUID gEfiAdvanceByoFormsetGuid;
extern EFI_GUID gEfiChipsetByoFormsetGuid;  
extern EFI_GUID gEfiBootByoFormsetGuid;
extern EFI_GUID gEfiSecurityByoFormsetGuid;
extern EFI_GUID gEfiExitByoFormsetGuid;
extern EFI_GUID gEfiPowerByoFormsetGuid;
extern EFI_GUID gSecureBootConfigFormSetGuid;
extern EFI_GUID gTcgConfigFormSetGuid;
extern EFI_GUID gTcg2ConfigFormSetGuid;
extern VOID     *gMyFontBin;


EFI_GUID  gHddPasswordVendorGuid = 
  {0xd5fd1546, 0x22c5, 0x4c2e, { 0x96, 0x9f, 0x27, 0x3c, 0x0, 0x77, 0x10, 0x80}};

EFI_FORM_BROWSER2_PROTOCOL              *gFormBrowser2;
EFI_BYO_FORM_BROWSER_EXTENSION_PROTOCOL *gFormBrowserEx;






VOID
SignalEnterSetupEvent (
  VOID
  )
{
  EFI_HANDLE                 Handle;
  EFI_STATUS                 Status;

  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiSetupEnterGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
}





/**
  Whether The Tpm be Add to hii database.

**/
EFI_STATUS
SetSomeFormsetPresentStatus (
  VOID
  )
{
  EFI_STATUS                        Status;
  EFI_BYO_FORMSET_MANAGER_PROTOCOL  *FormsetManager;
  SETUP_VOLATILE_DATA               SetupVData;
  UINTN                             VariableSize;

	
  Status = gBS->LocateProtocol (
                  &gEfiByoFormsetManagerProtocolGuid,
                  NULL,
                  &FormsetManager
                  );
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "LocateProtocol(gEfiByoFormsetManagerProtocolGuid):%r\n", Status));
    return Status;
  }

  VariableSize = sizeof(SetupVData);
  ZeroMem(&SetupVData, VariableSize);	
  	
  if (FormsetManager->CheckFormset(&gTcgConfigFormSetGuid)){
    SetupVData.TpmFormsetPresent = 1;
  }
  if (FormsetManager->CheckFormset(&gTcg2ConfigFormSetGuid)){
    SetupVData.Tpm2FormsetPresent = 1;
  }
  if(FormsetManager->CheckFormset(&gHddPasswordVendorGuid)) {
    SetupVData.HdpFormsetPresent = 1;
  }
  if(FormsetManager->CheckFormset(&gSecureBootConfigFormSetGuid)) {
    SetupVData.SecureBootFormsetPresent = 1;
  }	
  if (FormsetManager->CheckFormset(&gTcmSetupConfigGuid)){
    SetupVData.TcmFormsetPresent = 1;
  }
  if (FormsetManager->CheckFormset(&gTrustedComputingSetupConfigGuid)){
    SetupVData.TcFormsetPresent = 1;
  }

  

  DEBUG((EFI_D_INFO, "TpmFormset : %d\n", SetupVData.TpmFormsetPresent));
  DEBUG((EFI_D_INFO, "Tpm2Formset: %d\n", SetupVData.Tpm2FormsetPresent));  
  DEBUG((EFI_D_INFO, "HdpFormset : %d\n", SetupVData.HdpFormsetPresent));	
  DEBUG((EFI_D_INFO, "SecureBoot : %d\n", SetupVData.SecureBootFormsetPresent));		
  DEBUG((EFI_D_INFO, "TcmFormset : %d\n", SetupVData.TcmFormsetPresent));	
  DEBUG((EFI_D_INFO, "TcFormset  : %d\n", SetupVData.TcFormsetPresent));

  gRT->SetVariable (
        L"SetupVolatileData",
        &gPlatformSetupVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS,
        VariableSize,
        &SetupVData
        );	

  return Status;
}


/**
  Insert All Formset to Byo Formset Manager Protocol.

**/
EFI_STATUS
RunByoFormset (
  VOID
  )
{
  EFI_STATUS    Status;
  EFI_BYO_FORMSET_MANAGER_PROTOCOL    *FormsetManager = NULL;
        
  DEBUG((EFI_D_INFO, "RunByoFormset()\n"));	

  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_USER_SETUP);
  
  Status = gBS->LocateProtocol (
                  &gEfiByoFormsetManagerProtocolGuid,
                  NULL,
                  &FormsetManager
                  );
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "RunByoFormset(), Locate Error :gEfiByoFormsetManagerProtocolGuid.\n"));
    return Status;
  }
  
  FormsetManager->Insert(FormsetManager, &gEfiMainByoFormsetGuid);
  FormsetManager->Insert(FormsetManager, &gEfiChipsetByoFormsetGuid);
  FormsetManager->Insert(FormsetManager, &gEfiAdvanceByoFormsetGuid);
  FormsetManager->Insert(FormsetManager, &gEfiPowerByoFormsetGuid);
  FormsetManager->Insert(FormsetManager, &gEfiSecurityByoFormsetGuid);
  FormsetManager->Insert(FormsetManager, &gEfiBootByoFormsetGuid);
  FormsetManager->Insert(FormsetManager, &gEfiExitByoFormsetGuid);

  SetSomeFormsetPresentStatus();
  FormsetManager->Run(FormsetManager, &gEfiMainByoFormsetGuid);
  return Status;
}



#define _FREE_NON_NULL(POINTER) \
  do{ \
    if((POINTER) != NULL) { \
      FreePool((POINTER)); \
      (POINTER) = NULL; \
    } \
  } while(FALSE)


EFI_GUID *gIgnoredFormsetGuidArray[] = {
  &gEfiMainByoFormsetGuid,
  &gEfiAdvanceByoFormsetGuid,
  &gEfiChipsetByoFormsetGuid,
  &gEfiPowerByoFormsetGuid,
  &gEfiBootByoFormsetGuid,
  &gEfiSecurityByoFormsetGuid,
  &gEfiExitByoFormsetGuid,
  &gTcgConfigFormSetGuid,
  &gTcg2ConfigFormSetGuid,
  &gTcmSetupConfigGuid,
  &gSecureBootConfigFormSetGuid,
  &gTrustedComputingSetupConfigGuid,
  &gHddPasswordVendorGuid
};  

BOOLEAN IsFormsetIgnore(EFI_GUID *Guid)
{
  UINTN  Index;
  UINTN  Count;

  Count = sizeof(gIgnoredFormsetGuidArray)/sizeof(gIgnoredFormsetGuidArray[0]);
  for(Index=0;Index<Count;Index++){
    if(CompareGuid(Guid, gIgnoredFormsetGuidArray[Index])){
      return TRUE;
    }  
  }

  return FALSE;
}

typedef struct {
  EFI_GUID       FormsetGuid;
  EFI_STRING     FormSetTitle;
  EFI_STRING     FormSetHelp;
} HII_FORMSET_GOTO_INFO;


STATIC 
EFI_STATUS 
GetDynamicHiiFormset (
  HII_FORMSET_GOTO_INFO  **Info,
  UINTN                  *InfoCount
  )
{
  EFI_STATUS                   Status;
  EFI_HII_HANDLE               HiiHandle;
  EFI_HII_HANDLE               *HiiHandleBuffer;
  UINTN                        Index; 
  EFI_HII_DATABASE_PROTOCOL    *HiiDB;
  UINTN                        BufferSize;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  UINT32                       Offset;
  UINT32                       Offset2;
  UINT32                       PackageListLength;
  UINT8                        *Package;
  UINT8                        *OpCodeData;
  EFI_GUID                     *FormsetGuid;  
  UINT8                        OpCode;
  HII_FORMSET_GOTO_INFO        *GotoInfo = NULL;
  UINTN                        GotoIndex = 0;
  EFI_STRING_ID                FormSetTitle;
  EFI_STRING_ID                FormSetHelp; 

  
  HiiHandleBuffer = NULL;
  HiiPackageList  = NULL;
  
  Status = gBS->LocateProtocol(&gEfiHiiDatabaseProtocolGuid, NULL, &HiiDB);
  if(EFI_ERROR(Status)){
    goto ProcExit;
  }  
  
  HiiHandleBuffer = HiiGetHiiHandles(NULL);
  if(HiiHandleBuffer == NULL){
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }

  for(Index=0; HiiHandleBuffer[Index]!=NULL; Index++){
    HiiHandle = HiiHandleBuffer[Index];
    
    _FREE_NON_NULL(HiiPackageList);
    BufferSize = 0;
    Status = HiiDB->ExportPackageLists(HiiDB, HiiHandle, &BufferSize, HiiPackageList);
    if(Status != EFI_BUFFER_TOO_SMALL){
      if(Status == EFI_SUCCESS){Status = EFI_ABORTED;}
      goto ProcExit;
    }
    HiiPackageList = AllocatePool(BufferSize);
    ASSERT(HiiPackageList != NULL);
    Status = HiiDB->ExportPackageLists(HiiDB, HiiHandle, &BufferSize, HiiPackageList);
    if(EFI_ERROR(Status)){
      goto ProcExit;
    } 

    if(IsFormsetIgnore(&HiiPackageList->PackageListGuid)){
      continue;
    }  
    DEBUG((EFI_D_INFO, "Pkg[%d] %g\n", Index, &HiiPackageList->PackageListGuid));

    PackageListLength = ReadUnaligned32(&HiiPackageList->PackageLength);
    Offset  = sizeof(EFI_HII_PACKAGE_LIST_HEADER);  // total 
    Offset2 = 0;                                    // form

    while(Offset < PackageListLength){
      Package = (UINT8*)HiiPackageList + Offset;
      CopyMem(&PackageHeader, Package, sizeof(EFI_HII_PACKAGE_HEADER));
      
      if(PackageHeader.Type != EFI_HII_PACKAGE_FORMS){
        Offset += PackageHeader.Length;
        continue;
      }
      
      Offset2 = sizeof(EFI_HII_PACKAGE_HEADER);
      while(Offset2 < PackageHeader.Length){
        OpCodeData = Package + Offset2;
        OpCode     = ((EFI_IFR_OP_HEADER*)OpCodeData)->OpCode;
        if(OpCode != EFI_IFR_FORM_SET_OP){
          Offset2 += ((EFI_IFR_OP_HEADER*)OpCodeData)->Length;
          continue;
        }

        GotoInfo = ReallocatePool(
                     GotoIndex * sizeof(HII_FORMSET_GOTO_INFO), 
                     (GotoIndex+1) * sizeof(HII_FORMSET_GOTO_INFO),
                     GotoInfo
                     );
        ASSERT(GotoInfo != NULL);

        FormsetGuid = (EFI_GUID*)(OpCodeData + OFFSET_OF(EFI_IFR_FORM_SET, Guid));
        CopyMem(&FormSetTitle, &((EFI_IFR_FORM_SET*)OpCodeData)->FormSetTitle, sizeof(EFI_STRING_ID));
        CopyMem(&FormSetHelp, &((EFI_IFR_FORM_SET*)OpCodeData)->Help, sizeof(EFI_STRING_ID));
        CopyMem(&GotoInfo[GotoIndex].FormsetGuid, FormsetGuid, sizeof(EFI_GUID));          

        GotoInfo[GotoIndex].FormSetTitle = HiiGetString(HiiHandle, FormSetTitle, NULL);
        GotoInfo[GotoIndex].FormSetHelp  = HiiGetString(HiiHandle, FormSetHelp, NULL);            
        GotoIndex++;
            
        Offset2 += ((EFI_IFR_OP_HEADER*)OpCodeData)->Length;  
      }
 
      Offset += PackageHeader.Length;
    }
    _FREE_NON_NULL(HiiPackageList);
  }

  DEBUG((EFI_D_INFO, "InfoCount:%d\n", GotoIndex));

  if(GotoIndex){
    *Info      = GotoInfo;
    *InfoCount = GotoIndex;
  } else {
    *Info      = NULL;
    *InfoCount = 0;
  }

ProcExit:
  _FREE_NON_NULL(HiiHandleBuffer);
  _FREE_NON_NULL(HiiPackageList);
  return Status;  
}



EFI_STATUS AddDynamicFormset()
{
  VOID                   *StartOpCodeHandle = NULL;
  VOID                   *EndOpCodeHandle   = NULL;
  EFI_IFR_GUID_LABEL     *StartLabel        = NULL;
  EFI_IFR_GUID_LABEL     *EndLabel          = NULL;
  EFI_STATUS             Status;
  HII_FORMSET_GOTO_INFO  *Info;
  UINTN                  InfoCount;
  UINTN                  Index;
  EFI_HII_HANDLE         *DevHii;
  EFI_STRING_ID          Title;
  EFI_STRING_ID          Help;


  DevHii = HiiGetHiiHandles(&gEfiChipsetByoFormsetGuid);
  if(DevHii == NULL){
    DEBUG((EFI_D_ERROR, "Device Formset Not Found\n"));
    Status = EFI_NOT_FOUND;
    goto ProcExit;
  }  

  Status = GetDynamicHiiFormset(&Info, &InfoCount);
  if(EFI_ERROR(Status) || InfoCount == 0){
    goto ProcExit;
  } 

//DEBUG((EFI_D_ERROR, "InfoCount:%d\n", InfoCount));
  

  StartOpCodeHandle = HiiAllocateOpCodeHandle();
  if (StartOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit;   
  }
  EndOpCodeHandle = HiiAllocateOpCodeHandle();
  if (EndOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ProcExit; 
  }

  StartLabel = (EFI_IFR_GUID_LABEL*)HiiCreateGuidOpCode(StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof(EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = 0x1234;

  EndLabel = (EFI_IFR_GUID_LABEL*)HiiCreateGuidOpCode(EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof(EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = 0xFFFF;

  for(Index=0;Index<InfoCount;Index++){
    Title = HiiSetString(DevHii[0], 0, Info[Index].FormSetTitle, NULL);
    Help  = HiiSetString(DevHii[0], 0, Info[Index].FormSetHelp, NULL);    
    HiiCreateGotoOpCode3 (
      StartOpCodeHandle,
      0,
      Title,
      Help,
      0,
      0,
      &Info[Index].FormsetGuid
      );
  }
  
  HiiUpdateForm (
    DevHii[0],
    &gEfiChipsetByoFormsetGuid,
    4608,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

ProcExit:
  return Status;
}





EFI_STATUS
EFIAPI
UiAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_LOGO_PROTOCOL        *BootLogo;
  
  
  DEBUG((EFI_D_INFO, "[Setup]\n"));

  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);

  gST->ConOut->ClearScreen (gST->ConOut);
  Status = gBS->LocateProtocol(&gEfiBootLogoProtocolGuid, NULL, (VOID**)&BootLogo);
  if (!EFI_ERROR (Status)) {
    BootLogo->SetBootLogo (BootLogo, NULL, 0, 0, 0, 0);
  }

  SignalEnterSetupEvent();
  BdsStartBootMaint ();
//  AddDynamicFormset();
  RunByoFormset();
  ByoFreeBMPackage ();

  return EFI_SUCCESS;
}




