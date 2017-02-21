/*******************************************************************************
 * (c) Copyright 2009-2014 Microsemi SoC Products Group.  All rights reserved.
 * 
 * CoreI2C driver interrupt control.
 * 
 * SVN $Revision: 6251 $
 * SVN $Date: 2014-03-25 12:34:59 +0000 (Tue, 25 Mar 2014) $
 */
#include "hal.h"
#include "hal_assert.h"
//#include "core_i2c.h"
#include "a2fxxxm3.h"
#include "main.h"
#include "irq.h"

extern i2c_instance_t g_core_i2c_pwr;

/*------------------------------------------------------------------------------
 * This function must be modified to enable interrupts generated from the
 * CoreI2C instance identified as parameter.
 */
void I2C_enable_irq( i2c_instance_t * this_i2c )
{
	if(this_i2c == &g_core_i2c_pwr)
	{
		HW_set_32bit_reg(CORE_IRQ_BASE + IRQ_ENA_INT, COREI2C_PWR_IRQ);
	}

}

/*------------------------------------------------------------------------------
 * This function must be modified to disable interrupts generated from the
 * CoreI2C instance identified as parameter.
 */
void I2C_disable_irq( i2c_instance_t * this_i2c )
{
	if(this_i2c == &g_core_i2c_pwr)
	{
		HW_set_32bit_reg(CORE_IRQ_BASE + IRQ_ENA_CLR, COREI2C_PWR_IRQ);
	}

}
