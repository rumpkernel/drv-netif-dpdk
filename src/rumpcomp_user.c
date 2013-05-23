/*-
 * Copyright (c) 2013 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This is a very simple hypercall layer for the rump kernel if_virt
 * to plug into DPDK.  It does not attempt to be fast.  It attempts
 * to 1) work 2) be quick to implement.
 */

#include <sys/types.h>
#include <sys/uio.h>

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <rte_config.h>
#include <rte_common.h>
#include <rte_eal.h>
#include <rte_pci.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>

#include <rump/rumpuser_component.h>

/*
 * Configurables.  Adjust these to be appropriate for your system.
 */

/* change blacklist parameters (-b) if necessary */
static const char *ealargs[] = {
	"if_virt",
	"-b 00:00:03.0",
	"-c 1",
	"-n 1",
};

/* change PORTID to the one your want to use */
#define IF_PORTID 0

/* change to the init method of your NIC driver */
#define PMD_INIT rte_wm_pmd_init


/*
 * No (immediate) need to edit below this line
 */

#define MBSIZE	2048
#define MBALIGN	32
#define NMBUF	8192
#define NDESC	256
#define NQUEUE	1

/* these thresholds don't matter at this stage of optimizing */
static const struct rte_eth_rxconf rxconf = {
	.rx_thresh = {
		.pthresh = 1,
		.hthresh = 1,
		.wthresh = 1,
	},
};
static const struct rte_eth_txconf txconf = {
	.tx_thresh = {
		.pthresh = 1,
		.hthresh = 1,
		.wthresh = 1,
	},
};

static struct rte_mempool *mbpool;

struct virtif_user {
	int only_one_interface_allowed;
};

#define OUT(a) do { printf(a) ; goto out; } while (/*CONSTCOND*/0)
static int
globalinit(void)
{
	int rv;

	if (rte_eal_init(sizeof(ealargs)/sizeof(ealargs[0]),
	    /*UNCONST*/(void *)(uintptr_t)ealargs) < 0)
		OUT("eal init\n");

	if ((mbpool = rte_mempool_create("mbuf_pool", NMBUF, MBSIZE, MBALIGN,
	    sizeof(struct rte_pktmbuf_pool_private),
	    rte_pktmbuf_pool_init, NULL, rte_pktmbuf_init, NULL, 0, 0)) == NULL)
		OUT("mbuf pool\n");

	if (PMD_INIT() < 0)
		OUT("wm driver\n");
	if (rte_eal_pci_probe() < 0)
		OUT("PCI probe\n");
	if (rte_eth_dev_count() == 0)
		OUT("no ports\n");
	rv = 0;

 out:
 	return rv;
}

int
rumpcomp_virtif_create(int devnum, struct virtif_user **viup)
{
	struct rte_eth_conf portconf;
	struct rte_eth_link link;
	int rv = EINVAL; /* XXX: not very accurate ;) */

	/* this is here only for simplicity */
	if (globalinit() != 0)
		goto out;

	memset(&portconf, 0, sizeof(portconf));
	if (rte_eth_dev_configure(IF_PORTID, NQUEUE, NQUEUE, &portconf) < 0)
		OUT("configure device\n");

	if (rte_eth_rx_queue_setup(IF_PORTID, 0, NDESC, 0, &rxconf, mbpool) <0)
		OUT("rx queue setup\n");

	if (rte_eth_tx_queue_setup(IF_PORTID, 0, NDESC, 0, &txconf) < 0)
		OUT("tx queue setup\n");

	if (rte_eth_dev_start(IF_PORTID) < 0)
		OUT("device start\n");

	rte_eth_link_get(IF_PORTID, &link);
	if (!link.link_status) {
		printf("warning: virt link down\n");
	}

	rte_eth_promiscuous_enable(IF_PORTID);
	rv = 0;

 out:
	*viup = NULL; /* not used by the driver in its current state */
	return rv;
}

/*
 * Get mbuf off of interface, copy it into memory provided by the
 * TCP/IP stack.  TODO: share TCP/IP stack mbufs with DPDK mbufs to avoid
 * data copy.
 */
int
rumpcomp_virtif_recv(struct virtif_user *viu,
	void *data, size_t dlen, size_t *rcvp)
{
	void *cookie = rumpuser_component_unschedule();
	uint8_t *p = data;
	struct rte_mbuf *m, *m0;
	struct rte_pktmbuf *mp;
	int nb_rx, rv;

	for (;;) {
		nb_rx = rte_eth_rx_burst(IF_PORTID, 0, &m, 1);

		if (nb_rx) {
			assert(nb_rx == 1);

			mp = &m->pkt;
			if (mp->pkt_len > dlen) {
				/* for now, just drop packets we can't handle */
				printf("warning: virtif recv packet too big "
				    "%d vs. %zu\n", mp->pkt_len, dlen);
				rte_pktmbuf_free(m);
				continue;
			}
			*rcvp = mp->pkt_len;
			m0 = m;
			do {
				mp = &m->pkt;
				memcpy(p, mp->data, mp->data_len);
				p += mp->data_len;
			} while ((m = mp->next) != NULL);
			rte_pktmbuf_free(m0);
			rv = 0;
			break;
		} else {
			usleep(10000); /* XXX: don't 100% busyloop */ 
		}
	}

	rumpuser_component_schedule(cookie);
	return rv;
}

/*
 * To send, we copy the data from the TCP/IP stack memory into DPDK
 * memory.  TODO: share TCP/IP stack mbufs with DPDK mbufs to avoid
 * data copy.
 */
void
rumpcomp_virtif_send(struct virtif_user *viu,
	struct iovec *iov, size_t iovlen)
{
	void *cookie = rumpuser_component_unschedule();
	struct rte_mbuf *m;
	void *dptr;
	unsigned i;

	m = rte_pktmbuf_alloc(mbpool);
	for (i = 0; i < iovlen; i++) {
		dptr = rte_pktmbuf_append(m, iov[i].iov_len);
		if (dptr == NULL) {
			/* log error somehow? */
			rte_pktmbuf_free(m);
			goto out;
		}
		memcpy(dptr, iov[i].iov_base, iov[i].iov_len);
	}
	rte_eth_tx_burst(IF_PORTID, 0, &m, 1);

 out:
	rumpuser_component_schedule(cookie);
}

void
rumpcomp_virtif_dying(struct virtif_user *viu)
{

	printf("I'M INVINCIBLE!\n");
}

void
rumpcomp_virtif_destroy(struct virtif_user *viu)
{

	printf("you're a loonie!\n");
}
