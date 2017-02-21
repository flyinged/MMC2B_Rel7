/*
 * max31785.c
 *
 *  Created on: 23.09.2015
 *      Author: malatesta_a
 */

#include "max31785.h"
//#include "core_i2c.h" //core i2c controller not supported
#include "mss_i2c.h"
#include "mss_watchdog.h"

#ifdef MAX31785_DEBUG
#include "mss_uart.h"
#endif

/********************************************** UTILITY FUNCTIONS **************************************************/
#include "core_i2c.h"

extern mss_i2c_status_t mss_i2c_dowrite(
        mss_i2c_instance_t * i2c_inst,
        uint8_t serial_addr,
        uint8_t * tx_buffer,
        uint8_t write_length,
        const uint8_t *msg );

extern mss_i2c_status_t mss_i2c_doread(
        mss_i2c_instance_t * i2c_inst, uint8_t serial_addr,
        uint8_t * tx_buffer, uint8_t write_length,
        uint8_t * rx_buffer, uint8_t read_length,
        const uint8_t *msg);

uint8_t do_i2c_write(void* i2c, uint8_t i2c_addr, uint8_t reg_addr, int16_t val, uint8_t size, const uint8_t* fname);
uint8_t do_i2c_read(void* i2c, uint8_t i2c_addr, uint8_t reg_addr, uint8_t* rx_buf, uint8_t size, const uint8_t* fname);

void max31785_wait_250ms(void);
void max31785_wait_1_4ms(void);

//wait at least 250 ms (used only for special commands)
//clock frequency is MAX31785_CPU_FREQ 250ms=MAX31785_CPU_FREQ/4 clock cycles
//1 for-cycle is 2 instructions (increment+compare), each 1 clock cycle
//number of needed for cycles = MAX31785_CPU_FREQ/4/2
void max31785_wait_250ms(void) {
    uint32_t i;

    for (i=0; i<(MAX31785_CPU_FREQ/8); i++);

    return;
}

//wait at least 1.4 ms (added to all i2c transactions)
//clock frequency is MAX31785_CPU_FREQ => 1.4ms=14*MAX31785_CPU_FREQ/10000 clock cycles
//1 for-cycle is 2 instructions (increment+compare), each 1 clock cycle
//number of needed for cycles = 7*MAX31785_CPU_FREQ/10000
void max31785_wait_1_4ms(void) {
    uint32_t i;

    for (i=0; i<(7*(MAX31785_CPU_FREQ/10000)); i++);

    return;
}

/*
 * local function for i2c write
 * i2c_addr: i2c device address
 * reg_addr: register address inside the i2c device
 * val: value to write (if 8 bit, lower byte is used)
 * size: in bytes. can be only 0 (command only), 1 (byte) or 2 (word)
 * fname: name of calling function (only for debug messages)
 */
uint8_t do_i2c_write(void* i2c, uint8_t i2c_addr, uint8_t reg_addr, int16_t val, uint8_t size, const uint8_t* fname) {

    uint8_t tx_buf[size+1];

    tx_buf[0] = reg_addr;
    tx_buf[1] = val & 0xFF;
    tx_buf[2] = (val>>8) & 0xFF;

    max31785_wait_1_4ms(); //at least 1.4ms delay between any stop/start

    return(mss_i2c_dowrite( i2c, i2c_addr, tx_buf, size+1, fname));
}

/*
 * local function for i2c read
 * i2c_addr: i2c device address
 * reg_addr: register address inside the i2c device
 * rx_buf: pointer to receive buffer (LSB in position 0)
 * size: in bytes. can be only 1 (byte) or 2 (word)
 * fname: name of calling function (only for debug messages)
 */
uint8_t do_i2c_read(void* i2c, uint8_t i2c_addr, uint8_t reg_addr, uint8_t* rx_buf, uint8_t size, const uint8_t* fname) {

    uint8_t tx_buf[1];

    tx_buf[0] = reg_addr;

    max31785_wait_1_4ms(); //at least 1.4ms delay between any stop/start

    return(mss_i2c_doread( i2c, i2c_addr, tx_buf, 1, rx_buf, size, fname));
}


/*
 ****************************************** Data conversion functions **************************************
 * Values are all 16bit signed
 * Basic conversion is   X = (1/m)*(Y*1e-R)-b
 * Reverse conversion is Y = (mX + b) * 1eR
 * X = converted value (in proper units)
 * Y = raw value
 * m = slope factor
 * R = exponent
 * b = offset
 *
 * Voltage is converted to MILLIVOLTS
 *   - V[mV] = Y
 * Voltage scale is dimensionless
 *   - VS = Y/32767
 * Temperature is is CELSIUS DEGREES
 *   - T[C] = Y/100
 * Fan Speed is in RPM
 *   - FS[RPM] = Y
 * Fan PWM is in PERCENT
 *   - FPWM[%] = Y/100
 */

/********************* raw to unit conversion ************************/
int16_t max31785_raw2volt(int16_t Y) {
	return (Y);
}

float max31785_raw2vscale(int16_t Y) {
	return (((float)Y)/32767.0);
}

float max31785_raw2temp(int16_t Y) {
	return (((float)Y)/100.0);
}

int16_t max31785_raw2rpm(int16_t Y) {
	return (Y);
}

float max31785_raw2pwm(int16_t Y) {
	return (((float)Y)/100.0);
}

/********************** unit to raw conversion ************************/
int16_t max31785_volt2raw(int16_t X) {
	return (X);
}

int16_t max31785_vscale2raw(float X) {
	return ((int16_t)(X*32767));
}

int16_t max31785_temp2raw(float X) {
	return ((int16_t)(X*100.0));
}

int16_t max31785_rpm2raw(int16_t X) {
	return (X);
}

int16_t max31785_pwm2raw(float X) {
	return ((int16_t)(X*100.0));
}

/********************** COMMANDS *****************************/

/*
 * Select page -------------------------------------------------------
 * See .h file for page codes
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout, 3=page not valid
 */
uint8_t max31785_page(void* i2c, uint8_t i2c_addr, uint8_t page) {

	if ((page>22) && (page<255)) {
		//unsupported page
#ifdef MAX31785_DEBUG
    	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_page: PAGE not valid\n\r");
#endif
		return (3);
	}

	return( do_i2c_write(i2c, i2c_addr, MAX31785_PAGE, (int16_t) page, 1, (const uint8_t *) "max31785_page") );
}


/*
 * Clear all fault flags --------------------------------------------------------------------------------------------------
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_clear_faults(void* i2c, uint8_t i2c_addr) {

	return( do_i2c_write(i2c, i2c_addr, MAX31785_CLEAR_FAULTS, 0, 0, (const uint8_t *) "max31785_clear_faults") );
}


/*
 * Write protect -------------------------------------------------------
 * Accepted values:
 *     0 = no protection
 *     1 = only WRITE_PROTECT and PAGE commands allowed
 *     2 = only WRITE_PROTECT command allowed
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout, 3=value not valid
 */
uint8_t max31785_write_protect(void* i2c, uint8_t i2c_addr, uint8_t val) {

	if (val>2) {
#ifdef MAX31785_DEBUG
    	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_write_protect: Value not valid\n\r");
#endif
		return (3);
	}

    //(val<<6): 0=>0x00, 1=>0x40, 2=>0x80
	return( do_i2c_write(i2c, i2c_addr, MAX31785_WRITE_PROTECT, (val<<6), 1, (const uint8_t *) "max31785_write_protect") );
}

/*
 * Store configuration data to internal FLASH memory---------------------------------------------------------------------------------------
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_store_default_all(void* i2c, uint8_t i2c_addr) {
	//uint32_t i;
	uint8_t retval;

	retval = do_i2c_write(i2c, i2c_addr, MAX31785_STORE_DEFAULT_ALL, 0, 0, (const uint8_t *) "max31785_store_default_all");

    //should wait at least 250 ms:
    //clock frequency is max 100MHz, 250ms@100MHz=25Mclock cycles
    //1 for-cycle is 2 instructions (increment+compare), each 1 clock cycle
    //number of needed for cycles = 25M/2=12500000
	max31785_wait_250ms();

    return(retval);
}

/*
 * Restore configuration data from internal FLASH memory---------------------------------------------------------------------------------------
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_restore_default_all(void* i2c, uint8_t i2c_addr) {
	uint32_t i;
	uint8_t retval;

	retval = do_i2c_write(i2c, i2c_addr, MAX31785_RESTORE_DEFAULT_ALL, 0, 0, (const uint8_t *) "max31785_restore_default_all");

    //should wait at least 250 ms:
    //clock frequency is max 100MHz, 250ms@100MHz=25Mclock cycles
    //1 for-cycle is 2 instructions (increment+compare), each 1 clock cycle
    //number of needed for cycles = 25M/2=12500000
    for (i=0; i<12500000; i++);

    return(retval);
}


/*
 * Vout scale monitor -------------------------------------------------------
 * Adc max range for voltage measure is 1.225V.
 * It's recommended to scale the voltage with a resistive divider, so that remote voltage=1.0V at ADC.
 * VAL = resistive divider ratio mutiplied by 32767. Can use vscale2raw() function for conversion
 * Accepted range: [0:32767]
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout, 3=value not valid
 */
uint8_t max31785_vout_scale_monitor(void* i2c, uint8_t i2c_addr, uint8_t page, uint16_t val) {

	if (val>0x7FFF) {
#ifdef MAX31785_DEBUG
    	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_vout_scale_monitor: Value not valid\n\r");
#endif
		return (3);
	}

	if (page<17 || page>22) {
	#ifdef MAX31785_DEBUG
	        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_vout_scale_monitor: Page not valid\n\r");
	#endif
	        return (3);
	}

	max31785_page(i2c, i2c_addr, page);

	return( do_i2c_write(i2c, i2c_addr, MAX31785_VOUT_SCALE_MONITOR, val, 2, (const uint8_t *) "max31785_vout_scale_monitor") );
}

/*
 * Fan enable -------------------------------------------------------
 * If en=0, FAN is disabled, otherwise it is enabled
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_fan_enable(void* i2c, uint8_t i2c_addr, uint8_t page, uint8_t en) {
	uint8_t rx_buf[1], tx_buf[1], retval;

	if (page>5) {
	    #ifdef MAX31785_DEBUG
	            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_fan_enable: Page not valid\n\r");
	    #endif
	    return (3);
	}

	max31785_page(i2c, i2c_addr, page);

	//first read current value
	retval = do_i2c_read(i2c, i2c_addr, MAX31785_FAN_CONFIG_1_2, rx_buf, 1, (const uint8_t *) "max31785_fan_enable");
	if (retval) return (retval);

	//modify value
	if (en) {
		tx_buf[0] = (rx_buf[0] | 0x80);
	} else {
		tx_buf[0] = (rx_buf[0] & 0x7F);
	}

	//write new value
	return( do_i2c_write(i2c, i2c_addr, MAX31785_FAN_CONFIG_1_2, tx_buf[0], 1, (const uint8_t *) "max31785_fan_enable") );

}

/*
 * Select RPM or PWM% as controlling parameter -----------------------------
 * If RPM_notPWM=0 then PWM is selected, otherwise RPM is selected
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_fan_rpm(void* i2c, uint8_t i2c_addr, uint8_t page, uint8_t RPM_notPWM) {
	uint8_t tx_buf[1], rx_buf[1], retval;

	if (page>5) {
#ifdef MAX31785_DEBUG
	    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_fan_rpm: Page not valid\n\r");
#endif
	    return (3);
	}

	max31785_page(i2c, i2c_addr, page);

	//first read current value
	retval = do_i2c_read(i2c, i2c_addr, MAX31785_FAN_CONFIG_1_2, rx_buf, 1, (const uint8_t *) "max31785_fan_rpm");
	if (retval) return (retval);

	//modify value
	if (RPM_notPWM) {
		tx_buf[0] = (rx_buf[0] | 0x40);
	} else {
		tx_buf[0] = (rx_buf[0] & 0xBF);
	}

	//write new value
	return( do_i2c_write(i2c, i2c_addr, MAX31785_FAN_CONFIG_1_2, tx_buf[0], 1, (const uint8_t *) "max31785_fan_rpm") );
}

/*
 * Select how many TACH pulses per FAN revolution -----------------------------
 * Valid input range [1:4]
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout, 3=pulses value not valid
 */
uint8_t max31785_fan_tach_pulses(void* i2c, uint8_t i2c_addr, uint8_t page, uint8_t npulses) {
	uint8_t tx_buf[1], rx_buf[1], retval;

	if (npulses==0 || npulses>4) {
#ifdef MAX31785_DEBUG
    	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_fan_tach_pulses: Value not valid\n\r");
#endif
		return (3);
	}

	if (page>5) {
#ifdef MAX31785_DEBUG
	    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_fan_tach_pulses: Page not valid\n\r");
#endif
	    return (3);
	}

	max31785_page(i2c, i2c_addr, page);

	//first read current value
	retval = do_i2c_read(i2c, i2c_addr, MAX31785_FAN_CONFIG_1_2, rx_buf, 1, (const uint8_t *) "max31785_fan_tach_pulses");
	if (retval) return (retval);

	//modify value
	rx_buf[0] &= 0xCF; //clear bits on old value
	tx_buf[0] = ((npulses-1)<<4); //encode value and move to right position
	tx_buf[0] |= rx_buf[0]; //merge old register with new settings

	//write new value
	return( do_i2c_write(i2c, i2c_addr, MAX31785_FAN_CONFIG_1_2, tx_buf[0], 1, (const uint8_t *) "max31785_fan_tach_pulses") );

}

/*
 * Enable automatic control -----------------------------------------------------
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout, 3=pulses value not valid
 */
uint8_t max31785_fan_auto_en(void* i2c, uint8_t i2c_addr, uint8_t page) {

    if (page>5) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_fan_auto_en: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

	return( do_i2c_write(i2c, i2c_addr, MAX31785_FAN_COMMAND_1, 0xFFFF, 2, (const uint8_t *) "max31785_fan_auto_en") );

}

/*
 * Force %PWM value (disable automatic control) ----------------------------------------------------------
 * WARNING: device shall be set to PWM with function max31785_fan_rpm()
 * Valid input range [0.00:100.00] (100.00 => 10000 = 0x2710)
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout, 3=input value not valid
 */
uint8_t max31785_fan_force_pwm(void* i2c, uint8_t i2c_addr, uint8_t page, float pwm) {
	int16_t raw;

	//xxx check if device mode is PWM

	if (page>5) {
#ifdef MAX31785_DEBUG
	    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_fan_force_pwm: Page not valid\n\r");
#endif
	    return (3);
	}
	max31785_page(i2c, i2c_addr, page);

	raw = max31785_pwm2raw(pwm); //100.0 => 10000

	if (raw<0 || raw>10000) {
#ifdef MAX31785_DEBUG
		MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_fan_force_pwm: Value not valid\n\r");
#endif
		return (3);
	}

	return( do_i2c_write(i2c, i2c_addr, MAX31785_FAN_COMMAND_1, raw, 2, (const uint8_t *) "max31785_fan_force_pwm") );

}

/*
 * Force %PWM value (disable automatic control) -----------------------------
 * WARNING: device shall be set to RPM with function max31785_fan_rpm()
 * Valid input range [0:32767]
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout, 3=input value not valid
 */
uint8_t max31785_fan_force_rpm(void* i2c, uint8_t i2c_addr, uint8_t page, int16_t rpm) {

    if (page>5) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_fan_force_rpm: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

	//xxx check if device mode is RPM

	//no conversion needed

	if (rpm<0 || rpm>32767) {
#ifdef MAX31785_DEBUG
		MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_fan_force_rpm: Value not valid\n\r");
#endif
		return (3);
	}

	return( do_i2c_write(i2c, i2c_addr, MAX31785_FAN_COMMAND_1, rpm, 2, (const uint8_t *) "max31785_fan_force_rpm") );

}

/*******************************************************************************************************
 * SET LIMITS
 *******************************************************************************************************/

/*
 * Force %PWM value (disable automatic control) -----------------------------
 * WARNING: device shall be set to RPM with function max31785_fan_rpm()
 * Input range: FULL (use voltage conversion function)
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_vout_ov_fault_limit(void* i2c, uint8_t i2c_addr, uint8_t page, int16_t val) {
    if (page<17 || page>22) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_vout_ov_fault_limit: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_VOUT_OV_FAULT_LIMIT, val, 2, (const uint8_t *) "max31785_vout_ov_fault_limit") );
}

uint8_t max31785_vout_ov_warn_limit(void* i2c, uint8_t i2c_addr, uint8_t page, int16_t val) {
    if (page<17 || page>22) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_vout_ov_warn_limit: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_VOUT_OV_WARN_LIMIT, val, 2, (const uint8_t *) "max31785_vout_ov_warn_limit") );
}

uint8_t max31785_vout_uv_fault_limit(void* i2c, uint8_t i2c_addr, uint8_t page, int16_t val) {
    if (page<17 || page>22) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_vout_uv_fault_limit: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_VOUT_UV_FAULT_LIMIT, val, 2, (const uint8_t *) "max31785_vout_uv_fault_limit") );
}

uint8_t max31785_vout_uv_warn_limit(void* i2c, uint8_t i2c_addr, uint8_t page, int16_t val) {
    if (page<17 || page>22) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_vout_uv_warn_limit: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_VOUT_UV_WARN_LIMIT, val, 2, (const uint8_t *) "max31785_vout_uv_warn_limit") );
}

uint8_t max31785_vout_ot_fault_limit(void* i2c, uint8_t i2c_addr, uint8_t page, int16_t val) {
    if (page<6 || page>16) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_vout_ot_fault_limit: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_OT_FAULT_LIMIT, val, 2, (const uint8_t *) "max31785_vout_ot_fault_limit") );
}

uint8_t max31785_vout_ot_warn_limit(void* i2c, uint8_t i2c_addr, uint8_t page, int16_t val) {
    if (page<6 || page>16) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_vout_ot_warn_limit: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_OT_WARN_LIMIT, val, 2, (const uint8_t *) "max31785_vout_ot_warn_limit") );
}

/*
 * get status word **************************************************************************************
 * Use following masks:
 * MAX31785_STATUS_WORD_NOTA     0x0001 //a status bit in range 15:8 is set
 * MAX31785_STATUS_WORD_CML      0x0002 //GET_STATUS_CML Communication, memory or logic fault (I2C/command error)
 * MAX31785_STATUS_WORD_TEMP     0x0004 //Temperature fault/warning (overtemperature)
 * MAX31785_STATUS_WORD_VOUT_OV  0x0020 //overvoltage fault
 * MAX31785_STATUS_WORD_FANS     0x0400 //GET_STATUS_FANS
 * MAX31785_STATUS_WORD_MFR      0x1000 //GET_STATUS_MFR_SPECIFIC
 * MAX31785_STATUS_WORD_VOUT     0x8000 //overvoltage fault/warning GET_STATUS_VOUT
 * if return value is 0xFFFF then an i2c error occurred
 */
uint16_t max31785_get_status(void* i2c, uint8_t i2c_addr) {
    uint8_t retval, rx_buf[2];

    retval = do_i2c_read(i2c, i2c_addr, MAX31785_STATUS_WORD, rx_buf, 2, (const uint8_t *) "max31785_get_status");

    if (retval) {
        return (0xFFFF);
    } else {
        return ((rx_buf[1]<<8) | rx_buf[0]);
    }
}

/*
 * get status for voltage monitors **************************************************************************************
 * To be read when STATUS_WORD_VOUT is set.
 * Use following masks:
 * MAX31785_STATUS_VOUT_OV_FAULT 0x80 //overvoltage fault
 * MAX31785_STATUS_VOUT_OV_WARN  0x40 //overvoltage warning
 * MAX31785_STATUS_VOUT_UV_FAULT 0x20 //undervoltage fault
 * MAX31785_STATUS_VOUT_UV_WARN  0x10 //undervoltage warning
 * if return value is 0xFF then an i2c error occurred
 */
uint8_t max31785_get_status_vout(void* i2c, uint8_t i2c_addr, uint8_t page) {
    uint8_t retval, rx_buf[1];

    if (page<17 || page>22) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_get_status_vout: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    retval = do_i2c_read(i2c, i2c_addr, MAX31785_STATUS_VOUT, rx_buf, 1, (const uint8_t *) "max31785_get_status_vout");

    if (retval) {
        return (0xFF);
    } else {
        return (rx_buf[0]);
    }
}

/*
 * get status for i2c interface **************************************************************************************
 * To be read when STATUS_WORD_CML is set.
 * Use following masks:
 * MAX31785_STATUS_CML_COMM_FAULT     //received invalid or unsupported command
 * MAX31785_STATUS_CML_DATA_FAULT     //received invalid or unsupported data
 * MAX31785_STATUS_CML_FAULT_LOG_FULL //MFR_MV_FAULT_LOG is full and needs to be cleared
 * if return value is 0xFF then an i2c error occurred
 */
uint8_t max31785_get_status_cml(void* i2c, uint8_t i2c_addr) {
    uint8_t retval, rx_buf[1];

    retval = do_i2c_read(i2c, i2c_addr, MAX31785_STATUS_CML, rx_buf, 1, (const uint8_t *) "max31785_get_status_cml");

    if (retval) {
        return (0xFF);
    } else {
        return (rx_buf[0]);
    }
}

/*
 * get status for i2c interface **************************************************************************************
 * To be read when STATUS_WORD_TEMP is set.
 * Use following masks:
 * MAX31785_STATUS_MFR_SPECIFIC_OT_WARN  0x40 //overtemperature warning
 * MAX31785_STATUS_MFR_SPECIFIC_OT_FAULT 0x20 //overtemperature fault
 * MAX31785_STATUS_MFR_SPECIFIC_WATCHDOG 0x10 //watchdog reset occurred
 * if return value is 0xFF then an i2c error occurred
*/
uint8_t max31785_get_status_mfr_specific(void* i2c, uint8_t i2c_addr, uint8_t page) {
    uint8_t retval, rx_buf[1];

    if (page<6 || page>16) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_get_status_mfr_specific: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    retval = do_i2c_read(i2c, i2c_addr, MAX31785_STATUS_MFR_SPECIFIC, rx_buf, 1, (const uint8_t *) "max31785_get_status_mfr_specific");

    if (retval) {
        return (0xFF);
    } else {
        return (rx_buf[0]);
    }
}

/*
 * get status for fans **************************************************************************************
 * To be read when STATUS_WORD_FANS is set.
 * Use following masks:
 * MAX31785_STATUS_FANS_FAULT   0x80 //fan fault
 * MAX31785_STATUS_FANS_WARN    0x20 //fan warning
 * Following values shall be COMPARED with lower nibble (instead of masked)
 * MAX31785_STATUS_FANS_RED     0x08 //very bad health
 * MAX31785_STATUS_FANS_ORANGE  0x04 //bad health
 * MAX31785_STATUS_FANS_YELLOW  0x02 //good health
 * MAX31785_STATUS_FANS_GREEN   0x01 //very good health
 * MAX31785_STATUS_FANS_UNKNOWN 0X00 //status unknown
 * if return value is 0xFF then an i2c error occurred
*/
uint8_t max31785_get_status_fans(void* i2c, uint8_t i2c_addr, uint8_t page) {
    uint8_t retval, rx_buf[1];

    if (page>5) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_get_status_fans: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    retval = do_i2c_read(i2c, i2c_addr, MAX31785_STATUS_FANS_1_2, rx_buf, 1, (const uint8_t *) "max31785_get_status_fans");

    if (retval) {
        return (0xFF);
    } else {
        return (rx_buf[0]);
    }
}

/*******************************************************************************************************
 * READ MEASURES
 *******************************************************************************************************/

/*
 * read voltage
 * returns raw data. shall be converted with raw2voltage()
 */
int16_t max31785_read_vout(void* i2c, uint8_t i2c_addr, uint8_t page) {
    uint8_t retval, rx_buf[2];

    if (page<17 || page>22) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_read_vout: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    retval = do_i2c_read(i2c, i2c_addr, MAX31785_READ_VOUT, rx_buf, 2, (const uint8_t *) "max31785_read_vout");

    //if (retval) {
    //    return (0xFFFF);
    //} else {
        return ((rx_buf[1]<<8) | rx_buf[0]);
    //}
}


/*
 * read temperature
 * returns raw data. shall be converted with raw2temp(),
 * 0x7FFF = FAULTY SENSOR, 0x0000 = SENSOR DISABLED
 */
int16_t max31785_read_temperature(void* i2c, uint8_t i2c_addr, uint8_t page) {
    uint8_t retval, rx_buf[2];

    if (page<6 || page>16) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_read_temperature: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    retval = do_i2c_read(i2c, i2c_addr, MAX31785_READ_TEMPERATURE_1, rx_buf, 2, (const uint8_t *) "max31785_read_temperature");

    //if (retval) {
    //    return (0xFFFF);
    //} else {
        return ((rx_buf[1]<<8) | rx_buf[0]);
    //}
}

/*
 * read fan speed
 * returns raw data. shall be converted with raw2rpm() or raw2pwm() according to value set by fan_rpm().
 */
int16_t max31785_read_fan_speed(void* i2c, uint8_t i2c_addr, uint8_t page) {
    uint8_t retval, rx_buf[2];

    if (page>5) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_read_fan_speed: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    retval = do_i2c_read(i2c, i2c_addr, MAX31785_READ_FAN_SPEED_1, rx_buf, 2, (const uint8_t *) "max31785_read_fan_speed");

    //if (retval) {
    //    return (0xFFFF);
    //} else {
        return ((rx_buf[1]<<8) | rx_buf[0]);
    //}
}

/***********************************************************************************************************************/
/*
 * force alert logging
 */
uint8_t max31785_force_nv_fault_log(void* i2c, uint8_t i2c_addr, uint8_t en) {
    uint8_t rx_buf[2], retval;
    uint16_t tx_val;
    uint32_t i;

    //first read current value
    retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_MODE, rx_buf, 2, (const uint8_t *) "max31785_force_nv_fault_log");
    if (retval) return (retval);

    //modify value
    tx_val = rx_buf[0] | (rx_buf[1]<<8);
    if (en) {
        tx_val |= MAX31785_MFR_MODE_FORCE_NVFL;
    } else {
        tx_val &= (~MAX31785_MFR_MODE_FORCE_NVFL);
    }

    //write new value
    retval = do_i2c_write(i2c, i2c_addr, MAX31785_MFR_MODE, tx_val, 2, (const uint8_t *) "max31785_force_nv_fault_log");

    //should wait at least 250 ms:
    //clock frequency is max 100MHz, 250ms@100MHz=25Mclock cycles
    //1 for-cycle is 2 instructions (increment+compare), each 1 clock cycle
    //number of needed for cycles = 25M/2=12500000
    for (i=0; i<12500000; i++);

    return(retval);
}

/*
 * clear non volatile log (bit is automatically reset)
 */
uint8_t max31785_clear_nv_fault_log(void* i2c, uint8_t i2c_addr, uint8_t en) {
    uint8_t rx_buf[2], retval;
    uint16_t tx_val;
    //uint32_t i;

    //first read current value
    retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_MODE, rx_buf, 2, (const uint8_t *) "max31785_clear_nv_fault_log");
    if (retval) return (retval);

    //modify value
    tx_val = rx_buf[0] | (rx_buf[1]<<8);
    if (en) {
        tx_val |= MAX31785_MFR_MODE_CLEAR_NVFL;
    } else {
        tx_val &= (~MAX31785_MFR_MODE_CLEAR_NVFL);
    }

    //write new value
    retval = do_i2c_write(i2c, i2c_addr, MAX31785_MFR_MODE, tx_val, 2, (const uint8_t *) "max31785_clear_nv_fault_log");

    //should wait at least 250 ms:
    //clock frequency is max 100MHz, 250ms@100MHz=25Mclock cycles
    //1 for-cycle is 2 instructions (increment+compare), each 1 clock cycle
    //number of needed for cycles = 25M/2=12500000
    max31785_wait_250ms();

    return(retval);
}

/*
 * set enable bit for alert function
 * when ALERT is on, faults will assert the ALERT# bit, and device will respond only to 7b'0C/8b'18 address
 */
uint8_t max31785_alert_en(void* i2c, uint8_t i2c_addr, uint8_t en) {
    uint8_t rx_buf[2], retval;
    uint16_t tx_val;
    uint32_t i;

    //first read current value
    retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_MODE, rx_buf, 2, (const uint8_t *) "max31785_alert_en");
    if (retval) return (retval);

    //modify value
    tx_val = rx_buf[0] | (rx_buf[1]<<8);
    if (en) {
        tx_val |= MAX31785_MFR_MODE_ALERT;
    } else {
        tx_val &= (~MAX31785_MFR_MODE_ALERT);
    }

    //write new value
    retval = do_i2c_write(i2c, i2c_addr, MAX31785_MFR_MODE, tx_val, 2, (const uint8_t *) "max31785_alert_en");

    //should wait at least 250 ms:
    //clock frequency is max 100MHz, 250ms@100MHz=25Mclock cycles
    //1 for-cycle is 2 instructions (increment+compare), each 1 clock cycle
    //number of needed for cycles = 25M/2=12500000
    for (i=0; i<12500000; i++);

    return(retval);
}

/*
 * send sequence to perform soft reset (1,0,1)
 */
uint8_t max31785_soft_reset(void* i2c, uint8_t i2c_addr) {
    uint8_t rx_buf[2], retval;
    uint16_t tx_val;
    uint32_t i;

    //first read current value
    retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_MODE, rx_buf, 2, (const uint8_t *) "max31785_soft_reset");
    if (retval) return (retval);

    //modify value
    tx_val = rx_buf[0] | (rx_buf[1]<<8);
    //set bit
    tx_val |= MAX31785_MFR_MODE_SOFT_RESET;
    retval = do_i2c_write(i2c, i2c_addr, MAX31785_MFR_MODE, tx_val, 2, (const uint8_t *) "max31785_soft_reset");
    if (retval) return (retval);
    //reset bit
    tx_val &= (~MAX31785_MFR_MODE_SOFT_RESET);
    retval = do_i2c_write(i2c, i2c_addr, MAX31785_MFR_MODE, tx_val, 2, (const uint8_t *) "max31785_soft_reset");
    if (retval) return (retval);
    //set bit again
    tx_val &= (~MAX31785_MFR_MODE_SOFT_RESET);
    retval = do_i2c_write(i2c, i2c_addr, MAX31785_MFR_MODE, tx_val, 2, (const uint8_t *) "max31785_soft_reset");
    if (retval) return (retval);

    //should wait at least 250 ms:
    //clock frequency is max 100MHz, 250ms@100MHz=25Mclock cycles
    //1 for-cycle is 2 instructions (increment+compare), each 1 clock cycle
    //number of needed for cycles = 25M/2=12500000
    for (i=0; i<12500000; i++);

    return(retval);

}

/*
 * set fan health criteria
 * GREEN/ORANGE/RED flag is set according to difference (D) between real and measured fan speed (in %).
 * Profiles are encoded as follows (accepted values: 0:3):
 * 0: difference D<10% GREEN, 10%<D<15% ORANGE, >15% RED
 * 1: difference D<10% GREEN, 10%<D<20% ORANGE, >20% RED
 * 2: difference D<15% GREEN, 15%<D<20% ORANGE, >20% RED
 * 3: difference D<15% GREEN, 15%<D<25% ORANGE, >25% RED
 */
uint8_t max31785_fan_health_criteria(void* i2c, uint8_t i2c_addr, uint8_t fhc) {
    uint8_t rx_buf[2], retval;
    uint16_t tx_val;
    uint32_t i;

    if (fhc>3) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_fan_health_criteria: Value not valid\n\r");
#endif
        return (3);
    }

    //first read current value
    retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_MODE, rx_buf, 2, (const uint8_t *) "max31785_fan_health_criteria");
    if (retval) return (retval);

    //modify value
    tx_val = rx_buf[0] | (rx_buf[1]<<8);
    //clear former value
    tx_val &= (~MAX31785_MFR_MODE_FHC);
    //set new value
    tx_val |= (fhc<<6);

    //write new value
    retval = do_i2c_write(i2c, i2c_addr, MAX31785_MFR_MODE, tx_val, 2, (const uint8_t *) "max31785_fan_health_criteria");

    //should wait at least 250 ms:
    //clock frequency is max 100MHz, 250ms@100MHz=25Mclock cycles
    //1 for-cycle is 2 instructions (increment+compare), each 1 clock cycle
    //number of needed for cycles = 25M/2=12500000
    for (i=0; i<12500000; i++);

    return(retval);
}

/*
 * enable voltage measurements on channels
 * mask = 6 bit value (channel 5:0 enable status)
 */
uint8_t max31785_voltage_sense_en(void* i2c, uint8_t i2c_addr, uint8_t mask) {
    uint8_t rx_buf[2], retval;
    uint16_t tx_val;
    uint32_t i;

    if (mask&0xC0) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_voltage_sense_en: Value not valid\n\r");
#endif
        return (3);
    }

    //first read current value
    retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_MODE, rx_buf, 2, (const uint8_t *) "max31785_voltage_sense_en");
    if (retval) return (retval);

    //modify value
    tx_val = rx_buf[0] | (rx_buf[1]<<8);
    //clear former value
    tx_val &= (~MAX31785_MFR_MODE_ADC_MASK);
    //set new value
    tx_val |= mask;

    //write new value
    retval = do_i2c_write(i2c, i2c_addr, MAX31785_MFR_MODE, tx_val, 2, (const uint8_t *) "max31785_voltage_sense_en");

    //should wait at least 250 ms:
    //clock frequency is max 100MHz, 250ms@100MHz=25Mclock cycles
    //1 for-cycle is 2 instructions (increment+compare), each 1 clock cycle
    //number of needed for cycles = 25M/2=12500000
    for (i=0; i<12500000; i++);

    return(retval);
}

/*******************************************************************************************************
 * MAX/MIN MONITORS (per page)
 *******************************************************************************************************/

int16_t max31785_get_voltage_peak(void* i2c, uint8_t i2c_addr, uint8_t page) {
    uint8_t rx_buf[2], retval;

    if (page<17 || page>22) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_get_voltage_peak: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    //read value
    retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_VOUT_PEAK, rx_buf, 2, (const uint8_t *) "max31785_get_voltage_peak");

    return(rx_buf[0] | (rx_buf[1]<<8));
}

uint8_t max31785_reset_voltage_peak(void* i2c, uint8_t i2c_addr, uint8_t page) {

    if (page<17 || page>22) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_reset_voltage_peak: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_MFR_VOUT_PEAK, 0x0000, 2, (const uint8_t *) "max31785_reset_voltage_peak") );
}

int16_t max31785_get_temperature_peak(void* i2c, uint8_t i2c_addr, uint8_t page) {
    uint8_t rx_buf[2], retval;

    if (page<6 || page>16) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_get_temperature_peak: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    //read value
    retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_TEMPERATURE_PEAK, rx_buf, 2, (const uint8_t *) "max31785_get_temperature_peak");

    return(rx_buf[0] | (rx_buf[1]<<8));
}
uint8_t max31785_reset_temperature_peak(void* i2c, uint8_t i2c_addr, uint8_t page) {

    if (page<6 || page>16) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_reset_temperature_peak: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_MFR_TEMPERATURE_PEAK, 0x8000, 2, (const uint8_t *) "max31785_reset_temperature_peak") );
}

int16_t max31785_get_voltage_min(void* i2c, uint8_t i2c_addr, uint8_t page){
    uint8_t rx_buf[2], retval;

    if (page<17 || page>22) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_get_voltage_min: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    //read value
    retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_VOUT_MIN, rx_buf, 2, (const uint8_t *) "max31785_get_voltage_min");

    return(rx_buf[0] | (rx_buf[1]<<8));
}

uint8_t max31785_reset_voltage_min(void* i2c, uint8_t i2c_addr, uint8_t page) {

    if (page<17 || page>22) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_reset_voltage_min: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_MFR_VOUT_MIN, 0x7FFF, 2, (const uint8_t *) "max31785_reset_voltage_min") );
}


/*
 * Set the MFR_FAULT_RESPONSE register
 *
 * MAX31785_MFR_FR_NV_LOG              //1=log fault in MFR_NV_FAULT_LOG
 * MAX31785_MFR_FR_NV_LOG_OV           //1=log also overvoltage (pages 17:22 only - remote voltages)
 * MAX31785_MFR_FR_UV_OV_FILTER        //1=need 2 events to set fault/warning (pages 17:22 only - remote voltages)
 * MAX31785_MFR_FR_FAULT_PIN_ENABLE_OV //1=enable fault pin also for overvoltage (pages 17:22 only - remote voltages)
 * MAX31785_MFR_FR_FAULT_PIN_ENABLE    //1=enable FAULT# pin
 * MAX31785_MFR_FR_FAULT_PIN_MONITOR   //1=force fan to 100% when FAULT# is asserted(pages 0:5 only - fans)
 */
uint8_t max31785_set_mfr_fault_response(void* i2c, uint8_t i2c_addr, uint8_t page, uint8_t reg) {

    if (page==0xFF) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_set_mfr_fault_response: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_MFR_FAULT_RESPONSE, reg, 1, (const uint8_t *) "max31785_set_mfr_fault_response") );
}

/*
 * get device total lifetime in seconds
 */
uint32_t max31785_get_mfr_time_count(void* i2c, uint8_t i2c_addr) {
    uint8_t rx_buf[4], retval;

    //read value
    retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_TIME_COUNT, rx_buf, 4, (const uint8_t *) "max31785_get_mfr_time_count");

    if (retval) {
        return (0xFFFFFFFF);
    } else {
        return(rx_buf[0] | (rx_buf[1]<<8) | (rx_buf[2]<<16) | (rx_buf[3]<<24));
    }
}

/*
 * config current temp sensor (select page first)
 * MAX31785_MFR_TCONFIG_ENABLE 0x8000 //enable current temperature sensor
 * MAX31785_MFR_TCONFIG_OFFSET 0x7C00 //0:1E=offset in Celsius degrees,1F=test mode
 * MAX31785_MFR_TCONFIG_FAN5   0x0020 //current sensor is used to control fan5
 * MAX31785_MFR_TCONFIG_FAN4   0x0010 //current sensor is used to control fan4
 * MAX31785_MFR_TCONFIG_FAN3   0x0008 //current sensor is used to control fan3
 * MAX31785_MFR_TCONFIG_FAN2   0x0004 //current sensor is used to control fan2
 * MAX31785_MFR_TCONFIG_FAN1   0x0002 //current sensor is used to control fan1
 * MAX31785_MFR_TCONFIG_FAN0   0x0001 //current sensor is used to control fan0
 */
uint8_t max31785_set_mfr_temp_sensor_config(void* i2c, uint8_t i2c_addr, uint8_t page, uint16_t reg) {

    if (page<6 || page>16) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_set_mfr_temp_sensor_config: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_MFR_TEMP_SENSOR_CONFIG, reg, 2, (const uint8_t *) "max31785_set_mfr_temp_sensor_config") );
}

/*
 * config current fan (select page first)
 * MAX31785_MFR_FAN_CONFIG_FREQ        0xE000 //PWM frequency 0=30Hz, 1=50Hz, 2=100Hz, 3=150Hz, 7=25kHz
 * MAX31785_MFR_FAN_CONFIG_DUAL_TACH   0x1000 //enable dual tachometer
 * MAX31785_MFR_FAN_CONFIG_HYS         0x0C00 //hysteresis for auto slow down 0,1,2,3=2,4,6,8°C
 * MAX31785_MFR_FAN_CONFIG_TSFO        0x0200 //0=ramp to 100% on sensor fault/no fan_command update, 1=on fault use last speed value
 * MAX31785_MFR_FAN_CONFIG_TACHO       0x0100 //0=ramp to 100% on fan fault, 1=don't
 * MAX31785_MFR_FAN_CONFIG_RAMP        0x00E0 //time to ramp 40% to 100%: (0,1,2,3,4,5,6,7)=(60,30,20,12,6,4,3,2.4 seconds)
 * MAX31785_MFR_FAN_CONFIG_HEALTH      0x0010 //enable fan health check
 * MAX31785_MFR_FAN_CONFIG_ROTOR_HI_LO 0x0008 //polarity of TACK for stopped rotor: 0=low, 1=high
 * MAX31785_MFR_FAN_CONFIG_ROTOR       0x0004 //0=TACH connected to actual tachometer, 1=TACH input used as stuck rotor signal
 * MAX31785_MFR_FAN_CONFIG_SPIN        0x0003 //start behavior: 0=no spinup control, 1=100% for 2 revolutions, 2=100% for 4 revolutions, 3=100% for 8 revolutions
*/
uint8_t max31785_set_mfr_fan_config(void* i2c, uint8_t i2c_addr, uint8_t page, uint16_t reg) {

    if (page>5) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_set_mfr_fan_config: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_MFR_FAN_CONFIG, reg, 2, (const uint8_t *) "max31785_set_mfr_fan_config") );
}

uint16_t max31785_get_mfr_fan_config(void* i2c, uint8_t i2c_addr, uint8_t page) {
    uint8_t rx_buf[2];

    rx_buf[0] = 0xFF;
    rx_buf[1] = 0xFF;

    if (page>5) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_get_mfr_fan_config: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    do_i2c_read(i2c, i2c_addr, MAX31785_MFR_FAN_CONFIG, rx_buf, 2, (const uint8_t *) "max31785_get_mfr_fan_config");
    return( rx_buf[0] | (rx_buf[1]<<8) );
}

/*
 * get current PWM of selected FAN
 */
int16_t max31785_mfr_read_fan_pwm(void* i2c, uint8_t i2c_addr, uint8_t page) {
    uint8_t rx_buf[2], retval;

    if (page>5) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_mfr_read_fan_pwm: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    //read value
    retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_READ_FAN_PWM, rx_buf, 2, (const uint8_t *) "max31785_mfr_read_fan_pwm");

    if (retval) {
        return (0xFFFF);
    } else {
        return(rx_buf[0] | (rx_buf[1]<<8) );
    }
}

/*----------------------------------------------------------------------------------
 * set FAN fault limit for currently selected FAN
 * input value: RPM/PWM according to mode. 0x0000=disable checking, 0X0001 SIGNAL ONLY LOCKED FAN
 * when the TACH measures less than the limit for 10s, the FAN_FAULT flag is set.
 */
uint8_t max31785_mfr_fan_fault_limit(void* i2c, uint8_t i2c_addr, uint8_t page, uint16_t limit) {

    if (page>5) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_mfr_fan_fault_limit: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_MFR_FAN_FAULT_LIMIT, limit, 2, (const uint8_t *) "max31785_mfr_fan_fault_limit") );
}

/*
 * set FAN warning limit for currently selected FAN
 * input value: RPM/PWM according to mode. 0x0000=disable checking.
 * when the TACH measures less than the limit for 10s, the FAN_WARN flag is set.
 */
uint8_t max31785_mfr_fan_warn_limit(void* i2c, uint8_t i2c_addr, uint8_t page, uint16_t limit) {

    if (page>5) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_mfr_fan_warn_limit: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    return( do_i2c_write(i2c, i2c_addr, MAX31785_MFR_FAN_WARN_LIMIT, limit, 2, (const uint8_t *) "max31785_mfr_fan_warn_limit") );
}

/*
 * get run time in HOURS of selected FAN
 */
uint16_t max31785_mfr_fan_run_time(void* i2c, uint8_t i2c_addr, uint8_t page) {
    uint8_t rx_buf[2], retval;

    if (page>5) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_mfr_fan_run_time: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    //read value
    retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_FAN_RUN_TIME, rx_buf, 2, (const uint8_t *) "max31785_mfr_fan_run_time");

    if (retval) {
        return (0xFFFF);
    } else {
        return(rx_buf[0] | (rx_buf[1]<<8) );
    }
}

/*
 * get average %PWM over current FAN's lifetime
 */
uint16_t max31785_mfr_fan_pwm_avg(void* i2c, uint8_t i2c_addr, uint8_t page) {
    uint8_t rx_buf[2], retval;

    if (page>5) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_mfr_fan_pwm_avg: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    //read value
    retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_FAN_PWM_AVG, rx_buf, 2, (const uint8_t *) "max31785_mfr_fan_pwm_avg");

    if (retval) {
        return (0xFFFF);
    } else {
        return(rx_buf[0] | (rx_buf[1]<<8) );
    }
}

/*
 * write table containing relation between fan speed and PWM for current FAN.
 * Used to monitor fan's health.
 * Table contains 4 16bit RPM values (LSB lower position):
 * Bytes 0-1: RPM at 40%  PWM
 * Bytes 2-3: RPM at 60%  PWM
 * Bytes 4-5: RPM at 80%  PWM
 * Bytes 6-7: RPM at 100% PWM
 */
uint8_t max31785_mfr_fan_pwm2rpm(void* i2c, uint8_t i2c_addr, uint8_t page, const uint8_t* table) {
    i2c_status_t status;
    uint8_t i, tx_buf[9]; //, rx_buf[8], retval;

    if (page>5) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_mfr_fan_pwm2rpm: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    //retval = do_i2c_read(i2c, i2c_addr, MAX31785_MFR_FAN_PWM2RPM, rx_buf, 8, (const uint8_t *) "rmax31785_mfr_fan_pwm2rpm");

    tx_buf[0] = MAX31785_MFR_FAN_PWM2RPM;
    for (i=0; i<8; i++) {
        tx_buf[i+1] = table[i];
    }

    status = mss_i2c_dowrite( i2c, i2c_addr, tx_buf, 9, (const uint8_t *) "rmax31785_mfr_fan_pwm2rpm");
    return (status);

//    MSS_I2C_write(i2c, i2c_addr, tx_buf, 9, I2C_RELEASE_BUS);
//    status = MSS_I2C_wait_complete(i2c, 3000u);

    if (status == I2C_FAILED) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "max31785_mfr_fan_pwm2rpm");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) ": I2C write transaction failed\n\r");
#endif
        return (1);
    }
    if (status == I2C_TIMED_OUT) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "max31785_mfr_fan_pwm2rpm");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) ": I2C write transaction timed-out\n\r");
#endif
        MSS_WD_reload();
        return (2);
    }

    return(0);
}


/************************************************************************************************/


uint8_t max31785_read(void* i2c, uint8_t i2c_addr, uint8_t reg, uint8_t page, uint8_t* rdata, uint8_t size) {

    if (page>22 && page<255) {
#ifdef MAX31785_DEBUG
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rmax31785_read: Page not valid\n\r");
#endif
        return (3);
    }
    max31785_page(i2c, i2c_addr, page);

    //read value
    return(do_i2c_read(i2c, i2c_addr, reg, rdata, size, (const uint8_t *) "max31785_read"));
}
