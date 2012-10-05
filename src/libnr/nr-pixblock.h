#ifndef __NR_PIXBLOCK_H__
#define __NR_PIXBLOCK_H__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <assert.h>

#include <libnr/nr-types.h>
#include <libnr/nr-forward.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NR_TINY_MAX 64

enum {
	NR_PIXBLOCK_SIZE_TINY, /* Fits in pixblock data */
	NR_PIXBLOCK_SIZE_4K, /* Pixelstore */
	NR_PIXBLOCK_SIZE_16K, /* Pixelstore */
	NR_PIXBLOCK_SIZE_64K, /* Pixelstore */
	NR_PIXBLOCK_SIZE_BIG, /* Normally allocated */
	NR_PIXBLOCK_SIZE_STATIC /* Externally managed */
};

enum {
	NR_PIXBLOCK_MODE_G8, /* Grayscale or mask */
	NR_PIXBLOCK_MODE_R8G8B8, /* 8 bit RGB */
	NR_PIXBLOCK_MODE_R8G8B8A8N, /* Normal 8 bit RGBA */
	NR_PIXBLOCK_MODE_R8G8B8A8P /* Premultiplied 8 bit RGBA */
};

// Automatically allocated rowstride is aligned at 4 bytes

struct _NRPixBlock {
	unsigned int size : 3;
	unsigned int mode : 2;
	unsigned int empty : 1;
	unsigned int rs;
	NRRectS area;
	union {
		unsigned char *px;
		unsigned char p[NR_TINY_MAX];
	} data;
};

#define NR_PIXBLOCK_MODE_BPP(m) ((m == NR_PIXBLOCK_MODE_G8) ? 1 : (m == NR_PIXBLOCK_MODE_R8G8B8) ? 3 : 4)
#define NR_PIXBLOCK_BPP(pb) (((pb)->mode == NR_PIXBLOCK_MODE_G8) ? 1 : ((pb)->mode == NR_PIXBLOCK_MODE_R8G8B8) ? 3 : 4)
#define NR_PIXBLOCK_PX(pb) (((pb)->size == NR_PIXBLOCK_SIZE_TINY) ? (pb)->data.p : (pb)->data.px)
#define NR_PIXBLOCK_ROW(pb,r) (NR_PIXBLOCK_PX (pb) + (r) * (pb)->rs)

void nr_pixblock_setup (NRPixBlock *pb, int mode, int x0, int y0, int x1, int y1, int clear);
void nr_pixblock_setup_fast (NRPixBlock *pb, int mode, int x0, int y0, int x1, int y1, int clear);
void nr_pixblock_setup_extern (NRPixBlock *pb, int mode, int x0, int y0, int x1, int y1, unsigned char *px, int rs, int empty, int clear);
void nr_pixblock_release (NRPixBlock *pb);

NRPixBlock *nr_pixblock_new (int mode, int x0, int y0, int x1, int y1, int clear);
NRPixBlock *nr_pixblock_free (NRPixBlock *pb);

/* Helpers */

unsigned int nr_pixblock_has_alpha (const NRPixBlock *pb);
void nr_pixblock_get_channel_limits (const NRPixBlock *pb, unsigned int minv[], unsigned int maxv[]);
void nr_pixblock_get_histogram (const NRPixBlock *pb, unsigned int histogram[][256]);
unsigned int nr_pixblock_get_crc32 (const NRPixBlock *pb);
NRULongLong nr_pixblock_get_crc64 (const NRPixBlock *pb);
unsigned int nr_pixblock_get_hash (const NRPixBlock *pb);
unsigned int nr_pixblock_is_equal (const NRPixBlock *a, const NRPixBlock *b);

/* Memory management */

unsigned char *nr_pixelstore_4K_new (int clear, unsigned char val);
void nr_pixelstore_4K_free (unsigned char *px);
unsigned char *nr_pixelstore_16K_new (int clear, unsigned char val);
void nr_pixelstore_16K_free (unsigned char *px);
unsigned char *nr_pixelstore_64K_new (int clear, unsigned char val);
void nr_pixelstore_64K_free (unsigned char *px);

#ifdef __cplusplus
};
#endif

#endif
