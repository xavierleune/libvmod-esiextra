/*-
 * Copyright 2017 UPLEX - Nils Goroll Systemoptimierung
 * All rights reserved.
 *
 * Author: Author: Nils Goroll <nils.goroll@uplex.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

//#include "cache/cache_varnishd.h"
#include "cache/cache.h"
#include "cache/cache_filter.h"
#include "vsha256.h"
#include "miniobj.h"

const struct vfp VFP_bodyhash;

struct bodyhash {
	unsigned		magic;
#define BODYHASH_MAGIC		0xb0d16a56

	struct VSHA256Context	sha256ctx;
	char			*hdr;
	// unsigned char	hash[VSHA256_LEN];
};

const char placeholder[] = "\"vmod-esiextra magic placeholder "	\
    "vmod-esiextra magic placeholder \"";
const size_t placeholder_l = sizeof(placeholder) - 1;

static enum vfp_status __match_proto__(vfp_init_f)
vfp_bodyhash_init(struct vfp_ctx *vc, struct vfp_entry *vfe)
{
	struct bodyhash *bh;

	CHECK_OBJ_NOTNULL(vc, VFP_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(vfe, VFP_ENTRY_MAGIC);

	AN(vfe->priv1);
	assert(vfe->vfp == &VFP_bodyhash);
	assert(placeholder_l == VSHA256_LEN * 2 + 2);

	// XXX workspace
	ALLOC_OBJ(bh, BODYHASH_MAGIC);
	if (bh == NULL)
		return (VFP_ERROR);
	VSHA256_Init(&bh->sha256ctx);
	bh->hdr = vfe->priv1;

	vfe->priv1 = bh;
	http_ForceHeader(vc->resp, H_ETag, placeholder);
	return (VFP_OK);
}

static enum vfp_status __match_proto__(vfp_pull_f)
vfp_bodyhash_pull(struct vfp_ctx *vc, struct vfp_entry *vfe, void *p,
    ssize_t *lp)
{
	struct bodyhash *bh;
	enum vfp_status vp;

	CHECK_OBJ_NOTNULL(vc, VFP_CTX_MAGIC);
	AN(lp);

	vp = VFP_Suck(vc, p, lp);
	if (*lp <= 0)
		return (vp);

	CHECK_OBJ_NOTNULL(vfe, VFP_ENTRY_MAGIC);
	CAST_OBJ_NOTNULL(bh, vfe->priv1, BODYHASH_MAGIC);
	VSHA256_Update(&bh->sha256ctx, p, *lp);
	return (vp);
}

const char hexe[16] = {
	"0123456789abcdef",
};

// XXX make non-void
static void __match_proto__(vfp_fini_f)
vfp_bodyhash_fini(struct vfp_ctx *vc, struct vfp_entry *vfe)
{
	struct bodyhash *bh;
	unsigned char	sha[VSHA256_LEN];
	const ssize_t	hexl = VSHA256_LEN * 2 + 3;
	char		hex[hexl];
	char		*etag, *p;
	int		i;
	ssize_t		l;

	CHECK_OBJ_NOTNULL(vc, VFP_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(vfe, VFP_ENTRY_MAGIC);

	if (vfe->priv1 == NULL)
		return;

	CAST_OBJ_NOTNULL(bh, vfe->priv1, BODYHASH_MAGIC);
	vfe->priv1 = NULL;

	VSHA256_Final(sha, &bh->sha256ctx);

	p = hex;
	*p++ = '"';
	for (i = 0; i < VSHA256_LEN; i++) {
		*p++ = hexe[(sha[i] & 0xf0) >> 4];
		*p++ = hexe[sha[i] & 0x0f];
	}
	*p++ = '"';
	*p++ = '\0';
	assert(pdiff(hex, p) == hexl);

	if (vc->resp && vc->resp->thd) {
		http_ForceHeader(vc->resp, H_ETag, hex);
		goto out;
	}

	// HACKY
	p = TRUST_ME(ObjGetAttr(vc->wrk, vc->oc, OA_HEADERS, &l));
	if (p == NULL) {
		VSLb(vc->wrk->vsl, SLT_Error, "bodyhash: no object");
		goto out;
	}
	etag = p;
	p = etag + l;
	do {
		etag = memchr(etag, '"', l);
		if (etag == NULL)
			break;
		l = p - etag;
		if (l < placeholder_l) {
			etag = NULL;
			break;
		}
		if (strncmp(etag, placeholder, placeholder_l) == 0)
			break;
		etag++;
	} while(1);

	if (etag == NULL) {
		VSLb(vc->wrk->vsl, SLT_Error, "bodyhash: no placeholder");
		goto out;
	}


	assert(*etag == '"');
	memcpy(etag, hex, hexl);

  out:
	FREE_OBJ(bh);
}

const struct vfp VFP_bodyhash = {
	.name = "bodyhash",
	.init = vfp_bodyhash_init,
	.pull = vfp_bodyhash_pull,
	.fini = vfp_bodyhash_fini
};

// XXX in cache_varnishd.h
struct vfp_entry *VFP_Push(struct vfp_ctx *, const struct vfp *);

void *
Bodyhash_push(struct vfp_ctx *vc, const char *hdr)
{
	struct vfp_entry *vfe = VFP_Push(vc, &VFP_bodyhash);
	if (vfe)
		vfe->priv1 = TRUST_ME(hdr);
	return (vfe);
}
