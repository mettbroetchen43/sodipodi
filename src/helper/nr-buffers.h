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

guchar *nr_buffer_1_4096_get (gboolean clear, guint val);
void nr_buffer_1_4096_free (guchar *buf);

guchar *nr_buffer_4_4096_get (gboolean clear, guint32 rgba);
void nr_buffer_4_4096_free (guchar *buf);

#endif
