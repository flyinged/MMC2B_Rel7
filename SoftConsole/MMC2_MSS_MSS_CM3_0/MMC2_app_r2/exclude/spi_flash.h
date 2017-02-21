/*
 * spi_flash.h
 *
 *  Created on: 30.09.2015
 *      Author: malatesta_a
 */

#ifndef SPI_FLASH_H_
#define SPI_FLASH_H_

/* S25FL256SAGNFI001 SPI FLASH MEMORY (256 Mbit, 133MHz, Uniform 4/64kB sectors)  */
/* when TBPARM=0, sectors 0-31 are 4kB, 32-541 are 64kB => start address = N*0x1000 when n=[0:31], N*0x10000 when N=[32:541) */
/* when TBPARM=1, sectors 0-509 are 32kB, 510-541 are 4kB => start address = N*0x10000 when N=[0:509], N*0x1000 when N=[510:541) */
#define SPIFLASH_PAGE_SIZE 256 //page size in bytes
/* commands */
#define SPIFLASH_CMD_REMS  0x90 //read electronic manufacturer signature: WR(0x9000), RD(0x0118)
#define SPIFLASH_CMD_RDID  0x9F //read JEDEC manufacturer ID WR(0x9F) RD(0x0102_194D_0180)
#define SPIFLASH_CMD_RES   0xAB //read electronic signature WR(0xAB00_0000) RD(0x18)

#define SPIFLASH_CMD_RDSR1 0x05 //read status register 1 WR(0x05) RD(SR)
#define SPIFLASH_CMD_RDCR  0x35 //read configuration register 1 WR(0x35) RD(CR)
#define SPIFLASH_CMD_WRR   0x01 //write register WR(0x01,SR,CR) NEED WREN BEFORE
#define SPIFLASH_CMD_WREN  0x06 //set write enable
#define SPIFLASH_CMD_WRDI  0x04 //clear write enable
#define SPIFLASH_CMD_CLSR  0x30 //clear status register
#define SPIFLASH_CMD_BRRD  0x16 //BAR read WR(0x16) RD(BAR)
#define SPIFLASH_CMD_BRWR  0x17 //BAR write WR(0x17,BAR) NO WREN NEEDED

#define SPIFLASH_CMD_MBR   0xFF //mode bit reset (like HW reset: recommended after system reset and before SW reset)
#define SPIFLASH_CMD_RESET 0xF0 //software reset
#define SPIFLASH_CMD_READ  0x03 //read WR(0x03AAAAAA) RD(DATA)
#define SPIFLASH_CMD_PP    0x02 //page program W(0x02aa_aaaa_ddd...) WRITE 1 to PAGE_SIZE bytes
#define SPIFLASH_CMD_P4E   0x20 //4kB parameter sector erase WR(0x20aa_aaaa)
#define SPIFLASH_CMD_SE    0xD8 //64kB sector erase WR(0x20aa_aaaa)
#define SPIFLASH_CMD_BE    0x60 //bulk erase W(0x60) BP bits SHALL BE 000

/* status register */
/* Block protection when TBPROT=0:
 * 000: none
 * 001: 1/64 from top (512 kB)
 * 010: 1/32 from top (1 MB)
 * 011: 1/16 from top (2 MB)
 * 100: 1/8 from top  (4 MB)
 * 101: 1/4 from top  (8 MB)
 * 110: 1/2 from top  (16 MB)
 * 111: all protected (32 MB)
 * When TBPROT=1, protected zone is counted from bottom  */
#define SPIFLASH_SR1_SRWD 0x80 //write disable
#define SPIFLASH_SR1_PERR 0x40 //programming error
#define SPIFLASH_SR1_EERR 0x20 //erase error
#define SPIFLASH_SR1_BP   0x1C //block protection
#define SPIFLASH_SR1_WEL  0x02 //write enable latch
#define SPIFLASH_SR1_WIP  0x01 //write in progress
/* configuration register */
#define SPIFLASH_CR1_LC     0xC0 //latency between address and data for read commands
#define SPIFLASH_CR1_TBPROT 0x20 //BP starts at bottom (vs top)
#define SPIFLASH_CR1_RFU    0x10 //reserved
#define SPIFLASH_CR1_BPNV   0x08 //set BP as volatile (vs non volatile)
#define SPIFLASH_CR1_TBPARM 0x04 //4kB sectors at top (vs bottom)
#define SPIFLASH_CR1_QUAD   0x02 //quad mode (vs dual/serial)
#define SPIFLASH_CR1_FREEZE 0x01 //lock the BP bits in SR1
/* BAR register */
#define SPIFLASH_BAR_EXTADD 0x80 //enable 4byte addressing  commands
#define SPIFLASH_BAR_BA24   0x01 //bit 24 of address (A24)

#endif /* SPI_FLASH_H_ */
