/*
 * uart_utility.h
 *
 *  Created on: 02.10.2015
 *      Author: malatesta_a
 */

#ifndef UART_UTILITY_H_
#define UART_UTILITY_H_

//#include "hal.h"
#include "mss_uart.h"

void clear_console(void);
void key2continue(const char *message);

//returns non terminated string
void uint_to_hexstr(uint32_t num, uint8_t *str, uint8_t ndigits);
void uint_to_decstr(uint32_t num, uint8_t *str, uint8_t ndigits);

//returns index of next-to-last char in string (always nd_int+nd_frac+2)
uint8_t float_to_string(float num, uint8_t *str, uint8_t nd_int, uint8_t nd_frac);
uint32_t hex_from_console(uint8_t* msg);

#endif /* UART_UTILITY_H_ */
