/*	$NetBSD: if_virt.h,v 1.3 2014/03/03 13:56:40 pooka Exp $	*/

/*
 * NOTE!  This file is supposed to work on !NetBSD platforms.
 */

#ifndef VIRTIF_BASE
#error Define VIRTIF_BASE
#endif

#define VIF_STRING(x) #x
#define VIF_STRINGIFY(x) VIF_STRING(x)
#define VIF_CONCAT(x,y) x##y
#define VIF_CONCAT3(x,y,z) x##y##z
#define VIF_BASENAME(x,y) VIF_CONCAT(x,y)
#define VIF_BASENAME3(x,y,z) VIF_CONCAT3(x,y,z)

#define VIF_CLONER VIF_BASENAME(VIRTIF_BASE,_cloner)
#define VIF_NAME VIF_STRINGIFY(VIRTIF_BASE)

#define VIFHYPER_INIT VIF_BASENAME3(rumpcomp_,VIRTIF_BASE,_init)
#define VIFHYPER_CREATE VIF_BASENAME3(rumpcomp_,VIRTIF_BASE,_create)
#define VIFHYPER_GETCAPS VIF_BASENAME3(rumpcomp_,VIRTIF_BASE,_getcaps)
#define VIFHYPER_DYING VIF_BASENAME3(rumpcomp_,VIRTIF_BASE,_dying)
#define VIFHYPER_DESTROY VIF_BASENAME3(rumpcomp_,VIRTIF_BASE,_destroy)
#define VIFHYPER_SENDMBUF VIF_BASENAME3(rumpcomp_,VIRTIF_BASE,_sendmbuf)
#define VIFHYPER_MBUF_FREECB VIF_BASENAME3(rumpcomp_,VIRTIF_BASE,_mbuf_cb)

#define VIFHYPER_FLAGS VIF_BASENAME3(rumpcomp_,VIRTIF_BASE,_flags)

#define VIF_MBUF_NEXT VIF_BASENAME3(rump_virtif_,VIRTIF_BASE,_mbuf_next)
#define VIF_MBUF_FREE VIF_BASENAME3(rump_virtif_,VIRTIF_BASE,_mbuf_free)
#define VIF_MBUF_EXTALLOC VIF_BASENAME3(rump_virtif_,VIRTIF_BASE,_mbuf_extalloc)

#define VIF_DELIVERMBUF VIF_BASENAME3(rump_virtif_,VIRTIF_BASE,_delivermbuf)

struct virtif_sc;
