/*
 * cs5480.c
 *
 *  Created on: 16.07.2015
 *      Author: malatesta_a
 */
#include "string.h"
#include "cs5480.h"
#include "mss_spi.h"
#include "mss_uart.h"
#include "uart_utility.h"

//extern spi_instance_t g_mss_spi1; //shall be declared in main
extern mss_uart_instance_t g_mss_uart0;
extern mss_spi_instance_t  g_mss_spi0;
extern void uint_to_decstr(uint32_t num, uint8_t *str, uint8_t ndigits); //shall be provided somewhere in the code

cs5480_t power_meter;

uint8_t cs5480_set_parameters(cs5480_t * pm, uint32_t v1_gain, uint32_t v2_gain) {
	uint32_t rval;
	//uint8_t txt[] = "0x00000000\n\r";
	/* write parameters set in PM structure (values from cs5480.h file) */

	/* page 0 */
	cs5480_select_page(0);

	cs5480_reg_write(CS5480_CONFIG0, CS5480_CONFIG0_DEF); //set init value
	rval = cs5480_reg_read(CS5480_CONFIG0); //readback
	if (rval == CS5480_CONFIG0_DEF) { //check
		pm->config0 = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM config0\n\r" );
		return 1;
	}

	cs5480_reg_write(CS5480_CONFIG1, CS5480_CONFIG1_DEF );
	rval = cs5480_reg_read(CS5480_CONFIG1);
	if (rval == CS5480_CONFIG1_DEF) { //check
		pm->config1 = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM config1\n\r" );
        return 1;
	}

	cs5480_reg_write(CS5480_MASK, CS5480_MASK_DEF);
	rval = cs5480_reg_read(CS5480_MASK);
	if (rval == CS5480_MASK_DEF) { //check
		pm->mask = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM mask\n\r" );
        return 1;
	}


//	cs5480_reg_write(CS5480_PC, CS5480_PC_DEF);
//	rval = cs5480_reg_read(CS5480_PC);
//	if (rval == CS5480_PC_DEF) { //check
//		pm->pc = rval;
//	} else {
//		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "INIT ERROR: pc\n\r" );
//	}
//
//	cs5480_reg_write(CS5480_SERCTRL, CS5480_SERCTL_DEF);
//	rval = cs5480_reg_read(CS5480_SERCTRL);
//	if (rval == CS5480_SERCTL_DEF) { //check
//		pm->serctl = rval;
//	} else {
//		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "INIT ERROR: serctl\n\r" );
//	}
//
//	/* write number of zero crossings used for line-freq detection */
//	cs5480_reg_write(CS5480_ZXNUM, CS5480_ZXNUM_DEF);
//	/* readback register to check */
//	rval = cs5480_reg_read(CS5480_ZXNUM);
//	if (rval == CS5480_ZXNUM_DEF) { //check
//		pm->zxnum = rval;
//	} else {
//		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "INIT ERROR: zxnum\n\r" );
//	}

	//reset all interrupts
	cs5480_reg_write(CS5480_STATUS0, CS5480_STATUS0_DEF);

	/* page 16 */
	cs5480_select_page(16);

//	//ADDED 2.2016
//	cs5480_reg_write(CS5480_SAMPCNT, CS5480_SAMPCNT_DEF);
//	rval = cs5480_reg_read(CS5480_SAMPCNT);
//	if (rval == CS5480_SAMPCNT_DEF) { //check
//	    //pm->mask = rval;
//	} else {
//	    MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize Sample count\n\r" );
//	    return 1;
//	}

	/* write config2 default */
	cs5480_reg_write(CS5480_CONFIG2, CS5480_CONFIG2_DEF);
	rval = cs5480_reg_read(CS5480_CONFIG2);
	if (rval == CS5480_CONFIG2_DEF) { //check
		pm->config2 = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM config2\n\r" );
		return 1;
	}

	/* offsets */
	cs5480_reg_write(CS5480_I1ACOFF, CS5480_I1ACOFF_DEF); //i1 AC offset
	rval = cs5480_reg_read(CS5480_I1ACOFF);
	if (rval == CS5480_I1ACOFF_DEF) { //check
		pm->i1acoff = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM i1acoff\n\r" );
		return 1;
	}

	cs5480_reg_write(CS5480_I1DCOFF, CS5480_I1DCOFF_DEF); //i1 DC offset
	rval = cs5480_reg_read(CS5480_I1DCOFF);
	if (rval == CS5480_I1DCOFF_DEF) { //check
		pm->i1dcoff = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM i1dcoff\n\r" );
		return 1;
	}

	cs5480_reg_write(CS5480_V1DCOFF, CS5480_V1DCOFF_DEF); //v1 DC offset
	rval = cs5480_reg_read(CS5480_V1DCOFF);
	if (rval == CS5480_V1DCOFF_DEF) { //check
		pm->v1dcoff = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM v1dcoff\n\r" );
		return 1;
	}

	cs5480_reg_write(CS5480_P1OFF,   CS5480_P1OFF_DEF); //p1 offset
	rval = cs5480_reg_read(CS5480_P1OFF);
	if (rval == CS5480_P1OFF_DEF) { //check
		pm->p1dcoff = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM p1dcoff\n\r" );
		return 1;
	}

	cs5480_reg_write(CS5480_Q1OFF,   CS5480_Q1OFF_DEF); //q1 offset
	rval = cs5480_reg_read(CS5480_Q1OFF);
	if (rval == CS5480_Q1OFF_DEF) { //check
		pm->q1dcoff = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM q1dcoff\n\r" );
        return 1;
	}

	cs5480_reg_write(CS5480_I2ACOFF, CS5480_I2ACOFF_DEF); //i1 DC offset
	rval = cs5480_reg_read(CS5480_I2ACOFF);
	if (rval == CS5480_I2ACOFF_DEF) { //check
		pm->i2acoff = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM i2acoff\n\r" );
        return 1;
	}

	cs5480_reg_write(CS5480_I2DCOFF, CS5480_I2DCOFF_DEF); //i2 DC offset
	rval = cs5480_reg_read(CS5480_I2DCOFF);
	if (rval == CS5480_I2DCOFF_DEF) { //check
		pm->i2dcoff = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM i2dcoff\n\r" );
        return 1;
	}

	cs5480_reg_write(CS5480_V2DCOFF, CS5480_V2DCOFF_DEF); //v2 DC offset
	rval = cs5480_reg_read(CS5480_V2DCOFF);
	if (rval == CS5480_V2DCOFF_DEF) { //check
		pm->v2dcoff = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM v2dcoff\n\r" );
        return 1;
	}

	cs5480_reg_write(CS5480_P2OFF,   CS5480_P2OFF_DEF); //p2 DC offset
	rval = cs5480_reg_read(CS5480_P2OFF);
	if (rval == CS5480_P2OFF_DEF) { //check
		pm->p2dcoff = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM p2dcoff\n\r" );
        return 1;
	}

	cs5480_reg_write(CS5480_Q2OFF,   CS5480_Q2OFF_DEF); //q2 DC offset
	rval = cs5480_reg_read(CS5480_Q2OFF);
	if (rval == CS5480_Q2OFF_DEF) { //check
		pm->q2dcoff = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM q2dcoff\n\r" );
        return 1;
	}

	/* gains */
	cs5480_reg_write(CS5480_I1GAIN, CS5480_I1GAIN_DEF); //i1 gain
	rval = cs5480_reg_read(CS5480_I1GAIN);
	if (rval == CS5480_I1GAIN_DEF) { //check
		pm->i1gain = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM i1gain\n\r" );
        return 1;
	}

	cs5480_reg_write(CS5480_V1GAIN, v1_gain); //CS5480_V1GAIN_DEF); //v1 gain
	rval = cs5480_reg_read(CS5480_V1GAIN);
	if (rval == v1_gain) { //CS5480_V1GAIN_DEF) { //check
		pm->v1gain = rval;
	} else {
        MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM v1gain\n\r" );
        return 1;
	}

	cs5480_reg_write(CS5480_I2GAIN, CS5480_I2GAIN_DEF); //i2 gain
	rval = cs5480_reg_read(CS5480_I2GAIN);
	if (rval == CS5480_I2GAIN_DEF) { //check
		pm->i2gain = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM i2gain\n\r" );
        return 1;
	}

	cs5480_reg_write(CS5480_V2GAIN, v2_gain); //CS5480_V2GAIN_DEF); //v2 gain
	rval = cs5480_reg_read(CS5480_V2GAIN);
	if (rval == v2_gain) { //CS5480_V2GAIN_DEF) { //check
		pm->v2gain = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM v2gain\n\r" );
        return 1;
	}

//	cs5480_reg_write(CS5480_ICHANLV, CS5480_ICHANLV_DEF);
//	rval = cs5480_reg_read(CS5480_ICHANLV);
//	if (rval == CS5480_ICHANLV_DEF) { //check
//		pm->ichanlv = rval;
//	} else {
//		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "INIT ERROR: ichanlv\n\r" );
//	}

	cs5480_reg_write(CS5480_TGAIN, CS5480_TGAIN_DEF);
	rval = cs5480_reg_read(CS5480_TGAIN);
	if (rval == CS5480_TGAIN_DEF) { //check
		pm->tgain = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM tgain\n\r" );
        return 1;
	}

	cs5480_reg_write(CS5480_TOFF, CS5480_TOFF_DEF);
	rval = cs5480_reg_read(CS5480_TOFF);
	if (rval == CS5480_TOFF_DEF) { //check
		pm->toff = rval;
	} else {
		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) " error: cannot initialize PM toff\n\r" );
        return 1;
	}

//	cs5480_reg_write(CS5480_PMIN, CS5480_PMIN_DEF);
//	rval = cs5480_reg_read(CS5480_PMIN);
//	if (rval == CS5480_PMIN_DEF) { //check
//		pm->pmin = rval;
//	} else {
//		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "INIT ERROR: pmin\n\r" );
//	}
//
//	cs5480_reg_write(CS5480_TSETTLE, CS5480_TSETTLE_DEF);
//	rval = cs5480_reg_read(CS5480_TSETTLE);
//	if (rval == CS5480_TSETTLE_DEF) { //check
//		pm->tsettle = rval;
//	} else {
//		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "INIT ERROR: tsettle\n\r" );
//	}
//
//	cs5480_reg_write(CS5480_LOADMIN, CS5480_LOADMIN_DEF);
//	rval = cs5480_reg_read(CS5480_LOADMIN);
//	if (rval == CS5480_LOADMIN_DEF) { //check
//		pm->loadmin = rval;
//	} else {
//		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "INIT ERROR: loadmin\n\r" );
//	}
//
//	cs5480_reg_write(CS5480_VFRMS, CS5480_VFRMS_DEF);
//	rval = cs5480_reg_read(CS5480_VFRMS);
//	if (rval == CS5480_VFRMS_DEF) { //check
//		pm->vfrms = rval;
//	} else {
//		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "INIT ERROR: vfrms\n\r" );
//	}
//
//	cs5480_reg_write(CS5480_SYSGAIN, CS5480_SYSGAIN_DEF);
//	rval = cs5480_reg_read(CS5480_SYSGAIN);
//	if (rval == CS5480_SYSGAIN_DEF) { //check
//		pm->sysgain = rval;
//	} else {
//		MSS_UART_polled_tx_string( &g_mss_uart0, (uint8_t*) "INIT ERROR: sysgain\n\r" );
//	}

	//add readback check from here on
	/* page 17 */
//	cs5480_select_page(17);
//
//	/* sag/overcurrent levels */
//	cs5480_reg_write(CS5480_V1SAGDUR, 0x000000); //V1 sag duration
//	rval = cs5480_reg_read(CS5480_V1SAGDUR);
//
//	cs5480_reg_write(CS5480_V2SAGDUR, 0x000000); //V2 sag duration
//	rval = cs5480_reg_read(CS5480_V2SAGDUR);
//
//	cs5480_reg_write(CS5480_I1OVRDUR, 0x000000); //I1 overcurrent duration
//	rval = cs5480_reg_read(CS5480_I1OVRDUR);
//
//	cs5480_reg_write(CS5480_I2OVRDUR, 0x000000); //I2 overcurrent duration
//	rval = cs5480_reg_read(CS5480_I2OVRDUR);
//
//	cs5480_reg_write(CS5480_V1SAGLVL, 0x000000); //V1 sag level
//	rval = cs5480_reg_read(CS5480_V1SAGLVL);
//
//	cs5480_reg_write(CS5480_V2SAGLVL, 0x000000); //V2 sag level
//	rval = cs5480_reg_read(CS5480_V2SAGLVL);
//
//	cs5480_reg_write(CS5480_I1OVRLVL, 0x7FFFFF); //I1 overcurrent level
//	rval = cs5480_reg_read(CS5480_I1OVRLVL);
//
//	cs5480_reg_write(CS5480_I2OVRLVL, 0x7FFFFF); //I2 overcurrent level
//	rval = cs5480_reg_read(CS5480_I2OVRLVL);
//
//	/* page 18 */
//	cs5480_select_page(18);
//
//	cs5480_reg_write(CS5480_IZXLVL,  0x100000);
//	rval = cs5480_reg_read(CS5480_IZXLVL);
//
//	cs5480_reg_write(CS5480_PLSRATE, 0x800000);
//	rval = cs5480_reg_read(CS5480_PLSRATE);
//
//	cs5480_reg_write(CS5480_INTGAIN, 0x143958);
//	rval = cs5480_reg_read(CS5480_INTGAIN);
//
//	cs5480_reg_write(CS5480_V1SWDUR, 0x000000);
//	rval = cs5480_reg_read(CS5480_V1SWDUR);
//
//	cs5480_reg_write(CS5480_V1SWLVL, 0x7FFFFF);
//	rval = cs5480_reg_read(CS5480_V1SWLVL);
//
//	cs5480_reg_write(CS5480_V2SWDUR, 0x000000);
//	rval = cs5480_reg_read(CS5480_V2SWDUR);
//
//	cs5480_reg_write(CS5480_V2SWLVL, 0x7FFFFF);
//	rval = cs5480_reg_read(CS5480_V2SWLVL);
//
//	cs5480_reg_write(CS5480_VZXLVL,  0x100000);
//	rval = cs5480_reg_read(CS5480_VZXLVL);
//
//	cs5480_reg_write(CS5480_SCALE,   0x4CCCCC);
//	rval = cs5480_reg_read(CS5480_SCALE);

	return 0;
}

extern volatile uint32_t tick_counter;
extern const uint32_t TOUT;

uint8_t cs5480_wait_drdy(cs5480_t * pm) {
    uint32_t tic;

	//select page 0
	cs5480_select_page(0);
	//read status until DRDY = 1
	tic = tick_counter;
	do {
		//SPI_transfer_block( &g_mss_spi0, tx_buf, 1, rx_buf, 3 );
		pm->status0 = cs5480_reg_read(CS5480_STATUS0);
		if ((tick_counter-tic) > 200) return 1; //2 seconds timeout
	} while ((pm->status0 & CS5480_STATUS0_DRDY)==0);

	return 0;
}

void cs5480_instruction(uint8_t instr) {
	uint8_t tx_buf[1];

	tx_buf[0] = CS5480_INSTR | (instr & 0x3F);
	MSS_SPI_transfer_frame( &g_mss_spi0, tx_buf[0] );
	return;
}

void cs5480_select_page(uint8_t page) {
	uint8_t tx_buf[1];

	tx_buf[0] = CS5480_PAGE_SEL | (page & 0x3F);
	MSS_SPI_transfer_frame( &g_mss_spi0, tx_buf[0] );
}

uint32_t cs5480_reg_read(uint8_t address) {
	uint8_t tx_buf[1];
	uint8_t rx_buf[3];

	tx_buf[0] = CS5480_REG_RD | (address&0x3F);
	MSS_SPI_transfer_frame( &g_mss_spi0, tx_buf[0] );
	rx_buf[0] = MSS_SPI_transfer_frame( &g_mss_spi0, 0xFF); //tx_buf[0] );
	rx_buf[1] = MSS_SPI_transfer_frame( &g_mss_spi0, 0xFF); //tx_buf[0] );
	rx_buf[2] = MSS_SPI_transfer_frame( &g_mss_spi0, 0xFF );

	return (rx_buf[0]<<16) | (rx_buf[1]<<8) | (rx_buf[2]);
}


void cs5480_reg_write(uint8_t address, uint32_t value) {
	uint8_t tx_buf[4];

	tx_buf[0] = CS5480_REG_WR | (address&0x3F);
	tx_buf[1] = (value>>16) & 0xFF;
	tx_buf[2] = (value>>8)  & 0xFF;
	tx_buf[3] = (value)     & 0xFF;
	MSS_SPI_transfer_frame( &g_mss_spi0, tx_buf[0] );
	MSS_SPI_transfer_frame( &g_mss_spi0, tx_buf[1] );
	MSS_SPI_transfer_frame( &g_mss_spi0, tx_buf[2] );
	MSS_SPI_transfer_frame( &g_mss_spi0, tx_buf[3] );

	return;
}

/* converts a measure in the corresponding human readable string
 * meas = 24-bit measure as read from the cs5480 registers. Shall represent a value in [0:1) or [-1:1)
 * str  = pointer to the string that will contain the value (UNTERMINATED)
 * type = type of measure
 * 		  0 = voltage
 * 	  	  1 = I1 current
 * 	      2 = I2 current
 * 		  3 = P1 power
 * 		  4 = P2 power
 * has_sign = 0 for unsigned, 1 for signed values
 *
 * return value = size of the returned string
 */

uint8_t cs5480_meas_to_str(int32_t meas, uint8_t *str, uint8_t type, uint8_t has_sign) {
	int32_t sbuf;
	float fbuf, i1scale, i2scale;

	//first of all determine sign and save absolute value
	if (has_sign) {
		if (meas & 0x00800000) { //is signed negative
			sbuf = meas | 0xFF800000; //extend sign
			sbuf = -sbuf; //make positive
			fbuf = (float) sbuf;
		} else { //is signed positive
		    fbuf = (float) (meas & 0x007FFFFF);
		}
		fbuf /= 8388608.0; //shift right by 23 bits
	} else { //is unsigned or signed positive
	    fbuf = (float) (meas & 0x00FFFFFF);
	    fbuf /= 16777216.0; //shift right by 24 bits
	}

	/********** Now "fbuf" contains a value in range [-1,1) ************/

	//Input voltage V is divided by 1651. Vin=250mv when V=250mv*1651= 412.54V
	//Input current I1 is scaled so that 1A=10mv.
	//    with x10 gain, vmax=250mV => Imax=25A
	//    with x50 gain, vmax=50mv  => Imax=5A

#if (CS5480_CONFIG0_DEF & CS5480_CONFIG0_I1PGA)
	i1scale = 5.0;
#else
	i1scale = 25.0;
#endif

#if (CS5480_CONFIG0_DEF & CS5480_CONFIG0_I2PGA)
    i2scale = 5.0;
#else
    i2scale = 25.0;
#endif

	//now determine scale factor
	switch(type) {
	case 0: //is voltage
	    fbuf *= 412.75;
	    return(float_to_string(fbuf, str, 3, 3));
		break;
	case 1: //is current1
	    fbuf *= i1scale;
	    return(float_to_string(fbuf, str, 2, 4));
		break;
	case 2: //is current2
        fbuf *= i2scale;
        return(float_to_string(fbuf, str, 2, 4));
		break;
	case 3: //is power 1
	    fbuf *= (412.75*i1scale);
        return(float_to_string(fbuf, str, 4, 2));
		break;
	case 4: //is power 2: 330.25*14.2857 = 4717.86 => 0x936E (3 frac bits)
        fbuf *= (412.75*i2scale);
        return(float_to_string(fbuf, str, 4, 2));
		break;
	default:
		memcpy(str, "ERROR!!!", 8);
	}


	return 8; //string always same length
}


float cs5480_meas_to_float(int32_t meas, uint8_t type, uint8_t has_sign) {
    int32_t sbuf;
    float fbuf, i1scale, i2scale;

    //first of all determine sign and save absolute value
    if (has_sign) {
        if (meas & 0x00800000) { //is signed negative
            sbuf = meas | 0xFF800000; //extend sign
            sbuf = -sbuf; //make positive
            fbuf = (float) sbuf;
        } else { //is signed positive
            fbuf = (float) (meas & 0x007FFFFF);
        }
        fbuf /= 8388608.0;
    } else { //is unsigned or signed positive
        fbuf = (float) (meas & 0x00FFFFFF);
        fbuf /= 16777216.0;
    }

    //Input voltage V is divided by 1651. Vin=250mv when V=250mv*1651= 412.54V
    //Input current I1 is scaled so that 1A=10mv.
    //    with x10 gain, vmax=250mV => Imax=25A
    //    with x50 gain, vmax=50mv  => Imax=5A

#if (CS5480_CONFIG0_DEF & CS5480_CONFIG0_I1PGA)
    i1scale = 5.0;
#else
    i1scale = 25.0;
#endif

#if (CS5480_CONFIG0_DEF & CS5480_CONFIG0_I2PGA)
    i2scale = 5.0;
#else
    i2scale = 25.0;
#endif

    //now determine scale factor
    switch(type) {
    case 0: //is voltage
        fbuf *= 412.75;
        return(fbuf);
        break;
    case 1: //is current1
        fbuf *= i1scale;
        return(fbuf);
        break;
    case 2: //is current2
        fbuf *= i2scale;
        return(fbuf);
        break;
    case 3: //is power 1
        fbuf *= (412.75*i1scale);
        return(fbuf);
        break;
    case 4: //is power 2: 330.25*14.2857 = 4717.86 => 0x936E (3 frac bits)
        fbuf *= (412.75*i2scale);
        return(fbuf);
        break;
    default:
        return (-1);
    }
}
