#ifndef __NR_ARENA_IMAGE_H__
#define __NR_ARENA_IMAGE_H__

/*
 * RGBA display list system for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define NR_TYPE_ARENA_IMAGE (nr_arena_image_get_type ())
#define NR_ARENA_IMAGE(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), NR_TYPE_ARENA_IMAGE, NRArenaImage))
#define NR_ARENA_IMAGE_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), NR_TYPE_ARENA_IMAGE, NRArenaImageClass))
#define NR_IS_ARENA_IMAGE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), NR_TYPE_ARENA_IMAGE))
#define NR_IS_ARENA_IMAGE_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), NR_TYPE_ARENA_IMAGE))

#include <libnr/nr-types.h>
#include "nr-arena-item.h"

struct _NRArenaImage {
	NRArenaItem item;

	GdkPixbuf *pixbuf;
	gdouble x, y;
	gdouble width, height;

	/* From GRID to PIXELS */
	NRMatrixF grid2px;
};

struct _NRArenaImageClass {
	NRArenaItemClass parent_class;
};

GType nr_arena_image_get_type (void);

void nr_arena_image_set_pixbuf (NRArenaImage *image, GdkPixbuf *pixbuf);
void nr_arena_image_set_geometry (NRArenaImage *image, gdouble x, gdouble y, gdouble width, gdouble height);

#endif
