
#include "SmmPlatform.h"
#include <Guid/CapsuleVendor.h>


VOID ClearDateAlarm()
{
// Guarantee day-of-month alarm is invalid (ACPI 1.0 section 4.7.2.4)
  CmosWrite(RTC_ADDRESS_DATE_ALARM, 0);
}


UINT8 gDayOfMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

VOID CalcNewDate(UINT16 Year, UINT8 *Month, UINT8 *Day, UINT8 WeekdayAdd)
{
  UINT8  MyDay;
  UINT8  MyMonth;

  if(IsLeapYear(Year)){gDayOfMonth[2-1] = 29;}  
  
  MyDay   = *Day + WeekdayAdd;
  MyMonth = *Month;
  if(MyDay > gDayOfMonth[*Month-1]){
    MyDay -= gDayOfMonth[*Month-1];
    MyMonth++;
  }
  if(MyMonth > 12){
    MyMonth -= 12;
  }

  *Month = MyMonth;
  *Day   = MyDay;
}

UINT8 GetNearestWeekdayIntervalNeeded(UINT8 CurWeekday, BOOLEAN TodayValid)
{
  UINT8  Index;
  UINT8  NewWeekDay;
  UINT8  WeekDayBitMap;

  WeekDayBitMap = (gSetupData.UserDefMon?BIT0:0)|
                  (gSetupData.UserDefTue?BIT1:0)|
                  (gSetupData.UserDefWed?BIT2:0)|
                  (gSetupData.UserDefThu?BIT3:0)|
                  (gSetupData.UserDefFri?BIT4:0)|
                  (gSetupData.UserDefSat?BIT5:0)|
                  (gSetupData.UserDefSun?BIT6:0);             
  Index = 0;
  if(!TodayValid){Index=1;}
  for(; Index<7; Index++){
    NewWeekDay = (CurWeekday+Index)%7;
    if((1<<NewWeekDay) & WeekDayBitMap){
      return Index;
    }
  }
  
  return 0xFF;
}



EFI_STATUS EnableS5RtcWake()
{
  EFI_STATUS            Status;
  RTC_REGISTER_B        RegisterB;
  UINT8                 WakeType;
  UINT8                 Second;
  UINT8                 Minute;  
  UINT8                 Hour;
  UINT16                Year;
  UINT8                 Month;
  UINT8                 Day;
  UINT8                 WeekDay;
  UINT16                CurYear;
  UINT8                 CurMonth;
  UINT8                 CurDay;
  UINT8                 CurWeekDay;
  UINT8                 WeekDayAdd;
  UINT8                 Cmos0B;
  BOOLEAN               Cmos0BChanged;
  UINT8                 CurHour;
  UINT8                 CurMinute;
  UINT8                 CurSecond;
  BOOLEAN               TodayValid;
  UINT64                TimeVal;
  UINT64                CurTimeVal;


  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__));

  Status        = EFI_SUCCESS;	
  Cmos0BChanged = FALSE;
  Cmos0B        = 2;
  
  WakeType = gSetupData.WakeOnRTC;
  if(WakeType == RTC_WAKE_VAL_DISABLE){
    goto ErrorExit;
  }
	
  Second   = gSetupData.RTCWakeupTime.Second;
  Minute   = gSetupData.RTCWakeupTime.Minute;
  Hour     = gSetupData.RTCWakeupTime.Hour;
  Year     = gSetupData.RTCWakeupDate.Year;
  Month    = gSetupData.RTCWakeupDate.Month;
  Day      = gSetupData.RTCWakeupDate.Day;
  WeekDay  = gSetupData.AlarmWeekDay;
	
  if(Second > 59 || Minute > 59 || Hour > 23 || 
     Day > 31 || Day < 1 || Month > 12 || Month < 1 ||
     WeekDay > 6){
    Status = EFI_INVALID_PARAMETER;
    DEBUG((EFI_D_ERROR, "(L%d)%r\n", __LINE__, Status));    
    goto ErrorExit;
  }

  Status = RtcWaitToUpdate();
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "(L%d)%r\n", __LINE__, Status));  
    goto ErrorExit;
  }   
  RegisterB.Data = CmosRead(RTC_ADDRESS_REGISTER_B);
  Cmos0B = RegisterB.Data;
  RegisterB.Bits.Set = 1;   // Updates inhibited
  RegisterB.Bits.Aie = 0;  
  RegisterB.Bits.Mil = 1;		// 24 hours
  RegisterB.Bits.Dm  = 0;		// BCD Format
  CmosWrite(RTC_ADDRESS_REGISTER_B, RegisterB.Data);
  Cmos0BChanged = TRUE;

// Now we could safely read/write register 0 ~ 9
  CurDay     = CheckAndConvertBcd8ToDecimal8(CmosRead(RTC_ADDRESS_DAY_OF_THE_MONTH));
  CurMonth   = CheckAndConvertBcd8ToDecimal8(CmosRead(RTC_ADDRESS_MONTH));
  CurYear    = CheckAndConvertBcd8ToDecimal8(CmosRead(RTC_ADDRESS_YEAR)) + 2000;		// assume 20 Century.
  CurWeekDay = CaculateWeekDay(CurYear, CurMonth, CurDay);
  CurHour    = CheckAndConvertBcd8ToDecimal8(CmosRead(RTC_ADDRESS_HOURS));
  CurMinute  = CheckAndConvertBcd8ToDecimal8(CmosRead(RTC_ADDRESS_MINUTES));
  CurSecond  = CheckAndConvertBcd8ToDecimal8(CmosRead(RTC_ADDRESS_SECONDS));

  if(WakeType == RTC_WAKE_VAL_SINGLE_EVENT){
    TimeVal    = LShiftU64(((Year<<16)    | (Month << 8)    | Day),    32) | ((Hour<<16)|(Minute<<8)|Second);
    CurTimeVal = LShiftU64(((CurYear<<16) | (CurMonth << 8) | CurDay), 32) | ((CurHour<<16)|(CurMinute<<8)|CurSecond);
    DEBUG((EFI_D_INFO, "Time   :%lX\n", TimeVal)); 
    DEBUG((EFI_D_INFO, "CurTime:%lX\n", CurTimeVal)); 		
    if(TimeVal < CurTimeVal){
      Status = EFI_INVALID_PARAMETER;
      goto ErrorExit;
    }
    DEBUG((EFI_D_ERROR, "[Single]%04d-%02d-%02d %02d:%02d:%02d\n", Year, Month, Day, Hour, Minute, Second));
    CmosWrite(RTC_ADDRESS_SECONDS_ALARM, DecimalToBcd8(Second));
    CmosWrite(RTC_ADDRESS_MINUTES_ALARM, DecimalToBcd8(Minute));
    CmosWrite(RTC_ADDRESS_HOURS_ALARM,   DecimalToBcd8(Hour));  
    CmosWrite(RTC_ADDRESS_DATE_ALARM,    DecimalToBcd8(Day));
    CmosWrite(RTC_ADDRESS_MONTH_ALARM,   DecimalToBcd8(Month));
    
  } else if(WakeType == RTC_WAKE_VAL_DAILY_EVENT){
    DEBUG((EFI_D_ERROR, "[Daily]%02d:%02d:%02d\n", Hour, Minute, Second));  
    CmosWrite(RTC_ADDRESS_SECONDS_ALARM, DecimalToBcd8(Second));
    CmosWrite(RTC_ADDRESS_MINUTES_ALARM, DecimalToBcd8(Minute));
    CmosWrite(RTC_ADDRESS_HOURS_ALARM,   DecimalToBcd8(Hour)); 
    CmosWrite(RTC_ADDRESS_DATE_ALARM,    0);
    CmosWrite(RTC_ADDRESS_MONTH_ALARM,   0);    
    
  } else if(WakeType == RTC_WAKE_VAL_WEEKLY_EVENT){
    DEBUG((EFI_D_ERROR, "%04d-%02d-%02d %X->%X\n", CurYear, CurMonth, CurDay, CurWeekDay, WeekDay)); 
    if(CurWeekDay <= WeekDay){
      WeekDayAdd = WeekDay - CurWeekDay;
    } else {
      WeekDayAdd = WeekDay + 7 - CurWeekDay;
    }
    Month = CurMonth;
    Day   = CurDay;
    CalcNewDate(CurYear, &Month, &Day, WeekDayAdd);
    DEBUG((EFI_D_ERROR, "[Weekly]%02d-%02d %02d:%02d:%02d\n", Month, Day, Hour, Minute, Second));    
    CmosWrite(RTC_ADDRESS_SECONDS_ALARM, DecimalToBcd8(Second));
    CmosWrite(RTC_ADDRESS_MINUTES_ALARM, DecimalToBcd8(Minute));
    CmosWrite(RTC_ADDRESS_HOURS_ALARM,   DecimalToBcd8(Hour));  
    CmosWrite(RTC_ADDRESS_DATE_ALARM,    DecimalToBcd8(Day));
    CmosWrite(RTC_ADDRESS_MONTH_ALARM,   DecimalToBcd8(Month));
    
  } else if(WakeType == RTC_WAKE_VAL_USER_DEFINED){
    DEBUG((EFI_D_ERROR, "Current: %04d-%02d-%02d %02d:%02d:%02d %d\n",  \
                        CurYear, CurMonth, CurDay,             \
                        CurHour, CurMinute, CurSecond,         \
                        CurWeekDay                             \
                        ));
    if((CurHour*3600 + CurMinute*60 + CurSecond) >= (Hour*3600 + Minute*60 + Second)){
      TodayValid = FALSE;
    } else {
      TodayValid = TRUE;
    }
    WeekDayAdd = GetNearestWeekdayIntervalNeeded(CurWeekDay, TodayValid);
    if(WeekDayAdd == 0xFF){Status = EFI_INVALID_PARAMETER;goto ErrorExit;}
    Month = CurMonth;
    Day   = CurDay;
    CalcNewDate(CurYear, &Month, &Day, WeekDayAdd);
    DEBUG((EFI_D_ERROR, "[User]%02d-%02d %02d:%02d:%02d\n", Month, Day, Hour, Minute, Second));    
    CmosWrite(RTC_ADDRESS_SECONDS_ALARM, DecimalToBcd8(Second));
    CmosWrite(RTC_ADDRESS_MINUTES_ALARM, DecimalToBcd8(Minute));
    CmosWrite(RTC_ADDRESS_HOURS_ALARM,   DecimalToBcd8(Hour)); 
    CmosWrite(RTC_ADDRESS_DATE_ALARM,    DecimalToBcd8(Day));
    CmosWrite(RTC_ADDRESS_MONTH_ALARM,   DecimalToBcd8(Month));
    
  } else {
    Status = EFI_INVALID_PARAMETER;
    goto ProcExit;
  }

  CmosRead(RTC_ADDRESS_REGISTER_C);  // Read 0xC to clear pending RTC interrupts  	
  RegisterB.Bits.Aie = 1;
  RegisterB.Bits.Set = 0;
  CmosWrite(RTC_ADDRESS_REGISTER_B, RegisterB.Data);
  IoWrite16(mAcpiBaseAddr + PMIO_STS_REG, PMIO_STS_RTC);
  IoOr16(mAcpiBaseAddr + PMIO_PM_EN, PMIO_PM_EN_RTC);  
  goto ProcExit;
  
ErrorExit:  
  if(Cmos0BChanged){
    CmosWrite(RTC_ADDRESS_REGISTER_B, Cmos0B);
  }
  
ProcExit:
  return Status;
}


BOOLEAN IsBiosWantUpdate()
{
  EFI_STATUS            Status;
  UINTN                 CapsuleDataPtr;
  UINTN                 DataSize;
  BOOLEAN               rc;

  if(mSmmVariable == NULL){
    rc = FALSE;
    goto ProcExit;
  }
  
  CapsuleDataPtr = 0;
  DataSize = sizeof(CapsuleDataPtr);
  Status = mSmmVariable->SmmGetVariable (
                           EFI_CAPSULE_VARIABLE_NAME,
                           &gEfiCapsuleVendorGuid,
                           NULL,
                           &DataSize,
                           &CapsuleDataPtr
                           );
  if(EFI_ERROR(Status) || CapsuleDataPtr==0){
    DEBUG((EFI_D_INFO, "NO CAPSULE\n"));
    rc = FALSE;
  } else {
    DEBUG((EFI_D_INFO, "CAPSULE:%r %X %d\n", Status, CapsuleDataPtr, DataSize));  
    rc = TRUE;
  }

ProcExit:  
  return rc;
}

// Change the S5 to an S3 (Synch with CHX001)
VOID SetSleepTypeS3()
{
  UINT16  Pm1Cnt;
  
  Pm1Cnt  = IoRead16(mAcpiBaseAddr + PMIO_PM1_CNT_REG); // PMIO_Rx04
  Pm1Cnt &= ~PMIO_PM1_CNT_SLP_TYP;
  Pm1Cnt |= PMIO_PM1_CNT_S3;
  IoWrite16(mAcpiBaseAddr + PMIO_PM1_CNT_REG, Pm1Cnt);
}
  
EFI_STATUS SetRtcWakeUpForCapsule(UINT8 SleepTimeSecond)
{
  EFI_STATUS            Status;
  RTC_REGISTER_B        RegisterB;  
  UINT8                 WakeupData;
  BOOLEAN               WakeIncrement;


  DEBUG((EFI_D_INFO, "%a(%d)\n", __FUNCTION__, SleepTimeSecond));
                           
  Status = RtcWaitToUpdate();
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "(L%d)%r\n", __LINE__, Status));  
    goto ProcExit;
  }   
  RegisterB.Data = CmosRead(RTC_ADDRESS_REGISTER_B);
  RegisterB.Bits.Set = 1;   // Updates inhibited
  RegisterB.Bits.Aie = 0;  
  RegisterB.Bits.Mil = 1;		// 24 hours
  RegisterB.Bits.Dm  = 0;		// BCD Format
  CmosWrite(RTC_ADDRESS_REGISTER_B, RegisterB.Data);
  
  CmosWrite(RTC_ADDRESS_DATE_ALARM,  0);
  CmosWrite(RTC_ADDRESS_MONTH_ALARM, 0);   
  
  WakeIncrement = FALSE;  
  WakeupData = CheckAndConvertBcd8ToDecimal8(CmosRead(RTC_ADDRESS_SECONDS)) + SleepTimeSecond;
  if (WakeupData >= 60) {
    WakeupData -= 60;
    WakeIncrement = TRUE;
  }
  CmosWrite(RTC_ADDRESS_SECONDS_ALARM, DecimalToBcd8(WakeupData));
  
  WakeupData = CheckAndConvertBcd8ToDecimal8(CmosRead(RTC_ADDRESS_MINUTES));
  if(WakeIncrement) {
    WakeIncrement = FALSE;
    WakeupData++;
    if(WakeupData >= 60){
      WakeupData -= 60;
      WakeIncrement = TRUE;
    }
  }  
  CmosWrite(RTC_ADDRESS_MINUTES_ALARM, DecimalToBcd8(WakeupData));

  WakeupData = CheckAndConvertBcd8ToDecimal8(CmosRead(RTC_ADDRESS_HOURS));
  if(WakeIncrement) {
    WakeupData++;
    if(WakeupData >= 24){
      WakeupData -= 24;
    }
  }  
  CmosWrite(RTC_ADDRESS_HOURS_ALARM, DecimalToBcd8(WakeupData));    

  DEBUG((EFI_D_INFO, "Cur:%02X:%02X:%02X\n", CmosRead(RTC_ADDRESS_HOURS), CmosRead(RTC_ADDRESS_MINUTES), CmosRead(RTC_ADDRESS_SECONDS)));
  DEBUG((EFI_D_INFO, "Wak:%02X:%02X:%02X\n", CmosRead(RTC_ADDRESS_HOURS_ALARM), CmosRead(RTC_ADDRESS_MINUTES_ALARM), CmosRead(RTC_ADDRESS_SECONDS_ALARM)));  

  CmosRead(RTC_ADDRESS_REGISTER_C);  // Read 0xC to clear pending RTC interrupts  	
  RegisterB.Bits.Aie = 1;
  RegisterB.Bits.Set = 0;
  CmosWrite(RTC_ADDRESS_REGISTER_B, RegisterB.Data);
  
  IoWrite16(mAcpiBaseAddr + PMIO_STS_REG, PMIO_STS_RTC);
  IoOr16(mAcpiBaseAddr + PMIO_PM_EN, PMIO_PM_EN_RTC);  

ProcExit:  
  return Status;
}


