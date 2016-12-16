;------------------------------------------------------------------------------
; X64 assembly file for AP startup vector.
;
; Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

.code

;-------------------------------------------------------------------------------------
;  YEfiReadMsr (&AddressM); Pass Parameter by common register
;-------------------------------------------------------------------------------------
YLEfiReadMsr PROC  PUBLIC 
    rdmsr
    sal     rdx, 32   ; edx:eax -> rax
    or      rax, rdx  ; rax = edx:eax
    ret
YLEfiReadMsr  ENDP
END
