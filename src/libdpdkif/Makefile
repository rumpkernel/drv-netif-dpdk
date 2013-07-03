LIB=	rumpnet_dpdkif

SRCS=	if_virt.c
SRCS+=	component.c

RUMPTOP= ${TOPRUMP}

.PATH:	${RUMPTOP}/net/lib/libvirtif

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpnet

# soon
#CPPFLAGS+=	-DVIRTIF_BASE=\"dpdk\"

RUMPCOMP_USER=	#defined
RUMPCOMP_USER_CPPFLAGS+= -I${RTE_SDK}/${RTE_TARGET}/include

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
