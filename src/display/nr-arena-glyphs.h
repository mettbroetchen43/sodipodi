#ifndef __NR_ARENA_GLYPHS_H__
#define __NR_ARENA_GLYPHS_H__

/*
 * RGBA display list system for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 *
 */

#define NR_TYPE_ARENA_GLYPHS (nr_arena_glyphs_get_type ())
#define NR_ARENA_GLYPHS(obj) (GTK_CHECK_CAST ((obj), NR_TYPE_ARENA_GLYPHS, NRArenaGlyphs))
#define NR_ARENA_GLYPHS_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), NR_TYPE_ARENA_GLYPHS, NRArenaGlyphsClass))
#define NR_IS_ARENA_GLYPHS(obj) (GTK_CHECK_TYPE ((obj), NR_TYPE_ARENA_GLYPHS))
#define NR_IS_ARENA_GLYPHS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), NR_TYPE_ARENA_GLYPHS))

#include "nr-arena-item.h"
#include "cpath-component.h"
#include "../forward.h"
#include "../sp-paint-server.h"

struct _NRArenaGlyphs {
	NRArenaItem item;
	/* Glyphs data */
	SPCurve *curve;
	SPStyle *style;
	/* State data */
	SPPainter *fill_painter;
	SPPainter *stroke_painter;
	ArtSVP *fill_svp;
	ArtSVP *stroke_svp;
};

struct _NRArenaGlyphsClass {
	NRArenaItemClass parent_class;
};

GtkType nr_arena_glyphs_get_type (void);

void nr_arena_glyphs_set_path (NRArenaGlyphs *glyphs, SPCurve *curve, gboolean private, const gdouble *affine);
void nr_arena_glyphs_set_style (NRArenaGlyphs *glyphs, SPStyle *style);

/* Integrated group of component glyphss */

typedef struct _NRArenaGlyphsGroup NRArenaGlyphsGroup;
typedef struct _NRArenaGlyphsGroupClass NRArenaGlyphsGroupClass;

#include "nr-arena-group.h"

#define NR_TYPE_ARENA_GLYPHS_GROUP (nr_arena_glyphs_group_get_type ())
#define NR_ARENA_GLYPHS_GROUP(obj) (GTK_CHECK_CAST ((obj), NR_TYPE_ARENA_GLYPHS_GROUP, NRArenaGlyphsGroup))
#define NR_ARENA_GLYPHS_GROUP_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), NR_TYPE_ARENA_GLYPHS_GROUP, NRArenaGlyphsGroupClass))
#define NR_IS_ARENA_GLYPHS_GROUP(obj) (GTK_CHECK_TYPE ((obj), NR_TYPE_ARENA_GLYPHS_GROUP))
#define NR_IS_ARENA_GLYPHS_GROUP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), NR_TYPE_ARENA_GLYPHS_GROUP))

struct _NRArenaGlyphsGroup {
	NRArenaGroup group;
	SPStyle *style;
};

struct _NRArenaGlyphsGroupClass {
	NRArenaGroupClass parent_class;
};

GtkType nr_arena_glyphs_group_get_type (void);

void nr_arena_glyphs_group_set_style (NRArenaGlyphsGroup *group, SPStyle *style);

/* Utility functions */

void nr_arena_glyphs_group_clear (NRArenaGlyphsGroup *group);
void nr_arena_glyphs_group_add_component (NRArenaGlyphsGroup *group, SPCurve *curve, gboolean private, const gdouble *affine);
void nr_arena_glyphs_group_set_component (NRArenaGlyphsGroup *group, SPCurve *curve, gboolean private, const gdouble *affine);

#endif
