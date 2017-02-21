/*
 * irq.c
 *
 *  Created on: 15.06.2015
 *      Author: malatesta_a
 */

#include "irq.h"
#include "MMC2_hw_platform.h"
#include "hal.h"
#include "core_i2c.h"


extern volatile uint32_t tick_counter;
extern i2c_instance_t g_core_i2c_pm;

//void config_irq() {
//	/* INTERRUPT CONTROLLER (APB peripheral) */
//	HW_set_8bit_reg (CORE_IRQ_BASE + FIQ_SOFT_CLR, 0xFF);       //clear soft FIQ
//	HW_set_8bit_reg (CORE_IRQ_BASE + FIQ_ENA_CLR,  0xFF);       //disable FIQ
//	HW_set_32bit_reg(CORE_IRQ_BASE + IRQ_SOFT_CLR, 0xFFFFFFFF); //clear soft IRQ
//	HW_set_32bit_reg(CORE_IRQ_BASE + IRQ_ENA_CLR,  0xFFFFFFFF); //disable all IRQ
//	return;
//}

//void enable_all_gpio_irq() {
//	MSS_GPIO_enable_irq( MSS_GPIO_14 );
//	MSS_GPIO_enable_irq( MSS_GPIO_15 );
//	MSS_GPIO_enable_irq( MSS_GPIO_16 );
//	MSS_GPIO_enable_irq( MSS_GPIO_17 );
//	MSS_GPIO_enable_irq( MSS_GPIO_18 );
//	MSS_GPIO_enable_irq( MSS_GPIO_19 );
//	MSS_GPIO_enable_irq( MSS_GPIO_20 );
//	MSS_GPIO_enable_irq( MSS_GPIO_21 );
//	MSS_GPIO_enable_irq( MSS_GPIO_22 );
//	MSS_GPIO_enable_irq( MSS_GPIO_23 );
//	return;
//}
//
//void disable_all_gpio_irq() {
//	MSS_GPIO_disable_irq( MSS_GPIO_14 );
//	MSS_GPIO_disable_irq( MSS_GPIO_15 );
//	MSS_GPIO_disable_irq( MSS_GPIO_16 );
//	MSS_GPIO_disable_irq( MSS_GPIO_17 );
//	MSS_GPIO_disable_irq( MSS_GPIO_18 );
//	MSS_GPIO_disable_irq( MSS_GPIO_19 );
//	MSS_GPIO_disable_irq( MSS_GPIO_20 );
//	MSS_GPIO_disable_irq( MSS_GPIO_21 );
//	MSS_GPIO_disable_irq( MSS_GPIO_22 );
//	MSS_GPIO_disable_irq( MSS_GPIO_23 );
//	return;
//}


/* Service the I2C timeout functionality (only external I2Cs) */
__attribute__((__interrupt__)) void SysTick_Handler(void)
{
	//MSS_I2C_system_tick(&g_mss_i2c0, 10); /* count timeout for I2C */
    //I2C_system_tick(&g_core_i2c1, 10); //add also for other channels if used

    I2C_system_tick(&g_core_i2c_pm, 10); /* count timeout for I2C */

    tick_counter++;
}

//__attribute__((__interrupt__)) void Timer1_IRQHandler(void) {
//
//	MSS_TIM1_clear_irq( );
//
//	if (ofabric_old & F2C_REG_SPE_I) { //got SPE from GPAC
//		write_to_gpac = 1;
//	}
//}

/* FABRIC IRQ CONNECTED TO CORE INTERRUPT IP */
__attribute__((__interrupt__)) void Fabric_IRQHandler(void)
{
	uint32_t reg;

	/* get IRQ info from Core Interrupt */
	reg = HW_get_32bit_reg(COREINTERRUPT_0 + 0x2C);

	if (reg & 0x1) {
		I2C_isr(&g_core_i2c_pm); /* Local I2C core ISR */
	}
}


/* INTERNAL GPIO INTERRUPTS */
//__attribute__((__interrupt__)) void GPIO14_IRQHandler( void )
//{
//	//disable_all_gpio_irq();
//    /* Clear interrupt */
//    MSS_GPIO_clear_irq( MSS_GPIO_14 );
//	/* signal that a gpio change has been detected */
//	mss_gpio_changed = 1;
//}
//__attribute__((__interrupt__)) void GPIO15_IRQHandler( void )
//{
//	//disable_all_gpio_irq();
//    /* Clear interrupt */
//    MSS_GPIO_clear_irq( MSS_GPIO_15 );
//	/* signal that a gpio change has been detected */
//	mss_gpio_changed = 1;
//}
//__attribute__((__interrupt__)) void GPIO16_IRQHandler( void )
//{
//	//disable_all_gpio_irq();
//    /* Clear interrupt */
//    MSS_GPIO_clear_irq( MSS_GPIO_16 );
//	/* signal that a gpio change has been detected */
//	mss_gpio_changed = 1;
//}
//__attribute__((__interrupt__)) void GPIO17_IRQHandler( void )
//{
//	//disable_all_gpio_irq();
//    /* Clear interrupt */
//    MSS_GPIO_clear_irq( MSS_GPIO_17 );
//	/* signal that a gpio change has been detected */
//	mss_gpio_changed = 1;
//}
//__attribute__((__interrupt__)) void GPIO18_IRQHandler( void )
//{
//	//disable_all_gpio_irq();
//    /* Clear interrupt */
//    MSS_GPIO_clear_irq( MSS_GPIO_18 );
//	/* signal that a gpio change has been detected */
//	mss_gpio_changed = 1;
//}
//__attribute__((__interrupt__)) void GPIO19_IRQHandler( void )
//{
//	//disable_all_gpio_irq();
//    /* Clear interrupt */
//    MSS_GPIO_clear_irq( MSS_GPIO_19 );
//	/* signal that a gpio change has been detected */
//	mss_gpio_changed = 1;
//}
//__attribute__((__interrupt__)) void GPIO20_IRQHandler( void )
//{
//	//disable_all_gpio_irq();
//    /* Clear interrupt */
//    MSS_GPIO_clear_irq( MSS_GPIO_20 );
//	/* signal that a gpio change has been detected */
//	mss_gpio_changed = 1;
//}
//__attribute__((__interrupt__)) void GPIO21_IRQHandler( void )
//{
//	//disable_all_gpio_irq();
//    /* Clear interrupt */
//    MSS_GPIO_clear_irq( MSS_GPIO_21 );
//	/* signal that a gpio change has been detected */
//	mss_gpio_changed = 1;
//}
//__attribute__((__interrupt__)) void GPIO22_IRQHandler( void )
//{
//	//disable_all_gpio_irq();
//    /* Clear interrupt */
//    MSS_GPIO_clear_irq( MSS_GPIO_22 );
//	/* signal that a gpio change has been detected */
//	mss_gpio_changed = 1;
//}
//__attribute__((__interrupt__)) void GPIO23_IRQHandler( void )
//{
//	//disable_all_gpio_irq();
//    /* Clear interrupt */
//    MSS_GPIO_clear_irq( MSS_GPIO_23 );
//	/* signal that a gpio change has been detected */
//	mss_gpio_changed = 1;
//}
