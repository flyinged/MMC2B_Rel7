/*******************************************************************************
 * (c) Copyright 2008 Actel Corporation.  All rights reserved.
 *
 * Burn-in Design 1.0.101
 * 
 *
 * SVN $Revision:$
 * SVN $Date:$
 *
 * Custom version 1.0, Alessandro Malatesta
 * 1.1 - Added support for direct execution of action code (menu bypass)
 * 1.2 - Display passed parameters (spi base adddress and action code), added support for image offset in SPI
 * 1.3 - corrected passed parameters' addresses
 * 1.4 - Added self reset after action is finished
 * 1.5 - Corrected Systick settings to speedup processing, added clock divider control
 * 1.6 - Added watchdog control
 * 1.7 - Added more WD reloads
 * 1.8 - Removed reset at end of main()
 * 1.9 - Removed write to clock control register
 * D - Removed WD, added reset to end of main, added dbg prints, changed optimization
 * D8 - commented single failing instruction from dp_exit()
 * D9 - commented out whole dp_exit() function
 * 1.10 - Microsemi bugfix for dp_exit() function
 */

#include "main.h"
#include "hal.h"
#include "mss_uart.h"
#include "DirectC/dpuser.h"
#include "DirectC/dpalg.h"
#include "mss_watchdog.h"
#include "mss_iap.h"
//#include "core_i2c.h"
#include "MMC2_hw_platform.h"

#define NEW_SPI_FLASH_DRIVER
#ifdef NEW_SPI_FLASH_DRIVER
#include "s25fl128s.h"
#else
#include "spi_flash.h"
#endif

#define SOFT_RST_CR 0xE0042030
#define MSS_CLK_CR  0xE0042048

extern void bx_user_code_envm(void);
void uint_to_hexstr(uint32_t num, uint8_t *str, uint8_t ndigits);

//debug var
uint8_t dbg_var;

////added for i2c
//i2c_instance_t g_core_i2c_pm;
//volatile uint32_t tick_counter = 0;

void action_menu(void)
{
    uint8_t tx_data[25][40] = {
        {"\r\n\r\n   Action:                    "},
        {"\r\n01 DEVICE_INFO                    "},
        {"\r\n02 READ_IDCODE                    "},
        {"\r\n03 ERASE                          "},
        {"\r\n04 ERASE_ALL                      "},
        {"\r\n05 PROGRAM                        "},
        {"\r\n06 VERIFY                         "},
        {"\r\n07 ENC_DATA_AUTHENTICATION        "},
        {"\r\n08 ERASE_ARRAY                    "},
        {"\r\n09 PROGRAM_ARRAY                  "},
        {"\r\n10 VERIFY_ARRAY                   "},
        {"\r\n11 ERASE_FROM                     "},
        {"\r\n12 PROGRAM_FROM                   "},
        {"\r\n13 VERIFY_FROM                    "},
        {"\r\n14 ERASE_SECURITY                 "},
        {"\r\n15 PROGRAM_SECURITY               "},
        {"\r\n16 PROGRAM_NVM                    "},
        {"\r\n17 VERIFY_NVM                     "},
        {"\r\n18 VERIFY_DEVICE_INFO             "},
        {"\r\n19 READ_USERCODE                  "},
        {"\r\n20 PROGRAM_NVM_ACTIVE             "},
        {"\r\n21 VERIFY_NVM_ACTIVE              "},
        {"\r\n22 IS_CORE_CONFIGURED             "},
        {"\r\n23 Reset Cortex-M3                "},
        {"\r\n Enter 1,2,3,4 .. 23, accordingly "}
    };


    uint8_t rx_data[2];
    uint8_t rx_size =0;
    uint8_t valid_entry = 0;

    MSS_UART_polled_tx (&g_mss_uart0, tx_data[0],sizeof(tx_data));

    while (!valid_entry)
    {
        //no watchdog reset: let it timeout if no input
        do {
            rx_size = MSS_UART_get_rx(&g_mss_uart0, &rx_data[1],1);
        } while(rx_size == 0);
        MSS_UART_polled_tx (&g_mss_uart0, rx_data+1,1);
        do {
            rx_size = MSS_UART_get_rx(&g_mss_uart0, &rx_data[0],1);
        } while(rx_size == 0);
        MSS_UART_polled_tx (&g_mss_uart0, rx_data,1);

        Action_code = (rx_data[1] - 48) * 10 + (rx_data[0] - 48);
        if ((Action_code > 0 ) && (Action_code < 24))
        {
            valid_entry = 1;
        }
        else
        {
#ifdef ENABLE_INFO
            dp_display_text("\n\rInvalid entry.Try again.");
#endif
        }
    }
    return;
}

//ML84 added: use SystemTick to count time
volatile uint32_t tick_counter = 0;

__attribute__((__interrupt__)) void SysTick_Handler(void)
{
    tick_counter++;
}

uint32_t SPI_IMAGE_ADDR; //global variable to store base address of IAP image in SPI flash

int main(void)
{
    //uint32_t reg32;
    dbg_var = 0;

    /* Set up the Watch Dog Timer */
    MSS_WD_disable();
//    MSS_WD_init(0xFFFFFFFF, MSS_WDOG_NO_WINDOW, MSS_WDOG_RESET_ON_TIMEOUT_MODE); //MAX timeout for init
//    MSS_WD_reload();

    //Soft reset control register (default 0x0003FFF8)
    //reg32 = *(volatile uint32_t*)(SOFT_RST_CR);
    //Release external reset, enable pin as reset input
    //*(volatile uint32_t*)(SOFT_RST_CR) = ((reg32 & 0xFFFEFFFF) | 0x00080000);
    //reg32 = *(volatile uint32_t*)(SOFT_RST_CR);

    /* Init tick counter (periodical interrupts, used by I2C driver and sleep() function)*/
    NVIC_SetPriority(SysTick_IRQn, 0xFFu); /* Lowest possible priority */
    SysTick_Config(100000000 / 1000000); //every 1 us

    *(volatile uint32_t*)(MBU_MMC_V2B_APB_0) = 0x0; //reset value to avoid loop

    MSS_UART_init(&g_mss_uart0, MSS_UART_57600_BAUD, MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT );

    //get parameters passed by bootloader
    Action_code    = *(volatile uint32_t*)(0x2000EFF8); //(0x20000000+0x10000-8);
    SPI_IMAGE_ADDR = *(volatile uint32_t*)(0x2000EFFC); //(0x20000000+0x10000-4);

    dp_display_text("\n\r*** IAP Programmer v1.10 ***\n\r");

    dp_display_text("Reading SPI image at ");
    dp_display_value(SPI_IMAGE_ADDR,HEX);
    dp_display_text("\n\rPreselected opcode = ");
    dp_display_value(Action_code,DEC);
    dp_display_text("\n\r");



#ifdef NEW_SPI_FLASH_DRIVER
    FLASH_init();
#else
    spi_flash_init();
#endif

    hardware_interface = IAP_SEL;

    while( 1 )
    {
        if (Action_code == 0) {
            action_menu();
        }

        if (Action_code == 23) {
            NVIC_SystemReset();
        }

#ifdef ENABLE_DEBUG
        dp_display_text("\r\nPerforming IAP actions...");
#endif
        enable_IAP();

        dp_top();
//#ifdef ENABLE_INFO
        dp_display_text("\r\nExit code = ");
        dp_display_value(error_code,DEC);
        dp_display_text("\r\n--------------------\r\n\r\n ");
//#endif

        NVIC_SystemReset(); //Microsemi bugfix: direct reset after dp_exit()

        disable_IAP();

        if (Action_code != 0) {
            NVIC_SystemReset();
        }
    }

    return 0;
}

