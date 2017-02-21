/*
 * cs5480.h
 *
 *  Created on: 16.07.2015
 *      Author: malatesta_a
 */

#ifndef CS5480_H_
#define CS5480_H_

#include "hal.h"

#define CS5480_USE_HPFILTER
//#define CS5480_DO_CALIBRATION

/* CS5480 energy meter */
struct cs5480struct {
	uint32_t config0:24;
	uint32_t config1:24;
	uint32_t config2:24;

	uint32_t status0:24;
	uint32_t status1:24;
	uint32_t status2:24;

	uint32_t tsettle:24;
};
typedef struct cs5480struct cs5480_t;


#define CS5480_MCLK 4096 //KHz
#define CS5480_OWR (CS5480_MCLK / 1024)
/* Memory has 64 pages of 64 24-bit locations each (Address = Page(6) & Reg_addr(6)) */
/* two MSB define the type of access */
#define CS5480_REG_RD   0x00 // 6LSB = Address
#define CS5480_REG_WR   0x40 // 6LSB = Address
#define CS5480_PAGE_SEL 0x80 // 6LSB = Page
#define CS5480_INSTR    0xC0 // 6LSB = Instruction
/* control instructions */
#define CS5480_I_SW_RST 0x01
#define CS5480_I_STDBY  0x02
#define CS5480_I_WAKEUP 0x03
#define CS5480_I_SINGLE 0x14 //single conversion
#define CS5480_I_CONT   0x15 //continuous conversion
#define CS5480_I_HALT   0x18 //halt conversion
/* calibration instructions */
#define CS5480_I_DCOFF_I1  0x21 //calibration of DC offset
#define CS5480_I_DCOFF_V1  0x22 //calibration of DC offset
#define CS5480_I_DCOFF_I2  0x23 //calibration of DC offset
#define CS5480_I_DCOFF_V2  0x24 //calibration of DC offset
#define CS5480_I_DCOFF_ALL 0x26 //calibration of DC offset

#define CS5480_I_ACOFF_I1  0x31 //calibration of AC offset
#define CS5480_I_ACOFF_V1  0x32 //calibration of AC offset
#define CS5480_I_ACOFF_I2  0x33 //calibration of AC offset
#define CS5480_I_ACOFF_V2  0x34 //calibration of AC offset
#define CS5480_I_ACOFF_ALL 0x36 //calibration of AC offset

#define CS5480_I_GAIN_I1  0x39 //calibration of Gain
#define CS5480_I_GAIN_V1  0x3A //calibration of Gain
#define CS5480_I_GAIN_I2  0x3B //calibration of Gain
#define CS5480_I_GAIN_V2  0x3C //calibration of Gain
#define CS5480_I_GAIN_ALL 0x3E //calibration of Gain

/* register map: DEFAULT VALUES SHALL BE OR-ed with any other setting before writing */
/* PAGE0 */
#define CS5480_CONFIG0 0x00 //default 0xC02000
#define 	CS5480_CONFIG0_DEF     0xC02000 //default
#define 	CS5480_CONFIG0_IZX_CH  0x000002 //channel for zero crossing detection
#define 	CS5480_CONFIG0_NO_OSC  0x000004 //disable crystal oscillator
#define 	CS5480_CONFIG0_I1PGA   0x000020 //50x gain for I1 channel
#define 	CS5480_CONFIG0_I2PGA   0x000080 //50x gain for I2 channel
#define 	CS5480_CONFIG0_INT_POL 0x000100 //interrupt polarity
#define CS5480_CONFIG1 0x01 //default 0x00EEEE
#define 	CS5480_CONFIG1_DEF     0x00E000
#define 	CS5480_CONFIG1_DO1MODE 0x00000F
#define 	CS5480_CONFIG1_DO2MODE 0x0000F0
#define 	CS5480_CONFIG1_DO3MODE 0x000F00
#define 	CS5480_CONFIG1_DO1_OD  0x010000
#define 	CS5480_CONFIG1_DO2_OD  0x020000
#define 	CS5480_CONFIG1_DO3_OD  0x030000
#define 	CS5480_CONFIG1_EPG1_ON 0x100000
#define 	CS5480_CONFIG1_EPG2_ON 0x200000
#define 	CS5480_CONFIG1_EPG3_ON 0x300000
#define CS5480_MASK    0x03 //Interrupt mask, default 0x000000
#define CS5480_PC      0x05 //Phase compensation control, default 0x000000
#define CS5480_SERCTRL 0x07 //UART control, default 0x02004D
#define 	CS5480_SERCTRL_DEF         0x000000
#define 	CS5480_SERCTRL_BR          0x00FFFF //Baud rate
#define 	CS5480_SERCTRL_RX_CSUM_OFF 0x020000 //disable checksum
#define 	CS5480_SERCTRL_RX_PU_OFF   0x040000 //disable pullup resistor on RX input
#define CS5480_PLSW    0x08 //Energy pulse width, default 0x000001
#define CS5480_PLSCTRL 0x09 //Energy Pulse control, default 0x000000
#define CS5480_STATUS0 0x17 //default 0x800000
#define 	CS5480_STATUS0_DEF      0x000000
#define 	CS5480_STATUS0_RX_TO    0x000001 //SRX timeout
#define 	CS5480_STATUS0_RX_CSERR 0x000003 //rx checksum error
#define 	CS5480_STATUS0_IC       0x000008 //invalid command received
#define 	CS5480_STATUS0_FUP      0x000010 //frequency updated
#define 	CS5480_STATUS0_TUP      0x000020 //temperature updated
#define 	CS5480_STATUS0_V1SAG    0x000040
#define 	CS5480_STATUS0_V2SAG    0x000080
#define 	CS5480_STATUS0_I1OC     0x000100 //I1 overcurrent
#define 	CS5480_STATUS0_I2OC     0x000200 //I2 overcurrent
#define 	CS5480_STATUS0_V1OR     0x000400 //V1 out of range
#define 	CS5480_STATUS0_V2OR     0x000800 //V2 out of range
#define 	CS5480_STATUS0_I1OR     0x001000 //I1 out of range
#define 	CS5480_STATUS0_I2OR     0x002000 //I2 out of range
#define 	CS5480_STATUS0_P1OR     0x004000 //power 1 out of range
#define 	CS5480_STATUS0_P2OR     0x008000 //power 2 out of range
#define 	CS5480_STATUS0_V1SWELL  0x010000
#define 	CS5480_STATUS0_V2SWELL  0x020000
#define 	CS5480_STATUS0_MIPS     0x040000
#define 	CS5480_STATUS0_WOF      0x200000
#define 	CS5480_STATUS0_CRDY     0x400000 //conversion ready
#define 	CS5480_STATUS0_DRDY     0x800000 //data ready
#define CS5480_STATUS1 0x18 //default 0x801800
#define 	CS5480_STATUS1_DEF 0x000000
#define 	CS5480_STATUS1_I1OD 0x000001 //Current 1 ADC modulator oscillation detected
#define 	CS5480_STATUS1_I2OD 0x000002 //Current 2 ADC modulator oscillation detected
#define 	CS5480_STATUS1_VOD  0x000004 //Voltage ADC modulator oscillation detected
#define 	CS5480_STATUS1_TOD  0x000008 //I1 ADC modulator oscillation detected
#define 	CS5480_STATUS1_LCOM 0x00FF00 //Last serial command executed
#define CS5480_STATUS2 0x19 //default 0x000000
#define 	CS5480_STATUS2_DEF       0x000000
#define 	CS5480_STATUS2_P1_SIGN   0x000001
#define 	CS5480_STATUS2_P2_SIGN   0x000002
#define 	CS5480_STATUS2_PSUM_SIGN 0x000004
#define 	CS5480_STATUS2_Q1_SIGN   0x000008
#define 	CS5480_STATUS2_Q2_SIGN   0x000010
#define 	CS5480_STATUS2_QSUM_SIGN 0x000020
#define CS5480_REGLOCK 0x22 //Register lock control, default 0x000000
#define CS5480_V1PEAK  0x24 //V1 peak voltage, range [-1:1)
#define CS5480_I1PEAK  0x25 //I1 peak current, range [-1:1)
#define CS5480_V2PEAK  0x26 //V2 peak voltage, range [-1:1)
#define CS5480_I2PEAK  0x27 //I2 peak current, range [-1:1)
#define CS5480_PSDC    0x30 //Phase sequence detection and control, default 0x000000
#define CS5480_ZXNUM   0x37 //Number of 0-crossings used for line freq Y, default 0x000064
/* PAGE 16 */
#define CS5480_CONFIG2 0x00 //default 0x000200
#define 	CS5480_CONFIG2_DEF    0x000000
#define 	CS5480_CONFIG2_IIROFF 0x000001 //bypass iir filter
#define 	CS5480_CONFIG2_V1FLT_HP  0x000002 //HP filter enable for v1
#define 	CS5480_CONFIG2_I1FLT_HP  0x000008 //HP filter enable for i1
#define 	CS5480_CONFIG2_V2FLT_HP  0x000020 //HP filter enable for v2
#define 	CS5480_CONFIG2_I2FLT_HP  0x000080 //HP filter enable for i2
#define 		CS5480_CONFIG2_NOFILT 0x0 //no filter
#define 		CS5480_CONFIG2_HPFILT 0x1 //high-pass filter
#define 		CS5480_CONFIG2_PMFILT 0x2 //phase-matching filter
#define 	CS5480_CONFIG2_AFC    0x000200 //enable auto line frequency detection
#define 	CS5480_CONFIG2_RCO    0x000400 //DISABLE CHECKSUM ON CRITICAL REGISTERS
#define 	CS5480_CONFIG2_AVGMOD 0x000800 //average mode
#define 	CS5480_CONFIG2_ZX_LPF 0x001000 //enable LPF zero cross detection
#define 	CS5480_CONFIG2_APCM   0x004000 //apparent power calculation method
#define 	CS5480_CONFIG2_MCFG   0x060000 //METER CONFIGURATION
#define 	CS5480_CONFIG2_IVSP   0x080000 //use Irms for auto channel selection
#define 	CS5480_CONFIG2_IHOLD  0x100000 //suspend auto channel selection
#define 	CS5480_CONFIG2_ICHAN  0x200000 //channel for P,Q,S sum
#define 	CS5480_CONFIG2_POS    0x400000 //only positive energy
#define 	CS5480_CONFIG2_VFIX   0x800000 //use internal reference
#define CS5480_REGCHK  0x01 //
#define CS5480_I1      0x02 //instantaneous current measure [-1.0:1.0)
#define CS5480_V1      0x03 //instantaneous voltage measure [-1.0:1.0)
#define CS5480_P1      0x04 //instantaneous active power measure [-1.0:1.0)
#define CS5480_P1AVG   0x05 //averaged active power measure [-1.0:1.0) over SampleCount samples
#define CS5480_I1RMS   0x06 //range [0:1)
#define CS5480_V1RMS   0x07 //range [0:1)
#define CS5480_I2      0x08 //instantaneous current measure [-1.0:1.0)
#define CS5480_V2      0x09 //instantaneous voltage measure [-1.0:1.0)
#define CS5480_P2      0x0A //instantaneous active power measure [-1.0:1.0)
#define CS5480_P2AVG   0x0B //averaged active power measure [-1.0:1.0) over SampleCount samples
#define CS5480_I2RMS   0x0C //range [0:1)
#define CS5480_V2RMS   0x0D //range [0:1)
#define CS5480_Q1AVG   0x0E //reactive power [-1:1)
#define CS5480_Q1      0x0F //instantaneous quadrature power [-1:1)
#define CS5480_Q2AVG   0x10 //reactive power [-1:1)
#define CS5480_Q2      0x11 //instantaneous quadrature power [-1:1)
#define CS5480_S1      0x14 //apparent power [0:1)
#define CS5480_PF1     0x15 //power factor [-1:1)
#define CS5480_S2      0x18 //apparent power [0:1)
#define CS5480_PF2     0x19 //power factor [-1:1)
#define CS5480_T       0x1B //temperature [-128:127], integer part 0x7F0000
#define CS5480_PSUM    0x1D //total active power [-1:1)
#define CS5480_SSUM    0x1E //total apparent power [0:1)
#define CS5480_QSUM    0x1F //total reactive power [-1:1)
#define CS5480_I1DCOFF 0x20 // 1/DC_OFFSET filled after offset calibration, range [-1,1)
#define CS5480_I1GAIN  0x21 //range [0:4), default 0x400000 = 1.0
#define CS5480_V1DCOFF 0x22 // 1/DC_OFFSET filled after offset calibration, range [-1,1)
#define CS5480_V1GAIN  0x23 //range [0:4), default 0x400000 = 1.0
#define CS5480_P1OFF   0x24 //offset, range [-1:1)
#define CS5480_I1ACOFF 0x25 //offset, range [0:1)
#define CS5480_Q1OFF   0x26 //offset, range [-1:1)
#define CS5480_I2DCOFF 0x27 // 1/DC_OFFSET filled after offset calibration, range [-1,1)
#define CS5480_I2GAIN  0x28 //range [0:4), default 0x400000 = 1.0
#define CS5480_V2DCOFF 0x29 // 1/DC_OFFSET filled after offset calibration, range [-1,1)
#define CS5480_V2GAIN  0x2A //range [0:4), default 0x400000 = 1.0
#define CS5480_P2OFF   0x2B //offset, range [-1:1)
#define CS5480_I2ACOFF 0x2C //offset, range [0:1)
#define CS5480_Q2OFF   0x2D //offset, range [-1:1)
#define CS5480_EPSILON 0x31 //default 0x01999A - 2's complement in range [-1.0,1.0)
#define CS5480_ICHANLV 0x32 //default 0x828F5C
#define CS5480_SAMPCNT 0x33 //sample count, number of OWR for lowrate results (default 0x000FA0 = 4000 = 1 second)
#define CS5480_TGAIN   0x36 //temperature gain [0:256), default 0x06B716= 6.715
#define CS5480_TOFF    0x37 //temperature offset [-128:128], default 0xD53998 = -42.775
#define CS5480_PMIN    0x38 //channel select minimum amplitude 0x00624D
#define CS5480_TSETTLE 0x39 //Filter settling time for conversion startup, OWR cycles, default 0x1E
#define CS5480_LOADMIN 0x3A //default 0x0
#define CS5480_VFRMS   0x3B //default 0x5A8259
#define CS5480_SYSGAIN 0x3C //default 0x500000 (=1.25) range -2:+2
#define CS5480_TIME    0x3D //counted in OWR samples (MCLK/1024=4Hz)
/* PAGE 17 */
#define CS5480_V1SAGDUR 0x00 //v1 sag duration (OWR samples)
#define CS5480_V1SAGLVL 0x01 //v1 sag level (threshold -1:+1)
#define CS5480_I1OVRDUR 0x04 //overcurrent duration
#define CS5480_I1OVRLVL 0x05 //overcurrent level (def 0x7FFFFF)
#define CS5480_V2SAGDUR 0x08 //
#define CS5480_V2SAGLVL 0x09 //
#define CS5480_I2OVRDUR 0x0C //
#define CS5480_I2OVRLVL 0x0D //
/* PAGE 18 */
#define CS5480_IZXLVL  0x18 //0x100000
#define CS5480_PLSRATE 0x1C //0x800000
#define CS5480_INTGAIN 0x2B //0x143958
#define CS5480_V1SWDUR 0x2E //swell duration 0x000000
#define CS5480_V1SWLVL 0x2F //swell level 0x7FFFFF
#define CS5480_V2SWDUR 0x32 //0x000000
#define CS5480_V2SWLVL 0x33 //0x7FFFFF
#define CS5480_VZXLVL  0x3A //0x100000
#define CS5480_CYCCNT  0x3E //
#define CS5480_SCALE   0x3F //calibration scale, [-1:1), default 0x4CCCCC (0.6)

void cs5480_instruction(uint8_t instr);
void cs5480_wait_drdy(cs5480_t * pm);
void cs5480_select_page(uint8_t page);
uint32_t cs5480_reg_read(uint8_t address);
void cs5480_reg_write(uint8_t address, uint32_t value);
uint8_t cs5480_meas_to_str(int32_t meas, uint8_t *str, uint8_t type, uint8_t has_sign);

#endif /* CS5480_H_ */
