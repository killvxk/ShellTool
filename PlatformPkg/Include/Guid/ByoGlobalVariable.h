/** @file
  GUID for Byo global Variable.

  Copyright (c) 2006 - 2009, Byo Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  GUID defined in UEFI 2.1
**/

#ifndef __BYO_GLOBAL_VARIABLE_H__
#define __BYO_GLOBAL_VARIABLE_H__

#define BYO_GLOBAL_VARIABLE_GUID \
  { \
    0x97bf7a1b, 0x2ed2, 0x4f68, { 0x8f, 0xe0, 0x91, 0xf5, 0xbb, 0x6d, 0xeb, 0x15 } \
  }
 extern EFI_GUID gByoGlobalVariableGuid;

#endif
