/*	$NetBSD: virtif_user.h,v 1.3 2014/03/14 10:06:22 pooka Exp $	*/

/*
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

struct virtif_user;
struct mbuf;

struct vif_mextdata {
	void	*mext_data;
	size_t	mext_dlen;
	void	*mext_arg;
};

#define VIFHYPER_REVISION 20140318

int 	VIFHYPER_CREATE(const char *, struct virtif_sc *, uint8_t *,
			struct virtif_user **);
int	VIFHYPER_DYING(struct virtif_user *);
void	VIFHYPER_DESTROY(struct virtif_user *);

void	VIFHYPER_SENDMBUF(struct virtif_user *,
			  struct mbuf *, int, void *, int);

void	VIFHYPER_MBUF_FREECB(void *, size_t, void *);

void	VIF_MBUF_NEXT(struct mbuf *, struct mbuf **, void **, int *);
void	VIF_MBUF_FREE(struct mbuf *);

int	VIF_MBUF_EXTALLOC(struct vif_mextdata *, size_t, struct mbuf **);
void	VIF_DELIVERMBUF(struct virtif_sc *, struct mbuf *);
