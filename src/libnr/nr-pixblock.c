#define __NR_PIXBLOCK_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <string.h>
#include <stdlib.h>
#include "nr-macros.h"
#include "nr-pixblock.h"

void
nr_pixblock_setup_fast (NRPixBlock *pb, int mode, int x0, int y0, int x1, int y1, int clear)
{
	int w, h, bpp, size;

	w = x1 - x0;
	h = y1 - y0;
	bpp = NR_PIXBLOCK_MODE_BPP (mode);

	pb->mode = mode;
	pb->area.x0 = x0;
	pb->area.y0 = y0;
	pb->area.x1 = x1;
	pb->area.y1 = y1;
	pb->rs = (bpp * w + 3) & 0xfffffffc;

	size = h * pb->rs;

	if (size <= NR_TINY_MAX) {
		pb->size = NR_PIXBLOCK_SIZE_TINY;
		if (clear && size) memset (pb->data.p, 0x0, size);
	} else if (size <= 4096) {
		pb->size = NR_PIXBLOCK_SIZE_4K;
		pb->data.px = nr_pixelstore_4K_new (clear, 0x0);
	} else if (size <= 16384) {
		pb->size = NR_PIXBLOCK_SIZE_16K;
		pb->data.px = nr_pixelstore_16K_new (clear, 0x0);
	} else if (size <= 65536) {
		pb->size = NR_PIXBLOCK_SIZE_64K;
		pb->data.px = nr_pixelstore_64K_new (clear, 0x0);
	} else {
		pb->size = NR_PIXBLOCK_SIZE_BIG;
		pb->data.px = nr_new (unsigned char, size);
		if (clear) memset (pb->data.px, 0x0, size);
	}

	pb->empty = 1;
}

void
nr_pixblock_setup (NRPixBlock *pb, int mode, int x0, int y0, int x1, int y1, int clear)
{
	int w, h, bpp, size;

	w = x1 - x0;
	h = y1 - y0;
	bpp = NR_PIXBLOCK_MODE_BPP (mode);

	pb->mode = mode;
	pb->area.x0 = x0;
	pb->area.y0 = y0;
	pb->area.x1 = x1;
	pb->area.y1 = y1;
	pb->rs = (bpp * w + 3) & 0xfffffffc;

	size = h * pb->rs;

	if (size <= NR_TINY_MAX) {
		pb->size = NR_PIXBLOCK_SIZE_TINY;
		if (clear && size) memset (pb->data.p, 0x0, size);
	} else {
		pb->size = NR_PIXBLOCK_SIZE_BIG;
		pb->data.px = nr_new (unsigned char, size);
		if (clear) memset (pb->data.px, 0x0, size);
	}

	pb->empty = 1;
}

void
nr_pixblock_setup_extern (NRPixBlock *pb, int mode, int x0, int y0, int x1, int y1, unsigned char *px, int rs, int empty, int clear)
{
	int w, bpp;

	w = x1 - x0;
	bpp = (mode == NR_PIXBLOCK_MODE_G8) ? 1 : (mode == NR_PIXBLOCK_MODE_R8G8B8) ? 3 : 4;

	pb->size = NR_PIXBLOCK_SIZE_STATIC;
	pb->mode = mode;
	pb->empty = empty;
	pb->area.x0 = x0;
	pb->area.y0 = y0;
	pb->area.x1 = x1;
	pb->area.y1 = y1;
	pb->data.px = px;
	pb->rs = rs;

	if (clear) {
		if (rs == bpp * w) {
			memset (pb->data.px, 0x0, bpp * (y1 - y0) * w);
		} else {
			int y;
			for (y = y0; y < y1; y++) {
				memset (pb->data.px + (y - y0) * rs, 0x0, bpp * w);
			}
		}
	}
}

void
nr_pixblock_release (NRPixBlock *pb)
{
	switch (pb->size) {
	case NR_PIXBLOCK_SIZE_TINY:
		break;
	case NR_PIXBLOCK_SIZE_4K:
		nr_pixelstore_4K_free (pb->data.px);
		break;
	case NR_PIXBLOCK_SIZE_16K:
		nr_pixelstore_16K_free (pb->data.px);
		break;
	case NR_PIXBLOCK_SIZE_64K:
		nr_pixelstore_64K_free (pb->data.px);
		break;
	case NR_PIXBLOCK_SIZE_BIG:
		nr_free (pb->data.px);
		break;
	case NR_PIXBLOCK_SIZE_STATIC:
		break;
	default:
		break;
	}
}

NRPixBlock *
nr_pixblock_new (int mode, int x0, int y0, int x1, int y1, int clear)
{
	NRPixBlock *pb;

	pb = nr_new (NRPixBlock, 1);

	nr_pixblock_setup (pb, mode, x0, y0, x1, y1, clear);

	return pb;
}

NRPixBlock *
nr_pixblock_free (NRPixBlock *pb)
{
	nr_pixblock_release (pb);

	nr_free (pb);

	return NULL;
}

/* Helpers */

unsigned int
nr_pixblock_has_alpha (const NRPixBlock *pb)
{
	if (!pb) return 0;
	if (pb->mode == NR_PIXBLOCK_MODE_R8G8B8A8N) return 1;
	if (pb->mode == NR_PIXBLOCK_MODE_R8G8B8A8N) return 1;
	return 0;
}

void
nr_pixblock_get_channel_limits (const NRPixBlock *pb, unsigned int minv[], unsigned int maxv[])
{
	int x, y, bpp, c;
	bpp = NR_PIXBLOCK_BPP(pb);
	for (c = 0; c < bpp; c++) {
		minv[c] = 255;
		maxv[c] = 0;
	}
	for (y = pb->area.y0; y < pb->area.y1; y++) {
		const unsigned char *s;
		s = NR_PIXBLOCK_PX (pb) + (y - pb->area.y0) * pb->rs;
		for (x = pb->area.x0; x < pb->area.x1; x++) {
			for (c = 0; c < bpp; c++) {
				if (s[c] < minv[c]) minv[c] = s[c];
				if (s[c] > maxv[c]) maxv[c] = s[c];
			}
			s += bpp;
		}
	}
}

void
nr_pixblock_get_histogram (const NRPixBlock *pb, unsigned int histogram[][256])
{
	int x, y, bpp, c, v;
	bpp = NR_PIXBLOCK_BPP(pb);
	for (c = 0; c < bpp; c++) {
		for (v = 0; v < 256; v++) histogram[c][v] = 0;
	}
	for (y = pb->area.y0; y < pb->area.y1; y++) {
		const unsigned char *s;
		s = NR_PIXBLOCK_PX (pb) + (y - pb->area.y0) * pb->rs;
		for (x = pb->area.x0; x < pb->area.x1; x++) {
			for (c = 0; c < bpp; c++) {
				histogram[c][s[c]] += 1;
			}
			s += bpp;
		}
	}
}

unsigned int
nr_pixblock_get_crc32 (const NRPixBlock *pb)
{
	unsigned int crc32, acc;
	unsigned int width, height, bpp, x, y, c, cnt;
	width = pb->area.x1 - pb->area.x0;
	height = pb->area.y1 - pb->area.y0;
	bpp = NR_PIXBLOCK_BPP(pb);
	crc32 = 0;
	acc = 0;
	cnt = 0;
	for (y = 0; y < height; y++) {
		const unsigned char *s = NR_PIXBLOCK_ROW(pb, y);
		for (x = 0; x < width; x++) {
			for (c = 0; c < bpp; c++) {
				acc <<= 8;
				acc |= *s++;
				if (++cnt >= 4) {
					crc32 ^= acc;
					acc = cnt = 0;
				}
			}
		}
		crc32 ^= acc;
		acc = cnt = 0;
	}
	return crc32;
}

NRULongLong
nr_pixblock_get_crc64 (const NRPixBlock *pb)
{
	NRULongLong crc64, acc, carry;
	unsigned int width, height, bpp, x, y, c, cnt;
	width = pb->area.x1 - pb->area.x0;
	height = pb->area.y1 - pb->area.y0;
	bpp = NR_PIXBLOCK_BPP(pb);
	crc64 = 0;
	acc = 0;
	carry = 0;
	cnt = 0;
	for (y = 0; y < height; y++) {
		const unsigned char *s = NR_PIXBLOCK_ROW(pb, y);
		for (x = 0; x < width; x++) {
			for (c = 0; c < bpp; c++) {
				acc <<= 8;
				acc |= *s++;
				if (++cnt >= 8) {
					if ((crc64 & 0x8000000000000000) && (acc & 0x8000000000000000)) carry = 1;
					crc64 += acc;
					crc64 += carry;
					acc = carry = 0;
					cnt = 0;
				}
			}
		}
		crc64 += acc;
		acc = cnt = 0;
	}
	return crc64;
}

unsigned int
nr_pixblock_get_hash (const NRPixBlock *pb)
{
	unsigned int hval;
	unsigned int width, height, bpp, x, y, c;
	width = pb->area.x1 - pb->area.x0;
	height = pb->area.y1 - pb->area.y0;
	bpp = NR_PIXBLOCK_BPP(pb);
	hval = pb->mode;
	hval = (hval << 5) - hval + pb->empty;
	hval = (hval << 5) - hval + pb->area.x0;
	hval = (hval << 5) - hval + pb->area.y0;
	hval = (hval << 5) - hval + pb->area.x1;
	hval = (hval << 5) - hval + pb->area.y1;
	for (y = 0; y < height; y++) {
		const unsigned char *s = NR_PIXBLOCK_ROW(pb, y);
		for (x = 0; x < width; x++) {
			for (c = 0; c < bpp; c++) {
				hval = (hval << 5) - hval + *s++;
			}
		}
	}
	return hval;
}

unsigned int
nr_pixblock_is_equal (const NRPixBlock *a, const NRPixBlock *b)
{
	unsigned int width, height, bpp, x, y, c;
	if (a == b) return 1;
	if (a->mode != b->mode) return 0;
	if (a->empty != b->empty) return 0;
	if (a->area.x0 - b->area.x0) return 0;
	if (a->area.y0 - b->area.y0) return 0;
	if (a->area.x1 - b->area.x1) return 0;
	if (a->area.y1 - b->area.y1) return 0;
	width = a->area.x1 - a->area.x0;
	height = a->area.y1 - a->area.y0;
	bpp = NR_PIXBLOCK_BPP(a);
	for (y = 0; y < height; y++) {
		const unsigned char *pa = NR_PIXBLOCK_ROW(a, y);
		const unsigned char *pb = NR_PIXBLOCK_ROW(b, y);
		for (x = 0; x < width; x++) {
			for (c = 0; c < bpp; c++) {
				if (*pa++ != *pb++) return 0;
			}
		}
	}
	return 1;
}

/* PixelStore operations */

#define NR_4K_BLOCK 32
static unsigned char **nr_4K_px = NULL;
static unsigned int nr_4K_len = 0;
static unsigned int nr_4K_size = 0;

unsigned char *
nr_pixelstore_4K_new (int clear, unsigned char val)
{
	unsigned char *px;

	if (nr_4K_len != 0) {
		nr_4K_len -= 1;
		px = nr_4K_px[nr_4K_len];
	} else {
		px = nr_new (unsigned char, 4096);
	}
	
	if (clear) memset (px, val, 4096);

	return px;
}

void
nr_pixelstore_4K_free (unsigned char *px)
{
	if (nr_4K_len == nr_4K_size) {
		nr_4K_size += NR_4K_BLOCK;
		nr_4K_px = nr_renew (nr_4K_px, unsigned char *, nr_4K_size);
	}

	nr_4K_px[nr_4K_len] = px;
	nr_4K_len += 1;
}

#define NR_16K_BLOCK 32
static unsigned char **nr_16K_px = NULL;
static unsigned int nr_16K_len = 0;
static unsigned int nr_16K_size = 0;

unsigned char *
nr_pixelstore_16K_new (int clear, unsigned char val)
{
	unsigned char *px;

	if (nr_16K_len != 0) {
		nr_16K_len -= 1;
		px = nr_16K_px[nr_16K_len];
	} else {
		px = nr_new (unsigned char, 16384);
	}
	
	if (clear) memset (px, val, 16384);

	return px;
}

void
nr_pixelstore_16K_free (unsigned char *px)
{
	if (nr_16K_len == nr_16K_size) {
		nr_16K_size += NR_16K_BLOCK;
		nr_16K_px = nr_renew (nr_16K_px, unsigned char *, nr_16K_size);
	}

	nr_16K_px[nr_16K_len] = px;
	nr_16K_len += 1;
}

#define NR_64K_BLOCK 32
static unsigned char **nr_64K_px = NULL;
static unsigned int nr_64K_len = 0;
static unsigned int nr_64K_size = 0;

unsigned char *
nr_pixelstore_64K_new (int clear, unsigned char val)
{
	unsigned char *px;

	if (nr_64K_len != 0) {
		nr_64K_len -= 1;
		px = nr_64K_px[nr_64K_len];
	} else {
		px = nr_new (unsigned char, 65536);
	}

	if (clear) memset (px, val, 65536);

	return px;
}

void
nr_pixelstore_64K_free (unsigned char *px)
{
	if (nr_64K_len == nr_64K_size) {
		nr_64K_size += NR_64K_BLOCK;
		nr_64K_px = nr_renew (nr_64K_px, unsigned char *, nr_64K_size);
	}

	nr_64K_px[nr_64K_len] = px;
	nr_64K_len += 1;
}

