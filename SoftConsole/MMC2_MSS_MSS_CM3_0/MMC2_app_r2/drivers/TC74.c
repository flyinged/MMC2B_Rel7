/*
 * TC74.c
 *
 *  Created on: 17.07.2015
 *      Author: malatesta_a
 */

#include "TC74.h"

int8_t TC74_get_temp(void *i2c, uint8_t addr) {
	uint8_t tx_buf[1], rx_buf[1];

	tx_buf[0] = TC74_CMD_RTR;
	rx_buf[0] = 0x00; //reset rx buffer

	core_i2c_doread( (i2c_instance_t*) i2c, addr, tx_buf, 1, rx_buf, 1, (const uint8_t *) "Read from I2C TEMP SENSOR: " );

	return (int8_t) rx_buf[0];

}

uint8_t TC74_meas_to_str(int8_t meas, uint8_t *str) {
	int8_t buf;

	if (meas >= 0) {
		buf = meas;
		str[0] = '+';
	} else {
		buf = -meas;
		str[0] = '-';
	}

	uint_to_decstr( (uint32_t) buf, str+1, 3); //3 integer part digits

	return 4;
}
