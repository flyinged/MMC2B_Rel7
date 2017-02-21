/*
 * goldelox_port.c
 *
 *  Created on: 01.10.2015
 *      Author: malatesta_a
 */

#include "goldelox_port.h"
#include "mss_uart.h"
#include "uart_utility.h"
#include "GOLDELOX_SERIAL_4DLIBRARY.h"

#define CPU_FREQUENCY 100000000

uint8_t WriteFile(mss_uart_instance_t *uart, void *tx_buf, uint32_t ntowrite, uint32_t *nwritten, void *dummy) {
    MSS_UART_polled_tx(uart, (const uint8_t *) tx_buf, ntowrite);
    *nwritten = ntowrite;
    return(1);
}

uint8_t ReadFile(mss_uart_instance_t *uart, void *rx_buf, uint32_t ntoread, uint32_t *nread, void *dummy) {
    *nread = MSS_UART_get_rx( uart, rx_buf, ntoread );
    return(1);
}

extern volatile uint32_t tick_counter;

void oled_unlock(void) {

    //unlock OLED (in case UART TX line moved during system bring-up
    //send 0xFF and wait 10ms until a NAK is received
    do {
        MSS_UART_polled_tx_string( &g_mss_uart1, (unsigned char*)"\255");
        sleep(1); //10ms
        GetAck() ;
    } while(oled_error != Err4D_NAK && oled_error != Err4D_Timeout);
}


//ML84 modified: callback updated a global variable with command result. Errors handled in code.
static unsigned char *Error4DText[] = {(unsigned char*)"OK\0",(unsigned char*) "Timeout\0",(unsigned char*) "NAK\0", (unsigned char*)"Length\0", (unsigned char*)"Invalid\0", (unsigned char*)"UART\0"};
int Callback(int ErrCode, unsigned char Errbyte)
{
    uint8_t textbuf[] = "0x00\n\r\0";
    MSS_UART_polled_tx_string( &g_mss_uart0, (unsigned char*)"Serial 4D Library reports error: ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (unsigned char*)Error4DText[ErrCode]);
    if (ErrCode == Err4D_NAK) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (unsigned char*)" returned data = ");
        uint_to_hexstr((uint32_t)Errbyte, textbuf+2, 2);
        MSS_UART_polled_tx_string( &g_mss_uart0, textbuf);
    } else {
        MSS_UART_polled_tx_string( &g_mss_uart0, (unsigned char*)"\r\n") ;
    }
    oled_error = ErrCode;
    //exit(2) ;

    if (ErrCode) { // == Err4D_Timeout) { //ML84 added: disable display after timeout (display not present)
        oled_en = 0;
    }
    return 0 ; // to make compiler happy
}

uint8_t oled_char(uint16_t ox, uint16_t oy, char c, uint16_t color) {
    uint16_t i;
    const uint8_t bm0[] = {0,1, 0,2, 0,3, 0,4, 0,5, 0,6,
                           1,0, 1,7,
                           2,0, 2,7,
                           3,1, 3,2, 3,3, 3,4, 3,5, 3,6,
                           255};
    const uint8_t bm1[] = {0,0, 0,6,
                           1,0, 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7,
                           2,0,
                           255};
    const uint8_t bm2[] = {0,0, 0,1, 0,5, 0,6,
                           1,0, 1,2, 1,7,
                           2,0, 2,3, 2,7,
                           3,0, 3,4, 3,5, 3,6,
                           255};
    const uint8_t bm3[] = {0,1, 0,2, 0,5, 0,6,
                           1,0, 1,7,
                           2,0, 2,3, 2,4, 2,7,
                           3,1, 3,2, 3,5, 3,6,
                           255};
    const uint8_t bm4[] = {0,2, 0,3,
                           1,2, 1,4,
                           2,2, 2,5, 2,6,
                           3,0, 3,1, 3,2, 3,3, 3,4, 3,5, 3,6, 3,7,
                           4,2,
                           255};
    const uint8_t bm5[] = {0,1, 0,2, 0,4, 0,5, 0,6, 0,7,
                           1,0, 1,4, 1,7,
                           2,0, 2,4, 2,7,
                           3,1, 3,2, 3,3, 3,7,
                           255};
    const uint8_t bm6[] = {0,1, 0,2, 0,3, 0,4, 0,5, 0,6,
                           1,0, 1,4, 1,7,
                           2,0, 2,4, 2,7,
                           3,1, 3,2, 3,3, 3,6,
                           255};
    const uint8_t bm7[] = {0,0, 0,1, 0,6, 0,7,
                           1,2, 1,3, 1,7,
                           2,4, 2,7,
                           3,5, 3,6, 3,7,
                           255};
    const uint8_t bm8[] = {0,1, 0,2, 0,3, 0,5, 0,6,
                           1,0, 1,4, 1,7,
                           2,0, 2,4, 2,7,
                           3,1, 3,2, 3,3, 3,5, 3,6,
                           255};
    const uint8_t bm9[] = {0,1, 0,4, 0,5, 0,6,
                           1,0, 1,3, 1,7,
                           2,0, 2,3, 2,7,
                           3,1, 3,2, 3,3, 3,4, 3,5, 3,6,
                           255};
    const uint8_t bmdot[] = {1,0,255};
    const uint8_t *bm;
    uint8_t xoff;

    switch(c) {
        case '0':
            bm=bm0;
            xoff = 4+1;
            break;
        case '1':
            bm=bm1;
            xoff = 3+1;
            break;
        case '2':
            bm=bm2;
            xoff = 4+1;
            break;
        case '3':
            bm=bm3;
            xoff = 4+1;
            break;
        case '4':
            bm=bm4;
            xoff = 5+1;
            break;
        case '5':
            bm=bm5;
            xoff = 4+1;
            break;
        case '6':
            bm=bm6;
            xoff = 4+1;
            break;
        case '7':
            bm=bm7;
            xoff = 4+1;
            break;
        case '8':
            bm=bm8;
            xoff = 4+1;
            break;
        case '9':
            bm=bm9;
            xoff = 4+1;
            break;
        case '.':
            bm=bmdot;
            xoff = 3;
            break;
        case ' ':
            return(4);
        default:
            return(0);
    }

    i=0;
    do {
        gfx_PutPixel(ox+bm[i], oy+8-bm[i+1], color);
        i+=2;
    } while (bm[i] != 0xFF);

    return(xoff);
}

void oled_clear_rect(uint8_t xstart, uint8_t xend, uint8_t ystart, uint8_t yend) {
    uint8_t x,y;

    for (x=xstart; x<=xend; x++) {
        for (y=ystart; y<=yend; y++) {
            gfx_PutPixel(x, y, 0x0000);
        }
    }
}

void oled_putstr_small(uint8_t xoff, uint8_t yoff, char* ipstr, uint16_t color) {
    uint8_t i = 0, xo, yo;

    xo = xoff;
    yo = yoff;

    while(i<32 && ipstr[i] != 0) {
        xo += oled_char(xo, yo, ipstr[i], color);
        i++;
    }
}
