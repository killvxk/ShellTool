#include <PlatformSetup.h>
#include <AutoBuildTime.h>
#include "CpuSetup.h"
#include <Protocol/DiskInfo.h>
#include "SetupItemId.h"
#include "VfrExtension.h"

VOID
GetDramInfomation(
  EFI_HII_HANDLE HiiHandle,
  UINT8 *DramCL,
  UINT8 *DramTrp,
  UINT8 *DramTrcd,
  UINT8 *DramTras
  )
{
  PLATFORM_MEMORY_INFO    *MemInfo; 
  EFI_PEI_HOB_POINTERS    GuidHob;
 

  GuidHob.Raw = GetFirstGuidHob(&gEfiPlatformMemInfoGuid);
  ASSERT(GuidHob.Raw != NULL);  
  MemInfo = (PLATFORM_MEMORY_INFO*)(GuidHob.Guid+1);

  *DramCL = MemInfo->DramCL;
  *DramTrp =MemInfo->DramTrp;
  *DramTrcd = MemInfo->DramTrcd;
  *DramTras =MemInfo->DramTras;


}

EFI_STATUS
EFIAPI
AdvancedFormCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
  )
{
  EFI_HII_HANDLE                 HiiHandle = NULL;  
  SETUP_DATA        CurrentSetupData;
  UINT8 i;
  UINT8 DramCL=0;
  UINT8 DramTrp=0;
  UINT8 DramTrcd=0;
  UINT8 DramTras=0;
  DEBUG((EFI_D_ERROR,"\nAdvancedFormCallback(), Action :%d.\n", Action));
    
  if ((Action == EFI_BROWSER_ACTION_FORM_OPEN) || \
      (Action == EFI_BROWSER_ACTION_FORM_CLOSE)) {
    //
    // Do nothing for UEFI OPEN/CLOSE Action.
    //
    return EFI_SUCCESS;
  }

  if (Action != EFI_BROWSER_ACTION_CHANGING && Action != EFI_BROWSER_ACTION_CHANGED) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changed.
    //
    return EFI_UNSUPPORTED;
  }

	//
	 // Get Security form hii databas handle.
	 //
	 for (i=0;i<gSetupCallbackInfoNumber;i++) {
	   if (ADVANCED_FORM_SET_CLASS == gSetupCallbackInfo[i].Class) {
		 HiiHandle = gSetupCallbackInfo[i].HiiHandle;
		 break;
	   }
	 }	
	 if (NULL == HiiHandle) {
	   DEBUG((EFI_D_ERROR,"AdvancedFormCallback(),Can't get Advanced Form HiiHandle.\n"));
	   return EFI_INVALID_PARAMETER;
	 }
 
 HiiGetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *) &CurrentSetupData);
 GetDramInfomation(HiiHandle,&DramCL,&DramTrp,&DramTrcd,&DramTras);
// DEBUG((EFI_D_ERROR,"\nDLA:\nDramCL=%x\nDramTrp=%x\nDramTrcd=%x\nDramTras=%x\n\n"));

	switch (KeyValue){

	case KEY_ACTIMING_OPTION:
	  if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 1)) {
	  	CurrentSetupData.DramCL=DramCL;
		CurrentSetupData.DramTrp=DramTrp;
		CurrentSetupData.DramTrcd=DramTrcd;
		CurrentSetupData.DramTras=DramTras;

	  }
	  break;
 
	default:
		 break;
	 }
	
	HiiSetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *)&CurrentSetupData, NULL);

		return EFI_SUCCESS;
}

