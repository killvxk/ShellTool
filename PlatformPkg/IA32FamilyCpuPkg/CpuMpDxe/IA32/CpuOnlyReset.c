/*++
  This file contains an 'Intel Peripheral Driver' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/** @file                                                          

  Code for cpu only reset.                                
                                                                   
  Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
  This software and associated documentation (if any) is furnished 
  under a license and may only be used or copied in accordance     
  with the terms of the license. Except as permitted by such       
  license, no part of this software or documentation may be        
  reproduced, stored in a retrieval system, or transmitted in any  
  form or by any means without the express written consent of      
  Intel Corporation.                                               
                                                                   
  Module Name:  CpuOnlyReset.c                                  
                                                                   
**/

#include "CpuOnlyReset.h"

/**
  Initiate CPU only reset.
  
  This function will save CPU context, call SetJump to mark the resume pointer
  and program MCH to trigger CPU only reset.

**/
VOID
InitiateCpuOnlyReset(
  VOID
  )
{
	return; 
}
