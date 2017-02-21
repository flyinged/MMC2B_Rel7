/*
 * irq.c
 *
 *  Created on: 15.06.2015
 *      Author: malatesta_a
 */

#include "irq.h"
#include "main.h"
#include "MMC2_hw_platform.h"
//#include "hw_reg_access.h"
#include "mss_timer.h"

extern volatile uint8_t mss_gpio_changed;
extern volatile uint8_t i2c_gpio_changed;
extern volatile uint8_t update_regmap;
extern volatile uint32_t tick_counter;
//extern uint16_t i2c_input_old;
extern uint32_t fabric2cpu_old;

void config_irq() {
	/* INTERRUPT CONTROLLER (APB peripheral) */
	HW_set_8bit_reg (CORE_IRQ_BASE + FIQ_SOFT_CLR, 0xFF);       //clear soft FIQ
	HW_set_8bit_reg (CORE_IRQ_BASE + FIQ_ENA_CLR,  0xFF);       //disable FIQ
	HW_set_32bit_reg(CORE_IRQ_BASE + IRQ_SOFT_CLR, 0xFFFFFFFF); //clear soft IRQ
	HW_set_32bit_reg(CORE_IRQ_BASE + IRQ_ENA_CLR,  0xFFFFFFFF); //disable all IRQ
	return;
}


/* Service the I2C timeout functionality (only external I2Cs) */
__attribute__((__interrupt__)) void SysTick_Handler(void)
{
    MSS_I2C_system_tick(&g_mss_i2c0, 10); /* count timeout for I2C */
    I2C_system_tick(&g_core_i2c_pm, 10);  /* count timeout for I2C */
    I2C_system_tick(&g_core_i2c_tmp, 10);  /* count timeout for I2C */
    I2C_system_tick(&g_core_i2c_pwr, 10);  /* count timeout for I2C */

    tick_counter++;
}

__attribute__((__interrupt__)) void Timer1_IRQHandler(void) {

    update_regmap = 1;

	/* Clear TIM1 interrupt */
	MSS_TIM1_clear_irq();
}

/* FABRIC IRQ CONNECTED TO CORE INTERRUPT IP */
__attribute__((__interrupt__)) void Fabric_IRQHandler(void)
{
	uint32_t reg;

	/* get IRQ info from Core Interrupt */
	reg = HW_get_32bit_reg(CORE_IRQ_BASE + IRQ_INT_STA);

    if (reg & COREI2C_PM_IRQ) {
        I2C_isr(&g_core_i2c_pm); /* Local I2C core ISR */
    }
    if (reg & COREI2C_TMP_IRQ) {
        I2C_isr(&g_core_i2c_tmp); /* Local I2C core ISR */
    }
    if (reg & COREI2C_PWR_IRQ) {
        I2C_isr(&g_core_i2c_pwr); /* Local I2C core ISR */
    }
}

