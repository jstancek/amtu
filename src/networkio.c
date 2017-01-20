//----------------------------------------------------------------------
//
// Module Name:  networkio.c
//
// Include File:  none
//
// Description:   Code for Abstract Machine Test I/O Controller-Network test.
//
// Notes:  This module performs the I/O Controller-Network tests
//              to ensure random data transmitted is data received 
//              for each configured network device.
// -----------------------------------------------------------------
// LANGUAGE:     C
//
// (C) Copyright International Businesses Machine Corp. 2003,2005
// Licensed under the Common Public License v. 1.0
// -----------------------------------------------------------------
//
// Change Activity:
// DATE     PGMR      COMMENTS
// -------- --------- ----------------------
// 08/25/03  JML       Added prolog, comments
// 10/17/03  KSimon    Removed NO_TAG
// 02/06/04  KSimon    Modified return code and error messages in
//		       networkio for get_interfaces return
// 03/15/05  D.Velarde Added AUDIT_LOG statements to be used if we're
//                     using libaudit instead of liblaus
// 06/12/09  JML       Added IPv6 support
//
// -----------------------------------------------------------------

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <ctype.h>
#include <syslog.h>
#include "amtu.h"

#define MAXMSGSIZE	512
#define MAX_INTERFACES	32
#define MSGSIZE		512
#define MAXTRIES	5

struct interface_info {
	unsigned int ifindex;
	char *ifname;
	unsigned char lladdr[14];
};

struct interface_info *interface_list[MAX_INTERFACES];
int ifcount;
char msgstr[MSGSIZE];

/****************************************************************/
/*								*/
/* FUNCTION: cleanup						*/
/*								*/
/* PURPOSE: Free memory malloc'd for interface list		*/
/*								*/
/****************************************************************/
void cleanup()
{
	int i;

	for (i = 0; i < ifcount; i++)
		if (interface_list[i]) {
			free(interface_list[i]->ifname);
			free(interface_list[i]);
		}
}

/****************************************************************/
/*								*/
/* FUNCTION: send_packet					*/
/* 								*/
/* PURPOSE: Prepare and send packet containing randmon string	*/ 
/* 	    to network device.Use PF_PACKET so we can talk	*/
/* 	    directly to the network device.			*/
/*								*/
/****************************************************************/
int send_packet(struct interface_info *iff)
{
	int ssock_fd;
	int numc, len;
	struct sockaddr_ll send_info;
	
	/* PF_PACKET requires generic sockaddr_ll structure */

	memset(&send_info, 0, sizeof(send_info));
	send_info.sll_family = AF_PACKET;
	send_info.sll_ifindex = iff->ifindex;
	send_info.sll_protocol = htons(ETH_P_LOOP);
	send_info.sll_halen = sizeof(send_info.sll_addr);
	memcpy(send_info.sll_addr, iff->lladdr, sizeof(send_info.sll_addr));
	
	ssock_fd = socket(PF_PACKET, SOCK_DGRAM, 0);

	if (ssock_fd < 0) {
		perror("send_packet:socket() failed");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed network I/O tests on creating PF_PACKET socket"));
#else
		AUDIT_LOG("amtu failed network I/O tests on creating PF_PACKET socket", 0);
#endif
		return(-1) ;
	}
	
	len = sizeof(send_info);	
	if (bind(ssock_fd, (struct sockaddr *)&send_info, len) < 0) {
		perror("send_packet:bind() failed");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed network I/O test on bind to a socket"));
#else
		AUDIT_LOG("amtu failed network I/O test on bind to a socket", 0);
#endif
		close(ssock_fd);
		return(-1);
	}

	/* send the random message on the interface under test */
	numc = sendto(ssock_fd, msgstr, sizeof(msgstr), 0, 
		      (struct sockaddr *)&send_info, sizeof(send_info));
	if (numc < 0) {
		perror("send_packet:sendto() failed");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed network I/O tests on sendto()"));
#else
		AUDIT_LOG("amtu failed network I/O tests on sendto()", 0);
#endif
		close(ssock_fd);
		return(-1);
	}
	
	close(ssock_fd);
	return 0;
}
		
/****************************************************************/
/*								*/
/* FUNCTION: get_interfaces					*/
/*								*/
/* PURPOSE: Get list of network interfaces from the kernel.	*/
/*	    AMTU will test only configured ethernet and token 	*/
/*	    ring interfaces.					*/
/*								*/
/****************************************************************/
int get_interfaces()
{
	struct ifaddrs *ifaddrs, *ifa;
	int i, j = 0;
	
	if (getifaddrs(&ifaddrs) == -1) {
		perror("get_interfaces: getifaddrs failed");	
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu failed network I/O tests in getifaddrs()"));	
#else
		AUDIT_LOG("amtu failed network I/O tests in getifaddrs()", 0);	
#endif
		return -1;
	}
	
	/* Find ipv4 and ipv6 configured interfaces */
	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
	
		struct interface_info *np;
		int found = 0;

		/* only testing ethernet and tokenring */
		if ((strncmp(ifa->ifa_name, "eth", 3) != 0) &&
		    (strncmp(ifa->ifa_name, "tr", 2) != 0))
			continue;

		/* check family */
		if (ifa->ifa_addr->sa_family != AF_INET && 
	            ifa->ifa_addr->sa_family != AF_INET6)
			continue;
		/* interface needs to be up and operative */
		if ((ifa->ifa_flags & (IFF_UP|IFF_RUNNING)) == 0)
			continue;
		if (ifa->ifa_flags & IFF_LOOPBACK)
			continue;
		if ((ifa->ifa_flags &
		    (IFF_MULTICAST|IFF_BROADCAST|IFF_POINTOPOINT)) == 0)
			continue;
			
		/* Any interface that makes it up to now can be added to
 		 * our interface list. 
 		 */

		/* Note: We test the network device.
 		 * Thus we use the MAC address associated with the configured
 		 * interfaces. If an interface has both ipv4 and ipv6
 		 * we could have it in our list twice, so check for duplicates.
 		 */
 		 
		for (i = 0; i < j; i++) {
			if (strncmp(interface_list[i]->ifname, ifa->ifa_name, 
			    sizeof(ifa->ifa_name)) == 0) {
				found = 1;
				break;
			}
		}
			
		/* add to our list if it hasn't been added yet. */
		if (!found) {
			np = (struct interface_info *)malloc(sizeof(*np));
			if (np == NULL) {
				fprintf(stderr, "get_interfaces: malloc failed\n");
#ifdef HAVE_LIBLAUS
				LAUS_LOG(("amtu failed network I/O test in malloc"));
#else
				AUDIT_LOG("amtu failed network I/O test in malloc", 0);
#endif
				freeifaddrs(ifaddrs);
				return -1;
			}	 
	
			np->ifindex = if_nametoindex(ifa->ifa_name);
			/* need to malloc size of string plus null terminator */
			np->ifname = (char *)malloc(sizeof(ifa->ifa_name) + 1);
			if (np->ifname == NULL) {
				fprintf(stderr,"get_interfaces:malloc np->ifname failed\n");
#ifdef HAVE_LIBLAUS
				LAUS_LOG(("amtu failed network I/O test in malloc np->ifname"));
#else
				AUDIT_LOG("amtu failed network I/O test in malloc np->ifname", 0);
#endif
				freeifaddrs(ifaddrs);
				free(np);
				return -1;	
			}
			memset(np->ifname, 0, sizeof(ifa->ifa_name) + 1);
			strncpy(np->ifname, ifa->ifa_name, sizeof(ifa->ifa_name));
			interface_list[j++] = np;
		}
	}                                                         

	/*
 	 * Step through interface list again and get link address for
 	 * configured devices in our interface_list.
	 * Note: For each device, getifaddrs pulls up a list of ip addresses
	 * configured on the device and the link address. The family
	 * for link address is AF_PACKET.  
 	 */
	
	for (i = 0; i < j; i++) {
	                     
		for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
			if ((ifa->ifa_addr->sa_family == AF_PACKET) &&
			    strncmp(ifa->ifa_name, interface_list[i]->ifname, 
			     sizeof(ifa->ifa_name))) {
				/* found the link address, copy it. */
				memcpy(interface_list[i]->lladdr, ifa->ifa_addr->sa_data, 14);
				break;
			}
		}
		
		/* If could not find link address, return with an error. */
		if (interface_list[i]->lladdr == NULL) {
			fprintf(stderr, "no link address for %s.\n", interface_list[i]->ifname);
#ifdef HAVE_LIBLAUS
			LAUS_LOG(("amtu failed network I/O test, could not find a link address"));
#else
			AUDIT_LOG("amtu failed network I/O test, could not find a link address", 0);
#endif
			freeifaddrs(ifaddrs);
			return -1;
		}
	}
			
	/* done getting address stuff */
	freeifaddrs(ifaddrs);
	return j;
}

/****************************************************************/
/*								*/
/* FUNCTION: networkio						*/
/*								*/
/* PURPOSE: Send a packet containing some random data to 	*/
/*	    each configured network device. Open a PF_SOCKET    */
/*	    to receive the data and verify it is the same data 	*/
/*	    that was sent.					*/
/*								*/
/****************************************************************/
int networkio(int argc, char *argv[])
{
	int i, rtn, len, cc, rsock_fd;
	int count, failures = 0;
	long int rnd;
	char c;
	char packetbuf[MAXMSGSIZE];
	struct sockaddr_in from;
	
	printf("Executing Network I/O Tests...\n");

	/* get a list of interfaces for this machine. */

	ifcount = get_interfaces();
	if (ifcount <= 0) {
		fprintf(stderr, "Failed to get list of network interfaces to test.\n");
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu: Failed to get list of network interfaces to test.\n"));
#else
		AUDIT_LOG("amtu: Failed to get list of network interfaces to test", 0);
#endif
		return -1;
	}
	if (debug) {
		printf("\nInterface list to test:\n");
		for (i = 0; i < ifcount; i++) {
			printf("   %s\n", interface_list[i]->ifname);
		}
	}	
	
	/* Now get a pseudo-random string to send to each interface. 
	 * This will be the data in the message we send and receive.
	 */
	i = 0;
	srandom(time(NULL));
	while (i < MSGSIZE) {
		rnd = random();
		c = (char)(rnd % 128);
		if (isprint(c)) {
			msgstr[i++] = (char) c;
		}
	}
	if (debug)
		printf("\nmessage string: %s\n", msgstr);
	
	
	/* 
	 * For each interface in the list, send a packet containing
	 * the random data. On same interface receive packets. Check
	 * that we receive a packet containing the randon data sent.
	 */

	for (i = 0; i < ifcount; i++) {

		struct sockaddr_ll rcv_info;
		int rcvlen;
		
		if (debug)
			printf("\nBeginning test for %s\n", 
				interface_list[i]->ifname);
		
		/* Setup buffers and sockaddr_ll structure required
		 * by PF_PACKET for receiving. Also open and bind socket.
		 */
		memset(packetbuf, 0, sizeof(packetbuf));
		memset(&from, 0, sizeof(from));
		len = sizeof(from);
		rsock_fd = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_LOOP));
		if (rsock_fd < 0) {
			perror("networkio:socket() failed");
#ifdef HAVE_LIBLAUS
			LAUS_LOG(("amtu failed network I/O tests on creating PF_PACKET socket"));
#else
			AUDIT_LOG("amtu failed network I/O tests on creating PF_PACKET socket", 0);
#endif
			return(-1) ;
		}
		
		memset(&rcv_info, 0, sizeof(rcv_info));
		rcv_info.sll_family = AF_PACKET;
		rcv_info.sll_ifindex = interface_list[i]->ifindex;
		rcv_info.sll_protocol = htons(ETH_P_ALL);
		
		/* bind socket to interface so that we receive packets
		 * destined only for this interface.
		 */
		rcvlen = sizeof(rcv_info);
		if (bind(rsock_fd, (struct sockaddr *)&rcv_info, rcvlen) < 0) {
			perror("networkio:bind failed");
#ifdef HAVE_LIBLAUS
			LAUS_LOG(("amtu failed network I/O test on bind to socket"));
#else
			AUDIT_LOG("amtu failed network I/O test on bind to socket", 0);
#endif
			close(rsock_fd);
			failures++;
			if (debug)
				printf("Interface %s failed test.\n",
					interface_list[i]->ifname);
			continue;
		}			
			
		/* Now send a packet on this interface. */	
		
		rtn = send_packet(interface_list[i]);
		if (rtn < 0) {
			failures++;
			if (debug)
				printf("Interface %s failed test.\n",
					interface_list[i]->ifname);
			continue;
		}
		
		/* Now receive packet. 
		 * I think there's a chance we could miss our packet...
		 * Set flag MSG_DONTWAIT in recvfrom so that we do not
		 * sit and wait. Try MAXTRIES to receive packet with our
		 * random data.  
		 */
		count = 0;	
		do {
			len = sizeof(from);
			cc = recvfrom(rsock_fd, packetbuf, sizeof(packetbuf),
				MSG_DONTWAIT, (struct sockaddr *)&from, 
				(socklen_t *)&len);
			
			if (debug && (cc > 0))
				printf("Received: %s\n", packetbuf);
		} while (cc != -1 && 
			(strncmp(msgstr, packetbuf, sizeof(msgstr)) != 0) &&
			   ++count < MAXTRIES);
			
		if (cc == -1 || count == MAXTRIES) {
			if(debug)
				printf("recvfrom failed to receive message on %s\n", 
					interface_list[i]->ifname);
#ifdef HAVE_LIBLAUS
			LAUS_LOG(("amtu failed network I/O test in recvfrom"));
#else
			AUDIT_LOG("amtu failed network I/O test in recvfrom", 0);
#endif
			failures++;
		} else {
			if (debug)
				printf("Interface %s passed test.\n", 
					interface_list[i]->ifname);
		}	

	}	

	/* cleanup before terminating */
	cleanup();
	if (!failures) {
		printf("Network I/O Controller Test SUCCESS!\n");	
#ifdef HAVE_LIBLAUS
		LAUS_LOG(("amtu - Network I/O Controller Test succeeded"));
#else
		AUDIT_LOG("amtu - Network I/O Controller Test succeeded", 1);
#endif
		return(0);
	} 
	
	printf("Network I/O Controller Test Failed.\n");
#ifdef HAVE_LIBLAUS
	LAUS_LOG(("amtu - Network I/O Controller Test failed"));
#else
	AUDIT_LOG("amtu - Network I/O Controller Test failed", 0);
#endif
	return(-1);
}
