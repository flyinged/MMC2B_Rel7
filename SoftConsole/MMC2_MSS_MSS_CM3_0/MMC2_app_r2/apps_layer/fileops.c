#include <string.h>

#include "ff.h"
#include "mss_uart.h"
#include "uart_utility.h"

FILINFO Finfo;
DWORD acc_size;
WORD acc_files, acc_dirs;

extern int set_fattime(char *Line);
extern int get_line (		/* 0:End of stream, 1:A line arrived */
	char* buff,		/* Pointer to the buffer */
	int len			/* Buffer length */
);

//ML84 added missing declaration
int xatoi (         /* 0:Failed, 1:Successful */
    char **str,     /* Pointer to pointer to the string */
    long *res       /* Pointer to the valiable to store the value */
);

static
void put_rc (FRESULT rc)
{
	const char *str =
		"OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
		"INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
		"INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0"
		"LOCKED\0" "NOT_ENOUGH_CORE\0" "TOO_MANY_OPEN_FILES\0";
	uint8_t txt[] = "0x00000000\0";
	FRESULT i;

	for (i = 0; i != rc && *str; i++) {
		while (*str++) ;
	}
	//iprintf("rc=%u FR_%s\n\r", (UINT)rc, str);
    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\r    rc=");
    uint_to_hexstr((UINT)rc, txt+2, 8);
    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " FR_");
    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) str);
    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\r");
}


static
FRESULT scan_files (char* path)
{
	DIR dirs;
	FRESULT res;
	BYTE i;


	if ((res = f_opendir(&dirs, path)) == FR_OK) {
		i = strlen(path);
		while (((res = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0]) {
			if (Finfo.fname[0] == '.') continue;
			if (Finfo.fattrib & AM_DIR) {
				acc_dirs++;
				*(path+i) = '/'; strcpy(path+i+1, &Finfo.fname[0]);
				res = scan_files(path);
				*(path+i) = '\0';
				if (res != FR_OK) break;
			} else {
				acc_files++;
				acc_size += Finfo.fsize;
			}
		}
	}

	return res;
}

int fatfs_ops(void)
{

	BYTE res;
	const BYTE ft[] = {0,12,16,32};
	char *ptr, *ptr2;
	uint8_t txt[32];
	long p1, p2;

//	WORD w1;
	UINT s1, s2, cnt;
	DIR dir;				/* Directory object */
	FIL file1, file2;		/* File object */
	FATFS *fs;
	char Line[80];				/* Console input buffer */
	BYTE Buff[512];				/* Working buffer */
	
	MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\r>");
	get_line(Line, sizeof(Line));

	ptr = Line;
	switch (*ptr++) {

	      case 't' :
	    	  set_fattime(ptr);
	          break;
	      case 'f' :
			switch (*ptr++) {

			case 's' :	/* fs - Show file system status */
				res = f_getfree("", (DWORD*)&p2, &fs);
				if (res) { put_rc(res); break; }
				//iprintf("FAT type = FAT%u\nBytes/Cluster = %lu\nNumber of FATs = %u\n"
				//		"Root DIR entries = %u\nSectors/FAT = %lu\nNumber of clusters = %lu\n"
				//		"FAT start (lba) = %lu\nDIR start (lba,clustor) = %lu\nData start (lba) = %lu\n\n...",
				//		ft[fs->fs_type & 3], fs->csize * 4096UL, fs->n_fats,
				//		fs->n_rootdir, fs->fsize, fs->n_fatent - 2,
				//		fs->fatbase, fs->dirbase, fs->database
				//);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "FAT type = FAT");
                memcpy(txt, "  \0", 3);
                uint_to_decstr(ft[fs->fs_type & 3], txt, 2);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\rBytes/Cluster = ");
                memcpy(txt, "       \0", 8);
                uint_to_decstr(fs->csize * 4096UL, txt, 2);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\rNumber of FATs = ");
                memcpy(txt, "   \0", 4);
                uint_to_decstr(fs->n_fats, txt, 3);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\rRoot DIR entries = ");
                memcpy(txt, "     \0", 6);
                uint_to_decstr(fs->n_rootdir, txt, 5);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\rSectors/FAT = ");
                memcpy(txt, "          \0", 11);
                uint_to_decstr(fs->fsize, txt, 10);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\rNumber of clusters = ");
                memcpy(txt, "          \0", 11);
                uint_to_decstr(fs->n_fatent-2, txt, 10);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\rFAT start (lba) = ");
                memcpy(txt, "          \0", 11);
                uint_to_decstr(fs->fatbase, txt, 10);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\rDIR start (lba,cluster) = ");
                memcpy(txt, "          \0", 11);
                uint_to_decstr(fs->dirbase, txt, 10);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\rData start (lba) = ");
                memcpy(txt, "          \0", 11);
                uint_to_decstr(fs->database, txt, 10);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

				acc_size = acc_files = acc_dirs = 0;
				res = scan_files(ptr);
				if (res) { put_rc(res); break; }
//				iprintf("\r%u files, %lu bytes.\n%u folders.\n"
//						"%lu bytes total disk space.\n%lu bytes available.\n",
//						acc_files, acc_size, acc_dirs,
//						(fs->n_fatent - 2) * fs->csize * 4096, p2 * fs->csize * 4096
//				);

                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\r");
                memcpy(txt, "     \0", 6);
                uint_to_decstr(acc_files, txt, 5);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " files, ");

                memcpy(txt, "          \0", 11);
                uint_to_decstr(acc_size, txt, 10);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " bytes.\n\r");

                memcpy(txt, "     \0", 6);
                uint_to_decstr(acc_dirs, txt, 5);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " folders.\n\r");

                memcpy(txt, "          \0", 11);
                uint_to_decstr((fs->n_fatent - 2) * fs->csize * 4096, txt, 10); //warning: could overflow 32 bits
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " bytes total disk space.\r\n");

                memcpy(txt, "          \0", 11);
                uint_to_decstr(p2 * fs->csize * 4096, txt, 10); //warning: could overflow 32 bits
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
				MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " bytes available.\n\r");
				break;

			case 'l' :	/* fl [<path>] - Directory listing */
				while (*ptr == ' ') ptr++;
				res = f_opendir(&dir, ptr);
				if (res) { put_rc(res); break; }
				p1 = s1 = s2 = 0;
				for(;;) {
					res = f_readdir(&dir, &Finfo);
					if ((res != FR_OK) || !Finfo.fname[0]) break;
					if (Finfo.fattrib & AM_DIR) {
						s2++;
					} else {
						s1++; p1 += Finfo.fsize;
					}
//					iprintf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\n\r",
//							(Finfo.fattrib & AM_DIR) ? 'D' : '-',
//							(Finfo.fattrib & AM_RDO) ? 'R' : '-',
//							(Finfo.fattrib & AM_HID) ? 'H' : '-',
//							(Finfo.fattrib & AM_SYS) ? 'S' : '-',
//							(Finfo.fattrib & AM_ARC) ? 'A' : '-',
//							(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
//							(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
//							Finfo.fsize, &(Finfo.fname[0]));

				     txt[0] = (Finfo.fattrib & AM_DIR) ? 'D' : '-';
				     txt[1] = (Finfo.fattrib & AM_RDO) ? 'R' : '-';
				     txt[2] = (Finfo.fattrib & AM_HID) ? 'H' : '-';
				     txt[3] = (Finfo.fattrib & AM_SYS) ? 'S' : '-';
				     txt[4] = (Finfo.fattrib & AM_ARC) ? 'A' : '-';
				     txt[5] = ' ';
				     txt[6] = '\0';
				     MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

				     memcpy(txt, "0000/00/00 \0", 12);
                     uint_to_decstr((Finfo.fdate >> 9) + 1980, txt, 4);
                     uint_to_decstr((Finfo.fdate >> 5) & 15,   txt+5, 2);
                     uint_to_decstr(Finfo.fdate & 31,          txt+8, 2);
                     MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

                     memcpy(txt, "00:00 \0", 7);
                     uint_to_decstr((Finfo.ftime >> 11), txt, 2);
                     uint_to_decstr((Finfo.ftime >> 5) & 63, txt+3, 2);
                     MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

                     memcpy(txt, "         \0", 10);
                     uint_to_decstr(Finfo.fsize, txt, 9);
                     MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
                     MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) Finfo.fname);

				}
				//iprintf("%4u File(s),%10lu bytes\n%4u Dir(s)", s1, p1, s2);
                memcpy(txt, "          \0", 11);
                uint_to_decstr(s1, txt, 10);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " File(s), ");
                uint_to_decstr(p1, txt, 10);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " bytes\n\r");
                uint_to_decstr(s2, txt, 10);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " Dir(s)");

				if (f_getfree(ptr, (DWORD*)&p1, &fs) == FR_OK) {
					//iprintf(", %10luK bytes free\n", p1 * fs->csize / 2);
	                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) ", ");
	                uint_to_decstr(p1 * fs->csize / 2, txt, 10);
	                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
	                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "K bytes free\n\r");
				}
				MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\r");
				break;

			case 'o' :	/* fo <file> - Open a file */
				while (*ptr == ' ') ptr++;
				res = f_open(&file1, ptr, FA_OPEN_EXISTING | FA_WRITE);
				if(res)
				{
					MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Creating a new file\r\n");
					res = f_open(&file1, ptr, FA_CREATE_ALWAYS | FA_WRITE);
					if (res)
					{
						MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Unable to create a file\r\n");
						return 0;
					}
				}
				else
				{
					MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Opening a existing file\r\n");
				}
				/* Logic to edit the file */
				MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Do you want to add the contents to the file? enter Y/N\r\n");
				//MSS_UART_polled_rx ( &g_mss_uart0, , 1 );
				get_line(Line, sizeof(Line));
				if ( (Line[0] == 'Y') || (Line[0] == 'y') )
				{
					MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Enter the content (Max 80 chars)at the end press enter key\r\n");
					get_line(Line, sizeof(Line));
					f_lseek(&file1, file1.fsize);
					
					f_puts(Line, &file1);

				}

				f_lseek(&file1, 0);					

				Buff[0] = '\0';
				for (;;)
				{

					res = f_read(&file1, Buff, sizeof(Buff), &s1);
					if (res || s1 == 0) break;   /* error or eof */
					//iprintf("%s",Buff);
					MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) Buff);
				}
				f_lseek(&file1, 0);
				res = f_close(&file1);
				put_rc(res);
				break;

			case 'c' :	/* fc - Close a file */
				res = f_close(&file1);
				put_rc(res);
				break;

			case 'e' :	/* fe - Seek file pointer */
				if (!xatoi(&ptr, &p1)) break;
				res = f_lseek(&file1, p1);
				put_rc(res);
				if (res == FR_OK)
					//iprintf("fptr = %lu(0x%lX)\n", file1.fptr, file1.fptr);
                    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "fptr = ");
                    memcpy(txt, "          \0", 11);
                    uint_to_decstr(file1.fptr, txt, 10);
                    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
                    memcpy(txt, "(0x00000000)\n\r\0", 15);
                    uint_to_hexstr(file1.fptr, txt+3, 8);
                    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
				break;

			case 'r' :	/* fr <len> - read file */
				if (!xatoi(&ptr, &p1)) break;
				p2 = 0;
				p1 =128;
				//Timer = 0;
				while (p1) {
					if (p1 >= sizeof(Buff))	{ cnt = sizeof(Buff); p1 -= sizeof(Buff); }
					else 			{ cnt = (WORD)p1; p1 = 0; }
					res = f_read(&file1, Buff, cnt, &s2);
					if (res != FR_OK) { put_rc(res); break; }
					p2 += s2;
					if (cnt != s2) break;
				}
				//s2 = Timer;
				//iprintf("%lu bytes read\n", p2);
                memcpy(txt, "          \0", 11);
                uint_to_decstr(p2, txt, 10);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " bytes read\n\r");
//				iprintf("%lu bytes read with %lu bytes/sec.\n", p2, p2 * 100 / s2);
				break;
			case 'v' :	/* fv - Truncate file */
				put_rc(f_truncate(&file1));
				break;

			case 'n' :	/* fn <old_name> <new_name> - Change file/dir name */
				while (*ptr == ' ') ptr++;
				ptr2 = strchr(ptr, ' ');
				if (!ptr2) break;
				*ptr2++ = 0;
				while (*ptr2 == ' ') ptr2++;
				put_rc(f_rename(ptr, ptr2));
				break;

			case 'u' :	/* fu <name> - Unlink a file or dir */
				while (*ptr == ' ') ptr++;
				put_rc(f_unlink(ptr));
				break;
#if _USE_MKFS
			case 'm' :	/* fm <partition rule> <sect/clust> - Create file system */
				//p2 = 0;
				//p3 = 2048;
				MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Formatting the SPI Flash..\r\n");
				put_rc(f_mkfs(0, 1, 2048)); //ML84 changed from MBR to SFD
				break;
#endif

			case 'x' : /* fx <src_name> <dst_name> - Copy file */
				while (*ptr == ' ') ptr++;
				ptr2 = strchr(ptr, ' ');
				if (!ptr2) break;
				*ptr2++ = 0;
				if (!*ptr2) break;
				//iprintf("Opening \"%s\"", ptr);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Opening \"");
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) ptr);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\"");

				res = f_open(&file1, ptr, FA_OPEN_EXISTING | FA_READ);
				if (res) {
					put_rc(res);
					break;
				}
				//iprintf("\nCreating \"%s\"", ptr2);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Creating \"");
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) ptr2);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\"");
				res = f_open(&file2, ptr2, FA_CREATE_ALWAYS | FA_WRITE);
				if (res) {
					put_rc(res);
					f_close(&file1);
					break;
				}
				MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\r\nCopying...");
				p1 = 0;
				for (;;) {
					res = f_read(&file1, Buff, sizeof(Buff), &s1);
					if (res || s1 == 0) break;   /* error or eof */
					res = f_write(&file2, Buff, s1, &s2);
					p1 += s2;
					if (res || s2 < s1) break;   /* error or disk full */
				}
				//iprintf("\n%lu bytes copied.\n", p1);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\r");
                memcpy(txt, "          \0", 11);
                uint_to_decstr(p1, txt, 10);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " bytes copied.\n\r");
				f_close(&file1);
				f_close(&file2);
				break;
			}
			break;
	}
	return 0;
}


