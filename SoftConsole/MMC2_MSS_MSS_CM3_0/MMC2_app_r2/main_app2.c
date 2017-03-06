/*
 * Author: Alessandro Malatesta
 *
 * Version History
 * 1.00 - Base
 * 1.10 - Added UART input polling to display current values and request a reset, added more info on I2C failures
 * 1.20 - Slowed down I2C
 * 1.21 - Added debug printouts
 * 1.30 - Added alarm mask (to disable alarms via SW)
 * 1.40 - Added periodic ISR (timer1) and HSC5 I2C write
 * 1.50 - Added APB regbank & relative test
 * 1.60 - Replaced MSS GPIO with APB register access
 * 1.70 - Added code to read from TMP112 sensor
 * 1.71 - Added code to test I2C EEPROM
 * 1.72 - Added code to test SPI EEPROM
 * 1.73 - Added code to test SPI FLASH
 * 1.74 - Added code to test W5200 ethernet controller
 * 1.75 - Added code to test Elapsed Time Counter
 * 1.76 - Added code to test Real Time Clock
 * 1.77 - Added code to test PWR board EEPROM
 * 1.78 - Added code to test PWR board temperature sensors
 * 1.79 - Added code to test PWR board heater
 * 1.80 - Cleanup: replaced uint32_to_hexstr with uint_to_hexstr
 * 2.00 - Adapted to board Rev B (also removed W5200)
 * 2.01 - Added EMC tests
 * 2.02 - Added FAN controller tests
 * 2.03 - PCA9555: replaced direct control with driver
 * 2.04 - I2C EEPROM: replaced direct control with driver
 * 2.05 - Added OLED display output
 * 2.06 - Updated fan tests
 * 2.07 - Added code to read/write SD card on uOLED module.
 * 2.08 - Read board info from EEPROM and print it to OLED
 * 2.09 - Added Ethernet test (uIP stack, ip displayed on OLED)
 * 2.10 - Added timeout on all user input
 * 2.11 - Added OLED display auto switch-off feature
 * 2.12 - Added Energy Meter tests
 * 2.13 - Added support for single-supply use (save supply to use in EEPROM)
 * 3.00 - Added code to support power supply switching.
 * 3.01 - MASKED Heater HT alarms
 * 3.02 - Added power/voltage measure option
 * 3.03 - Changed supply switch logic: never switch from 2 to 1. Use 2 when both fail
 * 3.04 - Added shunt resistor value as parameter to display power meter readings
 * 3.05 - REMOVED heater alarm masking, added "first boot" init code
 * 3.06 - Added control on PCLK1 divider
 * 3.07 - Made minor changes to test functions, disabled EMC tests to avoid memory corruption
 * 3.08 - Fixed RTC display
 * 3.09 - Test tftp
 * 3.10 - DEBUG
 * 3.11 - I2C TMP controller completely moved to FPGA fabric.
 * 3.12 - Changed Temperature read and Heater read/write (use I2C_auto FW block)
 * 3.13 - Added self-reset Watchdog
 * 3.14 - DEBUG
 * 3.15 - New test functions, added "test-all" option
 * 3.16 - Added TFTP server
 * 3.17 - Added command interface, DHCP client timeout
 * 3.18 - Adapted to new FW version (I2C GPIOs controlled by FW)
 * 3.19 - Changed OLED display color to WHITE, new format for automatic test output, added TFTP reset command, added MAC read from EEPROM
 * 3.20 - Added TFTP commands for IAP programming
 * 3.21 - Added control on MAC address validity (trying to set a multicast mac causes CPU reset)
 * 3.22 - Added EOF at end of commands. Added commands (V, +KH, +KR, +OW, +OR). Converted menu commands to same compact format.
 * 3.23 - Added support for custom commands to fabric MSS I2C controller (FAN controllers). Added FAN temperature read.
 * 3.24 - Enabled HP filters and AFC in PDM energy meter
 * 3.25 - Changed power meters' VSENSE display to CURRENT.
 *        Wait 15 seconds before confirming a power module failure.
 *        Load energy meter calibration from PWD board EEPROM (page1, 3+3 bytes).
 *        Display currently used power supply on OLED
 * 3.26 - Enabled I2C slave to receive data from GPAC
 * 3.27 - Implemented remote update via GPAC (receive data via I2C slave)
 * 3.28 - Changed +OW command to write always 512 Bytes of data (1 whole SD-Card sector)
 * 3.29 - Added power-down command (+X0, +X1), Added TFTP commands PWR_ON/PWR_OFF, changed FAN temperature printout (degC/100 => cdegC)
 * 3.30 - Corrected scaling factors for power meters' ADIN display
 * 4.00 - Implement I2C read from GPAC
 * 4.14D- Remove power meter init from test function, added to main init section
 * 4.17D- Added debug info to i2c_slave ISR. Added PDM to regmap.
 * 4.18D- Added update time counter. Increased power-failure confirmation time to 60 seconds. Added FAN reading.
 * 4.19D- Changed format of regmap display. Enable power supply monitoring after 60 seconds from SW startup. Added read from SDCARD.
 * 4.20D- Disabled read from TEMP sensor. Release forced shutdown after init (shutdown forced in bootloader).
 *        Disabled access to I2C eeprom and temp sensor when GPAC is active.
 * 4.21D- Write magic word to SPI flash 0xF00000 when SPI flash update via GPAC is successful.^Added SPI flash 0xF00000 to regmap.
 * 4.22D- Added command to read from SPI flash.
 * 4.24D- Changed flash erase procedure during remote programming from GPAC.
 * 4.25D- Added force shutdown at startup (removed from bootloader).
 * 4.30D- Added CRC check for GPAC programming
 * 4.31D- Added CRC check menu option
 * 4.38D- Added CRC computation to TFTP
 * 4.39D- Added command to corrupt SPI flash
 * 4.40D- Added forced shutdown before all calls to reset function.
 * 4.40 - Release (changes from 4.14D to 4.40D)
 * 4.41 - Corrected display of N12V (use ADIN for VIN and compute power instead of reading it)
 * 4.42 - Added size param to sd_write function. Added SDcard write from GPAC.
 * 4.43 - added wdreload() during dhcp, Fixed ARP problem. Added host name in BOOTP.
 * 4.44 - Implemented DHCP retry, added validity check on hostname, added IP on register map
 * 4.45 - Added FAN I2C "read debug info" and "reset command". Added fan reset command to TFTP and GPAC command set (code 8)
 * 4.46 - Removed wdreload() on I2C timeout. Bypass i2c_pwr read during regmap update when a transaction fails.
 *        Inserted wait before reset. Disable DHCP when I2C slave is active.
 *   D0 - Added uptime display. Added eth link detection.
 *   D1 - DHCP on cable connection
 *   D2 - Disabled PCLK change
 *   D3 - Delayed start of regmap update by 10 seconds. Fabric reset at startup.
 *   D4 - Removed fabric reset at startup
 *   D5 - Repeat TFTP init
 *   D6 - Debug info on GPIO
 *   D8 - Enable/disable command
 *   D9 - Automatic fabric reset
 *   D10 - No auto fabric reset
 *   D11 - Reprogram FPGA when stuck
 *   D12 - Add MAC address display
 *   D13 - Fixed bug: update of bitstream info in FLASH (missing sector erase)
 *   D14 - Fixed bug: cable connected but no DHCP caused a loop
 *   D15 - updated bootp packet content (added client ID [61] and requested list[55])
 *   D16 - updated bootp packet content
 *   D17 - Removed self reprogramming
 *   D18 - Don't repeat mac init on cable reconnection
 *   D19 - WD reload during safe procedures
 *   D20 - I2C auto enable at start
 *   D21 - Repeat check on shutdown assertion before self reset
 *   D22 - Repeat I2C_AUTO enable
 *   D23 - Reset GPIO
 * 5.00 - Replaced auto_i2c with core_i2c
 * 5.01D - Pause after bad I2C access, increased i2c timeout from 1 to 3 s
 *   D1 -- Added generic APB access
 *   D3 -- Added hsc5 alert enable
 * 5.02 - Moved control bits SUPPLY2, SHUTDOWN, HSC5_ALERT_EN
 * 5.03 - self reset when gpio does not respond
 * 5.04 - Updated drivers
 * 5.05 - Reset dhcp_ip_found and ip_known on cable disconnect. Corrected DHCP timer.
 * 5.06 - Fixed HS temp display
 *   D0 - added dhcp debug prints
 *   D1 - modified dhcp code to wait for DHCP ack
 * 5.07 - Disabled uptime display, fixed bug in energy_monitor read
 * 5.08 - Fixed bug that caused reset when MMC EEPROM does not respond, hardwired 1st i2c write
 * 5.09 - DISABLED ETHERNET
 * 5.09D0 - Changed OLED init
 * 5.10 - Changed slave I2C timeout from 5 to 10 seconds
 * 5.11 - Removed printout of RTC errors during register map update
 * 5.12 - Fixed bug that didn't allow USE_SUPPLY2 flag to be displayed, added read of fan speed from EEPROM, setting of FAN speed
 * 5.13 - Inserted 20 seconds pause before power-up
 * 5.14 - Re enabled Ethernet, reverted DHCP changes (DHCP working), changed BOOTP timeouts, send logic, TTL (32->128), SECONDS (100->0). Added client ID
 * 5.15 - added network options on MMC EEPROM page 5 (fixed/dynamic address)
 * 5.16 - DHCP fix
 *     .0 - Split mac initialization and IP retrieval in two different functions
 *     .1 - added dhcp status
 *     .2 - implemented DHCP request retry
 *     .3 - added server id and requested address options to DHCPREQUEST packet
 *     .4 - implemented rebind timer
 *     .5 - added hostname to DHCP packets
 * 5.17 - Fixed oled color setting on errors. Added additional info on OLED.
 *     .1 - replaced DHCP RENEW with a new REQUEST, added command to force DHCP renew
 *     .2 - added command to set DHCP renew time
 *     .3 - fix on forced lease renew
 *     .4 - added netif structure update after obtaining a new IP address
 * 5.18D  - Removed start delay.
 * 5.18   - Re-enabled start delay, fixed problem with oled dim out
 * 5.18D2 - Removed start delay, added timed shutdown I2C command (CMD9)
 * 5.18D3 - Fixed bug with timed shutdown command
 * 5.19   - Added CMD10 to write 4 bytes at position 0x200 in I2C table. Written value will be inverted. (added ptr32)
 * 5.19.1 - Added command to reset and reinitialize MSS I2C
 * 5.19.2 - Fixed ISR FSM reset after timeout
 * 5.19.3 - Changed ISR FSM timeout from 10 s to 500 ms
 * 5.19.4 - Changed MSS_I2C0 configuration: FREQ=100, GLITCHREG=6
 * 5.19.5 - Slowed down MSS_I2C to 100M/960=104KHz
 * 5.19.6 - Modified command to reset/re-initialize I2C. Added code for I2C self check. Added command to enable/disable self check.
 * 5.19.7 - Automatic reinitialization of I2C driver whenever GPAC is removed (SPE 1->0)
 * 5.20   - Updated TFTP help messages
 * 5.21   - Changed addressing mode for I2C slave reads: pointer to TX buffer is handled directly
 * 5.21.1 - Added flash read and write buffers
 * 5.21.2 - Added commands to display buffers
 * 5.21.3 - Set up framework for GPAC3 commands
 * 5.21.4 - Implemented Reset, Timed-shutdown and SDcard-access I2C commands (GPAC3)
 * 5.21.5 - Implemented FLASH-read I2C command (GPAC3)
 * 5.21.6 - Dummy version of FLASH erase command (GPAC3)
 * 5.21.7 - Enabled FLASH-Erase and FLASH-Program commands (GPAC3)
 * 5.21.8 - Added Write-File-Size command (GPAC3)
 * 5.21.9 - Added CRC dummy command
 * 5.21.10- Added LOCK feature to GPAC3 commands (protection against accidental execution of some commands)
 * 5.21.11- Added IAP commands (GPAC3)
 * 5.22   - GPAC3 I2C commands. Release
 * 5.23   - GPAC3 I2C commands: extended SD card access commands.
 */

#include <string.h>
#include "MMC2_hw_platform.h"
#include "main.h"
#include "TMP112.h"
#include "TC74.h"
#include "AD5321.h"
#include "LTC2945.h"
#include "max31785.h"
//#include "pca9555.h"
#include "i2c_eeprom.h"
#include "ds1682.h"
#include "pca2129.h"
#include "spi_eeprom.h"
#include "uart_utility.h"
#include "goldelox_port.h"
#include "GOLDELOX_SERIAL_4DLIBRARY.h"
#include "s25fl128s.h" //spi flash
#include "S29GLxxxP.h" //parallel flash
//#include "eth_test.h"
#include "cs5480.h"
#include "I2C_auto.h"
#include "test_functions.h"
#include "tcp.h"
#include "etharp.h"
#include "mss_ethernet_mac.h"
#include "mss_i2c_controller.h"
#include "core_i2c.h"
#include "ethernet.h"

/* VERSIONS */
const uint8_t sw_version[]   = "5.23\n\r";
uint8_t       fw_version[]   = "000\n\r\0";
uint8_t       fw_version_x[] = "105\n\r\0"; //expected FW version

/* global variables used in ISRs */
volatile uint8_t  update_regmap = 0;
volatile uint32_t tick_counter = 0;

uint8_t  power_fail_alarm = 0;
uint32_t power_fail_time;

/* i2c slave */
#define I2C_SLAVE_TXBUF_SIZE 512
#define I2C_SLAVE_RXBUF_SIZE 512

#define FLASH_BUF_LEN    256
#define CMD_BUF_LEN      32
#define CMD_WBUF_START   0x0800
#define CMD_RBUF_START   0x0900
#define FLASH_WBUF_START 0x1000
#define FLASH_RBUF_START (FLASH_WBUF_START+FLASH_BUF_LEN)

#define CMD_BUF_RES_OFF 4
#define CMD_BUF_ADR_OFF 8
#define CMD_BUF_DAT_OFF 12
#define CMD_BUF_KEY_OFF 16

uint8_t cmd_buf[CMD_BUF_LEN];
uint8_t flash_rbuf[FLASH_BUF_LEN];
uint8_t flash_wbuf[FLASH_BUF_LEN];

volatile uint8_t i2c_slave_tx_buf[I2C_SLAVE_TXBUF_SIZE];
uint8_t i2c_slave_rx_buf[I2C_SLAVE_RXBUF_SIZE];

/* other global variables */
uint16_t i2c_input_old, i2c_input_new, i2c_output_old, i2c_output_new; //I2C GPIOs
uint32_t fabric2cpu_new, fabric2cpu_old, cpu2fabric_new, cpu2fabric_old; /* gpio read value and local copy */
uint8_t  power_fail_new, power_fail_old; //supply1 0x1, supply2 0x2

const mss_i2c_clock_divider_t I2C_DIV = MSS_I2C_PCLK_DIV_160; /* 50M/160=312KHz */
const uint32_t TOUT = 60*0x64; //USER INPUT TIMEOUT. 0x64 = 1s when system tick = 10ms
const uint32_t OLED_TOUT = 60*0x64; //OLED DISPLAY DIMMING TIMEOUT

#define WD_1SEC 100000000u //1second at 100 MHz
const uint32_t WD_TOUT = 10*WD_1SEC; //max 42 seconds @ 100MHz

uint32_t oled_update_time;

int  tftp(uint8_t do_mac_init, uint8_t network_mode);
volatile unsigned char my_hostname[8];
volatile unsigned char my_mac[6];// = {0x02,0x04,0x25,0x40,0x40,0x40}; //mac address. Removed from tcpip.c

void ethernetif_input(void * );
void show_ip(char *ipStr);
char ipStr[20];
volatile unsigned char my_ip[4] = {127,0,0,1};

static uint8_t network_en, network_mode, dhcp_force_renew = 0;
static uint32_t dhcp_lease_timeout;

uint32_t pdm_v1_gain, pdm_v2_gain;

//I2C SLAVE FSM
#define I2C_S_BUF_LEN           256 //writable to SPI in a single transaction
static uint8_t  i2c_s_buf[I2C_S_BUF_LEN]; //rx data buffer
volatile uint32_t i2c_s_timer   = 0;    //used for command timeout
static uint8_t  i2c_s_tout    = 0x00; //used for command timeout
static uint8_t  i2c_s_status  = 0x00; //FSM status
static uint8_t  i2c_s_cmd     = 0xFF; //received command
static uint8_t  i2c_s_sd_access = 0x00; //access SD card
static uint32_t i2c_s_len     = 0x00000000; //length of received file
static uint32_t i2c_s_waddr   = 0x00000000; //spi EEPROM write address
static uint32_t i2c_s_cnt     = 0x00000000; //buffer index
static uint32_t i2c_s_dcnt    = 0x00000000; //data counter
static uint32_t i2c_s_crc     = 0x00000000; //computed crc
static uint32_t expected_crc  = 0x00000000;
static uint32_t update_time = 0;
static uint32_t start_time  = 0;
static uint32_t uptime = 0;
static uint32_t dhcp_lease_tick  = 0; //used to count one second
static uint32_t dhcp_lease_count = 0; //used to count DHCP lease time in seconds
static uint8_t core_i2c_errcnt = 0;
static uint8_t timed_shutdown = 0;
static uint8_t i2c_s_access = 0;

extern unsigned char dhcp_ip_found;
extern unsigned char ip_known;
extern unsigned int  dhcp_lease_time; //got from dhcp
extern void get_ip(char *ipStr, uint8_t network_mode);

uint32_t crc32_1byte(const void* data, size_t length, uint32_t previousCrc32);
uint32_t compute_spi_crc(uint8_t index);
void write_result_reg(uint8_t *buf, uint16_t cmd, uint16_t data);

extern uint8_t LTC2945_errcnt;

//void print_i2c_drv_status(mss_i2c_instance_t * i2c); //_FIXME remove

uint8_t pwr_switch_dbg = 0;

#define POWERUP_DELAY_S 10

/* ********************************** MAIN *****************************************/
int main()
{
	/********************** declarations **************************/
    uint8_t tx_buf[4];
    uint8_t rx_buf[4];
	uint8_t sdcard_ok, first_boot = 1, cnt;
    uint32_t rval = 0x00000000; // rval1;
	uint8_t text_buf[64]; //, regmap_update_en = 0;
    uint32_t arp_timer = 0; //make static?
    int32_t eth_status;
	//uint16_t tcp_timer = 0;
    uint16_t oled_color = 0xFFFF;
    uint32_t *ptr32;
    uint8_t old_spe = 0;
    uint8_t new_spe = 0;

//    uint8_t rx_buffer[1600];
//    char ipStr[20];

	/************************************ HARDWARE initialization **************************/
	/* Set up the Watch Dog Timer */
    MSS_WD_init(0xFFFFFFFF, MSS_WDOG_NO_WINDOW, MSS_WDOG_RESET_ON_TIMEOUT_MODE); //MAX timeout for init
    MSS_WD_reload();

    /* Init tick counter (periodical interrupts, used by I2C driver and sleep() function)*/
    NVIC_SetPriority(SysTick_IRQn, 0xFFu); /* Lowest possible priority */
    SysTick_Config(CPU_FREQ / 100); //every 10 ms

    /* Initialize and configure UART0 (console) */
    MSS_UART_init(&g_mss_uart0, MSS_UART_57600_BAUD, MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);


    //reset register that forces reboot
    cnt = 0;
    while(cnt < 5) {
        rval = HW_get_32bit_reg(MBU_MMC_V2B_APB_0);
        if (rval & 0xFF000000) {
            dbg_print("Warning: reboot register was not reset: 0x");
            dbg_printnum(rval,8);
            dbg_print(". Resetting...\n\r");
            HW_set_32bit_reg(MBU_MMC_V2B_APB_0, 0);
            cnt++;
        } else {
            cnt = 5;
        }
    }

    /*********************************** INIT ON CHIP PERIPHERALS **************************************/
    /* init and config interrupt controller */
    config_irq(); //set IRQ controller to known status
    NVIC_EnableIRQ( Fabric_IRQn ); //enable IRQ from FPGA fabric

    /* init and config I2C */
    config_i2c(); //init I2C controllers and configure I2C GPIO ICs

    /* init and config SPI */
    config_spi(); //config SPI controllers

    config_gpios(); //only for debug purposes

    //FORCE SHUTDOWN
    rval = HW_get_32bit_reg(CPU2FABRIC_REG2);
    if ((rval & C2F_REG_PWR_CYC) == 0xABCD0000) { //_TODO: when shutdown is bypassed, no write access should be made to I2C eeprom
        dbg_print("Bypassing shutdown.\n\r");
        HW_set_32bit_reg(CPU2FABRIC_REG2, (rval & 0x0000FFFF));
    } else {
        dbg_print("Forcing shutdown...\n\r");
        HW_set_32bit_reg(CPU2FABRIC_REG2, (rval | C2F_REG_SHUTDOWN | C2F_REG_HSC5_EN ));
    }

	/********************************* GENERAL SW STARTUP  *********************************/
    MSS_WD_reload();
	//clear_console(10);
	/* Send welcome logo over UART */
	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) welcomeLogo);

	rval = HW_get_32bit_reg(CPU2FABRIC_REG2);
	if ((rval & C2F_REG_PWR_CYC) == 0xABCD0000) {
	    first_boot = 0;
	} else {
	    //get info string from EEPROM and display it on OLED (page 0, max 16 bytes)
	    i2c_eeprom_read(&g_mss_i2c0, I2C_EEPROM_SER_ADDR, 0x00, text_buf, 16, (const uint8_t *) "Read page0 from MMC I2C EEPROM: ");
	    text_buf[6] = '\0'; //compare only first 6 characters
	    if (strcmp((char*) text_buf, "MMC2-B")) { //strings differ
	        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n*** First boot: writing board ID and power supply status to EEPROM ***\n\r");
	        memcpy(text_buf, "MMC2-B\0", 6);
	        i2c_eeprom_write(&g_mss_i2c0, I2C_EEPROM_SER_ADDR, 0x00, text_buf, 6, (const uint8_t *) "Write page0 to MMC I2C EEPROM: "); //one-time
	        ptr32 = (uint32_t*)(text_buf); //added in 5.19
            //((uint32_t*)(text_buf))[0] = 0x3CCCCC3C; //careful: reversed byte order
            //((uint32_t*)(text_buf))[1] = 0x0000CCCC;
            ptr32[0] = 0x3CCCCC3C; //careful: reversed byte order
            ptr32[1] = 0x0000CCCC;
	        i2c_eeprom_write(&g_mss_i2c0, I2C_PWR_EE_SER_ADDR, 0x10, text_buf, 6, (const uint8_t *) "Write page1 to PWR I2C EEPROM: "); //one-time
	    } else {
	        first_boot = 0;
	    }
	}


	/* Display Software Version */
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Software Version: ");
    MSS_UART_polled_tx_string( &g_mss_uart0, sw_version);


	/* Read & Display Firmware Version */
	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\rFirmware Version: ");
	rval = HW_get_32bit_reg(VER_REG); //read FW version register
	uint_to_decstr((rval&0xFFFF), (uint8_t*) fw_version, 3); //convert to readable format
	MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) fw_version); //display

	//Check compatibility between SW and FW version
    if (strcmp((char*) fw_version, (char*) fw_version_x)) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r>>>WARNING<<< This Software version has been tested only with FW version ");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) fw_version_x);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
        //if (oled_en) txt_FGcolour(0xF800); //red rgb565
        oled_color = 0xF800; //RED RGB656
    }

    //Read fan speeds from EEPROM Page 4
    dbg_print("\n\rReading Fan speeds from MMC EEPROM page 4:\n\r");
    i2c_eeprom_read(&g_mss_i2c0, I2C_EEPROM_SER_ADDR, 0x40, (uint8_t*) text_buf, 6, (const uint8_t *) "Read page4 from MMC I2C EEPROM: ");

    rval = (text_buf[0]<<8 | text_buf[1]); //front-top fans
    if ((rval==0) || (rval==0xFFFF)) {
        dbg_print("  WARNING: bad speed in EEPROM for FRONT-TOP fans. Using default.\n\r");
        rval = FAN_RPM_FU_DEF;
    }
    dbg_print("    Front Top Speed (RPM)    = 0x");
    dbg_printnum(rval,4);
    dbg_print("\n\r");
    HW_set_32bit_reg(MSS_I2C_CONTROLLER_0+0x4, rval);

    rval = (text_buf[2]<<8 | text_buf[3]); //front-bottom fans
    if ((rval==0) || (rval==0xFFFF)) {
        dbg_print("  WARNING: bad speed in EEPROM for FRONT-BOTTOM fans. Using default.\n\r");
        rval = FAN_RPM_FB_DEF;
    }
    dbg_print("    Front Bottom Speed (RPM) = 0x");
    dbg_printnum(rval,4);
    dbg_print("\n\r");
    HW_set_32bit_reg(MSS_I2C_CONTROLLER_0+0x8, rval);

    rval = (text_buf[4]<<8 | text_buf[5]); //back fans
    if ((rval==0) || (rval==0xFFFF)) {
        dbg_print("  WARNING: bad speed in EEPROM for BACK fans. Using default.\n\r");
        rval = FAN_RPM_B_DEF;
    }
    dbg_print("    Back Speed (RPM)         = 0x");
    dbg_printnum(rval,4);
    dbg_print("\n\r");
    HW_set_32bit_reg(MSS_I2C_CONTROLLER_0+0xC, rval);


    //Read MAC address from EEPROM page 2
    i2c_eeprom_read(&g_mss_i2c0, I2C_EEPROM_SER_ADDR, 0x20, (uint8_t*) my_mac, 6, (const uint8_t *) "Read page2 from MMC I2C EEPROM: ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rRead following MAC address from EEPROM:\n\r");
    memcpy(text_buf, " 00\0", 4);
    for (rval=0; rval<6; rval++) {
        uint_to_hexstr(my_mac[rval], text_buf+1, 2);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
    if ((my_mac[0]&1) != 0 ) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "WARNING: mac address read from EEPROM is not unicast. Network won't be initialized\n\r");
        network_en = 0;
    } else {
        network_en = 1;
    }

    //read network options from MMC EEPROM page 5
    if (network_en) {
        i2c_eeprom_read(&g_mss_i2c0, I2C_EEPROM_SER_ADDR, 0x50, (uint8_t*) text_buf, 5, (const uint8_t *) "Read page5 from MMC I2C EEPROM: ");
        dbg_print("Network settings (EEPROM PAGE 5): ");
        if (text_buf[0] == 'S') {
            dbg_print("Static IP\n\r");
            network_mode = 'S';
            my_ip[0] = text_buf[1];
            my_ip[1] = text_buf[2];
            my_ip[2] = text_buf[3];
            my_ip[3] = text_buf[4];
        } else if (text_buf[0] == 'D') {
            dbg_print("Dynamic IP\n\r");
            network_mode = 'D';
        } else {
            dbg_print("Network disabled\n\r");
            network_en   = 0;
            network_mode = 0;
        }
    }

    //Read PDM gain from PWR board EEPROM page 1
    i2c_eeprom_read(&g_mss_i2c0, I2C_PWR_EE_SER_ADDR, 0x10, text_buf, 6, (const uint8_t *) "Read page1 from PWR I2C EEPROM: ");
    pdm_v1_gain = (text_buf[0]<<16) | (text_buf[1]<<8) | text_buf[2];
    pdm_v2_gain = (text_buf[3]<<16) | (text_buf[4]<<8) | text_buf[5];
    memcpy(text_buf, "0x000000\n\r\0", 11);
    if (pdm_v1_gain == 0) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "WARNING: PDM_V1_GAIN read from EEPROM is zero. Using default value 0x400000\n\r");
        pdm_v1_gain = 0x400000;
    } else {
        uint_to_hexstr(pdm_v1_gain, text_buf+2, 6);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rRead following PDM_V1_GAIN from EEPROM: ");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }
    if (pdm_v2_gain == 0) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "WARNING: PDM_V2_GAIN read from EEPROM is zero. Using default value 0x400000\n\r");
        pdm_v2_gain = 0x400000;
    } else {
        uint_to_hexstr(pdm_v2_gain, text_buf+2, 6);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Read following PDM_V2_GAIN from EEPROM: ");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
    }

    /**************************INIT OLED DISPLAY****************************/
    TimeLimit4D = 500; //set timeout for OLED commands (5 seconds, number depends on SysTick resolution)
    Callback4D  = Callback ; //callback defined in goldelox_port.h;
    ComHandle4D = &g_mss_uart1;
    MSS_UART_init( ComHandle4D, MSS_UART_9600_BAUD, MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT );

    //clear the display
    //dbg_print("OLED CLS\n\r");
    gfx_Cls(); //always do this

    //unlock display if NACK is received
    if (oled_error == Err4D_NAK) {
        dbg_print("OLED unlock\n\r");
        oled_unlock();
    }
    oled_en = 1; //oled enable. if any OLED function times out / returns NACK, this bit is reset (avoid getting stuck on crazy display)
    oled_contrast = 15;
    if (oled_en) {
        //dbg_print("OLED set contrast\n\r");
        gfx_Contrast(oled_contrast);
    }
    if (oled_en) {
        //dbg_print("OLED set timeout\n\r");
        SSTimeout(0xFFFF); //max timeout before screen saver
    }
    if (oled_en) {
        //dbg_print("OLED CLS\n\r");
        gfx_Cls();
    }
    if (oled_en) {
        //dbg_print("OLED set color\n\r");
        txt_FGcolour(oled_color);
    }

    //Display SW version on oled
    if (oled_en) {
        putstr((unsigned char*)"MMC2-B\n") ;
        putstr((unsigned char*)"SW v");
        putstr((unsigned char*)sw_version);
        putstr((unsigned char*)"FW v");
        putstr((unsigned char*)fw_version);
        //txt_FGcolour(0xFFFF); //white
    }

    //SDCARD on OLED module
	if (oled_en) {
	    sdcard_ok = media_Init() ;
	    if (!sdcard_ok) {
	        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t*) "NO uSD card in uOLED module\n\r");
	    } else {
	        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t*) "\n\rReading data from uSD card\n\r");
	        //Read serials
	        text_buf[0] = 0x00;
	        text_buf[1] = 0x00;
	        text_buf[2] = 0x00;
	        text_buf[3] = 0x00;
	        sd_read(text_buf, (uint8_t*)i2c_slave_tx_buf+0x1E0, 8);
	        /* 0x01E8-0x01EF SDCARD content from offset 0x100 */
	        text_buf[2] = 0x02;
	        sd_read(text_buf, (uint8_t*)i2c_slave_tx_buf+0x1E8, 8);
	        /* 0x01F0-0x01F7 SDCARD content from offset 0x200 */
	        text_buf[2] = 0x04;
            sd_read(text_buf, (uint8_t*)i2c_slave_tx_buf+0x1F0, 8);
            sd_read(text_buf, (uint8_t*)my_hostname, 8);

	        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t*) "  Front panel version: ");
	        MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t*)(i2c_slave_tx_buf+0x1E0), 8);
	        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t*) "\n\r");

            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t*) "  Backplane version  : ");
            MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t*)(i2c_slave_tx_buf+0x1E8), 8);
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t*) "\n\r");

            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t*) "  Crate serial       : ");
            //MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t*)(i2c_slave_tx_buf+0x1F0), 8);
            MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t*)my_hostname, 8);
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t*) "\n\r");

	        //media_SetSector(0, 0) ;
	    }
	}


    /************************************ APPLICATION STARTUP ***************************************/
    MSS_WD_reload();

	/* Write input values from I2C to fabric *******************************************/
	//on first boot mark both power supplies as good
	if (first_boot) {
	    power_fail_new = 0x0; //both power supplies good
	    i2c_eeprom_write(&g_mss_i2c0, I2C_EEPROM_SER_ADDR, 0x10, &power_fail_new, 1, (const uint8_t *) "Write page1 to MMC I2C EEPROM: ");
	}

    //read power supply status from EEPROM
    power_fail_new = 0xF; //reset buffer, then read
    i2c_eeprom_read(&g_mss_i2c0, I2C_EEPROM_SER_ADDR, 0x10, &power_fail_new, 1, (const uint8_t *) "Read page1 from MMC I2C EEPROM: ");
    power_fail_old = power_fail_new;
    if (power_fail_old & 0x03) { //OLED written in RED if something's wrong
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r>>>WARNING<<< At least one power supply is marked as defective.\n\r");
        //if (oled_en) txt_FGcolour(0xF800); //red rgb565
        oled_color = 0xF800;
    }

    //get previous state
    cpu2fabric_old = HW_get_32bit_reg(CPU2FABRIC_REG);
    cpu2fabric_old &= 0xFFFF0000; //clear bits to be updated (GPI, SHUTDOWN, POWER SUPPLY)

    //get current GPI
    tx_buf[0] = 0; //input register
    if(core_i2c_doread(&g_core_i2c_pm, I2C_GPI_SER_ADDR, tx_buf, 1, rx_buf, 2, (uint8_t*) "Get GPI")) {
        dbg_print("\n\r*** First GPI read FAILED ***\n\r");
//        sleep(5000);
//        NVIC_SystemReset();
        cpu2fabric_new = (cpu2fabric_old | 0x3FFF);
    } else {
        cpu2fabric_new = ((rx_buf[1] << 8) | rx_buf[0]); //LSB first
        dbg_print("\n\rFirst GPI read = 0x"); dbg_printnum(cpu2fabric_new,4); dbg_print("\n\r");
        cpu2fabric_new &= 0x3FFF; //keep only used values
        cpu2fabric_new |= cpu2fabric_old; //merge with previous value
    }

    //if both good (00) start with supply 1
    //if both bad (11) start with supply 2
    //if 01 start with supply 2
    //if 10 start with supply 1
    rval = HW_get_32bit_reg(CPU2FABRIC_REG2);
    if (power_fail_old & 0x01) { //supply 1 bad: start with 2 (even when both are bad)
        //cpu2fabric_old = cpu2fabric_new | C2F_REG_USE_V2;
        HW_set_32bit_reg(CPU2FABRIC_REG2, rval | C2F_REG_USE_V2);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rUsing power supply 2\n\r");
        if (oled_en) {
            txt_MoveCursor(0x0005,0x0000);
            putstr((unsigned char*)"Supply 2\n");
        }
    } else { //use power supply 1 whenever it's good
        //cpu2fabric_old = cpu2fabric_new; //~C2F_REG_USE_V2;
        HW_set_32bit_reg(CPU2FABRIC_REG2, rval & ~C2F_REG_USE_V2);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rUsing power supply 1\n\r");
        if (oled_en) {
            txt_MoveCursor(0x0005,0x0000);
            putstr((unsigned char*)"Supply 1\n");
        }
    }
    //set fabric inputs
    HW_set_32bit_reg(CPU2FABRIC_REG, cpu2fabric_old );
    dbg_print("First CPU2FABRIC write = 0x"); dbg_printnum(cpu2fabric_old,8); dbg_print("\n\r");
    //get facbric outputs
    fabric2cpu_new = HW_get_32bit_reg(FABRIC2CPU_REG);
    dbg_print("First FABRIC2CPU read = 0x"); dbg_printnum(fabric2cpu_new,8); dbg_print("\n\r");


    /**********************************************************************************************************
     ************************************* FAILSAFE STUFF *****************************************************
     **********************************************************************************************************/

    if (network_en) {
        dbg_print("\n\rmain:: Network is enabled\n\r");
        tftp(1, network_mode); //init TFTP server, do mac init

        eth_status = MSS_MAC_link_status();
        if (eth_status & MSS_MAC_LINK_STATUS_LINK) {
            dbg_print("main:: ETH cable connected\n\r");
        } else {
            dbg_print("main:: ETH cable not connected, disabling network\n\r");
            network_en = 0;
        }
    } else {
        dbg_print("main:: Network is disabled.\n\r");
    }
    dbg_print("\n");

    if (oled_en && !network_en) {
        txt_MoveCursor(0x0003, 0x0000); //start of line 4
        putstr((unsigned char*)"ETH OFF\n") ;
    }

    /* START MONITORING *************************************************************/
    MSS_WD_reload();
    oled_update_time = tick_counter;

    /* initalize I2C TX BUFFER */
    for (rval = 0; rval<I2C_SLAVE_TXBUF_SIZE; rval++) {
        i2c_slave_tx_buf[rval] = (uint8_t)rval;
    }

    /***********************************************************************************************************
     * initialize power meter and start conversion
     */
    MSS_WD_reload();
    //reset
    cs5480_instruction(CS5480_I_SW_RST);
    if (cs5480_wait_drdy(&power_meter)) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) " error: Power Meter not responding\n\r");
    }
    //initialize parameters
    if (cs5480_set_parameters(&power_meter, pdm_v1_gain, pdm_v2_gain)) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) " error: Cannot configure power meter\n\r");
    }
    //start continuous conversion
    cs5480_instruction(CS5480_I_CONT);
    cs5480_wait_drdy(&power_meter);

    //RELEASE SHUTDOWN
    dbg_print("Releasing shutdown in "); dbg_printnum_d(POWERUP_DELAY_S,3); dbg_print(" s\n\r");
    MSS_WD_reload();
    sleep(1000*POWERUP_DELAY_S);
    MSS_WD_reload();
    rval = HW_get_32bit_reg(CPU2FABRIC_REG2);
    HW_set_32bit_reg(CPU2FABRIC_REG2, (rval & (~C2F_REG_SHUTDOWN) ));

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r>>> RUNNING >>>\n\r");
    start_time = tick_counter;
    uptime = 0;

    MSS_WD_init(WD_TOUT, MSS_WDOG_NO_WINDOW, MSS_WDOG_RESET_ON_TIMEOUT_MODE); //watchdog timeout: 10 seconds

    MSS_TIM1_init( MSS_TIMER_PERIODIC_MODE );
    MSS_TIM1_load_immediate( 100000000 );
    MSS_TIM1_start();
    MSS_TIM1_enable_irq();

    /**********************************************************************************************************
      ************************************* MAIN LOOP **********************************************************
      **********************************************************************************************************/

    for(;;) {

        if (timed_shutdown) {
            timed_shutdown = 0;
            dbg_print("\n\rStart of 5 seconds shutdown\n\r");
            rval = HW_get_32bit_reg(CPU2FABRIC_REG2);
            HW_set_32bit_reg(CPU2FABRIC_REG2, (rval | C2F_REG_SHUTDOWN ));
            sleep(5000);
            dbg_print("\n\rEnd of 5 seconds shutdown\n\r");
            HW_set_32bit_reg(CPU2FABRIC_REG2, (rval & (~C2F_REG_SHUTDOWN) ));
        }

        MSS_WD_reload();
        if ((tick_counter - uptime)> 60000) {
            uptime = tick_counter;
        }

        /* REINITIALIZE MSS_I2C DRIVER WHENEVER POWER IS SWITCHED OFF */
        /* get GPAC status */
        rval    = HW_get_32bit_reg(FABRIC2CPU_REG);
        new_spe = (rval & F2C_REG_SPE_I)?(1):(0);
        /* compare with previous status */
        if (old_spe && !new_spe) { /* GPAC was on, now it's off */
                /* reinitialize the I2C driver */
                sleep(100);
                MSS_I2C_init( &g_mss_i2c0, I2C_GPAC_S_ADDR, MSS_I2C_PCLK_DIV_256); //100M/256 = 390kHz (max 400)
                MSS_I2C_set_slave_tx_buffer( &g_mss_i2c0, (const uint8_t*) i2c_slave_tx_buf, I2C_SLAVE_TXBUF_SIZE );
                MSS_I2C_set_slave_rx_buffer( &g_mss_i2c0, i2c_slave_rx_buf, I2C_SLAVE_RXBUF_SIZE );
                MSS_I2C_set_slave_mem_offset_length( &g_mss_i2c0, 0 ); //changed: tx buf pointer handled by ISR
                MSS_I2C_register_write_handler( &g_mss_i2c0, i2c_slave_write_handler );
                MSS_I2C_clear_gca( &g_mss_i2c0 );
                MSS_I2C_enable_slave( &g_mss_i2c0 );
                dbg_print("GPAC was switched off: reinitializing I2C link\n\r");
        }
        old_spe = new_spe;

        /**************************************** ETHERNET/TFTP *******************************************/
        if (network_en) { //MAC address is valid, MAC has been initialized

            //check cable connection
            //            if ((eth_status & MSS_MAC_LINK_STATUS_LINK) && !(MSS_MAC_link_status() & MSS_MAC_LINK_STATUS_LINK)) {
            //            } else if (!(eth_status & MSS_MAC_LINK_STATUS_LINK) && (MSS_MAC_link_status() & MSS_MAC_LINK_STATUS_LINK)) {
            //            }

            /* ARP update */
            if(tick_counter - arp_timer > ARP_TMR_INTERVAL) {
                arp_timer = tick_counter;
                etharp_tmr();
            }
            /* process received packets */
            if(MSS_MAC_rx_pckt_size())
            {
                ethernetif_input(NULL);
            }

            /* Handle DHCP */
            if (network_mode == 'D') { //DHCP
                //rebind timer
                if (dhcp_ip_found) {
                    if (!dhcp_force_renew) {
                        dhcp_lease_timeout = (dhcp_lease_time/2);
                    } //else use set value

                    if (dhcp_lease_count >= dhcp_lease_timeout) {
                        //REBIND
                        dbg_print("MAIN::DHCP expired: renew\n\r");
                        dhcp_lease_count = 0;
                        get_ip(ipStr, network_mode);
                        eth_update_addresses();
                        /* display IP */
                        if (oled_en) {
                            oled_contrast = 15;
                            gfx_Contrast(oled_contrast); //display on
                            oled_update_time = tick_counter;
                            txt_MoveCursor(0x0003, 0x0000); //start of line 4
                            gfx_RectangleFilled(0, 24, 96, 24+8, 0);
                            if (ip_known) {
                                oled_putstr_small(0, 3*8, ipStr, 0xFFFF);
                            } else {
                                putstr((unsigned char*)"NO IP\n") ;
                            }
                        }
                    } else { //count time
                        if ((tick_counter - dhcp_lease_tick) >= 100) { //1 second elapsed
                            dhcp_lease_tick = tick_counter;
                            dhcp_lease_count++;
                        }
                    }
                } else if (MSS_MAC_link_status() & MSS_MAC_LINK_STATUS_LINK) { //no ip, cable connected: try and get a new IP
                    //test this
                    dbg_print("MAIN::DHCP retry\n\r");
                    dhcp_lease_count = 0;
                    get_ip(ipStr, network_mode);
                    eth_update_addresses();
                    /* display IP */
                    if (oled_en) {
                        oled_contrast = 15;
                        gfx_Contrast(oled_contrast); //display on
                        oled_update_time = tick_counter;
                        txt_MoveCursor(0x0003, 0x0000); //start of line 4
                        gfx_RectangleFilled(0, 24, 96, 24+8, 0);
                        if (ip_known) {
                            oled_putstr_small(0, 3*8, ipStr, 0xFFFF);
                        } else {
                            putstr((unsigned char*)"NO IP\n") ;
                        }
                    }
                }
            }
        } //network en


        /**************************************** OLED/FP BUTTONS *******************************************/

        /* turn-on OLED on button press, turn off oled if timeout has elapsed,  */
        if (oled_en) {
            if (joystick()){
                oled_contrast = 15;
                gfx_Contrast(oled_contrast); //display on
                oled_update_time = tick_counter;
            } else if ((tick_counter - oled_update_time) > OLED_TOUT && (oled_contrast != 0)) {
                oled_contrast = 0;
                gfx_Contrast(oled_contrast); //display off
            }
        }

        /**************************************** MAINTENANCE *******************************************/

        /* poll UART: any keypress shows menu with valid input characters */

        poll_uart();

    	/************************************ GPAC (MMC as I2C master) **********************************/

        /* flag set by timer ISR (called periodically) */

        if (update_regmap == 1 && i2c_s_status == 0x00) {
            MSS_TIM1_disable_irq();
            update_register_map();
            update_regmap = 0;
            MSS_TIM1_enable_irq();
        }

        /**************************************** UPDATE CONTROLLER's INPUTS *******************************************/
        //get current GPI

        tx_buf[0] = 0; //input register
        core_i2c_doread(&g_core_i2c_pm, I2C_GPI_SER_ADDR, tx_buf, 1, rx_buf, 2, (uint8_t*) "Get GPI"); //read I2C inputs
        cpu2fabric_new = ((rx_buf[1] << 8) | rx_buf[0]); //LSB first
        cpu2fabric_new &= 0x3FFF;
        //read previous values and merge
        //cpu2fabric_old = HW_get_32bit_reg(CPU2FABRIC_REG) & 0xFFFFC000; //keep only USE_V2 an SHUTDOWN bits
        cpu2fabric_old = cpu2fabric_new;
        //update fabric inputs
        HW_set_32bit_reg(CPU2FABRIC_REG, cpu2fabric_old);

        /**************************************** POWER SUPPLIES *******************************************/

        /* added: check power supply alarms */
        //check only if a NEW power supply alarm was set (resets are not taken into account)
        /* OLD NEW
         * 00  01 10 11 => update
         * 01  1-       => update
         * 10  -1       => update
         * 11           => never update
         */
        fabric2cpu_old = HW_get_32bit_reg(FABRIC2CPU_REG); //read FW outputs

        //_MYDEBUG power switching debug
        //power_fail_new  = (fabric2cpu_old >>22) & 0x3; //get power fail bits
        //power_fail_new |= power_fail_old; //add up

        //new logic: "power_fail_old" holds current failure status. "power_fail_new" is used for computation
        //read power good bits and reverse them
        power_fail_new = (~(fabric2cpu_old >>20)) & 0x3;
        power_fail_new |= power_fail_old; //previous failure status has priority
        //read power fail bits
        //power_fail_new = ((fabric2cpu_old >>22)) & 0x3;

        //mask value according to active power supply
        if (HW_get_32bit_reg(CPU2FABRIC_REG2) & C2F_REG_USE_V2) {
            //using supply 2: mask supply1 failures
            power_fail_new &= 0x2;
        } else {
            //using supply 1: mask supply2 failures
            power_fail_new &= 0x1;
        }

        //DEBUG INFO IN FLASH
        //0xE00000 = alarm start time
        //0xE10000 = alarm

        //if ((power_fail_new != power_fail_old) &&
        //if ((fabric2cpu_old & F2C_REG_FP_ON) && (tick_counter - start_time > 6000)) { //power switch must be on

        if ((fabric2cpu_old & (F2C_REG_REM_EN1|F2C_REG_REM_EN2)) && (tick_counter - start_time > 6000)) { //at least one supply is enabled
            if (power_fail_new) { //only if supply in use fails

                if ( power_fail_alarm == 0x00 ) { //first detection: start counting
                    power_fail_time = tick_counter;
                    power_fail_alarm = 0x01;
                    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t *) "Power failure pre-alarm\n\r");

                    if (pwr_switch_dbg) { //_MYDEBUG write DBG info to flash
                        //write updated buffer
                        //flash_addr = 0xE00000 + (N*S25FL256_SECTOR_SIZE);
                        FLASH_erase_sector(0xE00000);
                        FLASH_program(0xE00000, (uint8_t*)&power_fail_time, 4);
                    }

                } else if ( (tick_counter - power_fail_time) > 6000) { //60 second wait before power failure confirmation
                    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t *) "Power failure confirmed\n\r");

                    //update eeprom only if new failure value is different from the previous one
                    if ((power_fail_new | power_fail_old) != power_fail_old) {

                        power_fail_old |= power_fail_new;

                        /* FORCE SHUTDOWN */
                        rval = HW_get_32bit_reg(CPU2FABRIC_REG2); //RMF
                        HW_set_32bit_reg(CPU2FABRIC_REG2, (rval | C2F_REG_SHUTDOWN) );

                        /* Write to EEPROM */
                        power_fail_alarm = 0x00;
                        i2c_eeprom_write(&g_mss_i2c0, I2C_EEPROM_SER_ADDR, 0x10, &power_fail_old, 1, (const uint8_t *) "Write page1 to MMC I2C EEPROM: ");

                        /* Do power supply switch if needed */
                        rval = HW_get_32bit_reg(CPU2FABRIC_REG2);
                        if (rval & C2F_REG_USE_V2) {
                            //using supply 2: never switch to 1
                        } else {
                            //using supply 1: switch to 2 whenever supply 1 fails
                            if (power_fail_old & 0x01) {
                                //cpu2fabric_old |= C2F_REG_USE_V2;
                                //HW_set_32bit_reg(CPU2FABRIC_REG, cpu2fabric_old | C2F_REG_HSC5_EN);
                                HW_set_32bit_reg(CPU2FABRIC_REG2, rval | C2F_REG_USE_V2);
                                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rWARNING: switching from Power_Supply_1 to Power_Supply_2\n\r");
                                if (oled_en) {
                                    txt_MoveCursor(0x0005,0x0000);
                                    putstr((unsigned char*)"Supply 2\n");
                                }
                            } //else: no change
                        }

                        /* RELEASE FORCED SHUTDOWN */
                        rval = HW_get_32bit_reg(CPU2FABRIC_REG2);
                        HW_set_32bit_reg(CPU2FABRIC_REG2, rval&(~C2F_REG_SHUTDOWN));
                    } //write failure to eeprom
                } else { //else check timer
                    if (pwr_switch_dbg && (((tick_counter - start_time)%100) == 0)) { //_MYDEBUG write DBG info to flash
                        //write updated buffer
                        //flash_addr = 0xE00000 + (N*S25FL256_SECTOR_SIZE);
                        FLASH_erase_sector(0xE10000);
                        FLASH_program(0xE10000, (uint8_t*)&power_fail_time, 4);
                    }
                }
            } else { //no power fail
                power_fail_alarm = 0x00;
            }
        }


        /**************************************** UPDATE I2C GPOs *******************************************/

        //get current Fabric outputs
        fabric2cpu_old = HW_get_32bit_reg(FABRIC2CPU_REG);
        //write GPOs
        tx_buf[0] = 2; //output register
        tx_buf[1] = fabric2cpu_old        & 0xFF;
        tx_buf[2] = (fabric2cpu_old >> 8) & 0x3F;
        core_i2c_dowrite(&g_core_i2c_pm, I2C_GPO_SER_ADDR, tx_buf, 3, (uint8_t*) "Set GPO");

        //Execute Write to SDcard command
        if (i2c_s_sd_access == 1 && i2c_s_status == 0x00) {
            dbg_print("Writing SD card.\n\r    Address = 0x");
            memcpy(text_buf, "0x00000000\n\r\0", 13);
            rval = i2c_s_buf[0]<<24 | i2c_s_buf[1]<<16 | i2c_s_buf[2]<<8 | i2c_s_buf[3];
            dbg_printnum(rval, 8);
            dbg_print("    Data = ");
            for(rval=4; rval<20; rval++) dbg_printnum(i2c_s_buf[rval], 2);
            dbg_print("\n\r");

            sd_write(i2c_s_buf, i2c_s_buf+4, 16);
            i2c_s_sd_access = 0;
            MSS_I2C_enable_slave( &g_mss_i2c0 ); //re-enable slave
        }

        /* GPAC3 SD Card access */
        if (i2c_s_sd_access == 2) { //read

            sd_read(cmd_buf+CMD_BUF_ADR_OFF, flash_rbuf, 256);
            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, 7, oled_error);
            i2c_s_sd_access = 0;
            MSS_I2C_enable_slave( &g_mss_i2c0 ); //re-enable slave

        } else if (i2c_s_sd_access == 3) { //write

            sd_write(cmd_buf+CMD_BUF_ADR_OFF, flash_wbuf, 256);
            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, 8, oled_error);
            i2c_s_sd_access = 0;
            MSS_I2C_enable_slave( &g_mss_i2c0 ); //re-enable slave
        }

        //handle I2c slave timeout
    	if (i2c_s_status == 0x00) {
            i2c_s_tout  = 0;
    	} else if (i2c_s_tout == 0) { //i2c slave FSM running
    	    if (tick_counter - i2c_s_timer > 50) { //0.5 seconds timeout between interrupts (changed 5.19.3)
    	        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rI2C_SLAVE:ERROR: command timeout\n\r");
    	        i2c_s_tout = 1; //reset FSM
    	        i2c_s_status = 0; //Added 5.19.2
    	    }
    	}

    	//reset if a value has been set on upper byte of version register (PROGRAMMING COMMAND FROM I2C SLAVE)
    	if ((*(volatile uint32_t*)(MBU_MMC_V2B_APB_0)) & 0xFF000000) {
            sleep(1000);
    	    NVIC_SystemReset();
    	}

    }


    return 0;
}

/*****************************************************************************************************************************/

//#define DBG_MENU

void poll_uart(void) {
	size_t  rx_size;
	uint8_t uart_rx_buf[1];
	uint8_t txt[16], txt2[16];
	uint32_t i, j;
    uint8_t i2c_rw, *ptr8;
    //uint8_t i2c_id, i2c_addr, i2c_n, tx_buf[5], rx_buf[4];

    MSS_WD_reload();

	rx_size = MSS_UART_get_rx ( &g_mss_uart0, uart_rx_buf, sizeof(uart_rx_buf) ); //get char from console

	if( rx_size > 0 )
	{
		//MSS_TIM1_disable_irq(); /* temporarily disable periodical interrupt */
		switch(uart_rx_buf[0]) {

/* here are disabled debug menu items */
		case '+': //command mode
		    process_command();
		    //return;
		    break;
		case '0': //display REGMAP
		    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "REGISTER MAP. Last update time = "); //write to console
		    memcpy(txt, "0000 ms\n\r\0", sizeof("0000 ms\n\r\0"));
		    uint_to_decstr(update_time, txt, 4);
		    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt); //write to console

            memcpy(txt,  "00\0", 3);
            memcpy(txt2, "  0000:  \0", 10);
		    for(i=0; i<I2C_SLAVE_TXBUF_SIZE; i++) {
		        if((i%16) == 0) {
		            uint_to_hexstr(i, txt2+2, 4);
	                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt2);
		        }
		        uint_to_hexstr(i2c_slave_tx_buf[i], txt, 2);
		        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
                if ((i%4) == 3) {
                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "  ");
                }
                if ((i%16) == 15) {
                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
                }
		    }
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
		    break;
		case '1': //DEBUG access SPI flash
		    i = hex_from_console("SPI FLASH address to read: ",8);
		    memcpy(txt, "0x00000000\n\r\0", 13);
		    uint_to_hexstr(i, txt+2, 8);
		    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
		    FLASH_read(i, txt2, 16);
            memcpy(txt, " 00\0", 4);
		    for (i=0; i<16; i++) {
		        uint_to_hexstr(txt2[i], txt+1, 2);
		        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
		    }
		    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
		    break;
        case '2': //DEBUG compute crc
            i = hex_from_console("COMPUTE CRC: enter SPI file index: ",1);
            if (i<4) {
                j = compute_spi_crc(i);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "COMPUTED CRC = ");
                memcpy(txt, "0x00000000\n\r\0", 13);
                uint_to_hexstr(j, txt+2, 8);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                FLASH_read(0xF00000+i*0x10000+8, txt2, 4); //read expected CRC
                j = txt2[0]<<24 | txt2[1]<<16 | txt2[2]<<8 | txt2[3];
                uint_to_hexstr(j, txt+2, 8);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "EXPECTED CRC = ");
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
            } else {
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Bad index. Valid values: 0 to 3\n\r");
            }
            break;
        case '3': //display command buffer
            dbg_print("\n\rCommand buffer\n\r");
            for (i=0; i<CMD_BUF_LEN; i++) {
                if ((i%4) == 0) dbg_print("\n\r");
                dbg_printnum(cmd_buf[i],2); dbg_print(" ");
            }
            dbg_print("\n\r");
            break;
        case '4': //display flash buffers
            dbg_print("\n\rRead buffer");
            for (i=0; i<FLASH_BUF_LEN; i++) {
                if ((i%8) == 0) dbg_print("\n\r");
                dbg_printnum(flash_rbuf[i],2); dbg_print(" ");
            }
            dbg_print("\n\n\rWrite buffer");
            for (i=0; i<FLASH_BUF_LEN; i++) {
                if ((i%8) == 0) dbg_print("\n\r");
                dbg_printnum(flash_wbuf[i],2); dbg_print(" ");
            }
            dbg_print("\n\r");
            break;
#if 0
        case '3': //corrupt programming file (to test CRC checks)
            i = hex_from_console((uint8_t*)"CORRUPT SPI FLASH: enter SPI file index: ");
            if (i<4) {
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Erasing first sector of file.\n\r");
                FLASH_erase_sector(i*0x100000);
            } else {
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Error: file index not valid\n\r");
            }
            break;

        /*****************************************************************************/
        case '3': //read I2C_FAN debug info
            i = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0);
            dbg_print("MSS_I2C_CONTROLLER");
            dbg_print("\n\r    MAIN FSM    : "); dbg_printnum(i&0xFF, 2);
            dbg_print("\n\r    I2C FSM     : "); dbg_printnum((i>>8)&0xFF, 2);
            dbg_print("\n\r    APP FSM     : "); dbg_printnum((i>>16)&0xFF, 2);
            dbg_print("\n\r    I2C TS      : "); dbg_printnum((i>>24)&0x3, 1);
            dbg_print("\n\r    VERSION     : "); dbg_printnum((i>>28)&0xF, 1);
            i = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+4); //i2c_error counter
            dbg_print("\n\r    I2C ERR CNT : 0x"); dbg_printnum(i&0x3FFFFFFF, 8);
            dbg_print("\n\r    CMD FIFO F/E: "); dbg_printnum((i>>30)&1, 1); dbg_print("/"); dbg_printnum((i>>31)&1, 1);
            i = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+8); //FIFO reads
            dbg_print("\n\r    FIFO READS  : 0x"); dbg_printnum(i&0xFFFF, 8);
            dbg_print("\n\r    FAN RST CNT : 0x"); dbg_printnum((i>>16)&0xFFFF, 8);
            i = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+12); //i2c_error counter
            dbg_print("\n\r    LAST FIFO RD: 0x"); dbg_printnum(i, 8);
            dbg_print("\n\r");
            break;
        /*****************************************************************************/
        case '4': //
            dbg_print("Resetting MSS_I2C_CONTROLLER\n\r");
            HW_set_32bit_reg(MSS_I2C_CONTROLLER_0+MSS_I2C_CTRL_CMD, MSS_I2C_CTRL_RST);
            break;
            /*****************************************************************************/
        case '5': //access I2C
            dbg_print("i2c access\n\r");
            i2c_rw = hex_from_console("1=read, 2=write: ",1);
            if (i2c_rw < 1 && i2c_rw > 2) {
                dbg_print("    bad choice\n\r");
                break;
            }
            i2c_id =  hex_from_console("Select I2C: 1=PM, 2=TMP, 3=PWR: ",1);
            if (i2c_id < 1 && i2c_id > 3) {
                dbg_print("    bad choice\n\r");
                break;
            }
            i2c_addr  = hex_from_console("I2C address  (HEX): ",8);
            tx_buf[0] = hex_from_console("I2C register (HEX): ",2);
            i2c_n     = hex_from_console("Number of bytes   : ",1);
            if (i2c_n < 1 && i2c_id > 4) {
                dbg_print("    bad choice\n\r");
                break;
            }
            if (i2c_rw == 2) {
                i = hex_from_console("Data to write (HEX): ",8);
                tx_buf[1] = i&0xFF;
                tx_buf[2] = (i>>8)&0xFF;
                tx_buf[3] = (i>>16)&0xFF;
                tx_buf[4] = (i>>24)&0xFF;
            }
            switch (i2c_id) {
                case 1:
                    if (i2c_rw == 1) {
                        core_i2c_doread(&g_core_i2c_pm,  i2c_addr, tx_buf, 1, rx_buf, i2c_n, (uint8_t*) "Arbitrary read");
                    } else if (i2c_rw == 2) {
                        core_i2c_dowrite(&g_core_i2c_pm, i2c_addr, tx_buf, 1+i2c_n, (uint8_t*) "Arbitrary write");
                    }
                    break;
                case 2:
                    if (i2c_rw == 1) {
                        core_i2c_doread(&g_core_i2c_tmp, i2c_addr, tx_buf, 1, rx_buf, i2c_n, (uint8_t*) "Arbitrary read");
                    } else if (i2c_rw == 2) {
                        core_i2c_dowrite(&g_core_i2c_tmp, i2c_addr, tx_buf, 1+i2c_n, (uint8_t*) "Arbitrary write");
                    }
                    break;
                case 3:
                    if (i2c_rw == 1) {
                        core_i2c_doread(&g_core_i2c_pwr, i2c_addr, tx_buf, 1, rx_buf, i2c_n, (uint8_t*) "Arbitrary read");
                    } else if (i2c_rw == 2) {
                        core_i2c_dowrite(&g_core_i2c_pwr, i2c_addr, tx_buf, 1+i2c_n, (uint8_t*) "Arbitrary write");
                    }
                    break;
                default:
                    dbg_print("    bad choice\n\r");
            }

            if (i2c_rw == 1) {
                //display read value
                dbg_print("READ from i2c( ");
                dbg_printnum(i2c_id,1);
                dbg_print("), addr = 0x");
                dbg_printnum(i2c_addr,2);
                dbg_print(", reg = 0x");
                dbg_printnum(tx_buf[0],2);
                dbg_print(", ");
                dbg_printnum(i2c_n, 1);
                dbg_print(" bytes. DATA = ");
                for (i=0; i<i2c_n; i++) {
                    dbg_printnum(rx_buf[i2c_n-1-i],2);
                }
                dbg_print("\n\r");
            } else {
                //display read value
                dbg_print("WRITE to i2c( ");
                dbg_printnum(i2c_id,1);
                dbg_print("), addr = 0x");
                dbg_printnum(i2c_addr,2);
                dbg_print(", reg = 0x");
                dbg_printnum(tx_buf[0],2);
                dbg_print(", ");
                dbg_printnum(i2c_n, 1);
                dbg_print(" bytes. DATA = ");
                for (i=0; i<i2c_n; i++) {
                    dbg_printnum(tx_buf[i2c_n-i], 2);
                }
                dbg_print("\n\n\r");
            }
            break;
#endif
        case '6': //access APB
            dbg_print("APB access\n\r");
            i2c_rw = hex_from_console("1=read, 2=write: ",1);

            memcpy(txt, "0x00000000\n\r\0", 13);
            switch (i2c_rw) {
            case 1:
                i = hex_from_console("Address to read? > ",8);
                j = *(volatile uint32_t*)(i);
                uint_to_hexstr(j, txt+2, 8);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
                break;
            case 2:
                i = hex_from_console("Address to write? > ",8);
                j = hex_from_console("Data to write?    > ",8);
                *(volatile uint32_t*)(i) = j;
                break;
            default:
                dbg_print("    bad choice\n\r");
            }
            break;
        case '7':
            dbg_print("GPIO access\n\r");
            i2c_rw = hex_from_console("1=read, 2=write: ",1);
            if (i2c_rw < 1 && i2c_rw > 2) {
                dbg_print("    bad choice\n\r");
                break;
            }
            memcpy(txt, "0x00000000\n\r\0", 13);
            if (i2c_rw == 1) {
                j = MSS_GPIO_get_outputs();
                uint_to_hexstr(j, txt+2, 8);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
            } else if (i2c_rw == 2) {
                j = hex_from_console("Data to write?    > ",8);
                MSS_GPIO_set_outputs(j);
            }
            break;
        case '8': //reset fabric
            //Soft reset control register (read current value)
            i = *(volatile uint32_t*)(SOFT_RST_CR);

            dbg_print("  Asserting fabric reset.\n\r");
            i |= 0x20000; //assert reset
            *(volatile uint32_t*)(SOFT_RST_CR) = ( i ); //reset pad disabled, release driven resets
            break;
        case '9':
            //Soft reset control register (read current value)
            i = *(volatile uint32_t*)(SOFT_RST_CR);

            dbg_print("  Releasing fabric reset.\n\r");
            i &= 0xFFFDFFFF; //deassert reset
            *(volatile uint32_t*)(SOFT_RST_CR) = ( i ); //reset pad disabled, release driven resets
            break;
		/*********************************************************************************************/
		case 'R': //reset
            sleep(1000);
			NVIC_SystemReset();
			break;
		/*********************************************************************************************/
	    case 'A': //ALL tests
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r#\n\r# COMMAND A (Test All)\n\r#\n\r");

	        //execute tests
            display_version(sw_version, fw_version, fw_version_x);
	        test_status_display(0); MSS_WD_reload();
            test_elapsed_time_counter(0); MSS_WD_reload();
            test_i2c_eeproms(); MSS_WD_reload();
            test_spi_flash(); MSS_WD_reload();
            test_heater(0); MSS_WD_reload();
            test_realtime_clock(0); MSS_WD_reload();
            test_power_meters(0); MSS_WD_reload();
            test_energy_meter(0); MSS_WD_reload();
            test_spi_eeprom(); MSS_WD_reload();
            test_temperatures(0); MSS_WD_reload();
            test_fan_read(0); MSS_WD_reload();
	        break;
		/*********************************************************************************************/
		case 'C': //elapsed time counter
		    test_elapsed_time_counter(0);
			break;
	    /*********************************************************************************************/
        case 'D': //read all (fabric registers and I2C GPIO)
            test_status_display(0);
            break;
        case 'd': //read all (fabric registers and I2C GPIO)
            test_status_display(1);
            break;
        /*********************************************************************************************/
		case 'E': //i2c eeprom
		    i = HW_get_32bit_reg(FABRIC2CPU_REG);
		    if (i & F2C_REG_SPE_I) { //GPAC PRESENT and CRATE ON
		        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rERROR: GPAC is mastering I2C bus. Switch it off to access EEPROMS\n\r");
		    } else {
		        test_i2c_eeproms();
		    }
			break;
		/*********************************************************************************************/
		case 'F': //spi flash
		    test_spi_flash();
            break;
        /*********************************************************************************************/
        //case 'G': //FREE
        //    break;
        /*********************************************************************************************/
		case 'H': //heater
		    test_heater(0);
			break;
		/*********************************************************************************************/
		case 'I': //RESET MSS I2C
		    dbg_print("Initializing MSS_I2C_0\n\r");
		    MSS_I2C_init( &g_mss_i2c0, I2C_GPAC_S_ADDR, MSS_I2C_PCLK_DIV_256); //100M/256 = 390kHz (max 400)
		    /* Specify the transmit buffer containing the data that will be
		     * returned to the master during read and write-read transactions. */
		    MSS_I2C_set_slave_tx_buffer( &g_mss_i2c0, (const uint8_t*) i2c_slave_tx_buf, I2C_SLAVE_TXBUF_SIZE );
		    /* Specify the buffer used to store the data written by the I2C master. */
		    MSS_I2C_set_slave_rx_buffer( &g_mss_i2c0, i2c_slave_rx_buf, I2C_SLAVE_RXBUF_SIZE );

		    /* expect 2 bytes indicating the offset on i2c_slave_tx_buf (when external master reads from slave) */
		    MSS_I2C_set_slave_mem_offset_length( &g_mss_i2c0, 0 ); //changed: tx buf pointer handled by ISR

		    /* register handler for written data */
		    MSS_I2C_register_write_handler( &g_mss_i2c0, i2c_slave_write_handler );
		    /* Disable recognition of the General Call Address */
		    MSS_I2C_clear_gca( &g_mss_i2c0 );
		    /* Enable I2C slave. */
		    MSS_I2C_enable_slave( &g_mss_i2c0 );
		    dbg_print("Init done\n\n\r");
		    break;
//		case 'i': //display I2C SLAVE STATUS
//            print_i2c_drv_status(&g_mss_i2c0);
//		    break;
		/*********************************************************************************************/
		case 'J': //reset power supply alarm flags
		    if (HW_get_32bit_reg(FABRIC2CPU_REG) & F2C_REG_SPE_I) { //GPAC PRESENT and CRATE ON
		        dbg_print("ERROR: cannot access I2C bus when GPAC is powered.\n\r ");
		    } else {
		        dbg_print("Power supply failures cleared. Reset to make the change effective\n\r");
		        power_fail_new = power_fail_old = 0x0; //both power supplies good
		        i2c_eeprom_write(&g_mss_i2c0, I2C_EEPROM_SER_ADDR, 0x10, &power_fail_new, 1, (const uint8_t *) "Write page1 to MMC I2C EEPROM: ");
		    }
		    break;
		case 'K': //RealTimeClock (WARNING: I2C does not support repeated-start!!!)
		    test_realtime_clock(0);
			break;
		case 'L':
		    if (network_en) {
		        dbg_print("Forcing DHCP lease renew\n\r");
		        dhcp_lease_count = dhcp_lease_time;
		    }
		    break;
		case 'l':
		    if (network_en) {
		        if (dhcp_force_renew) {
                    dbg_print("Forced DHCP renew interval disabled\n\r");
		            dhcp_force_renew = 0;
		        } else {
		            dhcp_lease_timeout = hex_from_console("Set new DHCP renew time (seconds) = 0x", 8);
                    dhcp_force_renew = 1;
		            dbg_print("    set to ");
		            dbg_printnum_d(dhcp_lease_timeout,8);
		            dbg_print("\n\r");
		        }
		    }
		    break;
	    /*********************************************************************************************/
	    //case 'M': //FREE
	    //    break;
	    /*********************************************************************************************/
		case 'N': //Network chip
		    dbg_print("\n\rMAC = ");
            dbg_printnum(my_mac[0],2);
            dbg_printnum(my_mac[1],2);
            dbg_printnum(my_mac[2],2);
            dbg_printnum(my_mac[3],2);
            dbg_printnum(my_mac[4],2);
            dbg_printnum(my_mac[5],2); dbg_print("\n\r");
		    if (network_en) {
		        if (ip_known) {
		            dbg_print("TFTP listening on following IP (port 69)\n\r");
		            //show_ip(ipStr);
		            memcpy(txt, "000.000.000.000\0", 16);
		            uint_to_decstr(my_ip[0], txt, 3);
		            uint_to_decstr(my_ip[1], txt+4, 3);
		            uint_to_decstr(my_ip[2], txt+8, 3);
		            uint_to_decstr(my_ip[3], txt+12, 3);
		            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
		            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
		            if (network_mode == 'S') {
		                dbg_print("Static IP\n\r");
		            } else if (network_mode == 'D') {
		                dbg_print("Dynamic IP (Renew at = ");
		                dbg_printnum_d(dhcp_lease_timeout, 8);
                        dbg_print("s, Elapsed = ");
                        dbg_printnum_d(dhcp_lease_count, 8);
                        dbg_print(" s)\n\r");
		            } else {
		                dbg_print("Unknown net_mode 0x");
		                dbg_printnum(network_mode,2);
		                dbg_print("\n\r");
		            }
		        } else {
		            dbg_print("\n\rNO IP\n\r");
		        }
		    } else {
		        dbg_print("Network is disabled.\n\r");
		    }
			break;
		case 'n': //set network options and IP in EEPROM
            str_from_console((uint8_t *) "Enter network mode ('S'=Static, 'D'=Dynamic, others=disabled): ", txt);
            if (txt[0] == 'S') {
                i = hex_from_console("Enter IP address as single 32-bit hex: ", 8);
            } else {
                i = 0;
            }
            ptr8 = (uint8_t*) &i;
            txt[1] = ptr8[3];
            txt[2] = ptr8[2];
            txt[3] = ptr8[1];
            txt[4] = ptr8[0];
		    i2c_eeprom_write(&g_mss_i2c0, I2C_EEPROM_SER_ADDR, 0x50, txt, 5, (const uint8_t *) "Write page5 to MMC I2C EEPROM: "); //one-time
		    break;
        case 'O': //force OLED enable
            if (oled_en == 0) {
                oled_en = 1;
                dbg_print("OLED is now ENABLED\n\r");
            }
            break;
        case 'o': //force OLED disable
            if (oled_en == 1) {
                oled_en = 0;
                dbg_print("OLED is now DISABLED\n\r");
            }
            break;
		/*********************************************************************************************/
		case 'P': //power meters
		    test_power_meters(0);
			break;
        /*********************************************************************************************/
        case 'Q': //Energy meter on PM board
            test_energy_meter(0);
            break;
        /*********************************************************************************************/
		case 'S': //SPI EEPROM
		    test_spi_eeprom();
			break;
		/*********************************************************************************************/
		case 'T': //temperature sensors
		    test_temperatures(0);
		    break;

		/*********************************************************************************************/
		case 'U': //show uptime
		    dbg_print("Uptime: 0x");
		    dbg_printnum(tick_counter/6000, 8);
		    dbg_print(" minutes\n\r");
		    break;
	    /*********************************************************************************************/
        case 'V':
            display_version(sw_version, fw_version, fw_version_x);
            break;
        /*********************************************************************************************/
        case 'W': //_MYDEBUG: enable shutdown debug
            if (pwr_switch_dbg) {
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rDisabling power switching debug\n\r");
                pwr_switch_dbg = 0;
            } else {
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rEnabling power switching debug\n\r");
                pwr_switch_dbg = 1;
            }
            break;
        case 'w': //_MYDEBUG: read debug info from flash
            FLASH_read(0xE00000, (uint8_t*)&i, 4);
            FLASH_read(0xE10000, (uint8_t*)&j, 4);
            memcpy(txt, "0x00000000\n\r\0", 13);

            uint_to_hexstr(i, txt+2, 8);
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rLast power failure start timer: ");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

            uint_to_hexstr(j, txt+2, 8);
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rLast power failure end timer  : ");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

            uint_to_hexstr(j-i, txt+2, 8);
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rLast power failure diff timer : ");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
            break;
        /*********************************************************************************************/
        case 'X': //power meter and energy meter together
            test_power_monitoring();
            break;
        /*********************************************************************************************/
        case 'Y': //set fan controller's RPM values
            i = hex_from_console("Which fan? (0=FT, 1=FB, 2=B):", 1);
            if (i > 3) {
                dbg_print("ERROR: Bad index.\n\r");
            } else {
                j = hex_from_console("Enter RPM (hex, 16 bit):", 4);
                HW_set_32bit_reg(MSS_I2C_CONTROLLER_0+(4*(i+1)), j);
                dbg_print("    Set value to ");
                dbg_printnum(j, 4);
                dbg_print("\n\r");
            }
            break;
		case 'Z': //FAN controllers
		    test_fan_read(0);
		    break;
		case 'z':
            test_display(MSS_I2C_CONTROLLER_0+MSS_I2C_SET_RPM_FU_REG,   (uint8_t*)"RPM FRONT TOP SETTING   ", (uint8_t*)"rpm");
            test_display(MSS_I2C_CONTROLLER_0+MSS_I2C_SET_RPM_FB_REG,   (uint8_t*)"RPM FRONT BOTTOM SETTING", (uint8_t*)"rpm");
            test_display(MSS_I2C_CONTROLLER_0+MSS_I2C_SET_RPM_B_REG,   (uint8_t*)"RPM FRONT BACK SETTING  ", (uint8_t*)"rpm");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
            test_display(MSS_I2C_CONTROLLER_0+FC1_1_FSPD,   (uint8_t*)"FC1_RPM1 (FT)", (uint8_t*)"rpm");
            test_display(MSS_I2C_CONTROLLER_0+FC2_2_FSPD,   (uint8_t*)"FC2_RPM2 (FT)", (uint8_t*)"rpm");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "--\n\r");
            test_display(MSS_I2C_CONTROLLER_0+FC1_2_FSPD,   (uint8_t*)"FC1_RPM2 (FB)", (uint8_t*)"rpm");
            test_display(MSS_I2C_CONTROLLER_0+FC2_1_FSPD,   (uint8_t*)"FC2_RPM1 (FB)", (uint8_t*)"rpm");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "--\n\r");
            test_display(MSS_I2C_CONTROLLER_0+FC1_3_FSPD,   (uint8_t*)"FC1_RPM3 (B) ", (uint8_t*)"rpm");
            test_display(MSS_I2C_CONTROLLER_0+FC1_4_FSPD,   (uint8_t*)"FC1_RPM4 (B) ", (uint8_t*)"rpm");
            test_display(MSS_I2C_CONTROLLER_0+FC2_3_FSPD,   (uint8_t*)"FC2_RPM3 (B) ", (uint8_t*)"rpm");
            test_display(MSS_I2C_CONTROLLER_0+FC2_4_FSPD,   (uint8_t*)"FC2_RPM4 (B) ", (uint8_t*)"rpm");
            break;
		default:
			/* display options */
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r=== Available choices ===\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "0 = Print register map.\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "1 = Read from SPI FLASH.\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "2 = Check CRC on programming files.\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "3 = Display command buffer\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "4 = Display flash buffers\n\r");
            //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "3 = Corrupt a programming file (test CRC checks)\n\r");
            //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "3 = Read FAN I2C debug info\n\r");
            //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "4 = Reset FAN I2C\n\r");
            //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "5 = Core I2C arbitrary access\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "6 = APB arbitrary access\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "7 = GPIO arbitrary access\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "8 = Assert Fabric reset\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "9 = Release Fabric reset\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "A = EXECUTE ALL TESTS\n\r");
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "C = Test elapsed time counter\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "D = Display all IO values\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "d = Display all IO values (with help)\n\r");
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "E = Test I2C EEPROM\n\r");
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "F = Test SPI FLASH\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "H = Test PWR board heater\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I = Reset and reinitialize MSS I2C hard IP\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "i = Print I2C ISR status\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "J = Reset power supply failure flags on EEPROM\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "K = Test Real-time clock\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "L = Force DHCP RENEW\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "N = Ethernet status\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "n = Set Ethernet mode and IP\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "O = Force OLED enable\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "o = Force OLED disable\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "P = Test power meters\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Q = Test energy monitor\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "R = CPU reset\n\r");
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "S = Test SPI EEPROM\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "T = Display temperatures\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "U = Show system uptime\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "V = Display version\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "W = Enable/disable pwr-switching debug mode\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "w = Read pwr-switching debug info\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "X = Power measuring tests\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Y = Set FAN RPM\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Z = Test FAN controllers\n\r");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "z = Read FAN speeds\n\r");

            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\n\r>>> Monitoring >>>\n\r");
            //return;
		}

		//send EOF character
		uart_rx_buf[0] = 0x04;
		MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t *) uart_rx_buf, 1);
	}

    MSS_WD_reload();
	return;
}
/************************************************************************************/

void process_command(void) {

    uint8_t uart_rx_buf[4+512], bad_cmd = 0, wd;
    uint32_t rval;
    uint8_t eeprom_i2c_addr, i2c_buf[19], text_buf[20];


    MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t *) "+", 1); //echo

    if (uart_rx_timeout(uart_rx_buf, 1)) return;

    switch (uart_rx_buf[0]) {
        case 'X': //shutdown
            if (uart_rx_timeout(uart_rx_buf, 1)) return;
            //rval = HW_get_32bit_reg(CPU2FABRIC_REG);
            rval = HW_get_32bit_reg(CPU2FABRIC_REG2);
            if (uart_rx_buf[0] == '0') {
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r*** FORCED SHUTDOWN ENABLED ***\n\r");
                //HW_set_32bit_reg(CPU2FABRIC_REG, (rval|C2F_REG_SHUTDOWN));
                HW_set_32bit_reg(CPU2FABRIC_REG2, (rval|C2F_REG_SHUTDOWN));
                if (oled_en) {
                    txt_FGcolour(0xF800); //red rgb565
                    txt_MoveCursor(0x0006,0x0000);
                    putstr((unsigned char*)"SHUTDOWN\n");
                }
            } else {
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r*** FORCED SHUTDOWN DISABLED ***\n\r");
                //HW_set_32bit_reg(CPU2FABRIC_REG, (rval&(~C2F_REG_SHUTDOWN)));
                HW_set_32bit_reg(CPU2FABRIC_REG2, (rval&(~C2F_REG_SHUTDOWN)));
                if (oled_en) {
                    txt_MoveCursor(0x0006,0x0000);
                    putstr((unsigned char*)"        \n");
                }
            }
            bad_cmd = 0;
            break;
        case 'K': //clock
            if (uart_rx_timeout(uart_rx_buf, 1)) return;
            if (uart_rx_buf[0] == 'W') {
                //clock write
                //...01 3 5 7 9 B
                //+KWwDDMMYYhhmmss (example: +KW4010116120000, friday 1/1/2016 12:00:00)
                if (uart_rx_timeout(uart_rx_buf, 13)) return;

                /* set time and date */
                //                01234567 890123456 7
                memcpy(text_buf, "00:00:00\000/00/00\0", sizeof("00:00:00\000/00/00\0"));
                text_buf[ 0] = uart_rx_buf[ 7]; //h
                text_buf[ 1] = uart_rx_buf[ 8]; //h
                text_buf[ 3] = uart_rx_buf[ 9]; //m
                text_buf[ 4] = uart_rx_buf[10]; //m
                text_buf[ 6] = uart_rx_buf[11]; //s
                text_buf[ 7] = uart_rx_buf[12]; //s
                text_buf[ 9] = uart_rx_buf[1]; //D
                text_buf[10] = uart_rx_buf[2]; //D
                text_buf[12] = uart_rx_buf[3]; //M
                text_buf[13] = uart_rx_buf[4]; //M
                text_buf[15] = uart_rx_buf[5]; //Y
                text_buf[16] = uart_rx_buf[6]; //Y
                //wd= weekday, sunday=0, hour[]="00:00:00", date[]="00/00/00"
                pca2129_set_clock(&g_core_i2c_pm, I2C_RTC_SER_ADDR, (uart_rx_buf[0]-'0'), (char*)text_buf, (char*)text_buf+9);
                bad_cmd = 0;
            } else if (uart_rx_buf[0] == 'R') { //read
                //clock read +KR
                /* read date */
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rDATE: ");
                pca2129_get_date(&g_core_i2c_pm, I2C_RTC_SER_ADDR, 2);
                /* read time */
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rTIME: ");
                pca2129_get_time(&g_core_i2c_pm, I2C_RTC_SER_ADDR, 1);
                bad_cmd = 0;
            } else {
                bad_cmd = 1;
            }
            break;
        case 'E': //eeprom
            if (HW_get_32bit_reg(FABRIC2CPU_REG) & F2C_REG_SPE_I) { //GPAC PRESENT and CRATE ON
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "ERROR: cannot access I2C bus when GPAC is powered.\n\r ");
                return;
            }
            if (uart_rx_timeout(uart_rx_buf, 1)) return;
            if (uart_rx_buf[0] == 'W') {
                //eeprom write
                //+EWep<16xD> (e: select EEPROM, 0=MMC, 1=PWR; p: select page 0:7, D=1byte data)
                if (uart_rx_timeout(uart_rx_buf, 18)) return;

                if (uart_rx_buf[0] == '0') {
                    eeprom_i2c_addr = I2C_EEPROM_SER_ADDR;
                    bad_cmd = 0;
                } else if (uart_rx_buf[0] == '1') {
                    eeprom_i2c_addr = I2C_PWR_EE_SER_ADDR;
                    bad_cmd = 0;
                } else {
                    bad_cmd = 1;
                    break;
                }

                if ( (uart_rx_buf[1] < '0') || (uart_rx_buf[1] > '7') ) { //allowed pages 0:7
                    bad_cmd = 1;
                    break;
                } else {
                    i2c_eeprom_write(&g_mss_i2c0, eeprom_i2c_addr, 16*(uart_rx_buf[1]-'0'), uart_rx_buf+2, 16, (const uint8_t *) "Write data to I2C EEPROM: ");
                    bad_cmd = 0;
                }
            } else if (uart_rx_buf[0] == 'R') { //read
                //eeprom read
                //+ERep (e: select EEPROM, 0=MMC, 1=PWR; p: select page 0:8, page 8 reads EUI48)
                if (uart_rx_timeout(uart_rx_buf, 2)) return;

                if (uart_rx_buf[0] == '0') {
                    eeprom_i2c_addr = I2C_EEPROM_SER_ADDR;
                    bad_cmd = 0;
                } else if (uart_rx_buf[0] == '1') {
                    eeprom_i2c_addr = I2C_PWR_EE_SER_ADDR;
                    bad_cmd = 0;
                } else {
                    bad_cmd = 1;
                    break;
                }

                if ( (uart_rx_buf[1] < '0') || (uart_rx_buf[1] > '8') ) { //allowed pages 0:8
                    bad_cmd = 1;
                    break;
                } else if (uart_rx_buf[1] == '8') {
                    i2c_eeprom_read_eui48(&g_mss_i2c0, eeprom_i2c_addr, i2c_buf, (const uint8_t *) "\n\rRead EUI48 from I2C EEPROM: ");
                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
                    memcpy(text_buf, "00 \0", 4);
                    for(wd=0; wd<6; wd++) {
                        uint_to_hexstr(i2c_buf[wd], text_buf, 2);
                        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
                    }
                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
                    bad_cmd = 0;
                } else {
                    //READ PAGE
                    i2c_eeprom_read(&g_mss_i2c0, eeprom_i2c_addr, 16*(uart_rx_buf[1]-'0'), i2c_buf, 16, (const uint8_t *) "Readback from I2C EEPROM: "); //content of page 0

                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
                    memcpy(text_buf, "00 \0", 4);
                    for(wd=0; wd<16; wd++) {
                        uint_to_hexstr(i2c_buf[wd], text_buf, 2);
                        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
                    }
                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
                    bad_cmd = 0;
                }
            } else {
                bad_cmd = 1;
            }
            break;
        case 'F': //fan speed +F<R|W>SS (SSSS = RPM in HEX)
            if (uart_rx_timeout(uart_rx_buf, 1)) return;
            if (uart_rx_buf[0] == 'W') { //write
                bad_cmd = 0;
                //+HWnbb Set speed <bb> for fan <n>
                //n=(0,1,2)=(FrontTop,FrontBottom,Back)
                //bb= 2 bytes, MSB first
                if (uart_rx_timeout(&wd, 1)) return;

                if (wd < 3) {
                   if (uart_rx_timeout(uart_rx_buf, 2)) return;
                   rval = (uart_rx_buf[0]<<8) | (uart_rx_buf[1]);
                   HW_set_32bit_reg(MSS_I2C_CONTROLLER_0+(4*(wd+1)), rval);
                } else {
                   bad_cmd = 1;
                }

            } else if (uart_rx_buf[0] == 'R') { //read +HRn
                bad_cmd = 0;

                if (uart_rx_timeout(&wd, 1)) return;
                if (wd < 3) {
                    dbg_print("Fan RPM set value ");
                    dbg_printnum((uint32_t)wd, 1);
                    rval = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+(4*(wd+1)));
                    dbg_print(" = ");
                    dbg_printnum((uint32_t)rval, 4);
                    dbg_print("\n\r");
                } else {
                    bad_cmd = 1;
                }
            } else {
                bad_cmd = 1;
            }

            break;
        case 'H': //heater
            if (uart_rx_timeout(uart_rx_buf, 1)) return;
            if (uart_rx_buf[0] == 'W') { //write
                bad_cmd = 0;
                //+HWbb Set heater value (16 bit, only 12LSB valid)
                //bb= 2 bytes, MSB first
                if (uart_rx_timeout(uart_rx_buf, 2)) return;
                rval = (uart_rx_buf[0]<<8) | (uart_rx_buf[1]);
                AD5321_set_level(&g_core_i2c_tmp, I2C_TMP_DAC_SER_ADDR, rval);
            } else if (uart_rx_buf[0] == 'R') { //read
                bad_cmd = 0;
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "MBU_PWR-HEATER_DAC\n\r ");
                memcpy(text_buf, "0x000\n\r\0",8);
                rval = AD5321_get_level(&g_core_i2c_tmp, I2C_TMP_DAC_SER_ADDR);
                uint_to_hexstr(rval, text_buf+2, 3);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
            } else {
                bad_cmd = 1;
            }
            break;
        case 'O': //oled SDcard
            bad_cmd = 0;
            //her command mode (read/write)
            if (uart_rx_timeout(uart_rx_buf, 1)) return;

            if (uart_rx_buf[0] == 'W') { //write
                //+OWaaaa<512xD>
                //aaaa = 4byte address, MSB first, D = 1 byte data
                if (uart_rx_timeout(uart_rx_buf, 4+512)) return;

                //write 16B to selected address
                sd_write(uart_rx_buf, uart_rx_buf+4, 512);

            } else if (uart_rx_buf[0] == 'R') { //read
                //+ORaaaa
                //aaaa = 4byte address
                if (uart_rx_timeout(uart_rx_buf, 4)) return;

                //READ 16B DATA
                sd_read(uart_rx_buf, uart_rx_buf+4, 16);

                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
                memcpy(text_buf, "00 \0", 4);
                for(wd=4; wd<20; wd++) {
                    uint_to_hexstr(uart_rx_buf[wd], text_buf, 2);
                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) text_buf);
                }
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
            } else {
                bad_cmd = 1;
            }
            break;
        default:
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) " UNKNOWN COMMAND");
    }

    if (bad_cmd) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) " BAD ARGUMENTS");
    }
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
    return;
}
/************************************************************************************/

void config_gpios(void) {
	/* Initialize MSS GPIOs. */
	MSS_GPIO_init();
	/* config GPIOs */
	MSS_GPIO_config( MSS_GPIO_0  , MSS_GPIO_OUTPUT_MODE );
	//MSS_GPIO_config( MSS_GPIO_1  , MSS_GPIO_INPUT_MODE | MSS_GPIO_IRQ_EDGE_POSITIVE); //w5200_int
	return;
}

void config_spi() {
	/* init core 0 */
	MSS_SPI_init( &g_mss_spi0 );
	/* set options for PM energy meter cs5480 (slave0) */
	MSS_SPI_configure_master_mode(
			&g_mss_spi0,
			MSS_SPI_SLAVE_0,
			MSS_SPI_MODE3,
			MSS_SPI_PCLK_DIV_64, //100MHz/64 = 1.56 MHz (MAX ALLOWED is 2MHz)
			8); //1 byte frames
    MSS_SPI_set_slave_select( &g_mss_spi0, MSS_SPI_SLAVE_0 ); //permanently select SPI interface (only one slave)

	/* init core 1 */
	MSS_SPI_init( &g_mss_spi1 );
	/* set options for SPI FLASH (slave0) */
	MSS_SPI_configure_master_mode(
				&g_mss_spi1,
				MSS_SPI_SLAVE_0,
				MSS_SPI_MODE3,
				MSS_SPI_PCLK_DIV_2, //25MHz/2 = 12.5 MHz (MAX ALLOWED 50MHz)
				8); //1 byte frames
	/* set options for SPI EEPROM (slave1) */
	MSS_SPI_configure_master_mode(
				&g_mss_spi1,
				MSS_SPI_SLAVE_1,
				MSS_SPI_MODE0,
				MSS_SPI_PCLK_DIV_8, //25MHz/8 = 3.125 MHz (MAX=5MHz)
				8); //1 byte frames
	return;
}


/* Write handler for mss_i2c0 slave mode
 *
 * The instance parameter is a pointer to the mss_i2c_instance_t for which this
 * slave write handler has been declared.
 * The data parameter is a pointer to a buffer (received data buffer) holding
 * the data written to the MSS I2C slave.
 * The size parameter is the number of bytes held in the received data buffer.
 *
 * Handler functions must return one of the following values:
 *     MSS_I2C_REENABLE_SLAVE_RX
 *     MSS_I2C_PAUSE_SLAVE_RX.
 * If the handler function returns MSS_I2C_PAUSE_SLAVE_RX, the MSS I2C slave
 *  responds to subsequent write requests with a non-acknowledge bit (NACK),
 *  until the received data buffer content has been processed by some other part
 *  of the software application.
 *  A call to MSS_I2C_enable_slave() is required at some point after
 *  returning MSS_I2C_PAUSE_SLAVE_RX in order to release the received data
 *  buffer so it can be used to store data received by subsequent I2C write
 *  transactions.
 *
 * Defining the macro MSS_I2C_INCLUDE_SLA_IN_RX_PAYLOAD causes the driver to
 * insert the actual address used to access the slave as the first byte in the
 * buffer. This allows applications tailor their response based on the actual
 * address used to access the slave (primary address or GCA).
 */
#undef MSS_I2C_INCLUDE_SLA_IN_RX_PAYLOAD

#define USE_SECURE_COMMANDS //when defined, CMD_UNLOCK_KEY should be written to CMD_KEY register before every secure command (2,3,4,5,6,8,9,A,B)
#define CMD_UNLOCK_KEY 0x4F50454E //ASCII for 'OPEN'
#define MAX_CMD 10

#define buf8_to_16(x) ((x[0]<<8) | x[1])
#define buf8_to_32(x) ((x[0]<<24) | (x[1]<<16) | (x[2]<<8) | x[3])

mss_i2c_slave_handler_ret_t i2c_slave_write_handler( mss_i2c_instance_t *instance, uint8_t * data, uint16_t size )
{
    uint8_t txt[] = "0x00000000\n\r\0";
    uint8_t txt2[] = " 00\0";
    uint8_t flash_buf[16], i, lock;
    uint32_t reg32; //used for GPAC2 commands
    uint32_t flash_addr = 0, flash_info_addr, data_reg;
    uint16_t a16 = 0, d16 = 0;

    i2c_s_timer = tick_counter;
    i2c_s_access = 1;

    MSS_WD_reload();


    //GPAC2: 2-byte write transactions (either 1B command + 1B data, or 2B data)
    //GPAC3: 4-byte transaction, fixed format: 2B address + 2B data
    if (size == 4) {
        /*
         * Code for GPAC3 here
         */
        a16 = (data[0]<<8) | data[1];
        d16 = (data[2]<<8) | data[3];

        //Take action according to specified address
        if (a16 < I2C_SLAVE_TXBUF_SIZE) { //measurements buffer

            //just set the buffer for reading
            MSS_I2C_set_slave_tx_buffer( &g_mss_i2c0, (const uint8_t*) (i2c_slave_tx_buf+a16), (I2C_SLAVE_TXBUF_SIZE-a16) );

        } else if ( (a16 >= FLASH_WBUF_START) && (a16 < FLASH_RBUF_START) ) { //Write buffer

            a16 -= FLASH_WBUF_START; //remove offset from address
            MSS_I2C_set_slave_tx_buffer( &g_mss_i2c0, (const uint8_t*) (flash_rbuf+a16), (FLASH_BUF_LEN-a16) );
            //write both buffers
            flash_wbuf[a16]   = data[2];
            flash_rbuf[a16]   = data[2];
            flash_wbuf[a16+1] = data[3];
            flash_rbuf[a16+1] = data[3];

        } else if ( (a16 >= FLASH_RBUF_START) && (a16 < (FLASH_RBUF_START+FLASH_BUF_LEN)) ) { //Read buffer

            a16 -= FLASH_RBUF_START; //remove offset from address
            MSS_I2C_set_slave_tx_buffer( &g_mss_i2c0, (const uint8_t*) (flash_rbuf+a16), (FLASH_BUF_LEN-a16) );

        } else if ( (a16 >= CMD_WBUF_START) && (a16 < (CMD_WBUF_START+CMD_BUF_LEN)) ) { //Command buffer

            a16 -= CMD_WBUF_START; //remove offset from address
            MSS_I2C_set_slave_tx_buffer( &g_mss_i2c0, (const uint8_t*) cmd_buf+a16, CMD_BUF_LEN-a16 );

            //update command buffer
            cmd_buf[a16]   = data[2];
            cmd_buf[a16+1] = data[3];

            //get current address from command register
            flash_addr = buf8_to_32((cmd_buf+CMD_BUF_ADR_OFF));
            data_reg   = buf8_to_32((cmd_buf+CMD_BUF_DAT_OFF));

            //compute address of info section
            i2c_s_cmd = ( (flash_addr >> 20) & 0xF ); //sequential ID of 1MB sectors in FLASH memory
            flash_info_addr = 0x00F00000 + (i2c_s_cmd * S25FL256_SECTOR_SIZE); //space reserved in FLASH for information on the 1MB sectors 0 to 14

            //security
#ifdef USE_SECURE_COMMANDS
            if (buf8_to_32((cmd_buf+CMD_BUF_KEY_OFF)) == CMD_UNLOCK_KEY) {
                lock = 0;
            } else {
                lock = 1;
            }
#else
            lock = 0;
#endif

            if (a16 == 0) { //execute command

                switch (d16) {
                    case 0x0000: //do nothing (can be used to set the address in order to read the whole command buffer)
                        break;
                    case 0x0001: //read flash
                        //dbg_print("I2C_SLAVE:Read flash\n\r");
                        if (flash_addr > 0xFFFF00) {
                            dbg_print("    ERROR: Maximum allowed address is 0x00FFFF00\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 1);
                        } else {
                            FLASH_read(flash_addr, flash_rbuf, FLASH_BUF_LEN);
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 0);
                        }
                        break;
                    case 0x0002: //erase flash sector
                        //dbg_print("I2C_SLAVE:Erase flash sector\n\r");
                        if (lock) {
                            dbg_print("    ERROR: This command needs the unlock key\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 15);
                        } else if (flash_addr % S25FL256_SECTOR_SIZE) {
                            dbg_print("    ERROR: Address shall be aligned to sector size (0x10000)\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 1);
                        } else if (flash_addr > 0x00EFFFFF) {
                            dbg_print("    ERROR: Maximum allowed address is 0x00EF0000\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 2);
                        } else {

                            /* Erase info section
                             * Warning: each flash section (1MB) has a corresponding info section at address 0xF00000+(N*0x10000)
                             * Whenever a sector is erased in the section, the corresponding info section is erased too.
                             * AFTER a file is written, the info section shall be correctly initialized, otherwise the
                             * file will be treated as not valid.
                             * Info section currently uses three 32-bit fields (offset, name, description):
                             * 0x0: Status: 0xDDDDDDDD when file is valid
                             * 0x4: File length: used to compute CRC and to copy the file in external RAM during boot. MAX=0x100000 (1MB)
                             * 0x8: CRC: CRC32 calculated bytewise using file-length.
                             */
                            FLASH_read(flash_info_addr, flash_buf, 4);
                            data_reg = buf8_to_32(flash_buf);
                            if (data_reg != 0xFFFFFFFF) {
                                FLASH_erase_sector(flash_info_addr); //erase info section
                            }

                            //set status to "file not valid"  (0xFFFF) in register map
                            if (i2c_s_cmd < 4) {
                                i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xFF;
                                i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xFF;
                            }

                            //Perform actual sector erase
                            FLASH_erase_sector(flash_addr);
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 0);
                        }
                        break;
                    case 0x0003: //write flash
                        //dbg_print("I2C_SLAVE:Program flash\n\r");
                        if (lock) {
                            dbg_print("    ERROR: This command needs the unlock key\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 15);
                        } else if (flash_addr % FLASH_BUF_LEN) {
                            dbg_print("    ERROR: Address shall be aligned to flash buffer size (0x100)\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 1);
                        } else if (flash_addr > 0x00EFFF00) {
                            dbg_print("    ERROR: Maximum allowed address is 0x00EFFF00\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 2);
                        } else {
                            /* When writing to a file, make sure file info is cleared */
                            FLASH_read(flash_info_addr, flash_buf, 4);
                            data_reg = buf8_to_32(flash_buf);
                            if (data_reg != 0xFFFFFFFF) {
                                FLASH_erase_sector(flash_info_addr); //erase info section
                            }

                            FLASH_program(flash_addr, flash_wbuf, FLASH_BUF_LEN);
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 0);
                        }
                        break;
                    case 0x0004: //run IAP with FW0
                        dbg_print("I2C_SLAVE:PROGRAM FPGA: Image 0\n\r");
                        if (lock) {
                            dbg_print("    ERROR: This command needs the unlock key\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 15);
                        } else {
                            *(volatile uint32_t*)(MBU_MMC_V2B_APB_0) = 0x35000000; //'5'
                            MSS_TIM1_disable_irq();
                            return MSS_I2C_PAUSE_SLAVE_RX;
                        }
                        break;
                    case 0x0005: //run IAP with FW1
                        dbg_print("I2C_SLAVE:PROGRAM FPGA: Image 1\n\r");
                        if (lock) {
                            dbg_print("    ERROR: This command needs the unlock key\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 15);
                        } else {
                            *(volatile uint32_t*)(MBU_MMC_V2B_APB_0) = 0x36000000; //'6'
                            MSS_TIM1_disable_irq();
                            return MSS_I2C_PAUSE_SLAVE_RX;
                        }
                        break;
                    case 0x0006: //RESET
                        dbg_print("I2C_SLAVE:Reset CPU\n\r");
                        if (lock) {
                            dbg_print("    ERROR: This command needs the unlock key\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 15);
                        } else {
                            *(volatile uint32_t*)(MBU_MMC_V2B_APB_0) = 0xFF000000; //value not supported by bootloader: causes just a reboot
                            MSS_TIM1_disable_irq();
                            return MSS_I2C_PAUSE_SLAVE_RX;
                        }
                        break;
                    case 0x0007: //read SD card (256B blocks)
                        dbg_print("I2C_SLAVE:SD-Card read\n\r");
                        i2c_s_sd_access = 2; //SD access executed outside ISR (not working otherwise)
                        MSS_I2C_disable_slave( &g_mss_i2c0 ); //avoid buffer to be overwritten before
                        break;
                    case 0x0008: //write SD card (256B blocks, sector aligned)
                        dbg_print("I2C_SLAVE:SD-Card write\n\r");
                        if (lock) {
                            dbg_print("    ERROR: This command needs the unlock key\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 15);
                        } else if (flash_addr % 0x200) {
                            dbg_print("    ERROR: Address shall be aligned to SD sector size (0x200)\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 1);
                        } else {
                            i2c_s_sd_access = 3; //SD access executed outside ISR (not working otherwise)
                            MSS_I2C_disable_slave( &g_mss_i2c0 ); //avoid buffer to be overwritten before
                        }
                        break;
                    case 0x0009: //timed shutdown
                        dbg_print("I2C_SLAVE:Timed shutdown\n\r");
                        if (lock) {
                            dbg_print("    ERROR: This command needs the unlock key\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 15);
                        } else {
                            timed_shutdown = 1; //command executed outside ISR
                        }
                        break;
                    case 0x000A: //set file length
                        dbg_print("I2C_SLAVE:Write file length\n\r");
                        if (lock) {
                            dbg_print("    ERROR: This command needs the unlock key\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 15);
                        } else if (flash_addr % 0x100000) {
                            dbg_print("    ERROR: Address shall be aligned to file size (1MiB)\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 1);
                        } else if (flash_addr > 0x00E00000) {
                            dbg_print("    ERROR: Maximum allowed address is 0x00E00000\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 2);
                        } else {
                            if (data_reg > 0x100000) {
                                dbg_print("    ERROR: Maximum allowed size is 0x100000 (1MiB)\n\r");
                                write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 3);
                            } else {
                                dbg_print("    Written 0xCCCCCCCC ");
                                dbg_printnum(data_reg,8);
                                dbg_print(" to flash address 0x");
                                dbg_printnum(flash_info_addr,8);
                                dbg_print("\n\r");

                                flash_buf[0] = 0xCC;
                                flash_buf[1] = 0xCC;
                                flash_buf[2] = 0xCC;
                                flash_buf[3] = 0xCC;
                                flash_buf[4] = ((data_reg >> 24) & 0xFF);
                                flash_buf[5] = ((data_reg >> 16) & 0xFF);
                                flash_buf[6] = ((data_reg >>  8) & 0xFF);
                                flash_buf[7] = ((data_reg      ) & 0xFF);

                                FLASH_erase_sector(flash_info_addr);
                                FLASH_program(flash_info_addr, flash_buf, 8);
                                write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 0);
                            }
                        }
                        break;
                    case 0x000B: //compute CRC and write it to flash
                        dbg_print("I2C_SLAVE:Compute file CRC\n\r");
                        if (lock) {
                            dbg_print("    ERROR: This command needs the unlock key\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 15);
                        } else if (flash_addr % 0x100000) {
                            dbg_print("    ERROR: Address shall be aligned to file size (1MiB)\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 1);
                        } else if (flash_addr > 0x00E00000) {
                            dbg_print("    ERROR: Maximum allowed address is 0x00E00000\n\r");
                            write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 2);
                        } else {
                            //read current status
                            FLASH_read(flash_info_addr, flash_buf, 8);
                            data_reg = buf8_to_32((flash_buf+4)); //file size

                            if (data_reg > 0x100000) {
                                dbg_print("    ERROR: Maximum allowed size is 0x100000 (1MiB)\n\r");
                                write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 3);
                            } else {
                                //compute CRC, erase FLASH sector, update with new data
                                reg32 = compute_spi_crc(i2c_s_cmd);

                                dbg_print("    Written 0xDDDDDDDD ");
                                dbg_printnum(data_reg,8);
                                dbg_printnum(reg32,8);
                                dbg_print(" to flash address 0x");
                                dbg_printnum(flash_info_addr,8);
                                dbg_print("\n\r");

                                flash_buf[0] = 0xDD;
                                flash_buf[1] = 0xDD;
                                flash_buf[2] = 0xDD;
                                flash_buf[3] = 0xDD;
                                flash_buf[8]  = (reg32>>24) & 0xFF;
                                flash_buf[9]  = (reg32>>16) & 0xFF;
                                flash_buf[10] = (reg32>> 8) & 0xFF;
                                flash_buf[11] = (reg32    ) & 0xFF;

                                FLASH_erase_sector(flash_info_addr);
                                FLASH_program(flash_info_addr, flash_buf, 12);
                                write_result_reg(cmd_buf+CMD_BUF_RES_OFF, d16, 0);
                            }
                        }
                        break;

                    default:
                        dbg_print("I2C:ERROR: received unsupported command: 0x");
                        dbg_printnum(d16,4);
                        dbg_print("\n\r");
                }

                memset((cmd_buf+CMD_BUF_KEY_OFF), 0, 4); //reset key after performing any command
            }
        } else if ( (a16 >= CMD_RBUF_START) && (a16 < (CMD_RBUF_START+CMD_BUF_LEN)) ) {

            a16 -= CMD_RBUF_START; //remove offset from address
            MSS_I2C_set_slave_tx_buffer( &g_mss_i2c0, (const uint8_t*) cmd_buf+a16, CMD_BUF_LEN-a16 );

        } else {

            dbg_print("I2C_ERROR: attempted access to unsupported address 0x");
            dbg_printnum(a16,4);
            dbg_print("\n\r");

        }

        return MSS_I2C_REENABLE_SLAVE_RX;

    } else if (size != 2) {
        dbg_print("\n\rI2C_SLAVE:ERROR: bad transaction size (shall be 2 or 4 bytes)\n\r");
        i2c_s_status = 0;
        return MSS_I2C_REENABLE_SLAVE_RX;
    }

    //Start of command with code n is the following sequence: n,'C'; n,'M'; n,'D'
    //WARNING: write/read operations could in principle trigger commands. Anyway all commands start with 0x**43.
    //         Thus as long as ODD addresses are never used, no command will ever be triggered.
    switch (i2c_s_status) {
        case 0x00: //IDLE
            if ((data[1] == 'C') && (data[0] <= MAX_CMD)) { //command starts with the byte sequence "CMD" (0x43, 0x4D, 0x44)
                i2c_s_cmd = data[0];
                i2c_s_status++;
            } else {
                //receive the address for a read transaction
                a16 = (data[0]<<8) | data[1];

                //set pointer to TX buffer according to specified address
                if (a16 < I2C_SLAVE_TXBUF_SIZE) {
                    MSS_I2C_set_slave_tx_buffer( &g_mss_i2c0, (const uint8_t*) (i2c_slave_tx_buf+a16), (I2C_SLAVE_TXBUF_SIZE-a16) );
                } else {
                    a16 = 0;
                }
            }
            break;
        /********************************** RECEIVING START OF COMMAND ****************************************/
        case 0x01: //received 'C', expecting 'M'
            if ((data[1] == 'M') && (data[0] == i2c_s_cmd) ) { //command starts with the byte sequence "CMD" (0x43, 0x4D, 0x44)
                i2c_s_status++;
            } else {
                i2c_s_status = 0;
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:ERROR1: bad start of command. RX = ");
                for (i=0; i<size; i++) {
                    uint_to_hexstr(data[i], txt2+1, 2);
                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt2);
                }
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
                uint_to_hexstr(data[1], txt+6, 2); uint_to_hexstr(data[0], txt+8, 2);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
            }
            break;
        case 0x02: //received "CM", expecting 'D'
            if ((data[1] == 'D') && (data[0] == i2c_s_cmd) ) { //command starts with the byte sequence "CMD" (0x43, 0x4D, 0x44)
                if (i2c_s_cmd < 4) { //write eeprom
                    MSS_TIM1_disable_irq(); //stop periodic readouts
                    i2c_s_waddr = i2c_s_cmd*0x100000;
                    memcpy(txt, "0x00000000\n\r\0", 13);
                    uint_to_hexstr(i2c_s_waddr, txt+2, 8);
                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:WRITE_SPI_FLASH: address = ");
                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                    //address 0xF00000+k*0x10000 contains info on programming data stored at address k*0x100000
                    flash_addr = 0xF00000 + (i2c_s_cmd*S25FL256_SECTOR_SIZE);
                    //set status to "update"
                    for (i=0; i<4; i++) {
                        flash_buf[i] = 0xAA;
                    }
                    //erase old data. Write is made once at the end
                    FLASH_erase_sector(flash_addr); //_TODO is it needed?
                    //update register map
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xAA;
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xAA;
                    //go on
                    i2c_s_status++;
                } else {
                    switch (i2c_s_cmd){
                        case 4:
                            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:PROGRAM FPGA: Image 0\n\r");
                            *(volatile uint32_t*)(MBU_MMC_V2B_APB_0) = 0x35000000; //'5'
                            i2c_s_status = 0x00;
                            //return MSS_I2C_REENABLE_SLAVE_RX;
                            MSS_TIM1_disable_irq();
                            return MSS_I2C_PAUSE_SLAVE_RX;
                            break;
                        case 5:
                            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:PROGRAM FPGA: Image 1\n\r");
                            *(volatile uint32_t*)(MBU_MMC_V2B_APB_0) = 0x36000000; //'6'
                            i2c_s_status = 0x00;
                            //return MSS_I2C_REENABLE_SLAVE_RX;
                            MSS_TIM1_disable_irq();
                            return MSS_I2C_PAUSE_SLAVE_RX;
                            break;
                        case 6:
                            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:RESET\n\r");
                            *(volatile uint32_t*)(MBU_MMC_V2B_APB_0) = 0xFF000000; //value not supported by bootloader: causes just a reboot
                            i2c_s_status = 0x00;
                            //return MSS_I2C_REENABLE_SLAVE_RX;
                            MSS_TIM1_disable_irq();
                            return MSS_I2C_PAUSE_SLAVE_RX;
                            break;
                        case 7:
                            i2c_s_status++; //receive address
                            break;
                        case 8: //fan reset
                            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:FAN_RESET\n\r");
                            *(volatile uint32_t*)(MSS_I2C_CONTROLLER_0+MSS_I2C_CTRL_CMD) = MSS_I2C_CTRL_RST;
                            return MSS_I2C_REENABLE_SLAVE_RX;
                            break;
                        case 9: //timed shutdown
                            i2c_s_status = 0x00;
                            timed_shutdown = 1;
                            return MSS_I2C_REENABLE_SLAVE_RX;
                            break;
//                        case 10: //write test register
//                            i2c_s_status++;
//                            break;
                        default:
                            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:ERROR: command not valid. RX = ");
                            uint_to_hexstr(data[1], txt+6, 2); uint_to_hexstr(data[0], txt+8, 2);
                            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
                            i2c_s_status = 0x00;
                            return MSS_I2C_REENABLE_SLAVE_RX;
                    }
                }
            } else {
                i2c_s_status = 0;
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:ERROR: bad start of command. RX = ");
                uint_to_hexstr(data[1], txt+6, 2); uint_to_hexstr(data[0], txt+8, 2);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
            }
            break;
            /********************************** RECEIVING FILE LENGTH ****************************************/
        case 0x03: //get length/address (byte3)
            if (data[0] == i2c_s_cmd) {
                i2c_s_len = ((uint32_t)(data[1]))<<24;
                i2c_s_status++;
            } else {
                i2c_s_status = 0;
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:ERROR3: command interrupted. RX = ");
                uint_to_hexstr(data[1], txt+6, 2); uint_to_hexstr(data[0], txt+8, 2);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                if (i2c_s_cmd < 4) {
                    //set status to "failed"
                    for (i=0; i<4; i++) {
                        flash_buf[i] = 0xBB;
                    }
                    //write updated buffer
                    flash_addr = 0xF00000 + (i2c_s_cmd*S25FL256_SECTOR_SIZE);
                    FLASH_erase_sector(flash_addr); //added
                    FLASH_program(flash_addr, flash_buf, 4); //write only status
                    //update register map
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xBB;
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xBB;

                    MSS_TIM1_enable_irq(); //restart periodic readouts
                }
            }
            break;
        case 0x04: //get length/address (byte2)
            if (data[0] == i2c_s_cmd) {
                i2c_s_len |= ((uint32_t)(data[1]))<<16;
                i2c_s_status++;
            } else {
                i2c_s_status = 0;
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:ERROR4: command interrupted. RX = ");
                uint_to_hexstr(data[1], txt+6, 2); uint_to_hexstr(data[0], txt+8, 2);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                if (i2c_s_cmd < 4) {
                    //set status to "failed"
                    for (i=0; i<4; i++) {
                        flash_buf[i] = 0xBB;
                    }
                    //write updated buffer
                    flash_addr = 0xF00000 + (i2c_s_cmd*S25FL256_SECTOR_SIZE);
                    FLASH_erase_sector(flash_addr); // added
                    FLASH_program(flash_addr, flash_buf, 4); //write only status
                    //update register map
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xBB;
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xBB;

                    MSS_TIM1_enable_irq(); //restart periodic readouts
                }
            }
            break;
        case 0x05: //get length/address (byte1)
            if (data[0] == i2c_s_cmd) {
                i2c_s_len |= ((uint32_t)(data[1]))<<8;
                i2c_s_status++;
            } else {
                i2c_s_status = 0;
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:ERROR5: command interrupted. RX = ");
                uint_to_hexstr(data[1], txt+6, 2); uint_to_hexstr(data[0], txt+8, 2);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                if (i2c_s_cmd < 4) {
                    //set status to "failed"
                    for (i=0; i<4; i++) {
                        flash_buf[i] = 0xBB;
                    }
                    //write updated buffer
                    flash_addr = 0xF00000 + (i2c_s_cmd*S25FL256_SECTOR_SIZE);
                    FLASH_erase_sector(flash_addr); // added
                    FLASH_program(flash_addr, flash_buf, 4);
                    //update register map
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xBB;
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xBB;

                    MSS_TIM1_enable_irq(); //restart periodic readouts
                }
            }
            break;
        case 0x06: //get length/address (byte0)
            if (data[0] == i2c_s_cmd) {
                i2c_s_len |= ((uint32_t)(data[1]));

                if (i2c_s_cmd < 4 ) {
                    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Writing SPI FLASH. File size = ");
                    uint_to_hexstr(i2c_s_len, txt+2, 8);
                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Erasing flash sectors:\n\r");
                    for(reg32 = i2c_s_waddr; reg32<(i2c_s_waddr+i2c_s_len); reg32+=S25FL256_SECTOR_SIZE) {
                        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    ");
                        uint_to_hexstr(reg32, txt+2, 8);
                        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
                        FLASH_erase_sector(reg32);
                    }

                    //write file length to FLASH
                    flash_addr = 0xF00000 + (i2c_s_cmd*S25FL256_SECTOR_SIZE);
                    FLASH_erase_sector(flash_addr); // added
                    flash_buf[0] = (i2c_s_len>>24)&0xFF;
                    flash_buf[1] = (i2c_s_len>>16)&0xFF;
                    flash_buf[2] = (i2c_s_len>> 8)&0xFF;
                    flash_buf[3] = (i2c_s_len    )&0xFF;
                    FLASH_program(flash_addr+4, flash_buf, 4);

                    i2c_s_crc  = 0;
                    i2c_s_dcnt = 0;
                    i2c_s_cnt  = 0;
                } else if (i2c_s_cmd == 10) {
                    i2c_s_len = ~i2c_s_len; //invert value
                    *(i2c_slave_tx_buf+0x200) = ((i2c_s_len>>24)&0xFF);
                    *(i2c_slave_tx_buf+0x201) = ((i2c_s_len>>16)&0xFF);
                    *(i2c_slave_tx_buf+0x202) = ((i2c_s_len>>8 )&0xFF);
                    *(i2c_slave_tx_buf+0x203) = ((i2c_s_len    )&0xFF);
                    i2c_s_status = 0;
                    return MSS_I2C_REENABLE_SLAVE_RX;
                } else { //write SD card (cmd == 7)
                    i2c_s_cnt = 0;
                    //store address MSB first
                    reg32 = i2c_s_len;
                    i2c_s_buf[i2c_s_cnt++] = (i2c_s_len >>24)&0xFF;
                    i2c_s_buf[i2c_s_cnt++] = (i2c_s_len >>16)&0xFF;
                    i2c_s_buf[i2c_s_cnt++] = (i2c_s_len >> 8)&0xFF;
                    i2c_s_buf[i2c_s_cnt++] = (i2c_s_len     )&0xFF;
                    i2c_s_len = 16; //fixed len for SD card write
                }
                i2c_s_status++;

            } else {
                i2c_s_status = 0;
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:ERROR6: command interrupted. RX = ");
                uint_to_hexstr(data[1], txt+6, 2); uint_to_hexstr(data[0], txt+8, 2);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                if (i2c_s_cmd < 4) {
                    //set status to "failed"
                    for (i=0; i<4; i++) {
                        flash_buf[i] = 0xBB;
                    }
                    //write updated buffer
                    flash_addr = 0xF00000 + (i2c_s_cmd*S25FL256_SECTOR_SIZE);
                    FLASH_erase_sector(flash_addr); // added
                    FLASH_program(flash_addr, flash_buf, 4);
                    //update register map
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xBB;
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xBB;

                    MSS_TIM1_enable_irq(); //restart periodic readouts
                }
            }
            break;
        /********************************** WRITE FLASH ****************************************/
        case 0x07: //get actual data
            if ((data[0] == i2c_s_cmd) && (i2c_s_len > 0)) {
                i2c_s_len--; //remaining RX data
                i2c_s_dcnt++; //received data

                //Write to EEPROM or to SD card
                //store data in buffer
                i2c_s_buf[i2c_s_cnt++] = data[1];

                if (i2c_s_cmd < 4) {
                    i2c_s_crc = crc32_1byte(data+1, 1, i2c_s_crc);

                    /* write full buffer */
                    if (i2c_s_cnt == I2C_S_BUF_LEN) {
                        //buffer full
                        i2c_s_cnt = 0;
                        MSS_UART_polled_tx(&g_mss_uart0, (uint8_t*) '.', 1);
                        FLASH_program(i2c_s_waddr, (uint8_t*) i2c_s_buf, I2C_S_BUF_LEN);
                        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) ".");
                        i2c_s_waddr += I2C_S_BUF_LEN;
                    }
                    /* All data received (write last buffer if needed) */
                    if (i2c_s_len == 0) {
                        if (i2c_s_cnt > 0) { //partial buffer
                            MSS_UART_polled_tx(&g_mss_uart0, (uint8_t*) '.', 1);
                            FLASH_program(i2c_s_waddr, (uint8_t*) i2c_s_buf, i2c_s_cnt);
                        }
                        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\n\rSUCCESSFUL\n\n\r");

                        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rComputed CRC = ");
                        uint_to_hexstr(i2c_s_crc, txt+2, 8);
                        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                        //update register map
                        i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xCC;
                        i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xCC;

                        //check CRC
                        i2c_s_status++;
                    }
                } else { //write SD card
                    if(i2c_s_len == 0) {
                        //sd_write(i2c_s_buf, i2c_s_buf+4, 16);
                        i2c_s_sd_access = 1; //SD card is written during main loop (write from here does not work)
                        MSS_I2C_disable_slave( &g_mss_i2c0 ); //avoid buffer to be overwritten before
                        i2c_s_status = 0;
                    }
                }
            } else {
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:ERROR7: command interrupted. RX = ");
                uint_to_hexstr(data[1], txt+6, 2); uint_to_hexstr(data[0], txt+8, 2);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Current data counter = ");
                uint_to_hexstr(i2c_s_len, txt+2, 8);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                if (i2c_s_cmd < 4) {
                    //set status to "failed"
                    for (i=0; i<4; i++) {
                        flash_buf[i] = 0xBB;
                    }
                    //write updated buffer
                    flash_addr = 0xF00000 + (i2c_s_cmd*S25FL256_SECTOR_SIZE);
                    FLASH_program(flash_addr, flash_buf, 4);
                    //update register map
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xBB;
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xBB;

                    MSS_TIM1_enable_irq(); //restart periodic readouts
                }

                i2c_s_status = 0;
            }
            break;
        /********************************** RECEIVE CRC ****************************************/
        case 0x08: //receive CRC byte 3
            if (data[0] == i2c_s_cmd) {
                expected_crc = ((uint32_t)(data[1]))<<24;
                i2c_s_status++;
            } else {
                i2c_s_status = 0;
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:ERROR8: command interrupted.\n\r");
                //uint_to_hexstr(data[1], txt+6, 2); uint_to_hexstr(data[0], txt+8, 2);
                //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                //set status to "failed"
                for (i=0; i<4; i++) {
                    flash_buf[i] = 0xBB;
                }
                //write updated buffer
                flash_addr = 0xF00000 + (i2c_s_cmd*S25FL256_SECTOR_SIZE);
                FLASH_program(flash_addr, flash_buf, 4); //write only status
                //update register map
                i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xBB;
                i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xBB;

                MSS_TIM1_enable_irq(); //restart periodic readouts
            }
            break;
        case 0x09: //receive CRC byte 2
            if (data[0] == i2c_s_cmd) {
                expected_crc |= ((uint32_t)(data[1]))<<16;
                i2c_s_status++;
            } else {
                i2c_s_status = 0;
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:ERROR9: command interrupted.\n\r");
                //uint_to_hexstr(data[1], txt+6, 2); uint_to_hexstr(data[0], txt+8, 2);
                //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                //set status to "failed"
                for (i=0; i<4; i++) {
                    flash_buf[i] = 0xBB;
                }
                //write updated buffer
                flash_addr = 0xF00000 + (i2c_s_cmd*S25FL256_SECTOR_SIZE);
                FLASH_program(flash_addr, flash_buf, 4); //write only status
                //update register map
                i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xBB;
                i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xBB;

                MSS_TIM1_enable_irq(); //restart periodic readouts
            }
            break;
        case 0x0A: //receive CRC byte 1
            if (data[0] == i2c_s_cmd) {
                expected_crc |= ((uint32_t)(data[1]))<<8;
                i2c_s_status++;
            } else {
                i2c_s_status = 0;
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:ERROR10: command interrupted.\n\r");
                //uint_to_hexstr(data[1], txt+6, 2); uint_to_hexstr(data[0], txt+8, 2);
                //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                //set status to "failed"
                for (i=0; i<4; i++) {
                    flash_buf[i] = 0xBB;
                }
                //write updated buffer
                flash_addr = 0xF00000 + (i2c_s_cmd*S25FL256_SECTOR_SIZE);
                FLASH_program(flash_addr, flash_buf, 4); //write only status
                //update register map
                i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xBB;
                i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xBB;

                MSS_TIM1_enable_irq(); //restart periodic readouts
            }
            break;
        /********************************** CHECK CRC ****************************************/
        case 0x0B: //receive CRC byte 0 and check
            if (data[0] == i2c_s_cmd) {
                expected_crc |= ((uint32_t)(data[1]));

                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Expected CRC = ");
                uint_to_hexstr(expected_crc, txt+2, 8);
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                //check CRC
                if (expected_crc == i2c_s_crc) {
                    //set status to "crc check ok"
                    for (i=0; i<4; i++) {
                        flash_buf[i] = 0xDD;
                    }
                    //update register map
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xDD;
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xDD;

                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "CRC CHECK SUCCESSFUL\n\r");
                } else {
                    //set status to "crc check ok"
                    for (i=0; i<4; i++) {
                        flash_buf[i] = 0xEE;
                    }
                    //update register map
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xEE;
                    i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xEE;

                    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "CRC CHECK FAILED\n\r");
                }

                //write status and CRC
                flash_addr = 0xF00000 + (i2c_s_cmd*S25FL256_SECTOR_SIZE);
                FLASH_program(flash_addr, flash_buf, 4);
                //write expected crc to flash
                flash_buf[0] = (expected_crc>>24) & 0xFF;
                flash_buf[1] = (expected_crc>>16) & 0xFF;
                flash_buf[2] = (expected_crc>> 8) & 0xFF;
                flash_buf[3] = (expected_crc    ) & 0xFF;
                FLASH_program(flash_addr+8, flash_buf, 4);


            } else {
                i2c_s_status = 0;
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:ERROR11: command interrupted.\n\r");
                //uint_to_hexstr(data[1], txt+6, 2); uint_to_hexstr(data[0], txt+8, 2);
                //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

                //set status to "failed"
                for (i=0; i<4; i++) {
                    flash_buf[i] = 0xBB;
                }
                //write updated buffer
                flash_addr = 0xF00000 + (i2c_s_cmd*S25FL256_SECTOR_SIZE);
                FLASH_program(flash_addr, flash_buf, 4); //write only status
                //update register map
                i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd]   = 0xBB;
                i2c_slave_tx_buf[0x1F8+2*i2c_s_cmd+1] = 0xBB;
            }

            MSS_TIM1_enable_irq(); //restart periodic readouts
            i2c_s_status = 0;
            break;

        default:
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C_SLAVE:ERROR: unexpected condition. RX = ");
            uint_to_hexstr(data[1], txt+6, 2); uint_to_hexstr(data[0], txt+8, 2);
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
            MSS_TIM1_enable_irq(); //restart periodic readouts
            i2c_s_status = 0;
    }

    return MSS_I2C_REENABLE_SLAVE_RX;
}


void config_i2c(void) {

	uint8_t tx_buf[4];
	uint32_t reg;

	/* first initialize ON-CHIP I2C cores */

    MSS_I2C_init( &g_mss_i2c0, I2C_GPAC_S_ADDR, MSS_I2C_PCLK_DIV_960); //100M/256 = 390kHz (max 400)

    /* Specify the transmit buffer containing the data that will be
     * returned to the master during read and write-read transactions. */
    MSS_I2C_set_slave_tx_buffer( &g_mss_i2c0, (const uint8_t*) i2c_slave_tx_buf, I2C_SLAVE_TXBUF_SIZE );

    /* Specify the buffer used to store the data written by the I2C master. */
    MSS_I2C_set_slave_rx_buffer( &g_mss_i2c0, i2c_slave_rx_buf, I2C_SLAVE_RXBUF_SIZE );

    /* expect 2 bytes indicating the offset on i2c_slave_tx_buf (when external master reads from slave) */
    MSS_I2C_set_slave_mem_offset_length( &g_mss_i2c0, 0 ); //changed: tx buf pointer handled by ISR

    /* register handler for written data */
    MSS_I2C_register_write_handler( &g_mss_i2c0, i2c_slave_write_handler );
    /* Disable recognition of the General Call Address */
    MSS_I2C_clear_gca( &g_mss_i2c0 );
    /* Enable I2C slave. */
    MSS_I2C_enable_slave( &g_mss_i2c0 );

    /* ADDED 5.19.4 */
    HW_set_32bit_reg(0x40002014, 0x64); /* Set FREQ to 100 (MHz) */
    HW_set_32bit_reg(0x40002018, 0x06); /* Set GLITCHREG to 6 (Freq in (80,100] MHz range)  */

	/* then initialize OFF-CHIP I2C peripherals */
    I2C_init(&g_core_i2c_pm, COREI2C, 0xFF, I2C_PCLK_DIV_256); //25M/256 = 97kHz (max 100)
    I2C_channel_init( &g_core_i2c_tmp, &g_core_i2c_pm, I2C_CHANNEL_1, I2C_PCLK_DIV_256 );
    I2C_channel_init( &g_core_i2c_pwr, &g_core_i2c_pm, I2C_CHANNEL_2, I2C_PCLK_DIV_256 );

	reg = HW_get_32bit_reg(CORE_IRQ_BASE + IRQ_ENA_INT);
	reg |= (COREI2C_PM_IRQ | COREI2C_TMP_IRQ | COREI2C_PWR_IRQ);
    HW_set_32bit_reg(CORE_IRQ_BASE + IRQ_ENA_INT, reg); //enable interrupt for local I2C cores only

    //GPI/GPO init
    tx_buf[0] = 0x6; //direction
    tx_buf[1] = 0x00; tx_buf[2] = 0x00; //output
    if (core_i2c_dowrite(&g_core_i2c_pm, I2C_GPO_SER_ADDR, tx_buf, 3, (uint8_t*) "Set GPO direction")) {
        dbg_print("***ERROR*** cannot set GPO direction. Resetting...\n\r");
        sleep(5000);
        NVIC_SystemReset();
    }
    tx_buf[1] = 0xFF; tx_buf[2] = 0xFF; //input
    if (core_i2c_dowrite(&g_core_i2c_pm, I2C_GPI_SER_ADDR, tx_buf, 3, (uint8_t*)"Set GPI direction")) {
        dbg_print("***ERROR*** cannot set GPI direction Resetting...\n\r");
        sleep(5000);
        NVIC_SystemReset();
    }
    //set default value for GPO
    tx_buf[0] = 0x2; //output register
    tx_buf[1] = 0x04; tx_buf[2] = 0x12; //defaults
    if (core_i2c_dowrite(&g_core_i2c_pm, I2C_GPI_SER_ADDR, tx_buf, 3, (uint8_t*)"Set GPO initial value")) {
        dbg_print("***ERROR*** cannot set GPO initial value Resetting...\n\r");
        sleep(5000);
        NVIC_SystemReset();
    }

	/*************** TMP112 setup ***************/
	//Temp Sensor: eventually set specific options
	//defaults: continuous mode, out-of-range alarm, alarm active low, set fault on 1 event, 12-bit mode, Tmax=80, Tmin=75
  	tx_buf[0] = TMP112_CFG_REG;
  	tx_buf[1] = 0x00; //OS, R1, R0, F1, F0, POL, TM, SD
  	tx_buf[2] = 0x00; //CR1, CR0, AL, EM, 0000
  	mss_i2c_dowrite( &g_mss_i2c0, I2C_TMP_SER_ADDR, tx_buf, 3, (const uint8_t *) "Write to TMP112: " );
}

/* write on I2C slave, then wait for end of transaction */
i2c_status_t core_i2c_dowrite(
		i2c_instance_t * i2c_inst,
		uint8_t serial_addr,
		uint8_t * tx_buffer,
		uint8_t write_length,
		const uint8_t *msg ) {

    i2c_status_t status;

#ifdef DEBUG
    key2continue("Press key to start I2C write transaction\n\r");
#endif

    I2C_write(i2c_inst, serial_addr, tx_buffer, write_length, I2C_RELEASE_BUS);

    status = I2C_wait_complete(i2c_inst, I2C_TIMEOUT);

    if (status == I2C_FAILED) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "ERROR:I2C write transaction failed : ");
    	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
    	//key2continue("    *** I2C write transaction failed");
    }
    if (status == I2C_TIMED_OUT) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "ERROR:I2C write transaction timed-out : ");
    	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
        MSS_WD_reload(); //let the system self reset when too many timeouts in a row
    	//key2continue("    *** I2C write transaction timed-out");
    }

    return status;
}

/* read from I2C slave, then wait for end of transaction
 * (this is actually a write/read operation)
 */

i2c_status_t core_i2c_doread(
		i2c_instance_t * i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		uint8_t * rx_buffer, uint8_t read_length,
		const uint8_t *msg) {

	i2c_status_t status;

#ifdef DEBUG
    key2continue("Press key to start I2C read transaction\n\r");
#endif
    I2C_write_read(i2c_inst, serial_addr, tx_buffer, write_length, rx_buffer, read_length, I2C_RELEASE_BUS);
    status = I2C_wait_complete(i2c_inst, I2C_TIMEOUT);

//    I2C_write(i2c_inst, serial_addr, tx_buffer, write_length, I2C_RELEASE_BUS);
//    status = I2C_wait_complete(i2c_inst, I2C_TIMEOUT);
//    I2C_read(i2c_inst, serial_addr, rx_buffer, read_length, I2C_RELEASE_BUS);
//    status = I2C_wait_complete(i2c_inst, I2C_TIMEOUT);

    if (msg) {
        if (status == I2C_FAILED) {
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "ERROR:I2C read (W+R) transaction failed : ");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
            core_i2c_errcnt++;
        }
        if (status == I2C_TIMED_OUT) {
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "ERROR:I2C read (W+R) transaction timed-out : ");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
            core_i2c_errcnt++;
            MSS_WD_reload(); //let the system self reset when too many timeouts in a row
        }
    }

    return status;
}

///* read from I2C slave, then wait for end of transaction
// * (no repeated start. write operation followed by a read)
// */
//i2c_status_t core_i2c_doread_nors(
//        i2c_instance_t * i2c_inst, uint8_t serial_addr,
//        uint8_t * tx_buffer, uint8_t write_length,
//        uint8_t * rx_buffer, uint8_t read_length,
//        const uint8_t *msg) {
//
//    i2c_status_t status;
//
//#ifdef DEBUG
//    key2continue("Press key to start I2C read transaction\n\r");
//#endif
//
//    I2C_write(i2c_inst, serial_addr, tx_buffer, write_length, I2C_RELEASE_BUS);
//    status = I2C_wait_complete(i2c_inst, I2C_TIMEOUT);
//
//    if (status == I2C_FAILED) {
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C read (W) transaction failed\n\r");
//    }
//    if (status == I2C_TIMED_OUT) {
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C read (W) transaction timed-out\n\r");
//    }
//
//    I2C_read(i2c_inst, serial_addr, rx_buffer, read_length, I2C_RELEASE_BUS);
//    status = I2C_wait_complete(i2c_inst, I2C_TIMEOUT);
//
//    if (status == I2C_FAILED) {
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C read (R) transaction failed\n\r");
//    }
//    if (status == I2C_TIMED_OUT) {
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "I2C read (R) transaction timed-out\n\r");
//    }
//
//    return status;
//}

/* write on I2C slave, then wait for end of transaction */
mss_i2c_status_t mss_i2c_dowrite(
		mss_i2c_instance_t * i2c_inst,
		uint8_t serial_addr,
		uint8_t * tx_buffer,
		uint8_t write_length,
		const uint8_t *msg ) {

    mss_i2c_status_t status;

    MSS_I2C_write(i2c_inst, serial_addr, tx_buffer, write_length, MSS_I2C_RELEASE_BUS);

    status = MSS_I2C_wait_complete(i2c_inst, I2C_TIMEOUT);

    if (status == MSS_I2C_FAILED) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "ERROR:I2C write transaction failed : ");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
    }
    if (status == MSS_I2C_TIMED_OUT) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "ERROR: I2C write transaction timed-out : ");
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
    	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
        MSS_WD_reload(); //let the system self reset when too many timeouts in a row
    }

    return status;
}

/* read from I2C slave, then wait for end of transaction */
mss_i2c_status_t mss_i2c_doread(
		mss_i2c_instance_t * i2c_inst,
		uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		uint8_t * rx_buffer, uint8_t read_length,
		const uint8_t *msg) {

	mss_i2c_status_t status;

#ifdef DEBUG
    key2continue("Press key to start I2C read transaction\n\r");
#endif

    MSS_I2C_write_read(i2c_inst, serial_addr, tx_buffer, write_length, rx_buffer, read_length, I2C_RELEASE_BUS);

    status = MSS_I2C_wait_complete(i2c_inst, I2C_TIMEOUT);

    if (msg) {
        if (status == MSS_I2C_FAILED) {
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "ERROR:I2C read (W+R) transaction failed :");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
        }
        if (status == MSS_I2C_TIMED_OUT) {
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "ERROR:I2C read (W+R) transaction timed-out : ");
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
            MSS_WD_reload(); //let the system self reset when too many timeouts in a row
        }
    }

    return status;
}

/************************************************************************************************************/
uint32_t sleep(uint32_t milliseconds) {
    uint32_t now;

    now = tick_counter; //set up in 10ms units

    while ((tick_counter-now)<(milliseconds/10));

    return(0);
}

//write 16 bytes.
//ADDR_BUF shall contain 4bytes address (MSB first)
//WR_BUFF shall contain 16 bytes of data
void sd_write(uint8_t *addr_buf, uint8_t *wr_buf, uint16_t n) {
    uint16_t i;

    if (oled_en == 0) return;

    //set address
    media_SetAdd((uint16_t)((addr_buf[0]<<8) | addr_buf[1]), (uint16_t)((addr_buf[2]<<8) | addr_buf[3]));

    for (i=0; i<n; i++) {
        media_WriteByte(wr_buf[i]);
    }
    media_Flush();

    return;
}

//write 16 bytes.
//ADDR_BUF shall contain 4bytes address (MSB first)
//N byte data will be returned into rd_buf
void sd_read(uint8_t *addr_buf, uint8_t *rd_buf, uint16_t n) {
    uint16_t i;

    if (oled_en == 0) return;

    //set address
    media_SetAdd((uint16_t)((addr_buf[0]<<8) | addr_buf[1]), (uint16_t)((addr_buf[2]<<8) | addr_buf[3]));

    for (i=0; i<n; i++) {
        rd_buf[i] = media_ReadByte();
    }

    return;
}

//read maximum 1 sector, stop at termination char or first non-printable character
uint16_t sd_read_string(uint32_t sector, uint8_t *rd_buf, uint16_t max) {
    uint16_t i, lim;

    if (oled_en == 0) return 0;

    media_SetSector((uint16_t)((sector>>16)&0xFFFF), (uint16_t)(sector&0xFFFF));

    if (max<512) {
        lim = max;
    } else {
        lim = 512;
    }

    for (i=0; i<lim; i++) {
        rd_buf[i] = media_ReadByte();
        if (rd_buf[i] == 0) {
            break;
        } else if (rd_buf[i] <32) { //avoid control characters to be sent through UART
            rd_buf[i] = 32;
        }
    }

    return (i);
}

void update_register_map(void) {

    uint32_t now, reg32, i ,j;
    //uint8_t txt[] = "    00000000";
    uint8_t *ptr8;
    float f, f1;

    ptr8 = (uint8_t*) &reg32;

    now = tick_counter;

    /************************************************************************/

    /* 0x0000 FW_VERSION */
    i2c_slave_tx_buf[0] = '0';
    memcpy((_PTR)i2c_slave_tx_buf+0x1, fw_version, 3);
    /* 0x0004 SW_VERSION */
    memcpy((_PTR)i2c_slave_tx_buf+0x4, sw_version, 4);
    /* 0x0008 CPU to FABRIC: control bits and I2C GPIO inputs */
    reg32 = (HW_get_32bit_reg(CPU2FABRIC_REG2) & 0x3)<<14; //only SHUTDOWN and USE_SUPPLY2 bits
    reg32 |= (HW_get_32bit_reg(CPU2FABRIC_REG) & 0x3FFF);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x8+i] = ptr8[3-i];
    /* 0x000C FPGA input and output pins */
    reg32 = HW_get_32bit_reg(FABRIC2CPU_REG) & 0xFFFF0FFF;
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0xC+i] = ptr8[3-i];

    /* T112 temperatures (signed 12bit, 8b integer, 4b fractional) (12 bit mode: measure stored in (15:4)) */
    /* 0x0010 Front+Rear outlet temperature */
    reg32 = T112_get_temp(&g_core_i2c_tmp, 0, I2C_TMP_FO_SER_ADDR, 0, (uint8_t*)"Get FO temp");
    reg32 <<= 16;
    reg32 |= T112_get_temp(&g_core_i2c_tmp, 0, I2C_TMP_RO_SER_ADDR, 0, (uint8_t*)"Get RO temp");
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x10+i] = ptr8[3-i];
    /* 0x0014 Air inlet+Power board temperature */
    reg32 = T112_get_temp(&g_core_i2c_tmp, 0, I2C_TMP_IL_SER_ADDR, 0, (uint8_t*)"Get IL temp");
    reg32 <<= 16;
    reg32 |= T112_get_temp(&g_core_i2c_tmp, 0, I2C_TMP_PB_SER_ADDR, 0, (uint8_t*)"Get PB temp");
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x14+i] = ptr8[3-i];
    /* 0x0018 HEATSINK+Heater DAC */
    reg32 = TC74_get_temp(&g_core_i2c_tmp, I2C_TMP_HS_SER_ADDR);
    reg32 <<= 16;
    reg32 |= AD5321_get_level(&g_core_i2c_tmp, I2C_TMP_DAC_SER_ADDR);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x18+i] = ptr8[3-i];

    /* 0x001C IP ADDRESS */
    //reg32 = HW_get_32bit_reg(I2C_AUTO_0+I2C_AUTO_HEAT) & 0x00000FFF; //valid data (11:0)
    reg32 = (my_ip[0]<<24) | (my_ip[1]<<16) | (my_ip[2]<<8) | (my_ip[3]);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x1C+i] = ptr8[3-i];

    /* 0x0020 DATE in binary-coded-decimal format */
    reg32 = pca2129_get_date(&g_core_i2c_pm, I2C_RTC_SER_ADDR,0);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x20+i] = ptr8[3-i];
    /* 0x0024 TIME in binary-coded-decimal format */
    reg32 = pca2129_get_time(&g_core_i2c_pm, I2C_RTC_SER_ADDR, 0);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x24+i] = ptr8[3-i];

    /* VOLTAGE METERS */
    ptr8 = (uint8_t*) &f;
    /* ORING1 */
    /* 0x0028 POWER (Rshunt = 3 mohm) */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O1_SER_ADDR, LTC2945_PWR_MSB2, 3);
    f = LTC2945_raw_to_power(reg32, 3);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x28+i] = ptr8[3-i];

    /* 0x002C ISENSE (Rshunt = 3 mohm) */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O1_SER_ADDR, LTC2945_SENSE_MSB, 2);
    f = LTC2945_raw_to_current(reg32, 3);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x2C+i] = ptr8[3-i];
    /* 0x0030 VIN */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O1_SER_ADDR, LTC2945_VIN_MSB, 2);
    f = LTC2945_raw_to_voltage(reg32);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x30+i] = ptr8[3-i];
    /* 0x0034 ADIN : multiplier 11 */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O1_SER_ADDR, LTC2945_ADIN_MSB, 2);
    f = LTC2945_raw_to_adin(reg32, 11);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x34+i] = ptr8[3-i];

    /* ORING2 */
    /* 0x0038 POWER (Rshunt = 3 mohm) */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O2_SER_ADDR, LTC2945_PWR_MSB2, 3);
    f = LTC2945_raw_to_power(reg32, 3);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x38+i] = ptr8[3-i];
    /* 0x003C ISENSE (Rshunt = 3 mohm) */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O2_SER_ADDR, LTC2945_SENSE_MSB, 2);
    f = LTC2945_raw_to_current(reg32, 3);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x3C+i] = ptr8[3-i];
    /* 0x0040 VIN */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O2_SER_ADDR, LTC2945_VIN_MSB, 2);
    f = LTC2945_raw_to_voltage(reg32);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x40+i] = ptr8[3-i];
    /* 0x0044 ADIN : multiplier 11 */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_O2_SER_ADDR, LTC2945_ADIN_MSB, 2);
    f = LTC2945_raw_to_adin(reg32, 11);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x44+i] = ptr8[3-i];

    /* 3V3 SUPPLY */
    /* 0x0048 POWER (Rshunt = 3 mohm) */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_3V3_SER_ADDR, LTC2945_PWR_MSB2, 3);
    f = LTC2945_raw_to_power(reg32, 3);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x48+i] = ptr8[3-i];
    /* 0x004C ISENSE (Rshunt = 3 mohm) */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_3V3_SER_ADDR, LTC2945_SENSE_MSB, 2);
    f = LTC2945_raw_to_current(reg32, 3);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x4C+i] = ptr8[3-i];
    /* 0x0050 VIN */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_3V3_SER_ADDR, LTC2945_VIN_MSB, 2);
    f = LTC2945_raw_to_voltage(reg32);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x50+i] = ptr8[3-i];
    /* 0x0054 ADIN : multiplier 2 */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_3V3_SER_ADDR, LTC2945_ADIN_MSB, 2);
    f = LTC2945_raw_to_adin(reg32, 2);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x54+i] = ptr8[3-i];

    /* 5V0 SUPPLY */
    /* 0x0058 POWER (Rshunt = 1 mohm) */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_5V0_SER_ADDR, LTC2945_PWR_MSB2, 3);
    f = LTC2945_raw_to_power(reg32, 1);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x58+i] = ptr8[3-i];
    /* 0x005C ISENSE (Rshunt = 1 mohm) */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_5V0_SER_ADDR, LTC2945_SENSE_MSB, 2);
    f = LTC2945_raw_to_current(reg32, 1);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x5C+i] = ptr8[3-i];
    /* 0x0060 VIN */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_5V0_SER_ADDR, LTC2945_VIN_MSB, 2);
    f = LTC2945_raw_to_voltage(reg32);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x60+i] = ptr8[3-i];
    /* 0x0064 ADIN : multiplier (147.0/47.0) */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_5V0_SER_ADDR, LTC2945_ADIN_MSB, 2);
    f = LTC2945_raw_to_adin(reg32, (147.0/47.0));
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x64+i] = ptr8[3-i];

    /* P12V SUPPLY */
    /* 0x0068 POWER (Rshunt = 3 mohm) */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_P12_SER_ADDR, LTC2945_PWR_MSB2, 3);
    f = LTC2945_raw_to_power(reg32, 3);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x68+i] = ptr8[3-i];
    /* 0x006C ISENSE (Rshunt = 3 mohm) */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_P12_SER_ADDR, LTC2945_SENSE_MSB, 2);
    f = LTC2945_raw_to_current(reg32, 3);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x6C+i] = ptr8[3-i];
    /* 0x0070 VIN */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_P12_SER_ADDR, LTC2945_VIN_MSB, 2);
    f = LTC2945_raw_to_voltage(reg32);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x70+i] = ptr8[3-i];
    /* 0x0074 ADIN : multiplier 11 */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_P12_SER_ADDR, LTC2945_ADIN_MSB, 2);
    f = LTC2945_raw_to_adin(reg32, 11);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x74+i] = ptr8[3-i];

    /* N12V SUPPLY */
    /* 0x007C ISENSE (Rshunt = 3 mohm) */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_N12_SER_ADDR, LTC2945_SENSE_MSB, 2);
    f = LTC2945_raw_to_current(reg32, 10);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x7C+i] = ptr8[3-i];
    f1 = f; //store the value of the current
    /* 0x0084 ADIN : multiplier -11 */
    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_N12_SER_ADDR, LTC2945_ADIN_MSB, 2);
    f = LTC2945_raw_to_adin(reg32, -11);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x84+i] = ptr8[3-i];
    /* 0x0080 VIN */
    //CHANGE: use ADIN value
    //    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_N12_SER_ADDR, LTC2945_VIN_MSB, 2);
    //    f = LTC2945_raw_to_voltage(reg32);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x80+i] = ptr8[3-i];
    /* 0x0078 POWER (Rshunt = 10 mohm) */
    //    reg32 = LTC2945_read_reg(&g_core_i2c_pwr, I2C_PMON_N12_SER_ADDR, LTC2945_PWR_MSB2, 3);
    //    f = LTC2945_raw_to_power(reg32, 10);
    //compute power
    f *= -f1/1000.0;
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x78+i] = ptr8[3-i];


    /* PDM ENERGY METER */
    ptr8 = (uint8_t*) &f;
    cs5480_select_page(16);
    /* 0x0088 I1 RMS (A) */
    reg32 = cs5480_reg_read(CS5480_I1RMS);
    f = cs5480_meas_to_float(reg32, 1, 0);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x88+i] = ptr8[3-i];
    /* 0x008C V1 RMS (V) */
    reg32 = cs5480_reg_read(CS5480_V1RMS);
    f = cs5480_meas_to_float(reg32, 0, 0);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x8C+i] = ptr8[3-i];
    /* 0x0090 P1 AVG (W) */
    reg32 = cs5480_reg_read(CS5480_P1AVG);
    f = cs5480_meas_to_float(reg32, 3, 1);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x90+i] = ptr8[3-i];
    /* 0x0094 I2 RMS (A) */
    reg32 = cs5480_reg_read(CS5480_I2RMS);
    f = cs5480_meas_to_float(reg32, 2, 0);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x94+i] = ptr8[3-i];
    /* 0x0098 V2 RMS (V)*/
    reg32 = cs5480_reg_read(CS5480_V2RMS);
    f = cs5480_meas_to_float(reg32, 0, 0);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x98+i] = ptr8[3-i];
    /* 0x009C P2 AVG (W) */
    reg32 = cs5480_reg_read(CS5480_P2AVG);
    f = cs5480_meas_to_float(reg32, 4, 1);
    for (i= 0; i<4; i++) i2c_slave_tx_buf[0x9C+i] = ptr8[3-i];

    /* FAN CONTROLLER */
    ptr8 = (uint8_t*) &reg32;
    //Fan controllers are powered only when fp_on = 1 and at least one power supply is on
    reg32 = HW_get_32bit_reg(FABRIC2CPU_REG);
    if ( ((reg32 & F2C_REG_FP_ON) == 0) || ((reg32 & 0x00300000) == 0) ) {
        //MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)" warning: Front power switch is off. Cannot access fan controllers.\n\r");
        //MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*)" warning: Both power supplies are off. Cannot access fan controllers.\n\r");
        for (i=0xA0; i<0x1DF; i++) {
            i2c_slave_tx_buf[i] = 0x00;
        }
    } else {
        /* 0x00A0 Fan Controller 1 STATUS */
        reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC1_ST  );
        for (i=0; i<4; i++) i2c_slave_tx_buf[0xA0+i] = ptr8[3-i];
        /* 0x00A4 Fan Controller 1 MFR MODE */
        reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC1_MODE);
        for (i=0; i<4; i++) i2c_slave_tx_buf[0xA4+i] = ptr8[3-i];
        /* 0x00A8 Fan Controller 1 LIFETIME (days) */
        reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC1_TIME);
        for (i=0; i<4; i++) i2c_slave_tx_buf[0xA8+i] = ptr8[3-i];


        /* 0x00AC Fan Controller 1 TEMPERATURE (c°C) *///new: MSS_I2C_CTRL_OP(ADDR,N,REG,RNW)
        HW_set_32bit_reg(MSS_I2C_CONTROLLER_0+MSS_I2C_CTRL_CMD, MSS_I2C_CTRL_OP(0x52,1,0x0,0));  //write page command (write 1byte, value 0, register 0)
        HW_set_32bit_reg(MSS_I2C_CONTROLLER_0+MSS_I2C_CTRL_CMD, 0x0000000C);                     //write page data (0xC=internal temp)
        HW_set_32bit_reg(MSS_I2C_CONTROLLER_0+MSS_I2C_CTRL_CMD, MSS_I2C_CTRL_OP(0x52,2,0x8D,1)); //read temp command (read 2 bytes from register 0x8D)
        //sleep(500);
        reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+MSS_I2C_CTRL_DAT); //read register containing requested value (previous read)
        for (i=0; i<4; i++) i2c_slave_tx_buf[0xAC+i] = ptr8[3-i];

        /* 0x00B0-D0 Fan Controller 1, Fan1 Registers */
        for (j=0; j<9; j++) {
            reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC1_1_CFG12+(4*j));
            for (i=0; i<4; i++) i2c_slave_tx_buf[0xB0+(4*j)+i] = ptr8[3-i];
        }
        /* 0x00D4-F4 Fan Controller 1, Fan2 Registers */
        for (j=0; j<9; j++) {
            reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC1_2_CFG12+(4*j));
            for (i=0; i<4; i++) i2c_slave_tx_buf[0xD4+(4*j)+i] = ptr8[3-i];
        }
        /* 0x00F8-118 Fan Controller 1, Fan3 Registers */
        for (j=0; j<9; j++) {
            reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC1_3_CFG12+(4*j));
            for (i=0; i<4; i++) i2c_slave_tx_buf[0xF8+(4*j)+i] = ptr8[3-i];
        }
        /* 0x011C-13C Fan Controller 1, Fan3 Registers */
        for (j=0; j<9; j++) {
            reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC1_4_CFG12+(4*j));
            for (i=0; i<4; i++) i2c_slave_tx_buf[0x11C+(4*j)+i] = ptr8[3-i];
        }

        /* 0x0140 Fan Controller 2 STATUS */
        reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC2_ST  );
        for (i=0; i<4; i++) i2c_slave_tx_buf[0x140+i] = ptr8[3-i];
        /* 0x0144 Fan Controller 2 MFR MODE */
        reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC2_MODE);
        for (i=0; i<4; i++) i2c_slave_tx_buf[0x144+i] = ptr8[3-i];
        /* 0x0148 Fan Controller 2 LIFETIME (days) */
        reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC2_TIME);
        for (i=0; i<4; i++) i2c_slave_tx_buf[0x148+i] = ptr8[3-i];


        /* 0x014C Fan Controller 2 TEMPERATURE (c°C) */
        //new: MSS_I2C_CTRL_OP(ADDR,N,REG,RNW)
        HW_set_32bit_reg(MSS_I2C_CONTROLLER_0+MSS_I2C_CTRL_CMD, MSS_I2C_CTRL_OP(0x53,1,0x0,0));  //write page command (write 1byte, value 0, register 0)
        HW_set_32bit_reg(MSS_I2C_CONTROLLER_0+MSS_I2C_CTRL_CMD, 0x0000000C);                     //write page data (0xC=internal temp)
        HW_set_32bit_reg(MSS_I2C_CONTROLLER_0+MSS_I2C_CTRL_CMD, MSS_I2C_CTRL_OP(0x53,2,0x8D,1)); //read temp command (read 2 bytes from register 0x8D)
        //sleep(500);
        reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+MSS_I2C_CTRL_DAT); //read register containing requested value (previous read)
        for (i=0; i<4; i++) i2c_slave_tx_buf[0x14C+i] = ptr8[3-i];

        /* 0x0150-170 Fan Controller 2, Fan1 Registers */
        for (j=0; j<9; j++) {
            reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC2_1_CFG12+(4*j));
            for (i=0; i<4; i++) i2c_slave_tx_buf[0x150+(4*j)+i] = ptr8[3-i];
        }
        /* 0x0174-194 Fan Controller 2, Fan2 Registers */
        for (j=0; j<9; j++) {
            reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC2_2_CFG12+(4*j));
            for (i=0; i<4; i++) i2c_slave_tx_buf[0x174+(4*j)+i] = ptr8[3-i];
        }
        /* 0x0198-1B8 Fan Controller 2, Fan3 Registers */
        for (j=0; j<9; j++) {
            reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC2_3_CFG12+(4*j));
            for (i=0; i<4; i++) i2c_slave_tx_buf[0x198+(4*j)+i] = ptr8[3-i];
        }
        /* 0x01BC-1DC Fan Controller 2, Fan3 Registers */
        for (j=0; j<9; j++) {
            reg32 = HW_get_32bit_reg(MSS_I2C_CONTROLLER_0+FC2_4_CFG12+(4*j));
            for (i=0; i<4; i++) i2c_slave_tx_buf[0x1BC+(4*j)+i] = ptr8[3-i];
        }

    }

    /* 0x01E0-0x01E7 SDCARD content from offset 0x0 */
    ptr8 = (uint8_t*) &reg32;
    ptr8[0] = 0x00;
    ptr8[1] = 0x00;
    ptr8[2] = 0x00;
    ptr8[3] = 0x00;
    sd_read(ptr8, (uint8_t*)i2c_slave_tx_buf+0x1E0, 8);

    /* 0x01E8-0x01EF SDCARD content from offset 0x100 */
    ptr8[2] = 0x02;
    sd_read(ptr8, (uint8_t*)i2c_slave_tx_buf+0x1E8, 8);

    /* 0x01F0-0x01F7 SDCARD content from offset 0x200 */
    ptr8[2] = 0x04;
    sd_read(ptr8, (uint8_t*)i2c_slave_tx_buf+0x1F0, 8);

    /* 0x01F8- 0x1FF SPI FLASH update status (FW0,FW1,SW0,SW1) , from flash sector 0xF00000 */
    FLASH_read(0xF00000, (uint8_t*)i2c_slave_tx_buf+0x1F8, 2);
    FLASH_read(0xF10000, (uint8_t*)i2c_slave_tx_buf+0x1FA, 2);
    FLASH_read(0xF20000, (uint8_t*)i2c_slave_tx_buf+0x1FC, 2);
    FLASH_read(0xF30000, (uint8_t*)i2c_slave_tx_buf+0x1FE, 2);

    /************************************************************************/

    update_time = 10*(tick_counter-now+1);
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
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "File length bigger than 1MB. CRC won't be computed.");
        return 0;
    }

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Computing...");

    flash_addr = index*0x100000;

    while (file_len) { //data to read
        MSS_WD_reload();
        if (file_len >= 256) {
            FLASH_read(flash_addr, flash_buf, 256);
            crc = crc32_1byte(flash_buf, 256, crc);
//            for (i=0; i<256; i++) {
//                crc = crc32_1byte(flash_buf+i, 1, crc);
//            }
            flash_addr += 256;
            file_len   -= 256;
        } else {
            FLASH_read(flash_addr, flash_buf, file_len);
            crc = crc32_1byte(flash_buf, file_len, crc);
//            for (i=0; i<file_len; i++) {
//                crc = crc32_1byte(flash_buf+i, 1, crc);
//            }
            file_len   = 0;
        }
    }

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "DONE\n\r");
    return crc;
}

void write_result_reg(uint8_t *buf, uint16_t cmd, uint16_t data) {
    buf[0] = cmd>>8;
    buf[1] = cmd&0xFF;
    buf[2] = data>>8;
    buf[3] = data&0xFF;
}

#if 0 //_FIXME: temporary for debug
void print_i2c_drv_status(mss_i2c_instance_t * i2c) {
    dbg_print("\n\rMSS I2C STATUS");
    dbg_print("\n\rser_address     :"); dbg_printnum(i2c->ser_address, 2);
    dbg_print("\n\rtarget_addr     :"); dbg_printnum(i2c->target_addr, 2);
    dbg_print("\n\rtransaction     :"); dbg_printnum(i2c->transaction, 1);
    dbg_print("\n\rrandom_read_addr:"); dbg_printnum(i2c->random_read_addr, 4);
    dbg_print("\n\roptions         :"); dbg_printnum(i2c->options, 2);
    dbg_print("\n\rirqn            :"); dbg_printnum(i2c->irqn, 2);
    dbg_print("\n\rhw_reg*         :"); dbg_printnum((uint32_t)i2c->hw_reg, 8);
    dbg_print("\n\rhw_reg_bit*     :"); dbg_printnum((uint32_t)i2c->hw_reg_bit, 8);
    dbg_print("\n\rhw_smb_reg_bit* :"); dbg_printnum((uint32_t)i2c->hw_smb_reg_bit, 8);
    dbg_print("\n\rmaster_tx_buffer:"); dbg_printnum((uint32_t)i2c->master_tx_buffer, 8);
    dbg_print("\n\rmaster_tx_size  :"); dbg_printnum(i2c->master_tx_size, 2);
    dbg_print("\n\rmaster_tx_idx   :"); dbg_printnum(i2c->master_tx_idx, 2);
    dbg_print("\n\rdir             :"); dbg_printnum(i2c->dir, 2);
    dbg_print("\n\rmaster_rx_buffer:"); dbg_printnum((uint32_t)i2c->master_rx_buffer, 8);
    dbg_print("\n\rmaster_rx_size  :"); dbg_printnum(i2c->master_rx_size, 2);
    dbg_print("\n\rmaster_rx_idx   :"); dbg_printnum(i2c->master_rx_idx, 2);
    dbg_print("\n\rmaster_tout_ms  :"); dbg_printnum(i2c->master_timeout_ms, 8);
    dbg_print("\n\rslave_tx_buffer :"); dbg_printnum((uint32_t)i2c->slave_tx_buffer, 8);
    dbg_print("\n\rslave_tx_size   :"); dbg_printnum(i2c->slave_tx_size, 2);
    dbg_print("\n\rslave_tx_idx    :"); dbg_printnum(i2c->slave_tx_idx, 2);
    dbg_print("\n\rslave_rx_buffer :"); dbg_printnum((uint32_t)i2c->slave_rx_buffer, 8);
    dbg_print("\n\rslave_rx_size   :"); dbg_printnum(i2c->slave_rx_size, 2);
    dbg_print("\n\rslave_rx_idx    :"); dbg_printnum(i2c->slave_rx_idx, 2);
    dbg_print("\n\rslave_status    :"); dbg_printnum(i2c->slave_status, 1);
    dbg_print("\n\rslave_mem_ofslen:"); dbg_printnum(i2c->slave_mem_offset_length, 2);
    dbg_print("\n\rslave_wr_handler:"); dbg_printnum((uint32_t)i2c->slave_write_handler, 8);
    dbg_print("\n\ris_slave_enabled:"); dbg_printnum(i2c->is_slave_enabled, 2);
    dbg_print("\n\rp_user_data     :"); dbg_printnum((uint32_t)i2c->p_user_data, 8);
    dbg_print("\n\rbus_status      :"); dbg_printnum(i2c->bus_status, 2);
    dbg_print("\n\ris_tr_pending   :"); dbg_printnum(i2c->is_transaction_pending, 2);
    dbg_print("\n\rpending_transact:"); dbg_printnum(i2c->pending_transaction, 2);
    dbg_print("\n\n\r");
}
#endif
