/*******************************************************************************
 *  PSI 2015
 *
 *  Bootloader for MMC2 Rev-B
 *
 *
 * Author : Alessandro Malatesta
 *
 * CHANGE LOG
 * 0.0 - Base version derived from Bootloader_prj in Microsemi's Application note AC362
 * 0.1 - Integrated FLASH loader into Bootloader
 * 0.2 - Integrated IAP into Bootloader
 * 0.3 - Removed reset after flash-load completion
 * 0.4 - Added code to execute IAP from eSRAM
 * 0.5 - Added support for selection of different IAP images from SPI
 * 0.6 - Corrected IAP image size
 * 0.7 - Added options to menu
 * 0.8 - Added PCLK1 divider control to speedup SPI access
 * 0.9 - Added readback checks and console messages during application copy
 * 1.0 - Added watchdog control
 * 1.1 - Changed EMC settings, added option to boot SW image 2
 * 1.2 - Changed boot timeout from 30 to 15 seconds
 * 1.3 - Added command selection over FW register
 * 1.4 - Added forced shutdown on system start.
 * 1.5 - Removed forced shutdown on system start. Added CRC computation on flash load. Added CRC check on boot(errors are only notified).
 * 1.6 - Fallback on safe image on CRC errors. If safe image does not pass the check, system won't boot.
 * 1.7 - Changed inital reset sequence. Disabled clock speed change.
 * 1.8 - Disable AUTO_I2C before programming, added CRC on FW files
 * 1.9 - Fixed bug: added missing sector erase before writing bitstream info to FLASH
 * 1.12 - Fixed bug: avoid self reset when loading long files via UART
 * 1.13 - Added forced shutdown
 *
 *******************************************************************************/

#define READBACK_CHECKS

/*************************************************** ***********************/
/* Standard Includes */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>

/**************************************************************************/
/* Firmware Includes */
/**************************************************************************/

#include "a2fxxxm3.h"
#include "mss_uart.h"
#include "mss_watchdog.h"
#include "mss_timer.h"
#include "boot_config.h"

#include "s25fl128s.h" //ML84 added: SPI flash
#include "uart_utility.h" //ML84 added: SPI flash

#include "MMC2_hw_platform.h"

/**************************************************************************/
/* RTOS Includes */
/**************************************************************************/

/**************************************************************************/
/*Extern Declarations*/
/**************************************************************************/

void bx_user_code_esram(void);
void bx_loader_envm(void); //ML84 added
//void bx_3rd_eNVM_Image(void);
void bx_user_code_emcram(void);

//#define VECTOR_TABLE_ADDR_3RD_IMAGE 0x00020000
//#define VECTOR_TABLE_ADDR_2ND_IMAGE 0x20000000

/* Frimware control register: needed to force shutdown (added v1.4) */
#define CPU2FABRIC_REG    (MBU_MMC_V2B_APB_0 + 0x00000004) //Write/Readback (inputs to logic)
#define CPU2FABRIC_REG2   (MBU_MMC_V2B_APB_0 + 0x0000000C) //shutdown
#define C2F_REG_SHUTDOWN  0x00000002 //force shutdown mask

/* 0x34020 -0x04020
 * Soft reset control register (default 0x0003FFF8)
 * PADRESETENABLE(19): When 1 enables external reset (DEF 0)
 * F2MRESETENABLE(18): When 1 fabric can reset MSS via F2M_RESET_N (DEF 0)
 * FPGA_SR(17)       : When 1 reset to fabric M2F_RESET_N is ASSERTED (DEF 1)
 * EXT_SR(16)        : when 1 keeps MSS_RESET_N asserted after startup (DEF 1)
 * IAP_SR(15) * GPIO_SR(14) * ACE_SR(13) * I2C1_SR(12)  : (DEF 1111)
 * I2C_0_SR(11) * SPI1_SR(10) * SPI0_SR(9) * UART1_SR(8): (DEF 1111)
 * UART0_SR(7) * TIMER_SR(6) * PDMA_SR(5) * MAC_SR(4)   : (DEF 1111)
 * EMC_SR(3) * ESRAM1_SR(2) * ESRAM0_SR(1) * ENVM_SR(0) : (DEF 1000)
 */
#define SOFT_RST_CR 0xE0042030

/*
 * Clock conditioning circuit, output divider control register
 * GLBDIV(13:12) : Ratio between MSS and fabric clock (0,1,2=/1/2/4; 3=RSVD)
 * ACMDIV(11:8)  : PCLK1/ACMDIV must be <10MHZ (1,2,4,8,0 = /1/2/4/8/16)
 * ACLKDIV(7:6)  : ACLK divider (0,1,2=/1/2/4; 3=RSVD)
 * PCLK1DIV(5:4) : PCLK1 divider (APB1 bus: UART1,SPI1,I2C1,GPIO,EFROM,RTC) (0,1,2=/1/2/4; 3=RSVD)
 * PCLK0DIV(3:2) : PCLK0 divider (APB0 bus: PDMA,TIMER,WD,UART0,SPI0,I2C0,MAC) (0,1,2=/1/2/4; 3=RSVD)
 * RMIICLKSEL(0) : 0=RMII clock from PAD, 1=RMII clock connected to GLC
 */
#define MSS_CLK_CR 0xE0042048

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
#define EMC_TYPE_ASRAM 0x00000001
#define EMC_TYPE_SSRAM 0x00000002
#define EMC_TYPE_FLASH 0x00000003
#define EMC_PORT_16BIT 0x00000004
#define EMC_LAT_RD1(lat) ((lat<<3) &(0x00000078))
#define EMC_LAT_RDN(lat) ((lat<<7) &(0x00000780))
#define EMC_LAT_WR(lat)  ((lat<<11)&(0x00007800))
#define EMC_IDD(lat)     ((lat<<15)&(0x00018000))
#define EMC_PIPE_RD  0x00020000
#define EMC_PIPE_WR  0x00040000
#define EMC_RW_WRITE 0x00080000
#define EMC_WEN_WO   0x00100000
#define EMC_CS_FE    0x00200000

//#define EMC_PRAM_SETTINGS  0x0020BBBD; //PSRAM type=ASRAM, 16bit, RD_LAT_1=7, RD_LAT_N=7, WR_LAT=7, IDD=1, NO PIPE, CS on falling edge
#define EMC_PRAM_SETTINGS  (EMC_TYPE_ASRAM | EMC_PORT_16BIT | EMC_LAT_RD1(7) | EMC_LAT_RDN(4) | EMC_LAT_WR(7) | EMC_IDD(1) | EMC_CS_FE);
#define EMC_FLASH_SETTINGS 0x0020DDDF; //FLASH type=FLASH, 16bit, RD_LAT_1=11, RD_LAT_N=11,  WR_LAT=11, IDD=1, NO PIPE, CS on rising edge


//BASE ADDRESSES FOR SYSTEM MEMORIES
#define ESRAM_BASE_ADDR    0x20000000
#define ENVM_BASE_ADDR     0x60000000
#define EMCRAM_BASE_ADDR   0x70000000
#define EMCFLASH_BASE_ADDR 0x74000000

//In-Application Programming Image
#define IAP_ENVM_BASE_ADDR   (ENVM_BASE_ADDR+0x10000) //base address in eNVM (only for storage)
#define IAP_IMAGE_SIZE_BYTES (0xF000-8) //60KB - 8B
#define IAP_CHOICE_REG       (ESRAM_BASE_ADDR+IAP_IMAGE_SIZE_BYTES) //contains address in eSRAM where IAP image address is stored
#define IAP_SPI_ADDR_REG     (IAP_CHOICE_REG+4) //contains IAP Action Code (for direct execution)

//System Memory map
//0x60000000 (64K): Bootloader
//0x60010000 (64K): IAP
//0x70000000 (1MB): App Golden Image

//SPI addressses
//FW images (1st 2MB of SPI)
#define FW_IMAGE_SIZE_BYTES 0x100000 //Fabric=452 kB, eNVM client = 256 kB => 708 kB (round to 1MB)
#define FW_GOLDEN_SPI_ADDR  0x000000
#define FW_IMG1_SPI_ADDR    0x100000 //1*FW_IMAGE_SIZE_BYTES
//SW images (rest of SPI)
#define APP_IMAGE_SIZE_BYTES 0x00100000 //1MB
#define APP_DEFAULT_SPI_ADDR (2*APP_IMAGE_SIZE_BYTES) //1st 2MB used for FW images. Start from 3rd.
#define APP_SAFE_SPI_ADDR      (APP_DEFAULT_SPI_ADDR+APP_IMAGE_SIZE_BYTES)
#define APP_2_SPI_ADDR      (APP_SAFE_SPI_ADDR+APP_IMAGE_SIZE_BYTES)
#define APP_3_SPI_ADDR      (APP_2_SPI_ADDR+APP_IMAGE_SIZE_BYTES)
#define APP_4_SPI_ADDR      (APP_3_SPI_ADDR+APP_IMAGE_SIZE_BYTES)
//#define APP_N_SPI_ADDR  1*APP_IMAGE_SIZE_BYTES //32 MB available

volatile uint16_t boot_timer   = 0;
volatile uint32_t tick_counter = 0;
const    uint32_t TOUT         = 60*0x64; //USER INPUT TIMEOUT. 0x64 = 1s when system tick = 1ms
uint32_t fabric_image_spi_offset;


//ML84 added
void copy_memory(uint32_t from, uint32_t to, uint32_t nbytes)
{
    unsigned long *exeDestAddr, *exeSrcAddr, ii;
#ifdef READBACK_CHECKS
    unsigned long nbytes_reg = nbytes;
    uint8_t txt[] = "0x00000000\0";
#endif

    //program shall be started from local RAM: copy the vector table
    exeSrcAddr  = (unsigned long *)from; //source
    exeDestAddr = (unsigned long *)to;

    while (nbytes % 4) nbytes++; //increment until multiple of 4

    for (ii=0; ii<(nbytes/4); ii++ ) { //copy vector table to local RAM
       *exeDestAddr++ = *exeSrcAddr++;
    }
#ifdef READBACK_CHECKS //readback & check
    uint8_t err_cnt = 0;

    exeSrcAddr  = (unsigned long *)from; //source
    exeDestAddr = (unsigned long *)to;
    nbytes = nbytes_reg;

    while (nbytes % 4) nbytes++; //increment until multiple of 4

    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\n\rChecking content of eSRAM:\n\r");
    for (ii=0; ii<(nbytes/4); ii++ ) { //copy vector table to local RAM
        if (*exeDestAddr != *exeSrcAddr) {
            err_cnt++;
            if (err_cnt < 64) {
                MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"    ERROR data at ");
                uint_to_hexstr((uint32_t)exeDestAddr, txt+2, 8);
                MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)txt);
                MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)" does not match data at \n\r");
                uint_to_hexstr((uint32_t)exeSrcAddr, txt+2, 8);
                MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)txt);
                MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\n\r");
            }
        }
        exeDestAddr++;
        exeSrcAddr++;
    }
    if (err_cnt) {
        MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\n\r>>> ERROR: check FAILED\n\r");
    } else {
        MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"SUCCESS\n\r");
    }
#endif
}

//ML84 added
void spi_flash_to_emc_sram(uint32_t srcAddr, uint32_t size)
{
    uint8_t img_buffer[1024], txt[] = "0x00000000\0";
    unsigned int ii=0, jj =0;
    unsigned long length = 0;
    unsigned long *exeDestAddr, *exeSrcAddr;
    unsigned char *destAddr = (unsigned char *)EMCRAM_BASE_ADDR; //destination: external PSRAM
#ifdef READBACK_CHECKS
    uint32_t size_reg = size;
#endif

    //Vector table offset register (vector table at eSRAM base address)
    //*(volatile unsigned long *)0xE000ED08 = ESRAM_BASE_ADDR; //COMMENTED BECAUSE IT CAUSED A RESET!!!

    //Init EMC controller
    *(volatile uint32_t*)0xE0042040 = EMC_PRAM_SETTINGS;
    *(volatile uint32_t*)0xE0042044 = EMC_FLASH_SETTINGS;

    //init SPI flash controller
    FLASH_init();

    MSS_WD_reload();
    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\nCopying main application from SPI flash to external RAM (");
    uint_to_hexstr(size, txt+2, 8);
    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)txt);
    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)" bytes)\n\r");

    ii = 0; //buffer offset
    jj = 0; //current offset in SPI flash
    while(size > 0) {
        //read 1KB sized chunks
        if (size >= 1024) {
            length = 1024;
        } else {
            length = size;
        }
        //fill buffer (length in bytes)
        FLASH_read(srcAddr + jj, img_buffer, length);

        //copy from buffer to external PSRAM (bytewise)
        for(ii=0; ii<length; ii++) {
            *destAddr++ = img_buffer[ii];
        }
        jj += length; //update SPI flash read address
        size -= length; //update remaining data counter

        uint_to_hexstr(size, txt+2, 8);
        MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)txt);
        MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\r");
    }
    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\n");

#ifdef READBACK_CHECKS //read back & check
    uint8_t err_cnt = 0;
    destAddr = (unsigned char *)EMCRAM_BASE_ADDR;

    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\nReading back \n\r");
    size = size_reg;
    uint_to_hexstr(size, txt+2, 8);
    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)txt);
    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)" bytes\n\r");

    ii = 0; //buffer offset
    jj = 0; //current offset in SPI flash
    while(size > 0) {
        //read 1KB sized chunks
        if (size >= 1024) {
            length = 1024;
        } else {
            length = size;
        }
        //fill buffer
        FLASH_read(srcAddr + jj, img_buffer, length);

        //compare buffer with external PSRAM
        for(ii=0; ii<length; ii++) {
            if (*destAddr != img_buffer[ii] && err_cnt < 64) {
                MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"    ERROR @address ");
                uint_to_hexstr((uint32_t)destAddr, txt+2, 8);
                MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)txt);
                MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\n\r");
                err_cnt++;
            }
            destAddr++;
        }
        jj += length; //update SPI flash read address
        size -= length; //update remaining data counter
    }
    if (err_cnt) {
        MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\n\r>>> ERROR: check failed\r\n");
    } else {
        MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"SUCCESS\r\n");
    }

#endif

    //program shall be started from local RAM: copy the vector table
    //copy_memory(EMCRAM_BASE_ADDR, ESRAM_BASE_ADDR, 664);
    exeDestAddr = (unsigned long *)ESRAM_BASE_ADDR;
    exeSrcAddr  = (unsigned long *)EMCRAM_BASE_ADDR; //source
    for (ii=0; ii<166; ii++ ) { //copy vector table to local RAM
       *exeDestAddr++ = *exeSrcAddr++;
    }

    //while(MSS_UART_tx_complete(&g_mss_uart0) == 0);
}

size_t UART_Polled_Rx ( mss_uart_instance_t * this_uart, uint8_t * rx_buff, size_t buff_size )
{
    size_t rx_size = 0U;
    uint16_t ii, now = boot_timer;

//    ASSERT( (this_uart == &g_mss_uart0) || (this_uart == &g_mss_uart1) );
    while( rx_size < buff_size )
    {
       while ( this_uart->hw_reg_bit->LSR_DR != 0U  )
       {
           rx_buff[rx_size] = this_uart->hw_reg->RBR;
           ++rx_size;
       }
       if(boot_timer >= BOOT_TIMEOUT_SEC) { //(boot_time_out) ML84 changed
           break;
       } else if (now != boot_timer) {
           now = boot_timer;
           for (ii = 0; ii<(BOOT_TIMEOUT_SEC-boot_timer); ii++) {
               MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)".", 1);
           }
           for (; ii<BOOT_TIMEOUT_SEC; ii++) {
               MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)" ", 1);
           }
           MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"\r", 1);
       }

    }

    return rx_size;
}


void Timer1_IRQHandler(void)
{
    boot_timer++;
    MSS_TIM1_clear_irq();
    //MSS_TIM1_disable_irq();

}

__attribute__((__interrupt__)) void SysTick_Handler(void)
{
    tick_counter++;
}

void flash_loader();
void main_iap();
extern uint32_t compute_spi_crc(uint8_t index);

/****************************************************************/
/****************************************************************/
int main()
{
    uint8_t rx_buff[1], util_reg;
    uint8_t txtbuf[] = "0x00000000 ";
    uint16_t now;
    uint32_t ii;

    uint8_t flash_buf[12];
    uint32_t file_status, file_crc, crc;

    MSS_WD_init(0xFFFFFFFF, MSS_WDOG_NO_WINDOW, MSS_WDOG_RESET_ON_TIMEOUT_MODE); //MAX timeout for init
    MSS_WD_reload();

    /* Init tick counter (periodical interrupts, used by I2C driver and sleep() function)*/
    NVIC_SetPriority(SysTick_IRQn, 0xFFu); /* Lowest possible priority */
    SysTick_Config(100000000 / 100); //every 10 ms


    /* Force shutdown (added v1.13) */
    ii = *(volatile uint32_t*)(CPU2FABRIC_REG2); //read-modify-write
    *(volatile uint32_t*)(CPU2FABRIC_REG2) = (ii | C2F_REG_SHUTDOWN); //force shutdown

    /* Initialization all necessary hardware components */
    MSS_UART_init( &g_mss_uart0,MSS_UART_57600_BAUD,MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT );

    //RESET CONTROL REGISTER: Release drive on MSS_RESET_N
    ii = *(volatile uint32_t*)(SOFT_RST_CR);
    //uint_to_hexstr(ii, txtbuf+2, 8);
    *(volatile uint32_t*)(SOFT_RST_CR) = ( (ii & 0xFFF0FFFF) ); //reset pad disabled, release driven resets

    MSS_TIM1_init(MSS_TIMER_PERIODIC_MODE);
    MSS_TIM1_load_immediate( (uint32_t)APB0_CLOCK_HZ ); //1 second
    MSS_TIM1_enable_irq();

    //Get utility register. Supported values:
    //'5'=0x35: IAP image 0, fabric+eNVM
    //'6'=0x36: IAP image 0, fabric+eNVM
    //'7'=0x37: IAP image 0, fabric+eNVM
    //'8'=0x38: IAP image 0, fabric+eNVM
    //'9'=0x39: IAP image 0, fabric+eNVM
    //'A'=0x41: IAP image 0, fabric+eNVM
    ii = *(volatile uint32_t*)(MBU_MMC_V2B_APB_0); //read version register
    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\n\rRead version register: ");
    uint_to_hexstr(ii, txtbuf+2, 8);
    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\n\r");
    util_reg = ((ii & 0xFF000000) >> 24);
    *(volatile uint32_t*)(MBU_MMC_V2B_APB_0) = 0x0; //reset value to avoid loop

    while(1)
    {
        MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\n\n\r#### MMC2-B Bootloader v1.13 ####\n\r");

        //MSS clock control register
//        ii = *(volatile uint32_t*)(MSS_CLK_CR); //get current value
//        ii &= 0xFFFFFFCF; //reset P1DIVIDER value (/1) in order to speed up SPI flash access
//        *(volatile uint32_t*)(MSS_CLK_CR) = ii; //update register

        if ( (util_reg>='0' && util_reg <= '9') || (util_reg == 'A') ) {
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\n\rAutomatic selection ");
            MSS_UART_polled_tx(&g_mss_uart0,&util_reg, 1);
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\n\r");
            rx_buff[0] = util_reg;
        } else {
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"\n\rPlease Select an option\n\r");
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"0. Application SAFE image (SPI flash 0x300000)\n\r");
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"1. Application active image (SPI flash 0x200000) - DEFAULT\n\r");
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"2. FLASH memory loader\n\r");
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"3. IAP Programmer MENU, Image0\n\r");
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"4. IAP Programmer MENU, Image1\n\r");
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"5. Image0 Fabric+eNVM\n\r");
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"6. Image1 Fabric+eNVM\n\r");
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"7. Image0 Fabric only\n\r");
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"8. Image1 Fabric only\n\r");
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"9. Image0 eNVM only\n\r");
            MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"A. Image1 eNVM only\n\r");

            //wait for timeout or user selection
            MSS_WD_reload();
            now = boot_timer;
            MSS_TIM1_start();
            while( (UART_Polled_Rx( &g_mss_uart0, rx_buff, 1 ) == 0) && (boot_timer < BOOT_TIMEOUT_SEC)); //timeout defined in boot_config.h
            MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"\n", 1);
            MSS_TIM1_stop();

            //if timed-out, select main application
            if(boot_timer >= BOOT_TIMEOUT_SEC)
            {
                MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"Timeout occurred: Booting main application\n\r");
                rx_buff[0] = '1';
            }
        }

        /******************** CHOICES ********************/
        MSS_WD_reload();
        switch(rx_buff[0]) {
            case '1': //Main application, default image (SPI flash => EMC PSRAM)
                //read bitstream info
                FLASH_init();
                FLASH_read(0xF20000, flash_buf, 12);
                file_status = (flash_buf[0]<<24 | flash_buf[1]<<16 | flash_buf[ 2]<<8 | flash_buf[ 3]);
                //file_size   = (flash_buf[4]<<24 | flash_buf[5]<<16 | flash_buf[ 6]<<8 | flash_buf[ 7]);
                file_crc    = (flash_buf[8]<<24 | flash_buf[9]<<16 | flash_buf[10]<<8 | flash_buf[11]);
                crc = compute_spi_crc(2);

                if (file_status != 0xDDDDDDDD) {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"ERROR: Status of main application file is bad. Falling back to safe image.\n\r");
                    //copy IAP application to emcSRAM (1MB from SPI address 0x200000 to 0x70000000)
                    spi_flash_to_emc_sram(APP_SAFE_SPI_ADDR, APP_IMAGE_SIZE_BYTES);
                } else if (file_crc != crc) {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"ERROR: CRC of main application does not match expected CRC. Falling back to safe image\n\r");
                    //copy IAP application to emcSRAM (1MB from SPI address 0x200000 to 0x70000000)
                    spi_flash_to_emc_sram(APP_SAFE_SPI_ADDR, APP_IMAGE_SIZE_BYTES);
                } else {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"CRC check passed: booting default SW image\n\r");
                    //copy IAP application to emcSRAM (1MB from SPI address 0x200000 to 0x70000000)
                    spi_flash_to_emc_sram(APP_DEFAULT_SPI_ADDR, APP_IMAGE_SIZE_BYTES);
                }

                //copy IAP application to emcSRAM (1MB from SPI address 0x200000 to 0x70000000)
                //spi_flash_to_emc_sram(APP_DEFAULT_SPI_ADDR, APP_IMAGE_SIZE_BYTES);

                now = tick_counter;
                while(tick_counter-now < 100);

                //branch
                bx_user_code_esram();
                break;
            case '0': //Main application, safe image (SPI flash => EMC PSRAM)
                //read bitstream info
                FLASH_init();
                FLASH_read(0xF30000, flash_buf, 12);
                file_status = (flash_buf[0]<<24 | flash_buf[1]<<16 | flash_buf[ 2]<<8 | flash_buf[ 3]);
                //file_size   = (flash_buf[4]<<24 | flash_buf[5]<<16 | flash_buf[ 6]<<8 | flash_buf[ 7]);
                file_crc    = (flash_buf[8]<<24 | flash_buf[9]<<16 | flash_buf[10]<<8 | flash_buf[11]);
                crc = compute_spi_crc(3);

                if (file_status != 0xDDDDDDDD) {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"CRITICAL: Status of safe application file is bad. System won't boot\n\r");
                } else if (file_crc != crc) {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"CRITICAL: CRC of main application does not match expected CRC. System won't boot\n\r");
                } else {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"CRC check passed: booting safe SW image\n\r");
                    //copy IAP application to emcSRAM (1MB from SPI address 0x300000 to 0x70000000)
                    spi_flash_to_emc_sram(APP_SAFE_SPI_ADDR, APP_IMAGE_SIZE_BYTES);

                    now = tick_counter;
                    while(tick_counter-now < 100);

                    //branch
                    bx_user_code_esram();
                }

                //copy IAP application to emcSRAM (1MB from SPI address 0x300000 to 0x70000000)
//                spi_flash_to_emc_sram(APP_SAFE_SPI_ADDR, APP_IMAGE_SIZE_BYTES);
//
//                now = tick_counter;
//                while(tick_counter-now < 100);
//
//                //branch
//                bx_user_code_esram();
                break;
            case '2': //FLASH loader
                //ML84 v0.1 flash loader integrated in bootloader
                flash_loader();
                break;

            //MENU
            case '3': //IAP MENU, image0
            case '4': //IAP MENU, image1
                //read bitstream info
                FLASH_init();
                if (rx_buff[0] == '3') {
                    FLASH_read(0xF00000, flash_buf, 12);
                } else {
                    FLASH_read(0xF10000, flash_buf, 12);
                }
                file_status = (flash_buf[0]<<24 | flash_buf[1]<<16 | flash_buf[ 2]<<8 | flash_buf[ 3]);
                //file_size   = (flash_buf[4]<<24 | flash_buf[5]<<16 | flash_buf[ 6]<<8 | flash_buf[ 7]);
                file_crc    = (flash_buf[8]<<24 | flash_buf[9]<<16 | flash_buf[10]<<8 | flash_buf[11]);
                if (rx_buff[0] == '3') {
                    crc = compute_spi_crc(0);
                } else {
                    crc = compute_spi_crc(1);
                }

                if (file_status != 0xDDDDDDDD) {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"CRITICAL: Status of Firmware file is bad. Won't update.\n\r");
                } else if (file_crc != crc) {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"CRITICAL: CRC of Firmware file does not match expected CRC. Won't update\n\r");
                } else {
                    //copy IAP application from eNVM to eSRAM
                    copy_memory(IAP_ENVM_BASE_ADDR, ESRAM_BASE_ADDR, IAP_IMAGE_SIZE_BYTES);

                    //pass custom value on last eSRAM double-word
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"Writing ", 8);
                    if (rx_buff[0] == '3') {
                        *(volatile uint32_t*)(IAP_SPI_ADDR_REG) = FW_GOLDEN_SPI_ADDR;
                        uint_to_hexstr(FW_GOLDEN_SPI_ADDR, txtbuf+2, 8);
                    } else {
                        *(volatile uint32_t*)(IAP_SPI_ADDR_REG) = FW_IMG1_SPI_ADDR;
                        uint_to_hexstr(FW_IMG1_SPI_ADDR, txtbuf+2, 8);
                    }
                    MSS_UART_polled_tx(&g_mss_uart0, txtbuf, 11);
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"at ", 3);
                    uint_to_hexstr(IAP_SPI_ADDR_REG, txtbuf+2, 8); MSS_UART_polled_tx(&g_mss_uart0, txtbuf, 11);
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"\n\r", 2);

                    *(volatile uint32_t*)(IAP_CHOICE_REG)   = 0; //choice=0 => MENU
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"Writing 0x00000000 at ", sizeof("Writing 0x00000000 at "));
                    uint_to_hexstr(IAP_CHOICE_REG, txtbuf+2, 8); MSS_UART_polled_tx(&g_mss_uart0, txtbuf, 11);
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"\n\r", 2);

                    now = tick_counter;
                    while(tick_counter-now < 100);

                    //branch
                    bx_user_code_esram();
                }
                break;
            case '5': //PROGRAM, image0
            case '6': //PROGRAM, image1
                //disable AUTO_I2C core
                //*(volatile uint32_t*)(I2C_AUTO_0+0x8C) = 0;

                //read bitstream info
                FLASH_init();
                if (rx_buff[0] == '5') {
                    FLASH_read(0xF00000, flash_buf, 12);
                } else {
                    FLASH_read(0xF10000, flash_buf, 12);
                }
                file_status = (flash_buf[0]<<24 | flash_buf[1]<<16 | flash_buf[ 2]<<8 | flash_buf[ 3]);
                //file_size   = (flash_buf[4]<<24 | flash_buf[5]<<16 | flash_buf[ 6]<<8 | flash_buf[ 7]);
                file_crc    = (flash_buf[8]<<24 | flash_buf[9]<<16 | flash_buf[10]<<8 | flash_buf[11]);
                if (rx_buff[0] == '5') {
                    crc = compute_spi_crc(0);
                } else {
                    crc = compute_spi_crc(1);
                }

                if (file_status != 0xDDDDDDDD) {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"CRITICAL: Status of Firmware file is bad. Won't update.\n\r");
                } else if (file_crc != crc) {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"CRITICAL: CRC of Firmware file does not match expected CRC. Won't update\n\r");
                } else {
                    //copy IAP application from eNVM to eSRAM
                    copy_memory(IAP_ENVM_BASE_ADDR, ESRAM_BASE_ADDR, IAP_IMAGE_SIZE_BYTES);

                    //pass custom value on last eSRAM double-word
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"Writing ", 8);
                    if (rx_buff[0] == '5') {
                        *(volatile uint32_t*)(IAP_SPI_ADDR_REG) = FW_GOLDEN_SPI_ADDR;
                        uint_to_hexstr(FW_GOLDEN_SPI_ADDR, txtbuf+2, 8);
                    } else {
                        *(volatile uint32_t*)(IAP_SPI_ADDR_REG) = FW_IMG1_SPI_ADDR;
                        uint_to_hexstr(FW_IMG1_SPI_ADDR, txtbuf+2, 8);
                    }
                    MSS_UART_polled_tx(&g_mss_uart0, txtbuf, 11);
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"at ", 3);
                    uint_to_hexstr(IAP_SPI_ADDR_REG, txtbuf+2,8); MSS_UART_polled_tx(&g_mss_uart0, txtbuf, 11);
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"\n\r", 2);

                    *(volatile uint32_t*)(IAP_CHOICE_REG)   = 5; //choice=5 => Program FABRIC+eNVM
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"Writing 0x00000005 at ", sizeof("Writing 0x00000005 at "));
                    uint_to_hexstr(IAP_CHOICE_REG, txtbuf+2, 8); MSS_UART_polled_tx(&g_mss_uart0, txtbuf, 11);
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"\n\r", 2);

                    now = tick_counter;
                    while(tick_counter-now < 100);

                    //branch
                    bx_user_code_esram();
                }
                break;
            case '7': //PROGRAM_ARRAY, image0
            case '8': //PROGRAM_ARRAY, image1
                //disable AUTO_I2C core
                //*(volatile uint32_t*)(I2C_AUTO_0+0x8C) = 0;

                //read bitstream info
                FLASH_init();
                if (rx_buff[0] == '7') {
                    FLASH_read(0xF00000, flash_buf, 12);
                } else {
                    FLASH_read(0xF10000, flash_buf, 12);
                }
                file_status = (flash_buf[0]<<24 | flash_buf[1]<<16 | flash_buf[ 2]<<8 | flash_buf[ 3]);
                //file_size   = (flash_buf[4]<<24 | flash_buf[5]<<16 | flash_buf[ 6]<<8 | flash_buf[ 7]);
                file_crc    = (flash_buf[8]<<24 | flash_buf[9]<<16 | flash_buf[10]<<8 | flash_buf[11]);
                if (rx_buff[0] == '7') {
                    crc = compute_spi_crc(0);
                } else {
                    crc = compute_spi_crc(1);
                }

                if (file_status != 0xDDDDDDDD) {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"CRITICAL: Status of Firmware file is bad. Won't update.\n\r");
                } else if (file_crc != crc) {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"CRITICAL: CRC of Firmware file does not match expected CRC. Won't update\n\r");
                } else {
                    //copy IAP application from eNVM to eSRAM
                    copy_memory(IAP_ENVM_BASE_ADDR, ESRAM_BASE_ADDR, IAP_IMAGE_SIZE_BYTES);

                    //pass custom value on last eSRAM double-word
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"Writing ", 8);
                    if (rx_buff[0] == '7') {
                        *(volatile uint32_t*)(IAP_SPI_ADDR_REG) = FW_GOLDEN_SPI_ADDR;
                        uint_to_hexstr(FW_GOLDEN_SPI_ADDR, txtbuf+2, 8);
                    } else {
                        *(volatile uint32_t*)(IAP_SPI_ADDR_REG) = FW_IMG1_SPI_ADDR;
                        uint_to_hexstr(FW_IMG1_SPI_ADDR, txtbuf+2, 8);
                    }
                    MSS_UART_polled_tx(&g_mss_uart0, txtbuf, 11);
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"at ", 3);
                    uint_to_hexstr(IAP_SPI_ADDR_REG, txtbuf+2,8); MSS_UART_polled_tx(&g_mss_uart0, txtbuf, 11);
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"\n\r", 2);

                    *(volatile uint32_t*)(IAP_CHOICE_REG)   = 9; //choice=9 => Program FABRIC
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"Writing 0x00000009 at ", sizeof("Writing 0x00000000 at "));
                    uint_to_hexstr(IAP_CHOICE_REG, txtbuf+2, 8); MSS_UART_polled_tx(&g_mss_uart0, txtbuf, 11);
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"\n\r", 2);

                    now = tick_counter;
                    while(tick_counter-now < 100);

                    //branch
                    bx_user_code_esram();
                }
                break;
            case '9': //PROGRAM_NVM, image0
            case 'A': //PROGRAM_NVM, image1
                //read bitstream info
                FLASH_init();
                if (rx_buff[0] == '9') {
                    FLASH_read(0xF00000, flash_buf, 12);
                } else {
                    FLASH_read(0xF10000, flash_buf, 12);
                }
                file_status = (flash_buf[0]<<24 | flash_buf[1]<<16 | flash_buf[ 2]<<8 | flash_buf[ 3]);
                //file_size   = (flash_buf[4]<<24 | flash_buf[5]<<16 | flash_buf[ 6]<<8 | flash_buf[ 7]);
                file_crc    = (flash_buf[8]<<24 | flash_buf[9]<<16 | flash_buf[10]<<8 | flash_buf[11]);
                if (rx_buff[0] == '9') {
                    crc = compute_spi_crc(0);
                } else {
                    crc = compute_spi_crc(1);
                }

                if (file_status != 0xDDDDDDDD) {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"CRITICAL: Status of Firmware file is bad. Won't update.\n\r");
                } else if (file_crc != crc) {
                    MSS_UART_polled_tx_string(&g_mss_uart0,(uint8_t *)"CRITICAL: CRC of Firmware file does not match expected CRC. Won't update\n\r");
                } else {
                    //copy IAP application from eNVM to eSRAM
                    copy_memory(IAP_ENVM_BASE_ADDR, ESRAM_BASE_ADDR, IAP_IMAGE_SIZE_BYTES);

                    //pass custom value on last eSRAM double-word
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"Writing ", 8);
                    if (rx_buff[0] == '9') {
                        *(volatile uint32_t*)(IAP_SPI_ADDR_REG) = FW_GOLDEN_SPI_ADDR;
                        uint_to_hexstr(FW_GOLDEN_SPI_ADDR, txtbuf+2, 8);
                    } else {
                        *(volatile uint32_t*)(IAP_SPI_ADDR_REG) = FW_IMG1_SPI_ADDR;
                        uint_to_hexstr(FW_IMG1_SPI_ADDR, txtbuf+2, 8);
                    }
                    MSS_UART_polled_tx(&g_mss_uart0, txtbuf, 11);
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"at ", 3);
                    uint_to_hexstr(IAP_SPI_ADDR_REG, txtbuf+2,8); MSS_UART_polled_tx(&g_mss_uart0, txtbuf, 11);
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"\n\r", 2);

                    *(volatile uint32_t*)(IAP_CHOICE_REG)   = 16; //choice=16 => Program eNVM
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"Writing 0x00000010 at ", sizeof("Writing 0x00000000 at "));
                    uint_to_hexstr(IAP_CHOICE_REG, txtbuf+2, 8); MSS_UART_polled_tx(&g_mss_uart0, txtbuf, 11);
                    MSS_UART_polled_tx(&g_mss_uart0,(uint8_t *)"\n\r", 2);

                    now = tick_counter;
                    while(tick_counter-now < 100);

                    //branch
                    bx_user_code_esram();
                }
                break;
            default:
                key2continue("\n\rIncorrect option. Press any key to continue\n\r");
                boot_timer = 0;
        }
    }
    return 0;
}

