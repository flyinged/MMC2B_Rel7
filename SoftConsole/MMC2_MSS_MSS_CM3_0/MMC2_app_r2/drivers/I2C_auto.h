/*
 * I2C_auto.h
 * FPGA core that automatically reads from I2C devices and updates a register bank.
 *
 *  Created on: 18.11.2015
 *      Author: malatesta_a
 */

#ifndef I2C_AUTO_H_
#define I2C_AUTO_H_

/* Register offsets (read only) */
#define I2C_AUTO_T_FO     0x00u /* T112 sensor, front air outlet */
#define I2C_AUTO_T_RO     0x04u /* T112 sensor, rear air outlet */
#define I2C_AUTO_T_PB     0x08u /* T112 sensor, power board */
#define I2C_AUTO_T_IL     0x0Cu /* T112 sensor, air inlet */
#define I2C_AUTO_T_HS     0x10u /* TC74 sensor, heatsink */
#define I2C_AUTO_HEAT     0x14u /* AD5321 DAC, heatsink control */
#define I2C_AUTO_GPO      0x18u /* GPO current outputs */
#define I2C_AUTO_GPI      0x1Cu /* GPI current inputs */
#define I2C_AUTO_RTC_TIME 0x20u /* RTC time register */
#define I2C_AUTO_RTC_DATE 0x24u /* RTC date register */
#define I2C_AUTO_ETC_CFG  0x28u /* ETC config register */
#define I2C_AUTO_ETC_ALC  0x2Cu /* ETC alarm counter */
#define I2C_AUTO_ETC_ETC  0x30u /* ETC elapsed time counter */
#define I2C_AUTO_ETC_EVC  0x34u /* ETC event counter */

//following are not automatically updated
#define I2C_AUTO_GPO_DIR 0x38u /* GPO dir */
#define I2C_AUTO_GPI_DIR 0x3Cu /* GPI dir */
#define I2C_AUTO_RTC_CR  0x40u /* RTC control register */
/* currently (mask):
 *   FSM 0x0000000F
 *   cmd_valid_a 0x00000010
 *   cmd_valid_x 0x00000020
 *   fifo_empty  0x00000040
 *   fifo_full   0x00000080
 *   periodic    0x00000100
 *   busy        0x00000200
 *   update      0x00000400
 *   ram_addr    0x007F0000
 */



/* Command register offset (write only)
 * Writing to this register pushes a command into the FIFO
 * Bit 10 is the read-not-write flag
 * Bits 9:0 are the index of the ROM location containing the I2C details
 * >>>>>>>>>>>>>>>>>>>>>WARNING: ROM INDEX = ADDRESS_OFFSET/4 <<<<<<<<<<<<<<<<<<<<<<<<
 * Written data (for writes) is taken from I2C_AUTO_DATA register
 *
 * Example write transaction:
 * 1. write <DATA> to address I2C_AUTO_DAT
 * 2. write (~I2C_AUTO_CMD_RNW & (<ID>&I2C_AUTO_CMD_ID)) to address I2C_AUTO_CMD
 *
 * Example read transaction
 * 1. write (I2C_AUTO_CMD_RNW | (<ID>&I2C_AUTO_CMD_ID)) to address I2C_AUTO_CMD
 * 2. read result from address <ID>
 */
#define I2C_AUTO_CMD 0x80u
#define     I2C_AUTO_CMD_RNW 0x400u
#define     I2C_AUTO_CMD_ID  0x3FFu
#define I2C_AUTO_DAT 0x84u
#define I2C_AUTO_ENA 0x88u
#define I2C_AUTO_DIS 0x8Cu

#endif /* I2C_AUTO_H_ */
