/**
   blocks Write Read Test
  @param[in] Lba          The Lba number to start from.
  @param[in] BlockCount   How many blocks to display.

  @retval SHELL_SUCCESS   The display was successful.
**/


#include "SATAReadWrite.h"
#include "Ps2Keyboard.h"

EFI_HANDLE        gShellSataHiiHandle          = NULL;

extern UINT8      SataReadwriteStrings[];


SHELL_STATUS
EFIAPI
Block_WR_Test(
  IN CONST UINT8                       Loop_Flag,
  IN CONST UINT8                       Data,
  IN CONST UINT64                      Lba,
  IN CONST UINT64                      BlockCount,
  IN CONST UINT8                       All_Flag,
  IN EFI_SYSTEM_TABLE                  *SystemTable
  )
{
  EFI_BLOCK_IO_PROTOCOL                *BlockIo;
  EFI_STATUS                           Status;
  SHELL_STATUS                         ShellStatus;
  UINT8                                *Write_Buffer;
  UINT8                                *Read_Buffer;
  UINT32                               i=0;
  UINT32                               j=0;
  UINT32                               Count=0;
  UINT8                                Find_Flag=0;
  EFI_HANDLE                           *HandleBIOBuffer;
  UINTN                                NumberOfBIOHandles;
  EFI_HANDLE                           *HandleDIFBuffer;
  UINTN                                NumberOfDIFHandles;
  EFI_DEVICE_PATH_PROTOCOL             *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL			   *DevPathNode;
  UINT64                               Start_Lba;
  UINT64                               Opera_Count;
  CHAR16                               *Textdevicepath =NULL;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL*    Device2TextProtocol=NULL;



//  EFI_INPUT_KEY                        Key;
//  UINTN                                EventIndex;
//  // UINT8                             KeyCount = 0;


  Start_Lba = Lba;
  Opera_Count = BlockCount;
  ShellStatus = SHELL_SUCCESS;





  Status=gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid,NULL,(VOID**)&Device2TextProtocol);
  if (EFI_ERROR (Status))
  {
    return Status;
  }
  ////////////Find BlockIoProtocol////////////////////////////
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiBlockIoProtocolGuid,
                   NULL,
                   &NumberOfBIOHandles,
                   &HandleBIOBuffer
                   );
  if (EFI_ERROR (Status))
  {
    DEBUG((EFI_D_ERROR,"LocateBlockIoProtocol Error!\n"));
    return (ShellStatus);
  }
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiDiskInfoProtocolGuid,
                   NULL,
                   &NumberOfDIFHandles,
                   &HandleDIFBuffer
                   );
  if (EFI_ERROR (Status))
  {
    DEBUG((EFI_D_ERROR,"LocateDiskInfoProtocol Error!\n"));
    return (ShellStatus);
  }




  for (i = 0; i < NumberOfBIOHandles; i++)
  {
     for(j=0;j<NumberOfDIFHandles;j++)
     {
       if(HandleBIOBuffer[i]==HandleDIFBuffer[j])
       	{

       	  Status = gBS->HandleProtocol (
                     HandleBIOBuffer[i],
                     &gEfiDevicePathProtocolGuid,
                     (VOID**) &DevicePath
                     );

    		  DevPathNode = DevicePath;
    		  while (!IsDevicePathEnd (DevPathNode))
    		  {
    		  
    		   if((DevPathNode->Type==0x3)&&(DevPathNode->SubType==0x1))
    		   	{    
    		   	     DEBUG((EFI_D_ERROR,"Sata IDE Mode\n"));
    		   	     Find_Flag = 1;
    				 break;
    		   	}
    		   else if((DevPathNode->Type==0x3)&&(DevPathNode->SubType==0x12))
    		   	{
    		   	     DEBUG((EFI_D_ERROR,"Sata AHCI Mode\n"));
    		   	     Find_Flag = 1;
    				 break;
    		   	}
    		   DevPathNode = NextDevicePathNode (DevPathNode);
    		  }

   	      if(Find_Flag==1)
   	      {
            DevPathNode = DevicePath;
            if(!IsDevicePathEnd (DevPathNode))
            {
             Print(L"(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
             DevPathNode = NextDevicePathNode (DevPathNode);
            }
            while(!IsDevicePathEnd (DevPathNode))
            {
             Print(L"/(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
             DevPathNode = NextDevicePathNode (DevPathNode);
            }
            Print(L"\n");

            DevPathNode = DevicePath;
            if (!IsDevicePathEnd (DevPathNode))
            {
             Textdevicepath=Device2TextProtocol->ConvertDevicePathToText(DevPathNode,TRUE,TRUE);
             Print(L"%s",Textdevicepath);
             if(Textdevicepath) gBS->FreePool(Textdevicepath);
            }
            Print(L"\n");
   	        break;
   	      }
       	}
     }
	 if(Find_Flag==1)
	 {
	   break;
	 }
  }

  
  if(Find_Flag==1)
  {
  	 Status = gBS->HandleProtocol (
                     HandleBIOBuffer[i],
                     &gEfiBlockIoProtocolGuid,
                     (VOID**) &BlockIo
                     );
    ASSERT_EFI_ERROR (Status);
  }
  else
  {
    DEBUG((EFI_D_ERROR,"ERROR: Can't find BlockIO!\n"));
	return (ShellStatus);
  }




  ////////////////////Write && Read Block Test//////////////////////////////
  if (BlockCount > 0) {
    Write_Buffer     = AllocateZeroPool(BlockIo->Media->BlockSize);
	Read_Buffer      = AllocateZeroPool(BlockIo->Media->BlockSize);
  } else {
    ShellPrintEx(-1,-1,L"  BlockSize: 0x%08x, BlockCount: 0x%08x\r\n", BlockIo->Media->BlockSize, BlockCount);
    Write_Buffer    = NULL;
	Read_Buffer     = NULL;
  }




  my_memset(Write_Buffer,Data,BlockIo->Media->BlockSize);
  do
  {
    if(Loop_Flag)
    {
     Count++;
     DEBUG((EFI_D_ERROR,"Test_Count=%x\n",Count));
    }
	if(All_Flag)
	{
	   Start_Lba = 0;
	   Opera_Count = ((EFI_BLOCK_IO_PROTOCOL *) BlockIo)->Media->LastBlock;
	   DEBUG((EFI_D_ERROR,"LastBlock=%d\n",Opera_Count));
	}

    
  	for(i=0;i<Opera_Count;i++)
    {
	     Display_Percent2(Start_Lba,i,Opera_Count);
	  	 Status = BlockIo->WriteBlocks(BlockIo, BlockIo->Media->MediaId, (Start_Lba+i), BlockIo->Media->BlockSize, Write_Buffer);
		 if(EFI_ERROR(Status))
		 {
		  	Display_Percent(Start_Lba,i,1,1);
//		  	goto Error;
		 }
         else
         {
         	Display_Percent(Start_Lba,i,1,0);
		 }


         
	     Status = BlockIo->ReadBlocks(BlockIo, BlockIo->Media->MediaId, (Start_Lba+i), BlockIo->Media->BlockSize, Read_Buffer);
		 if(EFI_ERROR(Status))
		 {
		 	Display_Percent(Start_Lba,i,0,1);
//			goto Error;
		 }
         else
         {
         	Display_Percent(Start_Lba,i,0,0);
		 }

	     
//	     if(my_memcpy(Write_Buffer,Read_Buffer,BlockIo->Media->BlockSize)!=0)
//	     {
//	        for(j=0;j<BlockIo->Media->BlockSize;j++)
//	        {
//	          if(Write_Buffer[j]!=Read_Buffer[j])
//	          {
//	         	 DEBUG((EFI_D_ERROR,"Lba=%x,Offset=%x not Same(Data=%x)\n",(Start_Lba+i),j,Read_Buffer[j]));
//				 goto Error;	
//	          }
//	        }
//	     }
//	     my_memset(Read_Buffer,0,BlockIo->Media->BlockSize);
         DEBUG((EFI_D_ERROR,"\n"));
	 }



    
//    for(i=0;i<Opera_Count;i++)
//    {
//  	 Status = BlockIo->ReadBlocks(BlockIo, BlockIo->Media->MediaId, (Start_Lba+i), BlockIo->Media->BlockSize, Read_Buffer);
//	 Display_Percent(Start_Lba,i,Opera_Count,1);
//	 if(EFI_ERROR(Status))
//	 {
//	 	DEBUG((EFI_D_ERROR,"ReadBlock Error\n"));
//		goto Error;
//	 }

//     
//     if(my_memcpy(Write_Buffer,Read_Buffer,BlockIo->Media->BlockSize)!=0)
//     {
//        for(j=0;j<BlockIo->Media->BlockSize;j++)
//        {
//          if(Write_Buffer[j]!=Read_Buffer[j])
//          {
//         	 DEBUG((EFI_D_ERROR,"Lba=%x,Offset=%x not Same(Data=%x)\n",(Start_Lba+i),j,Read_Buffer[j]));
//			 goto Error;	
//          }
//        }
//     }
//	 my_memset(Read_Buffer,0,BlockIo->Media->BlockSize);
//    }
//    DEBUG((EFI_D_ERROR,"\n"));

//    gBS->Stall (100);
//    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
//    gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
//    gBS->Stall (100);
//    Print(L"%s\n",Key.ScanCode);
//    if (!EFI_ERROR(Status)) 
//    {
//      if ((Key.UnicodeChar == 0x1b) || (Key.UnicodeChar == 0x03) || ((Key.UnicodeChar == 0) && (Key.ScanCode == SCAN_ESC))) 
//      {
//        Print(L"Keyboard pending.\n");
//        return Status;
//      }
//    }
  
 }while(Loop_Flag);





DEBUG((EFI_D_ERROR,"Write and Read OK!\n"));

//Error:
//  if (Write_Buffer != NULL) {
//    FreePool(Write_Buffer);
//  }
//  if (Read_Buffer  !=NULL) {
//  	FreePool(Read_Buffer);
//  }
  return (ShellStatus);
}



// VOID
// EFIAPI
// KeyboardWaitForKey2 (
//   IN  EFI_EVENT               Event,
//   IN  VOID                    *Context
//   )
// {
//   EFI_TPL                     OldTpl;
//   KEYBOARD_CONSOLE_IN_DEV     *ConsoleIn;
//   EFI_KEY_DATA                KeyData;



//   ConsoleIn = (KEYBOARD_CONSOLE_IN_DEV *) Context;

//   //
//   // Enter critical section
//   //
//   OldTpl = gBS->RaiseTPL (TPL_NOTIFY);



//   if (!ConsoleIn->KeyboardErr) {
//     //
//     // WaitforKey doesn't suppor the partial key.
//     // Considering if the partial keystroke is enabled, there maybe a partial
//     // keystroke in the queue, so here skip the partial keystroke and get the
//     // next key from the queue
//     //
//     while (!IsEfikeyBufEmpty (&ConsoleIn->EfiKeyQueue)) {
//       CopyMem (
//         &KeyData,
//         &(ConsoleIn->EfiKeyQueue.Buffer[ConsoleIn->EfiKeyQueue.Head]),
//         sizeof (EFI_KEY_DATA)
//         );
//       if ((KeyData.Key.ScanCode == NULL && KeyData.Key.UnicodeChar == CHAR_NULL) || (Key.UnicodeChar == 0 && Key.ScanCode == SCAN_ESC)) {
//         PopEfikeyBufHead (&ConsoleIn->EfiKeyQueue, &KeyData);
//         continue;
//       }

//             // gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
//         	// // gBS->Stall (100);
//         	// // Print(L"%s\n",Key.ScanCode);
//         	// if (!EFI_ERROR(Status)) 
//         	// {
//         	//   if ((Key.UnicodeChar == 0x1b) || (Key.UnicodeChar == 0x03) || ((Key.UnicodeChar == 0) && (Key.ScanCode == SCAN_ESC))) 
//         	//   {
//         	//     Print(L"Keyboard pending.\n");
//         	//     return Status;
//         	//   }
//         	// }


//       //
//       // if there is pending value key, signal the event.
//       //
//       gBS->SignalEvent (Event);
//       break;
//     }
//   }
//   //
//   // Leave critical section and return
//   //
//   gBS->RestoreTPL (OldTpl);
// }




/**
  Function for 'sata' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  LIST_ENTRY                *Package;
  CHAR16                    *ProblemParam;
  SHELL_STATUS              ShellStatus;
  CONST CHAR16              *DataString;
  CONST CHAR16              *LbaString;
  CONST CHAR16              *BlockCountString;
  UINT64                    Lba;
  UINT64                    BlockCount;
  UINT64                    Data;
    // KEYBOARD_CONSOLE_IN_DEV                   *ConsoleIn;

  UINT8                     Loop_Flag=0;
  UINT8                     All_Flag=0;
  EFI_HANDLE                gShellSataHiiHandle = NULL;


  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;



  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);
  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);


  gShellSataHiiHandle = HiiAddPackages (&gShellSataHiiGuid, gImageHandle, SataReadwriteStrings, NULL);
  if (gShellSataHiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }


  //
  // parse the command line
  //
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellSataHiiHandle, ProblemParam);
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SATA_MANUAL), gShellSataHiiHandle);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 6) 
    {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellSataHiiHandle);
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SATA_MANUAL), gShellSataHiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    }else if (ShellCommandLineGetCount(Package) < 4) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellSataHiiHandle);
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SATA_MANUAL), gShellSataHiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    }else {
      //
      // Parse the params
      //
      // sata_test data_pattern start_lba lba_count [L] [all]
      // example: sata_test 0x13 0x10 0x80 L
      DataString        = ShellCommandLineGetRawValue(Package, 1);
      LbaString         = ShellCommandLineGetRawValue(Package, 2);
      BlockCountString  = ShellCommandLineGetRawValue(Package, 3);
	  if((ShellCommandLineGetCount(Package) ==5)&&(StrCmp(ShellCommandLineGetRawValue(Package, 4),L"L")==0))
	  {
	  	Loop_Flag = 1;
		DEBUG((EFI_D_ERROR,"Loop_Flag=1\n"));
	  }
      else if((ShellCommandLineGetCount(Package) ==5)&&(StrCmp(ShellCommandLineGetRawValue(Package, 4),L"All")==0))
	  {
	  	All_Flag = 1;
		DEBUG((EFI_D_ERROR,"All_Flag=1\n"));
	  }
	  if((ShellCommandLineGetCount(Package) ==6)&&(StrCmp(ShellCommandLineGetRawValue(Package, 4),L"L")==0)&&(StrCmp(ShellCommandLineGetRawValue(Package, 5),L"All")==0))
	  {
	    Loop_Flag = 1;
		DEBUG((EFI_D_ERROR,"Loop_Flag=1\n"));
	  	All_Flag = 1;
		DEBUG((EFI_D_ERROR,"All_Flag=1\n"));
	  }
      else if((ShellCommandLineGetCount(Package) ==6)&&(StrCmp(ShellCommandLineGetRawValue(Package, 4),L"All")==0)&&(StrCmp(ShellCommandLineGetRawValue(Package, 5),L"L")==0))
	  {
	    Loop_Flag = 1;
		DEBUG((EFI_D_ERROR,"Loop_Flag=1\n"));
	  	All_Flag = 1;
		DEBUG((EFI_D_ERROR,"All_Flag=1\n"));
	  }


      if (DataString == NULL) {
        Data = 0;
      } else {
        if (!ShellIsHexOrDecimalNumber(DataString, TRUE, FALSE)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellSataHiiHandle, DataString);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
        ShellConvertStringToUint64(DataString, &Data, TRUE, FALSE);
		
      }



	  if (LbaString == NULL) {
			  Lba = 0;
			} else {
			  if (!ShellIsHexOrDecimalNumber(LbaString, TRUE, FALSE)) {
				ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellSataHiiHandle, LbaString);
				ShellStatus = SHELL_INVALID_PARAMETER;
			  }
			  ShellConvertStringToUint64(LbaString, &Lba, TRUE, FALSE);
			}




      if (BlockCountString == NULL) {
        BlockCount = 1;
      } else {
        if (!ShellIsHexOrDecimalNumber(BlockCountString, TRUE, FALSE)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellSataHiiHandle, BlockCountString);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
        ShellConvertStringToUint64(BlockCountString, &BlockCount, TRUE, FALSE);
      }
      


      if (ShellStatus == SHELL_SUCCESS) {
      	// // gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
      	// gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      	// // gBS->Stall (100);
      	// // Print(L"%s\n",Key.ScanCode);
      	// if (!EFI_ERROR(Status)) 
      	// {
      	//   if ((Key.UnicodeChar == 0x1b) || (Key.UnicodeChar == 0x03) || ((Key.UnicodeChar == 0) && (Key.ScanCode == SCAN_ESC))) 
      	//   {
      	//     Print(L"Keyboard pending.\n");
      	//     return Status;
      	//   }
      	// }

      	// Status = gBS->CreateEvent (
      	//                   EVT_TIMER | EVT_NOTIFY_SIGNAL,
      	//                   TPL_NOTIFY,
      	//                   KeyboardTimerHandler2,
      	//                   ConsoleIn,
      	//                   &ConsoleIn->TimerEvent
      	//                   );
      	//   if (EFI_ERROR (Status)) {
      	//     // Status      = EFI_OUT_OF_RESOURCES;
      	//     // StatusCode  = EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_CONTROLLER_ERROR;
      	//     Print(L"Keyboard pending.\n");
      	//     return Status;
      	//   }




      	//   Status = gBS->SetTimer (
      	//                   ConsoleIn->TimerEvent,
      	//                   TimerPeriodic,
      	//                   KEYBOARD_TIMER_INTERVAL
      	//                   );
      	//   if (EFI_ERROR (Status)) {
      	//     // Status      = EFI_OUT_OF_RESOURCES;
      	//     // StatusCode  = EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_CONTROLLER_ERROR;
      	//     Print(L"Keyboard pending.\n");
      	//     return Status;
      	//   }



        //
        // do the work if we have a valid block identifier
        //

        Block_WR_Test(Loop_Flag,(UINT8)Data,Lba,BlockCount,All_Flag,SystemTable);
      }
    }

    ShellCommandLineFreeVarList (Package);
  }
  return (ShellStatus);
}


