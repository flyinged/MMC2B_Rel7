#ifndef MMC2_HW_PLATFORM_H_
#define MMC2_HW_PLATFORM_H_
/*****************************************************************************
*
*Created by Microsemi SmartDesign  Mon Jan 30 10:52:35 2017
*
*Memory map specification for peripherals in MMC2
*/

/*-----------------------------------------------------------------------------
* MSS_CM3_0 subsystem memory map
* Master(s) for this subsystem: MSS_CM3_0 FABRIC2MSS_APB_BRIDGE 
*---------------------------------------------------------------------------*/
#define COREI2C                         0x40050000U
#define COREINTERRUPT_0                 0x40060000U
#define MBU_MMC_V2B_APB_0               0x40070000U
#define MSS_I2C_CONTROLLER_0            0x40090000U


/*-----------------------------------------------------------------------------
* MSS_I2C_controller_0 subsystem memory map
* Master(s) for this subsystem: MSS_I2C_controller_0 
*---------------------------------------------------------------------------*/
#define MMC2_MSS_0                      0x00000000U


#endif /* MMC2_HW_PLATFORM_H_*/
