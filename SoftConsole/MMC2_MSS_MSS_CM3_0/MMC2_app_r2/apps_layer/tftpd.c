#include <stdio.h>
#include <string.h>

#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/udp.h"
#include "lwip/mem.h"

#include "tftpd.h"
#include "tftputils.h"

//ML84 added
#include "../main.h"
#include "goldelox_port.h"
#include "GOLDELOX_SERIAL_4DLIBRARY.h"
#include "mss_uart.h"
#include "uart_utility.h"
#include "MSS_watchdog.h"
#include "s25fl128s.h" //spi flash
#include "MMC2_hw_platform.h" //spi flash

#define MAX_FILE_SIZE 0x100000 //1 MB

/* ML84 added */
uint8_t f_write_raw (
    uint8_t fid,            /* file identifier */
    const void *buff,   /* Pointer to the data to be written */
    UINT btw,           /* Number of bytes to write */
    UINT block          /* block number */
);
uint8_t f_read_raw (
    uint8_t fid,        /* file identifier */
    void *buff,     /* Pointer to data buffer (max size 2+512) */
    UINT btr,       /* Number of bytes to read */
    UINT block,     /* Number of bytes to read */
    UINT *br        /* Pointer to number of bytes read */
);

uint32_t tftp_file_size = 0; //ML84
uint32_t tftp_file_crc  = 0; //ML84
uint8_t  tftp_file_id   = 0; //ML84
extern uint32_t compute_spi_crc(uint8_t index);

void rrq_recv_callback(void *_args,
                       struct udp_pcb *upcb,
                       struct pbuf *pkt_buf,
                       struct ip_addr *addr,
                       u16_t port);

int tftp_process_read(struct udp_pcb *upcb,
                       struct ip_addr *to,
                       int to_port,
                       char *fname);

tftp_connection_args gArgs;

/* tftp_errorcode error strings */
char *tftp_errorcode_string[] =
    {
        "not defined",
        "file not found in SPI Flash",
        "command not supported access violation",
        "check the SPI interface or disk full",
        "illegal operation",
        "unknown transfer id",
        "file already exists in SPI Flash",
        "no such user",
        "\n\rAllowed file names:\n\r  MMC_FW_0.dat (SPI 0x0)\n\r  MMC_FW_1.dat (SPI 0x100000)\n\r  MMC_SW_0.bin (GOLDEN, SPI 0x200000)\n\r  MMC_SW_1.bin (FALLBACK, SPI 0x300000)\n\r  TEST_FILE (dummy)\n\r  RESET CPU (causes power cycle)\n\r  FAN_RST\n\r  PWR_ON\n\r  PWR_OFF\n\r  IAP_ALL_0 (program FPGA with FW0)\n\r  IAP_ALL_1 (program FPGA with FW1)\n\r", //ML84
    };

err_t tftp_send_message (struct udp_pcb *upcb,
		                 struct ip_addr *to_ip,
		                 int to_port,
		                 char *buf,
		                 int buflen)
{

    err_t err;
    struct pbuf *pkt_buf;

    pkt_buf = pbuf_alloc(PBUF_TRANSPORT, buflen, PBUF_POOL);
    if (!pkt_buf)
    {
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "error allocating pkt buffer\n\r");
        return ERR_MEM;
    }
    // Copy the original data buffer over to the packet buffer's payload
    memcpy(pkt_buf->payload, buf, buflen);
    /* send message */
    err = udp_sendto(upcb, pkt_buf, to_ip, to_port);
    pbuf_free(pkt_buf);
    return err;
}

/* construct an error message into buf using err as the error code */
int tftp_construct_error_message(char *buf,
		                         tftp_errorcode err)
{
    int errorlen;

    tftp_set_opcode(buf, TFTP_ERROR);
    tftp_set_errorcode(buf, err);

    tftp_set_errormsg(buf, tftp_errorcode_string[err]);
    errorlen = strlen(tftp_errorcode_string[err]);

    /* return message size */
    return 4 + errorlen + 1;
}

/* construct and send an error message back to client */
int tftp_send_error_message(struct udp_pcb *upcb,
		                    struct ip_addr *to,
		                    int to_port,
		                    tftp_errorcode err)
{
    char buf[512];
    int n;

    n = tftp_construct_error_message(buf, err);
    return tftp_send_message(upcb, to, to_port, buf, n);
}

/* construct and send a data packet */
int tftp_send_data_packet(struct udp_pcb *upcb,
		                  struct ip_addr *to,
		                  int to_port,
		                  int block,
		                  char *buf,
		                  int buflen)
{
    char packet[TFTP_DATA_PKT_LEN_MAX];
  
    tftp_set_opcode(packet, TFTP_DATA);
    tftp_set_block(packet, block);
    tftp_set_data_message(packet, buf, buflen);

    return tftp_send_message(upcb, to, to_port, packet, buflen + 4);
}

int tftp_send_ack_packet(struct udp_pcb *upcb,
		                 struct ip_addr *to,
		                 int to_port,
		                 int block)
{
    char packet[TFTP_ACK_PKT_LEN]; // create the maximum possible size packet that a TFTP ACK packet can be

    tftp_set_opcode(packet, TFTP_ACK); // define the first two bytes of the packet
    tftp_set_block(packet, block);
    return tftp_send_message(upcb, to, to_port, packet, TFTP_ACK_PKT_LEN);
}

void tftp_cleanup(struct udp_pcb *upcb,
		          tftp_connection_args *args)
{
    /* cleanup the args */
    //f_close(&args->fd); //ML84

    /* close the connection */
    udp_remove(upcb);
}

/* 1. get data from file. Read into args->data, full buffer
 *
 */
void tftp_send_next_block(struct udp_pcb *upcb,
		                  tftp_connection_args *args,
                          struct ip_addr *to_ip,
                          u16_t to_port)
{
    int res, s2;
    //uint8_t txt[] = "00 )\n\r\0";

    res = f_read_raw(args->fid, args->data, 512, args->block, (unsigned int *)&s2);
    args->block++;

    if(res)
    {
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Can not read from file\n\r");
		/* we are done */
		return tftp_cleanup(upcb, args);
    }

    /* send the data */
    //even in s2=0, send an empty packet to signal the end of transfer
    tftp_send_data_packet(upcb, to_ip, to_port, args->block, args->data, s2); // args->data_len);

    //MSS_UART_polled_tx(&g_mss_uart0, (uint8_t*) "TXD\n\r", 5); //xMYDEBUG

    if (s2 == 0)
    {
        //iprintf("closing connection, ret = %d\n\r", args->data_len);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "    closing connection\n\r");
        args->data_len = 0; //notify upper layer
        /* we are done */
        return tftp_cleanup(upcb, args);
    }

    return;
}



/* write callback */
void wrq_recv_callback(void *_args,
		               struct udp_pcb *upcb,
		               struct pbuf *pkt_buf,
		               struct ip_addr *addr,
		               u16_t port)
{
    unsigned int res; //, s2;
    int flag_connect = 0;
    tftp_connection_args *args = (tftp_connection_args *)_args;

    uint32_t flash_addr;
    uint8_t  flash_buf[8];

    // Does this packet have any valid data to write? (length > data packet header length)
    if ((pkt_buf->len > TFTP_DATA_PKT_HDR_LEN) &&
        (tftp_extract_block(pkt_buf->payload) == (args->block + 1))) //second word, byte swapped (BLOCK NUMBER == next expected block)
    {
        //xTodo f_seek
        //res = f_write(&(args->fd), (char *)pkt_buf->payload+TFTP_DATA_PKT_HDR_LEN, pkt_buf->tot_len-TFTP_DATA_PKT_HDR_LEN, &s2);
        res = f_write_raw(args->fid, (char *)pkt_buf->payload+TFTP_DATA_PKT_HDR_LEN, pkt_buf->tot_len-TFTP_DATA_PKT_HDR_LEN, args->block); //ML84
	    if (res)
	    {
	        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "write to file error\n\r");
	        tftp_send_error_message(upcb, addr, port, TFTP_ERR_FILE_NOT_FOUND);
	        pbuf_free(pkt_buf);
	        return tftp_cleanup(upcb, args);
        }
        //iprintf("%s",(char *)pkt_buf->payload+TFTP_DATA_PKT_HDR_LEN);
        args->block++;
        (args->tot_bytes)+=(pkt_buf->tot_len-TFTP_DATA_PKT_HDR_LEN);
        flag_connect = 1;

    }
#if 1 //ML84: enabled (was disabled, didn't ack correctly if last packet was of size 0
    else if(tftp_extract_block(pkt_buf->payload) == (args->block + 1))
    {
        args->block++;
    }
#endif

    tftp_send_ack_packet(upcb, addr, port, args->block);


    if( (pkt_buf->tot_len < TFTP_DATA_PKT_LEN_MAX) &&
    		(flag_connect))
    {
        pbuf_free(pkt_buf);
	
	    //res = f_close(&(args->fd)); ML84
//        res = 0;
//	    if(res){
//	        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Can't close the file\n\r");
//	    }

        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Computing CRC...\n\r");
        //write file size and crc to flash
        //write status and CRC
        flash_addr = 0xF00000 + (tftp_file_id*S25FL256_SECTOR_SIZE);
        FLASH_erase_sector(flash_addr); // added
        //write expected crc to flash
        flash_buf[0] = 0xDD; //file good
        flash_buf[1] = 0xDD;
        flash_buf[2] = 0xDD;
        flash_buf[3] = 0xDD;
        flash_buf[4] = (tftp_file_size>>24) & 0xFF; //file size
        flash_buf[5] = (tftp_file_size>>16) & 0xFF;
        flash_buf[6] = (tftp_file_size>> 8) & 0xFF;
        flash_buf[7] = (tftp_file_size    ) & 0xFF;
        FLASH_program(flash_addr, flash_buf, 8); //write status and size (compute_spi_crc function reads size from FLASH)
        tftp_file_crc =  compute_spi_crc(tftp_file_id);
        flash_buf[0] = (tftp_file_crc>>24) & 0xFF; //file crc
        flash_buf[1] = (tftp_file_crc>>16) & 0xFF;
        flash_buf[2] = (tftp_file_crc>> 8) & 0xFF;
        flash_buf[3] = (tftp_file_crc    ) & 0xFF;
        FLASH_program(flash_addr+8, flash_buf, 4); //write crc
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "DONE\n\r");

        return tftp_cleanup(upcb, args);
    }
    else
    {
        pbuf_free(pkt_buf);
        return;
    }
}

int tftp_process_write(struct udp_pcb *upcb,
		               struct ip_addr *to,
		               int to_port,
		               char *fname)
{
    //FIL fd; ML84
    unsigned int res, reg32;
    tftp_connection_args *args;
    args = &gArgs;

    //ML84 replaced FOPEN with a filename check
    //res = f_open(&fd,fname, FA_OPEN_EXISTING);
    /* TFTP SUPPORTED WRITE FILES */
    res = 0;
    if (strcmp(fname, "TEST_FILE") == 0) {
        args->fid = 42;
    } else if (strcmp(fname, "RESET")        == 0) {
//        reg32 = HW_get_32bit_reg(CPU2FABRIC_REG);
//        HW_set_32bit_reg(CPU2FABRIC_REG, (reg32|C2F_REG_SHUTDOWN));
//        MSS_TIM1_disable_irq();
//        MSS_I2C_disable_slave( &g_mss_i2c0 );
        sleep(1000);
        NVIC_SystemReset();
    } else if (strcmp(fname, "PWR_OFF")      == 0) {
        //reg32 = *(volatile unsigned int*)(CPU2FABRIC_REG);
        //*(volatile unsigned int*)(CPU2FABRIC_REG) = (reg32|C2F_REG_SHUTDOWN);
        reg32 = *(volatile unsigned int*)(CPU2FABRIC_REG2);
        *(volatile unsigned int*)(CPU2FABRIC_REG2) = (reg32|C2F_REG_SHUTDOWN);
        if (oled_en) {
            txt_FGcolour(0xF800); //red rgb565
            txt_MoveCursor(0x0006,0x0000);
            putstr((unsigned char*)"SHUTDOWN\n");
        }
        args->fid = 255;
    } else if (strcmp(fname, "PWR_ON")      == 0) {
        //reg32 = *(volatile unsigned int*)(CPU2FABRIC_REG);
        //*(volatile unsigned int*)(CPU2FABRIC_REG) = (reg32&(~C2F_REG_SHUTDOWN));
        reg32 = *(volatile unsigned int*)(CPU2FABRIC_REG2);
        *(volatile unsigned int*)(CPU2FABRIC_REG2) = (reg32&(~C2F_REG_SHUTDOWN));
        if (oled_en) {
            txt_MoveCursor(0x0006,0x0000);
            putstr((unsigned char*)"        \n");
        }
        args->fid = 255;
    } else if (strcmp(fname, "FAN_RST")      == 0) {
        *(volatile uint32_t*)(MSS_I2C_CONTROLLER_0+0x000001FC) = 0xFF000000;
        args->fid = 255;
    } else if (strcmp(fname, "IAP_ALL_0")    == 0) {
        *(volatile uint32_t*)(MBU_MMC_V2B_APB_0) = 0x35000000; //'5'
//        reg32 = HW_get_32bit_reg(CPU2FABRIC_REG);
//        HW_set_32bit_reg(CPU2FABRIC_REG, (reg32|C2F_REG_SHUTDOWN));
        reg32 = HW_get_32bit_reg(CPU2FABRIC_REG2);
        HW_set_32bit_reg(CPU2FABRIC_REG2, (reg32|C2F_REG_SHUTDOWN));
        MSS_TIM1_disable_irq();
        MSS_I2C_disable_slave( &g_mss_i2c0 );
        sleep(1000);
        NVIC_SystemReset();
    } else if (strcmp(fname, "IAP_ALL_1")    == 0) {
        *(volatile uint32_t*)(MBU_MMC_V2B_APB_0) = 0x36000000; //'6'
//        reg32 = HW_get_32bit_reg(CPU2FABRIC_REG);
//        HW_set_32bit_reg(CPU2FABRIC_REG, (reg32|C2F_REG_SHUTDOWN));
        reg32 = HW_get_32bit_reg(CPU2FABRIC_REG2);
        HW_set_32bit_reg(CPU2FABRIC_REG2, (reg32|C2F_REG_SHUTDOWN));
        MSS_TIM1_disable_irq();
        MSS_I2C_disable_slave( &g_mss_i2c0 );
        sleep(1000);
        NVIC_SystemReset();
    } else if (strcmp(fname, "MMC_FW_0.dat") == 0) {
        args->fid = 0;
    } else if (strcmp(fname, "MMC_FW_1.dat") == 0) {
        args->fid = 1;
    } else if (strcmp(fname, "MMC_SW_0.bin") == 0) {
        args->fid = 2;
    } else if (strcmp(fname, "MMC_SW_1.bin") == 0) {
        args->fid = 3;
    } else {
        res = 1;
    }

    //f_close(&fd); ML84
    if(!res) //OK
    {
        //f_unlink(fname); ML84
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\nFilename ");
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) fname);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " supported\n\r");
    }
    //res = f_open(&fd, fname, FA_CREATE_ALWAYS | FA_WRITE);

    if (res)
    {
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Filename ");
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) fname);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " not supported for write\n\r");
        //tftp_send_error_message(upcb, to, to_port, TFTP_ERR_DISKFULL); ML84
        tftp_send_error_message(upcb, to, to_port, TFTP_ERR_WRONG_FILENAME);
        udp_remove(upcb);
        return 1;
    }
    args->op = TFTP_WRQ;
    //args->fd = fd; ML84
    args->to_ip.addr = to->addr;
    args->to_port = to_port;
    args->block = 0;
    args->tot_bytes = 0;
    args->data_len = 600;

    /* set callback for receives on this UDP PCB (Protocol Control Block) */
    udp_recv(upcb, wrq_recv_callback, args);

    /* initiate the write transaction by sending the first ack */
    tftp_send_ack_packet(upcb, to, to_port, args->block);

    return 0;
}

void process_tftp_request(struct pbuf *pkt_buf,
		                  struct ip_addr *addr,
		                  u16_t port)
{
    tftp_opcode op = tftp_decode_op(pkt_buf->payload); //get LSB from opcode
    char fname[512];
    uint8_t txt[16];
    struct udp_pcb *upcb, *pcb_del;
    err_t err;

    /* create new UDP PCB structure */
    //First scan all PCB. Destroy old transaction PCBs. Keep only the one on port 69 (used to get requests)
    memcpy(txt, "00000\n\r\0", 8);
    for (upcb = udp_pcbs; upcb != NULL; ) {

        if (upcb->local_port != 69) {
            uint_to_decstr(upcb->local_port, txt, 5);
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "TFTP: removing PCB on port ");
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

            pcb_del = upcb; //store address of PCB to delete
            upcb    = upcb->next; //go to next
            udp_remove(pcb_del); //remove PCB
        } else {
            upcb    = upcb->next; //go to next
        }
    }

    upcb = udp_new();
    if (!upcb)
    {
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "PROC_TFTP_REQ: Error creating PCB. Out of Memory\n\r");
        return;
    }

    /* bind to port 0 to receive next available free port */
    // NOTE!  This is how TFTP works.  There is a UDP PCB for the standard port
    // 69 which all transactions begin communication on, however, _all_ subsequent
    // transactions for a given "stream" occur on another port!
    err = udp_bind(upcb, IP_ADDR_ANY, 0);
    if (err != ERR_OK)
    {
        //iprintf("Unable to bind to port %d: err = %d\n\r", port, err);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Unable to bind to port ");
        memcpy(txt, "     \0", 6);
        uint_to_decstr(port, txt, 5);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) ": err = ");
        memcpy(txt, "   \0", 4);
        uint_to_decstr(err, txt, 3);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\r");
        return;
    }

    switch (op)
    {
        case TFTP_WRQ:
            tftp_extract_filename(fname, pkt_buf->payload); //string starting from byte 2

            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\rTFTP: GOT WRITE REQUEST. File: ");
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) fname);
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\r");

            tftp_process_write(upcb, addr, port, fname);
            break;

        case TFTP_RRQ:
            tftp_extract_filename(fname, pkt_buf->payload); //string starting from byte 2

            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\rTFTP: GOT READ REQUEST. File: ");
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) fname);
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\r");

            tftp_process_read(upcb, addr, port, fname);
            break;

        default:
            /* send a generic access violation message */
            tftp_send_error_message(upcb, addr, port, TFTP_ERR_ACCESS_VIOLATION);

            //iprintf("TFTP unknown request op: %d\n", op);
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "TFTP unsupported request op: ");
            memcpy(txt, " \n\r\0", 4);
            uint_to_decstr(op, txt, 1);
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);

            udp_remove(upcb); // no need to use tftp_cleanup because no "tftp_connection_args" struct has been malloc'd
        break;
    }
    return;
}

void recv_callback(void *arg,
		           struct udp_pcb *upcb,
		           struct pbuf *pkt_buf,
		           struct ip_addr *addr,
		           u16_t port)
{
    /* process new connection request */
    process_tftp_request(pkt_buf, addr, port);

    pbuf_free(pkt_buf);
}

int tftpd_init()
{
    struct udp_pcb *upcb;
    err_t err;
    unsigned port = 69;
    uint8_t txt[8];

    /* create new UDP PCB structure */
    upcb = udp_new();
    if (!upcb)
    {
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "TFTPD_INIT: Error creating PCB. Out of Memory\n\r");
        return -1;
    }

    /* Bind this PCB to port 69 */
    err = udp_bind(upcb, IP_ADDR_ANY, port);
    if (err != ERR_OK)
    {
        //iprintf("Unable to bind to port %d: err = %d\n\r", port, err);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Unable to bind to port ");
        memcpy(txt, "     \0", 6);
        uint_to_decstr(port, txt, 5);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) ": err = ");
        memcpy(txt, "   \0", 4);
        uint_to_decstr(err, txt, 3);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) txt);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\r");
        return -2;
    }

    udp_recv(upcb, recv_callback, NULL);

    return 0;
}

/* ML84 added */
/*
 * Absolute offset in SPI flash depends on FID (0x100000*FID)
 * File offset = 0x100000 (1MB maximum file size)
 * Block offset = 512 (TFTP spec)
 */
uint8_t f_write_raw (
    uint8_t fid,        /* file identifier */
    const void *buff,   /* Pointer to the data to be written */
    UINT btw,           /* Number of bytes to write */
    UINT block          /* block number */
    )
{
    uint32_t waddr, ii;
    uint8_t *u8buf;
    //uint8_t text_buf[16];
    uint16_t nsectors;

    u8buf = (uint8_t*) buff;

    if (fid == 42) { //dummy file
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Storing into the NOTHING box...\n\r");
        return 0;
    } else if (fid == 255) {
        //command: do nothing
        return 0;
    }

    waddr = (MAX_FILE_SIZE*fid) + (512*block); //start offset + file_offset

    //ML84 ADDED: erase only sectors to be written
    if (block == 0) {
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Erasing SPI FLASH sector");
        nsectors = (0x100000/S25FL256_SECTOR_SIZE);
        //if (nsectors>512) nsectors = 512;
        for (ii=0; ii<nsectors; ii++) {
            MSS_UART_polled_tx(&g_mss_uart0, (uint8_t*) '.', 1);
            FLASH_erase_sector(waddr+(ii*S25FL256_SECTOR_SIZE));
        }
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\n\rProgramming\n\r");
    }

    FLASH_program(waddr, (uint8_t*) buff, 512);

    if (block == 0) {
        FLASH_erase_sector(0xF00000+(fid*S25FL256_SECTOR_SIZE));
        tftp_file_size = 512;
        tftp_file_id   = fid;
    } else {
        tftp_file_size += 512;
    }

#if 0
    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Writing fid");
    text_buf[0] = fid+48;
    MSS_UART_polled_tx(&g_mss_uart0, (uint8_t*) text_buf, 1);

    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) ", block ");
    memcpy(text_buf, "000 \0", 5); uint_to_decstr(block, text_buf, 3);
    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) text_buf);

    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) ", address ");
    memcpy(text_buf, "0x00000000\n\r\0", 13); uint_to_hexstr(waddr, text_buf+2, 8);
    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) text_buf);
#endif

    return 0; //handle return codes
}

uint8_t f_read_raw (
    uint8_t fid,        /* file identifier */
    void *buff,     /* Pointer to data buffer (max size 2+512) */
    UINT btr,       /* Number of bytes to read */
    UINT block,     /* Number of bytes to read */
    UINT *br        /* Pointer to number of bytes read */
)
{
    uint32_t base, offset;
    uint8_t *u8buf;


    //return if trying to read past the end of file (size fixed to 1MB)
    if (block >= (MAX_FILE_SIZE/512)) {
        *br = 0;
        return 0;
    }

    u8buf = (uint8_t*) buff;

    if (fid == 42) {
        if (block<200) { //TEST_FILE= 100KB
            memset(u8buf, block, 512);
            *br = 512;
        } else {
            *br = 0;
        }
        return 0;
    }

    base   = (MAX_FILE_SIZE*fid);
    offset = (512*block);

    MSS_WD_reload();

    if (offset+btr > MAX_FILE_SIZE) { //requested too many bytes from valid offset
        FLASH_read((base+offset), u8buf, (MAX_FILE_SIZE-offset));
        *br = (MAX_FILE_SIZE-offset);
    } else {
        FLASH_read((base+offset), u8buf, btr);
        *br = btr;
    }

    MSS_WD_reload();

    return 0;
}

int tftp_process_read(struct udp_pcb *upcb,
                       struct ip_addr *to,
                       int to_port,
                       char *fname)
{
    unsigned int res;
    tftp_connection_args *args;
    args = &gArgs;

    res = 0;
    if        (strcmp(fname, "TEST_FILE")    == 0) {
        args->fid = 42;
    } else if (strcmp(fname, "RESET")        == 0) {
//        res = HW_get_32bit_reg(CPU2FABRIC_REG);
//        HW_set_32bit_reg(CPU2FABRIC_REG, (res|C2F_REG_SHUTDOWN));
//        MSS_TIM1_disable_irq();
//        MSS_I2C_disable_slave( &g_mss_i2c0 );
        sleep(1000);
        NVIC_SystemReset();
    } else if (strcmp(fname, "MMC_FW_0.dat") == 0) {
        args->fid = 0;
    } else if (strcmp(fname, "MMC_FW_1.dat") == 0) {
        args->fid = 1;
    } else if (strcmp(fname, "MMC_SW_0.bin") == 0) {
        args->fid = 2;
    } else if (strcmp(fname, "MMC_SW_1.bin") == 0) {
        args->fid = 3;
    } else {
        res = 1;
    }

    if(!res) //OK
    {
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "\nFilename ");
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) fname);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " supported\n\r");
    }

    if (res)
    {
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Filename ");
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) fname);
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " not supported for read\n\r");
        tftp_send_error_message(upcb, to, to_port, TFTP_ERR_WRONG_FILENAME);
        udp_remove(upcb);
        return 1;
    }

    args->op = TFTP_RRQ;
    args->to_ip.addr = to->addr;
    args->to_port = to_port;
    args->block = 0;
    args->tot_bytes = 0; //total amount transferred (file pointer)
    args->data_len = 512; //packet size

    /* set callback for receives on this UDP PCB (Protocol Control Block) */
    udp_recv(upcb, rrq_recv_callback, args);

    /* initiate the read transaction by sending the first data block */
    tftp_send_next_block(upcb, args, to, to_port);

    return 0;
}

/* read callback */
/* wait for ACK, then send next block */
void rrq_recv_callback(void *_args,
                       struct udp_pcb *upcb,
                       struct pbuf *pkt_buf,
                       struct ip_addr *addr,
                       u16_t port)
{
    //unsigned int res; //, s2;
    //uint8_t txt[] = "00\n\r\0";
    //int flag_connect = 0;
    tftp_connection_args *args = (tftp_connection_args *)_args;

    // if correct ack, send next block
    /* first make sure this is a data ACK packet */
    if (tftp_decode_op(pkt_buf->payload) != TFTP_ACK) {
        //MSS_UART_polled_tx(&g_mss_uart0, (uint8_t*) "RX_NA\n\r", 5); //xMYDEBUG
        return;
    }

    //MSS_UART_polled_tx(&g_mss_uart0, (uint8_t*) "RXA\n\r", 5); //xMYDEBUG

    /* then get block number from ACK packet */
    args->block = tftp_extract_block(pkt_buf->payload);

    tftp_send_next_block(upcb, args, addr, port);
    //flag_connect = 1;

    if( args->data_len == 0)
    {
        pbuf_free(pkt_buf);
        return tftp_cleanup(upcb, args);
    }
    else
    {
        pbuf_free(pkt_buf);
        return;
    }
}
