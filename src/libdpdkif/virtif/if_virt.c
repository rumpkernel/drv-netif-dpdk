/*	$NetBSD: if_virt.c,v 1.45 2014/03/18 18:10:08 pooka Exp $	*/

/*
 * Copyright (c) 2008, 2013 Antti Kantee.  All Rights Reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_virt.c,v 1.45 2014/03/18 18:10:08 pooka Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/cprng.h>
#include <sys/module.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/in_var.h>

#include "if_virt.h"
#include "virtif_user.h"

/*
 * Virtual interface.  Uses hypercalls to shovel packets back
 * and forth.  The exact method for shoveling depends on the
 * hypercall implementation.
 */

static int	virtif_init(struct ifnet *);
static int	virtif_ioctl(struct ifnet *, u_long, void *);
static void	virtif_start(struct ifnet *);
static void	virtif_stop(struct ifnet *, int);

struct virtif_sc {
	struct ethercom sc_ec;
	struct virtif_user *sc_viu;

	int sc_num;
	char *sc_linkstr;
};

static int  virtif_clone(struct if_clone *, int);
static int  virtif_unclone(struct ifnet *);

struct if_clone VIF_CLONER =
    IF_CLONE_INITIALIZER(VIF_NAME, virtif_clone, virtif_unclone);

static int
virtif_create(struct ifnet *ifp)
{
	uint8_t enaddr[ETHER_ADDR_LEN] = { 0xb2, 0x0a, 0x00, 0x0b, 0x0e, 0x01 };
	char enaddrstr[3*ETHER_ADDR_LEN];
	struct virtif_sc *sc = ifp->if_softc;
	int error;

	if (sc->sc_viu)
		panic("%s: already created", ifp->if_xname);

	enaddr[2] = cprng_fast32() & 0xff;
	enaddr[5] = sc->sc_num & 0xff;

	if ((error = VIFHYPER_CREATE(sc->sc_linkstr,
	    sc, enaddr, &sc->sc_viu)) != 0) {
		printf("VIFHYPER_CREATE failed: %d\n", error);
		return error;
	}

	ether_ifattach(ifp, enaddr);
	ether_snprintf(enaddrstr, sizeof(enaddrstr), enaddr);
	aprint_normal_ifnet(ifp, "Ethernet address %s\n", enaddrstr);

	IFQ_SET_READY(&ifp->if_snd);

	return 0;
}

static int
virtif_clone(struct if_clone *ifc, int num)
{
	struct virtif_sc *sc;
	struct ifnet *ifp;
	int error = 0;

	sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
	sc->sc_num = num;
	ifp = &sc->sc_ec.ec_if;
	snprintf(ifp->if_xname, sizeof(ifp->if_xname), "%s%d", VIF_NAME, num);
	ifp->if_softc = sc;

	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = virtif_init;
	ifp->if_ioctl = virtif_ioctl;
	ifp->if_start = virtif_start;
	ifp->if_stop = virtif_stop;
	ifp->if_mtu = ETHERMTU;
	ifp->if_dlt = DLT_EN10MB;

	if_attach(ifp);

#ifndef RUMP_VIF_LINKSTR
	/*
	 * if the underlying interface does not expect linkstr, we can
	 * create everything now.  Otherwise, we need to wait for
	 * SIOCSLINKSTR.
	 */
#define LINKSTRNUMLEN 16
	sc->sc_linkstr = kmem_alloc(LINKSTRNUMLEN, KM_SLEEP);
	snprintf(sc->sc_linkstr, LINKSTRNUMLEN, "%d", sc->sc_num);
#undef LINKSTRNUMLEN
	error = virtif_create(ifp);
	if (error) {
		if_detach(ifp);
		kmem_free(sc, sizeof(*sc));
		ifp->if_softc = NULL;
	}
#endif /* !RUMP_VIF_LINKSTR */

	return error;
}

static int
virtif_unclone(struct ifnet *ifp)
{
	struct virtif_sc *sc = ifp->if_softc;
	int rv;

	if (ifp->if_flags & IFF_UP)
		return EBUSY;

	if ((rv = VIFHYPER_DYING(sc->sc_viu)) != 0)
		return rv;

	virtif_stop(ifp, 1);
	if_down(ifp);

	VIFHYPER_DESTROY(sc->sc_viu);

	kmem_free(sc, sizeof(*sc));

	ether_ifdetach(ifp);
	if_detach(ifp);

	return 0;
}

static int
virtif_init(struct ifnet *ifp)
{
	struct virtif_sc *sc = ifp->if_softc;

	if (sc->sc_viu == NULL)
		return ENXIO;

	ifp->if_flags |= IFF_RUNNING;
	return 0;
}

static int
virtif_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct virtif_sc *sc = ifp->if_softc;
	int rv;

	switch (cmd) {
#ifdef RUMP_VIF_LINKSTR
	struct ifdrv *ifd;
	size_t linkstrlen;

#ifndef RUMP_VIF_LINKSTRMAX
#define RUMP_VIF_LINKSTRMAX 4096
#endif

	case SIOCGLINKSTR:
		ifd = data;

		if (!sc->sc_linkstr) {
			rv = ENOENT;
			break;
		}
		linkstrlen = strlen(sc->sc_linkstr)+1;

		if (ifd->ifd_cmd == IFLINKSTR_QUERYLEN) {
			ifd->ifd_len = linkstrlen;
			rv = 0;
			break;
		}
		if (ifd->ifd_cmd != 0) {
			rv = ENOTTY;
			break;
		}

		rv = copyoutstr(sc->sc_linkstr,
		    ifd->ifd_data, MIN(ifd->ifd_len,linkstrlen), NULL);
		break;
	case SIOCSLINKSTR:
		if (ifp->if_flags & IFF_UP) {
			rv = EBUSY;
			break;
		}

		ifd = data;

		if (ifd->ifd_cmd == IFLINKSTR_UNSET) {
			panic("unset linkstr not implemented");
		} else if (ifd->ifd_cmd != 0) {
			rv = ENOTTY;
			break;
		} else if (sc->sc_linkstr) {
			rv = EBUSY;
			break;
		}

		if (ifd->ifd_len > RUMP_VIF_LINKSTRMAX) {
			rv = E2BIG;
			break;
		} else if (ifd->ifd_len < 1) {
			rv = EINVAL;
			break;
		}


		sc->sc_linkstr = kmem_alloc(ifd->ifd_len, KM_SLEEP);
		rv = copyinstr(ifd->ifd_data, sc->sc_linkstr,
		    ifd->ifd_len, NULL);
		if (rv) {
			kmem_free(sc->sc_linkstr, ifd->ifd_len);
			break;
		}

		rv = virtif_create(ifp);
		if (rv) {
			kmem_free(sc->sc_linkstr, ifd->ifd_len);
		}
		break;
#endif /* RUMP_VIF_LINKSTR */
	default:
		if (!sc->sc_linkstr)
			rv = ENXIO;
		else
			rv = ether_ioctl(ifp, cmd, data);
		if (rv == ENETRESET)
			rv = 0;
		break;
	}

	return rv;
}

/*
 * Output packets in-context until outgoing queue is empty.
 * Leave responsibility of choosing whether or not to drop the
 * kernel lock to VIPHYPER_SENDMBUF().
 */
static void
virtif_start(struct ifnet *ifp)
{
	struct virtif_sc *sc = ifp->if_softc;
	struct mbuf *m;

	ifp->if_flags |= IFF_OACTIVE;
	for (;;) {
		IF_DEQUEUE(&ifp->if_snd, m);
		if (!m) {
			break;
		}
		bpf_mtap(ifp, m);

		VIFHYPER_SENDMBUF(sc->sc_viu,
		    m, m->m_pkthdr.len, m->m_data, m->m_len);
	}
	ifp->if_flags &= ~IFF_OACTIVE;
}

static void
virtif_stop(struct ifnet *ifp, int disable)
{

	/* XXX: VIFHYPER_STOP() */

	ifp->if_flags &= ~IFF_RUNNING;
}

void
VIF_MBUF_NEXT(struct mbuf *m, struct mbuf **n, void **data, int *dlen)
{

	*n = m = m->m_next;
	if (m == NULL)
		return;

	*data = m->m_data;
	*dlen = m->m_len;
}

void
VIF_MBUF_FREE(struct mbuf *m)
{

	m_freem(m);
}

static void
freemextbuf(struct mbuf *m, void *buf, size_t buflen, void *arg)
{

	VIFHYPER_MBUF_FREECB(buf, buflen, arg);
	pool_cache_put(mb_cache, m);
}

int
VIF_MBUF_EXTALLOC(struct vif_mextdata *mextd, size_t n_mextdata,
	struct mbuf **mp)
{
	struct mbuf *m0, *m, *n;
	struct vif_mextdata *md;
	size_t totlen, i;
	int error = 0;

	KASSERT(n_mextdata > 0);
	m0 = m_gethdr(M_NOWAIT, MT_DATA);
	if (m0 == NULL)
		return ENOMEM;
	md = &mextd[0];
	MEXTADD(m0, md->mext_data, md->mext_dlen, MT_DATA,
	    freemextbuf, md->mext_arg);
	totlen = m0->m_len = md->mext_dlen;
	m = m0;
	for (i = 1; i < n_mextdata; i++) {
		md = &mextd[i];

		n = m_get(M_NOWAIT, MT_DATA);
		if (n == NULL) {
			error = ENOMEM;
			break;
		}
		m->m_next = n;
		m = n;
		MEXTADD(m, md->mext_data, md->mext_dlen, MT_DATA,
		    freemextbuf, md->mext_arg);
		totlen += md->mext_dlen;
		m->m_len = md->mext_dlen; /* necessary? */
	}

	if (__predict_false(error)) {
		/*
		 * MEXTRESET?  We need to make sure freeing the mbuf
		 * will _not_ cause the free'ing callback to execute,
		 * otherwise the hypercall layer would be left in an
		 * unknown (or at least hard-to-determine) state
		 */
		for (m = m0; m; m = m->m_next) {
			m->m_flags &= ~M_EXT;
			m->m_data = m->m_dat;
		}
		m_freem(m0);
		m0 = NULL;
	} else {
		m0->m_pkthdr.len = totlen;
	}
	*mp = m0;
	return error;
}

void
VIF_DELIVERMBUF(struct virtif_sc *sc, struct mbuf *m)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ether_header *eth;
	bool passup;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	eth = mtod(m, struct ether_header *);
	if (memcmp(eth->ether_dhost, CLLADDR(ifp->if_sadl),
	    ETHER_ADDR_LEN) == 0) {
		passup = true;
	} else if (ETHER_IS_MULTICAST(eth->ether_dhost)) {
		passup = true;
	} else if (ifp->if_flags & IFF_PROMISC) {
		m->m_flags |= M_PROMISC;
		passup = true;
	} else {
		passup = false;
	}

	if (passup) {
		m->m_pkthdr.rcvif = ifp;
		KERNEL_LOCK(1, NULL);
		bpf_mtap(ifp, m);
		ifp->if_input(ifp, m);
		KERNEL_UNLOCK_LAST(NULL);
	} else {
		m_freem(m);
	}
}

MODULE(MODULE_CLASS_DRIVER, if_virt, NULL);

static int
if_virt_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		if_clone_attach(&VIF_CLONER);
		break;
	case MODULE_CMD_FINI:
		/*
		 * not sure if interfaces are refcounted
		 * and properly protected
		 */
#if 0
		if_clone_detach(&VIF_CLONER);
#else
		error = ENOTTY;
#endif
		break;
	default:
		error = ENOTTY;
	}
	return error;
}
