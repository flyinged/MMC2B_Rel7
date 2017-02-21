/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
//#include "spi_flash.h" //_ML84
#include "s25fl128s.h"
#include "ffconf.h"

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */
/*-----------------------------------------------------------------------*/

#define ATA		0
#define MMC		1
#define USB		2

#define SECTOR_SIZE _MAX_SS //ML84 added (was hardcoded to 4096)

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize ( BYTE drv ) /* Physical drive number (0..) */
{

//	if (SPI_FLASH_SUCCESS == spi_flash_init())
//	{
//
//		if(SPI_FLASH_SUCCESS ==
//		   spi_flash_control_hw(SPI_FLASH_GLOBAL_UNPROTECT,0,NULL))
//		{
//   	       return 0;
//		}
//	}

	//ML84 changed
	FLASH_init();
	FLASH_global_unprotect();

    return 0;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
    //ML84 changed
	//return (spi_flash_read(sector*4096, buff, count*4096));

    FLASH_read(sector*SECTOR_SIZE, buff, count*SECTOR_SIZE);
    return 0;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
/* The FatFs module will issue multiple sector transfer request
/  (count > 1) to the disk I/O layer. The disk function should process
/  the multiple sector transfer properly Do. not translate it into
/  multiple single sector transfers to the media, or the data read/write
/  performance may be drastically decreased. */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive number (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */ //ML84 sector NUMBER (not address). The value is multiplied by 4096
	BYTE count			/* Number of sectors to write (1..255) */
)
{
    int i;

    for (i =1; i<=count; i++)
    {
        //ML84 changed
    	//spi_flash_control_hw(SPI_FLASH_4KBLOCK_ERASE,(sector*i*4096),NULL);
#if (SECTOR_SIZE <= 4096)
        //FLASH_erase_4k_block(sector*i*SECTOR_SIZE); //ML84 something's wrong: for sector 0 the same block will be erased multiple times
        FLASH_erase_4k_block((sector+i-1)*SECTOR_SIZE); //ML84 fixed
#else
    	FLASH_erase_sector((sector+i-1)*SECTOR_SIZE); //ML84 added
#endif
    }

    //ML84 changed
	//return (spi_flash_write(sector*4096, (uint8_t *) buff, count*4096));
	FLASH_program(sector*SECTOR_SIZE, (uint8_t *) buff, count*SECTOR_SIZE);

	//ML84 debug
	//uint8_t dbg[4096];
    //FLASH_read(sector*4096, dbg, count*4096);

	return 0;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
#define FLASH_SIZE_BYTES (1024*1024*8)

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive number (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	UINT *result = (UINT *)buff;
	switch (ctrl) {
    case CTRL_SYNC:
    	break;
    case CTRL_POWER:
    	break;
    case CTRL_LOCK:
    	break;
    case CTRL_EJECT:
    	break;
    case GET_SECTOR_COUNT:
    	*result = FLASH_SIZE_BYTES/SECTOR_SIZE; //ML84: was 2048
    	break;
    case GET_SECTOR_SIZE:
    	*result = SECTOR_SIZE; //ML84 changed 4096;
    	break;
    case GET_BLOCK_SIZE:
    	*result = 1;/*in no.of Sectors */
    	break;
    default:
    	break;
    }
	return 0;

}

