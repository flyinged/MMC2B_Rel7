/***************************************************************************//**
 * (c) Copyright 2009 Actel Corporation.  All rights reserved.
 * 
 *  Spansion S25Fl128S SPI flash driver implementation.
 *  Flash Driver implemented to support data transfers using PDMA and
 *  also without PDMA using SPI_FLASH_WITH_PDMA macro. by default is
 *  PDMA data transfers is disabled.
 *
 *  This is a modified version of the AT25DF641 driver.
 *  The only commands tested on the S25Fl128S are:
 *  	- FLASH_init();
 *  	- FLASH_global_unprotect();
 *  	- FLASH_read_device_id();
 *  	- FLASH_erase_sector();
 *  	- FLASH_program();
 *  	- FLASH_read();
 *
 *
 *
 * SVN $Revision:$
 * SVN $Date:$
 */

#include "s25fl128s.h"
#include "mss_spi.h"
//#include "mss_pdma.h"
/*******************************************************************************
 * Local Defines
 */
/* 
 * Macro To Enable SPI-FLASH with PDMA 
*/
//FJ #define SPI_FLASH_WITH_PDMA

//FJ #define READ_ARRAY_OPCODE   0x1B
#define DEVICE_ID_READ      0x9F //check


#define WRITE_ENABLE_CMD       0x06 //check
#define WRITE_DISABLE_CMD      0x04 //check
#define PROGRAM_PAGE_CMD       0x02 //check
#define WRITE_STATUS1_OPCODE   0x01 //check
#define CHIP_ERASE_OPCODE      0x60 //check
#define ERASE_4K_BLOCK_OPCODE  0x20 //check
//#define ERASE_32K_BLOCK_OPCODE 0x52 //NON EXISTENT
#define ERASE_64K_BLOCK_OPCODE 0xD8 //check
#define READ_STATUS            0x05 //check

#define DMA_TRANSFER_SIZE    32u
#define READY_BIT_MASK      0x01

#define UNPROTECT_SECTOR_OPCODE 0x39 //non existent

#define DONT_CARE       0

#define NB_BYTES_PER_PAGE   256 //check

//FJ defines
//#define ERASE_SECTOR_OPCODE  0xD8 //erase 64
#define READ_ARRAY_OPCODE    0x03 //check

#define SPI_INSTANCE &g_mss_spi1 //ML84 ADDED: SPI FLASH CONNECTED TO SPI1, SLAVE0, MODE0, DIV_2, 8 bit frame
#define SPI_SLAVE    MSS_SPI_SLAVE_0 //ML84 ADDED



//todo: add support for 32 bit addressing (to access 2nd half of memory array)

/*******************************************************************************
 * Local functions
 */
static void wait_ready(void);

static void program_flash(uint32_t, uint8_t *, size_t);
/*******************************************************************************
 * See st25df641.h for description of this function.
 */
void FLASH_init(void)
{
#ifdef SPI_FLASH_WITH_PDMA
    /*--------------------------------------------------------------------------
     * Configure DMA channel used as part of this SPI Flash driver.
     */
    PDMA_init();
    
    /*
     * Configure PDMA channel 0 to perform a memory to MMUART0 byte transfer
     * incrementing source address after each transfer and no increment for
     * destination address.
     */
    PDMA_configure(PDMA_CHANNEL_0, 
                   PDMA_TO_SPI_0,
                   PDMA_BYTE_TRANSFER | PDMA_LOW_PRIORITY | PDMA_INC_SRC_ONE_BYTE | PDMA_NO_INC,
                   PDMA_DEFAULT_WRITE_ADJ);
    
    /*
     * Configure PDMA channel 1 to perform a MMUART0 to memory byte transfer
     * incrementing the destination address after each transfer 
     * and No increment of source address. 
     */
    PDMA_configure(PDMA_CHANNEL_1, 
                   PDMA_FROM_SPI_0,
                   PDMA_BYTE_TRANSFER | PDMA_LOW_PRIORITY | PDMA_NO_INC | PDMA_INC_DEST_ONE_BYTE,
                   PDMA_DEFAULT_WRITE_ADJ);

#endif
    /*--------------------------------------------------------------------------
     * Configure SPI.
     */
    MSS_SPI_init(SPI_INSTANCE);
    
    MSS_SPI_configure_master_mode(SPI_INSTANCE,
                                   MSS_SPI_SLAVE_0,
                                   MSS_SPI_MODE3,
                                   MSS_SPI_PCLK_DIV_2, //256u, //not valid: MSS_SPI_PCLK_DIV_256 = 7
                                   MSS_SPI_BLOCK_TRANSFER_FRAME_SIZE);

//    MSS_SPI_configure_master_mode(
//		SPI_INSTANCE,
//		SPI_SLAVE,
//		MSS_SPI_MODE0,
//		MSS_SPI_PCLK_DIV_2, //100MHz/2 = 50 MHz (MAX ALLOWED)
//		MSS_SPI_BLOCK_TRANSFER_FRAME_SIZE
//	);
}

/*******************************************************************************
 * See st25df641.h for description of this function.
 */
uint32_t FLASH_read_device_id
(
        void
    //uint8_t * manufacturer_id,
    //uint8_t * device_id
)
{
    uint8_t read_buffer[4];
    uint8_t dummy[1];
    dummy[0] = DEVICE_ID_READ;

    MSS_SPI_set_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);

#ifdef SPI_FLASH_WITH_PDMA
    
    MSS_SPI_disable(SPI_INSTANCE);
    MSS_SPI_set_transfer_byte_count(SPI_INSTANCE, 3);

    PDMA_start(PDMA_CHANNEL_0,
               (uint32_t)dummy, 
               PDMA_SPI0_TX_REGISTER, 
               sizeof(read_buffer));

    PDMA_start(PDMA_CHANNEL_1, 
               PDMA_SPI0_RX_REGISTER, 
               (uint32_t)read_buffer, 
               sizeof(read_buffer));    
   
    MSS_SPI_enable(SPI_INSTANCE);
    
    while (!MSS_SPI_tx_done(SPI_INSTANCE))
    {
        ;
    }    

    MSS_SPI_clear_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);

    *manufacturer_id = read_buffer[1];
    *device_id = read_buffer[2];
        
#else
        
    MSS_SPI_transfer_block(SPI_INSTANCE, dummy, 1, read_buffer, 4);
    MSS_SPI_clear_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);

    //*manufacturer_id = read_buffer[0];
    //*device_id = read_buffer[1];
    return ((read_buffer[0]<<24) | (read_buffer[1]<<16) | (read_buffer[2]<<8) | (read_buffer[3]));
#endif
}

/*******************************************************************************
 * See st25df641.h for description of this function.
 */
void FLASH_read
(
    uint32_t address,
    uint8_t * rx_buffer,
    size_t size_in_bytes
)
{
//FJ    uint8_t cmd_buffer[6]; //Reduced to 4 to support READ_ARRAY_OPCODE = 0x03
    uint8_t cmd_buffer[4];
  
#ifdef SPI_FLASH_WITH_PDMA

    uint32_t transfer_size;
    uint32_t count;
    uint8_t *ptr; 
    uint8_t *dst_ptr;

    cmd_buffer[0] = READ_ARRAY_OPCODE;
    cmd_buffer[1] = (uint8_t)((address >> 16) & 0xFF);
    cmd_buffer[2] = (uint8_t)((address >> 8) & 0xFF);;
    cmd_buffer[3] = (uint8_t)(address & 0xFF);
    cmd_buffer[4] = DONT_CARE;
    cmd_buffer[5] = DONT_CARE;
    ptr = &cmd_buffer[0];
    /*
    * To Allocate buffer dynamically to store the read the data from flash.
    */
    dst_ptr = (uint8_t *) malloc((size_in_bytes*sizeof(uint8_t))+ sizeof(cmd_buffer));
    if(dst_ptr != NULL)
    {
        transfer_size = (size_in_bytes + sizeof(cmd_buffer));
        MSS_SPI_set_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
        wait_ready();
    
        MSS_SPI_disable(SPI_INSTANCE);
        MSS_SPI_set_transfer_byte_count(SPI_INSTANCE, transfer_size);

        PDMA_start(PDMA_CHANNEL_0,          /* channel_id */
                   (uint32_t)ptr,           /* src_addr */
                   PDMA_SPI0_TX_REGISTER,   /* dest_addr */
                   transfer_size);
    
        PDMA_start(PDMA_CHANNEL_1,          /* channel_id */
                   PDMA_SPI0_RX_REGISTER,   /* src_addr */
                   (uint32_t)dst_ptr,       /* dest_addr */
                   transfer_size);
    
        MSS_SPI_enable(SPI_INSTANCE);
    
        while(!MSS_SPI_tx_done(SPI_INSTANCE))
        {
            ;
        }
        MSS_SPI_clear_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
                
        for(count = 0; count < size_in_bytes; count++)
        {
            rx_buffer[count] = dst_ptr[6+count];
        }        
        free(dst_ptr);
    }
    else
    {

    }
        
#else
    cmd_buffer[0] = READ_ARRAY_OPCODE;
    cmd_buffer[1] = (uint8_t)((address >> 16) & 0xFF);
    cmd_buffer[2] = (uint8_t)((address >> 8) & 0xFF);;
    cmd_buffer[3] = (uint8_t)(address & 0xFF);
//FJ    cmd_buffer[4] = DONT_CARE;
//FJ    cmd_buffer[5] = DONT_CARE;
        
    MSS_SPI_set_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
    wait_ready();
    MSS_SPI_transfer_block(SPI_INSTANCE, cmd_buffer, sizeof(cmd_buffer), rx_buffer, size_in_bytes);
    wait_ready();
    MSS_SPI_clear_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
        
#endif

}

/*******************************************************************************
 * See st25df641.h for description of this function.
 */
void FLASH_global_unprotect(void)
{
    uint8_t cmd_buffer[2];
    /* Send Write Enable command */
    cmd_buffer[0] = WRITE_ENABLE_CMD;

    MSS_SPI_set_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
    wait_ready();
    MSS_SPI_transfer_block(SPI_INSTANCE, cmd_buffer, 1, 0, 0);
    
    /* Send Chip Erase command */
    cmd_buffer[0] = WRITE_STATUS1_OPCODE;
    cmd_buffer[1] = 0;
    
    wait_ready();
    MSS_SPI_transfer_block(SPI_INSTANCE, cmd_buffer, 2, 0, 0);
    wait_ready();
    MSS_SPI_clear_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
}

/*******************************************************************************
 * See st25df641.h for description of this function.
 */
void FLASH_chip_erase(void)
{
    uint8_t cmd_buffer;
    /* Send Write Enable command */
    cmd_buffer = WRITE_ENABLE_CMD;

    MSS_SPI_set_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
    wait_ready();
    MSS_SPI_transfer_block(SPI_INSTANCE, &cmd_buffer, 1, 0, 0);

      /* Send Chip Erase command */
    cmd_buffer = CHIP_ERASE_OPCODE;
    
    wait_ready();
    MSS_SPI_transfer_block(SPI_INSTANCE, &cmd_buffer, 1, 0, 0);

    wait_ready();
    MSS_SPI_clear_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
}

/*******************************************************************************
 * See st25df641.h for description of this function.
 */
void FLASH_erase_4k_block
(
    uint32_t address
)
{
    uint8_t cmd_buffer[4];
    /* Send Write Enable command */
    cmd_buffer[0] = WRITE_ENABLE_CMD;

    MSS_SPI_set_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
    wait_ready();
    MSS_SPI_transfer_block(SPI_INSTANCE, cmd_buffer, 1, 0, 0);
    
    /* Send Chip Erase command */
    cmd_buffer[0] = ERASE_4K_BLOCK_OPCODE;
    cmd_buffer[1] = (address >> 16) & 0xFF;
    cmd_buffer[2] = (address >> 8) & 0xFF;
    cmd_buffer[3] = address & 0xFF;
    
    wait_ready();
    MSS_SPI_transfer_block(SPI_INSTANCE, cmd_buffer, sizeof(cmd_buffer), 0, 0);
    wait_ready();
    MSS_SPI_clear_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
}

/*******************************************************************************
 * FJ Invented to erase 64kB or 256kB sector (according to chip model)
 */
void FLASH_erase_sector
(
    uint32_t address
)
{
    uint8_t cmd_buffer[4];
    /* Send Write Enable command */
    cmd_buffer[0] = WRITE_ENABLE_CMD;

    MSS_SPI_set_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
    wait_ready();
    MSS_SPI_transfer_block(SPI_INSTANCE, cmd_buffer, 1, 0, 0);

    /* Send Chip Erase command */
    cmd_buffer[0] = ERASE_64K_BLOCK_OPCODE; //ERASE_SECTOR_OPCODE;
    cmd_buffer[1] = (address >> 16) & 0xFF;
    cmd_buffer[2] = (address >> 8) & 0xFF;
    cmd_buffer[3] = address & 0xFF;

    wait_ready();
    MSS_SPI_transfer_block(SPI_INSTANCE, cmd_buffer, sizeof(cmd_buffer), 0, 0);
    wait_ready();
    MSS_SPI_clear_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
}


/*******************************************************************************
 * See st25df641.h for description of this function.
 */
void write_cmd_data
(
    mss_spi_instance_t * this_spi,
    const uint8_t * cmd_buffer,
    uint16_t cmd_byte_size,
    uint8_t * data_buffer,
    uint16_t data_byte_size
)
{
    MSS_SPI_set_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);

#ifdef SPI_FLASH_WITH_PDMA      
    uint32_t transfer_size;    
    transfer_size = cmd_byte_size + data_byte_size;
    /*
     * Register the PDMA handler that will feed the PDMA channel.
    */
    MSS_SPI_disable(this_spi);
    MSS_SPI_set_transfer_byte_count(this_spi, transfer_size);

    PDMA_start(PDMA_CHANNEL_0,         /* channel_id */
               (uint32_t)cmd_buffer,   /* src_addr */
               PDMA_SPI0_TX_REGISTER,  /* dest_addr */
               cmd_byte_size);
    
    PDMA_load_next_buffer(PDMA_CHANNEL_0,         /* channel_id */
                          (uint32_t)data_buffer,  /* src_addr */
                          PDMA_SPI0_TX_REGISTER,  /* dest_addr */
                          data_byte_size);

    MSS_SPI_enable(this_spi);
    
    while (!MSS_SPI_tx_done(this_spi))
    {
        ;
    }
#else

    uint8_t tx_buffer[516];
    uint16_t transfer_size;
    uint16_t idx = 0;
    
    transfer_size = cmd_byte_size + data_byte_size;
    
    for(idx = 0; idx < cmd_byte_size; ++idx)
    {
        tx_buffer[idx] = cmd_buffer[idx];
    }

    for(idx = 0; idx < data_byte_size; ++idx)
    {
        tx_buffer[cmd_byte_size + idx] = data_buffer[idx];
    }
    
    MSS_SPI_transfer_block(SPI_INSTANCE, tx_buffer, transfer_size, 0, 0);
    MSS_SPI_clear_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
#endif
}
        
/*******************************************************************************
 * See st25df641.h for description of this function.
 */

static void program_flash
(
    uint32_t address,
    uint8_t * write_buffer,
    size_t size_in_bytes
)
{
    uint8_t cmd_buffer[4];

    MSS_SPI_set_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
    
    /* Send Write Enable command */
    cmd_buffer[0] = WRITE_ENABLE_CMD;
    wait_ready();
    MSS_SPI_transfer_block(SPI_INSTANCE, cmd_buffer, 1, 0, 0);
    
    /* Unprotect sector */
    cmd_buffer[0] = UNPROTECT_SECTOR_OPCODE;
    cmd_buffer[1] = (address >> 16) & 0xFF;
    cmd_buffer[2] = (address >> 8) & 0xFF;
    cmd_buffer[3] = address & 0xFF;
    wait_ready();
    MSS_SPI_transfer_block(SPI_INSTANCE, cmd_buffer, sizeof(cmd_buffer), 0, 0);

    /* Send Write Enable command */
    cmd_buffer[0] = WRITE_ENABLE_CMD;
    wait_ready();
    MSS_SPI_transfer_block(SPI_INSTANCE, cmd_buffer, 1, 0, 0);

    wait_ready();
        
    /* Send Write Enable command */
    cmd_buffer[0] = WRITE_ENABLE_CMD;
    MSS_SPI_transfer_block(SPI_INSTANCE, cmd_buffer, 1, 0, 0);

    /* Program page */
    wait_ready();

    cmd_buffer[0] = PROGRAM_PAGE_CMD;
    cmd_buffer[1] = (address >> 16) & 0xFF;
    cmd_buffer[2] = (address >> 8) & 0xFF;
    cmd_buffer[3] = address & 0xFF;

    write_cmd_data(SPI_INSTANCE,
                   cmd_buffer,
                   sizeof(cmd_buffer),
                   write_buffer,
                   size_in_bytes);        
   
    /* Send Write Disable command. */
    cmd_buffer[0] = WRITE_DISABLE_CMD;

    wait_ready();

    MSS_SPI_transfer_block(SPI_INSTANCE, cmd_buffer, 1, 0, 0);
    MSS_SPI_clear_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
}

/*******************************************************************************
 * See st25df641.h for description of this function.
 */
uint8_t FLASH_get_status(void)
{
    uint8_t status;
    uint8_t command = READ_STATUS;
    
    MSS_SPI_set_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
    MSS_SPI_transfer_block(SPI_INSTANCE,
                           &command, 
                           sizeof(uint8_t), 
                           &status, 
                           sizeof(status));
    MSS_SPI_clear_slave_select(SPI_INSTANCE, MSS_SPI_SLAVE_0);
    return status;
}

/*******************************************************************************
 * See st25df641.h for description of this function.
 */
static void wait_ready(void)
{
    uint8_t ready_bit = 1;
    uint8_t command = READ_STATUS;
    
    do {
        MSS_SPI_transfer_block(SPI_INSTANCE, &command, sizeof(command), &ready_bit, sizeof(ready_bit));
        ready_bit = ready_bit & READY_BIT_MASK;
    } while(ready_bit == 1);
}
/***************************************************************************//**
 * See st25df641.h for description of this function.
 */

void FLASH_program
(
    uint32_t address,
    uint8_t * write_buffer,
    size_t size_in_bytes
)
{   
    uint32_t in_buffer_idx;
    uint32_t nb_bytes_to_write;
    uint32_t target_addr;
       
    /**/
    in_buffer_idx = 0;
    nb_bytes_to_write = size_in_bytes;
    target_addr = address;
    
    while (in_buffer_idx < size_in_bytes)
    {
        uint32_t size_left;
        nb_bytes_to_write = 0x100 - (target_addr & 0xFF);   /* adjust max possible size to page boundary. */
        size_left = size_in_bytes - in_buffer_idx;
        if (size_left < nb_bytes_to_write)
        {
            nb_bytes_to_write = size_left;
        }
        
        program_flash(target_addr, &write_buffer[in_buffer_idx], nb_bytes_to_write);
        
        target_addr += nb_bytes_to_write;
        in_buffer_idx += nb_bytes_to_write;
    }

}
