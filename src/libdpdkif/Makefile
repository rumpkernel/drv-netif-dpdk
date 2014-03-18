LIB=	rumpnet_dpdkif

SRCS=	if_virt.c
SRCS+=	component.c

RUMPTOP= ${TOPRUMP}

.PATH:	${.CURDIR}/../libvirtif

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpnet
CPPFLAGS+=	-I${.CURDIR}/../libvirtif
CPPFLAGS+=	-DVIRTIF_BASE=dpdk

RUMPCOMP_USER_SRCS=	rumpcomp_user.c
RUMPCOMP_USER_CPPFLAGS+= -I${RTE_SDK}/${RTE_TARGET}/include
RUMPCOMP_USER_CPPFLAGS+= -I${.CURDIR}/../libvirtif
RUMPCOMP_USER_CPPFLAGS+= -DVIRTIF_BASE=dpdk

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
