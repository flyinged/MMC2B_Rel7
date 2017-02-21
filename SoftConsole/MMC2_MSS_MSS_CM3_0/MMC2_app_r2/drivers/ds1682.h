/*
 * ds1682.h
 *
 *  Created on: 30.09.2015
 *      Author: malatesta_a
 */

#ifndef DS1682_H_
#define DS1682_H_

#include "hal.h"

#define DS1682_CPU_FREQ 100000000
#define DS1682_TEW 0.3 //300 ms, eeprom write time

/* DS1682 - I2C Elapsed time counter (LSB always at lowest address)*/
#define DS1682_LSB_SEC   0.25 //value of ETC LSB in seconds
#define DS1682_CFG   0x00 //config register
    #define DS1682_CFG_ECMSB 0x01 //event counter MSB
    #define DS1682_CFG_AP    0x02 //Alarm polarity
    #define DS1682_CFG_RE    0x04 //enable reset command
    #define DS1682_CFG_AOS   0x08 //Alarm output select (AF or Event Start/End)
    #define DS1682_CFG_WMDF  0x10 //Write memory disable (set via WMD register). Protect User Mem
    #define DS1682_CFG_WDF   0x20 //Write disable (set via WD register). Protect ALR,ETC,EVC
    #define DS1682_CFG_AF    0x40 //Alarm flag (ETC=ALR)
#define DS1682_ALR 0x01 //Alarm register, lower of 4 bytes
#define DS1682_ETC 0x05 //Elapsed time counter, lower of 4 bytes
#define DS1682_EVC 0x09 //Event counter, lower of 2 bytes
#define DS1682_USR 0x0B //User memory, lower of 10 bytes
#define DS1682_RST 0x1D //Reset command: write 0x55 twice (resets ETC,AF,RE,ECMSB)
#define DS1682_WD  0x1E //Write disable: write 0xAA twice (sets WDF)
#define DS1682_WMD 0x1F //Memory disable: write 0xF0 twice (sets WMDF)

uint8_t  ds1682_reset_etc(void* i2c, uint8_t i2c_addr);
uint8_t  ds1682_reset_evc(void* i2c, uint8_t i2c_addr);
uint8_t  ds1682_get_cfg_reg(void* i2c, uint8_t i2c_addr);
uint8_t  ds1682_set_alarm_reg(void* i2c, uint8_t i2c_addr, uint32_t val);
uint32_t ds1682_get_alarm_reg(void* i2c, uint8_t i2c_addr);
uint32_t ds1682_get_etc(void* i2c, uint8_t i2c_addr);
uint32_t ds1682_get_evc(void* i2c, uint8_t i2c_addr);

#endif /* DS1682_H_ */
