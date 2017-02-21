/*
 * core_irq.h
 *
 *  Created on: 15.06.2015
 *      Author: malatesta_a
 */

#ifndef CORE_IRQ_H_
#define CORE_IRQ_H_



/* IRQ peripheral's register offsets (fabric IRQs) */
/* fast interrupts */
#define FIQ_SOFT_INT 0x00 //soft interrupt control. Write 1 to set (RW)
#define FIQ_SOFT_CLR 0x04 //soft interrupt clear. Write 1 to clear (W)
#define FIQ_ENA_INT  0x08 //enable. write 1 to enable (RW)
#define FIQ_ENA_CLR  0x0C //disable. write 1 to disable (W)
#define FIQ_RAW_STA  0x10 //raw status (R)
#define FIQ_INT_STA  0x14 //masked status (r)
/* normal interrupts */
#define IRQ_SOFT_INT 0x18 //soft interrupt control. Write 1 to set (RW)
#define IRQ_SOFT_CLR 0x1C //soft interrupt clear. Write 1 to clear (W)
#define IRQ_ENA_INT  0x20 //enable. write 1 to enable (RW)
#define IRQ_ENA_CLR  0x24 //disable. write 1 to disable (W)
#define IRQ_RAW_STA  0x28 //raw status (R)
#define IRQ_INT_STA  0x2C //masked status (r)

void config_irq();
//void enable_all_gpio_irq();
//void disable_all_gpio_irq();

#endif /* CORE_IRQ_H_ */
