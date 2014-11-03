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
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rte_config.h>
#include <rte_common.h>
#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_pci.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>

#include <rump/rumpuser_component.h>

#include "if_virt.h"
#include "virtif_user.h"

#if VIFHYPER_REVISION != 20141104
#error VIFHYPER_REVISION mismatch
#endif

#include "configuration.h"

/*
 * No (immediate) need to edit below this line
 */

#define MBSIZE	2048
#define MBCACHE	32
#define NDESCTX 512
#define NDESCRX 256
#define NMBUF_BURST_SURPLUS 512
#define NMBUF_TX (NDESCTX + NMBUF_BURST_SURPLUS)
#define NMBUF_RX (NDESCRX + NMBUF_BURST_SURPLUS)
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
	.tx_rs_thresh = 1,
	.txq_flags = ETH_TXQ_FLAGS_NOMULTSEGS | ETH_TXQ_FLAGS_NOOFFLOADS,
};

static struct rte_mempool *mbpool_rx, *mbpool_tx;

struct virtif_user {
	char *viu_devstr;
	uint8_t viu_port_id;
	int viu_dying;
	struct virtif_sc *viu_virtifsc;
	pthread_t viu_rcvpt;

	/* burst receive context */
	struct rte_mbuf *viu_m_pkts[MAX_PKT_BURST];
	int viu_nbufpkts;
	int viu_bufidx;
};

static void
ifwarn(struct virtif_user *viu, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "warning dpdkif%s: ", viu->viu_devstr);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

#define OUT(a) do { fprintf(stderr, a); fprintf(stderr, "\n"); goto out; } while (/*CONSTCOND*/0)

int
VIFHYPER_INIT(void)
{
	int rv;

	if ((rv = rte_eal_init(sizeof(ealargs)/sizeof(ealargs[0]),
	    /*UNCONST*/(void *)(uintptr_t)ealargs)) < 0)
		OUT("eal init");

	if ((mbpool_tx = rte_mempool_create("mbuf_pool_tx", NMBUF_TX, MBSIZE, 0/*MBCACHE*/,
	    sizeof(struct rte_pktmbuf_pool_private),
	    rte_pktmbuf_pool_init, NULL,
	    rte_pktmbuf_init, NULL, 0, 0)) == NULL) {
		rv = -EINVAL;
		OUT("mbuf pool tx");
	}
	if ((mbpool_rx = rte_mempool_create("mbuf_pool_rx", NMBUF_RX, MBSIZE, 0/*MBCACHE*/,
	    sizeof(struct rte_pktmbuf_pool_private),
	    rte_pktmbuf_pool_init, NULL,
	    rte_pktmbuf_init, NULL, 0, 0)) == NULL) {
		rv = -EINVAL;
		OUT("mbuf pool tx");
	}

	if (rte_eth_dev_count() == 0) {
		rv = -1;
		OUT("no ports");
	}
	rv = 0;

 out:
 	return rv;
}
#undef OUT

#define OUT(a) do { ifwarn(viu, a) ; goto out; } while (/*CONSTCOND*/0)

void
VIFHYPER_MBUF_FREECB(void *data, size_t dlen, void *arg)
{

	rte_pktmbuf_free_seg(arg);
}

/*
 * Get mbuf off of interface, push it up into the TCP/IP stack.
 * TODO: share TCP/IP stack mbufs with DPDK mbufs to avoid
 * data copy (in rump_virtif_pktdeliver()).
 */
#define STACK_MEXTDATA 16
static void
deliverframe(struct virtif_user *viu)
{
	struct mbuf *m = NULL;
	struct rte_mbuf *rm, *rm0;
	struct vif_mextdata mext[STACK_MEXTDATA];
	struct vif_mextdata *mextp, *mextp0 = NULL;

	assert(viu->viu_nbufpkts > 0 && viu->viu_bufidx < MAX_PKT_BURST);
	rm0 = viu->viu_m_pkts[viu->viu_bufidx];
	assert(rm0 != NULL);
	viu->viu_bufidx++;
	viu->viu_nbufpkts--;

	if (rm0->pkt.nb_segs > STACK_MEXTDATA) {
		mextp = malloc(sizeof(*mextp) * rm0->pkt.nb_segs);
		if (mextp == NULL)
			goto drop;
	} else {
		mextp = mext;
	}
	mextp0 = mextp;

	for (rm = rm0; rm; rm = rm->pkt.next, mextp++) {
		mextp->mext_data = rte_pktmbuf_mtod(rm, void *);
		mextp->mext_dlen = rte_pktmbuf_data_len(rm);
		mextp->mext_arg = rm;
	}
	if (VIF_MBUF_EXTALLOC(mextp0, mextp - mextp0, &m) != 0)
		goto drop;

	VIF_DELIVERMBUF(viu->viu_virtifsc, m);

	if (mextp0 != mext)
		free(mextp0);
	return;

 drop:
	if (mextp0 != mext)
		free(mextp0);
	rte_pktmbuf_free(rm0);
}

static void *
receiver(void *arg)
{
	struct virtif_user *viu = arg;

	/* step 1: this newly created host thread needs a rump kernel context */
	rumpuser_component_kthread();

	/* step 2: deliver packets until interface is decommissioned */
	while (!viu->viu_dying) {
		/* we have cached frames. schedule + deliver */
		if (viu->viu_nbufpkts > 0) {
			rumpuser_component_schedule(NULL);
			while (viu->viu_nbufpkts > 0) {
				deliverframe(viu);
			}
			rumpuser_component_unschedule();
		}
		
		/* none cached.  ok, try to get some */
		if (viu->viu_nbufpkts == 0) {
			viu->viu_nbufpkts = rte_eth_rx_burst(viu->viu_port_id,
			    0, viu->viu_m_pkts, MAX_PKT_BURST);
			viu->viu_bufidx = 0;
		}
			
		if (viu->viu_nbufpkts == 0) {
			/*
			 * For now, don't ultrabusyloop.
			 * I don't have an overabundance of
			 * spare cores in my vm.
			 */
			usleep(10000);
		}
	}

	return NULL;
}


int
VIFHYPER_CREATE(const char *devstr, struct virtif_sc *vif_sc, uint8_t *enaddr,
	struct virtif_user **viup)
{
	struct rte_eth_conf portconf;
	struct rte_eth_link link;
	struct ether_addr ea;
	struct virtif_user *viu;
	unsigned long tmp;
	char *ep;
	int rv = EINVAL; /* XXX: not very accurate ;) */

	viu = malloc(sizeof(*viu));
	memset(viu, 0, sizeof(*viu));
	viu->viu_devstr = strdup(devstr);
	viu->viu_virtifsc = vif_sc;

	tmp = strtoul(devstr, &ep, 10);
	if (*ep != '\0')
		OUT("invalid dev string");

	if (tmp > 255)
		OUT("DPDK port id out of range");

	viu->viu_port_id = tmp;

	memset(&portconf, 0, sizeof(portconf));
	if ((rv = rte_eth_dev_configure(viu->viu_port_id,
	    NQUEUE, NQUEUE, &portconf)) < 0)
		OUT("configure device");

	if ((rv = rte_eth_rx_queue_setup(viu->viu_port_id,
	    0, NDESCRX, 0, &rxconf, mbpool_rx)) <0)
		OUT("rx queue setup");

	if ((rv = rte_eth_tx_queue_setup(viu->viu_port_id, 0, NDESCTX, 0, &txconf)) < 0)
		OUT("tx queue setup");

	if ((rv = rte_eth_dev_start(viu->viu_port_id)) < 0)
		OUT("device start");

	rte_eth_link_get(viu->viu_port_id, &link);
	if (!link.link_status) {
		ifwarn(viu, "link down");
	}

	rte_eth_promiscuous_enable(viu->viu_port_id);
	rte_eth_macaddr_get(viu->viu_port_id, &ea);
	memcpy(enaddr, ea.addr_bytes, ETHER_ADDR_LEN);

	rv = pthread_create(&viu->viu_rcvpt, NULL, receiver, viu);

 out:
	/* XXX: well this isn't much of an unrolling ... */
	if (rv != 0)
		free(viu);
	else
		*viup = viu;
	return rumpuser_component_errtrans(-rv);
}

void
VIFHYPER_GETCAPS(struct virtif_user *viu, int *ifcaps, int *ethercaps)
{
	/* TODO: Figure out what caps we're going to advertise. */
}

/*
 * Arrange for mbuf to be transmitted.
 *
 * TODO: use bulk transfers.  This should not be too difficult and will
 * have a big performance impact.
 */
void
VIFHYPER_SENDMBUF(struct virtif_user *viu, struct mbuf *m0,
	int pktlen, int csum_flags, uint32_t csum_data, void *d, int dlen)
{
	struct rte_mbuf *rm;
	struct mbuf *m;
	void *rmdptr;

	rm = rte_pktmbuf_alloc(mbpool_tx);
	for (m = m0; m; ) {
		rmdptr = rte_pktmbuf_append(rm, dlen);
		if (rmdptr == NULL) {
			/* log error somehow? */
			rte_pktmbuf_free(rm);
			break;
		}
		memcpy(rmdptr, d, dlen); /* XXX */
		VIF_MBUF_NEXT(m, &m, &d, &dlen);
	}
	VIF_MBUF_FREE(m0);
	
	rte_eth_tx_burst(viu->viu_port_id, 0, &rm, 1);
}

int
VIFHYPER_DYING(struct virtif_user *viu)
{

	viu->viu_dying = 1;
	return 0;
}

void
VIFHYPER_DESTROY(struct virtif_user *viu)
{

	assert(viu->viu_dying);
	pthread_join(viu->viu_rcvpt, NULL);
	/* XXX: missing some features, fix when fixing globalinit() */
	free(viu);
}
