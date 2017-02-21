
#include <string.h>

#include "conf_eth.h"
#include "lwipopts.h"

/* Scheduler include files. */
#if (NO_SYS == 0)
#include "FreeRTOS.h"
#include "task.h"
#endif


/* ethernet includes */
#include "ethernet.h"
#include "conf_eth.h"
#include "mss_ethernet_mac.h"
#include "mss_watchdog.h"

#if (HTTP_USED == 1)
  #include "BasicWEB.h"
#endif

#if (TFTP_USED == 1)
  #include "BasicTFTP.h"
#endif

#if (SMTP_USED == 1)
  #include "BasicSMTP.h"
#endif

/* lwIP includes */
#include "lwip/init.h"
#include "lwip/sys.h"
#include "lwip/api.h" 
#include "lwip/tcpip.h"
#include "lwip/memp.h" 
#include "lwip/stats.h"
#include "netif/loopif.h"
#include "lwipopts.h"
#include "lwip/ip.h"


/* global variable containing MAC Config (hw addr, IP, GW, ...) */
struct netif MACB_if;


/* Initialisation required by lwIP. */
void prvlwIPInit( void );

/* Initialisation of ethernet interfaces by reading config file */
static void prvEthernetConfigureInterface(void * param);



#if (NO_SYS == 0)
extern portTASK_FUNCTION( vBasicWEBServer, pvParameters );


void vStartEthernetTask( unsigned portBASE_TYPE uxPriority )
{
  /* Setup lwIP. */
  prvlwIPInit();


}
#endif

/*!
 *  \brief start lwIP layer.
 */
void portlwIPInit( void )
{
	/* Initialize lwIP and its interface layer. */
	#if LWIP_STATS
		stats_init();
	#endif
	lwip_init(); //init memory
#if 0    
	sys_init();
	mem_init();
	memp_init();
	pbuf_init();
	netif_init();
	ip_init();
    udp_init(); 
	tcp_init();
#endif	
#if (NO_SYS == 0)	

	
	/* once TCP stack has been initalized, set hw and IP parameters, initialize MACB too */
	tcpip_init( prvEthernetConfigureInterface, NULL );
#else
	prvEthernetConfigureInterface(NULL);
#endif
}

/*!
 *  \brief set ethernet config 
 */
#if (NO_SYS == 0)
extern void vMBServerTask( void *arg );
#endif
static void prvEthernetConfigureInterface(void * param)
{
struct ip_addr xIpAddr, xNetMask, xGateway;
extern err_t ethernetif_init( struct netif *netif );
extern unsigned char my_ip[4];
//extern unsigned char my_netmask[4];
//extern unsigned char my_gateway[4];
unsigned char MacAddress[6];

extern unsigned char my_mac[6]; //ML84
//struct ip_addr ip;
   /* Default MAC addr. */
   MacAddress[0] = my_mac[0]; //ETHERNET_CONF_ETHADDR0;
   MacAddress[1] = my_mac[1]; //ETHERNET_CONF_ETHADDR1;
   MacAddress[2] = my_mac[2]; //ETHERNET_CONF_ETHADDR2;
   MacAddress[3] = my_mac[3]; //ETHERNET_CONF_ETHADDR3;
   MacAddress[4] = my_mac[4]; //ETHERNET_CONF_ETHADDR4;
   MacAddress[5] = my_mac[5]; //ETHERNET_CONF_ETHADDR5;

   /* pass the MAC address to MACB module */
   //MSS_MAC_set_mac_address((uint8_t *)MacAddress); //_MYDEBUG already done in mac_init
	
   /* set MAC hardware address length to be used by lwIP */
   MACB_if.hwaddr_len = 6;
   
   /* set MAC hardware address to be used by lwIP */
   memcpy( MACB_if.hwaddr, MacAddress, MACB_if.hwaddr_len );
#if 0
    IP4_ADDR(&xIpAddr,  0, 0, 0, 0);
    IP4_ADDR(&xNetMask, 0, 0, 0, 0); 
    IP4_ADDR(&xGateway, 0, 0, 0, 0); 
#else
    /* Default ip addr. */
    IP4_ADDR( &xIpAddr,my_ip[0],my_ip[1],my_ip[2],my_ip[3] );
    
    /* Default Subnet mask. TODO mask from DHCP */
    IP4_ADDR( &xNetMask,ETHERNET_CONF_NET_MASK0,ETHERNET_CONF_NET_MASK1,ETHERNET_CONF_NET_MASK2,ETHERNET_CONF_NET_MASK3 );
    
    /* Default Gw addr. TODO gateway from DHCP */
    IP4_ADDR( &xGateway,ETHERNET_CONF_GATEWAY_ADDR0,ETHERNET_CONF_GATEWAY_ADDR1,ETHERNET_CONF_GATEWAY_ADDR2,ETHERNET_CONF_GATEWAY_ADDR3 );
#endif
   
   
   /* add data to netif */
   netif_add( &MACB_if, &xIpAddr, &xNetMask, &xGateway, NULL, ethernetif_init, ip_input );
   
   /* make it the default interface */
   netif_set_default( &MACB_if );
   
   /* bring it up */
   netif_set_up( &MACB_if );
#if LWIP_DHCP
   dhcp_start(&MACB_if);
#endif
   //   printf("IP = 0x%x\n", (MACB_if.ip_addr).addr);
   //show_ip();
#if (NO_SYS == 0)   
   sys_thread_new( "webtask",vBasicWEBServer, ( void * ) NULL, 1024, /*ethWEBSERVER_PRIORITY*/ configMAX_PRIORITIES - 1  );
#endif
}

void eth_update_addresses(void)
{
    struct ip_addr xIpAddr, xNetMask, xGateway;

    extern unsigned char my_ip[4];
    //extern unsigned char my_netmask[4];
    //extern unsigned char my_gateway[4];

    /* Default ip addr. */
    IP4_ADDR( &xIpAddr,my_ip[0],my_ip[1],my_ip[2],my_ip[3] );

    /* Default Subnet mask. TODO mask from DHCP */
    IP4_ADDR( &xNetMask,ETHERNET_CONF_NET_MASK0,ETHERNET_CONF_NET_MASK1,ETHERNET_CONF_NET_MASK2,ETHERNET_CONF_NET_MASK3 );

    /* Default Gw addr. TODO gateway from DHCP */
    IP4_ADDR( &xGateway,ETHERNET_CONF_GATEWAY_ADDR0,ETHERNET_CONF_GATEWAY_ADDR1,ETHERNET_CONF_GATEWAY_ADDR2,ETHERNET_CONF_GATEWAY_ADDR3 );

    netif_set_addr(&MACB_if, &xIpAddr, &xNetMask, &xGateway);
}


