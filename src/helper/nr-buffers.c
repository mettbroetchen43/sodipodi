#define __NR_BUFFERS_C__

/*
 * Quick buffer cache
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <string.h>
#include "nr-buffers.h"

static GSList *bl_4K = NULL;
static GSList *bl_16K = NULL;
static GSList *bl_64K = NULL;

NRBuffer *
nr_buffer_get (NRImageMode mode, gint w, gint h, gboolean clear, gboolean premul)
{
	NRBuffer *b;
	gint bpp;

	g_return_val_if_fail (w > 0, NULL);
	g_return_val_if_fail (h > 0, NULL);

	switch (mode) {
	case NR_IMAGE_A8:
		bpp = 1;
		break;
	case NR_IMAGE_R8G8B8A8:
		bpp = 4;
		break;
	default:
		g_warning ("file %s: line %d: Illegal buffer mode %d", __FILE__, __LINE__, mode);
		return NULL;
	}

	if (w * h * bpp <= 4096) {
		/* 4K buffer */
		if (bl_4K) {
			b = bl_4K->data;
			bl_4K = g_slist_remove (bl_4K, b);
		} else {
			b = g_new (NRBuffer, 1);
			b->px = g_new (guchar, 4096);
			b->size = NR_SIZE_4K;
		}
	} else if (w * h * bpp <= 16384) {
		/* 16K buffer */
		if (bl_16K) {
			b = bl_16K->data;
			bl_16K = g_slist_remove (bl_16K, b);
		} else {
			b = g_new (NRBuffer, 1);
			b->px = g_new (guchar, 16384);
			b->size = NR_SIZE_16K;
		}
	} else if (w * h * bpp <= 65536) {
		/* 64K buffer */
		if (bl_64K) {
			b = bl_64K->data;
			bl_64K = g_slist_remove (bl_64K, b);
		} else {
			b = g_new (NRBuffer, 1);
			b->px = g_new (guchar, 65536);
			b->size = NR_SIZE_64K;
		}
	} else {
		/* Big buffer */
		b = g_new (NRBuffer, 1);
		b->px = g_new (guchar, w * h * bpp);
		b->size = NR_SIZE_BIG;
	}

	b->mode = mode;
	b->empty = TRUE;
	b->premul = premul;
	b->w = w;
	b->h = h;
	b->rs = bpp * w;

	if (clear) {
		memset (b->px, 0x0, w * h * bpp);
	}

	return b;
}

NRBuffer *
nr_buffer_free (NRBuffer *b)
{
	g_return_val_if_fail (b != NULL, NULL);

	switch (b->size) {
	case NR_SIZE_4K:
		bl_4K = g_slist_prepend (bl_4K, b);
		break;
	case NR_SIZE_16K:
		bl_16K = g_slist_prepend (bl_16K, b);
		break;
	case NR_SIZE_64K:
		bl_64K = g_slist_prepend (bl_64K, b);
		break;
	case NR_SIZE_BIG:
		g_free (b->px);
		g_free (b);
		break;
	default:
		g_warning ("file %s: line %d: Illegal buffer size code %d", __FILE__, __LINE__, b->size);
		break;
	}

	return NULL;
}

#if 0
static GSList *bl_1_4096 = NULL;

guchar *
nr_buffer_1_4096_get (gboolean clear, guint val)
{
	guchar *buf;

	if (bl_1_4096) {
		buf = bl_1_4096->data;
		bl_1_4096 = g_slist_remove (bl_1_4096, buf);
	} else {
		buf = g_new (guchar, 4096);
	}

	if (clear) memset (buf, 4096, val);

	return buf;
}

void
nr_buffer_1_4096_free (guchar *buf)
{
	g_return_if_fail (buf != NULL);

	bl_1_4096 = g_slist_prepend (bl_1_4096, buf);
}

static GSList *bl_4_4096 = NULL;

guchar *
nr_buffer_4_4096_get (gboolean clear, guint32 rgba)
{
	guchar *buf;

	if (bl_4_4096) {
		buf = bl_4_4096->data;
		bl_4_4096 = g_slist_remove (bl_4_4096, buf);
	} else {
		if (clear && !rgba) {
			buf = g_new0 (guchar, 4 * 4096);
			return buf;
		}
		buf = g_new (guchar, 4 * 4096);
	}

	if (clear) {
		if (!rgba) {
			memset (buf, 0x00, 4 * 4096);
		} else {
			guint r, g, b, a, i;
			guchar *p;
			r = rgba >> 24;
			g = (rgba >> 16) & 0xff;
			b = (rgba >> 8) & 0xff;
			a = rgba & 0xff;
			p = buf;
			for (i = 0; i < 4096; i++) {
				*p++ = r;
				*p++ = g;
				*p++ = b;
				*p++ = a;
			}
		}
	}

	return buf;
}

void
nr_buffer_4_4096_free (guchar *buf)
{
	g_return_if_fail (buf != NULL);

	bl_4_4096 = g_slist_prepend (bl_4_4096, buf);
}
#endif
