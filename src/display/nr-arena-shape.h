#ifndef __NR_ARENA_SHAPE_H__
#define __NR_ARENA_SHAPE_H__

/*
 * RGBA display list system for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#define NR_TYPE_ARENA_SHAPE (nr_arena_shape_get_type ())
#define NR_ARENA_SHAPE(obj) (GTK_CHECK_CAST ((obj), NR_TYPE_ARENA_SHAPE, NRArenaShape))
#define NR_ARENA_SHAPE_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), NR_TYPE_ARENA_SHAPE, NRArenaShapeClass))
#define NR_IS_ARENA_SHAPE(obj) (GTK_CHECK_TYPE ((obj), NR_TYPE_ARENA_SHAPE))
#define NR_IS_ARENA_SHAPE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), NR_TYPE_ARENA_SHAPE))

#include "nr-arena-item.h"
#include "cpath-component.h"
#include "../forward.h"
#include "../sp-paint-server.h"

struct _NRArenaShape {
	NRArenaItem item;
	/* Shape data */
	SPCurve *curve;
	SPStyle *style;
	ArtDRect paintbox;
	/* State data */
	SPPainter *fill_painter;
	SPPainter *stroke_painter;
	ArtSVP *fill_svp;
	ArtSVP *stroke_svp;
};

struct _NRArenaShapeClass {
	NRArenaItemClass parent_class;
};

GtkType nr_arena_shape_get_type (void);

void nr_arena_shape_set_path (NRArenaShape *shape, SPCurve *curve, gboolean private, const gdouble *affine);
void nr_arena_shape_set_style (NRArenaShape *shape, SPStyle *style);
void nr_arena_shape_set_paintbox (NRArenaShape *shape, const ArtDRect *pbox);

#endif
