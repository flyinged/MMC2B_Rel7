/*
 * LTC2945.c
 *
 *  Created on: 17.07.2015
 *      Author: malatesta_a
 */

#include "LTC2945.h"
#include "core_i2c.h"
#include "string.h"
#include "mss_uart.h"
#include "uart_utility.h"

extern i2c_instance_t g_core_i2c_pwr;
//extern void uint_to_hexstr(uint32_t num, uint8_t *str, uint8_t ndigits);
//extern uint8_t float_to_string(float num, uint8_t *str, uint8_t nd_int, uint8_t nd_frac);

extern volatile uint32_t tick_counter;
static uint32_t t_last_err;

#define LTC2945_ERRCNT_MAX 5
uint8_t LTC2945_errcnt = 0;

uint32_t LTC2945_read_reg(void *i2c, uint8_t i2c_addr, uint8_t reg_addr, uint8_t nbytes) {
	uint8_t rx_buf[3], tx_buf[1];
    i2c_status_t status;

	tx_buf[0] = reg_addr;
	rx_buf[0] = 0x00; //reset rx buffer
	rx_buf[1] = 0x00; //reset rx buffer
	rx_buf[2] = 0x00; //reset rx buffer

	if (nbytes == 0 || nbytes > 3) {
		return 0;
	}

	if (LTC2945_errcnt > LTC2945_ERRCNT_MAX) {
	    return 0;
	}

	status = core_i2c_doread( (i2c_instance_t*) i2c, i2c_addr, tx_buf, 1, rx_buf, nbytes, (const uint8_t *) "Read from Power monitor: " );

	if (status != I2C_SUCCESS) {
	    LTC2945_errcnt++;
	    t_last_err = tick_counter;
	} else if ((tick_counter - t_last_err) > 6000) {
	    LTC2945_errcnt = 0;
	}

	if (nbytes == 1) {
//		tx_buf[0] = reg_addr;
//		core_i2c_doread( (i2c_instance_t*) i2c, i2c_addr, tx_buf, 1, rx_buf, 1, (const uint8_t *) "Read from Power monitor: " );
		return ( rx_buf[0] );
	} else if (nbytes == 2) {
//		tx_buf[0] = reg_addr;
//		core_i2c_doread( (i2c_instance_t*) i2c, i2c_addr, tx_buf, 1, rx_buf, 1, (const uint8_t *) "Read from Power monitor: " );
//		tx_buf[0] = reg_addr+1;
//		core_i2c_doread( (i2c_instance_t*) i2c, i2c_addr, tx_buf, 1, rx_buf+1, 1, (const uint8_t *) "Read from Power monitor: " );
		return ( (rx_buf[0]<<8) | (rx_buf[1]) );
	} else {
//		tx_buf[0] = reg_addr;
//		core_i2c_doread( (i2c_instance_t*) i2c, i2c_addr, tx_buf, 1, rx_buf, 1, (const uint8_t *) "Read from Power monitor: " );
//		tx_buf[0] = reg_addr+1;
//		core_i2c_doread( (i2c_instance_t*) i2c, i2c_addr, tx_buf, 1, rx_buf+1, 1, (const uint8_t *) "Read from Power monitor: " );
//		tx_buf[0] = reg_addr+2;
//		core_i2c_doread( (i2c_instance_t*) i2c, i2c_addr, tx_buf, 1, rx_buf+2, 1, (const uint8_t *) "Read from Power monitor: " );
		return ( (rx_buf[0]<<16) | (rx_buf[1]<<8) | rx_buf[2] );
	}
}


void LTC2945_write_reg(void *i2c, uint8_t i2c_addr, uint8_t reg_addr, uint32_t val, uint8_t nbytes) {
	uint8_t tx_buf[4];

	tx_buf[0] = reg_addr;

	switch (nbytes) {
	case 1:
		tx_buf[1] = (val & 0xFF);
//		core_i2c_dowrite( (i2c_instance_t*) i2c, i2c_addr, tx_buf, 1, (const uint8_t *) "Write0 to Power monitor: " );
		break;
	case 2:
		tx_buf[1] = (val & 0xFF00) >> 8;
		tx_buf[2] = (val & 0xFF);
//		core_i2c_dowrite( (i2c_instance_t*) i2c, i2c_addr, tx_buf, 1, (const uint8_t *) "Write0 to Power monitor: " );
//		core_i2c_dowrite( (i2c_instance_t*) i2c, i2c_addr, tx_buf+1, 1, (const uint8_t *) "Write1 to Power monitor: " );
		break;
	case 3:
		tx_buf[1] = (val & 0xFF0000) >> 16;
		tx_buf[2] = (val & 0xFF00) >> 8;
		tx_buf[3] = (val & 0xFF);
//		core_i2c_dowrite( (i2c_instance_t*) i2c, i2c_addr, tx_buf, 1, (const uint8_t *) "Write0 to Power monitor: " );
//		core_i2c_dowrite( (i2c_instance_t*) i2c, i2c_addr, tx_buf+1, 1, (const uint8_t *) "Write1 to Power monitor: " );
//		core_i2c_dowrite( (i2c_instance_t*) i2c, i2c_addr, tx_buf+2, 1, (const uint8_t *) "Write2 to Power monitor: " );
		break;
	default:
		return;
	}

	core_i2c_dowrite( (i2c_instance_t*) i2c, i2c_addr, tx_buf, nbytes+1, (const uint8_t *) "Write to Power monitor: " );

	return;
}


/* RAW_POWER(24 bit) = Vsense * VDD / Rshunt
 * Vsense(12bit) = max 102.4 mV (over 0.003 mOhm resistor)
 * VDD(12bit)    = max 102.4 V
 *
 * PMAX = (102.4) * (102.4 * 1e-3) / (3 * 1e-3) = 3495.25 W
 * PMAX/0xFFFFFF = LSB = 2.08e-4 W
 */
float LTC2945_raw_to_power(uint32_t raw_power, float Rshunt_mohm) {
	float f;

	f   = (float) raw_power;
	f = f*(102.4*102.4)/0xFFFFFF/1000; //[V*V]
	f = f*1000/Rshunt_mohm;

	return (f);
}

//returns value in volts (millivolts if vsense)
//max is 0xFFF.F = 102.4
float LTC2945_raw_to_voltage(uint16_t raw_voltage) {
	float f;

	f = (float) (raw_voltage>>4);
	f = f*102.4/0xFFF;

	return (f);
}

float LTC2945_raw_to_current(uint32_t raw_voltage, float Rshunt_mohm) {
    float f;

    f   = (float) raw_voltage;
    f = f*(102.4/0xFFFF); //voltage
    f = f*1000/Rshunt_mohm; //current (mA if voltage in mV, A if voltage in V)

    return (f);
}

//returns value in volts
//max is 0xFFF = 2.048 V
float LTC2945_raw_to_adin(uint16_t raw_voltage, float mult) {
	float f;

	f = (float) (raw_voltage>>4);
	f = mult*f*2.048/0xFFF;

	return (f);
}

void LTC2945_display(void *i2c_inst, uint32_t I2C_SER_ADDR, uint8_t simple, float SR_mohm, float mult) {
	uint32_t rval;
	uint8_t text_buf[80];

	if (simple == 0) {
	    //control register
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_CTRL, 1);
	    memcpy(text_buf, "CONTROL: 0x00\n\r\0", 16);
	    uint_to_hexstr(rval, text_buf+11, 2);
	    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
	    //ALERT REGISTER
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_ALERT, 1);
	    memcpy(text_buf, "ALERT  : 0x00\n\r\0", 16);
	    uint_to_hexstr(rval, text_buf+11, 2);
	    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
	    //status register
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_STATUS, 1);
	    memcpy(text_buf, "STATUS : 0x00\n\r\0", 16);
	    uint_to_hexstr(rval, text_buf+11, 2);
	    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
	    //fault register
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_FAULT, 1);
	    memcpy(text_buf, "FAULT  : 0x00\n\n\r\0", 17);
	    uint_to_hexstr(rval, text_buf+11, 2);
	    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
	}

	if (simple == 0) {
	    //                          1         2         3         4         5
	    //                0123456789012345678901234567890123456789012345678901234567890 1 2 3
	    memcpy(text_buf, "           | Measure   MAX       MIN       MAX_TH    MIN_TH  \n\r\0", 65);
	} else {
	    memcpy(text_buf, "           | Measure\n\r\0", sizeof("           | Measure\n\r\0"));
	}
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);

    if (simple == 0) {
        //Power           01234567890123456789
        memcpy(text_buf, "POWER (W)  | +0000.00  +0000.00  +0000.00  +0000.00  +0000.00\n\r\0", 65);
    } else {
        memcpy(text_buf, "POWER (W)  | +0000.00\n\r\0", sizeof("POWER (W)  | +0000.00\n\r\0"));
    }
	rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_PWR_MSB2, 3);
	float_to_string(LTC2945_raw_to_power(rval, SR_mohm), text_buf+13, 4, 2);
	if (simple == 0) {
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MAXP_MSB2, 3);
	    float_to_string(LTC2945_raw_to_power(rval, SR_mohm), text_buf+23, 4, 2);
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MINP_MSB2, 3);
	    float_to_string(LTC2945_raw_to_power(rval, SR_mohm), text_buf+33, 4, 2);
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MAXP_TH_MSB2, 3);
	    float_to_string(LTC2945_raw_to_power(rval, SR_mohm), text_buf+43, 4, 2);
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MINP_TH_MSB2, 3);
	    float_to_string(LTC2945_raw_to_power(rval, SR_mohm), text_buf+53, 4, 2);
	}
	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf); //write to console

	if (simple == 0) {
	    //Vsense (differential over 3 mOhm)
	    //                01234567890123456789
	    memcpy(text_buf, "VSENSE (mV)| +000.000  +000.000  +000.000  +000.000  +000.000\n\r\0", 65);
	} else {
        memcpy(text_buf, "VSENSE (mV)| +000.000\n\r\0", sizeof("VSENSE (mV)| +000.000\n\r\0"));
	}
	rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_SENSE_MSB, 2);
	float_to_string(LTC2945_raw_to_voltage(rval), text_buf+13, 3, 3);
	if (simple ==0) {
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MAX_SNS_MSB, 2);
	    float_to_string(LTC2945_raw_to_voltage(rval), text_buf+23, 3, 3);
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MIN_SNS_MSB, 2);
	    float_to_string(LTC2945_raw_to_voltage(rval), text_buf+33, 3, 3);
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MAX_SNS_TH_MSB, 2);
	    float_to_string(LTC2945_raw_to_voltage(rval), text_buf+43, 3, 3);
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MIN_SNS_TH_MSB, 2);
	    float_to_string(LTC2945_raw_to_voltage(rval), text_buf+53, 3, 3);
	}
	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf); //write to console

	if (simple == 0) {
	    //Vin (Vsense+ = P12V in)
	    memcpy(text_buf, "VIN (V)    | +000.000  +000.000  +000.000  +000.000  +000.000\n\r\0", 65);
	} else {
        memcpy(text_buf, "VIN (V)    | +000.000\n\r\0", sizeof("VIN (V)    | +000.000\n\r\0"));
	}
	rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_VIN_MSB, 2);
	float_to_string(LTC2945_raw_to_voltage(rval), text_buf+13, 3, 3);
	if (simple == 0) {
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MAX_VIN_MSB, 2);
	    float_to_string(LTC2945_raw_to_voltage(rval), text_buf+23, 3, 3);
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MIN_VIN_MSB, 2);
	    float_to_string(LTC2945_raw_to_voltage(rval), text_buf+33, 3, 3);
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MAX_VIN_TH_MSB, 2);
	    float_to_string(LTC2945_raw_to_voltage(rval), text_buf+43, 3, 3);
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MIN_VIN_TH_MSB, 2);
	    float_to_string(LTC2945_raw_to_voltage(rval), text_buf+53, 3, 3);
	}
	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf); //write to console

	if (simple == 0) {
	    //ADIN (P12V Out)
	    memcpy(text_buf, "ADIN (V)   | +000.000  +000.000  +000.000  +000.000  +000.000\n\r\0", 65);
	} else {
        memcpy(text_buf, "ADIN (V)   | +000.000\n\r\0", sizeof("ADIN (V)   | +000.000\n\r\0"));
	}
	rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_ADIN_MSB, 2);
	float_to_string(LTC2945_raw_to_adin(rval,mult), text_buf+13, 3, 3);
	if (simple == 0) {
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MAX_ADIN_MSB, 2);
	    float_to_string(LTC2945_raw_to_adin(rval,mult), text_buf+23, 3, 3);
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MIN_ADIN_MSB, 2);
	    float_to_string(LTC2945_raw_to_adin(rval,mult), text_buf+33, 3, 3);
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MAX_ADIN_TH_MSB, 2);
	    float_to_string(LTC2945_raw_to_adin(rval,mult), text_buf+43, 3, 3);
	    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_MIN_ADIN_TH_MSB, 2);
	    float_to_string(LTC2945_raw_to_adin(rval,mult), text_buf+53, 3, 3);
	}
	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf); //write to console

	return;
}

void LTC2945_report(void *i2c_inst, uint32_t I2C_SER_ADDR, float SR_mohm, const uint8_t *head, float mult) {
    uint32_t rval;
    uint8_t text_buf[80];

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) head);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "POWER\n\r ");
    memcpy(text_buf, "+0000.00;W\n\r\0", sizeof("+0000.00;W\n\r\0"));
    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_PWR_MSB2, 3);
    float_to_string(LTC2945_raw_to_power(rval, SR_mohm), text_buf, 4, 2);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf); //write to console

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) head);
    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "VSENSE\n\r ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "ISENSE\n\r ");
    //memcpy(text_buf, "+000.000;mV\n\r\0", sizeof("+000.000;mV\n\r\0"));
    memcpy(text_buf, "+00000.0;mA\n\r\0", sizeof("+00000.0;mA\n\r\0"));
    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_SENSE_MSB, 2);
    //float_to_string(LTC2945_raw_to_voltage(rval), text_buf, 3, 3);
    float_to_string(LTC2945_raw_to_current(rval, SR_mohm), text_buf, 5, 1);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf); //write to console

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) head);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "VIN\n\r ");
    memcpy(text_buf, "+000.000;V\n\r\0", sizeof("+000.000;V\n\r\0"));
    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_VIN_MSB, 2);
    float_to_string(LTC2945_raw_to_voltage(rval), text_buf, 3, 3);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf); //write to console

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) head);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "ADIN\n\r ");
    memcpy(text_buf, "+000.000;V\n\r\0", sizeof("+000.000;V\n\r\0"));
    rval = LTC2945_read_reg(i2c_inst, I2C_SER_ADDR, LTC2945_ADIN_MSB, 2);
    float_to_string(LTC2945_raw_to_adin(rval, mult), text_buf, 3, 3);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf); //write to console

    return;
}

// sets min to all '1' and max to all '0'
//if size = 2 writes 4 bytes 0x0000FFFF (starting from specified address)
//if size = 3 writes 6 bytes 0x000000FFFFFF (starting from specified address)
void LTC2945_reset_limits(void *i2c_inst, uint32_t I2C_SER_ADDR, uint8_t reg_addr, uint8_t size) {

	//enter test mode
	LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, LTC2945_CTRL, 0x15, 1);

	if (size == 2) {
//		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr,   0x0000, 2);
//		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr+2, 0xFFFF, 2);
		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr,   0x00, 1);
		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr+1, 0x00, 1);
		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr+2, 0xFF, 1);
		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr+3, 0xFF, 1);
	} else if (size == 3) {
//		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr,   0x000000, 3);
//		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr+3, 0xFFFFFF, 3);
		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr  , 0x00, 1);
		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr+1, 0x00, 1);
		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr+2, 0x00, 1);
		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr+3, 0xFF, 1);
		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr+4, 0xFF, 1);
		LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, reg_addr+5, 0xFF, 1);
	} else {
		//do nothing
	}

	//exit test mode
	LTC2945_write_reg(i2c_inst, I2C_SER_ADDR, LTC2945_CTRL, 0x05, 1);

	return;
}

void LTC2945_reset_all_limits(void *i2c_inst, uint32_t I2C_SER_ADDR) {
	LTC2945_reset_limits(i2c_inst, I2C_SER_ADDR, LTC2945_MAXP_MSB2, 3); //power limits
	LTC2945_reset_limits(i2c_inst, I2C_SER_ADDR, LTC2945_MAX_SNS_MSB, 2); //sense limits
	LTC2945_reset_limits(i2c_inst, I2C_SER_ADDR, LTC2945_MAX_VIN_MSB, 2); //vin limits
	LTC2945_reset_limits(i2c_inst, I2C_SER_ADDR, LTC2945_MAX_ADIN_MSB, 2); //adin limits
}
