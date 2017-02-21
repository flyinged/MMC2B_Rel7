/*
 * i2c_eeprom.c
 *
 *  Created on: 30.09.2015
 *      Author: malatesta_a
 */

#include "i2c_eeprom.h"
#include "mss_i2c.h"
#include "mss_watchdog.h"

extern mss_i2c_status_t mss_i2c_dowrite(
        mss_i2c_instance_t * i2c_inst,
        uint8_t serial_addr,
        uint8_t * tx_buffer,
        uint8_t write_length,
        const uint8_t *msg );

extern mss_i2c_status_t mss_i2c_doread(
        mss_i2c_instance_t * i2c_inst,
        uint8_t serial_addr,
        uint8_t * tx_buffer, uint8_t write_length,
        uint8_t * rx_buffer, uint8_t read_length,
        const uint8_t *msg);

uint8_t i2c_eeprom_read_eui48(void* i2c, uint8_t i2c_addr, uint8_t *rx_buf, const uint8_t *msg) {

    uint8_t tx_buf[1];

    tx_buf[0] = EEPROM_EUI48_ADR;

    return(mss_i2c_doread( i2c, i2c_addr, tx_buf, 1, rx_buf, 6, msg));

}

uint8_t i2c_eeprom_write(void* i2c, uint8_t i2c_addr, uint8_t address, uint8_t *data, uint8_t size, const uint8_t *msg) {

    if (size>EEPROM_PAGE_SIZE) return(1);

    mss_i2c_status_t i2c_status;
    uint8_t tx_buf[17], i;

    tx_buf[0] = address;
    for (i=0; i<size; i++) {
        tx_buf[i+1] = data[i];
    }

    mss_i2c_dowrite( i2c, i2c_addr, tx_buf, size+1, msg );

    /* 3: do ack polling */
    i2c_status = MSS_I2C_FAILED;
    i=0;
    while ((i2c_status != MSS_I2C_SUCCESS) && (i<3)) {
        MSS_WD_reload();
        //not using mss_i2c_dowrite to avoid warnings (expecting timeouts)
        MSS_I2C_write(i2c, i2c_addr, tx_buf, 1, MSS_I2C_RELEASE_BUS);
        i2c_status = MSS_I2C_wait_complete(i2c, 3000u);
        i++;
    }

    return(0);
}

uint8_t i2c_eeprom_read(void* i2c, uint8_t i2c_addr, uint8_t address, uint8_t *data, uint8_t size, const uint8_t *msg) {

    if (size>EEPROM_PAGE_SIZE) return(1);

    uint8_t tx_buf[1], i;

    tx_buf[0] = address;
    for (i=0; i<size; i++) {
        data[i] = 0x00; //reset rx buffer
    }

    return(mss_i2c_doread( i2c, i2c_addr, tx_buf, 1, data, size, msg));
}
