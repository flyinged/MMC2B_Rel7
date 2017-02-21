/*******************************************************************************
* Company: Microsemi Corporation
*
* File: bsp_config.h
* File history:
*      Revision: 1.0 Date: May 4, 2010
*
* Description:
*
* Configuration for the ON-BOARD peripherals for SmartFusion KITS.
* 
* Author: Upender Cherukupally
*         Upender.Cherukupally@microsemi.com
*         Corporate Applications Engineering
*
*******************************************************************************/

#ifndef BSP_CONFIG_H_
#define BSP_CONFIG_H_

//#include "../drivers/mss_i2c/mss_i2c.h"
/* Configuration for OLED */
//#define OLED_I2C_INSTANCE    &g_mss_i2c0
/* Configuration for the SPI Flash */
#define SPI_FLASH_ON_SF_DEV_KIT  1 //XXX IMPORTANT: MMC2 configuration needs this setting
#define SPI_FLASH_ON_SF_EVAL_KIT 0

#define USE_DMA_FOR_SPI_FLASH 1
#define SPI_FLASH_DMA_CHANNEL 0

#endif



