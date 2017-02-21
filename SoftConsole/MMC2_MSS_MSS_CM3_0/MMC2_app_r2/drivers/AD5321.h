/*
 * AD5321.h
 *
 *  Created on: 17.07.2015
 *      Author: malatesta_a
 */

#ifndef AD5321_H_
#define AD5321_H_

#include "hal.h"
#include "core_i2c.h"
#include "mss_uart.h"

/*
 * Input register is 16 bit wide: 4 control bits + 12 bits of data
 * x.x.PD1.PD0.DATA(12)
 * Following values are the whole 16bit register for different control combinations. Data bits are set to 0.
 */
#define AD5321_PD_NO   0x0000 //no powerdown
#define AD5321_PD_1K   0x1000 //powerdown 1Kohm load to ground
#define AD5321_PD_100K 0x2000 //powerdown 100Kohm load to ground
#define AD5321_PD_TRI  0x3000 //powerdown tristate output

#define AD5321_VMASK  0x0FFF //mask for the DAC set value

void AD5321_set_level(void *i2c, uint8_t addr, uint16_t val);
uint16_t AD5321_get_level(void *i2c, uint8_t addr);

extern i2c_status_t core_i2c_dowrite(
		i2c_instance_t *i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		const uint8_t *msg);
extern i2c_status_t core_i2c_doread(
		i2c_instance_t * i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		uint8_t * rx_buffer, uint8_t read_length,
		const uint8_t *msg);

#endif /* AD5321_H_ */
