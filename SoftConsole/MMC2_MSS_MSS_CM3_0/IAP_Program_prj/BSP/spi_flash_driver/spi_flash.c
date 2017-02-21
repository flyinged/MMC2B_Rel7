/*******************************************************************************
* Company: Actel Corporation
*
* File: spi_flash.c
* File history:
*      Revision: 1.0 Date: May 4, 2010
*
* Description:
*
* Device driver for the on-board SPI flash for SmartFusion KITS Atmel AT25DF641
* SPI flash driver implementation.
*
* Author: Upender Cherukupally
*         Upender.Cherukupally@actel.com
*         Corporate Applications Engineering
*
*******************************************************************************/

#include "spi_flash.h"
#include "mss_spi.h"

#ifdef USE_DMA_FOR_SPI_FLASH
#include "mss_pdma.h"
#endif

#define PAGE_SIZE                 0x100 //256 XXX ADDED

#define READ_ARRAY_OPCODE         0x03 //changed
#define FAST_READ_ARRAY_OPCODE    0x0B //added
#define DEVICE_ID_READ            0x9F


#define WRITE_ENABLE_CMD          0x06 //OK
#define WRITE_DISABLE_CMD         0x04 //OK
#define PROGRAM_PAGE_CMD          0x02 //ok
#define WRITE_STATUS1_OPCODE      0x01
#define CHIP_ERASE_OPCODE         0x60 //ok
#define ERASE_4K_BLOCK_OPCODE     0x20
#define ERASE_32K_BLOCK_OPCODE    0x52
#define ERASE_64K_BLOCK_OPCODE    0xD8
#define READ_STATUS               0x05 //ok
#define PROGRAM_RESUME_CMD        0xD0


#define READY_BIT_MASK            0x01
#define PROTECT_SECTOR_OPCODE     0x36
#define UNPROTECT_SECTOR_OPCODE   0x39

#define DONT_CARE                    0

#define NB_BYTES_PER_PAGE          256

#define BLOCK_ALIGN_MASK_4K      0xFFFFF000
#define BLOCK_ALIGN_MASK_32K     0xFFFF8000
#define BLOCK_ALIGN_MASK_64K     0xFFFF0000


#if (SPI_FLASH_ON_SF_DEV_KIT == 1)
#define SPI_INSTANCE    &g_mss_spi1 //SPI FLASH CONNECTED TO SPI1, SLAVE0, MODE0, DIV_2, 8 but frame
#define SPI_SLAVE       MSS_SPI_SLAVE_0
#define DMA_TO_PERI     PDMA_TO_SPI_1
#define SPI_DEST_TXBUFF PDMA_SPI1_TX_REGISTER //0x40011014
#endif

#if (SPI_FLASH_ON_SF_EVAL_KIT == 1)
#define SPI_INSTANCE    &g_mss_spi0
#define SPI_SLAVE       MSS_SPI_SLAVE_0
#define DMA_TO_PERI     PDMA_TO_SPI_0
#define SPI_DEST_TXBUFF 0x40001014
#endif

#if ((SPI_FLASH_ON_SF_DEV_KIT == 1) && (SPI_FLASH_ON_SF_EVAL_KIT == 1))
#error "Please select either DEV KIT or EVAL KIT \
        based on board usage in bsp_config.h"
#endif

static uint8_t wait_ready( void );

/******************************************************************************
 *For more details please refer the spi_flash.h file
 ******************************************************************************/
spi_flash_status_t spi_flash_init( void )
{
    /*--------------------------------------------------------------------------
     * Configure MSS_SPI.
     */
    MSS_SPI_init( SPI_INSTANCE );
//    MSS_SPI_configure_master_mode
//    (
//        SPI_INSTANCE,
//        SPI_SLAVE,
//        MSS_SPI_MODE3,
//        MSS_SPI_PCLK_DIV_128,
//        MSS_SPI_BLOCK_TRANSFER_FRAME_SIZE
//    );
    MSS_SPI_configure_master_mode(
		SPI_INSTANCE,
		SPI_SLAVE,
		MSS_SPI_MODE3, //FIXED changed from mode0
		MSS_SPI_PCLK_DIV_4, //100MHz/2 = 50 MHz (MAX ALLOWED)
		MSS_SPI_BLOCK_TRANSFER_FRAME_SIZE //1 byte frames
	);


#ifdef USE_DMA_FOR_SPI_FLASH
    /*--------------------------------------------------------------------------
     * Configure DMA channel used as part of this MSS_SPI Flash driver.
     */
    PDMA_init();

    PDMA_configure
    (
        SPI_FLASH_DMA_CHANNEL,
        DMA_TO_PERI,
        PDMA_LOW_PRIORITY | PDMA_BYTE_TRANSFER | PDMA_INC_SRC_ONE_BYTE,
        PDMA_DEFAULT_WRITE_ADJ
    );
#endif
    return 0;
}

/******************************************************************************
 *For more details please refer the spi_flash.h file
 ******************************************************************************/
spi_flash_status_t
spi_flash_control_hw
(
    spi_flash_control_hw_t operation,
    uint32_t peram1,
    void *   ptrPeram
)
{
    switch(operation){
//        case SPI_FLASH_READ_DEVICE_ID:
//        {
//            uint8_t read_device_id_cmd = DEVICE_ID_READ;
//            uint8_t read_buffer[2];
//            struct device_Info *ptrDevInfo = (struct device_Info *)ptrPeram;
//
//            MSS_SPI_set_slave_select( SPI_INSTANCE, SPI_SLAVE );
//
//            MSS_SPI_transfer_block( SPI_INSTANCE,
//                                    &read_device_id_cmd,
//                                    1,
//                                    read_buffer,
//                                    sizeof(read_buffer) );
//            MSS_SPI_clear_slave_select( SPI_INSTANCE, SPI_SLAVE );
//
//            ptrDevInfo->manufacturer_id = read_buffer[0];
//            ptrDevInfo->device_id = read_buffer[1];
//
//        }
//        break;
//        case SPI_FLASH_SECTOR_PROTECT:
//        {
//            uint8_t cmd_buffer[4];
//            uint32_t address = peram1;
//
//            MSS_SPI_set_slave_select( SPI_INSTANCE, SPI_SLAVE );
//
//            /* Send Write Enable command */
//            cmd_buffer[0] = WRITE_ENABLE_CMD;
//            if(wait_ready())
//                   return SPI_FLASH_UNSUCCESS;
//
//            MSS_SPI_transfer_block( SPI_INSTANCE, cmd_buffer, 1, 0, 0 );
//
//            /* protect sector */
//            cmd_buffer[0] = PROTECT_SECTOR_OPCODE;
//            cmd_buffer[1] = (address >> 16) & 0xFF;
//            cmd_buffer[2] = (address >> 8 ) & 0xFF;
//            cmd_buffer[3] = address & 0xFF;
//            if(wait_ready())
//                return SPI_FLASH_UNSUCCESS;
//
//            MSS_SPI_transfer_block( SPI_INSTANCE,
//                                    cmd_buffer,
//                                    sizeof(cmd_buffer),
//                                    0,
//                                    0 );
//            MSS_SPI_clear_slave_select( SPI_INSTANCE, SPI_SLAVE );
//
//        }
//        break;
//        case SPI_FLASH_SECTOR_UNPROTECT:
//        {
//            uint8_t cmd_buffer[4];
//            uint32_t address = peram1;
//
//            MSS_SPI_set_slave_select( SPI_INSTANCE, SPI_SLAVE );
//
//            /* Send Write Enable command */
//            cmd_buffer[0] = WRITE_ENABLE_CMD;
//            if(wait_ready())
//                return SPI_FLASH_UNSUCCESS;
//
//            MSS_SPI_transfer_block( SPI_INSTANCE, cmd_buffer, 1, 0, 0 );
//
//            /* Unprotect sector */
//            cmd_buffer[0] = UNPROTECT_SECTOR_OPCODE;
//            cmd_buffer[1] = (address >> 16) & 0xFF;
//            cmd_buffer[2] = (address >> 8 ) & 0xFF;
//            cmd_buffer[3] = address & 0xFF;
//            if(wait_ready())
//                return SPI_FLASH_UNSUCCESS;
//
//            MSS_SPI_transfer_block( SPI_INSTANCE,
//                                    cmd_buffer,
//                                    sizeof(cmd_buffer),
//                                    0,
//                                    0 );
//            MSS_SPI_clear_slave_select( SPI_INSTANCE, SPI_SLAVE );
//
//        }
//        break;
//
        case SPI_FLASH_GLOBAL_PROTECT:
        { //XXX ADDED
            uint8_t cmd_buffer[2];

            MSS_SPI_set_slave_select( SPI_INSTANCE, SPI_SLAVE ); //select device

            if(wait_ready()) //check that device is ready
               return SPI_FLASH_UNSUCCESS;

            /* Send Write Enable command */
            cmd_buffer[0] = WRITE_ENABLE_CMD; //checked
            MSS_SPI_transfer_block( SPI_INSTANCE, cmd_buffer, 1, 0, 0 ); //send write enable command

            if(wait_ready())
               return SPI_FLASH_UNSUCCESS;

            /* Set protection flags in  status registers */
            cmd_buffer[0] = WRITE_STATUS1_OPCODE; //write status register to 0
            cmd_buffer[1] = 0x1C; //BP2:0 = 1
            MSS_SPI_transfer_block( SPI_INSTANCE, cmd_buffer, 2, 0, 0 ); //send command

            if(wait_ready())
                return SPI_FLASH_UNSUCCESS;

            MSS_SPI_clear_slave_select( SPI_INSTANCE, SPI_SLAVE );
        }
        break;

        case SPI_FLASH_GLOBAL_UNPROTECT:
        {
            uint8_t cmd_buffer[2];

            MSS_SPI_set_slave_select( SPI_INSTANCE, SPI_SLAVE ); //select device

            if(wait_ready()) //check that device is ready
               return SPI_FLASH_UNSUCCESS;

            /* Send Write Enable command */
            cmd_buffer[0] = WRITE_ENABLE_CMD; //checked
            MSS_SPI_transfer_block( SPI_INSTANCE, cmd_buffer, 1, 0, 0 ); //send write enable command

            if(wait_ready())
               return SPI_FLASH_UNSUCCESS;

            /* Reset status registers (reset all protection bits) */
            cmd_buffer[0] = WRITE_STATUS1_OPCODE; //write status register to 0
            cmd_buffer[1] = 0;
            //eventually set CR1 latency flags (CR1 |= 0xC0)
            MSS_SPI_transfer_block( SPI_INSTANCE, cmd_buffer, 2, 0, 0 ); //send command

            if(wait_ready())
                return SPI_FLASH_UNSUCCESS;

            MSS_SPI_clear_slave_select( SPI_INSTANCE, SPI_SLAVE );
        }
        break;
        case SPI_FLASH_CHIP_ERASE:
        {
            uint8_t cmd_buffer;

            MSS_SPI_set_slave_select( SPI_INSTANCE, SPI_SLAVE );

            if(wait_ready())
                return SPI_FLASH_UNSUCCESS;

            /* Send Write Enable command */
            cmd_buffer = WRITE_ENABLE_CMD;
            MSS_SPI_transfer_block( SPI_INSTANCE, &cmd_buffer, 1, 0, 0 );

            if(wait_ready())
                return SPI_FLASH_UNSUCCESS;

            /* Send Chip Erase command */
            cmd_buffer = CHIP_ERASE_OPCODE;
            MSS_SPI_transfer_block( SPI_INSTANCE, &cmd_buffer, 1, 0, 0 );

            if(wait_ready())
                return SPI_FLASH_UNSUCCESS;

            MSS_SPI_clear_slave_select( SPI_INSTANCE, SPI_SLAVE );
        }
        break;
//        case SPI_FLASH_RESET:
//        {
//            uint8_t cmd_buffer;
//            /* Send Write Enable command */
//            cmd_buffer = WRITE_ENABLE_CMD;
//
//            if(wait_ready())
//                return SPI_FLASH_UNSUCCESS;
//
//            MSS_SPI_set_slave_select( SPI_INSTANCE, SPI_SLAVE );
//            MSS_SPI_transfer_block( SPI_INSTANCE, &cmd_buffer, 1, 0, 0 );
//            if(wait_ready())
//                return SPI_FLASH_UNSUCCESS;
//
//            MSS_SPI_clear_slave_select( SPI_INSTANCE, SPI_SLAVE );
//
//        }
//        break;
//
//        case SPI_FLASH_4KBLOCK_ERASE:
//        {
//            uint32_t address = peram1 & BLOCK_ALIGN_MASK_4K;
//            uint8_t cmd_buffer[4];
//            /* Send Write Enable command */
//            cmd_buffer[0] = WRITE_ENABLE_CMD;
//
//            if(wait_ready())
//                return SPI_FLASH_UNSUCCESS;
//
//            MSS_SPI_set_slave_select( SPI_INSTANCE, SPI_SLAVE );
//            MSS_SPI_transfer_block( SPI_INSTANCE, cmd_buffer, 1, 0, 0 );
//
//            /* Send Chip Erase command */
//            cmd_buffer[0] = ERASE_4K_BLOCK_OPCODE;
//            cmd_buffer[1] = (address >> 16) & 0xFF;
//            cmd_buffer[2] = (address >> 8 ) & 0xFF;
//            cmd_buffer[3] = address & 0xFF;
//
//            if(wait_ready())
//                return SPI_FLASH_UNSUCCESS;
//
//            MSS_SPI_transfer_block( SPI_INSTANCE,
//                                    cmd_buffer,
//                                    sizeof(cmd_buffer),
//                                    0,
//                                    0 );
//            if(wait_ready())
//                return SPI_FLASH_UNSUCCESS;
//            MSS_SPI_clear_slave_select( SPI_INSTANCE, SPI_SLAVE );
//        }
//        break;
//        case SPI_FLASH_32KBLOCK_ERASE:
//        {
//            uint32_t address = peram1 & BLOCK_ALIGN_MASK_32K;
//            uint8_t cmd_buffer[4];
//            /* Send Write Enable command */
//            cmd_buffer[0] = WRITE_ENABLE_CMD;
//
//            wait_ready();
//            MSS_SPI_set_slave_select( SPI_INSTANCE, SPI_SLAVE );
//            MSS_SPI_transfer_block( SPI_INSTANCE, cmd_buffer, 1, 0, 0 );
//
//            /* Send Chip Erase command */
//            cmd_buffer[0] = ERASE_32K_BLOCK_OPCODE;
//            cmd_buffer[1] = (address >> 16) & 0xFF;
//            cmd_buffer[2] = (address >> 8 ) & 0xFF;
//            cmd_buffer[3] = address & 0xFF;
//
//            if(wait_ready())
//                return SPI_FLASH_UNSUCCESS;
//
//            MSS_SPI_transfer_block( SPI_INSTANCE,
//                                    cmd_buffer,
//                                    sizeof(cmd_buffer),
//                                    0,
//                                    0 );
//            if(wait_ready())
//                return SPI_FLASH_UNSUCCESS;
//
//            MSS_SPI_clear_slave_select( SPI_INSTANCE, SPI_SLAVE );
//        }
//        break;
//        case SPI_FLASH_64KBLOCK_ERASE:
//        {
//            uint32_t address = peram1 & BLOCK_ALIGN_MASK_64K;
//            uint8_t cmd_buffer[4];
//            /* Send Write Enable command */
//            cmd_buffer[0] = WRITE_ENABLE_CMD;
//
//            if(wait_ready())
//                return SPI_FLASH_UNSUCCESS;
//
//            MSS_SPI_set_slave_select( SPI_INSTANCE, SPI_SLAVE );
//            MSS_SPI_transfer_block( SPI_INSTANCE, cmd_buffer, 1, 0, 0 );
//
//             /* Send Chip Erase command */
//            cmd_buffer[0] = ERASE_64K_BLOCK_OPCODE;
//            cmd_buffer[1] = (address >> 16) & 0xFF;
//            cmd_buffer[2] = (address >> 8 ) & 0xFF;
//            cmd_buffer[3] = address & 0xFF;
//
//            if(wait_ready())
//                return SPI_FLASH_UNSUCCESS;
//            MSS_SPI_transfer_block( SPI_INSTANCE,
//                                    cmd_buffer,
//                                    sizeof(cmd_buffer),
//                                    0,
//                                    0 );
//            if(wait_ready())
//                return SPI_FLASH_UNSUCCESS;
//
//            MSS_SPI_clear_slave_select( SPI_INSTANCE, SPI_SLAVE );
//        }
//        break;
//        case SPI_FLASH_GET_STATUS:
//        {
//            uint8_t status;
//            uint8_t command = READ_STATUS;
//
//            MSS_SPI_transfer_block( SPI_INSTANCE,
//                                    &command,
//                                    sizeof(uint8_t),
//                                    &status,
//                                    sizeof(status) );
//            return status;
//        }
//        break;

        default:
              return SPI_FLASH_INVALID_ARGUMENTS;
        break;
    }
    return 0;
}


/******************************************************************************
 *For more details please refer the spi_flash.h file
 ******************************************************************************/
spi_flash_status_t
spi_flash_read
(
    uint32_t address,
    uint8_t * rx_buffer,
    size_t size_in_bytes
)
{
    uint8_t cmd_buffer[4]; //[6]; //changed

    cmd_buffer[0] = READ_ARRAY_OPCODE;
    cmd_buffer[1] = (uint8_t)((address >> 16) & 0xFF);
    cmd_buffer[2] = (uint8_t)((address >> 8) & 0xFF);;
    cmd_buffer[3] = (uint8_t)(address & 0xFF);
    //changed
    //cmd_buffer[4] = DONT_CARE; //dummy bytes: not needed at 50MHz.
    //cmd_buffer[5] = DONT_CARE;

    MSS_SPI_set_slave_select( SPI_INSTANCE, SPI_SLAVE );
    if(wait_ready())
        return SPI_FLASH_UNSUCCESS; 

    MSS_SPI_transfer_block( SPI_INSTANCE,
                            cmd_buffer,
                            sizeof(cmd_buffer),
                            rx_buffer,
                            size_in_bytes );
    MSS_SPI_clear_slave_select( SPI_INSTANCE, SPI_SLAVE );
    return 0;
}


/*******************************************************************************
 * This function sends the command and data on SPI
 */
static void write_cmd_data
(
    mss_spi_instance_t * this_spi,
    const uint8_t * cmd_buffer,
    uint16_t cmd_byte_size,
    uint8_t * data_buffer,
    uint16_t data_byte_size
)
{
    uint32_t transfer_size;

    transfer_size = cmd_byte_size + data_byte_size;


#ifdef USE_DMA_FOR_SPI_FLASH
    MSS_SPI_disable( this_spi );
    MSS_SPI_set_transfer_byte_count( this_spi, transfer_size );

    //transfer command
    PDMA_start(
		SPI_FLASH_DMA_CHANNEL,  /* channel_id */
		(uint32_t)cmd_buffer,   /* src_addr */
		SPI_DEST_TXBUFF,             /* dest_addr */
		cmd_byte_size           /* transfer_count */
    );
    //transfer data
    PDMA_load_next_buffer(
		SPI_FLASH_DMA_CHANNEL,  /* channel_id */
		(uint32_t)data_buffer,  /* src_addr */
		SPI_DEST_TXBUFF,        /* dest_addr */
		data_byte_size          /* transfer_count */
	);
    MSS_SPI_enable( this_spi );
    while ( !MSS_SPI_tx_done(this_spi) );

#else
    {
#error "Non-DMA Transfers are not implemented. USE only DMA for TX"
    }

#endif

}

/******************************************************************************
 *For more details please refer the spi_flash.h file
 ******************************************************************************/
spi_flash_status_t
spi_flash_write
(
    uint32_t address,
    uint8_t * write_buffer,
    size_t size_in_bytes
)
{
    uint8_t cmd_buffer[4];

    uint32_t in_buffer_idx;
    uint32_t nb_bytes_to_write;
    uint32_t target_addr;

    MSS_SPI_set_slave_select( SPI_INSTANCE, SPI_SLAVE );

    wait_ready();

    /* Send Write Enable command (COMMENTED BECAUSE RELATED TO UNPROTECT SECTOR COMMAND)*/
//    cmd_buffer[0] = WRITE_ENABLE_CMD;
//    MSS_SPI_transfer_block( SPI_INSTANCE, cmd_buffer, 1, 0, 0 );
//    wait_ready();

    /* Unprotect sector XXX non existing in Spansion device */
//    cmd_buffer[0] = UNPROTECT_SECTOR_OPCODE;
//    cmd_buffer[1] = (address >> 16) & 0xFF;
//    cmd_buffer[2] = (address >> 8 ) & 0xFF;
//    cmd_buffer[3] = address & 0xFF;
//    MSS_SPI_transfer_block( SPI_INSTANCE,
//                            cmd_buffer,
//                            sizeof(cmd_buffer),
//                            0,
//                            0 );
//    if(wait_ready())
//            return SPI_FLASH_UNSUCCESS;

    /* Send Write Enable command */
    cmd_buffer[0] = WRITE_ENABLE_CMD;
    MSS_SPI_transfer_block( SPI_INSTANCE, cmd_buffer, 1, 0, 0 );

    /**/
    in_buffer_idx = 0; //written data counter
    nb_bytes_to_write = size_in_bytes;
    target_addr = address; //current write address

    while ( in_buffer_idx < size_in_bytes )
    {
        uint32_t size_left;
        nb_bytes_to_write = PAGE_SIZE - (target_addr & 0xFF); //number of bytes writable in current page

        /* adjust max possible size to page boundary. */
        size_left = size_in_bytes - in_buffer_idx; //bytes remaining
        if ( size_left < nb_bytes_to_write ) { //if remaining bytes are less than max writable, write them all
            nb_bytes_to_write = size_left;
        }

        if(wait_ready())
            return SPI_FLASH_UNSUCCESS; 

        /* Send Write Enable command */
        cmd_buffer[0] = WRITE_ENABLE_CMD;
        MSS_SPI_transfer_block( SPI_INSTANCE, cmd_buffer, 1, 0, 0 );

        if(wait_ready())
            return SPI_FLASH_UNSUCCESS; 

        /* Program page */
        cmd_buffer[0] = PROGRAM_PAGE_CMD; //3 bytes address (when BAR(7)=0, default)
        cmd_buffer[1] = (target_addr >> 16) & 0xFF; //Address byte 2
        cmd_buffer[2] = (target_addr >> 8 ) & 0xFF; //address byte 1
        cmd_buffer[3] = target_addr & 0xFF; //address byte 0

        //write command and data to SPI (using DMA)
        write_cmd_data (
            SPI_INSTANCE,
            cmd_buffer,
            sizeof(cmd_buffer), //4
            &write_buffer[in_buffer_idx],
            nb_bytes_to_write
        );

        //update counters
        target_addr += nb_bytes_to_write;
        in_buffer_idx += nb_bytes_to_write;
    }

    if(wait_ready())
        return SPI_FLASH_UNSUCCESS; 

    /* Send Write Disable command. */
    cmd_buffer[0] = WRITE_DISABLE_CMD;
    MSS_SPI_transfer_block( SPI_INSTANCE, cmd_buffer, 1, 0, 0 );
    MSS_SPI_clear_slave_select( SPI_INSTANCE, SPI_SLAVE );
    return 0;
}


/******************************************************************************
 * This function waits for the SPI operation to complete
 ******************************************************************************/
static uint8_t wait_ready( void )
{
    uint8_t ready_bit = 1;
    uint8_t command = READ_STATUS;

    do {
        MSS_SPI_transfer_block( SPI_INSTANCE,
                                &command, 
                                sizeof(command),
                                &ready_bit,
                                sizeof(ready_bit) );
        ready_bit = ready_bit & READY_BIT_MASK;
    } while( ready_bit == 1 );

    return (ready_bit);
}
