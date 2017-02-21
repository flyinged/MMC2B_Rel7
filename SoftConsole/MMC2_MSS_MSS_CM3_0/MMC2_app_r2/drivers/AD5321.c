/*
 * AD5321.c
 *
 *  Created on: 17.07.2015
 *      Author: malatesta_a
 */

#include "AD5321.h"
#include "mss_watchdog.h"

void AD5321_set_level(void *i2c, uint8_t addr, uint16_t val) {
	uint8_t tx_buf[2];
	uint16_t reg;

	reg = AD5321_PD_NO | (AD5321_VMASK & val);
	tx_buf[0] = (reg & 0xFF00) >> 8;
	tx_buf[1] = (reg & 0x00FF);

	core_i2c_dowrite( (i2c_instance_t*) i2c, addr, tx_buf, 2, (const uint8_t *) "Write to Heater DAC: " );

	return;
}

uint16_t AD5321_get_level(void *i2c, uint8_t addr) {
	uint8_t rx_buf[2];

	rx_buf[0] = 0x00; //reset rx buffer
	rx_buf[1] = 0x00; //reset rx buffer

	//core_i2c_doread( (i2c_instance_t*) i2c, addr, 0, 0, rx_buf, 2, (const uint8_t *) "Read from Heater DAC: " );

	i2c_status_t status;
	I2C_read((i2c_instance_t*) i2c, addr, rx_buf, 2, I2C_RELEASE_BUS);
	status = I2C_wait_complete((i2c_instance_t*) i2c, 3000u);

	    if (status == I2C_FAILED) {
	    	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
	    	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "AD5321_get_level: I2C read transaction failed\n\r");
	    }
	    if (status == I2C_TIMED_OUT) {
	    	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
	    	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "AD5321_get_level: I2C read transaction timed-out\n\r");
	        MSS_WD_reload();
	    }

	return (((rx_buf[0]<<8) | rx_buf[1]) & AD5321_VMASK);

}
