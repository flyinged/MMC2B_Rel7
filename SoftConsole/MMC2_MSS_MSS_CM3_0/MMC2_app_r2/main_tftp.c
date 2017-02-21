/*******************************************************************************
 * (c) Copyright 2009 Actel Corporation.  All rights reserved.
 *
 *  Application demo for Smartfusion
 *
 *
 * Author : Actel's Corporate Application Team
 * Rev    : 1.0.0.4
 *
 *******************************************************************************/

/**************************************************************************/
/* Standard Includes */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>

/**************************************************************************/
/* Firmware Includes */
/**************************************************************************/

#include "a2fxxxm3.h"

#include "main.h"
#include "mss_watchdog.h"
#include "mss_uart.h"
#include "mss_ethernet_mac.h"
#include "mss_rtc.h"
#include "mss_timer.h"

#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/ip.h"
#include "lwip/tcp.h"
#include "lwip/dhcp.h"
#include "netif/etharp.h"
#include "netif/loopif.h"

#include "ff.h"
#include "diskio.h"
#include "tftpd.h"

#include "goldelox_port.h"
#include "GOLDELOX_SERIAL_4DLIBRARY.h"

/**************************************************************************/
/* RTOS Includes */
/**************************************************************************/
extern void ethernetif_input( void * pvParameters );
extern void prvlwIPInit( void );

/**************************************************************************/
/* Preprocessor Macros*/
/**************************************************************************/
//static uint16_t System_ticks = 0; //use global

/* Simple periodic polling function (var=last saved time (updated by call), time=poll interval, function=executed at poll intervals*/
/*#define PERIODIC(var,time,function) \
                                     if((System_ticks - var) > time) \
                                     {                               \
                                        var += time;                 \
                                       function;                     \
                                     }                               \
*/

struct netif netif;
#if LWIP_HAVE_LOOPIF  
   struct netif netif_loop;
#endif

#define configCPU_CLOCK_HZ               ( ( unsigned long ) SystemFrequency )
#define configTICK_RATE_HZ               ( 100 ) //10 ms


#define portNVIC_SYSTICK_CTRL            ( ( volatile unsigned long *) 0xe000e010 )
#define portNVIC_SYSTICK_LOAD            ( ( volatile unsigned long *) 0xe000e014 )

#define SYS_TICK_CTRL_AND_STATUS_REG      0xE000E010
#define SYS_TICK_CONFIG_REG               0xE0042038
#define SYS_TICK_FCLK_DIV_32_NO_REF_CLK   0x31000000
#define ENABLE_SYS_TICK                   0x7

#define portNVIC_SYSTICK_CLK              0x00000004
#define portNVIC_SYSTICK_INT              0x00000002
#define portNVIC_SYSTICK_ENABLE           0x00000001

#define mainETH_TASK_PRIORITY            ( configMAX_PRIORITIES - 1)

/**************************************************************************/
/*Declaration of global variables*/
/**************************************************************************/
/**************************************************************************/
/*Extern Declarations*/
/**************************************************************************/
extern void init_mac();
extern void get_ip(char *ipStr, uint8_t network_mode);
void portlwIPInit( void );

/**************************************************************************/
/* Function to initialization all necessary hardware components for this*/
/* demo*/
/**************************************************************************/
FATFS fatfs;        /* File system object */

size_t UART_Polled_Rx(   mss_uart_instance_t * this_uart,   uint8_t * rx_buff,   size_t buff_size)
{
   size_t rx_size = 0U;
   while( rx_size < buff_size )
   {
      while ( this_uart->hw_reg_bit->LSR_DR != 0U  )
      {
         rx_buff[rx_size] = this_uart->hw_reg->RBR;
         ++rx_size;
      }
   }
   return rx_size;
}

//void Timer1_IRQHandler( void ) //why using Timer instead of Systick?
//{
//   System_ticks++;
//   /* Clear TIM1 interrupt */
//   MSS_TIM1_clear_irq();
//}


//Initializes HW (disable WD, init UART, get core clock frequency, init and start system tick)
#define SOFT_RST_CR 0xE0042030

void init_system() //just init timer
{
   uint32_t timer1_load_value;

   /* Disable the Watch Dog Timer */
//   MSS_WD_disable( );

   SystemCoreClockUpdate();
   /* Timer 1 for 10ms */
   timer1_load_value = g_FrequencyPCLK0/1000;

//   MSS_TIM1_init( MSS_TIMER_PERIODIC_MODE );
//   MSS_TIM1_load_immediate( timer1_load_value );
//   MSS_TIM1_start();
//   MSS_TIM1_enable_irq();
} 

//void Init_FS()
//{
//   long p2;
//   FATFS *fs; //pointer!
//   /* File System  on SPI Flash Init */
//   if(disk_initialize(0))
//   {
//       MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Disk Initialization Failed: SPI interface Error \n\r");
//   }
//   else
//   {
//      f_mount(0, &fatfs); //volume 0, global FS variable (init structure)
//      if(f_getfree("", (DWORD*)&p2, &fs)) //get number of free clusters, if result !=0, create FS
//      {
//         /* Create the File System */
//         //if (f_mkfs(0, 0, 2048))
//         if (f_mkfs(0, 1, 65536))
//         {
//             MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "File System Cannot be created \n\r");
//         }
//      }
//   }
//   MSS_RTC_init();
//   MSS_RTC_configure( MSS_RTC_NO_COUNTER_RESET );
//   MSS_RTC_start();
//}

//ML84 added missing function declarations
void show_ip(char *ipStr);


/****************************************************************/
/* Former main() function                             */
/****************************************************************/
extern volatile uint8_t oled_en, oled_contrast;
extern uint32_t oled_update_time;
extern unsigned char ip_known;

int tftp(uint8_t do_mac_init, uint8_t network_mode)
{
   char ipStr[20];

   MSS_WD_reload();
   if (do_mac_init) {
       init_mac();
   }

   if (oled_en) {
       oled_contrast = 15;
       gfx_Contrast(oled_contrast); //display on
       oled_update_time = tick_counter;
       txt_MoveCursor(0x0003, 0x0000); //start of line 4
       putstr((unsigned char*)"GET IP...\n") ;
   }

   get_ip(ipStr, network_mode);
   MSS_WD_reload();

   /* display IP */
   if (oled_en && ip_known) {
       //gfx_Contrast(15); //display on
       txt_MoveCursor(0x0003, 0x0000); //start of line 4
       gfx_RectangleFilled(0, 24, 96, 24+8, 0);
       if (ip_known) {
       oled_putstr_small(0, 3*8, ipStr, 0xFFFF);
       } else {
           putstr((unsigned char*)"NO IP\n") ;
       }
       oled_update_time = tick_counter;
   }

   if (!ip_known) {
       MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*)"tftp:: No IP available: TFTP Initialization failed\n\r");
       return 1;
   }

   

   portlwIPInit();

   MSS_WD_reload();
   if (tftpd_init())
   {
       MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*)"TFTP Initialization failed\n\r");
   }

   if (ip_known) {
       MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "TFTP ports are live with above IP address\n\r");
   }

//   do
//   {
//      MSS_WD_reload();
//      rx_size = MSS_UART_get_rx(&g_mss_uart0, &key, 1);
//      if (rx_size)
//      {
//          if (key == 'R') {
//              NVIC_SystemReset();
//          } else {
//              show_ip(ipStr);
//          }
//         //show_menu();
//         //rx_size = 0;
//      }
//
//      //PERIODIC(tcp_timer, TCP_TMR_INTERVAL, tcp_tmr());
//      PERIODIC(arp_timer, ARP_TMR_INTERVAL, etharp_tmr());
//      if(MSS_MAC_rx_pckt_size())
//      {
//         ethernetif_input(NULL);
//      }
//   } while( 1 );

   return 0;
}
