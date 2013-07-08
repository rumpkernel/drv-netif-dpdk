/*
 * Hah, made you look!  Maybe this is not a browser, but it can fetch one
 * web page using a tcp/ip service provided by a local rump kernel.
 */

#include <sys/types.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rump/rump.h>
#include <rump/netconfig.h>
#include <rump/rump_syscalls.h>

#include <netinet/in.h>

#define DESTHOST "www.netbsd.org"

static void __attribute__((__noreturn__))
die(int e, const char *msg)
{

	if (msg)
		warnx("%s: %d", msg, e);
	rump_sys_reboot(0, NULL);
	exit(e);
}

int
main()
{
	struct sockaddr_in sin;
	char buf[65535];
	struct hostent *hp;
	ssize_t nn;
	ssize_t off;
	int s, e;

	hp = gethostbyname(DESTHOST);
	if (!hp || hp->h_addrtype != AF_INET)
		errx(1, "failed to resolve \"%s\"", DESTHOST);

	rump_init();

	if ((e = rump_pub_netconfig_ifcreate("dpdk0")) != 0)
		die(e, "create virt0");
	if ((e = rump_pub_netconfig_dhcp_ipv4_oneshot("dpdk0")) != 0)
		die(e, "dhcp address");

	s = rump_sys_socket(PF_INET, SOCK_STREAM, 0);
	if (s == -1)
		die(errno, "socket");
	
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
#if 0
	sin.sin_len = sizeof(sin);
#endif
	sin.sin_port = htons(80);
	memcpy(&sin.sin_addr, hp->h_addr, sizeof(sin.sin_addr));

	if (rump_sys_connect(s, (struct sockaddr *)&sin, sizeof(sin)) == -1)
		die(errno, "connect");
	printf("connected\n");

#define WANTHTML "GET / HTTP/1.1\nHost: www.netbsd.org\n\n"
	nn = rump_sys_write(s, WANTHTML, sizeof(WANTHTML)-1);
	printf("write rv %zd\n", nn);

	for (;;) {
		nn = rump_sys_read(s, buf, sizeof(buf)-1);
		if (nn == -1)
			die(errno, "read failed");
		if (nn == 0)
			break;
		
		buf[nn] = '\0';
		printf("%s", buf);
	}
	rump_sys_close(s);

	die(0, NULL);
}
