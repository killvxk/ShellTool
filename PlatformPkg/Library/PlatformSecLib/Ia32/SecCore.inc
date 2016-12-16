
;------------------------------------------------------------------------------
STATUS_CODE MACRO status
  mov  al,  status
  out  80h, al
ENDM




CALL_MMX macro   RoutineLabel
  local   ReturnAddress
  mov     esi, offset ReturnAddress
  movd    mm7, esi                      ; save ReturnAddress into MM7
  jmp     RoutineLabel
ReturnAddress:
endm

RET_MMX  macro
  movd    esi, mm7                      ; restore ESP from MM7
  jmp     esi
endm

CALL_EBP macro   RoutineLabel
  local   ReturnAddress
  mov     ebp, offset ReturnAddress
  jmp     RoutineLabel
ReturnAddress:
endm

RET_EBP  macro
  jmp     ebp                           ; restore ESP from EBP
endm



