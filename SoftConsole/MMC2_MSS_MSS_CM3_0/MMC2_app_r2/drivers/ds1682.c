/*
 * ds1682.c
 *
 *  Created on: 30.09.2015
 *      Author: malatesta_a
 */

#include "ds1682.h"
#include "core_i2c.h"


void wait_tew(void);

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

void wait_tew(void) {
    uint32_t i;

    //each instuction needs approximately 2 clock cycles (increment+compare)
    for (i=0; i<(DS1682_CPU_FREQ*DS1682_TEW/2); i++);
}

/* write Elapsed Time Counter (reset) */
uint8_t ds1682_reset_etc(void* i2c, uint8_t i2c_addr) {

    uint8_t tx_buf[5];

    tx_buf[0] = DS1682_ETC;
    tx_buf[1] = 0x00;
    tx_buf[2] = 0x00;
    tx_buf[3] = 0x00;
    tx_buf[4] = 0x00;

    return(core_i2c_dowrite(i2c, i2c_addr, tx_buf, 5, (const uint8_t *) "ds1682_etc_reset\n\r"));
}

/* write Event Counter (reset) */
uint8_t ds1682_reset_evc(void* i2c, uint8_t i2c_addr) {

    uint8_t tx_buf[3];

    tx_buf[0] = DS1682_EVC;
    tx_buf[1] = 0x00;
    tx_buf[2] = 0x00;

    return(core_i2c_dowrite(i2c, i2c_addr, tx_buf, 3, (const uint8_t *) "ds1682_evc_reset\n\r"));
}


/* write Event Counter (reset) */
uint8_t ds1682_get_cfg_reg(void* i2c, uint8_t i2c_addr) {

    uint8_t tx_buf[1], rx_buf[1];

    tx_buf[0] = DS1682_CFG;
    rx_buf[0] = 0xFF;
    core_i2c_doread(i2c, i2c_addr, tx_buf, 1, rx_buf, 1, (const uint8_t *) "ds1682_get_cfg_reg\n\r");

    return(rx_buf[0]);
}


/* send reset command */
uint8_t ds1682_reset(void* i2c, uint8_t i2c_addr) {
    uint8_t tx_buf[3], retval;

    tx_buf[0] = DS1682_RST;
    tx_buf[1] = 0x55;
    tx_buf[2] = 0x55;
    retval = core_i2c_dowrite(i2c, i2c_addr, tx_buf, 3, (const uint8_t *) "ds1682_reset\n\r");

    wait_tew();

    return(retval);
}

/* write 0 to disable alarm. when ETC reaches alarm register value, ALARM# pin is asserted */
uint8_t ds1682_set_alarm_reg(void* i2c, uint8_t i2c_addr, uint32_t val) {
    uint8_t tx_buf[5];

    /* write alarm register */
    tx_buf[0] = DS1682_ALR;
    tx_buf[1] = (val)     & 0xFF;
    tx_buf[2] = (val>>8)  & 0xFF;
    tx_buf[3] = (val>>16) & 0xFF;
    tx_buf[4] = (val>>24) & 0xFF;
    return(core_i2c_dowrite(i2c, i2c_addr, tx_buf, 5, (const uint8_t *) "ds1682_write_alarm_reg\n\r"));
}


uint32_t ds1682_get_alarm_reg(void* i2c, uint8_t i2c_addr) {
    uint8_t tx_buf[1], rx_buf[4], i;

    tx_buf[0] = DS1682_ALR;
    for (i=0; i<4; i++) {
        rx_buf[i] = 0x00; //reset rx buffer
    }
    core_i2c_doread(i2c, i2c_addr, tx_buf, 1, rx_buf, 4, (const uint8_t *) "ds1682_read_alarm_reg\n\r");
    return ( (rx_buf[3]<<24) | (rx_buf[2]<<16) | (rx_buf[1]<<8) | rx_buf[0] );
}


/* read & display elapsed time counter */
uint32_t ds1682_get_etc(void* i2c, uint8_t i2c_addr) {
    uint8_t tx_buf[1], rx_buf[4], i;

    tx_buf[0] = DS1682_ETC;
    for (i=0; i<4; i++) {
        rx_buf[i] = 0x00; //reset rx buffer
    }
    core_i2c_doread(i2c, i2c_addr, tx_buf, 1, rx_buf, 4, (const uint8_t *) "ds1682_read_etc\n\r");
    return( (rx_buf[3]<<24) | (rx_buf[2]<<16) | (rx_buf[1]<<8) | rx_buf[0]);
}

uint32_t ds1682_get_evc(void* i2c, uint8_t i2c_addr) {
    uint8_t tx_buf[1], rx_buf[2], msb;

    msb = ds1682_get_cfg_reg(i2c, i2c_addr);

    tx_buf[0] = DS1682_EVC;
    rx_buf[0] = 0x00; //reset rx buffer
    rx_buf[1] = 0x00; //reset rx buffer

    core_i2c_doread(i2c, i2c_addr, tx_buf, 1, rx_buf, 2, (const uint8_t *) "ds1682_get_evc\n\r");
    return ( (msb<<16) | (rx_buf[1]<<8) | rx_buf[0] );
}
