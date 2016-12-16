#include <PlatformSetup.h>

EFI_STATUS
EFIAPI
PowerFormCallback (
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
   
  DEBUG((EFI_D_ERROR,"\nPowerFormCallback(), Action :%d.\n", Action));
    
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
	   if (POWER_FORM_SET_CLASS == gSetupCallbackInfo[i].Class) {
		 HiiHandle = gSetupCallbackInfo[i].HiiHandle;
		 break;
	   }
	 }	
	 if (NULL == HiiHandle) {
	   DEBUG((EFI_D_ERROR,"PowerFormCallback(),Can't get Devices Form HiiHandle.\n"));
	   return EFI_INVALID_PARAMETER;
	 }
 
 HiiGetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *) &CurrentSetupData);

	switch (KeyValue){
		case KEY_C4P_CONTROL:
		   //
		   // If C4P Control is enabled, then set SATA C4P, USB C4P and NM C4P to enable.
		   //
		   if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)) 
		  { // SDIO Ver3.0 Support is disable.
			//CurrentSetupData.SataC4P = 0;
			//CurrentSetupData.UsbC4P  = 0;
			//CurrentSetupData.XhciC4P = 0;
			//CurrentSetupData.NMC4P   = 0;
		   }
		   else
		   {
			 //CurrentSetupData.SataC4P = 1;
			//CurrentSetupData.UsbC4P  = 1;
			//CurrentSetupData.XhciC4P = 1;
			 //CurrentSetupData.NMC4P   = 1;
		   }
		   
		   break;

	default:
		 break;
	 }
	
	HiiSetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *)&CurrentSetupData, NULL);

		return EFI_SUCCESS;
}

