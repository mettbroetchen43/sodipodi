#ifndef __NR_BUFFERS_H__
#define __NR_BUFFERS_H__

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

#include <glib.h>

typedef struct _NRBuffer NRBuffer;

typedef enum {
	NR_SIZE_4K,
	NR_SIZE_16K,
	NR_SIZE_64K,
	NR_SIZE_BIG
} NRSizeClass;

typedef enum {
	NR_IMAGE_A8,
	NR_IMAGE_R8G8B8A8
} NRImageMode;

struct _NRBuffer {
	guint size : 4;
	guint mode : 4;
	guint empty : 1;
	guint premul : 1;
	gint w, h, rs;
	guchar *px;
};

NRBuffer *nr_buffer_get (NRImageMode mode, gint w, gint h, gboolean clear, gboolean premul);
NRBuffer *nr_buffer_free (NRBuffer *buffer);

guchar *nr_buffer_1_4096_get (gboolean clear, guint val);
void nr_buffer_1_4096_free (guchar *buf);

guchar *nr_buffer_4_4096_get (gboolean clear, guint32 rgba);
void nr_buffer_4_4096_free (guchar *buf);

#endif
