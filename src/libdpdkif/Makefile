LIB=	rumpnet_dpdkif

SRCS=	if_virt.c
SRCS+=	component.c

RUMPTOP= ${TOPRUMP}

.PATH:	${RUMPTOP}/net/lib/libvirtif

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpnet
CPPFLAGS+=	-I${RUMPTOP}/net/lib/libvirtif
CPPFLAGS+=	-DVIRTIF_BASE=dpdk

RUMPCOMP_USER=	#defined
RUMPCOMP_USER_CPPFLAGS+= -I${RTE_SDK}/${RTE_TARGET}/include
RUMPCOMP_USER_CPPFLAGS+= -I${RUMPTOP}/net/lib/libvirtif
RUMPCOMP_USER_CPPFLAGS+= -DVIRTIF_BASE=dpdk

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
