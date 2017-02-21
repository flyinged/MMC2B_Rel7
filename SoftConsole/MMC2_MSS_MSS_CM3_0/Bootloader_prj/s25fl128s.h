/***************************************************************************//**
 * (c) Copyright 2009 Actel Corporation.  All rights reserved.
 * 
 *  Spansion S25FL128S SPI flash driver API.
 *
 * SVN $Revision:$
 * SVN $Date:$
 */


#ifndef __S25FL128S_SPI_FLASH_H_
#define __S25FL128S_SPI_FLASH_H_

#include <stdint.h>
#include <stdlib.h>

#define S25FL256_SECTOR_SIZE 0x10000 //64 KB
#define S25FL256_BLOCK_SIZE  0x1000  //4 KB
#define S25FL256_PAGE_SIZE   256     //programming buffer 256 Bytes

void FLASH_init( void );

uint32_t FLASH_read_device_id(void);
//(
//    uint8_t * manufacturer_id,
//    uint8_t * device_id
//);

void FLASH_read
(
    uint32_t address,
    uint8_t * rx_buffer,
    size_t size_in_bytes
);

void FLASH_global_unprotect( void );

void FLASH_chip_erase( void );

void FLASH_erase_4k_block
(
    uint32_t address
);

void FLASH_erase_sector
(
    uint32_t address
);

uint8_t FLASH_get_status( void );

void FLASH_program
(
    uint32_t address,
    uint8_t * write_buffer,
    size_t size_in_bytes
);
#endif
