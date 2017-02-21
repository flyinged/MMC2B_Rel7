/*
 * FAN_controller.h
 * Firmware block that implements PID controller
 *
 *  Created on: 16.11.2015
 *      Author: malatesta_a
 */

#ifndef FAN_CONTROLLER_H_
#define FAN_CONTROLLER_H_

//BASE ADDR IS FAN_CONTROLLER_0 from MMC2_hw_platform.h

//debug code
// seq(8) | ms_enc(5) | ts(3) | i2c_s(8) | temp(15:8)

/* temperature MSB */
#define FC_TMP_MASK  0xFF
#define FC_TMP_SHIFT 0

/* I2C FSM status */
#define FC_IFS_MASK  0xFF
#define FC_IFS_SHIFT 8
//status encoding
#define FC_IFS_START   0x08 //start has been sent
#define FC_IFS_RESTART 0x10 //repeated start has been sent
#define FC_IFS_STOP    0xE0 //stop has been sent
#define FC_IFS_SLAW_A  0x18 //slave write addr sent, ack received
#define FC_IFS_SLAW_NA 0x20 //slave write addr sent, nack received
#define FC_IFS_TXD_A   0x28 //data sent, ack received
#define FC_IFS_TXD_NA  0x30 //data sent, nack received
#define FC_IFS_ARB_LST 0x38 //arbitration lost in SLAW/SLAR/TXD
#define FC_IFS_SLAR_A  0x40 //slave read addr sent, ack received
#define FC_IFS_SLAR_NA 0x48 //slave read addr sent, nack received
#define FC_IFS_RXD_A   0x50 //data received, ack returned
#define FC_IFS_RXD_NA  0x58 //data received, nack returned

/* transaction status */
#define FC_TS_MASK  0x7
#define FC_TS_SHIFT 16
//transaction_status encoding
#define TS_UNKNOWN 0
#define TS_RUNNING 1
#define TS_SUCCESS 2
#define TS_FAILURE 3
#define TS_TIMEOUT 4

const char *fc_ts_name[] = { "UNKNOWN ",
                             "RUNNING ",
                             "SUCCESS ",
                             "FAILURE ",
                             "TIMEOUT ",
                             "N_VALID "};

/* main status */
#define FC_MS_MASK  0x1F
#define FC_MS_SHIFT 19
//main encoding
#define FC_MS_INIT_0      0x00
#define FC_MS_INIT_1      0x01
#define FC_MS_INIT_2      0x02
#define FC_MS_IDLE        0x03
#define FC_MS_WAIT_DONE   0x04
#define FC_MS_INIT_STATUS 0x05
#define FC_MS_READ_STATUS 0x06
#define FC_MS_SAVE_STATUS 0x07
#define FC_MS_CHECK_IRQ   0x08
#define FC_MS_RESET_IRQ   0x09
#define FC_MS_ISR         0x0A
#define FC_MS_WRITE_START 0x0B
#define FC_MS_WRITE_STOP  0x0C
#define FC_MS_WRITE_AA    0x0D
#define FC_MS_SEND_DATA   0x0E
#define FC_MS_READ_DATA   0x0F
#define FC_MS_SAVE_DATA   0x10
#define FC_MS_CHECK_RXCNT 0x11
#define FC_MS_STOP_AND_OK 0x12
#define FC_MS_FAIL        0x13
#define FC_MS_SUCCESS     0x14

const char *fc_ms_name[] = { "INIT_0      ",
                             "INIT_1      ",
                             "INIT_2      ",
                             "IDLE        ",
                             "WAIT_DONE   ",
                             "INIT_STATUS ",
                             "READ_STATUS ",
                             "SAVE_STATUS ",
                             "CHECK_IRQ   ",
                             "RESET_IRQ   ",
                             "ISR         ",
                             "WRITE_START ",
                             "WRITE_STOP  ",
                             "WRITE_AA    ",
                             "SEND_DATA   ",
                             "READ_DATA   ",
                             "SAVE_DATA   ",
                             "CHECK_RXCNT ",
                             "STOP_AND_OK ",
                             "FAIL        ",
                             "SUCCESS     "};

/* sequence number */
#define FC_SEQ_MASK  0xFF
#define FC_SEQ_SHIFT 24

#endif /* FAN_CONTROLLER_H_ */
