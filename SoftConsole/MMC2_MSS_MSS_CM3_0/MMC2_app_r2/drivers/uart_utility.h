/*
 * uart_utility.h
 *
 *  Created on: 02.10.2015
 *      Author: malatesta_a
 */

#ifndef UART_UTILITY_H_
#define UART_UTILITY_H_

//#include "hal.h"
//#include "mss_uart.h"

void clear_console(uint8_t n);
uint8_t key2continue(const char *message);

//returns non terminated string
void uint_to_hexstr(uint32_t num, uint8_t *str, uint8_t ndigits);
void uint_to_decstr(uint32_t num, uint8_t *str, uint8_t ndigits);

//returns index of next-to-last char in string (always nd_int+nd_frac+2)
uint8_t float_to_string(float num, uint8_t *str, uint8_t nd_int, uint8_t nd_frac);
uint32_t hex_from_console(char* msg, uint8_t nmax);
uint8_t str_from_console(uint8_t* msg, uint8_t *str);
uint8_t uart_rx_timeout(uint8_t *buf, uint16_t num);
uint8_t string_compare(uint8_t *str1, uint8_t *str2);

void dbg_print(char *msg);
void dbg_printnum(uint32_t num, uint8_t len);
void dbg_printnum_d(uint32_t num, uint8_t len);

#endif /* UART_UTILITY_H_ */
