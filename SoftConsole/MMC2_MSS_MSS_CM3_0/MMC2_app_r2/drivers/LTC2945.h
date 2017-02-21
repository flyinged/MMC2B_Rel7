/*
 * LTC2945.h
 *
 *  Created on: 17.07.2015
 *      Author: malatesta_a
 */

#ifndef LTC2945_H_
#define LTC2945_H_

#include "hal.h"
#include "core_i2c.h"

#define LTC2945_SNS_LSBV 25  //SENSE lsb value, microvolts
#define LTC2945_VIN_LSBV 25  //VIN lsb value, millivolts
#define LTC2945_AIN_LSBV 500 //ADIN lsb, microvolts

#define LTC2945_CTRL            0x00
#define     LTC2945_CTRL_SNAP_EN 0x80 //enable snapshot mode
#define     LTC2945_CTRL_SNAP_LB 0x60 //channel label for snapshot mode 00=SENSE, 01=VIN, 10=ADIN
#define     LTC2945_CTRL_TEST_EN 0x10 //test mode enable
#define     LTC2945_CTRL_SNAP_BS 0x08 //busy (read only, snapshot mode)
#define     LTC2945_CTRL_VIN_MON 0x04 //voltage monitoring Vin: 1=SENSE+ (default), 0=VDD
#define     LTC2945_CTRL_SD_EN   0x02 //shutdown (0= normal operation)
#define     LTC2945_CTRL_MULT    0x01 //select power multiplier: 0=ADIN, 1=Vin=SENSE+/VDD (default)
#define LTC2945_ALERT           0x01 //enable various alerts (max/min defined by threshold registers)
#define 	LTC2945_ALERT_MAXP 0x80 //power > max
#define 	LTC2945_ALERT_MINP 0x40 //power < min
#define 	LTC2945_ALERT_MAXS 0x20 //sense > max
#define 	LTC2945_ALERT_MINS 0x10 //sense < min
#define 	LTC2945_ALERT_MAXV 0x08 //VIN   > max
#define 	LTC2945_ALERT_MINV 0x04 //VIN   < min
#define 	LTC2945_ALERT_MAXA 0x02 //ADIN  > max
#define 	LTC2945_ALERT_MINA 0x01 //ADIN  < min
#define LTC2945_STATUS          0x02 //actual alerts
#define 	LTC2945_STATUS_PWR_OV 0x80 //power over max
#define 	LTC2945_STATUS_PWR_UV 0x40 //power under min
#define 	LTC2945_STATUS_SNS_OV 0x20 //SENSE over max
#define 	LTC2945_STATUS_SNS_UV 0x10 //SENSE under min
#define 	LTC2945_STATUS_VIN_OV 0x08 //VIN over max
#define 	LTC2945_STATUS_VIN_UV 0x04 //VIN under min
#define 	LTC2945_STATUS_AIN_OV 0x02 //ADIN over max
#define 	LTC2945_STATUS_AIN_UV 0x01 //ADIN under min
#define LTC2945_FAULT           0x03 //past alerts
#define 	LTC2945_FAULT_PWR_OV 0x80 //power over max
#define 	LTC2945_FAULT_PWR_UV 0x40 //power under min
#define 	LTC2945_FAULT_SNS_OV 0x20 //SENSE over max
#define 	LTC2945_FAULT_SNS_UV 0x10 //SENSE under min
#define 	LTC2945_FAULT_VIN_OV 0x08 //VIN over max
#define 	LTC2945_FAULT_VIN_UV 0x04 //VIN under min
#define 	LTC2945_FAULT_AIN_OV 0x02 //ADIN over max
#define 	LTC2945_FAULT_AIN_UV 0x01 //ADIN under min
#define LTC2945_FAULT_COR       0x04
/* ALL 12 bit values have 4LSB don't care */
/* power measures 24 bit (can be written only in test mode) */
#define LTC2945_PWR_MSB2        0x05
#define LTC2945_PWR_MSB1        0x06
#define LTC2945_PWR_LSB         0x07
#define LTC2945_MAXP_MSB2       0x08
#define LTC2945_MAXP_MSB1       0x09
#define LTC2945_MAXP_LSB        0x0A
#define LTC2945_MINP_MSB2       0x0B
#define LTC2945_MINP_MSB1       0x0C
#define LTC2945_MINP_LSB        0x0D
/* power thresholds 24 bit */
#define LTC2945_MAXP_TH_MSB2    0x0E
#define LTC2945_MAXP_TH_MSB1    0x0F
#define LTC2945_MAXP_TH_LSB     0x10
#define LTC2945_MINP_TH_MSB2    0x11
#define LTC2945_MINP_TH_MSB1    0x12
#define LTC2945_MINP_TH_LSB     0x13
/* sense voltage measures 12 bit (can be written only in test mode) */
#define LTC2945_SENSE_MSB       0x14
#define LTC2945_SENSE_LSB       0x15
#define LTC2945_MAX_SNS_MSB     0x16
#define LTC2945_MAX_SNS_LSB     0x17
#define LTC2945_MIN_SNS_MSB     0x18
#define LTC2945_MIN_SNS_LSB     0x19
/* sense voltage thresholds 12 bit */
#define LTC2945_MAX_SNS_TH_MSB  0x1A
#define LTC2945_MAX_SNS_TH_LSB  0x1B
#define LTC2945_MIN_SNS_TH_MSB  0x1C
#define LTC2945_MIN_SNS_TH_LSB  0x1D
/* input voltage measures 12 bit (can be written only in test mode) */
#define LTC2945_VIN_MSB         0x1E
#define LTC2945_VIN_LSB         0x1F
#define LTC2945_MAX_VIN_MSB     0x20
#define LTC2945_MAX_VIN_LSB     0x21
#define LTC2945_MIN_VIN_MSB     0x22
#define LTC2945_MIN_VIN_LSB     0x23
/* input voltage thresholds 12 bit */
#define LTC2945_MAX_VIN_TH_MSB  0x24
#define LTC2945_MAX_VIN_TH_LSB  0x25
#define LTC2945_MIN_VIN_TH_MSB  0x26
#define LTC2945_MIN_VIN_TH_LSB  0x27
/* generic input measures 12 bit (can be written only in test mode) */
#define LTC2945_ADIN_MSB        0x28
#define LTC2945_ADIN_LSB        0x29
#define LTC2945_MAX_ADIN_MSB    0x2A
#define LTC2945_MAX_ADIN_LSB    0x2B
#define LTC2945_MIN_ADIN_MSB    0x2C
#define LTC2945_MIN_ADIN_LSB    0x2D
/* generic input thresholds 12 bit */
#define LTC2945_MAX_ADIN_TH_MSB 0x2E
#define LTC2945_MAX_ADIN_TH_LSB 0x2F
#define LTC2945_MIN_ADIN_TH_MSB 0x30
#define LTC2945_MIN_ADIN_TH_LSB 0x31

uint32_t LTC2945_read_reg(void *i2c, uint8_t i2c_addr, uint8_t reg_addr, uint8_t nbytes);
void LTC2945_write_reg(void *i2c, uint8_t i2c_addr, uint8_t reg_addr, uint32_t val, uint8_t nbytes);
float LTC2945_raw_to_power(uint32_t raw_power, float Rshunt_mohm);
float LTC2945_raw_to_current(uint32_t raw_voltage, float Rshunt_mohm);
float LTC2945_raw_to_voltage(uint16_t raw_voltage);
float LTC2945_raw_to_adin(uint16_t raw_voltage, float mult);
void  LTC2945_display(void *i2c_inst, uint32_t I2C_SER_ADDR, uint8_t simple, float SR_mohm, float mult);
void LTC2945_report(void *i2c_inst, uint32_t I2C_SER_ADDR, float SR_mohm, const uint8_t *head, float mult);
void LTC2945_reset_limits(void *i2c_inst, uint32_t I2C_SER_ADDR, uint8_t reg_addr, uint8_t size);
void LTC2945_reset_all_limits(void *i2c_inst, uint32_t I2C_SER_ADDR);

extern i2c_status_t core_i2c_dowrite(
		i2c_instance_t *i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		const uint8_t *msg);
extern i2c_status_t core_i2c_doread(
		i2c_instance_t * i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		uint8_t * rx_buffer, uint8_t read_length,
		const uint8_t *msg);

#endif /* LTC2945_H_ */
