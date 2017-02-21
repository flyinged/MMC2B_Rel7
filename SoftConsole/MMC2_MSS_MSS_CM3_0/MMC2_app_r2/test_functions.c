/*
 * test_functions.c
 *
 *  Created on: 14.10.2015
 *      Author: malatesta_a
 */

#include "main.h"
#include "string.h"
#include "uart_utility.h"
#include "MMC2_hw_platform.h"

//#include "max31785.h"
#include "ds1682.h" //elapsed time counter
#include "cs5480.h" //energy meter
#include "i2c_eeprom.h"
#include "s25fl128s.h" //spi flash
#include "I2C_auto.h" //fabric i2c master (auto-read)
#include "pca2129.h" //real-time clock
#include "LTC2945.h" //power meter
#include "spi_eeprom.h" //power meter
#include "TMP112.h" //temperature sensor
#include "TC74.h" //temperature sensor
#include "AD5321.h" //temperature sensor

#include "mss_i2c_controller.h"

#define MYDEBUG

extern volatile uint32_t tick_counter;
extern const uint32_t TOUT;

void display_version(const uint8_t *sw_version, uint8_t *fw_version, uint8_t *fw_version_x) {
    //display FW version
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-FW\n\r ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) fw_version);
    //display SW version
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-SW\n\r ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) sw_version);
    //display warning if FW version is not that expected
    if (string_compare(fw_version, fw_version_x)) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) " warning: This Software version has been tested only with FW version ");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) fw_version_x);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
    }
}

void test_status_display(uint8_t user_input) {
    uint32_t rval, rval2;
    uint8_t text_buf[13];

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r#---- STATUS AND CONTROL SIGNALS ----\n\r");

//    rval = HW_get_32bit_reg(I2C_AUTO_0+I2C_AUTO_GPO);
//    memcpy(text_buf, "0x00000000\n\r\0", 13);
//    uint_to_hexstr(rval, text_buf+2, 8);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rI2C outputs ===> ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//    rval = HW_get_32bit_reg(I2C_AUTO_0+I2C_AUTO_GPI);
//    memcpy(text_buf, "0x00000000\n\r\0", 13);
//    uint_to_hexstr(rval, text_buf+2, 8);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rI2C inputs ===> ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);


    rval  = HW_get_32bit_reg(CPU2FABRIC_REG);
    rval2 = HW_get_32bit_reg(CPU2FABRIC_REG2);
    rval = (rval & 0x3FFF) | ((rval2 & 0x3)<<14);
    memcpy(text_buf, "0x0000\n\r\0", 9);
    uint_to_hexstr(rval, text_buf+2, 4);

    if (user_input) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rDATA from CPU to FABRIC (including I2C inputs) ===> ");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "(15:12) force_shutdown    | use_pwr_supply2   | etc_alert_b      | pm_alert_b\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "(11:08) fan_alert_b       | tmp_alert_b       | tmp_ht2_alert_b  | tmp_ht1_alert_b\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "(07:04) pwr_p12v2_ft_b    | pwr_p12v1_ft_b    | pwr_n12v_alert_b | pwr_p12v_alert_b\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "(03:00) pwr_p12v2_alert_b | pwr_p12v1_alert_b | pwr_3v3_alert_b  | pwr_5v0_alert_b\n\r");
    } else {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-STATUS\n\r ");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }

    rval = HW_get_32bit_reg(FABRIC2CPU_REG);
    memcpy(text_buf, "0x000\n\r\0", 8);
    uint_to_hexstr((rval & 0xFFFF), text_buf+2, 3);
    if (user_input) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rFABRIC to I2C GPO ===> ");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "(11:08) tmp_ht_pd_b   | spe_b_out     | master_reset  | fan_fault2  \n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "(07:04) fan_fault_1   | fan_hw_rst2   | fan_hw_rst1   | fp_rem_on_led\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "(03:00) fp_hw_led     | oled_rst_n    | fp_fw_led     | 0        \n\r");
    } else {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-CTRL\n\r ");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }

    memcpy(text_buf, "0x0000\n\r\0", 9);
    uint_to_hexstr(((rval>>16) & 0xFFFF), text_buf+2, 4);
    if (user_input) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rFABRIC REAL-TIME DATA ===> ");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "(15:12) 0           | 0           | v2_sw_off     | v1_sw_off\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "(11:08) v2_rem_en   | v1_rem_en   | hsc5_alert    | 0\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "(07:04) v2_pwr_fail | v1_pwr_fail | v2_pwr_good   | v1_pwr_good\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "(03:00) watchdog    | spe_in      | fan_present_b | fp_on\n\r");
    } else {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-RT\n\r ");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }
}

void test_elapsed_time_counter(uint8_t user_input)
{
    uint8_t text_buf[] = "0x00000000\n\r\0";
    uint32_t rval; //, bit16;

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r#---- ELAPSED TIME COUNTER ----\n\r");

    //eventually implement reset command (requires FW mod)

    /* read & display elapsed time counter */
    rval = ds1682_get_etc(&g_core_i2c_pm, I2C_ETC_SER_ADDR);
    memcpy(text_buf, "0x00000000\n\r\0", 13);
    uint_to_hexstr(rval, text_buf+2, 8);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-ELAPSED_TIME_COUNTER\n\r ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-ELAPSED_TIME_COUNTER\n\r ");

    /* read back & display alarm register */
    rval = ds1682_get_alarm_reg(&g_core_i2c_pm, I2C_ETC_SER_ADDR);
    memcpy(text_buf, "0x00000000\n\r\0", 13);
    uint_to_hexstr(rval, text_buf+2, 8);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-ALARM_COUNTER\n\r ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);

    /* read & display event counter (bit 16 is LSB of Config register) */
    rval = ds1682_get_evc(&g_core_i2c_pm, I2C_ETC_SER_ADDR);
    memcpy(text_buf, "0x00000\n\r\0", 10);
    uint_to_hexstr(rval, text_buf+2, 5);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-EVENT_COUNTER\n\r ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);

}

void test_i2c_eeproms()
{
    uint8_t i, sel, current_i2c_addr, text_buf[11], rx_buf[20], head[]="MBU_xxx-";

    sel = 0;

    while (1)
    {
        if (sel == 0) {
            current_i2c_addr = I2C_EEPROM_SER_ADDR;
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r#--- MMC EEPROM ---\n\r");
            memcpy(head, "MBU_MMC-", 8);
            sel++;
        } else if (sel == 1) {
            current_i2c_addr = I2C_PWR_EE_SER_ADDR;
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r#--- POWER BOARD EEPROM ---\n\r");
            memcpy(head, "MBU_PWR-", 8);
            sel++;
        } else {
            return;
        }

        /* 1: read and display EUI48 identifier */
        i2c_eeprom_read_eui48(&g_mss_i2c0, current_i2c_addr, rx_buf, (const uint8_t *) "Read EUI48 from I2C EEPROM");

        memcpy(text_buf, "0x00, \0", 7);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) head);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "EEP_EUI48\n\r ");
        for (i=0; i<6; i++) {
            uint_to_hexstr(rx_buf[i], text_buf+2, 2);
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
        }
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");

        /* 4: read back data */
        i2c_eeprom_read(&g_mss_i2c0, current_i2c_addr, 0x0, rx_buf, 16, (const uint8_t *) "Readback from I2C EEPROM"); //content of page 0
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) head);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "EEP_PAGE_0\n\r ");
        for (i=0; i<16; i++) {
            uint_to_hexstr(rx_buf[i], text_buf+2, 2);
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
        }
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
    }

}

void test_spi_flash(void)
{
    //uint8_t tx_buf[1],
    uint8_t i, rx_buf[20], text_buf[13];
    uint32_t rval;

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r#--- SPI FLASH ---\n\r");

    memcpy(text_buf, "0x00000000\n\r\0", 13);

    //FLASH_init(); //already configured

    /* read & display Jedec ID */
    rval = FLASH_read_device_id();
    uint_to_hexstr((uint32_t) rval, text_buf+2, 8);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-SPI_FLASH_JEDEC_ID\n\r ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);

    /* read & display Status register */
    rx_buf[0] = FLASH_get_status();
    uint_to_hexstr((uint32_t) rx_buf[0], text_buf+2, 8);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-SPI_FLASH_SR\n\r ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);

//    /* read & display Configuration register */
//    tx_buf[0] = 0x35; //SPIFLASH_CMD_RDCR; //read CR command
//    rx_buf[0] = 0x0; //reset rx register
//    MSS_SPI_transfer_block( &g_mss_spi1, tx_buf, 1, rx_buf, 1 );
//    uint_to_hexstr((uint32_t) rx_buf[0], text_buf+2, 8);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Config Register: ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//    /* read & display BAR */
//    tx_buf[0] = 0x16; //SPIFLASH_CMD_BRRD; //read BAR command
//    rx_buf[0] = 0x0; //reset rx register
//    MSS_SPI_transfer_block( &g_mss_spi1, tx_buf, 1, rx_buf, 1 );
//    uint_to_hexstr((uint32_t) rx_buf[0], text_buf+2, 8);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "BAR Register: ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);

    /* read & display start of first 4k sector */
    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Reading data...\n\r");

    memcpy(text_buf, "0x00, \0", 7);

    FLASH_read(0x0, rx_buf, 16); //13);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-SPI_FLASH_DATA0\n\r ");
    for(i=0; i<16; i++) {
        uint_to_hexstr(rx_buf[i], text_buf+2, 2);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");

#if 0
    FLASH_read(0x100000, rx_buf, 16); //13);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-SPI_FLASH_DATA1\n\r ");
    for(i=0; i<16; i++) {
        uint_to_hexstr(rx_buf[i], text_buf+2, 2);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");

    FLASH_read(0x200000, rx_buf, 16); //13);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-SPI_FLASH_DATA2\n\r ");
    for(i=0; i<16; i++) {
        uint_to_hexstr(rx_buf[i], text_buf+2, 2);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");

    FLASH_read(0x300000, rx_buf, 16); //13);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-SPI_FLASH_DATA3\n\r ");
    for(i=0; i<16; i++) {
        uint_to_hexstr(rx_buf[i], text_buf+2, 2);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
#endif
}


void test_heater(uint8_t user_input)
{
    uint32_t tic, rval;
    uint8_t rx_size, uart_rx_buf[1], text_buf[8];

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r#--- POWER BOARD's HEATER ---\n\r");

    while(user_input) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rHeater control\n\rPress 1 to read current heater value, 2 to write a new value, any other key to exit:\n\r");
        tic = tick_counter;
        do {
            MSS_WD_reload();
            rx_size = MSS_UART_get_rx ( &g_mss_uart0, uart_rx_buf, sizeof(uart_rx_buf) );

            if ((tick_counter-tic) > TOUT) {
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rTIMEOUT while waiting for user input\n\r");
                return;
            }
        } while (rx_size == 0);

        if (uart_rx_buf[0] == '1') {
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rCurrent DAC value: ");
            memcpy(text_buf, "0x000\n\r\0",8);
            rval = AD5321_get_level(&g_core_i2c_tmp, I2C_TMP_DAC_SER_ADDR);
            uint_to_hexstr(rval, text_buf+2, 3);
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
        } else if (uart_rx_buf[0] == '2') {
            //get value from user
            rval = hex_from_console("Input 12 bit HEX value: ",4);
            AD5321_set_level(&g_core_i2c_tmp, I2C_TMP_DAC_SER_ADDR, rval);
        } else {
            break;
        }
    }

    if (user_input == 0) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_PWR-HEATER_DAC\n\r ");
        memcpy(text_buf, "0x000\n\r\0",8);
        rval = AD5321_get_level(&g_core_i2c_tmp, I2C_TMP_DAC_SER_ADDR);
        uint_to_hexstr(rval, text_buf+2, 3);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }
}

void test_realtime_clock(uint8_t user_input)
{
    uint8_t uart_rx_buf[1], rx_size, wd;
    uint8_t text_buf[20];
    uint32_t tic; //, rval;

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r#--- Real-Time Clock ---\n\r");

    /* read date */
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-DATE\n\r ");
    if (user_input) {
        pca2129_get_date(&g_core_i2c_pm, I2C_RTC_SER_ADDR, 2);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) ";(WD) DD/MM/YY\n\r");
    } else {
        pca2129_get_date(&g_core_i2c_pm, I2C_RTC_SER_ADDR, 1);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) ";DD/MM/YY\n\r");
    }

    /* read time */
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-TIME\n\r ");
    pca2129_get_time(&g_core_i2c_pm, I2C_RTC_SER_ADDR, 1);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) ";HH:MM:SS\n\r");


    if (user_input) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rPress 'S' to set the clock, any other key to continue > ");

        tic = tick_counter;
        do {
            MSS_WD_reload();
            rx_size = MSS_UART_get_rx ( &g_mss_uart0, uart_rx_buf, sizeof(uart_rx_buf) );

            if ((tick_counter-tic) > TOUT) {
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rTIMEOUT while waiting for user input\n\r");
                return;
            }
        } while (rx_size == 0);

        if (uart_rx_buf[0] == 'S') {
            /* get date */
            wd = (uint8_t) hex_from_console("\n\rEnter weekday as number (0=Sun, 1=Mon ... 6=Sat): ",1);
            str_from_console((uint8_t*)"Enter date in the format dd/mm/yy > ", text_buf);
            text_buf[8] = '\0';
            /* get time */
            str_from_console((uint8_t*)"Enter hour in the format hh:mm:ss > ", text_buf+9);
            text_buf[17] = '\0';
            //wd= weekday, sunday=0, hour[]="00:00:00", date[]="00/00/00"
            pca2129_set_clock(&g_core_i2c_pm, I2C_RTC_SER_ADDR, wd, (char*)text_buf+9, (char*)text_buf);
        }
    }
}

void test_power_meters(uint8_t user_input)
{

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r#--- POWER METERS ---\n\r");

    if (user_input) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_PWR-PM_ORING1\n\r ");
        LTC2945_display(&g_core_i2c_pwr, I2C_PMON_O1_SER_ADDR, 0, 3, 11);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_PWR-PM_ORING2\n\r ");
        LTC2945_display(&g_core_i2c_pwr, I2C_PMON_O2_SER_ADDR, 0, 3, 11);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_PWR-PM_3V3\n\r ");
        LTC2945_display(&g_core_i2c_pwr, I2C_PMON_3V3_SER_ADDR, 0, 3, 2);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_PWR-PM_5V0\n\r ");
        LTC2945_display(&g_core_i2c_pwr, I2C_PMON_5V0_SER_ADDR, 0, 1, (147.0/47.0));
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_PWR-PM_P12V\n\r ");
        LTC2945_display(&g_core_i2c_pwr, I2C_PMON_P12_SER_ADDR, 0, 3, 11);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_PWR-PM_N12V\n\r ");
        LTC2945_display(&g_core_i2c_pwr, I2C_PMON_N12_SER_ADDR, 0, 10, -11);
    } else {
        LTC2945_report(&g_core_i2c_pwr, I2C_PMON_O1_SER_ADDR,  3,  (const uint8_t *) "MBU_PWR-PM_ORING1-",11);
        LTC2945_report(&g_core_i2c_pwr, I2C_PMON_O2_SER_ADDR,  3,  (const uint8_t *) "MBU_PWR-PM_ORING2-",11);
        LTC2945_report(&g_core_i2c_pwr, I2C_PMON_3V3_SER_ADDR, 3,  (const uint8_t *) "MBU_PWR-PM_3V3-",2);
        LTC2945_report(&g_core_i2c_pwr, I2C_PMON_5V0_SER_ADDR, 1,  (const uint8_t *) "MBU_PWR-PM_5V0-",(147.0/47.0));
        LTC2945_report(&g_core_i2c_pwr, I2C_PMON_P12_SER_ADDR, 3,  (const uint8_t *) "MBU_PWR-PM_P12V-",11);
        LTC2945_report(&g_core_i2c_pwr, I2C_PMON_N12_SER_ADDR, 10, (const uint8_t *) "MBU_PWR-PM_N12V-",-11);
    }


//          //reset all MIN/MAX values
//          LTC2945_reset_all_limits(&g_core_i2c_pwr, I2C_PMON_O1_SER_ADDR);
//          LTC2945_reset_all_limits(&g_core_i2c_pwr, I2C_PMON_O2_SER_ADDR);
//          LTC2945_reset_all_limits(&g_core_i2c_pwr, I2C_PMON_3V3_SER_ADDR);
//          LTC2945_reset_all_limits(&g_core_i2c_pwr, I2C_PMON_5V0_SER_ADDR);
//          LTC2945_reset_all_limits(&g_core_i2c_pwr, I2C_PMON_P12_SER_ADDR);
//          LTC2945_reset_all_limits(&g_core_i2c_pwr, I2C_PMON_N12_SER_ADDR);
}

extern cs5480_t power_meter;
extern uint32_t pdm_v1_gain, pdm_v2_gain;

void test_energy_meter(uint8_t user_input)
{
    uint32_t rval;
    uint8_t text_buf[13];

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r#--- PDM ENERGY METER ---\n\r");

//    //reset
//    cs5480_instruction(CS5480_I_SW_RST);
//    if (cs5480_wait_drdy(&power_meter)) {
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) " error: Power Meter not responding\n\r");
//        return;
//    }
//
//    //initialize parameters
//    if (cs5480_set_parameters(&power_meter, pdm_v1_gain, pdm_v2_gain)) {
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) " error: Cannot configure power meter\n\r");
//        return;
//    }
//
//    //start continuous conversion
//    cs5480_instruction(CS5480_I_CONT);
//    cs5480_wait_drdy(&power_meter);

    if (user_input==0) { //report mode
        memcpy(text_buf, "+000.000;A\n\r\0", sizeof("+000.000;A\n\r\0"));
        cs5480_select_page(16);
        /* I1 RMS */
        rval = cs5480_reg_read(CS5480_I1RMS);
        cs5480_meas_to_str(rval, text_buf, 1, 0);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_I1_RMS\n\r " );
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* V1 RMS */
        rval = cs5480_reg_read(CS5480_V1RMS);
        cs5480_meas_to_str(rval, text_buf, 0, 0);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_V1_RMS\n\r " );
        text_buf[9] = 'V';
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* P1 AVG */
        rval = cs5480_reg_read(CS5480_P1AVG);
        cs5480_meas_to_str(rval, text_buf, 3, 1);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_P1_AVG\n\r " );
        text_buf[9] = 'W';
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* I2 RMS */
        rval = cs5480_reg_read(CS5480_I2RMS);
        cs5480_meas_to_str(rval, text_buf, 2, 0);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_I2_RMS\n\r " );
        text_buf[9] = 'A';
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* V2 RMS */
        rval = cs5480_reg_read(CS5480_V2RMS);
        cs5480_meas_to_str(rval, text_buf, 0, 0);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_V2_RMS\n\r " );
        text_buf[9] = 'V';
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* P2 AVG */
        rval = cs5480_reg_read(CS5480_P2AVG);
        cs5480_meas_to_str(rval, text_buf, 4, 1);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_P2_AVG\n\r " );
        text_buf[9] = 'W';
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        //stop conversion
//        cs5480_instruction(CS5480_I_HALT);
//        cs5480_wait_drdy(&power_meter);
        return;
    } else {
        memcpy(text_buf, "0x00000000\n\r\0", sizeof("0x00000000\n\r\0"));
        cs5480_select_page(16);
        /* I1 RMS */
        rval = cs5480_reg_read(CS5480_I1RMS);
        uint_to_hexstr(rval, text_buf+2, 8);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_I1_RMS\n\r " );
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* V1 RMS */
        rval = cs5480_reg_read(CS5480_V1RMS);
        uint_to_hexstr(rval, text_buf+2, 8);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_V1_RMS\n\r " );
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* P1 AVG */
        rval = cs5480_reg_read(CS5480_P1AVG);
        uint_to_hexstr(rval, text_buf+2, 8);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_P1_AVG\n\r " );
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* Q1 AVG */
        rval = cs5480_reg_read(CS5480_Q1AVG);
        uint_to_hexstr(rval, text_buf+2, 8);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_Q1_AVG\n\r " );
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* Q1 AVG */
        rval = cs5480_reg_read(CS5480_PF1);
        uint_to_hexstr(rval, text_buf+2, 8);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_PF1_AVG\n\r " );
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* I2 RMS */
        rval = cs5480_reg_read(CS5480_I2RMS);
        uint_to_hexstr(rval, text_buf+2, 8);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_I2_RMS\n\r " );
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* V2 RMS */
        rval = cs5480_reg_read(CS5480_V2RMS);
        uint_to_hexstr(rval, text_buf+2, 8);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_V2_RMS\n\r " );
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* P2 AVG */
        rval = cs5480_reg_read(CS5480_P2AVG);
        uint_to_hexstr(rval, text_buf+2, 8);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_P2_AVG\n\r " );
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* Q1 AVG */
        rval = cs5480_reg_read(CS5480_Q2AVG);
        uint_to_hexstr(rval, text_buf+2, 8);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_Q2_AVG\n\r " );
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* PF2 AVG */
        rval = cs5480_reg_read(CS5480_PF2);
        uint_to_hexstr(rval, text_buf+2, 8);
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "MBU_PWR-PDU_PF2_AVG\n\r " );
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//        //stop conversion
//        cs5480_instruction(CS5480_I_HALT);
//        cs5480_wait_drdy(&power_meter);
        return;
    }
#if 0
    memcpy(text_buf, "+000.000, \0", sizeof("+000.000, \0"));

    //if (user_input) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rPress X to exit, P to pause, any other key to resume\n\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r+++ CONVERSION STARTED +++\n\n\r");
    //}

    //read & display measures (1 per second when usr_input)    +000.000, +000.000, +000.000,
    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"I1,       V1,       P1,     ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"I1 RMS,   V1 RMS,   P1 AVG,   ");
    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"I2,       V2,       P2,     ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"I2 RMS,   V2 RMS,   P2 AVG    ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"\n\r");

    i=1;
    tic = tick_counter;
    while (1) {


        if (i==0) continue;

        cs5480_select_page(16);

        //memcpy(text_buf, "+000.000, \0", sizeof("+000.000, \0"));
#if 0
        /* I1 */
        rval = cs5480_reg_read(CS5480_I1);
        i = cs5480_meas_to_str(rval, text_buf, 1, 1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* V1 */
        rval = cs5480_reg_read(CS5480_V1);
        cs5480_meas_to_str(rval, text_buf, 0, 1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* P1 */
        rval = cs5480_reg_read(CS5480_P1);
        cs5480_meas_to_str(rval, text_buf, 3, 1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
#endif
        /* I1 RMS */
        rval = cs5480_reg_read(CS5480_I1RMS);
        cs5480_meas_to_str(rval, text_buf, 1, 0);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* V1 RMS */
        rval = cs5480_reg_read(CS5480_V1RMS);
        cs5480_meas_to_str(rval, text_buf, 0, 0);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* P1 AVG */
        rval = cs5480_reg_read(CS5480_P1AVG);
        cs5480_meas_to_str(rval, text_buf, 3, 1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /********************************************************/

#if 0
        /* I2 */
        rval = cs5480_reg_read(CS5480_I2);
        cs5480_meas_to_str(rval, text_buf, 2, 1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* V2 */
        rval = cs5480_reg_read(CS5480_V2);
        cs5480_meas_to_str(rval, text_buf, 0, 1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* P2 */
        rval = cs5480_reg_read(CS5480_P2);
        cs5480_meas_to_str(rval, text_buf, 4, 1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
#endif

        /* I2 RMS */
        rval = cs5480_reg_read(CS5480_I2RMS);
        cs5480_meas_to_str(rval, text_buf, 2, 0);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* V2 RMS */
        rval = cs5480_reg_read(CS5480_V2RMS);
        cs5480_meas_to_str(rval, text_buf, 0, 0);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* P2 AVG */
        rval = cs5480_reg_read(CS5480_P2AVG);
        cs5480_meas_to_str(rval, text_buf, 4, 1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );


        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"\n\r" );

        //if (user_input == 0) break;

        //reload WD and handle user-input timeout
        MSS_WD_reload();
        if ((tick_counter-tic) > TOUT) {
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rTIMEOUT while waiting for user input\n\r");
            return;
        }

        //poll user input
        rx_size = MSS_UART_get_rx ( &g_mss_uart0, uart_rx_buf, 1 );
        if (rx_size) {
            if (uart_rx_buf[0] == 'X') {
                break;
            } else if (i) { //pause
                i=0;
            } else { //resume
                i=1;
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"\n\n\r" );
            }
        }

        sleep(1000); //pause 1 sec
    }

    //stop conversion
    cs5480_instruction(CS5480_I_HALT);
    cs5480_wait_drdy(&power_meter);
    //if (user_input) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r--- CONVERSION STOPPED ---\n\r");
    //}
#endif
}

void test_power_monitoring()
{
    cs5480_t power_meter;
    uint32_t i, tic; //, rval;
    uint8_t rx_size, uart_rx_buf[1], run, text_buf[80];
    float fval;


    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r********** POWER MONITORING TEST **********\n\r");

    // RESET HEATER
    AD5321_set_level(&g_core_i2c_tmp, I2C_TMP_DAC_SER_ADDR, 0);

//    //Setup energy meter
//    cs5480_instruction(CS5480_I_SW_RST); //reset
//    cs5480_wait_drdy(&power_meter);
//    cs5480_set_parameters(&power_meter, v1_gain, v2_gain); //initialize parameters

    //Setup power meters
    //ctrl register:
    //  bit3 => Vin = Sense+, not Vdd
    //  bit0 => Use Vin, not ADIN for power calculation
    LTC2945_write_reg(&g_core_i2c_pwr, I2C_PMON_O1_SER_ADDR,  LTC2945_CTRL, 0x5, 1);
    LTC2945_write_reg(&g_core_i2c_pwr, I2C_PMON_O2_SER_ADDR,  LTC2945_CTRL, 0x5, 1);
    LTC2945_write_reg(&g_core_i2c_pwr, I2C_PMON_3V3_SER_ADDR, LTC2945_CTRL, 0x5, 1);
    LTC2945_write_reg(&g_core_i2c_pwr, I2C_PMON_5V0_SER_ADDR, LTC2945_CTRL, 0x5, 1);
    LTC2945_write_reg(&g_core_i2c_pwr, I2C_PMON_P12_SER_ADDR, LTC2945_CTRL, 0x5, 1);
    LTC2945_write_reg(&g_core_i2c_pwr, I2C_PMON_N12_SER_ADDR, LTC2945_CTRL, 0x5, 1);

    //start continuous conversion on energy meter
    cs5480_instruction(CS5480_I_CONT);
    cs5480_wait_drdy(&power_meter);
    cs5480_select_page(16); //pre-select page

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rPress X to exit, any other key to pause/resume\n\n\r");

    //read & display measures (1 per second)                   +0.000, +000.0,
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"NRG METER,,     ");
    //                                                         +00.00, +00.00, +00.00, +000.0,
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"ORING1,,,,                      ");
    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"ORING2 --------------------------, ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"3.3V,,,,                        ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"5.0V,,,,                        ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"+12V,,,,                        ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"-12V,,,,                        ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"\n\r");

    //                                                         +0.000, +000.0,
    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"I1 RMS, V1 RMS, P1 AVG, "); //don't display voltage
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"I1 RMS, P1 AVG, ");

    //                                                         +00.00, +00.00, +00.00, +000.0,
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"dSENSE, SENSE+, ADIN,   PWR,    ");
    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"dSENSE, SENSE+, ADIN,   PWR,   ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"dSENSE, SENSE+, ADIN,   PWR,    ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"dSENSE, SENSE+, ADIN,   PWR,    ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"dSENSE, SENSE+, ADIN,   PWR,    ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"dSENSE, SENSE+, ADIN,   PWR,    ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"\n\r");


    run=1; //run-pause# flag
    i=0;
    tic = tick_counter;
    while (1) {
        MSS_WD_reload();
        if ((tick_counter-tic) > TOUT) {
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rTIMEOUT while waiting for user input\n\r");
            return;
        }

        rx_size = MSS_UART_get_rx ( &g_mss_uart0, uart_rx_buf, 1 );
        if (rx_size) {
            if (uart_rx_buf[0] == 'X') {
                break;
            } else if (run) { //pause
                run=0;
            } else { //resume
                run=1;
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"\n\n\r" );
            }
        } else if(i==8) { //auto pause after 8 samples
            i=0;
            run=0;
            //                    if (reg16 == -1) {
            //                        break;
            //                    } else if (reg16<=0xFFF) {
            //                        reg16 += 41;
            //                        AD5321_set_level(&g_core_i2c_tmp, I2C_TMP_DAC_SER_ADDR, (uint16_t) reg16);
            //                    } else if (reg16>0xFFF) {
            //                        reg16 = -1;
            //                        AD5321_set_level(&g_core_i2c_tmp, I2C_TMP_DAC_SER_ADDR, 0x0);
            //                    }
            //                    memcpy(text_buf, "0000, \0", sizeof("0000, \0"));
            //                    uint_to_decstr(reg16, text_buf, 4);
            //                    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
            //                    sleep(10000);
        }

        if (run==0) continue;

        //first display nrg meter data
        memcpy(text_buf, "+0.000, \0", sizeof("+0.000, \0")); //current
        /* I1 RMS */
        fval = cs5480_meas_to_float(cs5480_reg_read(CS5480_I1RMS), 1, 0);
        float_to_string(fval, text_buf, 1,3);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        //                /* V1 RMS */
        //                memcpy(text_buf, "+00.00, \0", sizeof("+00.00, \0"));
        //                fval = cs5480_meas_to_float(cs5480_reg_read(CS5480_V1RMS), 0, 0);
        //                float_to_string(fval, text_buf, 2,2);
        //                MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
        /* P1 AVG */
        memcpy(text_buf, "+000.0, \0", sizeof("+000.0, \0")); //power
        fval = cs5480_meas_to_float(cs5480_reg_read(CS5480_P1AVG), 3, 1);
        float_to_string(fval, text_buf, 3,1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

        //then display power meters' data

        //                0       8      16      24
        memcpy(text_buf, "+00.00, +00.00, +00.00, +000.0, \0", sizeof("+00.00, +00.00, +00.00, +000.0, \0"));

        //ORING1
        fval = LTC2945_raw_to_voltage(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O1_SER_ADDR, LTC2945_SENSE_MSB, 2));
        float_to_string(fval, text_buf+0, 2,2);
        fval = LTC2945_raw_to_voltage(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O1_SER_ADDR, LTC2945_VIN_MSB, 2));
        float_to_string(fval, text_buf+8, 2,2);
        fval = LTC2945_raw_to_adin(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O1_SER_ADDR, LTC2945_ADIN_MSB, 2),11);
        float_to_string(fval, text_buf+16, 2,2);
        fval = LTC2945_raw_to_power(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O1_SER_ADDR, LTC2945_PWR_MSB2, 3), 3); //3 mOhm shunt resistor
        float_to_string(fval, text_buf+24, 3,1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
        //                //ORING2
        //                fval = LTC2945_raw_to_voltage(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O2_SER_ADDR, LTC2945_SENSE_MSB, 2));
        //                float_to_string(fval, text_buf+0, 2,2);
        //                fval = LTC2945_raw_to_voltage(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O2_SER_ADDR, LTC2945_VIN_MSB, 2));
        //                float_to_string(fval, text_buf+8, 2,2);
        //                fval = LTC2945_raw_to_adin(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O2_SER_ADDR, LTC2945_ADIN_MSB, 2));
        //                float_to_string(fval, text_buf+16, 2,2);
        //                fval = LTC2945_raw_to_power(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O2_SER_ADDR, LTC2945_PWR_MSB2, 3));
        //                float_to_string(fval, text_buf+24, 2,1);
        //                MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
        //3v3
        fval = LTC2945_raw_to_voltage(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_3V3_SER_ADDR, LTC2945_SENSE_MSB, 2));
        float_to_string(fval, text_buf+0, 2,2);
        fval = LTC2945_raw_to_voltage(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_3V3_SER_ADDR, LTC2945_VIN_MSB, 2));
        float_to_string(fval, text_buf+8, 2,2);
        fval = LTC2945_raw_to_adin(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_3V3_SER_ADDR, LTC2945_ADIN_MSB, 2),2);
        float_to_string(fval, text_buf+16, 2,2);
        fval = LTC2945_raw_to_power(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_3V3_SER_ADDR, LTC2945_PWR_MSB2, 3), 3); //3 mOhm shunt resistor
        float_to_string(fval, text_buf+24, 3,1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
        //5v0
        fval = LTC2945_raw_to_voltage(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_5V0_SER_ADDR, LTC2945_SENSE_MSB, 2));
        float_to_string(fval, text_buf+0, 2,2);
        fval = LTC2945_raw_to_voltage(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_5V0_SER_ADDR, LTC2945_VIN_MSB, 2));
        float_to_string(fval, text_buf+8, 2,2);
        fval = LTC2945_raw_to_adin(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_5V0_SER_ADDR, LTC2945_ADIN_MSB, 2),(147/47));
        float_to_string(fval, text_buf+16, 2,2);
        fval = LTC2945_raw_to_power(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_5V0_SER_ADDR, LTC2945_PWR_MSB2, 1), 1); //1 mOhm shunt resistor
        float_to_string(fval, text_buf+24, 3,1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
        //p12v
        fval = LTC2945_raw_to_voltage(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_P12_SER_ADDR, LTC2945_SENSE_MSB, 2));
        float_to_string(fval, text_buf+0, 2,2);
        fval = LTC2945_raw_to_voltage(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_P12_SER_ADDR, LTC2945_VIN_MSB, 2));
        float_to_string(fval, text_buf+8, 2,2);
        fval = LTC2945_raw_to_adin(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_P12_SER_ADDR, LTC2945_ADIN_MSB, 2),11);
        float_to_string(fval, text_buf+16, 2,2);
        fval = LTC2945_raw_to_power(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_P12_SER_ADDR, LTC2945_PWR_MSB2, 3), 3); //3 mOhm shunt resistor
        float_to_string(fval, text_buf+24, 3,1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
        //n12v
        fval = LTC2945_raw_to_voltage(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_N12_SER_ADDR, LTC2945_SENSE_MSB, 2));
        float_to_string(fval, text_buf+0, 2,2);
        fval = LTC2945_raw_to_voltage(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_N12_SER_ADDR, LTC2945_VIN_MSB, 2));
        float_to_string(fval, text_buf+8, 2,2);
        fval = LTC2945_raw_to_adin(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_N12_SER_ADDR, LTC2945_ADIN_MSB, 2),-12);
        float_to_string(fval, text_buf+16, 2,2);
        fval = LTC2945_raw_to_power(LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_N12_SER_ADDR, LTC2945_PWR_MSB2, 3), 10); //10 mOhm shunt resistor
        float_to_string(fval, text_buf+24, 3,1);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *)"\n\r" );
        sleep(1000);
        i++;
    }

    //stop conversion
    cs5480_instruction(CS5480_I_HALT);
    cs5480_wait_drdy(&power_meter);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r---------- CONVERSION STOPPED ----------\n\r");
}

void test_spi_eeprom(uint8_t user_input)
{
    uint8_t text_buf[16], tx_buf[2], rx_buf[20], i;
    //uint32_t rval;

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r#--- SPI EEPROM TEST ---\n\r");

    memcpy(text_buf, "0x00000000\n\r\0",13);

    MSS_SPI_clear_slave_select( &g_mss_spi1, MSS_SPI_SLAVE_0 ); //just in case
    MSS_SPI_set_slave_select( &g_mss_spi1, MSS_SPI_SLAVE_1 ); //SPI EEPROM


    if (user_input) {
        /* read & display status register */
        tx_buf[0] = SPIEE_CMD_RDST; //read status register command
        rx_buf[0] = 0x0; //reset rx register
        MSS_SPI_transfer_block( &g_mss_spi1, tx_buf, 1, rx_buf, 1 );
        uint_to_hexstr((uint32_t) rx_buf[0], text_buf+2, 8);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Status register: ");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }

    /* read & display EUI48 */
    tx_buf[0] = SPIEE_CMD_READ; //read data command
    tx_buf[1] = SPIEE_EUI48_ADR; //read address
    for (i=0; i<6; i++) rx_buf[i] = 0x0; //reset rx buffer (6 bytes for EUI48)
    MSS_SPI_transfer_block( &g_mss_spi1, tx_buf, 2, rx_buf, 6 );

    memcpy(text_buf, "0x00, \0", 7);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-SPI_EEP_EUI48\n\r ");
    for (i=0; i<6; i++) {
        uint_to_hexstr(rx_buf[i], text_buf+2, 2);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");

    //rval = (rx_buf[0]<<16) | (rx_buf[1]<<8) | rx_buf[2];
    //uint_to_hexstr( rval, text_buf+2, 8 );
    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "EUI48:OUI(default:0x001Ec0): ");
    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    //rval = (rx_buf[3]<<16) | (rx_buf[4]<<8) | rx_buf[5];
    //uint_to_hexstr( rval, text_buf+2, 8 );
    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "EUI48:EI(chip specific)    : ");
    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);

#ifdef WRITE_NVMEM
    /* write pattern */
    tx_buf[0] = SPIEE_CMD_WREN; //write enable
    MSS_SPI_transfer_block( &g_mss_spi1, tx_buf, 1, rx_buf, 0 ); //send write enable
    tx_buf[0] = SPIEE_CMD_WRITE; //write data
    tx_buf[1] = 0x10; //address (test page2)
    for (i=0; i<12; i++) {
        tx_buf[i+2] = msg[i];
    }
    MSS_SPI_transfer_block( &g_mss_spi1, tx_buf, 14, rx_buf, 0 ); //send data

    /* poll status register */
    //MSS_SPI_set_slave_select( &g_mss_spi1, MSS_SPI_SLAVE_1 ); //SPI EEPROM
    tx_buf[0] = SPIEE_CMD_RDST; //read status register command
    rx_buf[0] = SPIEE_SR_WIP;
    while (rx_buf[0] == SPIEE_SR_WIP) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Waiting for write to complete...\n\r");
        MSS_SPI_transfer_block( &g_mss_spi1, tx_buf, 1, rx_buf, 1 );
    }
#endif

    /* readback */
    tx_buf[0] = SPIEE_CMD_READ; //read data
    tx_buf[1] = 0x0; //read address
    for (i=0; i<16; i++) {
        rx_buf[i] = 0x0;
    }
    MSS_SPI_transfer_block( &g_mss_spi1, tx_buf, 2, rx_buf, 16 ); //read data

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-SPI_EEP_PAGE0\n\r ");
    memcpy(text_buf, "0x00, \0", 7);
    for(i=0; i<16; i++) {
        uint_to_hexstr(rx_buf[i], text_buf+2, 2);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");

    MSS_SPI_clear_slave_select( &g_mss_spi1, MSS_SPI_SLAVE_1 ); //SPI EEPROM
}

void test_temperatures(uint8_t user_input)
{
    uint32_t tic;
    uint8_t i, text_buf[16], read_mmc_temp;
    int8_t T_hs;
    int16_t T_fo, T_ro, T_il, T_pb, T_mmc;

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r#--- TEMPERATURES ---\n\r");


    if (HW_get_32bit_reg(FABRIC2CPU_REG) & F2C_REG_SPE_I) { //GPAC PRESENT and CRATE ON
        read_mmc_temp = 0;
    } else {
        read_mmc_temp = 1;
    }


    if (user_input) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rpress any key to stop\n\n\r");
    } else {
        //read temperatures
        T_fo = T112_get_temp(&g_core_i2c_tmp, 0, I2C_TMP_FO_SER_ADDR, 0, (uint8_t*)"Get FO temp");
        T_ro = T112_get_temp(&g_core_i2c_tmp, 0, I2C_TMP_RO_SER_ADDR, 0, (uint8_t*)"Get RO temp");
        T_il = T112_get_temp(&g_core_i2c_tmp, 0, I2C_TMP_IL_SER_ADDR, 0, (uint8_t*)"Get IL temp");
        T_pb = T112_get_temp(&g_core_i2c_tmp, 0, I2C_TMP_PB_SER_ADDR, 0, (uint8_t*)"Get PB temp");
        T_hs = TC74_get_temp(&g_core_i2c_tmp, I2C_TMP_HS_SER_ADDR);
        if (read_mmc_temp) {
            T_mmc = T112_get_temp(&g_mss_i2c0, 1, I2C_TMP_SER_ADDR, 0, (uint8_t*)"Get MMC temp");
        }

        //display
        memcpy(text_buf, "+000.000;degC\n\r\0", 16);

        if (read_mmc_temp) {
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_MMC-TMP_BOARD\n\r ");
            T112_meas_to_str(T_mmc, text_buf);
            MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
        }

        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_PWR-TMP_BOARD\n\r ");
        T112_meas_to_str(T_pb, text_buf);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_PWR-TMP_INLET\n\r ");
        T112_meas_to_str(T_il, text_buf);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_PWR-TMP_FRONT_OUTLET\n\r ");
        T112_meas_to_str(T_fo, text_buf);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_PWR-TMP_REAR_OUTLET\n\r ");
        T112_meas_to_str(T_ro, text_buf);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

        memcpy(text_buf, "+000;degC\n\r\0", sizeof("+000;degC\n\r\0"));
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_PWR-TMP_HEATSINK\n\r ");
        TC74_meas_to_str(T_hs, text_buf);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
        return;
    }

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Power board, Air inlet, Front outlet, Rear outlet, Heatsink\n\r");
    //                                                          +000.000...,.+000.000.,.+000.000....,.+000.000...,.000

    tic = tick_counter;
    while (1) {
        MSS_WD_reload();
        if ((tick_counter-tic) > TOUT) {
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rTIMEOUT while waiting for user input\n\r");
            return;
        }
        //read temperatures
        T_fo = T112_get_temp(&g_core_i2c_tmp, 0, I2C_TMP_FO_SER_ADDR, 0, (uint8_t*)"Get FO temp");
        T_ro = T112_get_temp(&g_core_i2c_tmp, 0, I2C_TMP_RO_SER_ADDR, 0, (uint8_t*)"Get RO temp");
        T_il = T112_get_temp(&g_core_i2c_tmp, 0, I2C_TMP_IL_SER_ADDR, 0, (uint8_t*)"Get IL temp");
        T_pb = T112_get_temp(&g_core_i2c_tmp, 0, I2C_TMP_PB_SER_ADDR, 0, (uint8_t*)"Get PB temp");
        T_hs = TC74_get_temp(&g_core_i2c_tmp, I2C_TMP_HS_SER_ADDR);

        //display
        memcpy(text_buf, "+000.000   , \0", 14);
        i = T112_meas_to_str(T_pb, text_buf);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

        memcpy(text_buf, "+000.000 , \0", 12);
        i = T112_meas_to_str(T_il, text_buf);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

        memcpy(text_buf, "+000.000    , \0", 15);
        i = T112_meas_to_str(T_fo, text_buf);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

        memcpy(text_buf, "+000.000   , \0", 14);
        i = T112_meas_to_str(T_ro, text_buf);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

        memcpy(text_buf, "+000.000\n\r\0", 14);
        i = TC74_meas_to_str(T_hs, text_buf);
        MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

        if (MSS_UART_get_rx ( &g_mss_uart0, &i, 1 ) || (user_input == 0)) break;
        sleep(2000);
    }
}

//print name[], then read address and print data, if present print unit
void test_display(uint32_t addr, uint8_t *name, uint8_t *unit) {
    uint32_t ureg32;
    uint8_t text_buf[] = "0x00000000\0";

    MSS_UART_polled_tx_string( &g_mss_uart0, name );
    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)"\n\r " );
    ureg32 = HW_get_32bit_reg(addr);
    uint_to_hexstr(ureg32, text_buf+2, 8);
    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    if (unit) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)";");
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)unit);
    }
    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)"\n\r");
}

void test_fan_read(uint8_t user_input)
{
    uint8_t text_buf[16];
    uint32_t base, ureg32;

    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)"\n\r#--- FAN CONTROLLERS ---\n\r");

    //Fan controllers are powered only when fp_on = 1 and at least one power supply is on
    ureg32 = HW_get_32bit_reg(FABRIC2CPU_REG);
    if ((ureg32 & 0x00010000) == 0) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)" warning: Front power switch is off. Cannot access fan controllers.\n\r");
        return;
    } else if ((ureg32 & 0x00300000) == 0) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)" warning: Both power supplies are off. Cannot access fan controllers.\n\r");
        return;
    }

    //          //check reset bit
    //          ureg32 = HW_get_32bit_reg(0xE0042030); //reset
    //          if (ureg32 & 0x00001000) { //mss_i2c1 under reset
    //              MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)"\n\n\rMSS_I2C_1 was under reset: releasing reset\n\r");
    //              HW_set_32bit_reg(0xE0042030, (ureg32 & 0xFFFF7FFF)); //release reset bit
    //          }

    base = MSS_I2C_CONTROLLER_0; //base address

    //            MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)"\n\n\rI2C ST, I2C CTRL, FSM            : ");
    //          ureg32 = HW_get_32bit_reg(base); //0
    //          uint_to_hexstr(ureg32, text_buf+2, 8);
    //          MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    //
    //          MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\rLast I2C states                  : ");
    //          ureg32 = HW_get_32bit_reg(base+4); //4
    //          uint_to_hexstr(ureg32, text_buf+2, 8);
    //          MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    //
    //          MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\rI2C error count                  : ");
    //          ureg32 = HW_get_32bit_reg(base+8); //8
    //          uint_to_hexstr(ureg32, text_buf+2, 8);
    //          MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    //
    //          MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\rM_BUSY, M_ERR, TS | M_DAT | M_ADR: ");
    //          ureg32 = HW_get_32bit_reg(base+12); //C
    //          uint_to_hexstr(ureg32, text_buf+2, 8);
    //          MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

    if (user_input == 0) {
        test_display(base+FC1_ST,       (uint8_t*)"MBU_FAN1-STATUS", NULL);
        test_display(base+FC1_MODE,     (uint8_t*)"MBU_FAN1-MFR_MODE", NULL);
        test_display(base+FC1_TIME,     (uint8_t*)"MBU_FAN1-LIFETIME", (uint8_t*)"days");
        //new: MSS_I2C_CTRL_OP(ADDR,N,REG,RNW)
        HW_set_32bit_reg(base+MSS_I2C_CTRL_CMD, MSS_I2C_CTRL_OP(0x52,1,0x0,0));  //write page command (write 1byte, value 0, register 0)
        HW_set_32bit_reg(base+MSS_I2C_CTRL_CMD, 0x0000000C);                     //write page data (0xC=internal temp)
        HW_set_32bit_reg(base+MSS_I2C_CTRL_CMD, MSS_I2C_CTRL_OP(0x52,2,0x8D,1)); //read temp command (read 2 bytes from register 0x8D)
        sleep(500);
        test_display(base+MSS_I2C_CTRL_DAT, (uint8_t*)"MBU_FAN1-TEMP", (uint8_t*)"cdegC"); //read register containing requested value

        test_display(base+FC1_1_CFG12,  (uint8_t*)"MBU_FAN1-CFG12_1", NULL);
        test_display(base+FC1_2_CFG12,  (uint8_t*)"MBU_FAN1-CFG12_2", NULL);
        test_display(base+FC1_3_CFG12,  (uint8_t*)"MBU_FAN1-CFG12_3", NULL);
        test_display(base+FC1_4_CFG12,  (uint8_t*)"MBU_FAN1-CFG12_4", NULL);
        test_display(base+FC1_1_CFGMFR, (uint8_t*)"MBU_FAN1-MFRCFG_1", NULL);
        test_display(base+FC1_2_CFGMFR, (uint8_t*)"MBU_FAN1-MFRCFG_2", NULL);
        test_display(base+FC1_3_CFGMFR, (uint8_t*)"MBU_FAN1-MFRCFG_3", NULL);
        test_display(base+FC1_4_CFGMFR, (uint8_t*)"MBU_FAN1-MFRCFG_4", NULL);
        test_display(base+FC1_1_FRESP,  (uint8_t*)"MBU_FAN1-FRESP_1", NULL);
        test_display(base+FC1_2_FRESP,  (uint8_t*)"MBU_FAN1-FRESP_2", NULL);
        test_display(base+FC1_3_FRESP,  (uint8_t*)"MBU_FAN1-FRESP_3", NULL);
        test_display(base+FC1_4_FRESP,  (uint8_t*)"MBU_FAN1-FRESP_4", NULL);
        test_display(base+FC1_1_FST,    (uint8_t*)"MBU_FAN1-FSTATUS_1", NULL);
        test_display(base+FC1_2_FST,    (uint8_t*)"MBU_FAN1-FSTATUS_2", NULL);
        test_display(base+FC1_3_FST,    (uint8_t*)"MBU_FAN1-FSTATUS_3", NULL);
        test_display(base+FC1_4_FST,    (uint8_t*)"MBU_FAN1-FSTATUS_4", NULL);
        test_display(base+FC1_1_FCMD,   (uint8_t*)"MBU_FAN1-FCMD_1", NULL);
        test_display(base+FC1_2_FCMD,   (uint8_t*)"MBU_FAN1-FCMD_2", NULL);
        test_display(base+FC1_3_FCMD,   (uint8_t*)"MBU_FAN1-FCMD_3", NULL);
        test_display(base+FC1_4_FCMD,   (uint8_t*)"MBU_FAN1-FCMD_4", NULL);
        test_display(base+FC1_1_FSPD,   (uint8_t*)"MBU_FAN1-FSPEED_1", (uint8_t*)"rpm");
        test_display(base+FC1_2_FSPD,   (uint8_t*)"MBU_FAN1-FSPEED_2", (uint8_t*)"rpm");
        test_display(base+FC1_3_FSPD,   (uint8_t*)"MBU_FAN1-FSPEED_3", (uint8_t*)"rpm");
        test_display(base+FC1_4_FSPD,   (uint8_t*)"MBU_FAN1-FSPEED_4", (uint8_t*)"rpm");
        test_display(base+FC1_1_FTIME,  (uint8_t*)"MBU_FAN1-FTIME_1", (uint8_t*)"hours");
        test_display(base+FC1_2_FTIME,  (uint8_t*)"MBU_FAN1-FTIME_2", (uint8_t*)"hours");
        test_display(base+FC1_3_FTIME,  (uint8_t*)"MBU_FAN1-FTIME_3", (uint8_t*)"hours");
        test_display(base+FC1_4_FTIME,  (uint8_t*)"MBU_FAN1-FTIME_4", (uint8_t*)"hours");
        test_display(base+FC1_1_FPWM,   (uint8_t*)"MBU_FAN1-FPWM_1", (uint8_t*)"%");
        test_display(base+FC1_2_FPWM,   (uint8_t*)"MBU_FAN1-FPWM_2", (uint8_t*)"%");
        test_display(base+FC1_3_FPWM,   (uint8_t*)"MBU_FAN1-FPWM_3", (uint8_t*)"%");
        test_display(base+FC1_4_FPWM,   (uint8_t*)"MBU_FAN1-FPWM_4", (uint8_t*)"%");
        test_display(base+FC1_1_FAPWM,  (uint8_t*)"MBU_FAN1-FAVGPWM_1", (uint8_t*)"%");
        test_display(base+FC1_2_FAPWM,  (uint8_t*)"MBU_FAN1-FAVGPWM_2", (uint8_t*)"%");
        test_display(base+FC1_3_FAPWM,  (uint8_t*)"MBU_FAN1-FAVGPWM_3", (uint8_t*)"%");
        test_display(base+FC1_4_FAPWM,  (uint8_t*)"MBU_FAN1-FAVGPWM_4", (uint8_t*)"%");

        test_display(base+FC2_ST,       (uint8_t*)"MBU_FAN2-STATUS", NULL);
        test_display(base+FC2_MODE,     (uint8_t*)"MBU_FAN2-MFR_MODE", NULL);
        test_display(base+FC2_TIME,     (uint8_t*)"MBU_FAN2-LIFETIME", (uint8_t*)"days");
        //new
        HW_set_32bit_reg(base+MSS_I2C_CTRL_CMD, MSS_I2C_CTRL_OP(0x53,1,0x0,0));  //write page command
        HW_set_32bit_reg(base+MSS_I2C_CTRL_CMD, 0x0000000C);                     //write page data (0xC=internal temp)
        HW_set_32bit_reg(base+MSS_I2C_CTRL_CMD, MSS_I2C_CTRL_OP(0x53,2,0x8D,1)); //read temp command
        sleep(500);
        test_display(base+MSS_I2C_CTRL_DAT, (uint8_t*)"MBU_FAN2-TEMP", (uint8_t*)"cdegC");

        test_display(base+FC2_1_CFG12,  (uint8_t*)"MBU_FAN2-CFG12_1", NULL);
        test_display(base+FC2_2_CFG12,  (uint8_t*)"MBU_FAN2-CFG12_2", NULL);
        test_display(base+FC2_3_CFG12,  (uint8_t*)"MBU_FAN2-CFG12_3", NULL);
        test_display(base+FC2_4_CFG12,  (uint8_t*)"MBU_FAN2-CFG12_4", NULL);
        test_display(base+FC2_1_CFGMFR, (uint8_t*)"MBU_FAN2-MFRCFG_1", NULL);
        test_display(base+FC2_2_CFGMFR, (uint8_t*)"MBU_FAN2-MFRCFG_2", NULL);
        test_display(base+FC2_3_CFGMFR, (uint8_t*)"MBU_FAN2-MFRCFG_3", NULL);
        test_display(base+FC2_4_CFGMFR, (uint8_t*)"MBU_FAN2-MFRCFG_4", NULL);
        test_display(base+FC2_1_FRESP,  (uint8_t*)"MBU_FAN2-FRESP_1", NULL);
        test_display(base+FC2_2_FRESP,  (uint8_t*)"MBU_FAN2-FRESP_2", NULL);
        test_display(base+FC2_3_FRESP,  (uint8_t*)"MBU_FAN2-FRESP_3", NULL);
        test_display(base+FC2_4_FRESP,  (uint8_t*)"MBU_FAN2-FRESP_4", NULL);
        test_display(base+FC2_1_FST,    (uint8_t*)"MBU_FAN2-FSTATUS_1", NULL);
        test_display(base+FC2_2_FST,    (uint8_t*)"MBU_FAN2-FSTATUS_2", NULL);
        test_display(base+FC2_3_FST,    (uint8_t*)"MBU_FAN2-FSTATUS_3", NULL);
        test_display(base+FC2_4_FST,    (uint8_t*)"MBU_FAN2-FSTATUS_4", NULL);
        test_display(base+FC2_1_FCMD,   (uint8_t*)"MBU_FAN2-FCMD_1", NULL);
        test_display(base+FC2_2_FCMD,   (uint8_t*)"MBU_FAN2-FCMD_2", NULL);
        test_display(base+FC2_3_FCMD,   (uint8_t*)"MBU_FAN2-FCMD_3", NULL);
        test_display(base+FC2_4_FCMD,   (uint8_t*)"MBU_FAN2-FCMD_4", NULL);
        test_display(base+FC2_1_FSPD,   (uint8_t*)"MBU_FAN2-FSPEED_1", (uint8_t*)"rpm");
        test_display(base+FC2_2_FSPD,   (uint8_t*)"MBU_FAN2-FSPEED_2", (uint8_t*)"rpm");
        test_display(base+FC2_3_FSPD,   (uint8_t*)"MBU_FAN2-FSPEED_3", (uint8_t*)"rpm");
        test_display(base+FC2_4_FSPD,   (uint8_t*)"MBU_FAN2-FSPEED_4", (uint8_t*)"rpm");
        test_display(base+FC2_1_FTIME,  (uint8_t*)"MBU_FAN2-FTIME_1", (uint8_t*)"hours");
        test_display(base+FC2_2_FTIME,  (uint8_t*)"MBU_FAN2-FTIME_2", (uint8_t*)"hours");
        test_display(base+FC2_3_FTIME,  (uint8_t*)"MBU_FAN2-FTIME_3", (uint8_t*)"hours");
        test_display(base+FC2_4_FTIME,  (uint8_t*)"MBU_FAN2-FTIME_4", (uint8_t*)"hours");
        test_display(base+FC2_1_FPWM,   (uint8_t*)"MBU_FAN2-FPWM_1", (uint8_t*)"%");
        test_display(base+FC2_2_FPWM,   (uint8_t*)"MBU_FAN2-FPWM_2", (uint8_t*)"%");
        test_display(base+FC2_3_FPWM,   (uint8_t*)"MBU_FAN2-FPWM_3", (uint8_t*)"%");
        test_display(base+FC2_4_FPWM,   (uint8_t*)"MBU_FAN2-FPWM_4", (uint8_t*)"%");
        test_display(base+FC2_1_FAPWM,  (uint8_t*)"MBU_FAN2-FAVGPWM_1", (uint8_t*)"%");
        test_display(base+FC2_2_FAPWM,  (uint8_t*)"MBU_FAN2-FAVGPWM_2", (uint8_t*)"%");
        test_display(base+FC2_3_FAPWM,  (uint8_t*)"MBU_FAN2-FAVGPWM_3", (uint8_t*)"%");
        test_display(base+FC2_4_FAPWM,  (uint8_t*)"MBU_FAN2-FAVGPWM_4", (uint8_t*)"%");

        return;
    }

    memcpy(text_buf, "0x00000000 \0",12);

    /* STATUS */
    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)"\n\r             FAN CTRL1  FAN CTRL2 ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)"\n\rSTATUS     : ");
    ureg32 = HW_get_32bit_reg(base+FC1_ST);
    uint_to_hexstr(ureg32, text_buf+2, 8);
    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    ureg32 = HW_get_32bit_reg(base+FC2_ST);
    uint_to_hexstr(ureg32, text_buf+2, 8);
    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)"\n\rMFR MODE   : ");
    ureg32 = HW_get_32bit_reg(base+FC1_MODE);
    uint_to_hexstr(ureg32, text_buf+2, 8);
    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    ureg32 = HW_get_32bit_reg(base+FC2_MODE);
    uint_to_hexstr(ureg32, text_buf+2, 8);
    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)"\n\rTIME (days): ");
    ureg32 = HW_get_32bit_reg(base+FC1_TIME);
    uint_to_hexstr(ureg32/(24*3600), text_buf+2, 8);
    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    ureg32 = HW_get_32bit_reg(base+FC2_TIME);
    uint_to_hexstr(ureg32/(24*3600), text_buf+2, 8);
    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

    //                                                                 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)"\n\n\r         --------------FAN CONTROLLER 1------------ | --------------FAN CONTROLLER 2-------------");
    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\r         -----1----------2----------3----------4--- | -----1----------2----------3----------4----");
    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\rCFG12  : ");
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_1_CFG12), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_2_CFG12), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_3_CFG12), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_4_CFG12), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_1_CFG12), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_2_CFG12), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_3_CFG12), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_4_CFG12), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\rMFRCFG : ");
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_1_CFGMFR), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_2_CFGMFR), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_3_CFGMFR), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_4_CFGMFR), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_1_CFGMFR), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_2_CFGMFR), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_3_CFGMFR), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_4_CFGMFR), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\rFRESP  : ");
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_1_FRESP), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_2_FRESP), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_3_FRESP), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_4_FRESP), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_1_FRESP), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_2_FRESP), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_3_FRESP), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_4_FRESP), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\rFSTATUS: ");
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_1_FST), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_2_FST), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_3_FST), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_4_FST), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_1_FST), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_2_FST), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_3_FST), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_4_FST), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\rFCMD   : ");
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_1_FCMD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_2_FCMD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_3_FCMD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_4_FCMD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_1_FCMD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_2_FCMD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_3_FCMD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_4_FCMD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\rFSPEED : ");
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_1_FSPD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_2_FSPD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_3_FSPD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_4_FSPD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_1_FSPD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_2_FSPD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_3_FSPD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_4_FSPD), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\rFTIME(h) ");
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_1_FTIME), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_2_FTIME), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_3_FTIME), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_4_FTIME), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_1_FTIME), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_2_FTIME), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_3_FTIME), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_4_FTIME), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\rFPWM   : ");
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_1_FPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_2_FPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_3_FPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_4_FPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_1_FPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_2_FPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_3_FPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_4_FPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);

    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\rFAVGPWM: ");
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_1_FAPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_2_FAPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_3_FAPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC1_4_FAPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_1_FAPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_2_FAPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_3_FAPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    uint_to_hexstr(HW_get_32bit_reg(base+FC2_4_FAPWM), text_buf+2, 8); MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)  "\n\r");
}

//voif test_emc_memories()
//{
//          memcpy(text_buf, "0x00000000\n\r\0",13);
//
//          //write controller settings
//          *(volatile uint32_t*)0xE0042040 = EMC_PRAM_SETTINGS;
//          *(volatile uint32_t*)0xE0042044 = EMC_FLASH_SETTINGS;
//
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "***** TEST PSRAM *****\n\r");
//          // !!! USE ABSOLUTE ADDRESSING !!!
//          //fill RAM with pattern
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Filling PSRAM with pattern...");
//          for (i=EXT_ASRAM_BASE_ADDR; i<=EXT_ASRAM_END_ADDR; i+=4) {
//              *(volatile uint32_t*)i = (i>>2);
//          }
//          //read back pattern and count errors
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r    Checking data written to PSRAM...");
//          rval = 0; //error counter
//          for (i=EXT_ASRAM_BASE_ADDR; i<=EXT_ASRAM_END_ADDR; i+=4) {
//              ureg32 = *(volatile uint32_t*)i;
//              if ( ureg32 != (i>>2) ) {
//                  //READ ERROR
//                  rval++;
//              }
//          }
//          uint_to_hexstr(rval, text_buf+2, 8);
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r    Number of read errors = ");
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r***** TEST PARALLEL FLASH MEMORY *****\n\r");
//
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Erasing FLASH sector...");
//          //emc_flash_chip_erase(EXT_FLASH_BASE_ADDR);
//          //flash_status = lld_ChipEraseOp((FLASHDATA*) EXT_FLASH_BASE_ADDR); //erase & wait for completion ***WARNING: CAN BE REALLY LONG***
//          flash_status = lld_SectorEraseOp((FLASHDATA*) EXT_FLASH_BASE_ADDR, 0x0); //fist 128KB
//          if (flash_status != DEV_NOT_BUSY) {
//              //ERASE ERROR
//              MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "FAILURE\n\r");
//          } else {
//              MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "SUCCESS\n\r");
//          }
//
//          //Check that FLASH was erased correctly
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Checking erased sector...");
//          //verify data
//          rval = 0; //readback errors
//          //for (i=0; i<EXT_FLASH_SIZE; i+=2) {
//          expected = 0xFFFF;
//          for (i=0; i<EXT_FLASH_SEC_SIZE; i++) {
//              reg16 = lld_ReadOp((FLASHDATA*) EXT_FLASH_BASE_ADDR, (ADDRESS) i);
//              if (reg16 != expected) {
//                  //read error
//                  rval++;
//              }
//          }
//          uint_to_hexstr(rval, text_buf+2, 8);
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r    Number of non-erased words = ");
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//          //fill flash with data
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Writing sector with data...");
//          rval = 0;
//          for (i=0; i<EXT_FLASH_PAGE_SIZE; i++) { //bytes
//              //first fill buffer with byte sequence
//              flash_buffer[i] = i;
//          }
//          //buffer full (NOTE: buffer is 16 bit, so length = nbytes/2)
//          flash_status = lld_memcpy((FLASHDATA*) EXT_FLASH_BASE_ADDR, 0x0, EXT_FLASH_PAGE_SIZE/2, (FLASHDATA*) flash_buffer);
//          if (flash_status != DEV_NOT_BUSY) {
//              //WRITE ERROR
//              MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "FAILURE\n\r");
//          } else {
//              MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "SUCCESS\n\r");
//          }
//
//          //verify data
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Reading back data...");
//          rval = 0;
//          for (i=0; i<(EXT_FLASH_PAGE_SIZE/2); i++) {
//              reg16 = lld_ReadOp((FLASHDATA*) EXT_FLASH_BASE_ADDR, (ADDRESS) i); //WORD address
//              expected = ((2*i+1)<<8)|(2*i); //(1st byte on lower position)
//              if (reg16 != expected ) {
//                  //read error
//                  rval++;
//              }
//          }
//          uint_to_hexstr(rval, text_buf+2, 8);
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r    Readback errors = ");
//          MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//}


///* FAN CONTROLLER TEST FUNCTIONS */
//
///****** Set up fans ******/
//void TEST_fan_setup(void) {
//    uint8_t i;
//
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\n\rWriting custom settings (0xD0,0xE310)\n\n\r");
//    max31785_write_protect(&g_mss_i2c1, I2C_FAN0_SER_ADDR,0);
//    max31785_write_protect(&g_mss_i2c1, I2C_FAN1_SER_ADDR,0);
//
//    //disable all channels
//
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Disable all channels before setting up\n\r");
//    for (i=0; i<6; i++) {
//        max31785_fan_enable(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN0+i, 0);
//        max31785_fan_enable(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN0+i, 0);
//    }
//
//    //Set fan table for health monitoring
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Write PWM to RPM tables\n\r");
//    max31785_mfr_fan_pwm2rpm(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN0, PWM2RPM_PSD1206); //11 SUNON
//    max31785_mfr_fan_pwm2rpm(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN1, PWM2RPM_NMB06038DA12S); //22 NMB
//    max31785_mfr_fan_pwm2rpm(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN2, PWM2RPM_PSD1206); //13 SUNON
//    max31785_mfr_fan_pwm2rpm(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN3, PWM2RPM_PSD1206); //24 SUNON
//    //
//    max31785_mfr_fan_pwm2rpm(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN0, PWM2RPM_NMB06038DA12S); //21 NMB
//    max31785_mfr_fan_pwm2rpm(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN1, PWM2RPM_PSD1206); //12 SUNON
//    max31785_mfr_fan_pwm2rpm(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN2, PWM2RPM_PSD1206); //23 SUNON
//    max31785_mfr_fan_pwm2rpm(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN3, PWM2RPM_PSD1206); //14 SUNON
//
//
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Setting MFR_FAN_CONFIG to 0xE310\n\r");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Setting RPM mode, and speed = 3000 RPM\n\r");
//    for(i=0; i<4; i++) {
//        /*
//         * Fan configuration:
//         * config current fan (select page first)
//         * MAX31785_MFR_FAN_CONFIG_FREQ        0xE000 //PWM frequency 0=30Hz, 1=50Hz, 2=100Hz, 3=150Hz, 7=25kHz
//         * MAX31785_MFR_FAN_CONFIG_DUAL_TACH   0x1000 //enable dual tachometer
//         * MAX31785_MFR_FAN_CONFIG_HYS         0x0C00 //hysteresis for auto slow down 0,1,2,3=2,4,6,8°C
//         * MAX31785_MFR_FAN_CONFIG_TSFO        0x0200 //0=ramp to 100% on sensor fault/no fan_command update, 1=on fault use last speed value
//         * MAX31785_MFR_FAN_CONFIG_TACHO       0x0100 //0=ramp to 100% on fan fault, 1=don't
//         * MAX31785_MFR_FAN_CONFIG_RAMP        0x00E0 //time to ramp 40% to 100%: (0,1,2,3,4,5,6,7)=(60,30,20,12,6,4,3,2.4 seconds)
//         * MAX31785_MFR_FAN_CONFIG_HEALTH      0x0010 //enable fan health check
//         * MAX31785_MFR_FAN_CONFIG_ROTOR_HI_LO 0x0008 //polarity of TACK for stopped rotor: 0=low, 1=high
//         * MAX31785_MFR_FAN_CONFIG_ROTOR       0x0004 //0=TACH connected to actual tachometer, 1=TACH input used as stuck rotor signal
//         * MAX31785_MFR_FAN_CONFIG_SPIN        0x0003 //start behavior: 0=no spinup control, 1=100% for 2 revolutions, 2=100% for 4 revolutions, 3=100% for 8 revolutions
//         *
//         * NOTE: when health check is on max31785_mfr_fan_pwm2rpm shall be setup
//         */
//
//        max31785_set_mfr_fan_config(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN0+i, 0xE310);
//        max31785_set_mfr_fan_config(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN0+i, 0xE310);
//
//        //set to RPM mode
//        max31785_fan_rpm(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN0+i, 1);
//        max31785_fan_rpm(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN0+i, 1);
//
//        //2 tach pulses
//        max31785_fan_tach_pulses(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN0+i, 2); //Fan 11,22 (front), 13,24 (back)
//        max31785_fan_tach_pulses(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN0+i, 2); //Fan 11,22 (front), 13,24 (back)
//
//        //no auto mode
//        //max31785_fan_auto_en(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN0+i);
//        //max31785_fan_force_pwm(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN0+i, 0.0);
//        //max31785_fan_force_pwm(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN0+i, 0.0);
//        max31785_fan_force_rpm(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN0+i, 3000);
//        max31785_fan_force_rpm(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN0+i, 3000);
//    }
//
//    //enable needed channels
//
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Enabling fans\n\r");
//    for (i=0; i<4; i++) {
//        max31785_fan_enable(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN0+i, 1);
//        max31785_fan_enable(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN0+i, 1);
//    }
//}
//
//uint16_t TEST_fan_get_status(uint8_t fc_i2c_addr) {
//    uint8_t text_buf[32], ureg8, i;
//    uint16_t ureg16, status;
//
//    memcpy(text_buf, "0x0000\n\r\0",9);
//    status = max31785_get_status(&g_mss_i2c1, fc_i2c_addr);
//    uint_to_hexstr((uint32_t) status, text_buf+2, 4);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rStatus 0 (0x0 = no error): ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//    if (status & MAX31785_STATUS_WORD_CML) {
//        memcpy(text_buf, "0x00\n\r\0",7);
//        ureg8 = max31785_get_status_cml(&g_mss_i2c1, fc_i2c_addr);
//        uint_to_hexstr((uint32_t) ureg8, text_buf+2, 2);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Status CML: ");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//    }
//    if (status & MAX31785_STATUS_WORD_MFR) {
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Status MFR(0,OT_Fault,OT_Warn,WD_reset,0000):\n\r");
//        memcpy(text_buf, "    0x00\0",9);
//        for (i=6; i<17; i++) {
//            ureg8 = max31785_get_status_mfr_specific(&g_mss_i2c1, fc_i2c_addr, i);
//            uint_to_hexstr((uint32_t) ureg8, text_buf+6, 2);
//            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//        }
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
//    }
//    if (status & MAX31785_STATUS_WORD_VOUT) {
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Status VOUT(OV_Fault,OV_Warn,UV_Fault,UV_Warn,0000):\n\r");
//        memcpy(text_buf, "    0x00\0",9);
//        for (i=17; i<23; i++) {
//            ureg8 = max31785_get_status_vout(&g_mss_i2c1, fc_i2c_addr, i);
//            uint_to_hexstr((uint32_t) ureg8, text_buf+6, 2);
//            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//        }
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
//    }
//
//
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r*** Info on FAN channels 0 to 4***\n\r");
//    for (i=0; i<4; i++) {
//        /* MAX31785_STATUS_FANS_FAULT   0x80 //fan fault
//         * MAX31785_STATUS_FANS_WARN    0x20 //fan warning
//         * Following values shall be COMPARED with lower nibble (instead of masked)
//         * MAX31785_STATUS_FANS_RED     0x08 //very bad health
//         * MAX31785_STATUS_FANS_ORANGE  0x04 //bad health
//         * MAX31785_STATUS_FANS_YELLOW  0x02 //good health
//         * MAX31785_STATUS_FANS_GREEN   0x01 //very good health
//         * MAX31785_STATUS_FANS_UNKNOWN 0X00 //status unknown */
//        ureg8 = max31785_get_status_fans(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i);
//        memcpy(text_buf, "0x00\n\r\0", 7);
//        uint_to_hexstr((uint32_t) ureg8, text_buf+2, 2);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r    FAN status (FAULT,0,WARN,0,Red,Orange,Yellow,Green): ");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//        max31785_read(&g_mss_i2c1, fc_i2c_addr, MAX31785_FAN_CONFIG_1_2, MAX31785_PAGE_FAN0+i, &ureg8, 1);
//        memcpy(text_buf, "0x00\n\r\0", 7);
//        uint_to_hexstr((uint32_t) ureg8, text_buf+2, 2);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Fan Config 1_2 (EN,RPM,TACH_Pulses(2),0000): ");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//        ureg16 = max31785_get_mfr_fan_config(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i);
//        memcpy(text_buf, "0x0000\n\r\0", 9);
//        uint_to_hexstr((uint32_t) ureg16, text_buf+2, 4);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    MFR Fan Config (freq(3),dual,hyst(2),100fault(2),accel(3),health,stuck(2),spinup(2)): ");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//        ureg16 = max31785_mfr_fan_run_time(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i);
//        memcpy(text_buf, "00000\n\r\0", 8);
//        uint_to_decstr((uint32_t) ureg16, text_buf, 5);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Fan Runtime(hrs): ");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//        ureg16 = max31785_mfr_fan_pwm_avg(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i);
//        memcpy(text_buf, " 000.00\n\r\0", 10);
//        float_to_string(((float) ureg16)/100, text_buf, 3,2);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Fan AvgPWM(%): ");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//    }
//
//    return (status);
//}
//
//
///****** Set up and enable temperature sensors ******/
//void TEST_fan_setup_monitors(void) {
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rSetup Temperature Sensors\n\r");
//    //Diode on ADC0: reset peak, no control over fans, enable temp sensor
//    max31785_reset_temperature_peak(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_TD0);
//    max31785_set_mfr_temp_sensor_config(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_TD0, 0x8000);
//
//    max31785_reset_temperature_peak(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_TD0);
//    max31785_set_mfr_temp_sensor_config(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_TD0, 0x8000);
//
//    /*********** Set up and enable remote voltage monitors *************/
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rSetup Voltage Monitors\n\r");
//    max31785_vout_scale_monitor(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV1, 0x09D9); //set scale for 12V
//    max31785_reset_voltage_peak(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV1); //reset peak measure
//    max31785_reset_voltage_min( &g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV1); //reset min measure
//
//    max31785_vout_scale_monitor(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV2, 0x27FF); //set scale for 3.3V
//    max31785_reset_voltage_peak(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV2); //reset peak measure
//    max31785_reset_voltage_min( &g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV2); //reset min measure
//
//    max31785_voltage_sense_en(  &g_mss_i2c1, I2C_FAN0_SER_ADDR, 0x06); //enable channels 1+2
//
//    max31785_vout_scale_monitor(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_RV1, 0x09D9); //set scale for 12V
//    max31785_reset_voltage_peak(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_RV1); //reset peak measure
//    max31785_reset_voltage_min( &g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_RV1); //reset min measure
//
//    max31785_vout_scale_monitor(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_RV2, 0x27FF); //set scale for 3.3V
//    max31785_reset_voltage_peak(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_RV2); //reset peak measure
//    max31785_reset_voltage_min( &g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_RV2); //reset min measure
//
//    max31785_voltage_sense_en(  &g_mss_i2c1, I2C_FAN1_SER_ADDR, 0x06); //enable channels 1+2
//}
//
//
///* profile fans (sets control to PWM) */
//void TEST_fan_profile(uint8_t fc_i2c_addr) {
//    uint8_t i, j, text_buf[16], ureg8;
//    uint16_t ureg16;
//    int16_t reg16;
//
//    for(i=0; i<4; i++) {
//        //first set control to PWM
//        max31785_fan_rpm(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i, 0);
//
//        /* MAX31785_STATUS_FANS_FAULT   0x80 //fan fault
//         * MAX31785_STATUS_FANS_WARN    0x20 //fan warning
//         * Following values shall be COMPARED with lower nibble (instead of masked)
//         * MAX31785_STATUS_FANS_RED     0x08 //very bad health
//         * MAX31785_STATUS_FANS_ORANGE  0x04 //bad health
//         * MAX31785_STATUS_FANS_YELLOW  0x02 //good health
//         * MAX31785_STATUS_FANS_GREEN   0x01 //very good health
//         * MAX31785_STATUS_FANS_UNKNOWN 0X00 //status unknown */
//        ureg8 = max31785_get_status_fans(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i);
//        memcpy(text_buf, "0x00\n\r\0", 7);
//        uint_to_hexstr((uint32_t) ureg8, text_buf+2, 2);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Fan Status: ");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//        max31785_read(&g_mss_i2c1, fc_i2c_addr, MAX31785_FAN_CONFIG_1_2, MAX31785_PAGE_FAN0+i, &ureg8, 1);
//        memcpy(text_buf, "0x00\n\r\0", 7);
//        uint_to_hexstr((uint32_t) ureg8, text_buf+2, 2);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Fan Config 1_2: ");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//        ureg16 = max31785_get_mfr_fan_config(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i);
//        memcpy(text_buf, "0x0000\n\r\0", 9);
//        uint_to_hexstr((uint32_t) ureg8, text_buf+2, 4);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    MFR Fan Config: ");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//        ureg16 = max31785_mfr_fan_run_time(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i);
//        memcpy(text_buf, "00000\n\r\0", 8);
//        uint_to_decstr((uint32_t) ureg16, text_buf, 5);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Fan Runtime(hrs): ");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//        ureg16 = max31785_mfr_fan_pwm_avg(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i);
//        memcpy(text_buf, " 000.00\n\r\0", 10);
//        float_to_string(((float) ureg16)/100, text_buf, 3,2);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Fan AvgPWM(%): ");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//        //set value, wait, read status (PWM= 40,60,80,100)
//        //                                                         "    000%   :  +000.00  00000
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    PWM(w) :  PWM(r)   RPM(r)\n\r");
//        for(j=1; j<=10; j++) {
//            memcpy(text_buf, "    000,      \0", 14);
//            uint_to_decstr(10*j, text_buf+4, 3);
//            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//            max31785_fan_force_pwm(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i, (float)(j*10));
//            sleep(10000);
//
//            reg16 = max31785_mfr_read_fan_pwm(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i);
//            memcpy(text_buf, " 000.00, \0", 10);
//            float_to_string(((float) reg16)/100, text_buf, 3,2);
//            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//            reg16 = max31785_read_fan_speed(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i);
//            memcpy(text_buf, "00000\n\r\0", 8);
//            uint_to_decstr((uint32_t) reg16, text_buf, 5);
//            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//        }
//
//        max31785_fan_force_pwm(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i, 10.0); //reset speed to non-noisy level
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
//    }
//}
//
///* set speed as RPM and check reading accuracy */
//void TEST_fan_set_speed(uint8_t fc_i2c_addr, uint16_t min, uint16_t max, uint16_t step) {
//    uint8_t i, text_buf[16];
//    uint16_t ureg16, j;
//
//    for(i=0; i<4; i++) {
//        //first set control to RPM
//        max31785_fan_rpm(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i, 1);
//
//        //set value, wait, read status (PWM= 40,60,80,100)
//        //                                                         "    00000  : 00000
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    RPM(w) : RPM(r)\n\r");
//        for(j=min; j<=max; j+=step) {
//            memcpy(text_buf, "    00000  : \0", 14);
//            uint_to_decstr(j, text_buf+4, 5);
//            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//            max31785_fan_force_rpm(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i, j);
//            sleep(10000);
//
//            ureg16 = max31785_read_fan_speed(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i);
//            memcpy(text_buf, "00000\n\r\0", 8);
//            uint_to_decstr(ureg16, text_buf, 5);
//            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//        }
//
//        max31785_fan_force_rpm(&g_mss_i2c1, fc_i2c_addr, MAX31785_PAGE_FAN0+i, 2000); //reset speed to non-noisy level
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
//    }
//}
//
///* set speed and stability on all fans */
//void TEST_fan_speed_stability(uint16_t speed, uint8_t nreads) {
//    uint8_t i,t, text_buf[16], rx_size; //, rx_buff;
//    uint16_t ureg16;
//
//    for(i=0; i<4; i++) {
//        //first set control to RPM
//        max31785_fan_rpm(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN0+i, 1);
//        max31785_fan_rpm(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN0+i, 1);
//        //then set speed
//        max31785_fan_force_rpm(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN0+i, speed);
//        max31785_fan_force_rpm(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN0+i, speed);
//    }
//
//    rx_size = 0;
//    memcpy(text_buf, "    00000,\0", 10);
//    //read RPM every second
//    //while (rx_size == 0) {
//    for (t=0; t<nreads; t++) {
//        //rx_size = MSS_UART_get_rx( &g_mss_uart0, &rx_buff, 1 );
//
//        for (i=0; i<4; i++) {
//            ureg16 = max31785_read_fan_speed(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_FAN0+i);
//            uint_to_decstr(ureg16, text_buf+4, 5);
//            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//        }
//
//        //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) " | ");
//        for (i=0; i<4; i++) {
//            ureg16 = max31785_read_fan_speed(&g_mss_i2c1, I2C_FAN1_SER_ADDR, MAX31785_PAGE_FAN0+i);
//            uint_to_decstr(ureg16, text_buf+4, 5);
//            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//        }
//
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
//        sleep(10000);
//    }
//}
//
//void TEST_fan_get_temp_volt(void) {
//    uint8_t text_buf[16], rx_buf[1];
//    int16_t reg16;
//
//    memcpy(text_buf, "0x00000000\n\r\0", 13);
//    //get readings from temperature sensor
//    rx_buf[0] = max31785_get_status_mfr_specific(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_TD0);
//    uint_to_hexstr((uint32_t) rx_buf[0], text_buf+2, 8);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\n\rTemp Sensor Status: ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//    reg16 = max31785_read_temperature(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_TD0);
//    memcpy(text_buf, " 000.00  \0", 10);
//    float_to_string(((float) reg16)/100, text_buf, 3,2);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Temp: ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//    reg16 = max31785_get_temperature_peak(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_TD0);
//    memcpy(text_buf, " 000.00  \0", 10);
//    float_to_string(((float) reg16)/100, text_buf, 3,2);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "TempPeak: ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//    //get readings from voltage monitors
//    rx_buf[0] = max31785_get_status_vout(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV1);
//    memcpy(text_buf, "0x00000000\n\r\0", 13);
//    uint_to_hexstr((uint32_t) rx_buf[0], text_buf+2, 8);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\n\rVoltage Monitor 1 Status: ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//    reg16 = max31785_read_vout(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV1);
//    memcpy(text_buf, "00000\n\r\0", 8);
//    uint_to_decstr((uint32_t) reg16, text_buf, 5);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Voltage1(mV): ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//    reg16 = max31785_get_voltage_peak(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV1);
//    memcpy(text_buf, "00000\n\r\0", 8);
//    uint_to_decstr((uint32_t) reg16, text_buf, 5);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Voltage1 peak(mV): ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//    reg16 = max31785_get_voltage_min(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV1);
//    memcpy(text_buf, "00000\n\r\0", 8);
//    uint_to_decstr((uint32_t) reg16, text_buf, 5);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Voltage1 min(mV): ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//    //--------------------------------------------------------------------------------------
//    rx_buf[0] = max31785_get_status_vout(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV2);
//    memcpy(text_buf, "0x00000000\n\r\0", 13);
//    uint_to_hexstr((uint32_t) rx_buf[0], text_buf+2, 8);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\n\rVoltage Monitor 2 Status: ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//    reg16 = max31785_read_vout(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV2);
//    memcpy(text_buf, "00000\n\r\0", 8);
//    uint_to_decstr((uint32_t) reg16, text_buf, 5);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Voltage2(mV): ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//    reg16 = max31785_get_voltage_peak(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV2);
//    memcpy(text_buf, "00000\n\r\0", 8);
//    uint_to_decstr((uint32_t) reg16, text_buf, 5);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Voltage2 peak(mV): ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//
//    reg16 = max31785_get_voltage_min(&g_mss_i2c1, I2C_FAN0_SER_ADDR, MAX31785_PAGE_RV2);
//    memcpy(text_buf, "00000\n\r\0", 8);
//    uint_to_decstr((uint32_t) reg16, text_buf, 5);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Voltage2 min(mV): ");
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
//}
//
//
//
//
//void TEST_nrg_read_all(void) {
//    uint16_t i;
//    uint32_t base;
//    uint8_t text_buf[] = "Reg 0x00: 0x000000\n\r\0";
//
//    //memcpy(text_buf, "Reg 0x00: 0x000000\n\r\0", 21); //init string for display
//
//    /* read & display current status */
//    /* read page 0, register 0x0 to 0x37 */
//    cs5480_select_page(0);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r******** Page 00:\n\r");
//    i=0;
//    while (i<56) {
//        for (i=0; i <= 0x37; i++) {
//            if (  i==0 ||
//                    i==1 ||
//                    i==3 ||
//                    i==5 ||
//                    i==7 ||
//                    i==8 ||
//                    i==9 ||
//                    i==23 ||
//                    i==24 ||
//                    i==25 ||
//                    i==34 ||
//                    i==36 ||
//                    i==37 ||
//                    i==38 ||
//                    i==39 ||
//                    i==48 ||
//                    i==55) {
//                base = cs5480_reg_read(i);
//                uint_to_hexstr(i, text_buf+6, 2);
//                uint_to_hexstr(base, text_buf+12, 6);
//                MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//            }
//        }
//    }
//
//    /* read page 16, register 0x0 to 0x3D */
//    cs5480_select_page(16);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r******** Page 16:\n\r");
//    for (i=0; i <= 0x3D; i++) {
//        if (i!=18 &&
//                i!=19 &&
//                i!=22 &&
//                i!=23 &&
//                i!=26 &&
//                i!=28 &&
//                i!=46 &&
//                i!=47 &&
//                i!=48 &&
//                i!=52 &&
//                i!=53 &&
//                i!=62 &&
//                i!=63 ) {
//            base = cs5480_reg_read(i);
//            uint_to_hexstr(i, text_buf+6, 2);
//            uint_to_hexstr(base, text_buf+12, 6);
//            MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//        }
//    }
//    /* read page 17, register 0x0 to 0x0D */
//    cs5480_select_page(17);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r******** Page 17:\n\r");
//    for (i=0; i <= 0x0D; i++) {
//        if (i==0  ||
//                i==1  ||
//                i==4  ||
//                i==5  ||
//                i==8  ||
//                i==9  ||
//                i==12 ||
//                i==13){
//            base = cs5480_reg_read(i);
//            uint_to_hexstr(i, text_buf+6, 2);
//            uint_to_hexstr(base, text_buf+12, 6);
//            MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//        }
//    }
//    /* read page 18, register 0x18 to 0x3F */
//    cs5480_select_page(18);
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r******** Page 18:\n\r");
//    for (i=0x18; i <= 0x3F; i++) {
//        if (i==24 ||
//                i==28 ||
//                i==43 ||
//                i==46 ||
//                i==47 ||
//                i==50 ||
//                i==51 ||
//                i==58 ||
//                i==62 ||
//                i==63){
//            base = cs5480_reg_read(i);
//            uint_to_hexstr(i, text_buf+6, 2);
//            uint_to_hexstr(base, text_buf+12, 6);
//            MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//        }
//    }
//}
//
//
//void TEST_nrg_init_params(void) {
//    uint32_t base;
//    uint8_t text_buf[36];
//
//    /* page 0 */
//    cs5480_select_page(0);
//
//    /* write config0 default value */
//    cs5480_reg_write(CS5480_CONFIG0, CS5480_CONFIG0_DEF);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_CONFIG0);
//    memcpy(text_buf, "CONFIG0 = 0x000000 (0xC02000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* write config0 default value */
//    cs5480_reg_write(CS5480_CONFIG1, CS5480_CONFIG1_DEF | CS5480_CONFIG0_I1PGA | CS5480_CONFIG0_I2PGA); //50x gain for I
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_CONFIG1);
//    memcpy(text_buf, "CONFIG1 = 0x000000 (0x00E0A0)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* clear interrupts */
//    cs5480_reg_write(CS5480_STATUS0, 0xE7FFFD);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_STATUS0);
//    memcpy(text_buf, "IRQFLAG = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* write interrupt mask default value */
//    cs5480_reg_write(CS5480_MASK, 0x000000);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_MASK);
//    memcpy(text_buf, "IRQMASK = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* write phase compensation control default value */
//    cs5480_reg_write(CS5480_PC, 0x000000);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_PC);
//    memcpy(text_buf, "PHCOMP  = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* write serial interface control default value */
//    cs5480_reg_write(CS5480_SERCTRL, 0x02004D);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_SERCTRL);
//    memcpy(text_buf, "SERCTRL = 0x000000 (0x02004D)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* write number of zero crossings used for line-freq detection */
//    cs5480_reg_write(CS5480_ZXNUM, 0x000064);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_ZXNUM);
//    memcpy(text_buf, "ZXNUM   = 0x000000 (0x000064)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* page 16 */
//    cs5480_select_page(16);
//
//    /* write config2 default */
//    cs5480_reg_write(CS5480_CONFIG2, 0x000200 | CS5480_CONFIG2_I2FLT_HP | CS5480_CONFIG2_V2FLT_HP | CS5480_CONFIG2_I1FLT_HP | CS5480_CONFIG2_V1FLT_HP); //HP filters
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_CONFIG2);
//    memcpy(text_buf, "CONFIG2 = 0x000000 (0x0002AA)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* offsets */
//    cs5480_reg_write(CS5480_I1DCOFF, 0x000000); //i1 DC offset
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_I1DCOFF);
//    memcpy(text_buf, "I1DCOFF = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_V1DCOFF, 0x000000); //v1 DC offset
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_V1DCOFF);
//    memcpy(text_buf, "V1DCOFF = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_P1OFF,   0x000000); //p1 offset
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_P1OFF);
//    memcpy(text_buf, "P1OFF   = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_Q1OFF,   0x000000); //q1 offset
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_Q1OFF);
//    memcpy(text_buf, "Q1OFF   = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_I2DCOFF, 0x000000); //i2 DC offset
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_I2DCOFF);
//    memcpy(text_buf, "I2DCOFF = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_V2DCOFF, 0x000000); //v2 DC offset
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_V2DCOFF);
//    memcpy(text_buf, "V2DCOFF = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_P2OFF,   0x000000); //p2 DC offset
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_P2OFF);
//    memcpy(text_buf, "P2OFF   = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_Q2OFF,   0x000000); //q2 DC offset
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_Q2OFF);
//    memcpy(text_buf, "Q2OFF   = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* gains */
//    cs5480_reg_write(CS5480_I1GAIN, 0x400000); //i1 gain
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_I1GAIN);
//    memcpy(text_buf, "I1GAIN  = 0x000000 (0x400000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_V1GAIN, 0x400000); //v1 gain
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_V1GAIN);
//    memcpy(text_buf, "V1GAIN  = 0x000000 (0x400000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_I2GAIN, 0x400000); //i2 gain
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_I2GAIN);
//    memcpy(text_buf, "I2GAIN  = 0x000000 (0x400000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_V2GAIN, 0x400000); //v2 gain
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_V2GAIN);
//    memcpy(text_buf, "V2GAIN  = 0x000000 (0x400000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//
//    /* write automatic channel selection level default */
//    cs5480_reg_write(CS5480_ICHANLV, 0x828F5C);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_ICHANLV);
//    memcpy(text_buf, "ICHANLV = 0x000000 (0x828F5C)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* write temp gain default */
//    cs5480_reg_write(CS5480_TGAIN, 0x06B716);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_TGAIN);
//    memcpy(text_buf, "TMPGAIN = 0x000000 (0x06B716)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* write temp offset default */
//    cs5480_reg_write(CS5480_TOFF, 0xD53998);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_TOFF);
//    memcpy(text_buf, "TMPOFFS = 0x000000 (0xD53998)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* write chan select min amplitude default */
//    cs5480_reg_write(CS5480_PMIN, 0x00624D);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_PMIN);
//    memcpy(text_buf, "CS_AMIN = 0x000000 (0x00624D)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* write filter settle time default */
//    cs5480_reg_write(CS5480_TSETTLE, 0x00001E);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_TSETTLE);
//    memcpy(text_buf, "TSETTLE = 0x000000 (0x00001E)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* write no-load threshold default */
//    cs5480_reg_write(CS5480_LOADMIN, 0x000000);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_LOADMIN);
//    memcpy(text_buf, "LOADMIN = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* write voltage fixed RMS reference default */
//    cs5480_reg_write(CS5480_VFRMS, 0x5A8259);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_VFRMS);
//    memcpy(text_buf, "VF_RMS  = 0x000000 (0x5A8259)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* write system gain default */
//    cs5480_reg_write(CS5480_SYSGAIN, 0x500000);
//    /* readback register to check */
//    base = cs5480_reg_read(CS5480_SYSGAIN);
//    memcpy(text_buf, "SYSGAIN = 0x000000 (0x500000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* page 17 */
//    cs5480_select_page(17);
//
//    /* sag/overcurrent levels */
//    cs5480_reg_write(CS5480_V1SAGDUR, 0x000000); //V1 sag duration
//    base = cs5480_reg_read(CS5480_V1SAGDUR);
//    memcpy(text_buf, "V1SAG_D = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_V2SAGDUR, 0x000000); //V2 sag duration
//    base = cs5480_reg_read(CS5480_V2SAGDUR);
//    memcpy(text_buf, "V2SAG_D = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_I1OVRDUR, 0x000000); //I1 overcurrent duration
//    base = cs5480_reg_read(CS5480_I1OVRDUR);
//    memcpy(text_buf, "I1OVR_D = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_I2OVRDUR, 0x000000); //I2 overcurrent duration
//    base = cs5480_reg_read(CS5480_I2OVRDUR);
//    memcpy(text_buf, "I2OVR_D = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_V1SAGLVL, 0x000000); //V1 sag level
//    base = cs5480_reg_read(CS5480_V1SAGLVL);
//    memcpy(text_buf, "V1SAG_L = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_V2SAGLVL, 0x000000); //V2 sag level
//    base = cs5480_reg_read(CS5480_V2SAGLVL);
//    memcpy(text_buf, "V2SAG_L = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_I1OVRLVL, 0x7FFFFF); //I1 overcurrent level
//    base = cs5480_reg_read(CS5480_I1OVRLVL);
//    memcpy(text_buf, "I1OVR_L = 0x000000 (0x7FFFFF)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_I2OVRLVL, 0x7FFFFF); //I2 overcurrent level
//    base = cs5480_reg_read(CS5480_I2OVRLVL);
//    memcpy(text_buf, "I2OVR_L = 0x000000 (0x7FFFFF)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    /* page 18 */
//    cs5480_select_page(18);
//
//    cs5480_reg_write(CS5480_IZXLVL,  0x100000);
//    base = cs5480_reg_read(CS5480_IZXLVL);
//    memcpy(text_buf, "IZX_LVL = 0x000000 (0x100000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_PLSRATE, 0x800000);
//    base = cs5480_reg_read(CS5480_PLSRATE);
//    memcpy(text_buf, "PLSRATE = 0x000000 (0x800000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_INTGAIN, 0x143958);
//    base = cs5480_reg_read(CS5480_INTGAIN);
//    memcpy(text_buf, "INTGAIN = 0x000000 (0x143958)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_V1SWDUR, 0x000000);
//    base = cs5480_reg_read(CS5480_V1SWDUR);
//    memcpy(text_buf, "V1SWDUR = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_V1SWLVL, 0x7FFFFF);
//    base = cs5480_reg_read(CS5480_V1SWLVL);
//    memcpy(text_buf, "V1SWLVL = 0x000000 (0x7FFFFF)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_V2SWDUR, 0x000000);
//    base = cs5480_reg_read(CS5480_V2SWDUR);
//    memcpy(text_buf, "V2SWDUR = 0x000000 (0x000000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_V2SWLVL, 0x7FFFFF);
//    base = cs5480_reg_read(CS5480_V2SWLVL);
//    memcpy(text_buf, "V2SWLVL = 0x000000 (0x7FFFFF)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_VZXLVL,  0x100000);
//    base = cs5480_reg_read(CS5480_VZXLVL);
//    memcpy(text_buf, "VZX_LVL = 0x000000 (0x100000)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//
//    cs5480_reg_write(CS5480_SCALE,   0x4CCCCC);
//    base = cs5480_reg_read(CS5480_SCALE);
//    memcpy(text_buf, "SYSCALE = 0x000000 (0x4CCCCC)\n\r\0", 32); //init string for display
//    uint_to_hexstr(base, text_buf+12, 6);
//    MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
//}
