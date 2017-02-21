/*
 * i2c_eeprom.h
 *
 *  Created on: 30.09.2015
 *      Author: malatesta_a
 */

#ifndef I2C_EEPROM_H_
#define I2C_EEPROM_H_

/* 24AA025E48, I2C 2K EEPROM with EUI48 support */
#define EEPROM_PAGE_SIZE 16
#define EEPROM_EUI48_ADR 0xFA //write protected from 0x80 to 0xFF
#define EEPROM_LAST_ADR 0x7F

#include "hal.h"

uint8_t i2c_eeprom_read_eui48(void* i2c, uint8_t i2c_addr, uint8_t *rx_buf, const uint8_t *msg);
uint8_t i2c_eeprom_write(void* i2c, uint8_t i2c_addr, uint8_t address, uint8_t *data, uint8_t size, const uint8_t *msg);
uint8_t i2c_eeprom_read(void* i2c, uint8_t i2c_addr, uint8_t address, uint8_t *data, uint8_t size, const uint8_t *msg);

#endif /* I2C_EEPROM_H_ */
