/*
 * pca2129.c
 *
 *  Created on: 05.11.2015
 *      Author: malatesta_a
 */

#include <string.h>
#include "pca2129.h"
#include "uart_utility.h"
#include "mss_watchdog.h"

uint8_t str_to_bcd(char* str);

void pca2129_check_osc(void* i2c, uint8_t i2c_addr)
{
    uint8_t tx_buf[1], rx_buf[1];

    /* Check oscillator stop flag PCA2129_SECS */
    tx_buf[0] = PCA2129_SECS;
    rx_buf[0] = 0x00;
    core_i2c_doread_nors(i2c, i2c_addr, tx_buf, 1, rx_buf, 1, (const uint8_t *) "RTC: Read OSF\n\r");
    /* if oscillator has stopped, clear flag and refresh OTP */
    if (rx_buf[0] & PCA2129_SECS_OSF) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rRTC: Found oscillator stop flag active. Clearing flag...\n\r");
        tx_buf[0] = PCA2129_SECS;
        tx_buf[1] = 0x00;
        core_i2c_dowrite(i2c, i2c_addr, tx_buf, 2, (const uint8_t *) "RTC: Clear OSF flag\n\r");

        /* OTP refresh after startup/reset */
        tx_buf[0] = PCA2129_CKOUT_CTRL;
        tx_buf[1] = 0x00;
        core_i2c_dowrite(i2c, i2c_addr, tx_buf, 2, (const uint8_t *) "RTC: OTP reset phase 1\n\r");
        tx_buf[1] = PCA2129_CKOUT_OTPR;
        core_i2c_dowrite(i2c, i2c_addr, tx_buf, 2, (const uint8_t *) "RTC: OTP reset phase 2\n\r");
    }
}

void pca2129_disable_alarms(void* i2c, uint8_t i2c_addr)
{
    uint8_t tx_buf[6];

    /* disable all alarms */
    tx_buf[0] = PCA2129_ALARM_SEC;
    tx_buf[1] = PCA2129_ALARM_SEC_DIS;
    tx_buf[2] = PCA2129_ALARM_SEC_DIS;
    tx_buf[3] = PCA2129_ALARM_SEC_DIS;
    tx_buf[4] = PCA2129_ALARM_SEC_DIS;
    tx_buf[5] = PCA2129_ALARM_SEC_DIS;
    core_i2c_dowrite(i2c, i2c_addr, tx_buf, 6, (const uint8_t *) "RTC: Disable alarms\n\r");
}

void pca2129_write_ctrl_reg(void* i2c, uint8_t i2c_addr, uint8_t reg, uint8_t val)
{
    uint8_t tx_buf[1];

    /* reset ctrl0 register */
    switch (reg) {
        case 2:
            tx_buf[0] = PCA2129_CTRL2;
            break;
        case 3:
            tx_buf[0] = PCA2129_CTRL3;
            break;
        default:
            tx_buf[0] = PCA2129_CTRL1;
            break;
    }
    tx_buf[1] = val;
    core_i2c_dowrite(i2c, i2c_addr, tx_buf, 2, (const uint8_t *) "RTC: write control register 1\n\r");
}

uint32_t pca2129_read_ctrl_status(void* i2c, uint8_t i2c_addr)
{
    uint8_t i, tx_buf[1], rx_buf[3], text_buf[] = "0x00\n\r\0";

    tx_buf[0] = PCA2129_CTRL1;
    core_i2c_doread_nors(i2c, i2c_addr, tx_buf, 1, rx_buf, 3, (const uint8_t *) "RTC:Read control registers\n\r");

    /* display */
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rControl registers (1 to 3):\n\r");
    for (i=0; i<3; i++) {
        uint_to_hexstr(rx_buf[i], text_buf+2, 2);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }
    if (rx_buf[PCA2129_CTRL1] & PCA2129_CTRL1_STOP) MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "WARNING: Clock stopped.\n\r");
    if (rx_buf[PCA2129_CTRL2] & PCA2129_CTRL2_AF)   MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "WARNING: Alarm flag set.\n\r");
    if (rx_buf[PCA2129_CTRL2] & PCA2129_CTRL2_WDTF) MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "WARNING: Watchdog flag set.\n\r");
    if (rx_buf[PCA2129_CTRL3] & PCA2129_CTRL3_BLF)  MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "WARNING: Battery low.\n\r");
    if (rx_buf[PCA2129_CTRL3] & PCA2129_CTRL3_BF)   MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "WARNING: Battery switchover flag set.\n\r");

    return(rx_buf[2]<<16 | rx_buf[1]<<8 | rx_buf[0]);
}

//wd= weekday, sunday=0, hour[]="00:00:00", date[]="00/00/00"
void pca2129_set_clock(void* i2c, uint8_t i2c_addr, uint8_t wd, char* hour, char* date)
{
    uint8_t tx_buf[8];

    /* set clock */
    /* send STOP */
    tx_buf[0] = PCA2129_CTRL1;
    tx_buf[1] = PCA2129_CTRL1_STOP;
    core_i2c_dowrite(i2c, i2c_addr, tx_buf, 2, (const uint8_t *) "RTC: Send STOP command\n\r");

    /* set time */
    tx_buf[0] = PCA2129_SECS;
    tx_buf[1] = str_to_bcd(hour+6); //seconds
    tx_buf[2] = str_to_bcd(hour+3); //minutes
    tx_buf[3] = str_to_bcd(hour+0); //hours
    tx_buf[4] = str_to_bcd(date+0); //day
    tx_buf[5] = wd; //weekday (0=sunday)
    tx_buf[6] = str_to_bcd(date+3); //month
    tx_buf[7] = str_to_bcd(date+6); //year
    core_i2c_dowrite(i2c, i2c_addr, tx_buf, 8, (const uint8_t *) "RTC: Set time\n\r");

    /* remove STOP */
    tx_buf[0] = PCA2129_CTRL1;
    tx_buf[1] = 0x00;
    core_i2c_dowrite(i2c, i2c_addr, tx_buf, 2, (const uint8_t *) "RTC: Send START command\n\r");
}

//return time in BCD 0x00HHMMSS
uint32_t pca2129_get_time(void* i2c, uint8_t i2c_addr, uint8_t display)
{
    uint8_t tx_buf[1], rx_buf[3], str[] = "00:00:00\0";

    tx_buf[0] = PCA2129_SECS;
    core_i2c_doread_nors(i2c, i2c_addr, tx_buf, 1, rx_buf, 3, (const uint8_t *) "RTC: read time\n\r");

    if ((rx_buf[0] & 0x80) && display)
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rWARNING: Oscillator Stop Flag = 1\n\r");

    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rCurrent time: ");
    //hours = rx_buf[2]
    str[0] = ((rx_buf[2]>>4) & 0xF)+'0';
    str[1] = ((rx_buf[2]>>0) & 0xF)+'0';
    //minutes = rx_buf[1]
    str[3] = ((rx_buf[1]>>4) & 0xF)+'0';
    str[4] = ((rx_buf[1]>>0) & 0xF)+'0';
    //seconds = rx_buf[0]
    str[6] = ((rx_buf[0]>>4) & 0x7)+'0';
    str[7] = ((rx_buf[0]>>0) & 0xF)+'0';

    if (display) {
        MSS_UART_polled_tx_string( &g_mss_uart0, str);
    }

    return (rx_buf[2]<<16 | rx_buf[1]<<8 | rx_buf[0]);
}

//return date in BCD 0xWWDDMMYY
//display: 0=print nothing, 1=print date without weekday, 2=print full date
uint32_t pca2129_get_date(void* i2c, uint8_t i2c_addr, uint8_t display)
{
    uint8_t tx_buf[1], rx_buf[4], str[] = "www 00/00/00\0";

    tx_buf[0] = PCA2129_DAYS;
    core_i2c_doread_nors(i2c, i2c_addr, tx_buf, 1, rx_buf, 4, (const uint8_t *) "RTC: Read date\n\r");

    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rCurrent date: ");
    //weekdays = rx_buf[1]
    switch(rx_buf[1] & 0x7) {
        case 0:
            memcpy(str, "Sun", 3);
            break;
        case 1:
            memcpy(str, "Mon", 3);
            break;
        case 2:
            memcpy(str, "Tue", 3);
            break;
        case 3:
            memcpy(str, "Wed", 3);
            break;
        case 4:
            memcpy(str, "Thu", 3);
            break;
        case 5:
            memcpy(str, "Fri", 3);
            break;
        case 6:
            memcpy(str, "Sat", 3);
            break;
        default:
            memcpy(str, "???", 3);
    }

    //days = rx_buf[0]
    str[4]  = ((rx_buf[0]>>4) & 0xF)+'0';
    str[5]  = ((rx_buf[0]>>0) & 0xF)+'0';
    //months = rx_buf[2]
    str[7]  = ((rx_buf[2]>>4) & 0xF)+'0';
    str[8]  = ((rx_buf[2]>>0) & 0xF)+'0';
    //years = rx_buf[3]
    str[10] = ((rx_buf[3]>>4) & 0x7)+'0';
    str[11] = ((rx_buf[3]>>0) & 0xF)+'0';

    if (display == 1) { //ditch weekday
        memcpy(str, str+4, 9);
    }

    if (display) {
        MSS_UART_polled_tx_string( &g_mss_uart0, str);
    }

    return (rx_buf[1]<<24 | rx_buf[0]<<16 | rx_buf[2]<<8 | rx_buf[3]);
}

//convert string number to BCD format (1 nibble per digit)
//es: "18" => 0x18
uint8_t str_to_bcd(char* str)
{
   return ((str[0]-'0')<<4 | (str[1]-'0'));
}

/* read from I2C slave, then wait for end of transaction
 * (no repeated start. write operation followed by a read)
 */
i2c_status_t core_i2c_doread_nors(
        i2c_instance_t * i2c_inst, uint8_t serial_addr,
        uint8_t * tx_buffer, uint8_t write_length,
        uint8_t * rx_buffer, uint8_t read_length,
        const uint8_t *msg) {

    i2c_status_t status;

    I2C_write(i2c_inst, serial_addr, tx_buffer, write_length, I2C_RELEASE_BUS);
    status = I2C_wait_complete(i2c_inst, 3000u);

    if (status == I2C_FAILED) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C read (W) transaction failed\n\r");
    }
    if (status == I2C_TIMED_OUT) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C read (W) transaction timed-out\n\r");
        MSS_WD_reload();
    }

    I2C_read(i2c_inst, serial_addr, rx_buffer, read_length, I2C_RELEASE_BUS);
    status = I2C_wait_complete(i2c_inst, 3000u);

    if (status == I2C_FAILED) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C read (R) transaction failed\n\r");
    }
    if (status == I2C_TIMED_OUT) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C read (R) transaction timed-out\n\r");
        MSS_WD_reload();
    }

    return status;
}
