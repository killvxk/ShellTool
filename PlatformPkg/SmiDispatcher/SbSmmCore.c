
#include "SbSmm.h"
#include "SmmHelpers.h"
#include <Protocol/SmmBase2.h>
#include <Protocol/SmmControl2.h>
//
// /////////////////////////////////////////////////////////////////////////////
// MODULE / GLOBAL DATA
//
UINT32                mAcpiBaseAddr;
SB_SMI_PRIVATE_DATA   mPrivateData = {  // for the structure
  {
    NULL
  },                                    // 
  NULL,                                 // 
  {NULL, NULL, NULL, NULL, NULL}, // EFI handle returned when calling SmmInstallProtocolInterface
  {                                     // protocol arrays
    {
      PROTOCOL_SIGNATURE,
      UsbType,
      {
        (SB_SMM_GENERIC_REGISTER)SbSmmCoreRegister,
        (SB_SMM_GENERIC_UNREGISTER)SbSmmCoreUnRegister
      }
    },  
    {
      PROTOCOL_SIGNATURE,
      SxType,
      {
        (SB_SMM_GENERIC_REGISTER) SbSmmCoreRegister,
        (SB_SMM_GENERIC_UNREGISTER) SbSmmCoreUnRegister
      }
    },
    {
      PROTOCOL_SIGNATURE,
      SwType,
      {
        (SB_SMM_GENERIC_REGISTER)SbSmmCoreRegister,
        (SB_SMM_GENERIC_UNREGISTER)SbSmmCoreUnRegister,
        (UINTN) MAX_SWI_VALUE
      }
    },
    {
      PROTOCOL_SIGNATURE,
      PowerButtonType,
      {
        (SB_SMM_GENERIC_REGISTER) SbSmmCoreRegister,
        (SB_SMM_GENERIC_UNREGISTER) SbSmmCoreUnRegister
      }
    },
    {
      PROTOCOL_SIGNATURE,
      PeriodicTimerType,
      {
        (SB_SMM_GENERIC_REGISTER) SbSmmCoreRegister,
        (SB_SMM_GENERIC_UNREGISTER) SbSmmCoreUnRegister,
        0
      }
    },
  }
};

CONTEXT_FUNCTIONS  mContextFunctions[SbSmmProtocolTypeMax] = 
{
  {
    NULL,
    NULL,
    NULL
  },
  {
    SxGetContext,
    SxCmpContext,
    NULL
  },
  {
    SwGetContext,
    SwCmpContext,
    SwGetBuffer
  },
  {
    PowerButtonGetContext,
    PowerButtonCmpContext,
    NULL
  },
  {
    NULL,
    PeriodicTimerCmpContext,
    NULL
  },
};


EFI_STATUS
EFIAPI
SbSmmCoreDispatcher (
  IN EFI_HANDLE             SmmImageHandle,
  IN     CONST VOID         *ContextData,            OPTIONAL
  IN OUT VOID               *CommunicationBuffer,    OPTIONAL
  IN OUT UINTN              *SourceSize              OPTIONAL
  );


EFI_STATUS
EFIAPI
SmmDispatcherInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mAcpiBaseAddr = PcdGet16(AcpiIoPortBaseAddress);
  SbSmmPublishDispatchProtocols ();
  gSmst->SmiHandlerRegister (SbSmmCoreDispatcher, NULL,  &mPrivateData.SmiHandle);	
  InitializeListHead(&mPrivateData.CallbackDataBase);
  SbSmmInitHardware();

  return EFI_SUCCESS;
}

EFI_STATUS
SmiInputValueDuplicateCheck (
  UINTN           SwSmiInputValue
  )
{

  DATABASE_RECORD *RecordInDb;
  LIST_ENTRY      *LinkInDb;


  if(SwSmiInputValue < MIN_SMI_VALUE){
    return EFI_INVALID_PARAMETER;
  }  

  LinkInDb = GetFirstNode (&mPrivateData.CallbackDataBase);
  while (!IsNull (&mPrivateData.CallbackDataBase, LinkInDb)) {
    RecordInDb = DATABASE_RECORD_FROM_LINK (LinkInDb);
    if (RecordInDb->ProtocolType == SwType) {
      if (RecordInDb->ChildContext.Sw.SwSmiInputValue == SwSmiInputValue) {
        return EFI_INVALID_PARAMETER;
      }
    }

    LinkInDb = GetNextNode (&mPrivateData.CallbackDataBase, &RecordInDb->Link);
  }

  return EFI_SUCCESS;
}


EFI_STATUS
SmiInputValueAssignment (
  UINTN           *SwSmiValue
  )
{
  UINTN           CheckValue;
  BOOLEAN         Found;
  EFI_STATUS      Status;

  Found = FALSE;
  CheckValue = MIN_SMI_VALUE;
  while (!Found && CheckValue <= MAX_SWI_VALUE) {
    Status = SmiInputValueDuplicateCheck (CheckValue);
    if (Status == EFI_SUCCESS) {
      Found = TRUE;
      break;
    }
    CheckValue++;
  }

  if (!Found) {
    return EFI_OUT_OF_RESOURCES;
  }

  *SwSmiValue = CheckValue;
  return EFI_SUCCESS;
}


EFI_STATUS
SbSmmCoreRegister  (
  IN  SB_SMM_GENERIC_PROTOCOL  *This,
  IN  SB_SMM_CALLBACK          DispatchFunction,
  IN  SB_SMM_CONTEXT           *DispatchContext,
  OUT EFI_HANDLE               *DispatchHandle
  )
{
  EFI_STATUS                 Status;
  DATABASE_RECORD            *Record;
  SB_SMM_QUALIFIED_PROTOCOL  *Qualified;
  SB_SMM_SOURCE_DESC         NullSourceDesc = NULL_SOURCE_DESC_INITIALIZER;

  //
  // Create database record and add to database
  //
  if (gSmst == NULL) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gSmst->SmmAllocatePool (EfiRuntimeServicesData, sizeof (DATABASE_RECORD), &Record);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Gather information about the registration request
  //
  CopyMem(&Record->ChildContext, DispatchContext, sizeof(SB_SMM_CONTEXT));  
  Record->Callback          = DispatchFunction;
  Qualified                 = QUALIFIED_PROTOCOL_FROM_GENERIC (This);
  Record->ProtocolType      = Qualified->Type;
  Record->ContextFunctions  = mContextFunctions[Qualified->Type];
  Record->SourceSize        = 0;
  //
  // Perform linked list housekeeping
  //
  Record->Signature = DATABASE_RECORD_SIGNATURE;

//DEBUG((EFI_D_INFO, "SbSmmCoreRegister(%X)\n", Qualified->Type));

  switch (Qualified->Type) {
  //
  // By the end of this switch statement, we'll know the
  // source description the child is registering for
  //
  case UsbType:
    //
    // Check the validity of Context Type
    //
    if ((Record->ChildContext.Usb.Type < UsbLegacy) || (Record->ChildContext.Usb.Type > UsbWake)) {
      Status = EFI_INVALID_PARAMETER;
      goto Error;
    }

    InsertTailList (&mPrivateData.CallbackDataBase, &Record->Link);
    MapUsbToSrcDesc (DispatchContext, &(Record->SrcDesc));
    Record->ClearSource = NULL;
    //
    // use default clear source function
    //
    break;

  case SxType:
    //
    // Check the validity of Context Type and Phase
    //
    if ((Record->ChildContext.Sx.Type < SxS0) ||
        (Record->ChildContext.Sx.Type >= EfiMaximumSleepType) ||
        (Record->ChildContext.Sx.Phase < SxEntry) ||
        (Record->ChildContext.Sx.Phase >= EfiMaximumPhase)
        ) {
      Status = EFI_INVALID_PARAMETER;
      goto Error;
    }

    InsertTailList (&mPrivateData.CallbackDataBase, &Record->Link);
    CopyMem ((VOID *) &(Record->SrcDesc), (VOID *) (&SX_SOURCE_DESC), sizeof (SB_SMM_SOURCE_DESC));
    Record->ClearSource = NULL;
    //
    // use default clear source function
    //
    break;

  case SwType:
    //
    // Check the validity of Context Value
    //
//- DEBUG((EFI_D_INFO, "SwVal <- %X\n", Record->ChildContext.Sw.SwSmiInputValue));
    if (Record->ChildContext.Sw.SwSmiInputValue == (UINTN) -1) {
      if (SmiInputValueAssignment (&(Record->ChildContext.Sw.SwSmiInputValue)) != EFI_SUCCESS) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Error;
      }
      DispatchContext->Sw.SwSmiInputValue = Record->ChildContext.Sw.SwSmiInputValue;
    }

    Status = SmiInputValueDuplicateCheck(Record->ChildContext.Sw.SwSmiInputValue);
    if (EFI_ERROR(Status)) {
      Status = EFI_INVALID_PARAMETER;
      goto Error;
    }

    DEBUG((EFI_D_INFO, "SwVal:0x%X\n", DispatchContext->Sw.SwSmiInputValue));

    InsertTailList (&mPrivateData.CallbackDataBase, &Record->Link);
    CopyMem ((VOID *) &(Record->SrcDesc), (VOID *) (&SW_SOURCE_DESC), sizeof (SB_SMM_SOURCE_DESC));
    Record->ClearSource = NULL;
    Record->SourceSize = sizeof (EFI_SMM_SW_REGISTER_CONTEXT);
    //
    // use default clear source function
    //
    break;

  case PowerButtonType:
    //
    // Check the validity of Context Phase
    //
    if ((Record->ChildContext.PowerButton.Phase < EfiPowerButtonEntry) ||
        (Record->ChildContext.PowerButton.Phase > EfiPowerButtonExit)
        ) {
      Status = EFI_INVALID_PARAMETER;
      goto Error;
    }

    InsertTailList (&mPrivateData.CallbackDataBase, &Record->Link);
    CopyMem ((VOID *) &(Record->SrcDesc), (VOID *) &POWER_BUTTON_SOURCE_DESC, sizeof (SB_SMM_SOURCE_DESC));
    Record->ClearSource = NULL;
    //
    // use default clear source function
    //
    break;

  case PeriodicTimerType:
    //
    // Check the validity of timer value
    //
    if (DispatchContext->PeriodicTimer.SmiTickInterval <= 0) {
      Status = EFI_INVALID_PARAMETER;
      goto Error;
    }

    InsertTailList (&mPrivateData.CallbackDataBase, &Record->Link);
    MapPeriodicTimerToSrcDesc (DispatchContext, &(Record->SrcDesc));
    Record->SourceSize  = 0;
    Record->ClearSource = NULL;
    break;

  default:
    return EFI_INVALID_PARAMETER;
    break;
  }

  if (CompareSources (&Record->SrcDesc, &NullSourceDesc)) {
    Status = EFI_INVALID_PARAMETER;
    goto Error;
  }

  if (Record->ClearSource == NULL) {
    //
    // Clear the SMI associated w/ the source using the default function
    //
    SbSmmClearSource (&Record->SrcDesc);
  } else {
    //
    // This source requires special handling to clear
    //
    Record->ClearSource (&Record->SrcDesc);
  }

  SbSmmEnableSource (&Record->SrcDesc);

  //
  // Child's handle will be the address linked list link in the record
  //
  *DispatchHandle = (EFI_HANDLE) (&Record->Link);

  return EFI_SUCCESS;

Error:
  gSmst->SmmFreePool (Record);
  //
  // DEBUG((EFI_D_ERROR,"Free pool status %d\n", Status ));
  //
  return Status;
}

EFI_STATUS
SbSmmCoreUnRegister (
  IN SB_SMM_GENERIC_PROTOCOL    *This,
  IN EFI_HANDLE                 *DispatchHandle
  )
/*++

Routine Description:

  Unregister a child SMI source dispatch function with a parent SMM driver.

Arguments:

  This                    Pointer to the  SB_SMM_GENERIC_PROTOCOL instance.
  DispatchHandle          Handle of dispatch function to deregister.

Returns:

  EFI_SUCCESS             The dispatch function has been successfully 
                          unregistered and the SMI source has been disabled
                          if there are no other registered child dispatch
                          functions for this SMI source.
  EFI_INVALID_PARAMETER   Handle is invalid.
  
--*/
{
  //
  // BOOLEAN SafeToDisable;
  //
  DATABASE_RECORD *RecordToDelete;

  //
  // DATABASE_RECORD *RecordInDb;
  // EFI_LIST_NODE   *LinkInDb;
  //
  if (DispatchHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  RecordToDelete = DATABASE_RECORD_FROM_LINK (DispatchHandle);

  //
  // Take the entry out of the linked list
  //
//  if (RecordToDelete->Link.ForwardLink == (LIST_ENTRY *) EFI_BAD_POINTER) {
//    return EFI_INVALID_PARAMETER;
//  }

  RemoveEntryList (&RecordToDelete->Link);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SbSmmCoreDispatcher (
  IN EFI_HANDLE             SmmImageHandle,
  IN     CONST VOID         *ContextData,            OPTIONAL
  IN OUT VOID               *CommunicationBuffer,    OPTIONAL
  IN OUT UINTN              *SourceSize              OPTIONAL
  )
/*++

Routine Description:

  The callback function to handle subsequent SMIs.  This callback will be called by SmmCoreDispatcher.

Arguments:

  SmmImageHandle          Not used
  CommunicationBuffer     Not used
  SourceSize              Not used

Returns:

  EFI_SUCCESS             Function successfully completed
  

--*/
{
  //
  // Used to prevent infinite loops
  //
  UINTN               EscapeCount;
  BOOLEAN             ContextsMatch;
  BOOLEAN             SmiClear;
  BOOLEAN             SxChildWasDispatched;
  DATABASE_RECORD     *RecordInDb;
  LIST_ENTRY          *LinkInDb;
  DATABASE_RECORD     *RecordToExhaust;
  LIST_ENTRY          *LinkToExhaust;
  SB_SMM_CONTEXT      Context;
  EFI_STATUS          Status;
  SB_SMM_SOURCE_DESC  ActiveSource = NULL_SOURCE_DESC_INITIALIZER;
  UINTN               SwSrcSize;
  VOID                *SwSrcBuf;


  EscapeCount           = 100;
  ContextsMatch         = FALSE;
  SmiClear              = FALSE;
  SxChildWasDispatched  = FALSE;
  Status                = EFI_SUCCESS;

  
  if (!IsListEmpty (&mPrivateData.CallbackDataBase)) {
    //
    // We have children registered w/ us -- continue
    //
    while ((!SmiClear) && (EscapeCount > 0)) {
      EscapeCount--;

      LinkInDb = GetFirstNode (&mPrivateData.CallbackDataBase);

      while (!IsNull (&mPrivateData.CallbackDataBase, LinkInDb)) {
        RecordInDb = DATABASE_RECORD_FROM_LINK (LinkInDb);

        //
        // look for the first active source
        //
        if (!SourceIsActive (&RecordInDb->SrcDesc)) {
          //
          // Didn't find the source yet, keep looking
          //
          LinkInDb = GetNextNode (&mPrivateData.CallbackDataBase, &RecordInDb->Link);

          //
          // if it's the last one, try to clear EOS
          //
          if (IsNull (&mPrivateData.CallbackDataBase, LinkInDb)) {
            SmiClear = SbSmiSetInActive();
          }
        } else {
          //
          // We found a source. If this is a sleep type, we have to go to
          // appropriate sleep state anyway.No matter there is sleep child or not
          //
          if (RecordInDb->ProtocolType == SxType) {
            SxChildWasDispatched = TRUE;
          }
          //
          // "cache" the source description and don't query I/O anymore
          //
          CopyMem ((VOID *) &ActiveSource, (VOID *) &(RecordInDb->SrcDesc), sizeof (SB_SMM_SOURCE_DESC));
          LinkToExhaust = LinkInDb;

          //
          // exhaust the rest of the queue looking for the same source
          //
          while (!IsNull (&mPrivateData.CallbackDataBase, LinkToExhaust)) {
            RecordToExhaust = DATABASE_RECORD_FROM_LINK (LinkToExhaust);
            //
            // RecordToExhaust->Link might be removed (unregistered) by Callback function, and then the
            // system will hang in ASSERT() while calling GetNextNode().
            // To prevent the issue, we need to get next record in DB here (before Callback function). 
            //
            LinkToExhaust = GetNextNode (&mPrivateData.CallbackDataBase, &RecordToExhaust->Link);

            if (CompareSources (&RecordToExhaust->SrcDesc, &ActiveSource)) {
              //
              // These source descriptions are equal, so this callback should be
              // dispatched.
              //
              if (RecordToExhaust->ContextFunctions.GetContext != NULL) {
                //
                // This child requires that we get a calling context from
                // hardware and compare that context to the one supplied
                // by the child.
                //
                ASSERT (RecordToExhaust->ContextFunctions.CmpContext != NULL);

                //
                // Make sure contexts match before dispatching event to child
                //
                RecordToExhaust->ContextFunctions.GetContext (RecordToExhaust, &Context);
                ContextsMatch = RecordToExhaust->ContextFunctions.CmpContext (&Context, &RecordToExhaust->ChildContext);

              } else {
                //
                // This child doesn't require any more calling context beyond what
                // it supplied in registration.  Simply pass back what it gave us.
                //
                ASSERT (RecordToExhaust->Callback != NULL);
                Context       = RecordToExhaust->ChildContext;
                ContextsMatch = TRUE;
              }

              if (ContextsMatch) {
                if (RecordToExhaust->SourceSize != 0) {
                  ASSERT (RecordToExhaust->ContextFunctions.GetBuffer != NULL);
                  RecordToExhaust->ContextFunctions.GetBuffer (RecordToExhaust);
                  SwSrcBuf  = &RecordToExhaust->CommBuffer;
                  SwSrcSize = RecordToExhaust->SourceSize;
                } else {
                  SwSrcBuf  = NULL;
                  SwSrcSize = 0;
                }

                ASSERT (RecordToExhaust->Callback != NULL);

                RecordToExhaust->Callback ((EFI_HANDLE)&RecordToExhaust->Link, &Context, SwSrcBuf, &SwSrcSize);

                if (RecordToExhaust->ProtocolType == SxType) {
                  SxChildWasDispatched = TRUE;
                }
              }
            }
          }

          if (RecordInDb->ClearSource == NULL) {
            //
            // Clear the SMI associated w/ the source using the default function
            //
            SbSmmClearSource (&ActiveSource);
          } else {
            //
            // This source requires special handling to clear
            //
            RecordInDb->ClearSource (&ActiveSource);
          }
          //
          // Also, try to clear EOS
          //
          SmiClear = SbSmiSetInActive();
          //
          // Queue is empty, reset the search
          //
          break;
        }
      }
    }
  }
  //
  // If you arrive here, there are two possible reasons:
  // (1) you've got problems with clearing the SMI status bits in the
  // ACPI table.  If you don't properly clear the SMI bits, then you won't be able to set the
  // EOS bit.  If this happens too many times, the loop exits.
  // (2) there was a SMM communicate for callback messages that was received prior
  // to this driver.
  // If there is an asynchronous SMI that occurs while processing the Callback, let
  // all of the drivers (including this one) have an opportunity to scan for the SMI
  // and handle it.
  // If not, we don't want to exit and have the foreground app. clear EOS without letting
  // these other sources get serviced.
  //
  // This assert is not valid with CSM legacy solution because it generates software SMI
  // to test for legacy USB support presence.
  // This may not be illegal, so we cannot assert at this time.
  //
  //  ASSERT (EscapeCount > 0);
  //
  if (SxChildWasDispatched) {
    //
    // A child of the SmmSxDispatch protocol was dispatched during this call;
    // put the system to sleep.
    //
    SbSmmSxGoToSleep ();
  }

  return Status;
}
