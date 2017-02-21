/*
 * eth_test.h
 *
 *  Created on: 12.10.2015
 *      Author: malatesta_a
 */

#ifndef ETH_TEST_H_
#define ETH_TEST_H_

#include "hal.h"

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
#define PHY_ADDRESS               MSS_PHY_ADDRESS_AUTO_DETECT //1 xxx ML84 change
#define DHCP_ATTEMPTS             4
#define USER_RX_BUFF_SIZE         1600



void init_mac(char *ipStr);
unsigned char tcp_init(void);
void show_ip(char *ipStr);
void web_task(uint32_t TOUT);
void my_appcall(void);

#endif /* ETH_TEST_H_ */
