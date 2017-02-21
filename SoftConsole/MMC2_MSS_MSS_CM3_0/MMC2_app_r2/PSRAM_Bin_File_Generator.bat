cd C:/PRJ_Microsemi/MMC2B_Rel7/SoftConsole/MMC2_MSS_MSS_CM3_0/MMC2_app_r2
arm-none-eabi-objcopy.exe -O binary ./Release_PSRAM/MMC2_app_r2 "MMC2_sw.bin"

set /p tmp="Press a key to close this window"