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

	return buf;
}

void
nr_buffer_4_4096_free (guchar *buf)
{
	g_return_if_fail (buf != NULL);

	bl_4_4096 = g_slist_prepend (bl_4_4096, buf);
}
