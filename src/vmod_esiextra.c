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

#include <string.h>
#include <stdlib.h>

#include <cache/cache.h>
#include <vcl.h>

#include "vcc_esiextra_if.h"

#include "vfp_bodyhash.h"

VCL_VOID __match_proto__(td_esiextra_bodyhash)
vmod_bodyhash(VRT_CTX, VCL_HEADER hdr)
{
	struct busyobj *bo;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	if (ctx->method != VCL_MET_BACKEND_RESPONSE) {
		VRT_fail(ctx, "bodyhash only valid in vcl_backend_response");
		return;
	}

	if (hdr->where != HDR_BERESP) {
		VRT_fail(ctx, "bodyhash can only modify beresp.http.*");
		return;
	}

	CAST_OBJ_NOTNULL(bo, ctx->bo, BUSYOBJ_MAGIC);
	bo->do_stream = 0;
	if (Bodyhash_push(bo->vfc, hdr->what) == NULL)
		VRT_fail(ctx, "bodyhash push failed");
}

/* ============================================================================
 * Last-Modified for all of ESI
 */

/*
 * so far this is not much more as the VRT_priv ID
 */
struct vmod_esiextra_lm {
	unsigned	magic;
#define VMOD_ESIEXTRA_LM_MAGIC 0x3188a965
	const char	*vcl_name;
};

VCL_VOID __match_proto__(td_esiextra_lm__init)
vmod_lm__init(VRT_CTX, struct vmod_esiextra_lm **lmobj,
    const char *vcl_name)
{
	struct vmod_esiextra_lm *o;

	assert(sizeof(void *) >= sizeof(VCL_TIME));
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	AN(lmobj);
	AZ(*lmobj);
	ALLOC_OBJ(o, VMOD_ESIEXTRA_LM_MAGIC);
	AN(o);
	*lmobj = o;
	o->vcl_name = vcl_name;
	AN(*lmobj);
}

VCL_VOID __match_proto__(td_esiextra_lm__fini)
vmod_lm__fini(struct vmod_esiextra_lm **lmobj)
{
	AN(lmobj);
	if (*lmobj == NULL)
		return;
	FREE_OBJ(*lmobj);
	*lmobj = NULL;
}

static VCL_TIME *
lm_time(VRT_CTX, const struct vmod_esiextra_lm *lmobj)
{
	struct vmod_priv *priv;
	VCL_TIME *t;

	priv = VRT_priv_top(ctx, (void *)lmobj);
	AN(priv);
	t = (VCL_TIME *)(&priv->priv);
	if (!isnormal(*t))
		*t = 0.0;
	return (t);
}

static inline int
lm_bad_task(VRT_CTX)
{
	if ((ctx->method & VCL_MET_TASK_C) == 0) {
		VRT_fail(ctx, "lm object only usable in client context");
		return 1;
	}
	return 0;
}

VCL_BOOL __match_proto__(td_esiextra_lminspect)
vmod_lm_inspect(VRT_CTX, struct vmod_esiextra_lm *lmobj,
    VCL_TIME t)
{
	VCL_TIME *lm;
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	if (lm_bad_task(ctx))
		return (0);
	lm = lm_time(ctx, lmobj);
	if (t <= *lm)
		return (0);
	*lm = t;
	return (1);
}

VCL_VOID __match_proto__(td_esiextra_lmupdate)
vmod_lm_update(VRT_CTX, struct vmod_esiextra_lm *lmobj,
    VCL_TIME t)
{
	(void)vmod_lm_inspect(ctx, lmobj, t);
}

VCL_TIME __match_proto__(td_esiextra_lmget)
vmod_lm_get(VRT_CTX, struct vmod_esiextra_lm *lmobj)
{
	VCL_TIME *lm;
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	if (lm_bad_task(ctx))
		return (0);
	lm = lm_time(ctx, lmobj);
	return (*lm);
}
