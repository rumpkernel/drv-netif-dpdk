LIB=	rumpnet_virtif

SRCS=	if_virt.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern -I${.CURDIR}

RUMPCOMP_USER=	#defined
RUMPCOMP_USER_CPPFLAGS+= -I${RTE_SDK}/${RTE_TARGET}/include

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
