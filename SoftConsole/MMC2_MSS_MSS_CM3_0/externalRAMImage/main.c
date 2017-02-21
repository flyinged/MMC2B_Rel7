/*******************************************************************************
* Company: Microsemi Corporation
*
* File: main.c
* File history:
*      Revision: 1.0 Date: May 4, 2010
*
* Description:
*
* This sample program is targeted to the SmartFusion Development Kit.It blinks
* the development board's LEDs (D1,D2 and D3)using a delay loop. The states of
* LEDs are controlled through MSS GPIO 0,GPIO 1 and GPIO 2 configured as outputs.
* This application aslo prints memory location of the global variable on
* the HyperTerminal.
*
* Author: Upender Cherukupally
*         Upender.Cherukupally@microsemi.com
*         Corporate Applications Engineering
*
*******************************************************************************/

#include "drivers/mss_gpio/mss_gpio.h"
#include "drivers/mss_watchdog/mss_watchdog.h"
#include <stdio.h>

/*
 * Delay loop down counter load value.
 */
#define DELAY_LOAD_VALUE     0x00100000

uint8_t x = 0x55;

char testString[] = "Smart Fusion Says Hello\r\n";
/*-------------------------------------------------------------------------*//**
 * main() function.
 */
int main()
{


	volatile int32_t delay_count = 0;

   /*--------------------------------------------------------------------------
    * Disable watchdog.
    */
	MSS_WD_disable();

    /*
     * Initialize MSS GPIOs.
     */
    MSS_GPIO_init();

    /*
     * Configure MSS GPIOs.
     */
    MSS_GPIO_config( MSS_GPIO_0 , MSS_GPIO_OUTPUT_MODE );
    MSS_GPIO_config( MSS_GPIO_1 , MSS_GPIO_OUTPUT_MODE );
    MSS_GPIO_config( MSS_GPIO_2 , MSS_GPIO_OUTPUT_MODE );

    /*
     * Set initial delay used to blink the LED.
     */
    delay_count = DELAY_LOAD_VALUE;

    while (x != 0x55)
    	;

    /*
     * Infinite loop.
     */
    for(;;)
    {
        uint32_t gpio_pattern;
        /*
         * Decrement delay counter.
         */
        --delay_count;

        /*
         * Check if delay expired.
         */
        if ( delay_count <= 0 )
        {
            /*
             * Reload delay counter.
             */
            delay_count = DELAY_LOAD_VALUE;

            /*
             * Toggle GPIO output pattern by doing an exclusive OR of all
             * pattern bits with ones.
             */
            gpio_pattern = MSS_GPIO_get_outputs();
            gpio_pattern ^= 0xFFFFFFFF;
            MSS_GPIO_set_outputs( gpio_pattern );

            printf("%s This string is stored at %p\r\n",testString, &testString);
        }
    }

    return 0;
}
