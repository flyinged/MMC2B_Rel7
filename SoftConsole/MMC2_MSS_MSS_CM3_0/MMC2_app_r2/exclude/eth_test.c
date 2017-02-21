/*
 * eth_test.c
 *
 *  Created on: 12.10.2015
 *      Author: malatesta_a
 */

#include "eth_test.h"
#include "hal.h"
#include "mss_ethernet_mac.h"
#include "tcpip.h"
#include "uip_arp.h"
#include "uip.h"
#include "string.h"
#include "uart_utility.h"
#include "GOLDELOX_SERIAL_4DLIBRARY.h"

char ethAddr[6] = {0xaa,0xbb,0xcc,0x66,0x55,0x44};

extern unsigned char              my_ip[4];
extern unsigned int               num_pkt_rx;
extern unsigned char              ip_known;

extern unsigned char dhcp_ip_found;
//extern tcp_control_block_t tcb;

extern uint32_t oled_update_time;
extern volatile uint32_t tick_counter;

void init_mac(char *ipStr)
{
    uint32_t time_out = 0;
    int32_t mac_cfg;
    int32_t i;
    int32_t rx_size;
    uint8_t rx_buffer[USER_RX_BUFF_SIZE];
    MSS_MAC_init(PHY_ADDRESS );
    /*
     * Configure the MAC.
     */
    mac_cfg = MSS_MAC_get_configuration();

    mac_cfg &= ~( MSS_MAC_CFG_STORE_AND_FORWARD | MSS_MAC_CFG_PASS_BAD_FRAMES );
    mac_cfg |=
    MSS_MAC_CFG_RECEIVE_ALL |
    MSS_MAC_CFG_PROMISCUOUS_MODE |
    MSS_MAC_CFG_FULL_DUPLEX_MODE |
    MSS_MAC_CFG_TRANSMIT_THRESHOLD_MODE |
    MSS_MAC_CFG_THRESHOLD_CONTROL_00;
    MSS_MAC_configure(mac_cfg );
    MSS_MAC_set_mac_address((uint8_t *)ethAddr);
    tcp_init();

    ip_known = 0;
    num_pkt_rx = 0;
    time_out = 0;
    for (i = 0; i < 1600; i++)
    {
        rx_buffer[i] = 0;
    }

    /* Logic to get the open IP address */
#ifdef OPEN_IP
    do
    {
        send_bootp_packet(0);
        do
        {
            rx_size = MSS_MAC_rx_pckt_size();
            time_out++;
            if(dhcp_ip_found)
                break;
         }while ( rx_size == 0 && (time_out < 3000000));
         MSS_MAC_rx_packet( rx_buffer, USER_RX_BUFF_SIZE, MSS_MAC_BLOCKING );
         num_pkt_rx++;
         process_packet( rx_buffer );
    }while((!dhcp_ip_found) && (time_out < 7000000));
#endif

    show_ip(ipStr);
}

void show_ip(char *ipStr)
{
    //char *textStr;
    unsigned char i,j; //ML84

    MSS_UART_polled_tx_string(&g_mss_uart0, (const uint8_t*) "Successfully requested IP addr \n\r" );

    if((my_ip[0] == 192) &&
       (my_ip[1]==168)   &&
       (my_ip[2]==0)     &&
       (my_ip[3]==14))
    {
        MSS_UART_polled_tx_string(&g_mss_uart0, (const uint8_t*) "Static IP address: " );
        //textStr = "Browse Static IP:";
    }
    else
    {
        MSS_UART_polled_tx_string(&g_mss_uart0, (const uint8_t*) "Dynamic IP address: " );
        //textStr = "Browse Dynamic IP:";
    }
    //ML84 replace printf
    //printf( "%d.%d.%d.%d \n\r", my_ip[0], my_ip[1], my_ip[2], my_ip[3] );
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

    //sprintf(ipStr,"%d.%d.%d.%d     ",my_ip[0], my_ip[1], my_ip[2], my_ip[3]); //ML84

    //menu_show(textStr, ipStr);
}

/**************************************************************************/
/* Function for uIP initialization */
/**************************************************************************/

void uIP_init()
{
    uip_ipaddr_t ipaddr;
    static struct uip_eth_addr sTempAddr;

    /* init tcp/ip stack */
    uip_init();
    uip_ipaddr(ipaddr, my_ip[0], my_ip[1], my_ip[2], my_ip[3]);
    uip_sethostaddr(ipaddr);

    uip_ipaddr(ipaddr,
               DEFAULT_NETMASK0,
               DEFAULT_NETMASK1,
               DEFAULT_NETMASK2,
               DEFAULT_NETMASK3);
    uip_setnetmask(ipaddr);

    /* set mac address */
    sTempAddr.addr[0] = ethAddr[0];
    sTempAddr.addr[1] = ethAddr[1];
    sTempAddr.addr[2] = ethAddr[2];
    sTempAddr.addr[3] = ethAddr[3];
    sTempAddr.addr[4] = ethAddr[4];
    sTempAddr.addr[5] = ethAddr[5];

    uip_setethaddr(sTempAddr);

    /* init application */
    //httpd_init();
    uip_listen(HTONS(80));
}


//meant to be run as a task with an RTOS
void web_task(uint32_t TOUT)
{
    long lPeriodicTimer, lARPTimer, lPacketLength;
    unsigned long ulTemp, tic;
    unsigned char rx_size, uart_rx_buf[1];
    char ipStr[20];

    while(1)
    {
        /* This semaphore will wait for the user input to run webserver */
        //xSemaphoreTake( webSemHndl, portMAX_DELAY ) ;

        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t*) "Initializing ethernet...\n\r");
        init_mac(ipStr); ipStr[16] = '\n'; ipStr[17] = '\0';
        /* display IP */
        if (oled_en) {
            gfx_Contrast(15); //display on
            txt_MoveCursor(0x0003, 0x0000); //start of line 4
            oled_clear_rect(0, 96, 24, 24+8);
            oled_putstr_small(0, 3*8, ipStr);
            oled_update_time = tick_counter;
        }

        uIP_init();

        lPeriodicTimer = 0;
        lARPTimer = 0;
        tic = tick_counter;
        MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t*) "Now listening. Press key to exit.\n\r");

        while(1)
        {
            lPacketLength = MSS_MAC_rx_packet( uip_buf,
                                               sizeof(uip_buf),
                                               MSS_MAC_BLOCKING);
            if(lPacketLength > 0)
            {
                /*  Set uip_len for uIP stack usage.*/
                uip_len = (unsigned short)lPacketLength;

                /*  Process incoming IP packets here.*/
                if(BUF->type == htons(UIP_ETHTYPE_IP))
                {
                    uip_arp_ipin(); //update arp cache
                    uip_input(); //uip_process(1)

                    /*  If the above function invocation resulted in data that
                         should be sent out on the network, the global variable
                         uip_len is set to a value > 0.*/

                    if(uip_len > 0)
                    {
                        uip_arp_out();
                        MSS_MAC_tx_packet( uip_buf,
                                           uip_len,
                                           MSS_MAC_NONBLOCKING);
                        uip_len = 0;
                    }
                }

                /*  Process incoming ARP packets here. */

                else if(BUF->type == htons(UIP_ETHTYPE_ARP))
                {
                    uip_arp_arpin();

                    /*  If the above function invocation resulted in data that
                         should be sent out on the network, the global variable
                         uip_len is set to a value > 0  */

                    if(uip_len > 0)
                    {
                        MSS_MAC_tx_packet(uip_buf,
                                          uip_len,
                                          MSS_MAC_NONBLOCKING);
                        uip_len = 0;
                    }
                }
            }

            for(ulTemp = 0; ulTemp < UIP_CONNS; ulTemp++)
            {
                uip_periodic(ulTemp);
                /* If the above function invocation resulted in data that
                    should be sent out on the network, the global variable
                    uip_len is set to a value > 0 */

                if(uip_len > 0)
                {
                    uip_arp_out();
                    MSS_MAC_tx_packet(uip_buf,
                                      uip_len,
                                      MSS_MAC_NONBLOCKING);
                    uip_len = 0;
                }
            }

            uip_arp_timer();

            rx_size = MSS_UART_get_rx ( &g_mss_uart0, uart_rx_buf, sizeof(uart_rx_buf) ); //get char from console
            if(rx_size>0)
                return;
            if ((tick_counter-tic) > TOUT) {
                MSS_UART_polled_tx_string( &g_mss_uart0, (const uint8_t *) "\n\rTIMEOUT while waiting for user input\n\r");
                return;
            }
//            if(!inWebTask)
//                break;
        }
    }
}

void my_appcall(void){
    //placeholder
}
