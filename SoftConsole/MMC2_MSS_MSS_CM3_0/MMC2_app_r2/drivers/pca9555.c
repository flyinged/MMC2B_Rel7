/*
 * pca9555.c
 *
 *  Created on: 30.09.2015
 *      Author: malatesta_a
 */

#include "pca9555.h"
#include "core_i2c.h"

extern i2c_status_t core_i2c_dowrite(
        i2c_instance_t * i2c_inst,
        uint8_t serial_addr,
        uint8_t * tx_buffer,
        uint8_t write_length,
        const uint8_t *msg );

extern i2c_status_t core_i2c_doread(
        i2c_instance_t * i2c_inst, uint8_t serial_addr,
        uint8_t * tx_buffer, uint8_t write_length,
        uint8_t * rx_buffer, uint8_t read_length,
        const uint8_t *msg);

uint16_t pca9555_read(void* i2c, uint8_t i2c_addr, const uint8_t* msg) {
    uint8_t tx_buf[1], rx_buf[2];

    tx_buf[0] = PCA9555_IN; //register address
    rx_buf[0] = 0x00;
    rx_buf[1] = 0x00;

    core_i2c_doread( i2c, i2c_addr, tx_buf, 1, rx_buf, 2, msg);

    return ((rx_buf[1] << 8) | rx_buf[0]);
}


uint8_t pca9555_write(void* i2c, uint8_t i2c_addr, uint16_t data, const uint8_t* msg) {
    uint8_t tx_buf[3];

    tx_buf[0] = PCA9555_OUT;
    tx_buf[1] = (data) & 0xFF;
    tx_buf[2] = (data >> 8) & 0xFF;

    return(core_i2c_dowrite( i2c, i2c_addr, tx_buf, 3, msg));

}
