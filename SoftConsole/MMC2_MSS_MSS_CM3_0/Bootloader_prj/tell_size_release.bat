%echo off
cd C:/PRJ_Microsemi/MMC2B_Rel7/SoftConsole/MMC2_MSS_MSS_CM3_0/Bootloader_prj
cd
arm-none-eabi-size ./Release/Bootloader_prj
arm-none-eabi-size ./Release/Bootloader_prj > size_release.txt
set /p tmp="Press a key to close this window"