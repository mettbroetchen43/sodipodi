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

/* Integrated group of component shapes */

#include "nr-arena-group.h"

#define NR_TYPE_ARENA_SHAPE_GROUP (nr_arena_shape_group_get_type ())
#define NR_ARENA_SHAPE_GROUP(obj) (GTK_CHECK_CAST ((obj), NR_TYPE_ARENA_SHAPE_GROUP, NRArenaShapeGroup))
#define NR_ARENA_SHAPE_GROUP_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), NR_TYPE_ARENA_SHAPE_GROUP, NRArenaShapeGroupClass))
#define NR_IS_ARENA_SHAPE_GROUP(obj) (GTK_CHECK_TYPE ((obj), NR_TYPE_ARENA_SHAPE_GROUP))
#define NR_IS_ARENA_SHAPE_GROUP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), NR_TYPE_ARENA_SHAPE_GROUP))

struct _NRArenaShapeGroup {
	NRArenaGroup group;
	SPStyle *style;
};

struct _NRArenaShapeGroupClass {
	NRArenaGroupClass parent_class;
};

GtkType nr_arena_shape_group_get_type (void);

void nr_arena_shape_group_set_style (NRArenaShapeGroup *group, SPStyle *style);

/* Utility functions */

void nr_arena_shape_group_clear (NRArenaShapeGroup *group);
void nr_arena_shape_group_add_component (NRArenaShapeGroup *group, SPCurve *curve, gboolean private, const gdouble *affine);
void nr_arena_shape_group_set_component (NRArenaShapeGroup *group, SPCurve *curve, gboolean private, const gdouble *affine);

#endif
