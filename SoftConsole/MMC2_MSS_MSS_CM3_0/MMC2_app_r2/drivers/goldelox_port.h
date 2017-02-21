/*
 * type_matching.h
 *
 *  Created on: 01.10.2015
 *      Author: malatesta_a
 */

#ifndef TYPE_MATCHING_H_
#define TYPE_MATCHING_H_

#include "hal.h"
#include "mss_uart.h"

#define HANDLE void *
#define WORD uint16_t
#define DWORD uint32_t

//ML84 modified: callback updated a global variable with command result. Errors handled in code.
volatile uint8_t oled_error;
volatile uint8_t oled_en, oled_contrast;

int Callback(int ErrCode, unsigned char Errbyte);
uint8_t WriteFile(mss_uart_instance_t *uart, void *tx_buf, uint32_t ntowrite, uint32_t *nwritten, void *dummy);
uint8_t ReadFile(mss_uart_instance_t *uart, void *rx_buf, uint32_t ntoread, uint32_t *nread, void *dummy);
uint32_t sleep(uint32_t seconds);
void oled_unlock(void);
void oled_clear_rect(uint8_t xstart, uint8_t xend, uint8_t ystart, uint8_t yend);
uint8_t oled_char(uint16_t ox, uint16_t oy, char c, uint16_t color);
void oled_putstr_small(uint8_t xoff, uint8_t yoff, char* ipstr, uint16_t color);

extern volatile uint32_t tick_counter;

#endif /* TYPE_MATCHING_H_ */
