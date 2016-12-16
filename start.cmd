@echo off

set MAP_DRV_NO=Z

if exist %MAP_DRV_NO%: subst %MAP_DRV_NO%: /d 
subst %MAP_DRV_NO%: .

cd /d %MAP_DRV_NO%:

echo ----------------------------------
echo [STEP1] call edksetup.bat
echo [STEP2] build -a X64 -p shellpkg/shellpkg.dsc

cmd /k

@cmd /k

