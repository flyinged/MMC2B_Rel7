/*
 * max31785.h
 *
 *  Created on: 23.09.2015
 *      Author: malatesta_a
 *
 * Summary of available functions (grouped by type, all have prefix "max31785_")
 *
 * Unit conversion functions
 *     max31785_get_raw2volt
 *     max31785_get_raw2vscale
 *     max31785_get_raw2temp
 *     max31785_get_raw2rpm
 *     max31785_get_raw2pwm
 *
 *     max31785_get_volt2raw
 *     max31785_get_vscale2raw
 *     max31785_get_temp2raw
 *     max31785_get_rpm2raw
 *     max31785_get_pwm2raw
 *
 * Global (not channel specific)
 *     max31785_soft_reset
 *     max31785_page (select target for action: temp sensor, fan, ADC)
 *     max31785_clear_faults
 *     max31785_write_protect
 *     max31785_store_default_all (save configuration)
 *     max31785_restore_default_all (recall configuration)
 *     max31785_get_status (overall error flags)
 *     max31785_get_status_cml (communication specific error flags)
 *     max31785_alert_en (enable ALERT# pin)
 *     max31785_fan_health_criteria (select preset for FAN health warnings))
 *     max31785_voltage_sense_en (enable voltage measurement channels)
 *     max31785_get_mfr_time_count (get device's total lifetime)
 *
 * Per-channel settings (valid for all channels)
 *     max31785_set_mfr_fault_response (enable logging, fault behavior)
 *
 * Fan Channels
 *   SETTINGS
 *     max31785_fan_enable
 *     max31785_fan_rpm (set if speed is controlled as RPM or %PWM)
 *     max31785_fan_tach_pulses (tachometer pulses per revolution)
 *     max31785_fan_auto_en (automatic speed control with temperature sensors)
 *     max31785_set_mfr_fan_config (configure FAN's TACH, PWM, ramping)
 *     max31785_mfr_fan_fault_limit
 *     max31785_mfr_fan_warn_limit
 *     max31785_fan_force_pwm (force speed in %PWM)
 *     max31785_fan_force_rpm (force speed in RPM)
 *     max31785_mfr_fan_pwm2rpm (FAN profile: speed corresponding to various PWMs)
 *   STATUS/MEASURES
 *     max31785_get_status_fans (fault/warning flags and health indicators)
 *     max31785_read_fan_speed (current)
 *     max31785_mfr_read_fan_pwm (current)
 *     max31785_mfr_fan_run_time
 *     max31785_mfr_fan_pwm_avg
 *
 * Remote temperature sensor channels
 *   SETTINGS
 *     max31785_reset_temperature_peak
 *     max31785_set_mfr_temp_sensor_config (temp sensor enable, set offset, set controlled fans)
 *     max31785_vout_ot_fault_limit (set level for overtemperature fault notification)
 *     max31785_vout_ot_warn_limit (set level for overtemperature warning notification)
 *   STATUS/MEASURES
 *     max31785_get_status_mfr_specific (fault status for selected temp sensor, watchdog reset)
 *     max31785_read_temperature
 *     max31785_get_temperature_peak
 *
 * Remote voltage monitor channels (ADC)
 *   SETTINGS
 *     max31785_vout_scale_monitor (set scale factor for ADC)
 *     max31785_vout_ov_fault_limit (set level for overvoltage fault)
 *     max31785_vout_ov_warn_limit (set level for overvoltage warning)
 *     max31785_vout_uv_fault_limit (set level for undervoltage fault)
 *     max31785_vout_uv_warn_limit (set level for undervoltage warning)
 *     max31785_reset_voltage_peak
 *     max31785_reset_voltage_min
 *   STATUS/MEASURES
 *     max31785_get_status_vout (get fault/warning flags)
 *     max31785_read_vout
 *     max31785_get_voltage_peak
 *     max31785_get_voltage_min
 */

#ifndef MAX31785_H_
#define MAX31785_H_

/* enable debug to print messages to console */
#define MAX31785_DEBUG

#include <stdint.h>

/*
 * Info on MAX31785 communication protocol
 * - I2C allowed SCL frequencies: 10 to 100 KHz
 * - I2C needs 1.4 ms pause between STOP and START (250 ms for commands MFR_MODE, STORE_DEFAULT_ALL, RESTORE_DEFAULT_ALL
 * - Read Word:  (S) ADDR.W.A | CMD.A (RS) ADDR.R.A | LSB.A | MSB.NA (P)
 * - Read Byte:  (S) ADDR.W.A | CMD.A (RS) ADDR.R.A | BYTE.NA (P)
 * - Write Word: (S) ADDR.W.A | CMD.A | LSB.A | MSB.NA (P)
 * - Write Byte: (S) ADDR.W.A | CMD.A | BYTE.NA (P)
 * - Send Byte:  (S) ADDR.W.A | CMD.A (P)
 */
#define MAX31785_CPU_FREQ 100000000 //used to calculate delays

/*
 *  FAULT MANAGEMENT AND REPORTING
 *  Device asserts ALERT# (if enabled in MFR_MODE) and sets bits in status registers. Then it responds only to SMBUS ARA.
 *  Host shall send SMBUS ARA (0x0C/0x18). The device acknowledges, then sends back its address and deasserts ALERT#.
 *  After that is starts responding again to its own address.
 */
#define MAX31785_ARA7 0x0C //7bit address
#define MAX31785_ARA8 0x18 //8bit address

#define MAX31785_I2C_FMAX 100000 //100 KHz max (10KHz min)

//command codes
#define MAX31785_PAGE                   0x00 //select which target to apply the commands (write, 1byte)
#define     MAX31785_PAGE_FAN0          0x00 //Fan connected to PWM0
#define     MAX31785_PAGE_FAN1          0x01 //Fan connected to PWM1
#define     MAX31785_PAGE_FAN2          0x02 //Fan connected to PWM2
#define     MAX31785_PAGE_FAN3          0x03 //Fan connected to PWM3
#define     MAX31785_PAGE_FAN4          0x04 //Fan connected to PWM4
#define     MAX31785_PAGE_FAN5          0x05 //Fan connected to PWM5
#define     MAX31785_PAGE_TD0           0x06 //remote thermal diode connected to ADC0
#define     MAX31785_PAGE_TD1           0x07 //remote thermal diode connected to ADC1
#define     MAX31785_PAGE_TD2           0x08 //remote thermal diode connected to ADC2
#define     MAX31785_PAGE_TD3           0x09 //remote thermal diode connected to ADC3
#define     MAX31785_PAGE_TD4           0x0A //remote thermal diode connected to ADC4
#define     MAX31785_PAGE_TD5           0x0B //remote thermal diode connected to ADC5
#define     MAX31785_PAGE_TINT          0x0C //internal temperature sensor
#define     MAX31785_PAGE_TIIC0         0x0D //remote i2c temperature sensor 0
#define     MAX31785_PAGE_TIIC1         0x0E //remote i2c temperature sensor 1
#define     MAX31785_PAGE_TIIC2         0x0F //remote i2c temperature sensor 2
#define     MAX31785_PAGE_TIIC3         0x10 //remote i2c temperature sensor 3
#define     MAX31785_PAGE_RV0           0x11 //remote voltage connected to ADC0
#define     MAX31785_PAGE_RV1           0x12 //remote voltage connected to ADC1
#define     MAX31785_PAGE_RV2           0x13 //remote voltage connected to ADC2
#define     MAX31785_PAGE_RV3           0x14 //remote voltage connected to ADC3
#define     MAX31785_PAGE_RV4           0x15 //remote voltage connected to ADC4
#define     MAX31785_PAGE_RV5           0x16 //remote voltage connected to ADC5
#define     MAX31785_PAGE_ALL           0xFF //all pages

#define MAX31785_CLEAR_FAULTS           0x03
#define MAX31785_WRITE_PROTECT          0x10
#define MAX31785_STORE_DEFAULT_ALL      0x11
#define MAX31785_RESTORE_DEFAULT_ALL    0x12
//#define MAX31785_CAPABILITY             0x19
//#define MAX31785_VOUT_MODE              0x20
#define MAX31785_VOUT_SCALE_MONITOR     0x2A
#define MAX31785_FAN_CONFIG_1_2         0x3A
#define     MAX31785_FAN_CONFIG_1_2_EN  0x80 //fan enable
#define     MAX31785_FAN_CONFIG_1_2_RPM 0x40 //rpm/notPWM
#define     MAX31785_FAN_CONFIG_1_2_TP  0x30 //tach pulses (1,2,3,4)
#define MAX31785_FAN_COMMAND_1          0x3B
#define MAX31785_VOUT_OV_FAULT_LIMIT    0x40
#define MAX31785_VOUT_OV_WARN_LIMIT     0x42
#define MAX31785_VOUT_UV_WARN_LIMIT     0x43
#define MAX31785_VOUT_UV_FAULT_LIMIT    0x44
#define MAX31785_OT_FAULT_LIMIT         0x4F
#define MAX31785_OT_WARN_LIMIT          0x51

#define MAX31785_STATUS_BYTE            0x78 //lower byte of status word
#define MAX31785_STATUS_WORD            0x79
#define     MAX31785_STATUS_WORD_NOTA        0x0001 //a status bit in range 15:8 is set
#define     MAX31785_STATUS_WORD_CML         0x0002 //Communication, memory or logic fault (I2C/command error)
#define     MAX31785_STATUS_WORD_TEMP        0x0004 //Temperature fault/warning (overtemperature)
#define     MAX31785_STATUS_WORD_VOUT_OV     0x0020 //overvoltage fault
#define     MAX31785_STATUS_WORD_FANS        0x0400 //fan fault/warning
#define     MAX31785_STATUS_WORD_MFR         0x1000 //check STATUS_MFR_SPECIFIC
#define     MAX31785_STATUS_WORD_VOUT        0x8000 //overvoltage fault/warning

#define MAX31785_STATUS_VOUT            0x7A
#define     MAX31785_STATUS_VOUT_OV_FAULT 0x80//overvoltage fault
#define     MAX31785_STATUS_VOUT_OV_WARN  0x40 //overvoltage warning
#define     MAX31785_STATUS_VOUT_UV_WARN  0x20 //undervoltage warning
#define     MAX31785_STATUS_VOUT_UV_FAULT 0x10//undervoltage fault

#define MAX31785_STATUS_CML             0x7E
#define     MAX31785_STATUS_CML_COMM_FAULT     0x80 //received invalid or unsupported command
#define     MAX31785_STATUS_CML_DATA_FAULT     0x40 //received invalid or unsupported data
#define     MAX31785_STATUS_CML_FAULT_LOG_FULL 0x01 //MFR_MV_FAULT_LOG is full and needs to be cleared

#define MAX31785_STATUS_MFR_SPECIFIC    0x80
#define     MAX31785_STATUS_MFR_SPECIFIC_OT_WARN  0x40 //overtemperature warning
#define     MAX31785_STATUS_MFR_SPECIFIC_OT_FAULT 0x20 //overtemperature fault
#define     MAX31785_STATUS_MFR_SPECIFIC_WATCHDOG 0x10 //watchdog reset occurred

#define MAX31785_STATUS_FANS_1_2         0x81
#define     MAX31785_STATUS_FANS_FAULT   0x80 //fan fault
#define     MAX31785_STATUS_FANS_WARN    0x20 //fan warning
#define     MAX31785_STATUS_FANS_RED     0x08 //bad health
#define     MAX31785_STATUS_FANS_ORANGE  0x04 //medium health
#define     MAX31785_STATUS_FANS_YELLOW  0x02 //unstable speed
#define     MAX31785_STATUS_FANS_GREEN   0x01 //good health
#define     MAX31785_STATUS_FANS_UNKNOWN 0x00 //status unknown

#define MAX31785_READ_VOUT              0x8B
#define MAX31785_READ_TEMPERATURE_1     0x8D
#define MAX31785_READ_FAN_SPEED_1       0x90

//#define MAX31785_PMBUS_REVISION         0x98
//#define MAX31785_MFR_ID                 0x99
//#define MAX31785_MFR_MODEL              0x9A
//#define MAX31785_MFR_REVISION           0x9B
//#define MAX31785_MFR_LOCATION           0x9C
//#define MAX31785_MFR_DATE               0x9D
//#define MAX31785_MFR_SERIAL             0x9E

#define MAX31785_MFR_MODE               0xD1
#define     MAX31785_MFR_MODE_FORCE_NVFL 0x8000 //force NV fault log
#define     MAX31785_MFR_MODE_CLEAR_NVFL 0x4000 //clear NV fault log
#define     MAX31785_MFR_MODE_ALERT      0x2000 //ENABLE ALERT/ARA
#define     MAX31785_MFR_MODE_SOFT_RESET 0x0800 //Soft reset: 1,0,1 within 8ms
//fan health criteria: evaluate difference between measured and expected speed
// GREEN: below lower limit, ORANGE: between lower and upper limit, RED: over upper limit
// -00: lo=10%, hi=15% small green and orange zones
// -01: lo=10%, hi=20% small green zone, big orange zone
// -10: lo=15%, hi=20% big green zone, small orange zone
// -11: lo=15%, hi=25% big green and orange zones
#define     MAX31785_MFR_MODE_FHC       0x00C0
#define     MAX31785_MFR_MODE_ADC_MASK  0x003F
#define     MAX31785_MFR_MODE_ADC5_EN   0x0020 //enable voltage sense 5 (overridden if temp sens is enabled)
#define     MAX31785_MFR_MODE_ADC4_EN   0x0010 //enable voltage sense 4(overridden if temp sens is enabled)
#define     MAX31785_MFR_MODE_ADC3_EN   0x0008 //enable voltage sense 3 (overridden if temp sens is enabled)
#define     MAX31785_MFR_MODE_ADC2_EN   0x0004 //enable voltage sense 2 (overridden if temp sens is enabled)
#define     MAX31785_MFR_MODE_ADC1_EN   0x0002 //enable voltage sense 1 (overridden if temp sens is enabled)
#define     MAX31785_MFR_MODE_ADC0_EN   0x0001 //enable voltage sense 0 (overridden if temp sens is enabled)

#define MAX31785_MFR_VOUT_PEAK          0xD4
#define MAX31785_MFR_TEMPERATURE_PEAK   0xD6
#define MAX31785_MFR_VOUT_MIN           0xD7

#define MAX31785_MFR_FAULT_RESPONSE     0xD9 //one per page
#define     MAX31785_MFR_FR_NV_LOG              0x80 //1=log fault in MFR_NV_FAULT_LOG
#define     MAX31785_MFR_FR_NV_LOG_OV           0x40 //1=log also overvoltage (pages 17:22 only - remote voltages)
#define     MAX31785_MFR_FR_UV_OV_FILTER        0x20 //1=need 2 events to set fault/warning (pages 17:22 only - remote voltages)
#define     MAX31785_MFR_FR_FAULT_PIN_ENABLE_OV 0x04 //1=enable fault pin also for overvoltage (pages 17:22 only - remote voltages)
#define     MAX31785_MFR_FR_FAULT_PIN_ENABLE    0x02 //1=enable FAULT# pin
#define     MAX31785_MFR_FR_FAULT_PIN_MONITOR   0x01 //1=force fan to 100% when FAULT# is asserted(pages 0:5 only - fans)

#define MAX31785_MFR_NV_FAULT_LOG       0xDC
#define MAX31785_MFR_TIME_COUNT         0xDD

#define MAX31785_MFR_TEMP_SENSOR_CONFIG 0xF0 //per page
#define     MAX31785_MFR_TCONFIG_ENABLE 0x8000 //enable current temperature sensor
#define     MAX31785_MFR_TCONFIG_OFFSET 0x7C00 //0:1E=offset in Celsius degrees,1F=test mode
#define     MAX31785_MFR_TCONFIG_FAN5   0x0020 //current sensor is used to control fan5
#define     MAX31785_MFR_TCONFIG_FAN4   0x0010 //current sensor is used to control fan4
#define     MAX31785_MFR_TCONFIG_FAN3   0x0008 //current sensor is used to control fan3
#define     MAX31785_MFR_TCONFIG_FAN2   0x0004 //current sensor is used to control fan2
#define     MAX31785_MFR_TCONFIG_FAN1   0x0002 //current sensor is used to control fan1
#define     MAX31785_MFR_TCONFIG_FAN0   0x0001 //current sensor is used to control fan0

#define MAX31785_MFR_FAN_CONFIG         0xF1
#define     MAX31785_MFR_FAN_CONFIG_FREQ        0xE000 //PWM frequency 0=30Hz, 1=50Hz, 2=100Hz, 3=150Hz, 7=25kHz
#define     MAX31785_MFR_FAN_CONFIG_DUAL_TACH   0x1000 //enable dual tachometer
#define     MAX31785_MFR_FAN_CONFIG_HYS         0x0C00 //hysteresis for auto slow down 0,1,2,3=2,4,6,8°C
#define     MAX31785_MFR_FAN_CONFIG_TSFO        0x0200 //0=ramp to 100% on sensor fault/no fan_command update, 1=on fault use last speed value
#define     MAX31785_MFR_FAN_CONFIG_TACHO       0x0100 //0=ramp to 100% on fan fault, 1=don't
#define     MAX31785_MFR_FAN_CONFIG_RAMP        0x00E0 //time to ramp 40% to 100%: (0,1,2,3,4,5,6,7)=(60,30,20,12,6,4,3,2.4 seconds)
#define     MAX31785_MFR_FAN_CONFIG_HEALTH      0x0010 //enable fan health check
#define     MAX31785_MFR_FAN_CONFIG_ROTOR_HI_LO 0x0008 //polarity of TACK for stopped rotor: 0=low, 1=high
#define     MAX31785_MFR_FAN_CONFIG_ROTOR       0x0004 //0=TACH connected to actual tachometer, 1=TACH input used as stuck rotor signal
#define     MAX31785_MFR_FAN_CONFIG_SPIN        0x0003 //start behavior: 0=no spinup control, 1=100% for 2 revolutions, 2=100% for 4 revolutions, 3=100% for 8 revolutions

#define MAX31785_MFR_FAN_LUT            0xF2
#define MAX31785_MFR_READ_FAN_PWM       0xF3
#define MAX31785_MFR_FAN_FAULT_LIMIT    0xF5
#define MAX31785_MFR_FAN_WARN_LIMIT     0xF6
#define MAX31785_MFR_FAN_RUN_TIME       0xF7
#define MAX31785_MFR_FAN_PWM_AVG        0xF8
#define MAX31785_MFR_FAN_PWM2RPM        0xF9


/*
 * Data conversion functions
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
int16_t max31785_raw2volt(int16_t Y);
float   max31785_raw2vscale(int16_t Y);
float   max31785_raw2temp(int16_t Y);
int16_t max31785_raw2rpm(int16_t Y);
float   max31785_raw2pwm(int16_t Y);

int16_t max31785_volt2raw(int16_t X);
int16_t max31785_vscale2raw(float X);
int16_t max31785_temp2raw(float X);
int16_t max31785_rpm2raw(int16_t X);
int16_t max31785_pwm2raw(float X);



/********************** GLOBAL COMMANDS (do not depend on page) **********************/
/*
 * Select page -------------------------------------------------------
 * See .h file for page codes
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout, 3=page not valid
 * DAFAULT = 0x00
 */
uint8_t max31785_page(void* i2c, uint8_t i2c_addr, uint8_t page);

/*
 * Clear all fault flags -----------------------------------------------
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_clear_faults(void* i2c, uint8_t i2c_addr);

/*
 * Write protect -------------------------------------------------------
 * Accepted values:
 *     0 = no protection (DEFAULT)
 *     1 = only WRITE_PROTECT and PAGE commands allowed
 *     2 = only WRITE_PROTECT command allowed
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout, 3=value not valid
 */
uint8_t max31785_write_protect(void* i2c, uint8_t i2c_addr, uint8_t val);

/*
 * Store configuration data to internal FLASH memory--------------------
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_store_default_all(void* i2c, uint8_t i2c_addr);

/*
 * Restore configuration data from internal FLASH memory---------------------------------------------------------------------------------------
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_restore_default_all(void* i2c, uint8_t i2c_addr);

//uint8_t max31785_capability(void* i2c, uint8_t i2c_addr); //not needed
//uint8_t max31785_vout_mode(void* i2c, uint8_t i2c_addr); //not needed: always returns 0x40

/*
 * get status word ************************************************************
 * Use following masks:
 * MAX31785_STATUS_WORD_NOTA    0x0001 //a status bit in range 15:8 is set
 * MAX31785_STATUS_WORD_CML     0x0002 //Communication, memory or logic fault (I2C/command error)
 * MAX31785_STATUS_WORD_TEMP    0x0004 //Temperature fault/warning (overtemperature)
 * MAX31785_STATUS_WORD_VOUT_OV 0x0020 //overvoltage fault
 * MAX31785_STATUS_WORD_FANS    0x0400 //fan fault/warning
 * MAX31785_STATUS_WORD_MFR     0x1000 //check STATUS_MFR_SPECIFIC
 * MAX31785_STATUS_WORD_VOUT    0x8000 //overvoltage fault/warning
 * if return value is 0xFFFF then an i2c error occurred
 */
uint16_t max31785_get_status(void* i2c, uint8_t i2c_addr);


/*
 * get status for i2c interface **************************************************************************************
 * To be read when STATUS_WORD_CML is set.
 * Use following masks:
 * MAX31785_STATUS_CML_COMM_FAULT     //received invalid or unsupported command
 * MAX31785_STATUS_CML_DATA_FAULT     //received invalid or unsupported data
 * MAX31785_STATUS_CML_FAULT_LOG_FULL //MFR_MV_FAULT_LOG is full and needs to be cleared
 * if return value is 0xFF then an i2c error occurred
 */
uint8_t max31785_get_status_cml(void* i2c, uint8_t i2c_addr);


//following functions are not implemented since they always return the same value
//uint8_t max31785_pmbus_revision(void* i2c, uint8_t i2c_addr); //0x11
//uint8_t max31785_mfr_id(void* i2c, uint8_t i2c_addr); //0x4D
//uint8_t max31785_mfr_model(void* i2c, uint8_t i2c_addr); //0x53
//uint16_t max31785_mfr_revision(void* i2c, uint8_t i2c_addr); //0x3030

//functions to set/get info strings: not implemented
//uint8_t max31785_set_mfr_location(void* i2c, uint8_t i2c_addr, uint8_t* string); //loads a 8char string (def= 0x3130_3130_3130_3130)
//uint8_t max31785_set_mfr_date(void* i2c, uint8_t i2c_addr, uint8_t* string); //loads a 8char string (def= 0x3130_3130_3130_3130)
//uint8_t max31785_set_mfr_serial(void* i2c, uint8_t i2c_addr, uint8_t* string); //loads a 8char string (def= 0x3130_3130_3130_3130)

//MFR_MODE REGISTER (following functions perform a Read-modify-write)
//fault log control functions not implemented

/*
 * set enable bit for alert function (DEFAULT OFF)
 * when ALERT is on, faults will assert the ALERT# bit, and device will respond only to 7b'0C/8b'18 ARA address
 */
uint8_t max31785_alert_en(void* i2c, uint8_t i2c_addr, uint8_t en);

/*
 * send sequence to perform soft reset (1,0,1)
 */
uint8_t max31785_soft_reset(void* i2c, uint8_t i2c_addr);

/*
 * set fan health criteria
 * GREEN/ORANGE/RED flag is set according to difference (D) between real and measured fan speed (in %).
 * Profiles are encoded as follows (accepted values: 0:3):
 * 0: difference D<10% GREEN, 10%<D<15% ORANGE, >15% RED (DEFAULT)
 * 1: difference D<10% GREEN, 10%<D<20% ORANGE, >20% RED
 * 2: difference D<15% GREEN, 15%<D<20% ORANGE, >20% RED
 * 3: difference D<15% GREEN, 15%<D<25% ORANGE, >25% RED
 */
uint8_t max31785_fan_health_criteria(void* i2c, uint8_t i2c_addr, uint8_t fhc);

/*
 * enable voltage measurements on channels (DEFAULT ALL OFF)
 * mask = 6 bit value (channel 5:0 enable status)
 */
uint8_t max31785_voltage_sense_en(void* i2c, uint8_t i2c_addr, uint8_t mask);

/*
 * get fault log for current page
 * returns 255Bytes
 */
//uint8_t max31785_get_mfr_nv_fault_log(void* i2c, uint8_t i2c_addr, uint8_t* buf256);

/*
 * get device total lifetime in seconds
 */
uint32_t max31785_get_mfr_time_count(void* i2c, uint8_t i2c_addr);


/*****************************************************************************************
 * CHANNEL SPECIFIC SETTINGS (require page to be specified)
 ****************************************************************************************/

/*
 * Vout scale monitor -------------------------------------------------------
 * Adc max range for voltage measure is 1.225V.
 * It's recommended to scale the voltage with a resistive divider, so that remote voltage=1.0V at ADC.
 * VAL = resistive divider ratio mutiplied by 32767. Accepted range: [0:32767], DEFAULT=0x7FFF (DIV=1)
 * VALID PAGES = 17-22 (ADC remote voltages)
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout, 3=value not valid
 */
uint8_t max31785_vout_scale_monitor(void* i2c, uint8_t i2c_addr, uint8_t page, uint16_t val);

//following 3 functions all access FAN_CONFIG_1_2
/*
 * Fan enable (FAN_CONFIG_1_2) ---------------------------------------------
 * If en=0, FAN is disabled, otherwise it is enabled. DEFAULT OFF
 * VALID PAGES = 0-5 (fans)
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_fan_enable(void* i2c, uint8_t i2c_addr, uint8_t page, uint8_t en);

/*
 * Select RPM or PWM% as controlling parameter (FAN_CONFIG_1_2) -----------------------------
 * If RPM_notPWM=0 then PWM is selected, otherwise RPM is selected (DEFAULT PWM)
 * VALID PAGES = 0-5 (fans)
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_fan_rpm(void* i2c, uint8_t i2c_addr, uint8_t page, uint8_t RPM_notPWM);

/*
 * Select how many TACH pulses per FAN revolution (FAN_CONFIG_1_2) -----------------------------
 * Valid input range [1:4] (DEFAULT 1)
 * VALID PAGES = 0-5 (fans)
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout, 3=pulses value not valid
 */
uint8_t max31785_fan_tach_pulses(void* i2c, uint8_t i2c_addr, uint8_t page, uint8_t npulses);


/*
 * Enable automatic control (FAN_COMMAND_1) -----------------------------
 * DEFAULT = ENABLED
 * VALID PAGES = 0-5 (fans)
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_fan_auto_en(void* i2c, uint8_t i2c_addr, uint8_t page);

/*
 * Force %PWM value (FAN_COMMAND_1, disable automatic control) -----------------------------
 * WARNING: device shall be set to PWM with function max31785_fan_rpm()
 * Valid input range [0.00:100.00] (100.00 => 10000 = 0x2710)
 * VALID PAGES = 0-5 (fans)
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout, 3=input value not valid
 */
uint8_t max31785_fan_force_pwm(void* i2c, uint8_t i2c_addr, uint8_t page, float pwm);

/*
 * Force %PWM value (FAN_COMMAND_1, disable automatic control) -----------------------------
 * WARNING: device shall be set to RPM with function max31785_fan_rpm()
 * Valid input range [0:32767]
 * VALID PAGES = 0-5 (fans)
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout, 3=input value not valid
 */
uint8_t max31785_fan_force_rpm(void* i2c, uint8_t i2c_addr, uint8_t page, int16_t rpm);

/*******************************************************************************************************
 * SET LIMITS
 *******************************************************************************************************/

/*
 * Set warning/fault limits on voltages
 * Input range: FULL (use voltage conversion functions)
 * VALID PAGES = 17-22 (ADC remote voltages)
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_vout_ov_fault_limit(void* i2c, uint8_t i2c_addr, uint8_t page, int16_t val); //overvoltage fault, default=MAX VOLTAGE
uint8_t max31785_vout_ov_warn_limit( void* i2c, uint8_t i2c_addr, uint8_t page, int16_t val); //overvoltage warning, default=MAX VOLTAGE
uint8_t max31785_vout_uv_fault_limit(void* i2c, uint8_t i2c_addr, uint8_t page, int16_t val); //undervoltage fault, default=MIN VOLTAGE
uint8_t max31785_vout_uv_warn_limit( void* i2c, uint8_t i2c_addr, uint8_t page, int16_t val); //undervoltage warning, default=MIN VOLTAGE

/*
 * Set warning/fault limits on temperatures
 * Input range: FULL (use temperature conversion functions)
 * VALID PAGES = 6-16 (temperature sensors)
 * Return value: 0=ok, 1=i2c error, 2=i2c timeout
 */
uint8_t max31785_vout_ot_fault_limit(void* i2c, uint8_t i2c_addr, uint8_t page, int16_t val); //overtemperature fault, default=MAX TEMP
uint8_t max31785_vout_ot_warn_limit( void* i2c, uint8_t i2c_addr, uint8_t page, int16_t val); //overtemperature warning, default=MAX TEMP

/*******************************************************************************************************
 * GET STATUS
 *******************************************************************************************************/

/*
 * get status for voltage monitors ********************************************************
 * To be read when STATUS_WORD_VOUT is set.
 * VALID PAGES = 17-22 (ADC remote voltages)
 * Use following masks:
 * MAX31785_STATUS_VOUT_OV_FAULT //overvoltage fault
 * MAX31785_STATUS_VOUT_OV_WARN  //overvoltage warning
 * MAX31785_STATUS_VOUT_UV_FAULT //undervoltage fault
 * MAX31785_STATUS_VOUT_UV_WARN  //undervoltage warning
 * if return value is 0xFF then an i2c error occurred
 */
uint8_t max31785_get_status_vout(void* i2c, uint8_t i2c_addr, uint8_t page);


/*
 * get status for temperature/watchdog **************************************************************************************
 * To be read when STATUS_WORD_TEMP is set.
 * VALID PAGES = 6-16 (temperature sensors)
 * Use following masks:
 * MAX31785_STATUS_MFR_SPECIFIC_OT_WARN  //overtemperature warning
 * MAX31785_STATUS_MFR_SPECIFIC_OT_FAULT //overtemperature fault
 * MAX31785_STATUS_MFR_SPECIFIC_WATCHDOG //watchdog reset occurred
 * if return value is 0xFF then an i2c error occurred
*/
uint8_t max31785_get_status_mfr_specific(void* i2c, uint8_t i2c_addr, uint8_t page);

/*
 * get status for fans **************************************************************************************
 * To be read when STATUS_WORD_FANS is set.
 * VALID PAGES = 0-5 (fans)
 * Use following masks:
 * MAX31785_STATUS_FANS_FAULT  //fan fault
 * MAX31785_STATUS_FANS_WARN   //fan warning
 * Following values shall be compared with lower nibble (not masked)
 * MAX31785_STATUS_FANS_RED    //very bad health
 * MAX31785_STATUS_FANS_ORANGE //bad health
 * MAX31785_STATUS_FANS_YELLOW //good health
 * MAX31785_STATUS_FANS_GREEN  //very good health
 * MAX31785_STATUS_FANS_UNKNOWN  //status unknown
 * if return value is 0xFF then an i2c error occurred
*/
uint8_t max31785_get_status_fans(void* i2c, uint8_t i2c_addr, uint8_t page);

/*******************************************************************************************************
 * READ MEASURES (must select page first)
 *******************************************************************************************************/

/*
 * read voltage
 * VALID PAGES = 18-22 (remote voltages)
 * returns raw data. shall be converted with raw2voltage(),
 */
int16_t max31785_read_vout(void* i2c, uint8_t i2c_addr, uint8_t page);

/*
 * read temperature
 * VALID PAGES = 6-17 (temp sensors)
 * returns raw data. shall be converted with raw2temp(),
 * 0x7FFF = FAULTY SENSOR, 0x0000 = SENSOR DISABLED
 */
int16_t max31785_read_temperature(void* i2c, uint8_t i2c_addr, uint8_t page);

/*
 * read fan speed
 * VALID PAGES = 0-5 (fans)
 * returns raw data. shall be converted with raw2rpm() or raw2pwm() according to value set by fan_rpm().
 */
int16_t max31785_read_fan_speed(void* i2c, uint8_t i2c_addr, uint8_t page);



/*******************************************************************************************************
 * MAX/MIN MONITORS (per page)
 *******************************************************************************************************/

/*
 * read/reset voltage max/min
 * VALID PAGES = 17-22 (remote voltages)
 * returns raw data. shall be converted with raw2voltage()
 */
int16_t max31785_get_voltage_peak(  void* i2c, uint8_t i2c_addr, uint8_t page);
uint8_t max31785_reset_voltage_peak(void* i2c, uint8_t i2c_addr, uint8_t page);
int16_t max31785_get_voltage_min(   void* i2c, uint8_t i2c_addr, uint8_t page);
uint8_t max31785_reset_voltage_min( void* i2c, uint8_t i2c_addr, uint8_t page);

/*
 * read/reset temperature max/min
 * VALID PAGES = 6-16 (temp sensors)
 * returns raw data. shall be converted with raw2temperature()
 */
int16_t max31785_get_temperature_peak(  void* i2c, uint8_t i2c_addr, uint8_t page);
uint8_t max31785_reset_temperature_peak(void* i2c, uint8_t i2c_addr, uint8_t page);


/*
 * Set the MFR_FAULT_RESPONSE register
 * VALID PAGES = ALL (except 0xFF)
 * DEFAULT = ALL OFF
 * MAX31785_MFR_FR_NV_LOG              //1=log fault in MFR_NV_FAULT_LOG
 * MAX31785_MFR_FR_NV_LOG_OV           //1=log also overvoltage (pages 17:22 only - remote voltages)
 * MAX31785_MFR_FR_UV_OV_FILTER        //1=need 2 events to set fault/warning (pages 17:22 only - remote voltages)
 * MAX31785_MFR_FR_FAULT_PIN_ENABLE_OV //1=enable fault pin also for overvoltage (pages 17:22 only - remote voltages)
 * MAX31785_MFR_FR_FAULT_PIN_ENABLE    //1=enable FAULT# pin
 * MAX31785_MFR_FR_FAULT_PIN_MONITOR   //1=force fan to 100% when FAULT# is asserted(pages 0:5 only - fans)
 */
uint8_t max31785_set_mfr_fault_response(void* i2c, uint8_t i2c_addr, uint8_t page, uint8_t reg);

/*
 * config current temp sensor
 * VALID PAGES = 6-16 (temp sensors)
 * MAX31785_MFR_TCONFIG_ENABLE 0x8000 //enable current temperature sensor
 * MAX31785_MFR_TCONFIG_OFFSET 0x7C00 //0:1E=offset in Celsius degrees,1F=test mode
 * MAX31785_MFR_TCONFIG_FAN5   0x0020 //current sensor is used to control fan5
 * MAX31785_MFR_TCONFIG_FAN4   0x0010 //current sensor is used to control fan4
 * MAX31785_MFR_TCONFIG_FAN3   0x0008 //current sensor is used to control fan3
 * MAX31785_MFR_TCONFIG_FAN2   0x0004 //current sensor is used to control fan2
 * MAX31785_MFR_TCONFIG_FAN1   0x0002 //current sensor is used to control fan1
 * MAX31785_MFR_TCONFIG_FAN0   0x0001 //current sensor is used to control fan0
 */
uint8_t max31785_set_mfr_temp_sensor_config(void* i2c, uint8_t i2c_addr, uint8_t page, uint16_t reg);

/*
 * config current fan (select page first)
 * VALID PAGES = 0-5 (fans)
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
uint8_t max31785_set_mfr_fan_config(void* i2c, uint8_t i2c_addr, uint8_t page, uint16_t reg);
uint16_t max31785_get_mfr_fan_config(void* i2c, uint8_t i2c_addr, uint8_t page);

/*
 * write/read MFR_FAN_LUT
 * VALID PAGES = 0-5 (fans)
 * input: 32 bytes, 8 programmable temperature levels (4 bytes each)
 * each programmable level contains a 16bit temperature and a 16bit speed (%PWM or speed according to fan settings)
 * valid temperature ranges are:
 *     [-40; +85] for internal temperature sensor
 *     [-55;+125] for I2C temperature sensor
 *     [-40;+120] for thermal diode temperature sensor
 */
//uint8_t max31785_set_mfr_fan_lut(void* i2c, uint8_t i2c_addr, uint8_t* buf32);
//uint8_t max31785_get_mfr_fan_lut(void* i2c, uint8_t i2c_addr, uint8_t* buf32);

/*
 * get current PWM of selected FAN
 * VALID PAGES = 0-5 (fans)
 */
int16_t max31785_mfr_read_fan_pwm(void* i2c, uint8_t i2c_addr, uint8_t page);

/*
 * set FAN fault limit for currently selected FAN
 * VALID PAGES = 0-5 (fans)
 * input value: RPM/PWM according to mode. 0x0000=disable checking.
 * when the TACH measures less than the limit for 10s, the FAN_FAULT flag is set.
 */
uint8_t max31785_mfr_fan_fault_limit(void* i2c, uint8_t i2c_addr, uint8_t page, uint16_t limit);

/*
 * set FAN warning limit for currently selected FAN
 * VALID PAGES = 0-5 (fans)
 * input value: RPM/PWM according to mode. 0x0000=disable checking.
 * when the TACH measures less than the limit for 10s, the FAN_WARN flag is set.
 */
uint8_t max31785_mfr_fan_warn_limit(void* i2c, uint8_t i2c_addr, uint8_t page, uint16_t limit);


/*
 * get run time in HOURS of selected FAN
 * VALID PAGES = 0-5 (fans)
 */
uint16_t max31785_mfr_fan_run_time(void* i2c, uint8_t i2c_addr, uint8_t page);

/*
 * get average %PWM over current FAN's lifetime
 * VALID PAGES = 0-5 (fans)
 */
uint16_t max31785_mfr_fan_pwm_avg(void* i2c, uint8_t i2c_addr, uint8_t page);

/*
 * write table containing relation between fan speed and PWM for current FAN.
 * VALID PAGES = 0-5 (fans)
 * Used to monitor fan's health.
 * Table contains 4 16bit RPM values (LSB lower position):
 * Bytes 0-1: RPM at 40%  PWM
 * Bytes 2-3: RPM at 60%  PWM
 * Bytes 4-5: RPM at 80%  PWM
 * Bytes 6-7: RPM at 100% PWM
 */
uint8_t max31785_mfr_fan_pwm2rpm(void* i2c, uint8_t i2c_addr, uint8_t page, const uint8_t* table);

uint8_t max31785_force_nv_fault_log(void* i2c, uint8_t i2c_addr, uint8_t en);
uint8_t max31785_clear_nv_fault_log(void* i2c, uint8_t i2c_addr, uint8_t en);

/* generic read/write functions (mainly for debugging) */
uint8_t max31785_read(void* i2c, uint8_t i2c_addr, uint8_t reg, uint8_t page, uint8_t* wdata, uint8_t size);

#endif /* MAX31785_H_ */
