%echo off
cd C:/PRJ_Microsemi/MMC2B_Rel6/SoftConsole/MMC2_MSS_MSS_CM3_0/Bootloader_prj
arm-none-eabi-size ./Debug/Bootloader_prj
arm-none-eabi-size ./Debug/Bootloader_prj > size_debug.txt
set /p tmp="Press a key to close this window"