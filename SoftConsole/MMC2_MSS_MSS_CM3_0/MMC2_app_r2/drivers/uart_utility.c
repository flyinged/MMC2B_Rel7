/*
 * uart_utility.c
 *
 *  Created on: 02.10.2015
 *      Author: malatesta_a
 */

#include "mss_uart.h"
#include "uart_utility.h"
#include "mss_watchdog.h"

extern const uint32_t TOUT;
extern  uint32_t tick_counter;

void clear_console(uint8_t n) {
    uint8_t i;

    for (i=0; i<n; i++) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n");
    }
}

/************************************************************************************/
uint8_t key2continue(const char *message) {
    size_t rx_size;
    uint8_t uart_rx_buf[1];
    uint32_t tic;

    tic = tick_counter;
    rx_size = 0;
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) message);
    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r>>Press a key to continue\n\r");
    while (rx_size == 0) {
        MSS_WD_reload();
        rx_size = MSS_UART_get_rx ( &g_mss_uart0, uart_rx_buf, 1 );

        if ((tick_counter-tic) > TOUT) { //1 minute timeout
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rTIMEOUT while waiting for user input\n\r");
            return 0;
        }
    }
    return *uart_rx_buf;
}

/************************************************************************************/

//returns non terminated string
void uint_to_hexstr(uint32_t num, uint8_t *str, uint8_t ndigits) {
    uint8_t i;
    uint32_t buf;

    for (i=0; i<ndigits; i++) {
        buf = num >> (4*i);
        buf &= 0x0000000F;
        if (buf < 10) {
            str[ndigits-i-1] = buf+48;
        } else {
            str[ndigits-i-1] = buf+55;
        }
    }
    //str[ndigits] = '\0';

    return;
}

void uint_to_decstr(uint32_t num, uint8_t *str, uint8_t ndigits) {
    uint8_t i;
    uint32_t buf;

    buf = num;

    for (i=0; i<ndigits; i++) {
        str[ndigits-i-1] = (buf%10)+48;
        buf /= 10;
    }

    //str[ndigits] = '\0'; //null termination

    return;
}

//returns index of next-to-last char in string (always nd_int+nd_frac+2)
uint8_t float_to_string(float num, uint8_t *str, uint8_t nd_int, uint8_t nd_frac) {
    // s
    float fbuf;
    uint32_t ubuf;
    uint8_t i,j;

    //set sign
    i=0;

    if (num >= 0) {
        str[i] = '+';
        fbuf = num;

    } else {
        str[i] = '-';
        fbuf = -num;
    }

    //set integer part
    i++;

    ubuf = (uint32_t) fbuf; //integer part
    uint_to_decstr(ubuf, str+i, nd_int);

    i += nd_int;

    //set decimal point
    str[i] = '.';
    i++;

    //set fractional part
    fbuf -= ubuf; //remove integer part
    for(j=0; j<nd_frac; j++) fbuf *= 10;
    ubuf = (uint32_t) fbuf;
    uint_to_decstr(ubuf, str+i, nd_frac);

    i += nd_frac;

    return (i);
}

uint32_t hex_from_console(char* msg, uint8_t nmax){

    uint8_t i, step, inchar[9], rx_size;
    uint32_t outval;
    uint32_t tic;

    tic = tick_counter;

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
    //13=CR
    //48='0'

    //first get string
    step = 0;
    while(1){
        do {
            MSS_WD_reload();
            rx_size = MSS_UART_get_rx ( &g_mss_uart0, inchar+step, 1 );
            if ((tick_counter-tic) > TOUT) { //1 minute timeout
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rTIMEOUT while waiting for user input\n\r");
                return 0;
            }
        } while (rx_size == 0);

        if (inchar[step] == 13) { //check if termination character
            break;
        } else if (
                inchar[step]<48 ||
                (inchar[step]>57 && inchar[step]<65) ||
                (inchar[step]>70 && inchar[step]<97) ||
                inchar[step]>102 ) { //first check if character is valid
            step = 0;
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\r\nNot a valid hex digit\r\n");
        } else if (step == 8) { // && inchar[step] != 13) { //then check if string is too long
            step = 0;
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\r\nHex number is too big: maximum 32 bits allowed.\r\n");
        } else { //continue
            inchar[step+1] = 0;
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) inchar+step);
            step++;
        }

        if (step == nmax) break;
    }

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");

    //now process the value
    outval = 0;
    for (i = 0; i < step; i++) {
        switch (inchar[i]) {
        case 48 ... 57:
            outval += ( (inchar[i]-48) << ((step-i-1)*4) );
            break;
        case 65 ... 70:
            outval += ( (inchar[i]-55) << ((step-i-1)*4) );
            break;
        case 97 ... 102:
            outval += ( (inchar[i]-87) << ((step-i-1)*4) );
            break;
        }
    }

    return outval;
}


uint8_t str_from_console(uint8_t* msg, uint8_t *str){

    uint8_t i, rx_size;
    uint32_t tic;

    tic = tick_counter;

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) msg);
    //13=CR
    //48='0'

    //get string
    i = 0;
    while(1){
        do {
            MSS_WD_reload();
            rx_size = MSS_UART_get_rx ( &g_mss_uart0, str+i, 1 );
            if ((tick_counter-tic) > TOUT) { //1 minute timeout
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rTIMEOUT while waiting for user input\n\r");
                return 0;
            }
        } while (rx_size == 0);

        if (str[i] == 13) { //check if termination character
            MSS_UART_polled_tx( &g_mss_uart0, (uint8_t*)"\n\r", 2);
            break;
        } else {
            MSS_UART_polled_tx( &g_mss_uart0, str+i, 1);
            i++;
        }
    }
    str[i] = '\0';

    return(i);
}

/* receive NUM characters in buffer BUF[]
 * print each character after receiving it
 * timeout if time between characters is too big
 */
uint8_t uart_rx_timeout(uint8_t *buf, uint16_t num) {
    uint32_t tic;
    uint16_t i = 0;

    tic = tick_counter;

    while (i<num) {
        if (MSS_UART_get_rx ( &g_mss_uart0, buf+i, 1 ) ) {
            //got char
            MSS_UART_polled_tx( &g_mss_uart0, (const uint8_t *) buf+i, 1);
            i++;
        }

        MSS_WD_reload();
        if ((tick_counter-tic) > TOUT) {
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rUART TIMEOUT\n\r");
            return 1;
        }
    }
    return 0;
}

uint8_t string_compare(uint8_t *str1, uint8_t *str2) {
    uint32_t i = 0;

    while (str1[i] && str2[i]) {
        //both strings have valid chars, compare and increment index
        if (str1[i] != str2[i]) return 1; //difference found
        i++;
    }

    //found a termination char: if both terminated, strings are identical
    if (str1[i] == str1[i])
        return 0; //both terminated
    else return 1;
}


#define DBG_PRINT_EN

void dbg_print(char *msg) {
#ifdef DBG_PRINT_EN
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t*) msg);
#endif
}

void dbg_printnum(uint32_t num, uint8_t len) {
#ifdef DBG_PRINT_EN
    uint8_t txt[10],i;

    for(i=0; i<len; i++) {
        txt[i] = '0';
    }
    txt[len] = ' ';
    txt[len+1] = '\0';
    uint_to_hexstr(num,txt,len);
    MSS_UART_polled_tx_string( &g_mss_uart0, txt);
#endif
}

void dbg_printnum_d(uint32_t num, uint8_t len) {
#ifdef DBG_PRINT_EN
    uint8_t txt[16],i;

    for(i=0; i<len; i++) {
        txt[i] = '0';
    }
    txt[len] = ' ';
    txt[len+1] = '\0';
    uint_to_decstr(num,txt,len);
    MSS_UART_polled_tx_string( &g_mss_uart0, txt);
#endif
}
