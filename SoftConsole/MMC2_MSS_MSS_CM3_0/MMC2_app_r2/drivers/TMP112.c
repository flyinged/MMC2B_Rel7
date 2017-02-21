/*
 * TMP112.c
 *
 *  Created on: 17.07.2015
 *      Author: malatesta_a
 */

#include "TMP112.h"

int16_t T112_get_temp(void *i2c, uint8_t is_mss_i2c, uint8_t addr, uint8_t is_13bit, const uint8_t *msg ) {
	int16_t temperature;
	uint8_t tx_buf[1], rx_buf[2];

	tx_buf[0] = TMP112_TMP_REG;
	rx_buf[0] = 0x00; //reset rx buffer
	rx_buf[1] = 0x00; //reset rx buffer

	if (is_mss_i2c) {
		mss_i2c_doread( (mss_i2c_instance_t*) i2c, addr, tx_buf, 1, rx_buf, 2, (const uint8_t *) msg );
	} else {
		core_i2c_doread( (i2c_instance_t*) i2c, addr, tx_buf, 1, rx_buf, 2, (const uint8_t *) msg );
	}
	temperature = (rx_buf[0]<<8) | rx_buf[1];
	if (is_13bit) {
		temperature >>= TMP112_SH13;
	} else {
		temperature >>= TMP112_SH12;
	}

	return temperature;
}


uint8_t T112_meas_to_str(int16_t meas, uint8_t *str) {
	uint16_t buf;

	if (meas >= 0) {
		buf = meas;
		str[0] = '+';
	} else {
		buf = -meas;
		str[0] = '-';
	}

	uint_to_decstr((buf>>4), str+1, 3); //3 integer part digits
	str[4] = '.';
	uint_to_decstr( (1000*(buf&0xF))>>4, str+5, 3); //3 fractional part digits

	return 8;
}


/* read modify write on command register
 * mask defines bits to be changed (1), val specified the desired value
 * the function returns the current value of the status register.
 * If called with mask 0x0, no change is performed
 */
uint16_t T112_rmw(void *i2c, uint8_t is_mss_i2c, uint8_t addr, uint16_t mask, uint16_t val) {
	uint16_t reg;
	uint8_t tx_buf[3], rx_buf[2];

	tx_buf[0] = TMP112_CFG_REG;
	rx_buf[0] = 0x00; //reset rx buffer
	rx_buf[1] = 0x00; //reset rx buffer

	if (is_mss_i2c) {
		mss_i2c_doread( (mss_i2c_instance_t*) i2c, addr, tx_buf, 1, rx_buf, 2, (const uint8_t *) "T112_rmw: " );
	} else {
		core_i2c_doread( (i2c_instance_t*) i2c, addr, tx_buf, 1, rx_buf, 2, (const uint8_t *) "T112_rmw: " );
	}
	reg = (rx_buf[0]<<8) | rx_buf[1];
	reg &= ~mask; //reset bits to be changed
	reg |= (val & mask); //update register
	tx_buf[1] = (reg & 0xFF00) >> 8;
	tx_buf[2] = (reg & 0x00FF);
	if (is_mss_i2c) {
		mss_i2c_dowrite( (mss_i2c_instance_t*) i2c, addr, tx_buf, 3, (const uint8_t *) "T112_rmw: " );
	} else {
		core_i2c_dowrite( (i2c_instance_t*) i2c, addr, tx_buf, 3, (const uint8_t *) "T112_rmw: " );
	}

	return reg;
}


void T112_set_hi_limit(void *i2c, uint8_t is_mss_i2c, uint8_t addr, uint16_t value ) {

	uint8_t tx_buf[3];

	tx_buf[0] = TMP112_THI_REG;
	tx_buf[1] = (value & 0xFF00) >> 8;
	tx_buf[2] = (value & 0x00FF);

	if (is_mss_i2c) {
		mss_i2c_dowrite( (mss_i2c_instance_t*) i2c, addr, tx_buf, 3, (const uint8_t *) "T112_set_hi_limit: " );
	} else {
		core_i2c_dowrite( (i2c_instance_t*) i2c, addr, tx_buf, 3, (const uint8_t *) "T112_set_hi_limit: " );
	}

	return;
}

void T112_set_lo_limit(void *i2c, uint8_t is_mss_i2c, uint8_t addr, uint16_t value ) {

	uint8_t tx_buf[3];

	tx_buf[0] = TMP112_TLO_REG;
	tx_buf[1] = (value & 0xFF00) >> 8;
	tx_buf[2] = (value & 0x00FF);

	if (is_mss_i2c) {
		mss_i2c_dowrite( (mss_i2c_instance_t*) i2c, addr, tx_buf, 3, (const uint8_t *) "T112_set_lo_limit: " );
	} else {
		core_i2c_dowrite( (i2c_instance_t*) i2c, addr, tx_buf, 3, (const uint8_t *) "T112_set_lo_limit: " );
	}

	return;
}

uint16_t T112_get_hi_limit(void *i2c, uint8_t is_mss_i2c, uint8_t addr ) {

	uint8_t tx_buf[1], rx_buf[2];

	tx_buf[0] = TMP112_THI_REG;
	rx_buf[1] = 0x0;
	rx_buf[2] = 0x0;

	if (is_mss_i2c) {
		mss_i2c_doread( (mss_i2c_instance_t*) i2c, addr, tx_buf, 1, rx_buf, 2, (const uint8_t *) "T112_get_hi_limit: " );
	} else {
		core_i2c_doread( (i2c_instance_t*) i2c, addr, tx_buf, 1, rx_buf, 2, (const uint8_t *) "T112_get_hi_limit: " );
	}

	return ( (rx_buf[0]<<8) | rx_buf[1] );
}

uint16_t T112_get_lo_limit(void *i2c, uint8_t is_mss_i2c, uint8_t addr ) {

	uint8_t tx_buf[1], rx_buf[2];

	tx_buf[0] = TMP112_TLO_REG;
	rx_buf[1] = 0x0;
	rx_buf[2] = 0x0;

	if (is_mss_i2c) {
		mss_i2c_doread( (mss_i2c_instance_t*) i2c, addr, tx_buf, 1, rx_buf, 2, (const uint8_t *) "T112_get_lo_limit: " );
	} else {
		core_i2c_doread( (i2c_instance_t*) i2c, addr, tx_buf, 1, rx_buf, 2, (const uint8_t *) "T112_get_lo_limit: " );
	}

	return ( (rx_buf[0]<<8) | rx_buf[1] );
}
