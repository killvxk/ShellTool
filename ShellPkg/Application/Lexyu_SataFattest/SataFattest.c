/** @file
  This is a simple shell application

  Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

// #define ATA_DEVICE_FROM_PACKET(a)         CR (a, ATA_DEVICE, Handle, ATA_DEVICE_SIGNATURE)
// #define ATA_DEVICE_FROM_PACKET(a)         CR (a, ATA_DEVICE, Packet, ATA_DEVICE_SIGNATURE)

#include <Uefi.h>
#include <Uefi/UefiSpec.h>




#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/SortLib.h>
#include <Library/HiiLib.h>
#include <Library/FileHandleLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/HandleParsingLib.h>
#include <Library/ReportStatusCodeLib.h>

#include <IndustryStandard/Atapi.h>



#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/PciIo.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellParameters.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/DriverDiagnostics2.h>
#include <Protocol/DriverDiagnostics.h>
#include <Protocol/PlatformDriverOverride.h>
#include <Protocol/BusSpecificDriverOverride.h>
#include <Protocol/PlatformToDriverConfiguration.h>
#include <Protocol/DriverSupportedEfiVersion.h>
#include <Protocol/DriverFamilyOverride.h>
#include <Protocol/DriverHealth.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/StorageSecurityCommand.h>




#include <Guid/GlobalVariable.h>
#include <Guid/ConsoleInDevice.h>
#include <Guid/ConsoleOutDevice.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/ShellLibHiiGuid.h>


EFI_STATUS
EFIAPI
PrintDevicePath( IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath)
{
    EFI_STATUS                            Status;
    EFI_DEVICE_PATH_PROTOCOL             *DevPathNode;
    CHAR16                               *Textdevicepath =NULL;
    EFI_DEVICE_PATH_TO_TEXT_PROTOCOL     *Device2TextProtocol=NULL;
                            


    Status=gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid,NULL,(VOID**)&Device2TextProtocol);
    ASSERT_EFI_ERROR (Status);

    DevPathNode=DevicePath;
    if(!IsDevicePathEnd(DevPathNode))
    {
      Print(L"(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
      DevPathNode=NextDevicePathNode(DevPathNode);
    }
    while(!IsDevicePathEnd(DevPathNode))
    {
      Print(L"/(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
      DevPathNode=NextDevicePathNode(DevPathNode);
    }
    Print(L"\n");
    

    DevPathNode=DevicePath;
    if(!IsDevicePathEnd(DevPathNode))
    {
      Textdevicepath=Device2TextProtocol->ConvertDevicePathToText(DevPathNode,TRUE,TRUE);
      Print(L"%s",Textdevicepath);
      Print(L"\n\n");
      if(Textdevicepath) gBS->FreePool(Textdevicepath);
    }

    return EFI_SUCCESS;
    
}






  
/**
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{

  // UINTN        						        	Index;
  EFI_STATUS  							           Status;
  UINTN       							           Number1;
  // UINTN        							           Number2;
  // UINTN        							           Number3;
  // EFI_HANDLE   							           *HandleIdeController;
  EFI_HANDLE                           *Handlesimplefilesystem;
  // EFI_HANDLE                           *Handleblockio;
  // EFI_HANDLE                           *Handlediskio;
  // EFI_HANDLE   							           *myHandle=NULL;
  UINT32       							           i=0;
  // UINT32                               j=0;
  // UINT32       							           k=0;
  EFI_DEVICE_PATH_PROTOCOL             *DevicePath;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL      *Volume;
  EFI_FILE_HANDLE                   FileHandle;
  EFI_FILE_PROTOCOL                 *EfiDirect=0;
  EFI_FILE_PROTOCOL                 *Readme=0;
  // EFI_DEVICE_PATH_PROTOCOL	           *DevPathNode;
  // CHAR16                               *Textdevicepath =NULL;
  // EFI_DEVICE_PATH_TO_TEXT_PROTOCOL*    Device2TextProtocol=NULL;
  
  // UINT8                                Find_Flag=0;
  // // CHAR16                            *Textdevicepath2 =NULL;

  // EFI_IDE_CONTROLLER_INIT_PROTOCOL     *IdeControllerInit;
  // EFI_PCI_IO_PROTOCOL                  *PciIo;
  // UINT64                               Supports;
  // UINT64                               OriginalPciAttributes;
  



  // ATA_ATAPI_PASS_THRU_INSTANCE         *Instance=NULL;
  // EFI_PCI_IO_PROTOCOL                  *PciIo=NULL;
  // UINT32 Mode = 0;
  // UINT8  ClassCode = 0;


  // EFI_BLOCK_IO_PROTOCOL            *BlockIo;
  // ATA_DEVICE                       *AtaDevice;
  // EFI_AHCI_REGISTERS               *AhciRegisters;
  // EFI_ATA_PASS_THRU_PROTOCOL              *AtaPassThru;
  // EFI_ATA_PASS_THRU_COMMAND_PACKET        *Packet=NULL;


  // UINTN                         Offset;
  // EFI_PHYSICAL_ADDRESS          PhyAddr;
  // VOID                          *Map;
  // UINTN                         MapLength;
  // EFI_PCI_IO_PROTOCOL_OPERATION Flag;
  // EFI_AHCI_COMMAND_FIS          CFis;
  // EFI_AHCI_COMMAND_LIST         CmdList;
  // UINTN                         FisBaseAddr;

  UINTN        BufSize;


  CHAR16      *Buf=(CHAR16*)L"This is a test file from lexyu.\n";
  Status                   = EFI_SUCCESS;
  // IdeControllerInit     = NULL;
  // Instance              = NULL;
  // OriginalPciAttributes = 0;





//////////////////////////////////////////////////// Get the handle of SATA controller /////////////////////

  Status=gBS->LocateHandleBuffer(
      ByProtocol,
      &gEfiSimpleFileSystemProtocolGuid,
      NULL,
      &Number1,
      &Handlesimplefilesystem
  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status))
  {
    Print(L"Locate gEfiSimpleFileSystemProtocolGuid Error!\n");
  }
  else
  {
    Print(L"Locate gEfiSimpleFileSystemProtocolGuid! Number1=%d\n\n",Number1);
  }





  // Status=gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid,NULL,(VOID**)&Device2TextProtocol);
  // ASSERT_EFI_ERROR (Status);


  for(i=0;i<Number1;i++)
  {
    // Find_Flag=0;
    Status=gBS->HandleProtocol(
      Handlesimplefilesystem[i],
      &gEfiDevicePathProtocolGuid,
      (VOID**) &DevicePath
      );

   PrintDevicePath(DevicePath);

   Status=gBS->HandleProtocol(
     Handlesimplefilesystem[i],
     &gEfiSimpleFileSystemProtocolGuid,
     (VOID**) &Volume
     );
   if(!EFI_ERROR(Status))
   {
   		Status=Volume->OpenVolume(Volume,&FileHandle);

   		Status=FileHandle->Open(
   			FileHandle,
   			&Readme,
   			(CHAR16*)L"Readme.txt",
   			EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE,
   			0
   			);

   		Status=Readme->Delete(Readme);

   		Status=FileHandle->Open(
   			FileHandle,
   			&EfiDirect,
   			(CHAR16*)L"EFI",
   			EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE,
   			EFI_FILE_DIRECTORY
   			);
   		Status=FileHandle->Open(
   			EfiDirect,
   			&Readme,
   			(CHAR16*)L"Readme.txt",
   			EFI_FILE_MODE_CREATE|EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE,
   			0
   			);
   		if(Readme&& !EFI_ERROR(Status))
   		{
   			BufSize=200;
   			Status=Readme->Write(Readme,&BufSize,Buf);
   			Print(L"BufSize=%d\n",BufSize);
   		}
   		Status=Readme->Close(Readme);
   		Status=EfiDirect->Close(EfiDirect);

        FileHandle->Close(FileHandle);
   }
   // DevPathNode=DevicePath;
   // if(!IsDevicePathEnd(DevPathNode))
   // {
   //   Textdevicepath=Device2TextProtocol->ConvertDevicePathToText(DevPathNode,TRUE,TRUE);
   //   if(strcmp(Textdevicepath,"PciRoot(0x0)/Pci(0xF,0x0)"))
   //   {  
   //      // Find_Flag=1;
   //      j=i;
   //      myHandle=HandleAtaatapipassthru[i];
  
   //      Print(L"The handle of the required satacontroller is 0x%x\n",myHandle);
  
   //      DevPathNode=DevicePath;
   //      if(!IsDevicePathEnd(DevPathNode))
   //      {
   //        Print(L"(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
   //        DevPathNode=NextDevicePathNode(DevPathNode);
   //      }
   //      while(!IsDevicePathEnd(DevPathNode))
   //      {
   //        Print(L"/(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
   //        DevPathNode=NextDevicePathNode(DevPathNode);
   //      }
   //      Print(L"\n");
  
  
  
   //      Print(L"%s",Textdevicepath);
   //      Print(L"\n\n");
   //      if(Textdevicepath) gBS->FreePool(Textdevicepath);
   //      break;
   //   }
   //   if(Textdevicepath) gBS->FreePool(Textdevicepath);
   // }
 }
 return Status;
}
 // Print(L"%d\n\n",j);









// ///////////////////////////////////////// Check the SATA controller's mode /////////////////////////////////////
//   Status  = gBS->HandleProtocol (
//                    HandleAtaatapipassthru[j],
//                    &gEfiAtaPassThruProtocolGuid,
//                    (VOID **) &Instance
//                    );

//   ASSERT_EFI_ERROR (Status);


//   Status = gBS->HandleProtocol (
//                  HandleAtaatapipassthru[j],
//                  &gEfiPciIoProtocolGuid,
//                  (VOID**) &PciIo
//                  );
//   ASSERT_EFI_ERROR (Status);

//   Status = PciIo->Pci.Read (
//                         PciIo,
//                         EfiPciIoWidthUint8,
//                         0x0A,
//                         0x1,
//                         &ClassCode
//                         );
//   ASSERT_EFI_ERROR (Status);


//   if(ClassCode==0x01){
//      Print(L"IDE MODE\n");
//   }
//   else if(ClassCode==0x06){
//      Print(L"AHCI MODE\n");
//   }






// /////////////////////////////////////////////// Get the right SATA device based on the diskio and blockio protocol ////////////////////////
//   Status=gBS->LocateHandleBuffer(
//       ByProtocol,
//       &gEfiBlockIoProtocolGuid,
//       NULL,
//       &Number2,
//       &Handleblockio
//   );
//   ASSERT_EFI_ERROR (Status);
//   if (EFI_ERROR (Status))
//   {
//     Print(L"LocateBlockIoProtocol Error!\n");
//   }
//   else
//   {
//     Print(L"LocateBlockIoProtocol Success! Number2=%d\n",Number2);
//   }



//   Status=gBS->LocateHandleBuffer(
//       ByProtocol,
//       &gEfiDiskIoProtocolGuid,
//       NULL,
//       &Number3,
//       &Handlediskio
//   );
//   ASSERT_EFI_ERROR (Status);
//   if (EFI_ERROR (Status))
//   {
//     Print(L"LocatediskIoProtocol Error!\n");
//   }
//   else
//   {
//     Print(L"LocatediskIoProtocol Success! Number3=%d\n",Number3);
//   }






//   for (i=0;i<Number2;i++)
//   {
//      for(j=0;j<Number3;j++)
//      {
//        Find_Flag=0;
//        Print(L"i=%d,j=%d\n",i,j);
//        if(Handleblockio[i]==Handlediskio[j])
//         {

//           Status = gBS->HandleProtocol (
//                      Handlediskio[j],
//                      &gEfiDevicePathProtocolGuid,
//                      (VOID**) &DevicePath
//                      );



//           DevPathNode = DevicePath;
//           while (!IsDevicePathEnd (DevPathNode))
//           {
          
//            if((DevPathNode->Type==0x3)&&(DevPathNode->SubType==0x1))
//             {    
//                  DEBUG((EFI_D_ERROR,"Sata IDE Mode\n"));
//                  Find_Flag = 1;
//             }
//            else if((DevPathNode->Type==0x3)&&(DevPathNode->SubType==0x12))
//             {
//                  DEBUG((EFI_D_ERROR,"Sata AHCI Mode\n"));
//                  Find_Flag = 1;
//             }
//            DevPathNode = NextDevicePathNode (DevPathNode);
//           }




//           if(Find_Flag==1)
//           {
//             DevPathNode = DevicePath;
//             if(!IsDevicePathEnd (DevPathNode))
//             {
//              Print(L"(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
//              DevPathNode = NextDevicePathNode (DevPathNode);
//             }
//             while(!IsDevicePathEnd (DevPathNode))
//             {
//              Print(L"/(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
//              DevPathNode = NextDevicePathNode (DevPathNode);
//             }
//             Print(L"\n");

//             DevPathNode = DevicePath;
//             if (!IsDevicePathEnd (DevPathNode))
//             {
//              Textdevicepath=Device2TextProtocol->ConvertDevicePathToText(DevPathNode,TRUE,TRUE);
//              Print(L"%s",Textdevicepath);
//              if(Textdevicepath) gBS->FreePool(Textdevicepath);
//             }
//             Print(L"\n\n");





//             Print(L"j=%d\n",j);
//             Status = gBS->HandleProtocol (
//                      Handleblockio[i],
//                      &gEfiBlockIoProtocolGuid,
//                      (VOID**) &BlockIo
//                      );
//             AtaDevice = ATA_DEVICE_FROM_BLOCK_IO (BlockIo);

//             Packet=&AtaDevice->Packet;
//             Print(L"0x%x\n",Packet->OutDataBuffer);
//             goto here;
//           }
//         }
//      }
//   }






///////////////////////////////////////////////////  Issue the command to the SATA device /////////////////////
// here:
//   Print(L"Yes!\n");
//   // AhciDmaTransfer(Instance,AhciRegisters,0,0,)
//   Status = AhciDmaTransfer(
//              Instance,
//              &Instance->AhciRegisters,
//              0,
//              0,
//              NULL,
//              0,
//              TRUE,
//              Packet->Acb,
//              Packet->Asb,
//              Packet->InDataBuffer,
//              Packet->InTransferLength,
//              Packet->Timeout,
//              NULL
//              );

//   return Status;
// }


