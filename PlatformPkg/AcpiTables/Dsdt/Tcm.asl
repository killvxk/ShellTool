/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  tcm.asl

Abstract: 
  acpi asl file for tcm module.

Revision History:

Bug 3075 - Add TCM support.
TIME: 2011-11-14
$AUTHOR: Zhang Lin
$REVIEWERS: 
$SCOPE: SugarBay
$TECHNICAL: 
  1. Tcm module init version.
     Only support setup function.
$END--------------------------------------------------------------------

**/




Device (\_SB.TCM)
{
    Name(_HID, EISAID("ZIC0101"))
    Name(_STR, Unicode("TCM 1.2 Device"))
    Name(_CRS, ResourceTemplate(){
      Memory32Fixed(ReadWrite, 0xfed40000, 0x5000)
    })

    OperationRegion(TCMR, SystemMemory, 0xfed40000, 0x5000)
    Field(TCMR, AnyAcc, NoLock, Preserve){
        ACC0, 8,
    }
 
    Method(_STA, 0){
//      If(LEqual(\TCMP, 0)){
//        Return(0)
//      }
      if(LEqual(ACC0, 0xff)){
        Return(0)
      }
      Return (0x0f)
    }
}
