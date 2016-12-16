#include <PlatformSetup.h>

EFI_STATUS
EFIAPI
DevicesFormCallback (
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
   
  DEBUG((EFI_D_ERROR,"\nDevicesFormCallback(), Action :%d.\n", Action));
    
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
	   if (DEVICES_FORM_SET_CLASS == gSetupCallbackInfo[i].Class) {
		 HiiHandle = gSetupCallbackInfo[i].HiiHandle;
		 break;
	   }
	 }	
	 if (NULL == HiiHandle) {
	   DEBUG((EFI_D_ERROR,"DevicesFormCallback(),Can't get Devices Form HiiHandle.\n"));
	   return EFI_INVALID_PARAMETER;
	 }
 
 HiiGetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *) &CurrentSetupData);

	switch (KeyValue){
		case KEY_VALUE_PCIERST:
		  if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)) {
			  CurrentSetupData.PcieRstPEG	= 0;
			  CurrentSetupData.PcieRstPE0	= 0;
			  CurrentSetupData.PcieRstPE1	= 0;
			  CurrentSetupData.PcieRstPE2	= 0;
			  CurrentSetupData.PcieRstPE3	= 0;
			  CurrentSetupData.PcieRstPEG0	= 0;
			  CurrentSetupData.PcieRstPEG1	= 0;
			  CurrentSetupData.PcieRstPEG2	= 0;
			  CurrentSetupData.PcieRstPEG3	= 0;
		  }
		  break;

	case KEY_VALUE_PCIERP:
	  if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0)) {
	      CurrentSetupData.PciePE0  = 0;
		  CurrentSetupData.PciePE1  = 0;
		  CurrentSetupData.PciePE2  = 0;
		  CurrentSetupData.PciePE3  = 0;
		  CurrentSetupData.PciePEG0= 0;
		  CurrentSetupData.PciePEG1= 0;
		  CurrentSetupData.PciePEG2= 0;
		  CurrentSetupData.PciePEG3= 0;
		  CurrentSetupData.PciePEG= 0;
		  CurrentSetupData.PcieRst= 0;
	  }
	  break;
	  case KEY_VALUE_PCIE_PEG:
		// If PCIE PEG control is disabled, then disable Reset PEG When Link Fail.
		if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
		{
		   CurrentSetupData.PcieRstPEG = 0;
		}
		else
		{
		  CurrentSetupData.PcieRstPEG = 1;
		}
		
		break;

  case KEY_VALUE_PCIE_PEG0:
    // If PCIE PEG control is disabled, then disable Reset PEG When Link Fail.
    if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPEG0 = 0;
    }
    else
    {
      CurrentSetupData.PcieRstPEG0 = 1;
    }
    
    break;

	case KEY_VALUE_PCIE_PEG1:
    // If PCIE PEG control is disabled, then disable Reset PEG When Link Fail.
     if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPEG1 = 0;
    }
    else
    {
      CurrentSetupData.PcieRstPEG1 = 1;
    }
    
    break;
	
 
	case KEY_VALUE_PCIE_PEG2:
    // If PCIE PEG control is disabled, then disable Reset PEG When Link Fail.
     if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPEG2 = 0;
    }
    else
    {
      CurrentSetupData.PcieRstPEG2 = 1;
    }
    
    break;

  
  case KEY_VALUE_PCIE_PEG3:
  // If PCIE PEG control is disabled, then disable Reset PEG When Link Fail.
   if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
  {
	 CurrentSetupData.PcieRstPEG3 = 0;
  }
  else
  {
	CurrentSetupData.PcieRstPEG3 = 1;
  }
  
  break;
  

  case KEY_VALUE_PCIE_PE0:
    // If PCIE PE0 control is disabled, then disable Reset PE0 When Link Fail.
     if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPE0 = 0;
    }
    else
    {
      CurrentSetupData.PcieRstPE0 = 1;
    }
    break;
  
  case KEY_VALUE_PCIE_PE1:
    // If PCIE PE1 control is disabled, then disable Reset PE1 When Link Fail.
    if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPE1 = 0;
    }
    else
    {
      CurrentSetupData.PcieRstPE1 = 1;
    }
    
    break;
  
  case KEY_VALUE_PCIE_PE2:
    // If PCIE PE2 control is disabled, then disable Reset PE2 When Link Fail.
    if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPE2 = 0;
    }
    else
    {
      CurrentSetupData.PcieRstPE2 = 1;
    }
    
    break;
  
  case KEY_VALUE_PCIE_PE3:
    // If PCIE PE3 control is disabled, then disable Reset PE3 When Link Fail.
    if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
    {
       CurrentSetupData.PcieRstPE3 = 0;
    }
    else
    {
      CurrentSetupData.PcieRstPE3 = 1;
    }
    
    break;
 
	default:
		 break;
	 }
	
	HiiSetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *)&CurrentSetupData, NULL);

		return EFI_SUCCESS;
}

