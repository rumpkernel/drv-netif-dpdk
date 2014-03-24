LIB=	rumpnet_dpdkif
VIRTIF=	dpdk

RUMPTOP= ${TOPRUMP}
.include "${.CURDIR}/virtif/Makefile.virtif"

RUMPCOMP_USER_SRCS=	dpdkif_user.c
RUMPCOMP_USER_CPPFLAGS+= -I${RTE_SDK}/${RTE_TARGET}/include

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
