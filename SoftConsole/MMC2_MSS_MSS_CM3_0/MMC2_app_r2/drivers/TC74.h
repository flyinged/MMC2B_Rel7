/*
 * TC74.h
 *
 *  Created on: 17.07.2015
 *      Author: malatesta_a
 */

#ifndef TC74_H_
#define TC74_H_

#include "hal.h"
#include "core_i2c.h"

#define TC74_CMD_RTR  0x00 //read temperature
#define TC74_CMD_RWCR 0x01 //read-write config register

#define TC74_CFG_STDBY 0x80 //RW, 1=stand-by
#define TC74_CFG_STDBY 0x80 //RW, 1=stand-by
#define TC74_CFG_DRDY  0x40 //RO, data ready

int8_t  TC74_get_temp(void *i2c, uint8_t addr);
uint8_t TC74_meas_to_str(int8_t meas, uint8_t *str);

extern void uint_to_decstr(uint32_t num, uint8_t *str, uint8_t ndigits);
extern i2c_status_t core_i2c_dowrite(
		i2c_instance_t *i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		const uint8_t *msg);
extern i2c_status_t core_i2c_doread(
		i2c_instance_t * i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		uint8_t * rx_buffer, uint8_t read_length,
		const uint8_t *msg);

#endif /* TC74_H_ */
