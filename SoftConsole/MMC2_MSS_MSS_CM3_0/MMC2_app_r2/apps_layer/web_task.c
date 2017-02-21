/**************************************************************************
 * (c) Copyright 2009 Actel Corporation.  All rights reserved.
 *
 *  Application demo for Smartfusion
 *
 *
 * Author : Actel Application Team
 * Rev     : 1.0.0.3
 *
 **************************************************************************/

/**************************************************************************/
/* Standard Includes */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**************************************************************************/
/* Driver Includes */
/**************************************************************************/

#include "../drivers/mss_ethernet_mac/mss_ethernet_mac.h"
#include "../BSP/mac/tcpip.h"
#include "conf_eth.h"

#include "ff.h"
#include "lwip/tcp.h"

#include "cpu_types.h"
//#include "webpages.h"
#include "mss_uart.h"
#include "mss_watchdog.h"
#include "uart_utility.h"
#include "ethernet.h"

/**************************************************************************/
/* RTOS Includes */
/**************************************************************************/


/**************************************************************************/
/*Web Server Includes */
/**************************************************************************/

/**************************************************************************/
/* Definitions for Ethernet test */
/**************************************************************************/

#define OPEN_IP
#define BUF                       ((struct uip_eth_hdr *)&uip_buf[0])
#ifndef DEFAULT_NETMASK0
#define DEFAULT_NETMASK0          255
#endif

#ifndef DEFAULT_NETMASK1
#define DEFAULT_NETMASK1          255
#endif

#ifndef DEFAULT_NETMASK2
#define DEFAULT_NETMASK2          0
#endif

#ifndef DEFAULT_NETMASK3
#define DEFAULT_NETMASK3          0
#endif
#define TCP_PKT_SIZE              1600
#define IP_ADDR_LEN               4
#define PHY_ADDRESS               1
#define DHCP_ATTEMPTS             4
#define USER_RX_BUFF_SIZE         1600

/**************************************************************************/
/* Extern Declarations */
/**************************************************************************/
//extern unsigned char              my_ip[4];
extern unsigned int               num_pkt_rx;
extern unsigned char              ip_known;
extern unsigned char              my_ip[IP_ADDR_LEN];
extern unsigned char              tcp_packet[TCP_PKT_SIZE];
extern unsigned char              dhcp_ip_found;

extern unsigned char              dhcp_s; //ML84 added
extern unsigned char              dhcp_last_msg; //ML84 added

extern float                            real_voltage_value;
extern float                            real_current_value;
extern float                            real_temperature_value;
extern float                            real_temperature_value_tc;
extern float                            real_temperature_value_tk;
extern float                            real_temperature_value_tf;

volatile unsigned char            oled_on_irq = 0;
volatile unsigned char            sw1_menu_scroll = 0;
volatile unsigned char            sw2_select = 0;
volatile unsigned char            led_on = 0;
volatile unsigned char            analog_mode = 0;
volatile unsigned char            hyper_teminal_on = 0;
volatile unsigned char            webServerFlag = 0;

unsigned char appdata[5120];

char ethAddr[6] = {0xaa,0xbb,0xcc,0x66,0x55,0x44};
uint8_t                                     PRINT_MENU_var = 1;

/**************************************************************************/
/* Function to show IP address on OLED and UART */
/**************************************************************************/

void show_ip(char *ipStr)
{
    //char *textStr;
    unsigned char i,j; //ML84

    //MSS_UART_polled_tx_string(&g_mss_uart0, (const uint8_t*) "Successfully requested IP addr \n\r" );

//    if((my_ip[0] == 192) &&
//       (my_ip[1]==168)   &&
//       (my_ip[2]==0)     &&
//       (my_ip[3]==14))
//    {
//        MSS_UART_polled_tx_string(&g_mss_uart0, (const uint8_t*) "Static IP address: " );
//        //textStr = "Browse Static IP:";
//    }
//    else
//    {
//        MSS_UART_polled_tx_string(&g_mss_uart0, (const uint8_t*) "Dynamic IP address: " );
//        //textStr = "Browse Dynamic IP:";
//    }
    //ML84 replace printf
    //printf( "%d.%d.%d.%d \n\r", my_ip[0], my_ip[1], my_ip[2], my_ip[3] );
    MSS_UART_polled_tx_string(&g_mss_uart0, (const uint8_t*) "IP address: " );
    j=0;
    for (i=0; i<4; i++) {
        if (my_ip[i]<10) {
            uint_to_decstr(my_ip[i],(uint8_t*) ipStr+j,1);
            j+=1;
            ipStr[j]='.';
            j++;
        } else if (my_ip[i]<100) {
            uint_to_decstr(my_ip[i],(uint8_t*) ipStr+j,2);
            j+=2;
            ipStr[j]='.';
            j++;
        } else {
            uint_to_decstr(my_ip[i],(uint8_t*) ipStr+j,3);
            j+=3;
            ipStr[j]='.';
            j++;
        }
    }
    j--; //remove last dot
    memcpy(ipStr+j, "     \0", 6);
    MSS_UART_polled_tx_string(&g_mss_uart0, (const uint8_t*) ipStr);
    MSS_UART_polled_tx_string(&g_mss_uart0, (const uint8_t*) "\n\r");

    //    sprintf(ipStr,"%d.%d.%d.%d     ",my_ip[0], my_ip[1], my_ip[2], my_ip[3]);

    //    menu_show(textStr, ipStr);
}

/**************************************************************************/
/* Function to Initialize the MAC, setting the MAC address and */
/* fetches the IP address */
/**************************************************************************/

extern volatile uint16_t tick_counter;
extern unsigned char my_mac[6]; //ML84
extern uint32_t sleep(uint32_t milliseconds);

/* ML84 Replace init_mac() with two separate functions, one just to init the MAC core, the other for DHCP */

void init_mac(void)
{
    uint32_t mac_cfg;
    int32_t i;
    uint8_t txt[]="0x00000000\n\r\0";
    unsigned char MacAddress[6];

    /* Default MAC addr. */
    MacAddress[0] = my_mac[0]; //ETHERNET_CONF_ETHADDR0;//ML84
    MacAddress[1] = my_mac[1]; //ETHERNET_CONF_ETHADDR1;
    MacAddress[2] = my_mac[2]; //ETHERNET_CONF_ETHADDR2;
    MacAddress[3] = my_mac[3]; //ETHERNET_CONF_ETHADDR3;
    MacAddress[4] = my_mac[4]; //ETHERNET_CONF_ETHADDR4;
    MacAddress[5] = my_mac[5]; //ETHERNET_CONF_ETHADDR5;

    if ((MacAddress[0]&1) != 0 ) {
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "WARNING: mac address is not unicast. Network won't be initialized\n\r"); //_ML84
        return;
    }

    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Initializing MAC\n\r"); //_ML84

    MSS_MAC_init( PHY_ADDRESS ); //init peripheral
    mac_cfg = MSS_MAC_get_configuration(); //configure peripheral

    mac_cfg &= ~(MSS_MAC_CFG_PASS_BAD_FRAMES | MSS_MAC_CFG_PROMISCUOUS_MODE ); //_ML84 mac cfg
    mac_cfg |=
            MSS_MAC_CFG_STORE_AND_FORWARD | //ML84 added
            //MSS_MAC_CFG_PASS_ALL_MULTICAST | //ML84 no go
            MSS_MAC_CFG_RECEIVE_ALL |
            //MSS_MAC_CFG_PROMISCUOUS_MODE | //ML84
            MSS_MAC_CFG_FULL_DUPLEX_MODE |
            MSS_MAC_CFG_TRANSMIT_THRESHOLD_MODE |
            MSS_MAC_CFG_THRESHOLD_CONTROL_00;

    MSS_MAC_configure(mac_cfg );

    //_ML84
    mac_cfg = MSS_MAC_get_configuration();
    memcpy(txt, "0x00000000\n\r\0", 13);
    uint_to_hexstr(mac_cfg, txt+2, 8);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Current MAC config: ");
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);

    MSS_MAC_set_mac_address((uint8_t *)MacAddress);

    //_ML84
    memcpy(txt, "00 \0", 4);
    MSS_MAC_get_mac_address((uint8_t *)MacAddress);
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    MAC address: ");
    for (i=0; i<6; i++) {
        uint_to_hexstr(MacAddress[i], txt, 2);
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
    }
    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");

} // init_mac (new)

/******************************* DHCP ***************************************/

static int32_t rx_size2;
static uint8_t rx_buffer2[USER_RX_BUFF_SIZE];

void get_ip(char *ipStr, uint8_t network_mode)
{
    uint32_t time_out = 0;
    int32_t i,j,k=0;
    int32_t rx_size;
    uint8_t rx_buffer[USER_RX_BUFF_SIZE];
    uint8_t n_bootp;

    dhcp_ip_found = 0; //ML84 added
    ip_known      = 0;
    num_pkt_rx    = 0;
    time_out      = 0;
    for (i = 0; i < 1600; i++)
    {
        rx_buffer[i] = 0;
    }

    if (network_mode == 'S') {
        //static
        dbg_print("Static mode: using IP from EEPROM\n\r");
        ip_known = 1;
        //dhcp_ip_found = 1; //_FIXME use ip_known
        show_ip(ipStr);
        return;
    } else if (network_mode != 'D') {
        //not static nor dynamic: network off
        dbg_printnum(network_mode, 2);
        dbg_print("Unknown mode: no IP\n\r");
        return;
    } else {
        dbg_print("Dynamic mode: getting IP from DHCP server\n\r");
    }

#ifdef OPEN_IP

    n_bootp = 0;
    j = tick_counter; //start time

#if 0 //original code
    k = tick_counter-500;
    do{
        n_bootp++;
        dbg_printnum(n_bootp,2); dbg_print(": BOOTP: send\n\r");
        send_bootp_packet(0, 0);
        i = tick_counter; //time since last sent packet
        MSS_WD_reload();
        //wait for a packet (max 2 seconds)
        do {
            rx_size = MSS_MAC_rx_pckt_size();
            //time_out++;
            if(dhcp_ip_found)
                break;//goto here;
        } while ( rx_size == 0 && ((tick_counter-i)<200) ); //ML84 2 sec timeout

        MSS_WD_reload();
        if (rx_size != 0) {
            dbg_printnum(n_bootp,2); dbg_print(": BOOTP: got packet\n\r");
            MSS_MAC_rx_packet( rx_buffer, USER_RX_BUFF_SIZE, MSS_MAC_BLOCKING );
            num_pkt_rx++;
            process_packet( rx_buffer, rx_size ); //return: 0=OK, 1=ERROR, 0xFF=DHCP
        } else {
            dbg_printnum(n_bootp,2); dbg_print(": BOOTP: timeout (2s)\n\r");
        }

        MSS_WD_reload();
        if ((tick_counter-i)>=100) {
            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "."); //_ML84
            i = tick_counter;
        }

    } while((!dhcp_ip_found) && ((tick_counter-j)<1000)); //(time_out < 7000000)); //ML84
#endif
#if 0 //tested, working
    k = tick_counter-500;
    do{
            /* DISCOVERY packet is sent when timeout has elapsed since last packet */
            if ((tick_counter-k)>500) {
                n_bootp++;
                dbg_printnum(n_bootp,2); dbg_print(": BOOTP: send\n\r");
                send_bootp_packet(0, 0);
                k = tick_counter; //time since last BOOTP packet has been sent
            }

            MSS_WD_reload();
            //wait for a packet (max 2 seconds)
            i = tick_counter; //wait for packet start time
            do {
                rx_size = MSS_MAC_rx_pckt_size();
                if(dhcp_ip_found)
                    break;//goto here;
            } while ( rx_size == 0 && ((tick_counter-i)<200) ); //ML84 2 sec timeout

            MSS_WD_reload();
            //process packet
            if (rx_size != 0) {
                //dbg_printnum(num_pkt_rx,4); dbg_print(": BOOTP: got packet\n\r");
                MSS_MAC_rx_packet( rx_buffer, USER_RX_BUFF_SIZE, MSS_MAC_BLOCKING );
                num_pkt_rx++;
                process_packet( rx_buffer, rx_size ); //return: 0=OK, 1=ERROR, 0xFF=DHCP
            } else {
                dbg_print("BOOTP: timeout (2s)\n\r");
            }

            MSS_WD_reload();
//          if ((tick_counter-i)>=100) {
//              MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "."); //_ML84
//              i = tick_counter;
//          }

        } while((!dhcp_ip_found) && ((tick_counter-j)<2000)); //(time_out < 7000000)); //ML84
#endif

#if 1 //new
    do {
        MSS_WD_reload();

        switch(dhcp_s) {
            case DHCP_INIT:
                //Send DISCOVER, then go to SELECTING state
                n_bootp++;
                send_bootp_packet(0, 0);
                k = tick_counter; //time since last BOOTP packet has been sent
                dbg_print("DHCP::INIT=>SELECTING\n\r");
                dhcp_s = DHCP_SEL;
                break;
            case DHCP_SEL:
                //Wait for OFFER. If timeout, back to INIT.
                do {
                    rx_size = MSS_MAC_rx_pckt_size();
                } while ( rx_size == 0 && ((tick_counter-k)<500) );

                if (rx_size) { //got packet
                    MSS_MAC_rx_packet( rx_buffer, USER_RX_BUFF_SIZE, MSS_MAC_BLOCKING ); //retrieve packet
                    //num_pkt_rx++; //keep packet counter
                    process_packet( rx_buffer, rx_size ); //process packet
                    if (dhcp_last_msg == 2) { //got an offer, sent a request
                        k = tick_counter; //time since REQUEST has been sent
                        dbg_print("DHCP::SELECTING=>REQUESTING\n\r");
                        rx_size2 = rx_size;
                        memcpy(rx_buffer2, rx_buffer, USER_RX_BUFF_SIZE);
                        dhcp_s = DHCP_REQ;
                    } else { //got no useful packet
                        //stay here and wait for OFFER or timeout
                    }
                } else {
                    //timeout, send new DISCOVER
                    dbg_print("DHCP::SELECTING=>INIT\n\r");
                    dhcp_s = DHCP_INIT;
                }
                break;
            case DHCP_REQ:
                //Wait for ACK/NACK. If timeout, send another REQUEST.
                do {
                    rx_size = MSS_MAC_rx_pckt_size();
                } while ( rx_size == 0 && ((tick_counter-k)<500) );

                if (rx_size) { //got packet
                    MSS_MAC_rx_packet( rx_buffer, USER_RX_BUFF_SIZE, MSS_MAC_BLOCKING ); //retrieve packet
                    //num_pkt_rx++; //keep packet counter
                    process_packet( rx_buffer, rx_size ); //process packet
                    if (dhcp_last_msg == 5) { //got an ACK, go to BOUND state
                        dbg_print("DHCP::REQUESTING=>BOUND\n\r");
                        dhcp_s = DHCP_BOUND;
                    } else if (dhcp_last_msg == 6) { //got NACK, restart from scratch
                        dbg_print("DHCP::REQUESTING=>INIT\n\r");
                        dhcp_s = DHCP_INIT;
                    } else { //no useful packet
                        //keep waiting until ACK/NACK or timeout
                    }
                } else { //timeout, send another request
                    dbg_print("DHCP::send request\n\r");
                    k = tick_counter; //time since REQUEST has been sent
                    send_bootp_packet( rx_buffer2, rx_size2 ); //(rx_buffer same as before)
                }
                break;
            case DHCP_BOUND:
                //if the get_ip() function is called while BOUND, then go to the RENEWING  state
                dbg_print("DHCP::BOUND=>RENEWING\n\r");
                k = tick_counter-500; //set k so that a request is sent immediately when in RENEWING state
                //dhcp_s = DHCP_REN;
                dhcp_s = DHCP_INIT;
                break;
            case DHCP_REN:
                //Wait for ACK/NACK. If timeout, send another REQUEST.
                do {
                    rx_size = MSS_MAC_rx_pckt_size();
                } while ( rx_size == 0 && ((tick_counter-k)<500) );

                if (rx_size) { //got packet
                    MSS_MAC_rx_packet( rx_buffer, USER_RX_BUFF_SIZE, MSS_MAC_BLOCKING ); //retrieve packet
                    //num_pkt_rx++; //keep packet counter
                    process_packet( rx_buffer, rx_size ); //process packet
                    if (dhcp_last_msg == 5) { //got an ACK, go to BOUND state
                        dbg_print("DHCP::RENEWING=>BOUND\n\r");
                        dhcp_s = DHCP_BOUND;
                    } else if (dhcp_last_msg == 6) { //got NACK, restart from scratch
                        dbg_print("DHCP::RENEWING=>INIT\n\r");
                        dhcp_s = DHCP_INIT;
                    } else { //no useful packet
                        //keep waiting until ACK/NACK or timeout
                    }
                } else { //timeout, send another request
                    dbg_print("DHCP::send request\n\r");
                    k = tick_counter; //time since REQUEST has been sent
                    send_bootp_packet( rx_buffer2, rx_size2 ); //(rx_buffer same as before)
                }
                break;
            default:
                //error
                dbg_print("DHCP::ERROR::Bad state\n\r");
                dhcp_s = DHCP_INIT;
        }

    } while((!dhcp_ip_found) && ((tick_counter-j)<4500));
#endif //new code

    if (dhcp_ip_found) {
        show_ip(ipStr);
    } else { //timed out
        dbg_print("Warning: no IP.\n\r");
        dhcp_s = DHCP_INIT;
    }

#endif //OPEN_IP

}

//void init_mac(char *ipStr, uint8_t do_mac_init, uint8_t network_mode)
//{
//    uint32_t mac_cfg;
//	uint32_t time_out = 0;
//	int32_t i,j,k;
//	int32_t rx_size;
//    uint8_t rx_buffer[USER_RX_BUFF_SIZE], txt[]="0x00000000\n\r\0";
//    uint8_t n_bootp;
//
//    unsigned char MacAddress[6];
//
//    if (do_mac_init) {
//        /* Default MAC addr. */
//        MacAddress[0] = my_mac[0]; //ETHERNET_CONF_ETHADDR0;//ML84
//        MacAddress[1] = my_mac[1]; //ETHERNET_CONF_ETHADDR1;
//        MacAddress[2] = my_mac[2]; //ETHERNET_CONF_ETHADDR2;
//        MacAddress[3] = my_mac[3]; //ETHERNET_CONF_ETHADDR3;
//        MacAddress[4] = my_mac[4]; //ETHERNET_CONF_ETHADDR4;
//        MacAddress[5] = my_mac[5]; //ETHERNET_CONF_ETHADDR5;
//
//        if ((MacAddress[0]&1) != 0 ) {
//            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "WARNING: mac address is not unicast. Network won't be initialized\n\r"); //_ML84
//            return;
//        }
//        //	//Reset MAC peripheral
//        //	MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Resetting MAC\n\r"); //_ML84
//        //	mac_cfg = *(volatile uint32_t*)(0xE0042030); //SOFT RST CR
//        //	*(volatile uint32_t*)(0xE0042030) = ( (mac_cfg | 0x00000010) ); //assert MAC peripheral reset
//        //	sleep(1000);
//        //    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Releasing MAC from reset\n\r"); //_ML84
//        //    mac_cfg = *(volatile uint32_t*)(0xE0042030); //SOFT RST CR
//        //    *(volatile uint32_t*)(0xE0042030) = ( (mac_cfg & 0xFFFFFFEF) ); //release MAC peripheral reset
//        //    sleep(1000);
//
//
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Initializing MAC\n\r"); //_ML84
//
//        MSS_MAC_init( PHY_ADDRESS );
//        /*
//         * Configure the MAC.
//         */
//        mac_cfg = MSS_MAC_get_configuration();
//
//        //	mac_cfg &= ~( MSS_MAC_CFG_STORE_AND_FORWARD | MSS_MAC_CFG_PASS_BAD_FRAMES );
//        mac_cfg &= ~(MSS_MAC_CFG_PASS_BAD_FRAMES | MSS_MAC_CFG_PROMISCUOUS_MODE ); //_ML84 mac cfg
//        mac_cfg |=
//                MSS_MAC_CFG_STORE_AND_FORWARD | //ML84 added
//                //MSS_MAC_CFG_PASS_ALL_MULTICAST | //ML84 no go
//                MSS_MAC_CFG_RECEIVE_ALL |
//                //MSS_MAC_CFG_PROMISCUOUS_MODE | //ML84
//                MSS_MAC_CFG_FULL_DUPLEX_MODE |
//                MSS_MAC_CFG_TRANSMIT_THRESHOLD_MODE |
//                MSS_MAC_CFG_THRESHOLD_CONTROL_00;
//
//        MSS_MAC_configure(mac_cfg );
//
//        //_ML84
//        mac_cfg = MSS_MAC_get_configuration();
//        memcpy(txt, "0x00000000\n\r\0", 13);
//        uint_to_hexstr(mac_cfg, txt+2, 8);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    Current MAC config: ");
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
//
//        MSS_MAC_set_mac_address((uint8_t *)MacAddress);
//
//        //_ML84
//        memcpy(txt, "00 \0", 4);
//        MSS_MAC_get_mac_address((uint8_t *)MacAddress);
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "    MAC address: ");
//        for (i=0; i<6; i++) {
//            uint_to_hexstr(MacAddress[i], txt, 2);
//            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) txt);
//        }
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r");
//
//    }
//
//    //dhcp_ip_found = 0; //ML84 added
//    ip_known      = 0;
//    num_pkt_rx    = 0;
//    time_out      = 0;
//    for (i = 0; i < 1600; i++)
//    {
//        rx_buffer[i] = 0;
//    }
//
//        //_MYDEBUG set PASS filter on broadcast address
//        //MSS_MAC_set_mac_filters( 2u, MacAddress);
//
//    if (network_mode == 'S') {
//        //static
//        dbg_print("Static mode: using IP from EEPROM\n\r");
//        ip_known = 1;
//        dhcp_ip_found = 1; //_FIXME temp
//        show_ip(ipStr);
//        return;
//    } else if (network_mode != 'D') {
//        //not static nor dynamic: network off
//        dbg_printnum(network_mode, 2);
//        dbg_print(" <= Unknown mode: ERROR\n\r");
//        return;
//    } else {
//        dbg_print("Dynamic mode: getting IP from DHCP server\n\r");
//    }
//
//#ifdef OPEN_IP
//
//    //MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Getting IP address"); //_ML84
//    j = tick_counter; //start time
//    k = tick_counter-500;
//    n_bootp = 0;
//#if 0 //original code
//	do{
//	    n_bootp++;
//	    dbg_printnum(n_bootp,2); dbg_print(": BOOTP: send\n\r");
//	    send_bootp_packet(0, 0);
//        i = tick_counter; //time since last sent packet
//	    MSS_WD_reload();
//	    //wait for a packet (max 2 seconds)
//	    do {
//	        rx_size = MSS_MAC_rx_pckt_size();
//	        //time_out++;
//	        if(dhcp_ip_found)
//	            break;//goto here;
//	    } while ( rx_size == 0 && ((tick_counter-i)<200) ); //ML84 2 sec timeout
//
//        MSS_WD_reload();
//	    if (rx_size != 0) {
//	        dbg_printnum(n_bootp,2); dbg_print(": BOOTP: got packet\n\r");
//	        MSS_MAC_rx_packet( rx_buffer, USER_RX_BUFF_SIZE, MSS_MAC_BLOCKING );
//	        num_pkt_rx++;
//	        process_packet( rx_buffer, rx_size ); //return: 0=OK, 1=ERROR, 0xFF=DHCP
//	    } else {
//	        dbg_printnum(n_bootp,2); dbg_print(": BOOTP: timeout (2s)\n\r");
//	    }
//
//        MSS_WD_reload();
//        if ((tick_counter-i)>=100) {
//            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "."); //_ML84
//            i = tick_counter;
//        }
//
//	} while((!dhcp_ip_found) && ((tick_counter-j)<1000)); //(time_out < 7000000)); //ML84
//#else
//	do{
//            /* DISCOVERY packet is sent when timeout has elapsed since last packet */
//            if ((tick_counter-k)>500) {
//                n_bootp++;
//                dbg_printnum(n_bootp,2); dbg_print(": BOOTP: send\n\r");
//                send_bootp_packet(0, 0);
//                k = tick_counter; //time since last BOOTP packet has been sent
//            }
//
//	        MSS_WD_reload();
//	        //wait for a packet (max 2 seconds)
//	        i = tick_counter; //wait for packet start time
//	        do {
//	            rx_size = MSS_MAC_rx_pckt_size();
//	            if(dhcp_ip_found)
//	                break;//goto here;
//	        } while ( rx_size == 0 && ((tick_counter-i)<200) ); //ML84 2 sec timeout
//
//	        MSS_WD_reload();
//	        //process packet
//	        if (rx_size != 0) {
//	            //dbg_printnum(num_pkt_rx,4); dbg_print(": BOOTP: got packet\n\r");
//	            MSS_MAC_rx_packet( rx_buffer, USER_RX_BUFF_SIZE, MSS_MAC_BLOCKING );
//	            num_pkt_rx++;
//	            process_packet( rx_buffer, rx_size ); //return: 0=OK, 1=ERROR, 0xFF=DHCP
//	        } else {
//	            dbg_print("BOOTP: timeout (2s)\n\r");
//	        }
//
//	        MSS_WD_reload();
////	        if ((tick_counter-i)>=100) {
////	            MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "."); //_ML84
////	            i = tick_counter;
////	        }
//
//	    } while((!dhcp_ip_found) && ((tick_counter-j)<2000)); //(time_out < 7000000)); //ML84
//#endif
//
//
//    MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\r"); //_ML84
//    if (dhcp_ip_found) {
//        show_ip(ipStr);
//    } else {
//        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "Warning: no IP.\n\r");
//    }
//
//#endif //OPEN_IP
//
//} // End of ethernet



#if 0
struct http_state {
  char *file;
  u32_t left;
  u8_t retries;
};

/*-----------------------------------------------------------------------------------*/
static void
conn_err(void *arg, err_t err)
{
  struct http_state *hs;

  hs = arg;
//  mem_free(hs);
  hs->file = NULL;
  hs->left = 0;

}
/*-----------------------------------------------------------------------------------*/
static void
close_conn(struct tcp_pcb *pcb, struct http_state *hs)
{
#if 1
  tcp_arg(pcb, NULL);
  tcp_sent(pcb, NULL);
  tcp_recv(pcb, NULL);
//  mem_free(hs);
  hs->file = NULL;
  hs->left = 0;
  tcp_close(pcb);
#endif  
}
/*-----------------------------------------------------------------------------------*/
static void
send_data(struct tcp_pcb *pcb, struct http_state *hs)
{
  err_t err;
  u16_t len, len2;
  u8_t data_to_send = 0;

  /* We cannot send more data than space available in the send buffer */
  if (tcp_sndbuf(pcb) < hs->left) {
      len = tcp_sndbuf(pcb);
  }
  else {
      len = hs->left;
      LWIP_ASSERT("hs->left did not fit into u16_t!",
                  (len == hs->left));
  }
  if (len > (2*pcb->mss)) {
      len = 2*pcb->mss;
  }

  do {
      //DEBUG_PRINT("Sending %d bytes\n", len);

      /* If the data is being read from a buffer in RAM, we need to copy
      * it into the PCB. If it's in flash, however, we can avoid the
      * copy since the data is obviously not going to be overwritten
      * during the life of the connection.
      */
      err = tcp_write(pcb, hs->file, len, 0);
      if (err == ERR_MEM) {
          len /= 2;
      }
  } while (err == ERR_MEM && len > 1);

  if (err == ERR_OK) {
      data_to_send = 1;
      hs->file += len;
      hs->left -= len;
  }

   /* If we wrote anything to be sent, go ahead and send it now. */
   if (data_to_send) {
     //DEBUG_PRINT("tcp_output\n");
     tcp_output(pcb);
   }
//hs->file = NULL;
}


/*-----------------------------------------------------------------------------------*/
static err_t
http_poll(void *arg, struct tcp_pcb *pcb)
{
  struct http_state *hs;

  hs = arg;
  
  /*  printf("Polll\n");*/
  if (hs == NULL) {
    /*    printf("Null, close\n");*/
    tcp_abort(pcb);
    return ERR_ABRT;
  } else {
    ++hs->retries;
    if (hs->retries == 4) {
      tcp_abort(pcb);
      return ERR_ABRT;
    }
    send_data(pcb, hs);
  }

  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static err_t
http_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
  struct http_state *hs;

  hs = arg;

  hs->retries = 0;
  if(hs->left > 0) {    
    send_data(pcb, hs);
  } else {
    close_conn(pcb, hs);
  }

  return ERR_OK;
}

void delay(int x)
{
	int ii;
	for(ii=0; ii<x; ii++)
		;
}
extern void ace_task(void);

void update_multimeter_page()
{
	ace_task();
	snprintf((char *)appdata, 1200,
"<HTML><HEAD><TITLE>Actel SmartFusion Webserver</TITLE>"
"<META http-equiv=Content-Type content=\"text/html; charset=windows-1252\">"
"<META http-equiv=Refresh content=2>"
"<BODY>"
"<FORM action=realdata.shtm method=get>"
"<TABLE class=tbl_text cellSpacing=1 cellPadding=1 width=\"800%\" align=center>"
"<TBODY>"
"  <TR>"
"    <TD align=middle colSpan=2 td <></TD>"
"  <TR>"
"    <TD align=middle colSpan=2><B>Real Time Data"
"  Display<B></B></B></TD></TR></TBODY></TABLE>"
"<TABLE class=tbl_text width=\"400%\" align=center border=1>"
"<TBODY>"
"<TR>"
"<TD align=middle width=\"70%\"><FONT size=-1><B>Channel/Quantity</B></FONT></TD>"
"<TD align=middle><FONT size=-1><B>Value</B></FONT></TD></TD></TR>"
"<TR>"
" <TD>Potentiometer Voltage</TD>"
"<TD align=middle>"
"%.2f V"
"</TD></TR>"
"<TR>"
" <TD>Potentiometer Current</TD>"
"<TD align=middle>"
"%.2f mA"
"</TD></TR>"
"<TR>"
" <TD>External Temperature in Celcius</TD>"
"<TD align=middle>"
"%.2f °C"
"</TD></TR>"
"<TR>"
" <TD>External Temperature in Fahrenheit</TD>"
"<TD align=middle>"
"%.2f F"
"</TD></TR>"	
"<TR>"
" <TD>External Temperature in Kelvin</TD>"
"<TD align=middle>"
"%.2f K"
"</TD></TR></TBODY></TABLE>"
"<TABLE align=center>"
" <TBODY>"
"<form>"
"<input type = \"Button\" value = \"Home\" onclick = \"window.location.href='index.html'\">"
"</form>"
"</table></form>\n" 
"</body></html>\n \0", real_voltage_value, real_current_value, real_temperature_value_tc,real_temperature_value_tf,real_temperature_value_tk);

}

/*-----------------------------------------------------------------------------------*/
static err_t
http_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	  int i, res;
	  unsigned int s1;
	  FIL file1;
	  char *data;
	  struct http_state *hs;
	  //unsigned char Buff[10240];
      unsigned char *buffPtr = appdata;
      char *tempStrPtr;
      unsigned char oled_string[20];
      unsigned char c;
      
	  hs = (struct http_state *)arg;

	  if (err == ERR_OK && p != NULL) {

	    /* Inform TCP that we have taken the data. */
	    tcp_recved(pcb, p->tot_len);
	    
	    if (hs->file == NULL)
	    {
	       hs->left = 0;
	       data = (char *)p->payload;
           //iprintf("%s\n",data);
	       if (strncmp(data, "GET ", 4) == 0)
	       {

               if( !strncmp( data, "GET /realdata.shtml", 19 ) )
	  	       {
	  	           update_multimeter_page();
	  	    	   hs->file = (char *)appdata;
	  			   hs->left = 1200 /*sizeof(appdata)*/;
	  	       }
	  	       else if( !strncmp( data, "GET /mul.shtml", 14) )
	  	       {
	  	           update_multimeter_page();
	  	    	   hs->file = (char *)appdata;
	  			   hs->left = 1200/*sizeof(appdata)*/;
	  	       }
	  	       else
	  	       {
                   s1 = 0;
	  	    	   for(i = 0; i < 40; i++)
	               {
	                   if (((char *)data + 4)[i] == ' ' ||
	                       ((char *)data + 4)[i] == '\r' ||
	                       ((char *)data + 4)[i] == '\n')
	                   {
	                       ((char *)data + 4)[i] = 0;
	                       break;
	                   }
	               }

	               if (*(char *)(data + 4) == '/' &&
	                   *(char *)(data + 5) == 0)
	               {
	                   res = f_open(&file1, "index.html", FA_OPEN_EXISTING | FA_READ);
	               } 
	               else
	               {
	        	       res = f_open(&file1,(char *)data + 5, FA_OPEN_EXISTING | FA_READ);
	               }

		    	   if( !strncmp( data, "GET /TextTerminal.html?", 23 ))
	               {
	                   tempStrPtr = data + 37;
	                   for (i = 0; i < 20; i++)
	                   {
	                       c = *tempStrPtr++;
	                       if (c == ' ')
	                            break;
	                       if (c == '+')
	                       {
	                          c = ' ';
	                       }
	                       else if (c == '%') {
	                            unsigned char temp1,temp2;
	                            temp1=(*tempStrPtr++);
	                            temp2=(*tempStrPtr++);
	                            c = hex_digits_to_byte(temp1,temp2);
	                        }
	                        oled_string[i] = c;
	                    }
	                    oled_string[i] = '\0';

	                    iprintf("STRING Submitted: %s\n",oled_string);
	                    res = f_open(&file1, "TextTerminal.html", FA_OPEN_EXISTING | FA_READ);
	               }


	               if(res)
	               {
	                   if(f_open(&file1, "404.html", FA_OPEN_EXISTING | FA_READ))
	            	       iprintf("Web pages are not present in SPI Flash %s\n\r", (data + 5));
	                   s1 = 0;
	               }
	               else
	               {
           	           res = f_read(&file1, buffPtr, sizeof(appdata), &s1);
                       f_close(&file1);

	               }


                   hs->file = (char *)appdata;
		           hs->left = s1;
	  	       }
		       pbuf_free(p);

               /* Tell TCP that we wish be to informed of data that has been
	              successfully sent by a call to the http_sent() function. */
               tcp_sent(pcb, http_sent);
               send_data(pcb, hs);
	       }
	       else
	       {
	           pbuf_free(p);
	           close_conn(pcb, hs);
	       }
	   }
	   else
	   {
	       pbuf_free(p);
	   }
    }

	if (err == ERR_OK && p == NULL)
	{
	    close_conn(pcb, hs);
    }
	return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
struct http_state  gHS;
struct http_state *hsg = &gHS;
static err_t
http_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{


 
  /* Initialize the structure. */
  hsg->file = NULL;
  hsg->left = 0;
  hsg->retries = 0;

  /* Tell TCP that this is the structure we wish to be passed for our
     callbacks. */
  tcp_arg(pcb, hsg);

  /* Tell TCP that we wish to be informed of incoming data by a call
     to the http_recv() function. */
  tcp_recv(pcb, http_recv);

  tcp_err(pcb, conn_err);
  
  tcp_poll(pcb, http_poll, 4);
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
void
httpd_init(void)
{
  struct tcp_pcb *pcb;

  pcb = tcp_new();
  tcp_bind(pcb, IP_ADDR_ANY, 80);
  pcb = tcp_listen(pcb);
  tcp_accept(pcb, http_accept);
}
#endif
