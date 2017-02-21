/*******************************************************************************
* Company: PSI
*
* File: main_loader.c
* File history:
*      Revision: 1.0 Date: Oct 20, 2015
*      Revision: 1.1 removed reset after flash load
*      Revision: 1.3 fixed WD reload bug (reset when loading long files)
*
* Description:
* Derived from Microsemi's application note AC346.
* This code reads the executable binary image (.bin) or FPGA programming file (.dat) from UART,
* writing it either to SPI Flash or External NOR Flash memory.
*
* Author: Alessandro Malatesta
*
*******************************************************************************/

//#include "hal.h"
#include "S29GLxxxP.h"
#include "mss_uart.h"
#include "mss_watchdog.h"
#include "s25fl128s.h"
#include "uart_utility.h"


static void spi_flash_loader(void);
static void emc_flash_loader(void);
uint32_t compute_spi_crc(uint8_t index);
uint32_t crc32_1byte(const void* data, size_t length, uint32_t previousCrc32);

#define EXT_FLASH_BASE_ADDR  0x74000000
#define EXT_FLASH_SIZE       0x4000000 //64MB
#define EXT_FLASH_PAGE_SIZE  16 //bytes, 8 words
#define EXT_FLASH_SEC_SIZE   0x20000 //128 Kbytes, 64 Kwords
#define EXT_FLASH_SEC_NUM    512 //512x128kB = 64MB
#define EXT_FLASH_END_ADDR   (EXT_FLASH_BASE_ADDR+EXT_FLASH_SIZE-1)

#define EXT_ASRAM_BASE_ADDR  0x70000000
#define ESRAM_BASE_ADDR      0x20000000

/******************* EMC ***********************/
//CS_FE(21)       : 0=CS asserted on rising edge of clock, 1=CS asserted on falling edge of clk
//WEN/BEN#(20)    : 0=byte enable active for reads & writes, 1=byte enable active only for writes
//RW_POL(19)      : polarity of RW_N. 0=high-is-read 1=high-is-write
//PIPE_WR(18)     : pipelined write
//PIPE_RD(17)     : pipelined read
//IDD(16:15)      : inter-device delay
//WR_LAT(14:11)   : write latency
//RD_LAT_NXT(10:7): read latency, following cycles in burst
//RD_LAT_1ST(6:3) : read latency, 1st cycle
//PORT_SZ(2)      : 0=8-bit, 1=16-bit
//MEM_TYPE(1:0)   : 00=none, 01=ASRAM, 10=SSRAM, 11=NOR FLASH
#define EMC_PRAM_SETTINGS  0x0020BBBD; //PSRAM type=ASRAM, 16bit, RD_LAT_1=7, RD_LAT_N=7, WR_LAT=7, IDD=1, NO PIPE, CS on falling edge
#define EMC_FLASH_SETTINGS 0x0020DDDF; //FLASH type=FLASH, 16bit, RD_LAT_1=11, RD_LAT_N=11,  WR_LAT=11, IDD=1, NO PIPE, CS on rising edge

//lightweight version or UART read
extern size_t UART_Polled_Rx(mss_uart_instance_t * this_uart, uint8_t * rx_buff, size_t buff_size);

/*==============================================================================
 * main function.
 */


void flash_loader()
{

    uint8_t rx_buff[1];

    MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t *)"\n\n\r### MMC2-B Flash memory Loader v1.3 ###\n\r", sizeof("\n\n\r### MMC2-B Flash memory Loader v1.1 ###\n\r") );
    MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t *)"Please close Terminal application and start host loader.\n\r", sizeof("Please close Terminal application and start host loader.\n\r") );
    MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t *)"System will be reset after the programming procedure completes.\n\r", sizeof("System will be reset after the programming procedure completes.\n\r") );
    /* Handshake */
    /* wait for byte: if 'h', then send 'a'
     * wait for byte: if 'n', then send 'd'
     * wait for byte: if 's', then send 'h'
     * wait for byte: if 'a', then send 'k'
     * wait for byte: if 'e' then start
     */

    while(!(UART_Polled_Rx ( &g_mss_uart0, rx_buff, 1 ))); //receive byte 0
    if(rx_buff[0] == 'h') MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t *)"a", 1 );

    while(!(UART_Polled_Rx ( &g_mss_uart0, rx_buff, 1 ))); //receive byte 1
    if(rx_buff[0] == 'n') MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t *)"d", 1 );

    while(!(UART_Polled_Rx ( &g_mss_uart0, rx_buff, 1 ))); //receive byte 2
    if(rx_buff[0] == 's') MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t *)"h", 1 );

    while(!(UART_Polled_Rx ( &g_mss_uart0, rx_buff, 1 ))); //receive byte 3
    if(rx_buff[0] == 'a') MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t *)"k", 1 );

    while(!(UART_Polled_Rx ( &g_mss_uart0, rx_buff, 1 ))); //receive byte 4

    if(rx_buff[0] == 'e') { //byte 5 shall be an 'e' in order to start
        /* EMC or SPI Flash*/
        /*
         * wait for byte. if 's' run SPI loader, otherwise run FLASH loader
         */
        while( ! UART_Polled_Rx(&g_mss_uart0, rx_buff, 1));

        if(rx_buff[0] == 's') {
            spi_flash_loader();
        } else if(rx_buff[0] == 'e') {
            emc_flash_loader();
        }
    }

    //NVIC_SystemReset(); //restart from bootloader
    return;
}

//loads from UART to parallel flash connected to External Memory Controller (0x74000000)
static void emc_flash_loader(void)
{
    size_t rx_size;
    uint8_t img_buffer[1024];
    uint16_t ureg16, nsectors;
    uint32_t  address, oriAddress;
    int32_t img_size,ii,length, oriSize;
    DEVSTATUS fl_status;

    //write controller settings
    *(volatile uint32_t*)0xE0042040 = EMC_PRAM_SETTINGS;
    *(volatile uint32_t*)0xE0042044 = EMC_FLASH_SETTINGS;


    /* Read the file and burn into external flash */
    /* receive 4 bytes */
    while( ! UART_Polled_Rx(&g_mss_uart0, (uint8_t *)&img_size, 4));

    /* send back the 4 bytes received (ACK) */
    MSS_UART_polled_tx( &g_mss_uart0,(const uint8_t *)&img_size, 4 );
    oriSize = img_size;

    /* Read the file and burn into external flash */
    /* receive 4 bytes (ADDRESS) */
    while( ! UART_Polled_Rx(&g_mss_uart0, (uint8_t *)&address, 4));
    oriAddress = address;//used for ACK

    if (address<EXT_FLASH_BASE_ADDR) {
        return;
    } else {
        address -= EXT_FLASH_BASE_ADDR; //convert to BYTE offset
    }

    //ML84 ADDED: erase only sectors to be written
    nsectors = (img_size/EXT_FLASH_SEC_SIZE)+1;
    if (nsectors>256) nsectors = 256;
    for (ii=0; ii<nsectors; ii++) {
        lld_SectorEraseOp((FLASHDATA*) EXT_FLASH_BASE_ADDR, (address+(ii*EXT_FLASH_SEC_SIZE))/2);
    }

    /* send back the 4 bytes received (ACK?) */
    MSS_UART_polled_tx( &g_mss_uart0,(const uint8_t *)&oriAddress, 4 );

    ii = 0;
    while(img_size > 0)
    {
        rx_size =0;
        if (img_size >= 1024) { /* ceil length to 1024 (buffer size 1024) */
            length = 1024;
        } else {
            length = img_size;
        }

        do { /* fill buffer */
            rx_size = UART_Polled_Rx(&g_mss_uart0, (uint8_t *)&img_buffer, length);
        } while(!rx_size);

        if(rx_size != length) {
            return;
        }

        img_size -= length;
        /* Aligning to 32 bit (increase length so it is an integer multiple of 4) */
        if (length%4){
        	length += (4-(length%4));
        }

        /* write buffer to flash */
        //NOTE: buffer pointer is converted to 16 bit, length and address are divided by 2
        fl_status = lld_memcpy((FLASHDATA*) EXT_FLASH_BASE_ADDR, (ADDRESS)(address+ii)/2, (WORDCOUNT)(length/2), (FLASHDATA*) img_buffer);

        ii += length;

        /* send a character 'a' whenever a 1024 block is written */
        MSS_UART_polled_tx(&g_mss_uart0,(const uint8_t *)"a",1);

    }

    /* send a character 'y' when the programming is complete */
    MSS_UART_polled_tx(&g_mss_uart0,(const uint8_t *)"y",1);

#if 1 //ML84 added
    //read all flash content and send it through UART
    img_size = oriSize; //remaining data
    address  = oriAddress; //base address as received for UART

    //fill buffer from SPI FLASH
    for (ii=address; ii<(address+img_size); ii+=2) {
        ureg16 = *(FLASHDATA*)ii;
        MSS_UART_polled_tx(&g_mss_uart0,(const uint8_t * )&ureg16,2);
    }
#endif

    while(MSS_UART_tx_complete(&g_mss_uart0) == 0);
    return;
}


//load from UART to SPI flash
static void spi_flash_loader(void)
{
    size_t rx_size;
    uint8_t img_buffer[1024], img_buffer2[1024];
    uint16_t nsectors;
    uint32_t  address, oriAddress;
    int32_t img_size,ii,jj,length, oriSize;

    uint8_t file_id, flash_buf[8];
    uint32_t crc, flash_addr;

    /*********** INITIALIZATION ************/
    MSS_WD_reload();

    /* Read the file and burn into external flash */
    while( ! UART_Polled_Rx(&g_mss_uart0, (uint8_t *)&img_size, 4)); //get image size

    FLASH_init();

    MSS_UART_polled_tx( &g_mss_uart0,(const uint8_t *)&img_size, 4 ); //ack
    oriSize = img_size;

    /* Read the file and write it into external SPI flash */
    while( ! UART_Polled_Rx(&g_mss_uart0, (uint8_t *)&address, 4)); //get address
    oriAddress = address;

    if (address+img_size > 0x00FFFFFF) { //todo can be extended to 0x01FF0000 when 32bit commands are implemented
        return; //bad address
    }


    /*********** ERASE ************/
    MSS_WD_reload();
    //ML84 ADDED: erase only sectors to be written
    nsectors = (img_size/S25FL256_SECTOR_SIZE)+1;
    if (nsectors>512) nsectors = 512;
    for (ii=0; ii<nsectors; ii++) {
        FLASH_erase_sector(address+(ii*S25FL256_SECTOR_SIZE));
    }

    MSS_UART_polled_tx( &g_mss_uart0,(const uint8_t *)&address, 4 ); //ack

    /*********** GET DATA ************/
    MSS_WD_reload();
    ii = 0;
    MSS_WD_reload();
    while(img_size > 0)
    {
        MSS_WD_reload();

        rx_size =0;
        if (img_size >=1024) { //transfer in 1024B blocks
            length = 1024;
        } else {
            length = img_size;
        }

        do { //fill buffer
            rx_size = UART_Polled_Rx(&g_mss_uart0, (uint8_t *)&img_buffer, length);
        } while(!rx_size);

        if(rx_size != length) { //check that buffer was filled correctly
            return;
        }

        img_size -= length;

         /* aligning to 32 bit */
        if ((length%4) != 0) { //make length an integer multiple of 4
            length += (4-(length%4));
        }

        FLASH_program(address+ii, img_buffer, length);

        //DEBUG: added check
        FLASH_read(address+ii, img_buffer2, length);

        ii += length;

        for (jj=0; jj<length; jj++) {
            if (img_buffer[jj] != img_buffer2[jj]) {
                MSS_UART_polled_tx(&g_mss_uart0,(const uint8_t *)"n",1);
                return;
            }
        }

        MSS_UART_polled_tx(&g_mss_uart0,(const uint8_t *)"a",1);

    }

    MSS_UART_polled_tx(&g_mss_uart0,(const uint8_t *)"y",1);

#if 1 //ML84 added

    /*********** READBACK ************/
    MSS_WD_reload();
    //read all flash content and send it through UART
    img_size = oriSize; //remaining data
    address  = oriAddress; //base address as received for UART

    //read data and send it through UART
    ii=0;
    while(img_size > 0)
    {
        MSS_WD_reload();
        if (img_size >=1024)        {
            length = 1024;
        } else {
            length = img_size;
        }

        FLASH_read(address + ii, img_buffer, length);

        MSS_UART_polled_tx(&g_mss_uart0,(const uint8_t * )img_buffer,length);

        ii += length;
        img_size -= length;

    }
#endif

    while(MSS_UART_tx_complete(&g_mss_uart0) == 0);


    /** ADDED 4/5/2016 **/
    MSS_WD_reload();
    file_id = 0;
    switch (oriAddress) {
        case 0x00000000:
            file_id = 0;
            break;
        case 0x00100000:
            file_id = 1;
            break;
        case 0x00200000:
            file_id = 2;
            break;
        case 0x00300000:
            file_id = 3;
            break;
        default:
            file_id = 4;
    }
    //write status and CRC
    if (file_id < 4) {
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Computing CRC...\n\r");
        //write file size and crc to flash
        //write status and CRC
        flash_addr = 0xF00000 + (file_id*S25FL256_SECTOR_SIZE);
        FLASH_erase_sector(flash_addr); // added
        //write expected crc to flash
        flash_buf[0] = 0xDD; //file good
        flash_buf[1] = 0xDD;
        flash_buf[2] = 0xDD;
        flash_buf[3] = 0xDD;
        flash_buf[4] = (oriSize>>24) & 0xFF; //file size
        flash_buf[5] = (oriSize>>16) & 0xFF;
        flash_buf[6] = (oriSize>> 8) & 0xFF;
        flash_buf[7] = (oriSize    ) & 0xFF;
        FLASH_program(flash_addr, flash_buf, 8); //write status and size (compute_spi_crc function reads size from FLASH)
        crc =  compute_spi_crc(file_id);
        flash_buf[0] = (crc>>24) & 0xFF; //file crc
        flash_buf[1] = (crc>>16) & 0xFF;
        flash_buf[2] = (crc>> 8) & 0xFF;
        flash_buf[3] = (crc    ) & 0xFF;
        FLASH_program(flash_addr+8, flash_buf, 4); //write crc
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "DONE\n\r");
    }

    /*********** OVER ************/
    MSS_WD_reload();
    return;
}

uint32_t compute_spi_crc(uint8_t index) {
    uint32_t crc, flash_addr, file_len;
    uint8_t flash_buf[256], txt[] = "0x00000000\n\r\0";

    //reset CRC register
    crc = 0;

    //read file length
    flash_addr = 0xF00000 + (index*S25FL256_SECTOR_SIZE);
    FLASH_read(flash_addr+4, flash_buf, 4);
    file_len = (flash_buf[0]<<24) | (flash_buf[1]<<16) | (flash_buf[2]<<8) | flash_buf[3];

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "File length = ");
    uint_to_hexstr(file_len, txt+2, 8);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

    if (file_len > 0x100000) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "File length bigger than 1MB. CRC won't be computed\n\r");
        return 0;
    }

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Computing...");

    flash_addr = index*0x100000;

    while (file_len) { //data to read
        MSS_WD_reload();
        if (file_len >= 256) {
            FLASH_read(flash_addr, flash_buf, 256);
            crc = crc32_1byte(flash_buf, 256, crc);
            flash_addr += 256;
            file_len   -= 256;
        } else {
            FLASH_read(flash_addr, flash_buf, file_len);
            crc = crc32_1byte(flash_buf, file_len, crc);
            file_len   = 0;
        }
    }

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "DONE\n\r");
    return crc;
}
