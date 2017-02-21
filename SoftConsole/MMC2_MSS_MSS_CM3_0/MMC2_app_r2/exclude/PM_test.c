/*
 * Author: Alessandro Malatesta
 *
 * Version History
 * 1.00 - Base
 */

#include "main.h"
#include "string.h"
#include "cs5480.h"


volatile uint8_t mss_gpio_changed;
volatile uint32_t tick_counter;
cs5480_t power_meter;
volatile uint8_t tick, started, cnt;

/* ********************************** MAIN *****************************************/
int main()
{
	/********************** declarations **************************/

	const uint8_t sw_version[] = "\n\n\n\rSoftware version: PM Test 1.20\n\r";

	uint8_t i; /* for loops */
//	uint8_t tx_buf[4];
//	uint8_t rx_buf[4];
//  uint32_t rval = 0x00000000;
//  uint8_t text_buf[32];

	/********************** peripheral initialization **************************/

	/* Initialize and configure UART0. */
	MSS_UART_init(&g_mss_uart0, MSS_UART_57600_BAUD, MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);
	/* make room on console */
	for (i=0; i<80; i++) {
		MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n");
	}
	/* Send info over UART */
	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) welcomeLogo);
	/* Display Software Version */
	MSS_UART_polled_tx_string( &g_mss_uart0, sw_version);


	/* init and config interrupt controller */
	config_irq();
	NVIC_EnableIRQ( Fabric_IRQn );

	/* init and config SPI */
	config_spi();

	/* Init tick counter (periodical interrupts)*/
	NVIC_SetPriority(SysTick_IRQn, 0xFFu); /* Lowest possible priority */
	SysTick_Config(CPU_FREQ / 100); //every 10 ms

	/* Init timer, used for periodic actions */
	MSS_TIM1_init( MSS_TIMER_PERIODIC_MODE );
	MSS_TIM1_load_background( POLLING_PERIOD_S*CPU_FREQ );
	MSS_TIM1_enable_irq();

	/********************** startup **************************/




    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\n\rMonitoring...\n\r");

    //MSS_TIM1_start(); /* start timer: ISR performs periodic actions */

    //HW_set_32bit_reg(CORE_IRQ_BASE + IRQ_ENA_INT, COREI2C_PM_IRQ | COREI2C_TMP_IRQ | COREI2C_PWR_IRQ); //enable interrupt for local I2C cores only

	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rWaiting for command...\n\r");
	started = 0;
	SPI_set_slave_select( &g_core_spi, SPI_SLAVE_0 ); //select slave
	MSS_TIM1_start();
    while (1) {
    	poll_uart();
    }

    return 0;
}

void poll_uart(void) {
	size_t  rx_size;
	uint8_t i, text_buf[128];
	uint8_t uart_rx_buf[1];
	uint32_t rval;
	cs5480_t power_meter;


	MSS_TIM1_disable_irq(); /* temporarily disable periodical interrupt */
	//SPI_set_slave_select( &g_core_spi, SPI_SLAVE_0 ); //select slave

	rx_size = MSS_UART_get_rx ( &g_mss_uart0, uart_rx_buf, sizeof(uart_rx_buf) );
	if( rx_size > 0 ) {

		switch(uart_rx_buf[0]) {
		case 'R': //reset instruction
			//NVIC_SystemReset();
			/* send reset instruction */
			cs5480_instruction(CS5480_I_SW_RST);
			cs5480_wait_drdy(&power_meter);
			break;
		case 'A': //read all registers
			memcpy(text_buf, "Reg 0x00: 0x000000\n\r\0", 21); //init string for display

			/* read & display current status */
			/* read page 0, register 0x0 to 0x37 */
			cs5480_select_page(0);
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r******** Page 00:\n\r");
			i=0;
			while (i<56) {
				for (i=0; i <= 0x37; i++) {
					if (i==0 ||
							i==1 ||
							i==3 ||
							i==5 ||
							i==7 ||
							i==8 ||
							i==9 ||
							i==23 ||
							i==24 ||
							i==25 ||
							i==34 ||
							i==36 ||
							i==37 ||
							i==38 ||
							i==39 ||
							i==48 ||
							i==55) {
						//cs5480_select_page(0);
						rval = cs5480_reg_read(i);
						uint_to_hexstr(i, text_buf+6, 2);
						uint_to_hexstr(rval, text_buf+12, 6);
						MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
					}
				}
			}

			/* read page 16, register 0x0 to 0x3D */
			cs5480_select_page(16);
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r******** Page 16:\n\r");
			for (i=0; i <= 0x3D; i++) {
				if (i!=18 &&
					i!=19 &&
					i!=22 &&
					i!=23 &&
					i!=26 &&
					i!=28 &&
					i!=46 &&
					i!=47 &&
					i!=48 &&
					i!=52 &&
					i!=53 &&
					i!=62 &&
					i!=63 ) {
					rval = cs5480_reg_read(i);
					uint_to_hexstr(i, text_buf+6, 2);
					uint_to_hexstr(rval, text_buf+12, 6);
					MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
				}
			}
			/* read page 17, register 0x0 to 0x0D */
			cs5480_select_page(17);
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r******** Page 17:\n\r");
			for (i=0; i <= 0x0D; i++) {
				if (i==0  ||
					i==1  ||
					i==4  ||
					i==5  ||
					i==8  ||
					i==9  ||
					i==12 ||
					i==13){
					rval = cs5480_reg_read(i);
					uint_to_hexstr(i, text_buf+6, 2);
					uint_to_hexstr(rval, text_buf+12, 6);
					MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
				}
			}
			/* read page 18, register 0x18 to 0x3F */
			cs5480_select_page(18);
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r******** Page 18:\n\r");
			for (i=0x18; i <= 0x3F; i++) {
				if (i==24 ||
					i==28 ||
					i==43 ||
					i==46 ||
					i==47 ||
					i==50 ||
					i==51 ||
					i==58 ||
					i==62 ||
					i==63){
					rval = cs5480_reg_read(i);
					uint_to_hexstr(i, text_buf+6, 2);
					uint_to_hexstr(rval, text_buf+12, 6);
					MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
				}
			}
			break;


		case 'S': //start conversion
			cs5480_instruction(CS5480_I_CONT);
			cs5480_wait_drdy(&power_meter);
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r++++++++++ CONVERSION STARTED ++++++++++\n\n\r");

			started = 1; cnt = 0;
			break;
		case 'P': //stop conversion
			started = 0;
			cs5480_instruction(CS5480_I_HALT);
			cs5480_wait_drdy(&power_meter);
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r---------- CONVERSION STOPPED ----------\n\r");
			break;

		case 'I': //init parameters
			/* page 0 */
			cs5480_select_page(0);

			/* write config0 default value */
			cs5480_reg_write(CS5480_CONFIG0, CS5480_CONFIG0_DEF);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_CONFIG0);
			memcpy(text_buf, "CONFIG0 = 0x000000 (0xC02000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* write config0 default value */
			cs5480_reg_write(CS5480_CONFIG1, CS5480_CONFIG1_DEF | CS5480_CONFIG0_I1PGA | CS5480_CONFIG0_I2PGA); //50x gain for I
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_CONFIG1);
			memcpy(text_buf, "CONFIG1 = 0x000000 (0x00E0A0)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* clear interrupts */
			cs5480_reg_write(CS5480_STATUS0, 0xE7FFFD);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_STATUS0);
			memcpy(text_buf, "IRQFLAG = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* write interrupt mask default value */
			cs5480_reg_write(CS5480_MASK, 0x000000);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_MASK);
			memcpy(text_buf, "IRQMASK = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* write phase compensation control default value */
			cs5480_reg_write(CS5480_PC, 0x000000);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_PC);
			memcpy(text_buf, "PHCOMP  = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* write serial interface control default value */
			cs5480_reg_write(CS5480_SERCTRL, 0x02004D);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_SERCTRL);
			memcpy(text_buf, "SERCTRL = 0x000000 (0x02004D)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* write number of zero crossings used for line-freq detection */
			cs5480_reg_write(CS5480_ZXNUM, 0x000064);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_ZXNUM);
			memcpy(text_buf, "ZXNUM   = 0x000000 (0x000064)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* page 16 */
			cs5480_select_page(16);

			/* write config2 default */
			cs5480_reg_write(CS5480_CONFIG2, 0x000200 | CS5480_CONFIG2_I2FLT_HP | CS5480_CONFIG2_V2FLT_HP | CS5480_CONFIG2_I1FLT_HP | CS5480_CONFIG2_V1FLT_HP); //HP filters
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_CONFIG2);
			memcpy(text_buf, "CONFIG2 = 0x000000 (0x0002AA)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* offsets */
			cs5480_reg_write(CS5480_I1DCOFF, 0x000000); //i1 DC offset
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_I1DCOFF);
			memcpy(text_buf, "I1DCOFF = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_V1DCOFF, 0x000000); //v1 DC offset
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_V1DCOFF);
			memcpy(text_buf, "V1DCOFF = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_P1OFF,   0x000000); //p1 offset
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_P1OFF);
			memcpy(text_buf, "P1OFF   = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_Q1OFF,   0x000000); //q1 offset
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_Q1OFF);
			memcpy(text_buf, "Q1OFF   = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_I2DCOFF, 0x000000); //i2 DC offset
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_I2DCOFF);
			memcpy(text_buf, "I2DCOFF = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_V2DCOFF, 0x000000); //v2 DC offset
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_V2DCOFF);
			memcpy(text_buf, "V2DCOFF = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_P2OFF,   0x000000); //p2 DC offset
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_P2OFF);
			memcpy(text_buf, "P2OFF   = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_Q2OFF,   0x000000); //q2 DC offset
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_Q2OFF);
			memcpy(text_buf, "Q2OFF   = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* gains */
			cs5480_reg_write(CS5480_I1GAIN, 0x400000); //i1 gain
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_I1GAIN);
			memcpy(text_buf, "I1GAIN  = 0x000000 (0x400000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_V1GAIN, 0x400000); //v1 gain
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_V1GAIN);
			memcpy(text_buf, "V1GAIN  = 0x000000 (0x400000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_I2GAIN, 0x400000); //i2 gain
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_I2GAIN);
			memcpy(text_buf, "I2GAIN  = 0x000000 (0x400000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_V2GAIN, 0x400000); //v2 gain
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_V2GAIN);
			memcpy(text_buf, "V2GAIN  = 0x000000 (0x400000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );


			/* write automatic channel selection level default */
			cs5480_reg_write(CS5480_ICHANLV, 0x828F5C);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_ICHANLV);
			memcpy(text_buf, "ICHANLV = 0x000000 (0x828F5C)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* write temp gain default */
			cs5480_reg_write(CS5480_TGAIN, 0x06B716);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_TGAIN);
			memcpy(text_buf, "TMPGAIN = 0x000000 (0x06B716)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* write temp offset default */
			cs5480_reg_write(CS5480_TOFF, 0xD53998);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_TOFF);
			memcpy(text_buf, "TMPOFFS = 0x000000 (0xD53998)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* write chan select min amplitude default */
			cs5480_reg_write(CS5480_PMIN, 0x00624D);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_PMIN);
			memcpy(text_buf, "CS_AMIN = 0x000000 (0x00624D)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* write filter settle time default */
			cs5480_reg_write(CS5480_TSETTLE, 0x00001E);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_TSETTLE);
			memcpy(text_buf, "TSETTLE = 0x000000 (0x00001E)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* write no-load threshold default */
			cs5480_reg_write(CS5480_LOADMIN, 0x000000);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_LOADMIN);
			memcpy(text_buf, "LOADMIN = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* write voltage fixed RMS reference default */
			cs5480_reg_write(CS5480_VFRMS, 0x5A8259);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_VFRMS);
			memcpy(text_buf, "VF_RMS  = 0x000000 (0x5A8259)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* write system gain default */
			cs5480_reg_write(CS5480_SYSGAIN, 0x500000);
			/* readback register to check */
			rval = cs5480_reg_read(CS5480_SYSGAIN);
			memcpy(text_buf, "SYSGAIN = 0x000000 (0x500000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* page 17 */
			cs5480_select_page(17);

			/* sag/overcurrent levels */
			cs5480_reg_write(CS5480_V1SAGDUR, 0x000000); //V1 sag duration
			rval = cs5480_reg_read(CS5480_V1SAGDUR);
			memcpy(text_buf, "V1SAG_D = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_V2SAGDUR, 0x000000); //V2 sag duration
			rval = cs5480_reg_read(CS5480_V2SAGDUR);
			memcpy(text_buf, "V2SAG_D = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_I1OVRDUR, 0x000000); //I1 overcurrent duration
			rval = cs5480_reg_read(CS5480_I1OVRDUR);
			memcpy(text_buf, "I1OVR_D = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_I2OVRDUR, 0x000000); //I2 overcurrent duration
			rval = cs5480_reg_read(CS5480_I2OVRDUR);
			memcpy(text_buf, "I2OVR_D = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_V1SAGLVL, 0x000000); //V1 sag level
			rval = cs5480_reg_read(CS5480_V1SAGLVL);
			memcpy(text_buf, "V1SAG_L = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_V2SAGLVL, 0x000000); //V2 sag level
			rval = cs5480_reg_read(CS5480_V2SAGLVL);
			memcpy(text_buf, "V2SAG_L = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_I1OVRLVL, 0x7FFFFF); //I1 overcurrent level
			rval = cs5480_reg_read(CS5480_I1OVRLVL);
			memcpy(text_buf, "I1OVR_L = 0x000000 (0x7FFFFF)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_I2OVRLVL, 0x7FFFFF); //I2 overcurrent level
			rval = cs5480_reg_read(CS5480_I2OVRLVL);
			memcpy(text_buf, "I2OVR_L = 0x000000 (0x7FFFFF)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			/* page 18 */
			cs5480_select_page(18);

			cs5480_reg_write(CS5480_IZXLVL,  0x100000);
			rval = cs5480_reg_read(CS5480_IZXLVL);
			memcpy(text_buf, "IZX_LVL = 0x000000 (0x100000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_PLSRATE, 0x800000);
			rval = cs5480_reg_read(CS5480_PLSRATE);
			memcpy(text_buf, "PLSRATE = 0x000000 (0x800000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_INTGAIN, 0x143958);
			rval = cs5480_reg_read(CS5480_INTGAIN);
			memcpy(text_buf, "INTGAIN = 0x000000 (0x143958)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_V1SWDUR, 0x000000);
			rval = cs5480_reg_read(CS5480_V1SWDUR);
			memcpy(text_buf, "V1SWDUR = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_V1SWLVL, 0x7FFFFF);
			rval = cs5480_reg_read(CS5480_V1SWLVL);
			memcpy(text_buf, "V1SWLVL = 0x000000 (0x7FFFFF)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_V2SWDUR, 0x000000);
			rval = cs5480_reg_read(CS5480_V2SWDUR);
			memcpy(text_buf, "V2SWDUR = 0x000000 (0x000000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_V2SWLVL, 0x7FFFFF);
			rval = cs5480_reg_read(CS5480_V2SWLVL);
			memcpy(text_buf, "V2SWLVL = 0x000000 (0x7FFFFF)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_VZXLVL,  0x100000);
			rval = cs5480_reg_read(CS5480_VZXLVL);
			memcpy(text_buf, "VZX_LVL = 0x000000 (0x100000)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			cs5480_reg_write(CS5480_SCALE,   0x4CCCCC);
			rval = cs5480_reg_read(CS5480_SCALE);
			memcpy(text_buf, "SYSCALE = 0x000000 (0x4CCCCC)\n\r\0", 32); //init string for display
			uint_to_hexstr(rval, text_buf+12, 6);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

			break;
		default:
			/* display options */
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rR = Power meter reset\n\r");
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rA = Read all registers\n\r");
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rI = Initialize parameters\n\r");
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rA = Read all registers\n\r");
		}

		if (started == 0) {
			key2continue("\n\rPaused. Press a key to continue...\n\r");
			MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Waiting for input...\n\r");
		}

	}

	if (started == 1 && tick == 1) {
		if (cnt == 0) {
			memcpy(text_buf, "I1       V1       P1       I1 RMS   V1 RMS   P1 AVG     I2       V2       P2       I2 RMS   V2 RMS   P2 AVG\n\r\0", 111);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
			memcpy(text_buf, "-----------------------------------------------------------------------------------------------------------\n\r\0", 111);
			MSS_UART_polled_tx_string( &g_mss_uart0, text_buf);
			cnt++;
		} else if (cnt < 30) {
			cnt++;
		} else {
			cnt = 0;
		}


		tick = 0;
		/* page 17 */
		cs5480_select_page(16);

		/* I1 */
		rval = cs5480_reg_read(CS5480_I1);
		i = cs5480_meas_to_str(rval, text_buf, 1, 1); text_buf[i] = ' '; text_buf[i+1] = '\0';
		MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

		/* V1 */
		rval = cs5480_reg_read(CS5480_V1);
		cs5480_meas_to_str(rval, text_buf, 0, 1);
		MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

		/* P1 */
		rval = cs5480_reg_read(CS5480_P1);
		cs5480_meas_to_str(rval, text_buf, 3, 1);
		MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

		/* I1 RMS */
		rval = cs5480_reg_read(CS5480_I1RMS);
		cs5480_meas_to_str(rval, text_buf, 1, 0);
		MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

		/* V1 RMS */
		rval = cs5480_reg_read(CS5480_V1RMS);
		cs5480_meas_to_str(rval, text_buf, 0, 0);
		MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

		/* P1 AVG */
		rval = cs5480_reg_read(CS5480_P1AVG);
		cs5480_meas_to_str(rval, text_buf, 3, 1);
		MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

		/********************************************************/

		/* I2 */
		rval = cs5480_reg_read(CS5480_I2);
		cs5480_meas_to_str(rval, text_buf, 2, 1);
		MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

		/* V2 */
		rval = cs5480_reg_read(CS5480_V2);
		cs5480_meas_to_str(rval, text_buf, 0, 1);
		MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

		/* P2 */
		rval = cs5480_reg_read(CS5480_P2);
		cs5480_meas_to_str(rval, text_buf, 4, 1);
		MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

		/* I2 RMS */
		rval = cs5480_reg_read(CS5480_I2RMS);
		cs5480_meas_to_str(rval, text_buf, 2, 0);
		MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

		/* V2 RMS */
		rval = cs5480_reg_read(CS5480_V2RMS);
		cs5480_meas_to_str(rval, text_buf, 0, 0);
		MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );

		/* P2 AVG */
		rval = cs5480_reg_read(CS5480_P2AVG);
		cs5480_meas_to_str(rval, text_buf, 4, 1); memcpy(text_buf+i, " \n\r\0", 4);
		MSS_UART_polled_tx_string( &g_mss_uart0, text_buf );
	}


	//SPI_clear_slave_select( &g_core_spi, SPI_SLAVE_0 );

	MSS_TIM1_enable_irq(); /* re-enable periodical interrupt */
	return;
}

/************************************************************************************/
void key2continue(const char *message) {
	size_t rx_size;
	uint8_t uart_rx_buf[1];

	rx_size = 0;
	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) message);
	//MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r>>Press a key to continue\n\r");
	while (rx_size == 0) {
		rx_size = MSS_UART_get_rx ( &g_mss_uart0, uart_rx_buf, 1 );
	}
	return;
}

/************************************************************************************/

void uint_to_hexstr(uint32_t num, uint8_t *str, uint8_t ndigits) {
	uint8_t i;
	uint32_t buf;

	for (i=0; i<ndigits; i++) {
		buf = num >> (4*i);
		buf &= 0x0000000F;
		if (buf < 10) {
			str[ndigits-i-1] = buf+48;
		} else {
			str[ndigits-i-1] = buf+55;
		}
	}
	//str[ndigits] = '\0';

	return;
}

void uint_to_decstr(uint32_t num, uint8_t *str, uint8_t ndigits) {
	uint8_t i;
	uint32_t buf;

	buf = num;

	for (i=0; i<ndigits; i++) {
		str[ndigits-i-1] = (buf%10)+48;
		buf /= 10;
	}

	//str[ndigits] = '\0'; //null termination

	return;
}

//converts a signed floating point number in the range [-1:1)
//and returns a string repesen ti the number in decimal digits (+0.123456)
uint8_t int24_to_floatstr(int32_t num, uint8_t *str) {
	const uint8_t NDFD = 5; //number of decimal fractional digits
	uint8_t i, bfd, dfd;
	uint32_t buf;

	str[1] = '0';
	str[2] = '.';
	for (i=3; i<(3+NDFD); i++ ) {
		str[i] = '0';
	}

	//check sign bit
	if ( (num&0x00FFFFFF)== 0x00800000) {
		//special case -1.0
		str[0] = '-';
		str[1] = '1';
		return (3+NDFD);
	} else if (num & 0x00800000) {
		//negative, make sure all sign bits are 1
		buf = -(num | 0xFF800000); //TODO: check correctness
		str[0] = '-';
	} else {
		//positive: make sure all sign bits are 0
		buf = (num & 0x007FFFFF);
		str[0] = '+';
	}

	dfd = NDFD; //decimal fractional digits
	bfd = 23; //binary fractional digits
	while (bfd > 0 || dfd > 0) {
		if (dfd > 0) {
			dfd--;
			buf *= 10;
		}
		if (bfd > 0) {
			if (bfd >= 3) {
				buf >>= 3;
				bfd -= 3;
			} else {
				buf >>= bfd;
				bfd = 0;
			}
		}
	}

	uint_to_decstr(buf, str+3, NDFD);

	return (3+NDFD); //index of first char after end of string
}

//converts a signed floating point number in the range [0:1)
//and returns a string repesen ti the number in decimal digits (+0.123456)
uint8_t uint24_to_floatstr(uint32_t num, uint8_t *str) {
	const uint8_t NDFD = 5; //number of decimal fractional digits
	uint8_t i, bfd, dfd;
	uint32_t buf;

	str[1] = '0';
	str[2] = '.';
	for (i=3; i<(3+NDFD); i++ ) {
		str[i] = '0';
	}

	//always positive: make sure all sign bits are 0
	buf = (num & 0x00FFFFFF);
	str[0] = '+';

	dfd = NDFD; //decimal fractional digits
	bfd = 24; //binary fractional digits
	while (bfd > 0 || dfd > 0) {
		if (dfd > 0) {
			dfd--;
			buf *= 10;
		}
		if (bfd > 0) {
			if (bfd >= 3) {
				buf >>= 3;
				bfd -= 3;
			} else {
				buf >>= bfd;
				bfd = 0;
			}
		}
	}

	uint_to_decstr(buf, str+3, NDFD);

	return (3+NDFD); //index of first char after end of string
}

void config_spi() {
	/* Core SPI */
	SPI_init( &g_core_spi, CORESPI_0, 8 );
	SPI_configure_master_mode( &g_core_spi );

	return;
}

