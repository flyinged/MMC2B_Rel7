/*
 * spi_eeprom.h
 *
 *  Created on: 30.09.2015
 *      Author: malatesta_a
 */

#ifndef SPI_EEPROM_H_
#define SPI_EEPROM_H_

/* 25AA02E48T SPI 256B EEPROM (MAX 5 MHz) xxx write .c file */
#define SPIEE_CMD_READ  0x03 //read
#define SPIEE_CMD_WRITE 0x02 //write
#define SPIEE_CMD_WRDI  0x04 //write-disable
#define SPIEE_CMD_WREN  0x06 //write-enable
#define SPIEE_CMD_RDST  0x05 //read status
#define SPIEE_CMD_WRST  0x01 //write status
#define SPIEE_SR_BP     0x0C //block protect (R/W): 00=none, 01=0xC0-0xFF (default), 10=0x80-0xFF, 11=ALL
#define SPIEE_SR_WEL    0x02 //WRITE ENABLE LATCH (RO)
#define SPIEE_SR_WIP    0x01 //WRITE IN PROGRESS (RO)
#define SPIEE_EUI48_ADR 0xFA

#endif /* SPI_EEPROM_H_ */
