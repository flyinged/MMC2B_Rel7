/*
 * pca2129.h
 *
 *  Created on: 30.09.2015
 *      Author: malatesta_a
 */

#ifndef PCA2129_H_
#define PCA2129_H_

#include "mss_uart.h"
#include "core_i2c.h"

/* PCA2129 I2C REAL TIME CLOCK xxx write .c file */
#define PCA2129_CTRL1 0x00
    #define PCA2129_CTRL1_SI       0x01 //enable second interrupt
    #define PCA2129_CTRL1_MI       0x02 //enable minute interrupt
    #define PCA2129_CTRL1_12_24    0x04 //12 hour mode when 1
    #define PCA2129_CTRL1_POR_OVRD 0x08 //power-on reset override (0=disabled/normal operation)
//  #define PCA2129_CTRL1_TSF1     0x10 //timestamp interrupt enable (TSn input floating)
    #define PCA2129_CTRL1_STOP     0x20 //stop clock when 1
//  #define PCA2129_CTRL1_T        0x40 //unused: shall be always 0
    #define PCA2129_CTRL1_EXT_TEST 0x80 //external clock test mode (0=normal mode)
#define PCA2129_CTRL2 0x01
//  #define PCA2129_CTRL2_T    0x01 //unused: shall be always 0
    #define PCA2129_CTRL2_AIE  0x02 //enable interrupt on alarm flag
    #define PCA2129_CTRL2_TSIE 0x04 //enable interrupt on timestamp flag
//  #define PCA2129_CTRL2_T    0x08 //unused: shall be always 0
    #define PCA2129_CTRL2_AF   0x10 //alarm flag (write 0 to clear)
//  #define PCA2129_CTRL2_TSF2 0x20 //timestamp interrupt enable (TSn input floating)
    #define PCA2129_CTRL2_WDTF 0x40 //watchdog expired flag (clear by reading CTRL2 or loading WD timer)
    #define PCA2129_CTRL2_MSF  0x80 //minute/second flag (write 0 to clear)
#define PCA2129_CTRL3 0x02
    #define PCA2129_CTRL3_BLIE 0x01 //enable interrupt on battery low
    #define PCA2129_CTRL3_BIE  0x02 //enable interrupt on battery switchover
    #define PCA2129_CTRL3_BLF  0x04 //battery low alarm flag
    #define PCA2129_CTRL3_BF   0x08 //battery switchover flag (write 0 to clear)
    #define PCA2129_CTRL3_BTSE 0x10 //enable timestamp on battery switchover
    #define PCA2129_CTRL3_PWRMNG 0xE0 //battery info

#define PCA2129_SECS   0x03
    #define PCA2129_SECS_MASK 0x7F //Binary coded digits
    #define PCA2129_SECS_OSF  0x80 //oscillator stop flag (write 0 to clear)
#define PCA2129_MINS   0x04 //Binary coded digits
    #define PCA2129_MINS_MASK 0x7F
#define PCA2129_HOURS  0x05
    #define PCA2129_HOURS_MASK24  0x3F //Binary coded digits
    #define PCA2129_HOURS_MASK12  0x1F //Binary coded digits
    #define PCA2129_HOURS_AMPM    0x20 //0=AM, 1=PM
#define PCA2129_DAYS   0x06
    #define PCA2129_DAYS_MASK 0x3F //Binary coded digits
#define PCA2129_WDAYS  0x07
    #define PCA2129_WDAYS_MASK 0x07 //000=Sunday ... 110=Saturday
#define PCA2129_MONTHS 0x08
    #define PCA2129_MONTHS_MASK 0x1F //Binary coded digits
#define PCA2129_YEARS  0x09 //BCD, 0-99

#define PCA2129_ALARM_SEC  0x0A
    #define PCA2129_ALARM_SEC_DIS 0x80 //disable second alarm (0=enabled)
    #define PCA2129_ALARM_SEC_VAL 0x7A //alarm value
#define PCA2129_ALARM_MIN  0x0B
    #define PCA2129_ALARM_MIN_DIS 0x80 //disable minute alarm (0=enabled)
    #define PCA2129_ALARM_MIN_VAL 0x7F //alarm value
#define PCA2129_ALARM_HOUR 0x0C
    #define PCA2129_ALARM_HOUR_DIS 0x80 //disable hour alarm (0=enabled)
    #define PCA2129_ALARM_HOUR_VAL 0x3F //alarm value (when 12h mode then MSB=AM/PM)
#define PCA2129_ALARM_DAY  0x0D
    #define PCA2129_ALARM_DAY_DIS 0x80 //disable day alarm (0=enabled)
    #define PCA2129_ALARM_DAY_VAL 0x3F //alarm value
#define PCA2129_ALARM_WDAY 0x0E
    #define PCA2129_ALARM_WDAY_DIS 0x80 //disable weekday alarm (0=enabled)
    #define PCA2129_ALARM_WDAY_VAL 0x07 //alarm value

#define PCA2129_CKOUT_CTRL  0x0F
    #define PCA2129_CKOUT_TCR  0xC0 //temperature measurement period (def 4 min)
    #define PCA2129_CKOUT_OTPR 0x20 //OTP refresh: on start and after reset (OSF=1) write 0 then 1
//  #define PCA2129_CKOUT_COF  0x07 //clockout frequency (unconnected)
#define PCA2129_WD_TIM_CTRL 0x10
    #define PCA2129_WD_TIM_CTRL_EN  0x80 //enable watchdog timer
    #define PCA2129_WD_TIM_CTRL_PI  0x20 //set pulsed interrupt (vs level)
    #define PCA2129_WD_TIM_CTRL_CLK 0x03 //WD clock source (0:3) => 4.096Hz; 64Hz; 1Hz; 1/60Hz
#define PCA2129_WD_TIM_VAL  0x11 //timer period (clock cycles). when written, WD starts (write 0 to stop)
//timestamp pin left floating: timestamp registers record only last battery switchover (when BTSE=1)
#define PCA2129_TSTAMP_CTRL 0x12
#define PCA2129_TSTAMP_SEC  0x13
#define PCA2129_TSTAMP_MIN  0x14
#define PCA2129_TSTAMP_HOUR 0x15
#define PCA2129_TSTAMP_DAY  0x16
#define PCA2129_TSTAMP_MON  0x17
#define PCA2129_TSTAMP_YEAR 0x18
#define PCA2129_AGE_OFFSET  0x19

#define WDAY (const char*[7]){"Sunday ", "Monday ", "Tuesday ", "Wednesday " "Thursday " "Friday " "Saturday "}


extern i2c_status_t core_i2c_dowrite(
        i2c_instance_t *i2c_inst, uint8_t serial_addr,
        uint8_t * tx_buffer, uint8_t write_length,
        const uint8_t *msg);

i2c_status_t core_i2c_doread_nors(
        i2c_instance_t * i2c_inst, uint8_t serial_addr,
        uint8_t * tx_buffer, uint8_t write_length,
        uint8_t * rx_buffer, uint8_t read_length,
        const uint8_t *msg);

void pca2129_check_osc(void* i2c, uint8_t i2c_addr);
void pca2129_disable_alarms(void* i2c, uint8_t i2c_addr);
void pca2129_write_ctrl_reg(void* i2c, uint8_t i2c_addr, uint8_t reg, uint8_t val);
uint32_t pca2129_read_ctrl_status(void* i2c, uint8_t i2c_addr);
void pca2129_set_clock(void* i2c, uint8_t i2c_addr, uint8_t wd, char* hour, char* date);
uint32_t pca2129_get_time(void* i2c, uint8_t i2c_addr, uint8_t display);
uint32_t pca2129_get_date(void* i2c, uint8_t i2c_addr, uint8_t display);

#endif /* PCA2129_H_ */
