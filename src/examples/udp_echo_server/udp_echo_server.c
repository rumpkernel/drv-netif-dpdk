/*
 * UDP echo server - was tested with RUMP and DPDK integration
 *
 */

#include <sys/types.h>

#include <assert.h>
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>

#include <rump/rump.h>
#include <rump/netconfig.h>
#include <rump/rump_syscalls.h>

#include <netinet/in.h>
#include <sched.h>

#define IF_ADDR "10.0.0.2"
#define ECHOSERVER_RX_PORT 1111

#define IFNAME "dpdk0"

int
main()
{
	struct sockaddr_in client;
	struct sockaddr_in server;
	char msg[500];
	char buf[65535];
	socklen_t clen;
	ssize_t nn;
	ssize_t off;
	int size = 1024 * 1024;
	int s;


	rump_init();
	rump_pub_lwproc_rfork(0);
	if (rump_pub_netconfig_ifcreate(IFNAME) != 0)
		errx(1, "failed to create " IFNAME);

	if (rump_pub_netconfig_ipv4_ifaddr(IFNAME,
	    IF_ADDR, "255.255.255.0") != 0)
		errx(1, "failed to create " IFNAME);

	//rump_pub_netconfig_dhcp_ipv4_oneshot(IFNAME);


	s = rump_sys_socket(PF_INET, SOCK_DGRAM, 0);
	if (s == -1)
		err(1,"socket");

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(ECHOSERVER_RX_PORT);  
	server.sin_addr.s_addr = inet_addr(IF_ADDR);

	rump_sys_setsockopt(s, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

	if (rump_sys_bind(s, (struct sockaddr *)&server, sizeof(server)) == -1)
		err(1, "binded");
	printf("socket binded\r\n");

	printf("Waiting connection on port = %d\r\n",ECHOSERVER_RX_PORT);

	memset(&client, 0, sizeof(client));

	while(1)
	{
		memset(msg,0,sizeof(msg));
		clen = sizeof(client);

		nn = rump_sys_recvfrom(s,msg,sizeof(msg),0,(struct sockaddr *)&client,&clen);


		if (nn<0) {
			perror("Error receiving data");
		} else {
			rump_sys_sendto(s,buf,nn,0,(struct sockaddr *)&client,clen);
		}
	}
}
