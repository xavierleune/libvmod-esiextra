/*-
 * XXX TODO
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

	struct SHA256Context	sha256ctx;
	char			*hdr;
	// unsigned char	hash[SHA256_LEN];
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
	assert(placeholder_l == SHA256_LEN * 2 + 2);

	// XXX workspace
	ALLOC_OBJ(bh, BODYHASH_MAGIC);
	if (bh == NULL)
		return (VFP_ERROR);
	SHA256_Init(&bh->sha256ctx);
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
	SHA256_Update(&bh->sha256ctx, p, *lp);
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
	unsigned char	sha[SHA256_LEN];
	char		*etag, *p;
	int		i;
	ssize_t		l;

	CHECK_OBJ_NOTNULL(vc, VFP_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(vfe, VFP_ENTRY_MAGIC);

	if (vfe->priv1 == NULL)
		return;

	CAST_OBJ_NOTNULL(bh, vfe->priv1, BODYHASH_MAGIC);
	vfe->priv1 = NULL;

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

	SHA256_Final(sha, &bh->sha256ctx);

	assert(*etag == '"');
	p = etag;
	p++;
	for (i = 0; i < SHA256_LEN; i++) {
		*p++ = hexe[(sha[i] & 0xf0) >> 4];
		*p++ = hexe[sha[i] & 0x0f];
	}
	assert(*p == '"');

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
