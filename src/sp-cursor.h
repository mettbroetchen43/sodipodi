#ifndef __SP_CURSOR_H__
#define __SP_CURSOR_H__

/*
 * SVG <clipPath> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gdk/gdk.h>

void sp_cursor_bitmap_and_mask_from_xpm (GdkBitmap **bitmap, GdkBitmap **mask, gchar **xpm);
GdkCursor * sp_cursor_new_from_xpm (gchar ** xpm, gint hot_x, gint hot_y);

#endif
