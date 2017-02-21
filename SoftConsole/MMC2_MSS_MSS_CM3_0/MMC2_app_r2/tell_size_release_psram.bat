%echo off
cd C:/PRJ_Microsemi/MMC2B_Rel7/SoftConsole/MMC2_MSS_MSS_CM3_0/MMC2_app_r2
arm-none-eabi-size ./Release_PSRAM/MMC2_app_r2
arm-none-eabi-size ./Release_PSRAM/MMC2_app_r2 > size_release_psram.txt
set /p tmp="Press a key to close this window"