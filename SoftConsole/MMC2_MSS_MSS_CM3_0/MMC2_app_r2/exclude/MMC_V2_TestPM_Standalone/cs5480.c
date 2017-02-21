/*
 * cs5480.c
 *
 *  Created on: 16.07.2015
 *      Author: malatesta_a
 */
#include "cs5480.h"
#include "core_spi.h"

extern spi_instance_t g_core_spi; //shall be declared in main

extern void uint_to_decstr(uint32_t num, uint8_t *str, uint8_t ndigits); //shall be provided somewhere in the code

void cs5480_wait_drdy(cs5480_t * pm) {
	//TODO: implement timeout
	//uint8_t tx_buf[1];
	//uint8_t rx_buf[3];

	//select page 0
	cs5480_select_page(0);
	//read status until DRDY = 1
	do {
		//SPI_transfer_block( &g_core_spi, tx_buf, 1, rx_buf, 3 );
		pm->status0 = cs5480_reg_read(CS5480_STATUS0);
	} while ((pm->status0 & CS5480_STATUS0_DRDY)==0);

	return;
}

void cs5480_instruction(uint8_t instr) {
	uint8_t tx_buf[1];

	tx_buf[0] = CS5480_INSTR | (instr & 0x3F);
	//SPI_transfer_block( &g_core_spi, tx_buf, 1, 0, 0 ); //XXX CHANGED
	SPI_transfer_frame( &g_core_spi, tx_buf[0] );
	return;
}

void cs5480_select_page(uint8_t page) {
	uint8_t tx_buf[1];

	tx_buf[0] = CS5480_PAGE_SEL | (page & 0x3F);
	//SPI_transfer_block( &g_core_spi, tx_buf, 1, 0, 0 ); //XXX CHANGED
	SPI_transfer_frame( &g_core_spi, tx_buf[0] );
}

uint32_t cs5480_reg_read(uint8_t address) {
	uint8_t tx_buf[1];
	uint8_t rx_buf[3];

	tx_buf[0] = CS5480_REG_RD | (address&0x3F);

	//SPI_transfer_block( &g_core_spi, tx_buf, 1, rx_buf, 3 ); //XXX CHANGED
	//SPI_transfer_block( &g_core_spi, tx_buf, 1, 0, 0 ); //XXX CHANGED
	//SPI_transfer_block( &g_core_spi, 0, 0, rx_buf, 3 ); //XXX CHANGED

	SPI_transfer_frame( &g_core_spi, tx_buf[0] );
	rx_buf[0] = SPI_transfer_frame( &g_core_spi, 0xFF); //tx_buf[0] );
	rx_buf[1] = SPI_transfer_frame( &g_core_spi, 0xFF); //tx_buf[0] );
	rx_buf[2] = SPI_transfer_frame( &g_core_spi, 0xFF );

	return (rx_buf[0]<<16) | (rx_buf[1]<<8) | (rx_buf[2]);
}


void cs5480_reg_write(uint8_t address, uint32_t value) {
	uint8_t tx_buf[4];

	tx_buf[0] = CS5480_REG_WR | (address&0x3F);
	tx_buf[1] = (value>>16) & 0xFF;
	tx_buf[2] = (value>>8)  & 0xFF;
	tx_buf[3] = (value)     & 0xFF;
	//SPI_transfer_block( &g_core_spi, tx_buf, 4, 0, 0 ); //XXX CHANGED
	SPI_transfer_frame( &g_core_spi, tx_buf[0] );
	SPI_transfer_frame( &g_core_spi, tx_buf[1] );
	SPI_transfer_frame( &g_core_spi, tx_buf[2] );
	SPI_transfer_frame( &g_core_spi, tx_buf[3] );

	return;
}

/* converts a measure in the corresponding humand readable string
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
	uint32_t scale_factor;
	uint8_t scale_shift;
	uint64_t ubuf;

	//first of all determine sign and save absolute value
	if (has_sign) {
		if (meas & 0x00800000) { //is signed negative
			sbuf = meas | 0xFF800000; //extend sign
			sbuf = -sbuf; //make positive
			ubuf = (uint64_t) sbuf; //store value
			str[0] = '-';
		} else { //is signed positive
			ubuf = (uint64_t) meas & 0x007FFFFF;
			str[0] = '+';
		}
	} else { //is unsigned or signed positive
		ubuf = (uint64_t) meas & 0x00FFFFFF;
		ubuf >>= 1;
		str[0] = '+';
	}

	//now determine scale factor
	switch(type) {
	case 0: //is voltage: 330.25 => 0x529 (2 fractional bits)
		scale_factor = 0x529;
		scale_shift  = 2;
		ubuf *= scale_factor;
		uint_to_decstr( (ubuf)>>(23+scale_shift), str+1, 3);
		str[4] = '.';
		ubuf = 1000*(ubuf & 0xFFFFFF); //display 3 fractional bits in decimal
		uint_to_decstr( (ubuf)>>(23+scale_shift), str+5, 3);
		break;
	case 1: //is current1: 16.667 => 0x8555 (11 fractional bits)
		scale_factor = 0x8555;
		scale_shift  = 11;
		ubuf *= scale_factor;
		uint_to_decstr( (ubuf)>>(23+scale_shift), str+1, 2);
		str[3] = '.';
		ubuf = 10000*(ubuf); //display 4 fractional bits in decimal, mask 24+scale_shift
		uint_to_decstr( (ubuf)>>(23+scale_shift), str+4, 4);
		break;
	case 2: //is current2: 14.2857 => 0xE492 (12 fractional bits)
		scale_factor = 0xE492;
		scale_shift  = 12;
		ubuf *= scale_factor;
		uint_to_decstr( (ubuf)>>(23+scale_shift), str+1, 2);
		str[3] = '.';
		ubuf = 10000*(ubuf); //display 4 fractional bits in decimal
		uint_to_decstr( (ubuf)>>(23+scale_shift), str+4, 4);
		break;
	case 3: //is power 1: 330.25*16.6667 = 5504.17 => 0xAC01 (3 frac bits)
		scale_factor = 0xAC01;
		scale_shift  = 3;
		ubuf *= scale_factor;
		uint_to_decstr( (ubuf)>>(23+scale_shift), str+1, 4);
		str[5] = '.';
		ubuf = 100*(ubuf); //display 2 fractional bits in decimal
		uint_to_decstr( (ubuf)>>(23+scale_shift), str+6, 2);
		break;
	case 4: //is power 2: 330.25*14.2857 = 4717.86 => 0x936E (3 frac bits)
		scale_factor = 0x936E;
		scale_shift  = 3;
		ubuf *= scale_factor;
		uint_to_decstr( (ubuf)>>(23+scale_shift), str+1, 4);
		str[5] = '.';
		ubuf = 100*(ubuf); //display 2 fractional bits in decimal
		uint_to_decstr( (ubuf)>>(23+scale_shift), str+6, 2);
		break;
	default:
		memcpy(str, "ERROR!!!", 8);
	}


	return 8; //string always same length
}
