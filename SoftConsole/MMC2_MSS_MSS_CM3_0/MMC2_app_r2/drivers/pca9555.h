/*
 * pca9555.h
 *
 *  Created on: 30.09.2015
 *      Author: malatesta_a
 */

#ifndef PCA9555_H_
#define PCA9555_H_

#include "hal.h"

/* PCA9555, I2C GPIO */
/* define registers (Register Pairs. Only address of first one.) */
#define PCA9555_IN   0x0
#define PCA9555_OUT  0x2
#define PCA9555_POL  0x4
#define PCA9555_CONF 0x6

uint16_t pca9555_read(void* i2c, uint8_t i2c_addr, const uint8_t* msg);
uint8_t  pca9555_write(void* i2c, uint8_t i2c_addr, uint16_t data, const uint8_t* msg);

#endif /* PCA9555_H_ */
