#include"Sata_Block.h"
/**
   blocks Write Read Test
  @param[in] Lba          The Lba number to start from.
  @param[in] BlockCount   How many blocks to display.

  @retval SHELL_SUCCESS   The display was successful.
**/

SHELL_STATUS
EFIAPI
Block_WR_Test(
  IN CONST UINT8                    Loop_Flag,
  IN CONST UINT8                    Data,
  IN CONST UINT64                   Lba,
  IN CONST UINT64                   BlockCount,
  IN CONST UINT8                    All_Flag
  )
{
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  EFI_STATUS                Status;
  SHELL_STATUS              ShellStatus;
  UINT8                     *Write_Buffer;
  UINT8                     *Read_Buffer;
  UINT32                    i=0;
  UINT32                    j=0;
  UINT32                    Count=0;
  UINT8                     Find_Flag=0;
  EFI_HANDLE                    *HandleBIOBuffer;
  UINTN                         NumberOfBIOHandles;
  EFI_HANDLE                    *HandleDIFBuffer;
  UINTN                         NumberOfDIFHandles;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL				*DevPathNode;
  UINT64                       Start_Lba;
  UINT64                       Opera_Count;
  Start_Lba = Lba;
  Opera_Count = BlockCount;
  ShellStatus = SHELL_SUCCESS;




  
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
    DEBUG((EFI_D_ERROR,"LocateBlockIoProtocol_error\n"));
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
    DEBUG((EFI_D_ERROR,"LocateDiskInfoProtocol_error\n"));
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
    DEBUG((EFI_D_ERROR,"Can't find BlockIO---error------\n"));
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
  	 Status = BlockIo->WriteBlocks(BlockIo, BlockIo->Media->MediaId, (Start_Lba+i), BlockIo->Media->BlockSize, Write_Buffer);
     Display_Percent(Start_Lba,i,Opera_Count,0);
	 if(EFI_ERROR(Status))
	 {
	  DEBUG((EFI_D_ERROR,"Write Error\n"));
	  goto Error;
	 }
    }
	DEBUG((EFI_D_ERROR,"\n"));
    for(i=0;i<Opera_Count;i++)
    {
  	 Status = BlockIo->ReadBlocks(BlockIo, BlockIo->Media->MediaId, (Start_Lba+i), BlockIo->Media->BlockSize, Read_Buffer);
	 Display_Percent(Start_Lba,i,Opera_Count,1);
	 if(EFI_ERROR(Status))
	 {
	 	DEBUG((EFI_D_ERROR,"ReadBlock Error\n"));
		goto Error;
	 }
     if(my_memcpy(Write_Buffer,Read_Buffer,BlockIo->Media->BlockSize)!=0)
     {
        for(j=0;j<BlockIo->Media->BlockSize;j++)
        {
         if(Write_Buffer[j]!=Read_Buffer[j])
         {
         	DEBUG((EFI_D_ERROR,"Lba=%x,Offset=%x not Same(Data=%x)\n",(Start_Lba+i),j,Read_Buffer[j]));
			goto Error;
			
          }
        }
      }
	 my_memset(Read_Buffer,0,BlockIo->Media->BlockSize);
    }
    DEBUG((EFI_D_ERROR,"\n"));
  
 }while(Loop_Flag);





DEBUG((EFI_D_ERROR,"Write and Read Ok\n"));

Error:
  if (Write_Buffer != NULL) {
    FreePool(Write_Buffer);
  }
  if (Read_Buffer  !=NULL) {
  	FreePool(Read_Buffer);
  }
  return (ShellStatus);
}






/**
  Function for 'sata' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/

SHELL_STATUS
EFIAPI
ShellCommandRunSata (
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
  UINT8 Loop_Flag=0;
  UINT8 All_Flag=0;

  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);
  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);



  //
  // parse the command line
  //
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellSataHiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 6) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellSataHiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount(Package) < 4) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellSataHiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
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
	  if((ShellCommandLineGetCount(Package) ==6)&&(StrCmp(ShellCommandLineGetRawValue(Package, 5),L"all")==0))
	  {
	  	All_Flag = 1;
		DEBUG((EFI_D_ERROR,"All_Flag=1\n"));
	  }
	  if((ShellCommandLineGetCount(Package) ==5)&&(StrCmp(ShellCommandLineGetRawValue(Package, 4),L"all")==0))
	  {
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
        //
        // do the work if we have a valid block identifier
        //
        Block_WR_Test(Loop_Flag,(UINT8)Data,Lba,BlockCount,All_Flag);
      }
    }


    ShellCommandLineFreeVarList (Package);
  }
  return (ShellStatus);
}


















































