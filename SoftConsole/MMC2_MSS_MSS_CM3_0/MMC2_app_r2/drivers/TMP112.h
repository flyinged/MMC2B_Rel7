/*
 * TMP112.h
 *
 *  Created on: 17.07.2015
 *      Author: malatesta_a
 */

#ifndef TMP112_H_
#define TMP112_H_

#include "hal.h"
#include "mss_i2c.h"
#include "core_i2c.h"

/* TMP112, I2C Temperature sensor */
#define TMP112_SH12 4 //right shift to adjust 12bit temperature register
#define TMP112_SH13 3 //right shift to adjust 13bit temperature register
#define TMP112_TMP_REG 0x00
#define TMP112_CFG_REG 0x01
#define TMP112_TLO_REG 0x02
#define TMP112_THI_REG 0x03

#define TMP112_CFG_OS   0x8000 //write 1 to start single shot (SD must be 1)
#define TMP112_CFG_R10  0x6000 //RO: "11"=12 bit resolution
#define TMP112_CFG_F10  0x1800 //number of alarms for fault (1,2,3,4 => 1,2,4,6)
#define TMP112_CFG_POL  0x0400 //0= alarm active low, 1=alarm active high
#define TMP112_CFG_TM   0x0200 //0=out of range alarm, 1=over-max/below-min interrupt
#define TMP112_CFG_SD   0x0100 //shutdown (0=continuous)
#define TMP112_CFG_CR10 0x00C0 //conversions per second (0,1,2,3 => 0.25,1,4,8)
#define TMP112_CFG_AL   0x0020 //alarm (over max/below min, independent of TM). Normally active LOW
#define TMP112_CFG_EM   0x0010 //enable 13 bit mode

extern i2c_status_t core_i2c_dowrite(
		i2c_instance_t *i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		const uint8_t *msg);
extern i2c_status_t core_i2c_doread(
		i2c_instance_t * i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		uint8_t * rx_buffer, uint8_t read_length,
		const uint8_t *msg);

extern mss_i2c_status_t mss_i2c_dowrite(
		mss_i2c_instance_t *i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		const uint8_t *msg);

extern mss_i2c_status_t mss_i2c_doread(
		mss_i2c_instance_t * i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		uint8_t * rx_buffer, uint8_t read_length,
		const uint8_t *msg);

extern void uint_to_decstr(uint32_t num, uint8_t *str, uint8_t ndigits);

int16_t T112_get_temp(void *i2c, uint8_t is_mss_i2c, uint8_t addr, uint8_t is_13bit, const uint8_t *msg);
uint8_t T112_meas_to_str(int16_t meas, uint8_t *str);
uint16_t T112_rmw(void *i2c, uint8_t is_mss_i2c, uint8_t addr, uint16_t mask, uint16_t val);
void T112_set_hi_limit(void *i2c, uint8_t is_mss_i2c, uint8_t addr, uint16_t value );
void T112_set_lo_limit(void *i2c, uint8_t is_mss_i2c, uint8_t addr, uint16_t value );
uint16_t T112_get_hi_limit(void *i2c, uint8_t is_mss_i2c, uint8_t addr);
uint16_t T112_get_lo_limit(void *i2c, uint8_t is_mss_i2c, uint8_t addr);


#endif /* TMP112_H_ */
