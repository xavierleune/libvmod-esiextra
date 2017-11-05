#include "config.h"

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
