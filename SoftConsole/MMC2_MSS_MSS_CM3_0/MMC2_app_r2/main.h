/*
 * main.h
 *
 *  Created on: 08.06.2015
 *      Author: malatesta_a
 */

#ifndef MAIN_H_
#define MAIN_H_

/********************************* INCLUDES *******************************/
#include "hal.h"
#include "MMC2_hw_platform.h"
#include "mss_uart.h"
#include "mss_gpio.h"
#include "mss_spi.h"
#include "mss_timer.h"
#include "mss_watchdog.h"
#include "mss_i2c.h"
//#include "mss_ethernet_mac.h"
#include "core_i2c.h"
#include "irq.h"

/*************************** SYSTEM CONSTANTS **************************/
#define CPU_FREQ    100000000
#define APB0_FREQ   CPU_FREQ
#define APB1_FREQ   CPU_FREQ/4
#define FABRIC_FREQ CPU_FREQ/2

/************************** Base addresses *****************************/
/*
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
#define SOFT_RST_CR        0xE0042030
#define SOFT_RST_CR_ENVM   0x00000001
#define SOFT_RST_CR_ESRAM0 0x00000002
#define SOFT_RST_CR_ESRAM1 0x00000004
#define SOFT_RST_CR_EMC    0x00000008
#define SOFT_RST_CR_MAC    0x00000010
#define SOFT_RST_CR_PDMA   0x00000020
#define SOFT_RST_CR_TIMER  0x00000040
#define SOFT_RST_CR_UART0  0x00000080
#define SOFT_RST_CR_UART1  0x00000100
#define SOFT_RST_CR_SPI0   0x00000200
#define SOFT_RST_CR_SPI1   0x00000400
#define SOFT_RST_CR_I2C0   0x00000800
#define SOFT_RST_CR_I2C1   0x00001000
#define SOFT_RST_CR_ACE    0x00002000
#define SOFT_RST_CR_GPIO   0x00004000
#define SOFT_RST_CR_IAP    0x00008000
#define SOFT_RST_CR_EXT    0x00010000
#define SOFT_RST_CR_FPGA   0x00020000
#define SOFT_RST_CR_F2M_EN 0x00040000
#define SOFT_RST_CR_PAD_EN 0x00080000

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

#define CORE_IRQ_BASE COREINTERRUPT_0

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

#define EXT_FLASH_BASE_ADDR  0x74000000
#define EXT_FLASH_SIZE       0x4000000 //64MB
#define EXT_FLASH_PAGE_SIZE  16 //bytes, 8 words
#define EXT_FLASH_SEC_SIZE   128 //bytes, 64 words
#define EXT_FLASH_SEC_NUM    512 //512x128kB = 64MB
#define EXT_FLASH_END_ADDR   (EXT_FLASH_BASE_ADDR+EXT_FLASH_SIZE-1)

#define EXT_ASRAM_BASE_ADDR  0x70000000
#define EXT_ASRAM_SIZE       0x800000 //8MB
#define EXT_ASRAM_END_ADDR   (EXT_ASRAM_BASE_ADDR+EXT_ASRAM_SIZE-1)

/******************** FAN CONTROLLERS ***************************/
#define FAN_RPM_FU_DEF 0x1388 //5000 RPM
#define FAN_RPM_FB_DEF 0x2328 //9000 RPM
#define FAN_RPM_B_DEF  0x0BB8 //3000 RPM

/* Fan-specific tables that associate PWM to RPM. Each table has 4 16-bit entries. Each entry is a RPM value:
 * T[0]=RPM for  40%PWM
 * T[1]=RPM for  60%PWM
 * T[2]=RPM for  80%PWM
 * T[3]=RPM for 100%PWM
 * Each entry is stored in the array as 2 separate bytes. Lowest index is always Least-significant-byte
 * Array[0:7] = T[0]LSB, T[0]MSB, T[1]LSB, T[1]MSB, T[2]LSB, T[2]MSB, T[3]LSB, T[3]MSB
 */
//Delta Electronics PFR0612XHE-SP00 (FRONT): no table available. Linear from 0 to 16500 RPM
static const uint8_t PWM2RPM_PFR0612XHE[8] = {0xC8, 0x19, 0xAC, 0x26, 0x90, 0x33, 0x74, 0x40};

//NMB 06038DA12S-EA-D0 (FRONT): no table available. Linear from 0 to 17000RPM
//static const uint8_t  PWM2RPM_NMB06038DA12S[8] = {0x90, 0x1A, 0xD8, 0x27, 0x20, 0x35, 0x68, 0x42};
//MEASURED
static const uint8_t  PWM2RPM_NMB06038DA12S[8] = {0x71, 0x1A, 0x05, 0x27, 0x79, 0x31, 0x20, 0x38};

//Delta Electronics AFB0612DH-TP11 (REAR): no table available. Linear from 0 to 9000 RPM
static const uint8_t  PWM2RPM_AFB0612DH[8] = {0x10, 0x0E, 0x18, 0x15, 0x20, 0x1C, 0x28, 0x23};

//ARX FD1260-AP581E2A2 (REAR): table available => 3000,4500,6000,6500 = 0x0BB8,0x0FA0,0x1770,0x1964
static const uint8_t  PWM2RPM_FD1260[8] = {0xB8, 0x0B, 0xA0, 0x0F, 0x70, 0x17, 0x64, 0x19};

//SUNON PSD1206PMBX-A (measured)
static const uint8_t PWM2RPM_PSD1206[8] = {0x29, 0x1A, 0x3F, 0x26, 0x0F, 0x36, 0xBE, 0x42};

/********************* I2C ***********************/
/* interrupt sources (project dependent) */
#define COREI2C_PM_IRQ  0x1
#define COREI2C_TMP_IRQ 0x2
#define COREI2C_PWR_IRQ 0x4

/* i2c addresses (remote slaves) */
/* PM I2C BUS */
#define I2C_GPO_SER_ADDR    0x20 //MMC Output I2C GPIO
#define I2C_GPI_SER_ADDR    0x21 //MMC Input I2C GPIO
#define I2C_RTC_SER_ADDR    0x51 //MMC real-time clock
#define I2C_ETC_SER_ADDR    0x6B //MMC elapsed time counter
/* HSC5 I2C BUS */
#define I2C_TMP_SER_ADDR    0x48 //MMC temp sensor
#define I2C_PWR_EE_SER_ADDR 0x50 //PWR eeprom
#define I2C_EEPROM_SER_ADDR 0x54 //MMC eeprom
#define I2C_GPAC_SER_ADDR   0xFF //GPAC I2C never used as slave
/* FAN I2C BUS */
#define I2C_FAN0_SER_ADDR   0x52 //FAN controller 1
#define I2C_FAN1_SER_ADDR   0x53 //FAN controller 2
/* TMP I2C BUS */
#define I2C_TMP_HS_SER_ADDR  0x4D //PWR heatsink sensor TC74
#define I2C_TMP_FO_SER_ADDR  0x48 //PWR front outlet temperature
#define I2C_TMP_RO_SER_ADDR  0x49 //PWR rear outlet temperature
#define I2C_TMP_PB_SER_ADDR  0x4A //PWR board temperature
#define I2C_TMP_IL_SER_ADDR  0x4B //PWR inlet temperature
#define I2C_TMP_DAC_SER_ADDR 0x0D //PWR Heater DAC
/* PWR I2C BUS */
#define I2C_PMON_O1_SER_ADDR  0x67 //PWR oring1 power monitor
#define I2C_PMON_O2_SER_ADDR  0x68 //PWR oring2 power monitor
#define I2C_PMON_3V3_SER_ADDR 0x6A //PWR 3.3V power monitor
#define I2C_PMON_5V0_SER_ADDR 0x69 //PWR 5.0V power monitor
#define I2C_PMON_P12_SER_ADDR 0x6B //PWR +12V power monitor
#define I2C_PMON_N12_SER_ADDR 0x6C //PWR -12V power monitor


/* addresses of local interfaces as slaves */
#define I2C_GPAC_S_ADDR 0x3Eu //test address. to be confirmed
#define I2C_PWR_M_ADDR   0xFFu //core I2C0 master address not used

/* I2C operation timeout value in mS. Define as I2C_NO_TIMEOUT to disable the timeout functionality */
#define I2C_TIMEOUT 3000u

#define BUFFER_SIZE    32u /* I2C buffer size */

mss_i2c_instance_t g_mss_i2c0;
mss_i2c_instance_t g_mss_i2c1;
i2c_instance_t g_core_i2c_pm;
i2c_instance_t g_core_i2c_tmp;
i2c_instance_t g_core_i2c_pwr;

/********************* slave devices ***************/


/*
 * I2C GPO output bit mapping
 * 12 VPP_ON (1=enable permanent Vpump)
 * 11 TMP_HT_PD_B
 * 10 SPE_B_OUT
 * 09 MASTER_RESET
 * 08 FAN_FAULT2
 * 07 FAN_FAULT1
 * 06 FAN_HW_RESET_U2
 * 05 FAN_HW_RESET_U1
 * 04 FP_REMOTE_ON_LED
 * 03 FP_HW_LED (green/red#)
 * 02 OLED_RST_N
 * 01 FP_FW_LED (green/red#)
 * 00 NOT USED
 */
#define I2C_GPO_MASK   0x1FFF //valid bits on GPO
#define I2C_GPO_VPP_ON 0x1000

/*
 * I2C GPI input bit mapping
 * 13 ETC_ALERT_B
 * 12 PM_ALERT_B
 * 11 FAN_ALERT_B
 * 10 TMP_ALERT_B
 * 09 TMP_HT2_ALERT_B
 * 08 TMP_HT1_ALERT_B
 * 07 PWR_P12V2_FT_B
 * 06 PWR_P12V1_FT_B
 * 05 PWR_N12V_ALERT_B
 * 04 PWR_P12V_ALERT_B
 * 03 PWR_P12V2_ALERT_B
 * 02 PWR_P12V1_ALERT_B
 * 01 PWR_P3V3_ALERT_B
 * 00 PWR_P5V0_ALERT_B
 */
#define I2C_GPI_MASK   0x3FFF //valid bits on GPI

/*********************************** APPLICATION RELATED ***********************************/

/* Periodical operations, repetition handled by timer interrupts */
#define POLLING_PERIOD_S 1

/* Software filter of alarms read from I2C GPIO
 * Alarms are all active low. To mask an alarm, the corresponding bit is forced to 1
 * Thus a OR-mask is used. 1=masked, 0=active */
#define ALARM_MASK 0x000000C0 //ORING alarms masked (temporary)

/* APB REGISTER BANK */
#define VER_REG  (MBU_MMC_V2B_APB_0 + 0x00000000) //read version
//#define CTRL_REG (MBU_MMC_V2B_APB_0 + 0x00000000) //write control
//#define CTRL_REG_V1PG_MASK 0x00000001 //v1 powergood mask
//#define CTRL_REG_V2PG_MASK 0x00000002 //v2 powergood mask
//#define CTRL_REG_FANP_MASK 0x00000004 //fan_present mask

#define CPU2FABRIC_REG    (MBU_MMC_V2B_APB_0 + 0x00000004) //Write/Readback (inputs to logic)
/* READ ONLY */
#define C2F_REG_P5V0_A    0x00000001 //pwr_p5v0_alert_b
#define C2F_REG_P3V3_A    0x00000002 //pwr_p3v3_alert_b
#define C2F_REG_P12V1_A   0x00000004 //pwr_p12v1_alert_b
#define C2F_REG_P12V2_A   0x00000008 //pwr_p12v2_alert_b
#define C2F_REG_P12V_A    0x00000010 //pwr_p12v_alert_b
#define C2F_REG_N12V_A    0x00000020 //pwr_n12v_alert_b
#define C2F_REG_P12V1_FT  0x00000040 //pwr_p12v1_ft_b
#define C2F_REG_P12V2_FT  0x00000080 //pwr_p12v2_ft_b
#define C2F_REG_TMP_TH1_A 0x00000100 //tmp_th1_alert_b
#define C2F_REG_TMP_TH2_A 0x00000200 //tmp_th2_alert_b
#define C2F_REG_TMP_A     0x00000400 //tmp_alert_b
#define C2F_REG_FAN_A     0x00000800 //fan_alert_b
#define C2F_REG_PM_A      0x00001000 //pm_alert_b
#define C2F_REG_ETC_A     0x00002000 //etc_alert_b

#define CPU2FABRIC_REG2 (MBU_MMC_V2B_APB_0 + 0x0000000C) //commands
/* READ/WRITE */
#define C2F_REG_USE_V2    0x00000001 //use power supply2
#define C2F_REG_SHUTDOWN  0x00000002 //force shutdown
#define C2F_REG_HSC5_EN   0x00000004 //enable hsc5_alert_b output pin normal operation (toggle when 0)
#define C2F_REG_PWR_CYC   0xFFFF0000 //when upper 16 bits are set to 0xABCD, then the shutdown at start is bypassed


#define FABRIC2CPU_REG     (MBU_MMC_V2B_APB_0 + 0x00000008) //read only (outputs from logic)
//#define F2C_REG_TMP_HT_PD  0x40000000 //(30) tmp_ht_pd_b
#define F2C_REG_SW_OFF2    0x20000000 //(29) v2_sw_off
#define F2C_REG_SW_OFF1    0x10000000 //(28) v1_sw_off
#define F2C_REG_REM_EN2    0x08000000 //(27) v2_remote_enable
#define F2C_REG_REM_EN1    0x04000000 //(26) v1_remote_enable
#define F2C_REG_HSC5_A     0x02000000 //(25) hsc5_alert

#define F2C_REG_PFAIL2     0x00800000 //(23) v2_power_failure (accumulated time)
#define F2C_REG_PFAIL1     0x00400000 //(22) v1_power_failure (accumulated time)
#define F2C_REG_PGOOD2     0x00200000 //(21) v2_power_good
#define F2C_REG_PGOOD1     0x00100000 //(20) v1_power_good
#define F2C_REG_WD         0x00080000 //(19) watchdog
#define F2C_REG_SPE_I      0x00040000 //(18) spe (in)
#define F2C_REG_FANP       0x00020000 //(17) fan_present_b
#define F2C_REG_FP_ON      0x00010000 //(16) fp_on

#define F2C_REG_VPUMP      0x00001000 //(12) VPP
#define F2C_REG_TMP_HT_PD  0x00000800 //(11) tmp_ht_pd_b
#define F2C_REG_SPE_O      0x00000400 //(10) spe_b (out)
#define F2C_REG_MRESET     0x00000200 //(9)  master_reset
#define F2C_REG_FAN_FAULT2 0x00000100 //(8)  fan_fault2_b
#define F2C_REG_FAN_FAULT1 0x00000080 //(7)  fan_fault1_b
#define F2C_REG_FAN_HRST2  0x00000040 //(6)  fan_hw_reset_u2_b
#define F2C_REG_FAN_HRST1  0x00000020 //(5)  fan_hw_reset_u1_b
#define F2C_REG_REM_ON     0x00000010 //(4)  fp_remote_on_led
#define F2C_REG_FP_HW      0x00000008 //(3)  fp_LED_hw
#define F2C_REG_OLED_RSTN  0x00000004 //(2)  oled_rst_n
#define F2C_REG_FP_FW      0x00000002 //(1)  fp_LED_fw
//#define F2C_REG_FP_FW    0x00000001 //(0)  unconnected


#define welcomeLogo ""

/* function defines */
void poll_uart(void);
void process_command(void);
//void key2continue(const char *message);
//void uint32_to_hexstr(uint32_t num, uint8_t *str);
//void uint_to_decstr(uint32_t num, uint8_t *str, uint8_t ndigits);
//void uint_to_hexstr(uint32_t num, uint8_t *str, uint8_t ndigits);
//uint8_t float_to_string(float num, uint8_t *str, uint8_t nd_int, uint8_t nd_frac);
//uint32_t hex_from_console(uint8_t* msg);
void config_gpios(void);
void config_i2c(void);
void config_spi(void);

i2c_status_t core_i2c_dowrite(
		i2c_instance_t *i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		const uint8_t *msg);
i2c_status_t core_i2c_doread(
		i2c_instance_t * i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		uint8_t * rx_buffer, uint8_t read_length,
		const uint8_t *msg);
i2c_status_t core_i2c_doread_nors(
        i2c_instance_t * i2c_inst, uint8_t serial_addr,
        uint8_t * tx_buffer, uint8_t write_length,
        uint8_t * rx_buffer, uint8_t read_length,
        const uint8_t *msg);

mss_i2c_status_t mss_i2c_dowrite(
		mss_i2c_instance_t *i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		const uint8_t *msg);

mss_i2c_status_t mss_i2c_doread(
		mss_i2c_instance_t * i2c_inst, uint8_t serial_addr,
		uint8_t * tx_buffer, uint8_t write_length,
		uint8_t * rx_buffer, uint8_t read_length,
		const uint8_t *msg);

mss_i2c_slave_handler_ret_t i2c_slave_write_handler( mss_i2c_instance_t *instance, uint8_t * data, uint16_t size );

uint16_t TEST_fan_get_status(uint8_t fc_i2c_addr);
void TEST_fan_setup(void);
void TEST_fan_setup_monitors(void);
void TEST_fan_profile(uint8_t fc_i2c_addr);
void TEST_fan_get_temp_volt(void);
void TEST_fan_set_speed(uint8_t fc_i2c_addr, uint16_t min, uint16_t max, uint16_t step);
void TEST_fan_speed_stability(uint16_t speed, uint8_t nreads);


void sd_write(uint8_t *addr_buf, uint8_t *wr_buf, uint16_t n);
void sd_read(uint8_t *addr_buf, uint8_t *rd_buf, uint16_t n);
uint16_t sd_read_string(uint32_t sector, uint8_t *rd_buf, uint16_t max);

uint32_t sleep(uint32_t milliseconds);

void update_register_map(void);

#endif /* MAIN_H_ */
