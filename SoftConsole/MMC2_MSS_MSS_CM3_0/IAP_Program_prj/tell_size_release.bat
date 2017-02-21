@REM echo off
cd C:/PRJ_Microsemi/MMC2B_Rel7/SoftConsole/MMC2_MSS_MSS_CM3_0/IAP_Program_prj
arm-none-eabi-size ./Release/IAP_Program_prj
arm-none-eabi-size ./Release/IAP_Program_prj > size_release.txt
set /p tmp="Press a key to close this window"