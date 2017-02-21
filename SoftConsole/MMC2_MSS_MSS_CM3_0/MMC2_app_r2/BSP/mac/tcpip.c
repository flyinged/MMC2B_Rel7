/*******************************************************************************
 * (c) Copyright 2009 Actel Corporation,All Rights Reserved.  
 * 
 *  tcpip.c:TCP/IP implementation for webserver.
 */
#define addr_t CPUTYPES_addr_t //ML84
#include "cpu_types.h"
#undef addr_t //ML84
#include "nettype.h"
#include "../../drivers/mss_ethernet_mac/mss_ethernet_mac.h"
#include "../../drivers/mss_ethernet_mac/mss_ethernet_mac_regs.h"
#include "tcpip.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#define OK  0
#define ERR 1
#define MAC_BASE_ADDRESS			0x40003000


extern char ethAddr[6];

extern unsigned char my_ip[IP_ADDR_LEN]; //={127,0,0,1}; //ML84 added EXTERN
extern unsigned char my_mac[6]; //={0x02,0x04,0x25,0x40,0x40,0x40}; //ML84
extern unsigned char my_hostname[8]; //={0x02,0x04,0x25,0x40,0x40,0x40}; //ML84
unsigned char tcp_packet[1532];
unsigned char ip_known;
unsigned char dhcp_ip_found;
unsigned int  dhcp_lease_time;
unsigned char dhcp_srv_ip[4];

//ML84 added
unsigned char dhcp_s = DHCP_INIT;
unsigned char dhcp_last_msg = 0; //none

unsigned short int ip_id;
unsigned char selected_mode = 0;
unsigned char selectedwaveform = 0;
unsigned char rtc_count[5]={0,0,0,0,0};
unsigned char rtc_match_count[5]={0,5,0,0,0};
unsigned char get_count[5];
unsigned int num_pkt_tx = 0,num_pkt_rx = 0;
#define TCP_START_SEQ     0x10203040
static const unsigned char g_client_ip[IP_ADDR_LEN] = { 192, 168, 1, 10 };
unsigned char oled_string[20];
tcp_control_block_t tcb;
MAC_instance_t g_mac;


/***************************************************************************//**
 *  See tcpip.h for more information.
 */
unsigned char send_arp_reply(unsigned char *buf)
{
    /* Modify the packet in place */

    arp_pkt_xp arp_pkt = (arp_pkt_xp )(buf + sizeof(ether_hdr_t));
    eth_hdr_xp eth_hdr = (eth_hdr_xp ) buf;

    memcpy(eth_hdr->da, eth_hdr->sa, ETH_ADDR_LEN);
    memcpy(eth_hdr->sa, my_mac, ETH_ADDR_LEN);
    arp_pkt->opcode[1] = ARP_OPCODE_REPLY_1;
    memcpy(arp_pkt->mac_ta, arp_pkt->mac_sa, ETH_ADDR_LEN);
    memcpy(arp_pkt->ip_ta, arp_pkt->ip_sa, IP_ADDR_LEN);
    memcpy(arp_pkt->mac_sa, my_mac, ETH_ADDR_LEN);
    memcpy(arp_pkt->ip_sa, my_ip, IP_ADDR_LEN);
    num_pkt_tx++;
    MSS_MAC_tx_packet(buf,42, MSS_MAC_BLOCKING);
    return OK;
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */
void send_gratuitous_arp(unsigned char *buf)
{
    arp_pkt_xp arp_pkt = (arp_pkt_xp )(buf + sizeof(ether_hdr_t));
    eth_hdr_xp eth_hdr = (eth_hdr_xp ) buf;
    memset(eth_hdr->da, 0xFF, ETH_ADDR_LEN); /* broadcast */
    memcpy(eth_hdr->sa, my_mac, ETH_ADDR_LEN);
    eth_hdr->type_code[0] = ETH_TYPE_0;
    eth_hdr->type_code[1] = ETH_TYPE_ARP_1;
    arp_pkt->hw_type[0] = ARP_HW_TYPE_0;
    arp_pkt->hw_type[1] = ARP_HW_TYPE_1;
    arp_pkt->proto_type[0] = ETH_TYPE_0;
    arp_pkt->proto_type[1] = ETH_TYPE_IP_1;
    arp_pkt->hw_addr_len = ETH_ADDR_LEN;
    arp_pkt->proto_addr_len = IP_ADDR_LEN;
    arp_pkt->opcode[0] = ARP_OPCODE_0;
    arp_pkt->opcode[1] = ARP_OPCODE_REQ_1;
    memcpy(arp_pkt->mac_sa, my_mac, ETH_ADDR_LEN);
    memcpy(arp_pkt->ip_sa, my_ip, IP_ADDR_LEN);
    memset(arp_pkt->mac_ta, 0x00, ETH_ADDR_LEN);
    memcpy(arp_pkt->ip_ta, my_ip, IP_ADDR_LEN);
    //mac_tx_send(buf,42,0);
    num_pkt_tx++;
    MSS_MAC_tx_packet(buf,42, MSS_MAC_BLOCKING);
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 * 
 */
unsigned short int get_checksum(unsigned char *buf, unsigned short int len, unsigned short int pos)
{
    unsigned int sum;	/* our accumulated sum */
    unsigned short int delta;		/* the next 16-bit quantity to add  */
    unsigned short int i; 
    unsigned short int ilen;   
    sum = (unsigned int) 0; 
 	ilen=(len&1)?len-1:len; 
    for (i = 0; i < ilen; i += 2) { 
	if (i == pos) continue;
	delta = (unsigned short int)buf[i+1] + (unsigned short int)((unsigned short int)buf[i] << 8); 
	sum += delta; 
	if (sum & (unsigned int) 0x10000) {	/* if there's a carry...  */
	    sum &= 0xffff;	/* get rid of the carry bit  */
	    sum++;		/* and move it down here  */
	} 
    } 
	if ( len & 1)  {
	delta = (unsigned short int)((unsigned short int)buf[i] << 8); 
	sum += delta; 
	if (sum & (unsigned int) 0x10000) {	/* if there's a carry...  */
	    sum &= 0xffff;	/* get rid of the carry bit  */
	    sum++;		/* and move it down here  */
	} 				
	}
    sum = ~sum; 
    return sum;
  
} //end calc_checksum 
/***************************************************************************//**
 *  See tcpip.h for more information.
 */
unsigned char fix_checksum(unsigned char *buf, unsigned short int len, unsigned short int pos)
{
    unsigned short int sum = get_checksum(buf,len,pos);   
    buf[pos] = (unsigned char)(sum >> 8);
    buf[pos+1] = (unsigned char)sum;
    return OK;
}

/***************************************************************************//**
 *  See tcpip.h for more information.
 */
unsigned char check_checksum(unsigned char *buf, unsigned short int len, unsigned short int pos, char type)
{
    unsigned short int sum = get_checksum(buf,len,pos);
    
    if ((buf[pos] != (unsigned char)(sum >> 8)) || 
	(buf[pos+1] != (unsigned char) sum)) {

	type = 0;	    /* get around compiler warning */
	return ERR;
    } else {
	return OK;
    }
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */
unsigned char send_icmp_echo_reply(unsigned char *buf)
{
    eth_hdr_xp eth_hdr = (eth_hdr_xp ) buf;
    ip_hdr_xp ip_hdr = (ip_hdr_xp ) (buf + sizeof (ether_hdr_t));
    icmp_hdr_xp icmp_hdr = (icmp_hdr_xp ) 
	(buf + sizeof (ether_hdr_t) + sizeof(ip_hdr_t));
    unsigned short int elen = ((unsigned short int)ip_hdr->tlen[0] << 8) + (unsigned short int)ip_hdr->tlen[1] - sizeof(ip_hdr_t);
    memcpy(eth_hdr->da, eth_hdr->sa, ETH_ADDR_LEN);
    memcpy(eth_hdr->sa, my_mac, ETH_ADDR_LEN);
    memcpy(ip_hdr->da, ip_hdr->sa, IP_ADDR_LEN);
    memcpy(ip_hdr->sa, my_ip, IP_ADDR_LEN);
    ip_hdr->ttl--;
    fix_checksum((unsigned char *)ip_hdr, (unsigned short int) 20, (unsigned short int) 10);
    icmp_hdr->type = ICMP_TYPE_ECHO_REPLY;
    if (elen & 1) {
	((unsigned char *)icmp_hdr)[elen] = 0;
    }
    fix_checksum((unsigned char *)icmp_hdr, (unsigned short int) elen, (unsigned short int) 2);
    num_pkt_tx++;
    MSS_MAC_tx_packet(buf,elen + sizeof(ether_hdr_t) + sizeof(ip_hdr_t), MSS_MAC_BLOCKING);
    return OK;
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */
void dtoa_reverse(unsigned short int n, unsigned char *buf)
{
    buf--;
    if (n == 0) {
	*buf = '0';
	return;
    }
    while (n > 0) {
	*buf-- = (n % 10) + '0';
	n = n / 10;
    }
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */

void send_bootp_packet (unsigned char *buf, int size)
{
    uint16_t i,j;
    uint8_t opt_size, *p8;

    /*************** DECLARATIONS *******************/
    /* output packet */
    eth_hdr_xp   eth_hdr   = (eth_hdr_xp )   tcp_packet;
    ip_hdr_xp    ip_hdr    = (ip_hdr_xp )   (tcp_packet + sizeof(ether_hdr_t));
    udp_hdr_xp   udp_hdr   = (udp_hdr_xp )  (tcp_packet + sizeof(ether_hdr_t) + sizeof(ip_hdr_t));
    bootp_pkt_xp bootp_pkt = (bootp_pkt_xp )((unsigned char *)udp_hdr + sizeof(udp_hdr_t));
    unsigned char *opts    = bootp_pkt->vend;

    /* input packet */
//  eth_hdr_xp   ieth_hdr   = (eth_hdr_xp ) buf;
//  ip_hdr_xp    iip_hdr    = (ip_hdr_xp ) (buf + sizeof(ether_hdr_t));
    udp_hdr_xp   iudp_hdr   = (udp_hdr_xp ) (buf + sizeof(ether_hdr_t) + sizeof(ip_hdr_t));
    bootp_pkt_xp ibootp_pkt = (bootp_pkt_xp )((unsigned char *)iudp_hdr + sizeof(udp_hdr_t));
    unsigned char *iopts    = ibootp_pkt->vend; //ML84 added. Here's the magic number

    unsigned short int plen;

    /****************** FUNCTION ********************/
    /* Set up Bootp */
    memset(bootp_pkt, 0, sizeof(bootp_pkt_t)); //reset content
    bootp_pkt->op      = BOOTP_OP_REQUEST; //same for discovery and request
    bootp_pkt->hwtype  = BOOTP_HWTYPE_ETH; //always 1
    bootp_pkt->hlen    = ETH_ADDR_LEN;     //always 6
    bootp_pkt->secs[1] = 0;//0x64; //why 100?
    memcpy(bootp_pkt->chaddr, my_mac, ETH_ADDR_LEN); //fill client address with current HW address

    bootp_pkt->flags[0] = 0x80;	/* ask for a broadcast XXX: without does not work*/

    dhcp_last_msg = 0; //reset

    if (buf) { //if buffer not empty, then send a REQUEST, otherwise send a DISCOVERY
        //print info
//        int i;
//        dbg_print("\n\n\rbootp::RX udp:"); //ML84 print RX udp packet
//        for (i=0; i<size; i++) {
//            if ((i%16)==0) {
//                dbg_print("\n\r");
//                dbg_printnum(i, 4);
//            }
//            if (i%8 == 0) dbg_print(" ");
//            dbg_printnum(buf[i], 2);
//        }
//        dbg_print("\n\n\r");

        if (memcmp(my_mac, ibootp_pkt->chaddr, ETH_ADDR_LEN)) /* not for me ignore */
            return;
        memcpy(my_ip, ibootp_pkt->yiaddr, IP_ADDR_LEN); //store IP offered by DHCP
        if (dhcp_s == DHCP_REN) { //populate client address field only when renewing
            memcpy(bootp_pkt->ciaddr, my_ip, IP_ADDR_LEN);
        } //CIADDR stays 0 during request
        memcpy(bootp_pkt->xid, ibootp_pkt->xid, BOOTP_XID_LEN);

        //ML84 added: parse ibootp_pkt and check whether it's an OFFER or an ACK
        if (*iopts ==  99) iopts++; else return; //check magic cookie
        if (*iopts == 130) iopts++; else return;
        if (*iopts ==  83) iopts++; else return;
        if (*iopts ==  99) iopts++; else return;
        if (*iopts ==  53) iopts++; else return; //check DHCP message type option
        if (*iopts ==   1) iopts++; else return; //check DHCP message type length (must be 1)

//        ip_known = 1;
//        dhcp_ip_found = 1;

        dhcp_last_msg = *iopts;
        switch (dhcp_last_msg) {
            case 2: //OFFER
                //go on and send REQUEST
                break;
            case 5: //ACK
                ip_known = 1;
                dhcp_ip_found = 1;
                return;
                //break;
            case 6: //NACK
                return;
                //break;
            default:
                dbg_print("send_bootp_pkt::error::unsupported msg\n\r");
                return;
        }

        //parse rest of options
        iopts++;
        while (*iopts != 0xFF) {
            switch(*iopts) {
                case 1: //subnet mask
                    dbg_print("subnet = ");
                    iopts++; //next is size
                    opt_size = *iopts++;
                    if (opt_size == 4) {
                        dbg_printnum(*iopts++,3);
                        dbg_printnum(*iopts++,3);
                        dbg_printnum(*iopts++,3);
                        dbg_printnum(*iopts++,3);
                        dbg_print("\n\r");
                    } else { //ignore
                        dbg_print("bad size\n\r");
                        iopts += opt_size;
                    }
                    break;
                case 3: //router
                    dbg_print("router = ");
                    iopts++; //next is size
                    opt_size = *iopts++;
                    if (opt_size == 4) {
                        dbg_printnum(*iopts++,3);
                        dbg_printnum(*iopts++,3);
                        dbg_printnum(*iopts++,3);
                        dbg_printnum(*iopts++,3);
                        dbg_print("\n\r");
                    } else { //ignore
                        dbg_print("bad size\n\r");
                        iopts += opt_size;
                    }
                    break;
                case 6: //dns
                    dbg_print("DNS = ");
                    iopts++; //next is size
                    opt_size = *iopts++;
                    if (opt_size == 8) {
                        dbg_print("OK\n\r");
                    } else { //ignore
                        dbg_print("bad size\n\r");
                    }
                    iopts += opt_size;
                    break;
                case 15: //domain name
                    dbg_print("domain name = OK\n\r");
                    iopts++; //next is size
                    opt_size = *iopts++;
                    iopts += opt_size;
                    break;
                case 51: //address lease time
                    dbg_print("Lease time = ");
                    iopts++; //next is size
                    opt_size = *iopts++;
                    if (opt_size == 4) {
                        p8 = (uint8_t*)&dhcp_lease_time;
                        p8[3]=*iopts++;
                        p8[2]=*iopts++;
                        p8[1]=*iopts++;
                        p8[0]=*iopts++;
                        dbg_printnum(dhcp_lease_time, 8);
                    } else {
                        iopts += opt_size;
                        dbg_print("bad size");
                    }
                    dbg_print("\n\r");
                    break;
                case 54: //server id
                    dbg_print("DHCP srv id = ");
                    iopts++; //next is size
                    opt_size = *iopts++;
                    if (opt_size == 4) {
                        dhcp_srv_ip[0] = *iopts++;
                        dhcp_srv_ip[1] = *iopts++;
                        dhcp_srv_ip[2] = *iopts++;
                        dhcp_srv_ip[3] = *iopts++;
                        dbg_printnum(dhcp_srv_ip[0],3);
                        dbg_printnum(dhcp_srv_ip[1],3);
                        dbg_printnum(dhcp_srv_ip[2],3);
                        dbg_printnum(dhcp_srv_ip[3],3);
                        dbg_print("\n\r");
                    } else { //ignore
                        dbg_print("bad size\n\r");
                        iopts += opt_size;
                    }
                    break;
                default:
                    iopts++; //next is size
                    opt_size = *iopts++;
                    iopts += opt_size;
            }
        }


    } else { //DISCOVER
    	bootp_pkt->xid[0] = 0x90;
    }

    *opts++ = 99;		/* magic number */
    *opts++ = 130;
    *opts++ = 83;
    *opts++ = 99;
    *opts++ = BOOTP_OPTCODE_DHCP_TYPE; //DHCP type
    *opts++ = 1; //Length

    if (buf) { //REQUEST
        *opts++ = DHCP_TYPE_REQUEST;
    } else { //DISCOVER
    	*opts++ = DHCP_TYPE_DISCOVER;
    }

    //ML84 added DHCP server ID
    *opts++ = 54;
    *opts++ = 4; //length
    *opts++ = dhcp_srv_ip[0];
    *opts++ = dhcp_srv_ip[1];
    *opts++ = dhcp_srv_ip[2];
    *opts++ = dhcp_srv_ip[3];

    //ML84 added Client identifier
    *opts++ = 61;
    *opts++ = 7; //length
    *opts++ = 1; //type (hw address)
    *opts++ = my_mac[0]; //MAC
    *opts++ = my_mac[1]; //MAC
    *opts++ = my_mac[2]; //MAC
    *opts++ = my_mac[3]; //MAC
    *opts++ = my_mac[4]; //MAC
    *opts++ = my_mac[5]; //MAC

    //ML84 added requested address
    *opts++ = 50;
    *opts++ = 4; //length
    *opts++ = my_ip[0];
    *opts++ = my_ip[1];
    *opts++ = my_ip[2];
    *opts++ = my_ip[3];

    //ML84 add hostname
    *opts++ = 12;
    *opts++ = 9; //length
    *opts++ = my_hostname[0];
    *opts++ = my_hostname[1];
    *opts++ = my_hostname[2];
    *opts++ = my_hostname[3];
    *opts++ = my_hostname[4];
    *opts++ = my_hostname[5];
    *opts++ = my_hostname[6];
    *opts++ = my_hostname[7];
    *opts++ = 0;

    *opts++ = BOOTP_OPTCODE_END;


    /* Set up Udp */
    memset(udp_hdr, 0, sizeof(udp_hdr_t));
    udp_hdr->sp[1] = BOOTP_CLIENT_PORT; //source port = DHCP client (68)
    udp_hdr->dp[1] = BOOTP_SERVER_PORT; //destination port = DHCP server (67)
    plen = sizeof(udp_hdr_t) + sizeof(bootp_pkt_t);
    udp_hdr->len[0] = plen >> 8; //write header length
    udp_hdr->len[1] = (unsigned char) plen;
    /* _FIXME leave csum 0 */

    /* Set up IP */
    memset(ip_hdr, 0, sizeof(ip_hdr_t)); //cleanup IP header
    ip_hdr->ver_hlen = 0x45; 	/* IPv4 with 20 byte header */
    plen += sizeof(ip_hdr_t);
    //ip_hdr->tos = 0;
    ip_hdr->tlen[0] = plen >> 8;
    ip_hdr->tlen[1] = (unsigned char) plen;
    ip_hdr->id[0] = ip_id >> 8;
    ip_hdr->id[1] = (unsigned char) ip_id;
    ip_id++;
    ip_hdr->ttl = 128;
    ip_hdr->proto = UDP_PROTO;    
	memset(ip_hdr->da, 0xFF, IP_ADDR_LEN); //destination: broadcast
    fix_checksum((unsigned char *)ip_hdr, sizeof(ip_hdr_t), 10); //checksum, offset 10
    /* Set up Ethernet */
    eth_hdr->type_code[0] = ETH_TYPE_0; //set IPv4
    eth_hdr->type_code[1] = ETH_TYPE_IP_1;
    memcpy(eth_hdr->sa, my_mac, ETH_ADDR_LEN); //copy local MAC as source address
	memset(eth_hdr->da, 0xFF, ETH_ADDR_LEN); /* broadcast */
    num_pkt_tx++;

    MSS_MAC_tx_packet(tcp_packet,plen + sizeof(ether_hdr_t), MSS_MAC_BLOCKING);
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */
void send_dhcp_server_packet (unsigned char *buf)
{
    unsigned char * tcp_packet =  tcp_packet;

    /* output packet */
    //create pointers
    eth_hdr_xp eth_hdr     = (eth_hdr_xp   )  tcp_packet;
    ip_hdr_xp ip_hdr       = (ip_hdr_xp    ) (tcp_packet + sizeof(ether_hdr_t));
    udp_hdr_xp udp_hdr     = (udp_hdr_xp   ) (tcp_packet + sizeof(ether_hdr_t) + sizeof(ip_hdr_t));
    bootp_pkt_xp bootp_pkt = (bootp_pkt_xp )((unsigned char *)udp_hdr + sizeof(udp_hdr_t));
    unsigned char *opts = bootp_pkt->vend;

	/* input packet */
    eth_hdr_xp ieth_hdr     = (eth_hdr_xp ) buf;
//  ip_hdr_xp iip_hdr       = (ip_hdr_xp ) (buf + sizeof(ether_hdr_t));
    udp_hdr_xp iudp_hdr     = (udp_hdr_xp ) (buf + sizeof(ether_hdr_t) + sizeof(ip_hdr_t));
    bootp_pkt_xp ibootp_pkt = (bootp_pkt_xp )((unsigned char *)iudp_hdr + sizeof(udp_hdr_t));
    unsigned char *iopts    = ibootp_pkt->vend;

    unsigned short int plen;

    /* Set up Bootp */
    memset(bootp_pkt, 0, sizeof(bootp_pkt_t)); //reset content of output bootp packet
    bootp_pkt->op      = BOOTP_OP_REPLY; //sent from server after a discover
    bootp_pkt->hwtype  = BOOTP_HWTYPE_ETH;
    bootp_pkt->hlen    = ETH_ADDR_LEN;
    bootp_pkt->secs[1] = 0x64;
    memcpy(bootp_pkt->chaddr, ieth_hdr->sa, ETH_ADDR_LEN); //copy source MAC from received packet to client addr field
    bootp_pkt->flags[0] = 0x00;
    if (buf) { //if received packet contains data...
        memcpy(bootp_pkt->ciaddr, ibootp_pkt->yiaddr, IP_ADDR_LEN); //copy "your ip address" from received packet to "client address"
        memcpy(bootp_pkt->yiaddr, g_client_ip, IP_ADDR_LEN);        //copy hardcoded IP to "your ip address"
        memcpy(bootp_pkt->xid, ibootp_pkt->xid, BOOTP_XID_LEN);     //copy transaction ID
    } else {
    	bootp_pkt->xid[0] = 0x90;
    }
    *opts++ = 99;		/* magic number */
    *opts++ = 130;
    *opts++ = 83;
    *opts++ = 99;
    *opts++ = BOOTP_OPTCODE_DHCP_TYPE; //DHCP MESSAGE TYPE (53 LEN CODE)
    *opts++ = 1; //LEN = 1
    if (iopts[6] == DHCP_TYPE_DISCOVER)
    {
        *opts++ = DHCP_TYPE_OFFER; //send an OFFER if a DISCOVER has been received
    }
    else
    {
        *opts++ = DHCP_TYPE_ACK; //send an ACK if anything else has been received
    }        
    /* Server ID */
    *opts++ = BOOTP_OPTCODE_DHCP_SID; //Served ID (54 4 SERVER_IP)
    *opts++ = 4; //LEN
    *opts++ = my_ip[0];
    *opts++ = my_ip[1];
    *opts++ = my_ip[2];
    *opts++ = my_ip[3];
    /* Lease time (1 hour) */
    *opts++ = BOOTP_OPTCODE_DHCP_LEASE; //LEASE TIME (51 4 SECONDS)
    *opts++ = 4;
    *opts++ = 0x00;
    *opts++ = 0x00;
    *opts++ = 0x0E;
    *opts++ = 0x10; //0xE10 = 360 = 1 hour
    /* Renewal time */
    *opts++ = BOOTP_OPTCODE_DHCP_RENEW; //RENEWAL TIME (58 4 SECONDS)
    *opts++ = 4;
    *opts++ = 0x00;
    *opts++ = 0x00;
    *opts++ = 0x07;
    *opts++ = 0x08; //0x708 = 1800 = 30 min
    /* Rebinding time */
    *opts++ = BOOTP_OPTCODE_DHCP_REBIND; //REBIND TIME (59 4 SECONDS)
    *opts++ = 4;
    *opts++ = 0x00;
    *opts++ = 0x00;
    *opts++ = 0x0C;
    *opts++ = 0x4E; //0xC4E = 3150 = 52 min, 30 s
    /* Subnet mask */
    *opts++ = BOOTP_OPTCODE_DHCP_SUBNET; //SUBNET (1 4 ADDR)
    *opts++ = 4;
    *opts++ = 0xFF;
    *opts++ = 0xFF;
    *opts++ = 0xFF;
    *opts++ = 0x00; //255.255.255.0
    /* Router */
    *opts++ = BOOTP_OPTCODE_DHCP_ROUTER; //ROUTER (3 4 ADDR)
    *opts++ = 4;
    *opts++ = my_ip[0];
    *opts++ = my_ip[1];
    *opts++ = my_ip[2];
    *opts++ = my_ip[3];
    /* Domain */
    *opts++ = BOOTP_OPTCODE_DHCP_DOMAIN; //DOMAIN (6 4 ADDR)
    *opts++ = 4;
    *opts++ = my_ip[0];
    *opts++ = my_ip[1];
    *opts++ = my_ip[2];
    *opts++ = my_ip[3];
    
    *opts++ = BOOTP_OPTCODE_END; //options terminator (0xFF)

    /* Set up Udp */
    memset(udp_hdr, 0, sizeof(udp_hdr_t));
    udp_hdr->sp[1] = BOOTP_SERVER_PORT; //source BOOTP server
    udp_hdr->dp[1] = BOOTP_CLIENT_PORT; //destination BOOTP client
    plen = sizeof(udp_hdr_t) + sizeof(bootp_pkt_t);
    udp_hdr->len[0] = plen >> 8;
    udp_hdr->len[1] = (unsigned char) plen;
    /* leave csum 0 */

    /* Set up IP */
    memset(ip_hdr, 0, sizeof(ip_hdr_t));
    ip_hdr->ver_hlen = 0x45; 	/* IPv4 with 20 byte header */
    plen += sizeof(ip_hdr_t);
    ip_hdr->tlen[0] = plen >> 8;
    ip_hdr->tlen[1] = (unsigned char) plen;
    ip_hdr->id[0] = ip_id >> 8;
    ip_hdr->id[1] = (unsigned char) ip_id;
    ip_id++;
    ip_hdr->ttl = 255;
    ip_hdr->proto = UDP_PROTO;
	memcpy(ip_hdr->sa, my_ip, IP_ADDR_LEN);
	memset(ip_hdr->da, 0xFF, IP_ADDR_LEN);
    fix_checksum((unsigned char *)ip_hdr, sizeof(ip_hdr_t), 10);
    
    /* Set up Ethernet */
    eth_hdr->type_code[0] = ETH_TYPE_0;
    eth_hdr->type_code[1] = ETH_TYPE_IP_1;
    memcpy(eth_hdr->sa, my_mac, ETH_ADDR_LEN);    
	memset(eth_hdr->da, 0xFF, ETH_ADDR_LEN); /* broadcast */
    num_pkt_tx++;
    MSS_MAC_tx_packet(tcp_packet,plen + sizeof(ether_hdr_t), MSS_MAC_BLOCKING);
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */
unsigned char process_udp_packet (unsigned char *buf, int size)
{
    
    udp_hdr_xp udp_hdr = (udp_hdr_xp ) (buf + sizeof(ether_hdr_t) + sizeof(ip_hdr_t));

    if (udp_hdr->dp[1] != BOOTP_CLIENT_PORT) { //destination is not the client
        //dbg_print("dst port /= 68"); //ml84
        //send_dhcp_server_packet( buf ); //ML84 disabled DHCP server
        return OK;
    } else {
        //dbg_print("dst port == 68\n\r"); //ml84
    }

    if (ip_known) {
        return ERR;
    }

    /* some more error checking here? */
    send_bootp_packet(buf, size); //send request
    return OK;
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */

void send_tcp_packet (unsigned char control_bits,unsigned short int buflen) 
{
   
    eth_hdr_xp eth_hdr = (eth_hdr_xp ) tcp_packet;
    ip_hdr_xp  ip_hdr = (ip_hdr_xp ) (tcp_packet + sizeof(ether_hdr_t));
    tcp_hdr_xp  tcp_hdr = (tcp_hdr_xp ) 
	(tcp_packet + sizeof(ether_hdr_t) + sizeof(ip_hdr_t));
    tcp_pseudo_hdr_xp  tcp_pseudo_hdr = (tcp_pseudo_hdr_xp )
	(((unsigned char *)tcp_hdr) - sizeof(tcp_pseudo_hdr_t));
    unsigned char *tcp_data = tcp_packet + sizeof(ether_hdr_t) + sizeof(ip_hdr_t) + sizeof (tcp_hdr_t);
    unsigned char *seqp = (unsigned char *)(&tcb.local_seq);
    unsigned short int plen;
    memset(tcp_hdr, 0, sizeof(tcp_hdr_t));
    memcpy(tcp_hdr->sp, tcb.local_port, TCP_PORT_LEN);
    memcpy(tcp_hdr->dp, tcb.remote_port, TCP_PORT_LEN);
    tcp_hdr->seqnum[0] = seqp[3];
    tcp_hdr->seqnum[1] = seqp[2];
    tcp_hdr->seqnum[2] = seqp[1];
    tcp_hdr->seqnum[3] = seqp[0];
    tcb.local_seq++;
    if (buflen) {
	tcb.local_seq += buflen - 1;
    }
    if (control_bits & TCP_CNTRL_ACK) {
	seqp = (unsigned char *)(&tcb.remote_seq);
	tcp_hdr->acknum[3] = seqp[0];
	tcp_hdr->acknum[2] = seqp[1];
	tcp_hdr->acknum[1] = seqp[2];
	tcp_hdr->acknum[0] = seqp[3];
    }
    tcp_hdr->data_off = 0x50;	/* always 5 32 bit words for us */
    tcp_hdr->urg_ack_psh_rst_syn_fin = control_bits;
    tcp_hdr->wsize[0] = 0x08;	 /* this is 0x0800, which is 2K */
    if (buflen & 1) {
	tcp_data[buflen] = 0;
    }
    /* memset(tcp_pseudo_hdr, 0, sizeof(tcp_pseudo_hdr_t)); */
    memcpy(tcp_pseudo_hdr->sa, my_ip, IP_ADDR_LEN);
    memcpy(tcp_pseudo_hdr->da, tcb.remote_addr, IP_ADDR_LEN);
    tcp_pseudo_hdr->zero = 0;
    tcp_pseudo_hdr->proto = TCP_PROTO;
    plen = buflen + sizeof(tcp_hdr_t);
    tcp_pseudo_hdr->plen[0] = plen >> 8;
    tcp_pseudo_hdr->plen[1] = (unsigned char)plen;
    fix_checksum((unsigned char *)tcp_pseudo_hdr, 
		 (unsigned short int)(plen + sizeof(tcp_pseudo_hdr_t)), (unsigned short int)28);

    memset(ip_hdr, 0, sizeof(ip_hdr_t));

    ip_hdr->ver_hlen = 0x45; 	/* IPv4 with 20 byte header */
    plen += sizeof(ip_hdr_t);	/* add the size of the IP Header */
    ip_hdr->tlen[0] = plen >> 8;
    ip_hdr->tlen[1] = (unsigned char) plen;
    ip_hdr->id[0] = ip_id >> 8;
    ip_hdr->id[1] = (unsigned char) ip_id;
    ip_id++;
    ip_hdr->ttl = 32; 		/* max 32 hops */
    ip_hdr->proto = TCP_PROTO;
    memcpy(ip_hdr->sa, my_ip, IP_ADDR_LEN);
    memcpy(ip_hdr->da, tcb.remote_addr, IP_ADDR_LEN);
    fix_checksum((unsigned char *)ip_hdr, sizeof(ip_hdr_t), 10);
    /* Fix the Ethernet Header */
    eth_hdr->type_code[0] = ETH_TYPE_0;
    eth_hdr->type_code[1] = ETH_TYPE_IP_1;
    memcpy(eth_hdr->sa, my_mac, ETH_ADDR_LEN);
    memcpy(eth_hdr->da, tcb.remote_mac, ETH_ADDR_LEN); /* should be table lookup */
    num_pkt_tx++;    
    MSS_MAC_tx_packet(tcp_packet,plen + sizeof(ether_hdr_t), MSS_MAC_BLOCKING);
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */
unsigned char mac_tcp_init(void)
{
    memset(&tcb,0,sizeof(tcp_control_block_t));
    tcb.state = TCP_STATE_LISTEN;
    ip_id = 0;
    ip_known = 0;
    return OK;
}

/***************************************************************************//**
 *  See tcpip.h for more information.
 */
unsigned char hex_digits_to_byte(unsigned char u, unsigned char l) 
{
    if (u > '9')
	u = u - 'A' + 10;
    else
	u = u - '0';
    if (l > '9')
	l = l - 'A' + 10;
    else
	l = l - '0';
    return (u << 4) + l;
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */
unsigned char process_icmp_packet(unsigned char *buf)
{
    ip_hdr_xp ip_hdr = (ip_hdr_xp ) (buf + sizeof (ether_hdr_t));
    icmp_hdr_xp icmp_hdr = (icmp_hdr_xp ) 
	(buf + sizeof (ether_hdr_t) + sizeof(ip_hdr_t));
    unsigned short int elen = ((unsigned short int)ip_hdr->tlen[0] << 8) + (unsigned short int)ip_hdr->tlen[1] - sizeof(ip_hdr_t);
    if (check_checksum((unsigned char *)icmp_hdr, (unsigned short int) elen, (unsigned short int) 2, 'M') != OK) 
	return ERR;
    if (icmp_hdr->type != ICMP_TYPE_ECHO_REQUEST) {
	return ERR;
    }
    return send_icmp_echo_reply(buf);
}

 /*  See tcpip.h for more information.
 */

/***************************************************************************//**
 *  See tcpip.h for more information.
 */
unsigned char process_tcp_packet(unsigned char *buf)
{
    eth_hdr_xp eth_hdr = (eth_hdr_xp )buf;
    ip_hdr_xp ip_hdr = (ip_hdr_xp ) (buf + sizeof (ether_hdr_t));
    tcp_hdr_xp tcp_hdr = (tcp_hdr_xp ) 
	(buf + sizeof (ether_hdr_t) + sizeof(ip_hdr_t));
    unsigned short int elen = ((unsigned short int)ip_hdr->tlen[0] << 8) + (unsigned short int)ip_hdr->tlen[1] - sizeof(ip_hdr_t);
    unsigned char state;
    if ( !memcmp(tcb.remote_addr, ip_hdr->sa, IP_ADDR_LEN)  && /* same source IP */
	 !memcmp(tcb.remote_port, tcp_hdr->sp, TCP_PORT_LEN) && /* same source port */
	 !memcmp(tcb.local_port, tcp_hdr->dp, TCP_PORT_LEN)) { /* same dest port */
	state = tcb.state;
    } else {			/* copy it over, a new IP wants in */
	memcpy(tcb.remote_addr, ip_hdr->sa, IP_ADDR_LEN);
	memcpy(tcb.remote_port, tcp_hdr->sp, TCP_PORT_LEN);
	memcpy(tcb.local_port, tcp_hdr->dp, TCP_PORT_LEN);
	memcpy(tcb.remote_mac, eth_hdr->sa, ETH_ADDR_LEN);
	state = TCP_STATE_LISTEN;
    } 
    switch (state) {
    case TCP_STATE_LISTEN:
	if (tcp_hdr->urg_ack_psh_rst_syn_fin & TCP_CNTRL_SYN) {
	    /* recd SYN : new connection; send SYN+ACK */      
                      
	    tcb.local_seq = TCP_START_SEQ;
        tcb.remote_seq = 0;
        tcb.remote_seq = (tcb.remote_seq | tcp_hdr->seqnum[0]);
        tcb.remote_seq = ((tcb.remote_seq << 8) |tcp_hdr->seqnum[1]);
        tcb.remote_seq = ((tcb.remote_seq << 8) |tcp_hdr->seqnum[2]);
        tcb.remote_seq = ((tcb.remote_seq << 8) |tcp_hdr->seqnum[3]);    
	    tcb.remote_seq++;
	    send_tcp_packet( TCP_CNTRL_SYN | TCP_CNTRL_ACK, 0);
	    tcb.state = TCP_STATE_SYN_RECVD;
	} 
	break;	
    case TCP_STATE_SYN_RECVD:   
	if (tcp_hdr->urg_ack_psh_rst_syn_fin & TCP_CNTRL_ACK) { 
	    /* recd ack; send nothing */
	    tcb.state = TCP_STATE_ESTABLISHED;
	} 
    else {  
    tcb.state = TCP_STATE_LISTEN;
    }   
	break;
    case TCP_STATE_ESTABLISHED: 
	if (tcp_hdr->urg_ack_psh_rst_syn_fin & TCP_CNTRL_FIN) {
	    /* recd fin; send ack */
	    /* skip CLOSE_WAIT state; send fin along with ack */
        tcb.remote_seq = 0;
        tcb.remote_seq = (tcb.remote_seq | tcp_hdr->seqnum[0]);
        tcb.remote_seq = ((tcb.remote_seq << 8) |tcp_hdr->seqnum[1]);
        tcb.remote_seq = ((tcb.remote_seq << 8) |tcp_hdr->seqnum[2]);
        tcb.remote_seq = ((tcb.remote_seq << 8) |tcp_hdr->seqnum[3]);   
	    tcb.remote_seq++;
	    send_tcp_packet(TCP_CNTRL_ACK | TCP_CNTRL_FIN, 0);
	    tcb.state = TCP_STATE_LAST_ACK;   
	/* Default scroll message on OLED */
	}
    else if (tcp_hdr->dp[0] != 0 || \
		   tcp_hdr->dp[1] != 80) { /* HTTP Port */
	    break;
	}
    else if (elen > sizeof(tcp_hdr_t)) { /* dont respond to empty packets*/
        tcb.remote_seq = 0;
        tcb.remote_seq = (tcb.remote_seq | tcp_hdr->seqnum[0]);
        tcb.remote_seq = ((tcb.remote_seq << 8) |tcp_hdr->seqnum[1]);
        tcb.remote_seq = ((tcb.remote_seq << 8) |tcp_hdr->seqnum[2]);
        tcb.remote_seq = ((tcb.remote_seq << 8) |tcp_hdr->seqnum[3]);   
	    tcb.remote_seq += (unsigned long) (elen - sizeof(tcp_hdr_t));
	    //send_http_response(((unsigned char *)(tcp_hdr)) + sizeof (tcp_hdr_t));
	    tcb.state = TCP_STATE_MY_LAST;	   
	} 
	break;
    case TCP_STATE_MY_LAST: 
	if (tcp_hdr->urg_ack_psh_rst_syn_fin & TCP_CNTRL_FIN) {
				/* sent fin, got fin, ack the fin */
        tcb.remote_seq = 0;
        tcb.remote_seq = (tcb.remote_seq | tcp_hdr->seqnum[0]);
        tcb.remote_seq = ((tcb.remote_seq << 8) |tcp_hdr->seqnum[1]);
        tcb.remote_seq = ((tcb.remote_seq << 8) |tcp_hdr->seqnum[2]);
        tcb.remote_seq = ((tcb.remote_seq << 8) |tcp_hdr->seqnum[3]);   
	    tcb.remote_seq++;
	    send_tcp_packet(TCP_CNTRL_ACK, 0);
	    tcb.state = TCP_STATE_CLOSED;
	}
	break;
    case TCP_STATE_LAST_ACK:
   
	if (tcp_hdr->urg_ack_psh_rst_syn_fin & TCP_CNTRL_ACK) {
	    /* recd ack; send nothing */
	    tcb.state = TCP_STATE_CLOSED;
	}
	/* no break here... go on to CLOSED directly */
    case TCP_STATE_CLOSED:   
	memset (&tcb, 0, sizeof (tcp_control_block_t));
	break;	 
    default:
	break;
    }
    return 0;
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */
unsigned char process_ip_packet(unsigned char *buf, int size)
{
    ip_hdr_xp ip_hdr = (ip_hdr_xp ) (buf + sizeof (ether_hdr_t));

    /* Is the incoming pkt for me?
       (either explicity addressed to me or a broadcast address) */
    if (memcmp(my_ip, ip_hdr->da, IP_ADDR_LEN)) /* not my IP */ {
        if (ip_known) { //already have an IP: don't accept even broadcasts
            return ERR;
        }
        //we still don't have an IP: accept broadcasts
        if (ip_hdr->da[0] != 0xFF || ip_hdr->da[1] != 0xFF ||
            ip_hdr->da[2] != 0xFF || ip_hdr->da[3] != 0xFF) {
            return ERR; //return if not broadcast
        }
    }
    //control checksum
    if (check_checksum((unsigned char *)ip_hdr, (unsigned short int) 20, (unsigned short int) 10, 'I') != OK)
        return ERR;
    //check protocol
    switch (ip_hdr->proto) 
    {
        case TCP_PROTO:
            return process_tcp_packet(buf);
        case ICMP_PROTO:
            return process_icmp_packet(buf);
        case UDP_PROTO: //bootp
            return process_udp_packet(buf, size);
        default: {
            return ERR;
        }
    }
    return ERR;
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */
unsigned char process_arp_packet(unsigned char *buf)
{
    arp_pkt_xp arp_pkt = (arp_pkt_xp )(buf + sizeof(ether_hdr_t));
    if (arp_pkt->opcode[1] != ARP_OPCODE_REQ_1) { 
    if (arp_pkt->opcode[1] == ARP_OPCODE_REPLY_1)
    {    	
    	 if (!memcmp(my_ip, arp_pkt->ip_sa, IP_ADDR_LEN))
    	 {	 	 
    		 //printf("IP conflict with MAC");
    		 //printf("%02x:%02x:%02x:%02x:%02x:%02x",arp_pkt->mac_sa[0],arp_pkt->mac_sa[1],arp_pkt->mac_sa[2],arp_pkt->mac_sa[3],arp_pkt->mac_sa[4],arp_pkt->mac_sa[5]);
    	 }    	 	
    }
	return ERR;
    }   
    if (memcmp(my_ip, arp_pkt->ip_ta, IP_ADDR_LEN)) {
	return ERR;
    }   
    return send_arp_reply(buf);
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */
unsigned char process_packet( unsigned char * buf, int size )
{
    eth_hdr_xp eth_hdr;
    unsigned char typ;
    eth_hdr = (eth_hdr_xp ) buf;
    typ = eth_hdr->type_code[0];
    if (typ != ETH_TYPE_0)
    {
		return ERR;
    }
    typ = eth_hdr->type_code[1];
    if (typ == ETH_TYPE_ARP_1) 
    {
		return process_arp_packet(buf);
    }
    else if (typ == ETH_TYPE_IP_1) {
		return process_ip_packet(buf, size);
    }   
    else
    {
		return ERR;
    }
    return ERR;
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */
unsigned char *xstrcpy(unsigned char *d, const unsigned char *s)
{
    unsigned char c;

    while ((c = *s++)) 
    (*d++ = c) ;
    return d;
}
/***************************************************************************//**
 *  See tcpip.h for more information.
 */
// updated html page with fusion board link on page:

