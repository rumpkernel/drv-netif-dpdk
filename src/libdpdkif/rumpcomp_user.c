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
 * This is a very simple hypercall layer for the rump kernel
 * to plug into DPDK.  Current status: no known bugs.
 */

#include <sys/types.h>
#include <sys/uio.h>

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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

#include "if_virt.h"
#include "rumpcomp_user.h"

/*
 * Configurables.  Adjust these to be appropriate for your system.
 */

/* change blacklist parameters (-b) if necessary */
static const char *ealargs[] = {
	"if_dpdk",
	"-b 00:00:03.0",
	"-c 1",
	"-n 1",
};

/* change PORTID to the one your want to use */
#define IF_PORTID 0

/* change to the init method of your NIC driver */
#ifndef PMD_INIT
#define PMD_INIT rte_igb_pmd_init
#endif

/* Receive packets in bursts of 16 per read */
#define MAX_PKT_BURST 16

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
	/* burst receive context */
	struct rte_mbuf *m_pkts[MAX_PKT_BURST];
	int rcv_buffered_packets;
	int current_index_in_rcv_buffer;
};

#define OUT(a) do { printf(a) ; goto out; } while (/*CONSTCOND*/0)
static int
globalinit(void)
{
	int rv;

	if ((rv = rte_eal_init(sizeof(ealargs)/sizeof(ealargs[0]),
	    /*UNCONST*/(void *)(uintptr_t)ealargs)) < 0)
		OUT("eal init\n");

	if ((mbpool = rte_mempool_create("mbuf_pool", NMBUF, MBSIZE, MBALIGN,
	    sizeof(struct rte_pktmbuf_pool_private),
	    rte_pktmbuf_pool_init, NULL,
	    rte_pktmbuf_init, NULL, 0, 0)) == NULL) {
		rv = -EINVAL;
		OUT("mbuf pool\n");
	}

	if ((rv = PMD_INIT()) < 0)
		OUT("pmd init\n");
	if ((rv = rte_eal_pci_probe()) < 0)
		OUT("PCI probe\n");
	if ((rv = rte_eth_dev_count()) == 0)
		OUT("no ports\n");
	rv = 0;

 out:
 	return rv;
}

int
VIFHYPER_CREATE(int devnum, struct virtif_user **viup, uint8_t *enaddr)
{
	struct rte_eth_conf portconf;
	struct rte_eth_link link;
	struct ether_addr ea;
	struct virtif_user *viu;
	int rv = EINVAL; /* XXX: not very accurate ;) */

	/* this is here only for simplicity */
	if ((rv = globalinit()) != 0)
		goto out;

	memset(&portconf, 0, sizeof(portconf));
	if ((rv = rte_eth_dev_configure(IF_PORTID,
	    NQUEUE, NQUEUE, &portconf)) < 0)
		OUT("configure device\n");

	if ((rv = rte_eth_rx_queue_setup(IF_PORTID,
	    0, NDESC, 0, &rxconf, mbpool)) <0)
		OUT("rx queue setup\n");

	if ((rv = rte_eth_tx_queue_setup(IF_PORTID, 0, NDESC, 0, &txconf)) < 0)
		OUT("tx queue setup\n");

	if ((rv = rte_eth_dev_start(IF_PORTID)) < 0)
		OUT("device start\n");

	rte_eth_link_get(IF_PORTID, &link);
	if (!link.link_status) {
		printf("warning: virt link down\n");
	}

	rte_eth_promiscuous_enable(IF_PORTID);
	rte_eth_macaddr_get(IF_PORTID, &ea);
	memcpy(enaddr, ea.addr_bytes, ETHER_ADDR_LEN);

	viu = malloc(sizeof(*viu));
	memset(viu, 0, sizeof(*viu));
	*viup = viu;

	rv = 0;

 out:
	return rumpuser_component_errtrans(-rv);
}

/*
 * Get mbuf off of interface, copy it into memory provided by the
 * TCP/IP stack.  TODO: share TCP/IP stack mbufs with DPDK mbufs to avoid
 * data copy.
 */
static void
deliverframe(struct virtif_user *viu, void *data, size_t dlen, size_t *rcvp)
{
	struct rte_mbuf *m, *m0;
	struct rte_pktmbuf *mp;
	uint8_t *p = data;

	assert(viu->rcv_buffered_packets > 0);
	m = viu->m_pkts[viu->current_index_in_rcv_buffer];
	viu->current_index_in_rcv_buffer++;
	viu->rcv_buffered_packets--;

	mp = &m->pkt;
	if (mp->pkt_len > dlen) {
		/* for now, just drop packets we can't handle */
		printf("warning: virtif recv packet too big "
		    "%d vs. %zu\n", mp->pkt_len, dlen);
		rte_pktmbuf_free(m);
		return;
	}
	*rcvp = mp->pkt_len;
	m0 = m;
	do {
		mp = &m->pkt;
		memcpy(p, mp->data, mp->data_len);
		p += mp->data_len;
	} while ((m = mp->next) != NULL);
	rte_pktmbuf_free(m0);
}

int
VIFHYPER_RECV(struct virtif_user *viu,
	void *data, size_t dlen, size_t *rcvp)
{
	void *cookie;

	/* fastpath, we have cached frames */
	if (viu->rcv_buffered_packets > 0) {
		deliverframe(viu, data, dlen, rcvp);
		return 0;
	}
		
	/* none cached.  ok, try to get some */
	cookie = rumpuser_component_unschedule();
	for (;;) {
		if (viu->rcv_buffered_packets == 0) {
			viu->rcv_buffered_packets = rte_eth_rx_burst(IF_PORTID,
			    0, viu->m_pkts, MAX_PKT_BURST);
			viu->current_index_in_rcv_buffer = 0;
		}
		
		if (viu->rcv_buffered_packets > 0) {
			deliverframe(viu, data, dlen, rcvp);
			break;
		} else {
			usleep(10000); /* XXX: don't 100% busyloop */ 
		}
	}

	rumpuser_component_schedule(cookie);
	return 0; /* XXX */
}

/*
 * To send, we copy the data from the TCP/IP stack memory into DPDK
 * memory.  TODO: share TCP/IP stack mbufs with DPDK mbufs to avoid
 * data copy.
 */
void
VIFHYPER_SEND(struct virtif_user *viu,
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
VIFHYPER_DYING(struct virtif_user *viu)
{

	abort();
}

void
VIFHYPER_DESTROY(struct virtif_user *viu)
{

	abort();
}
