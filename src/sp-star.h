#ifndef __SP_STAR_H__
#define __SP_STAR_H__

/*
 * <sodipodi:star> implementation
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-polygon.h"

BEGIN_GNOME_DECLS

#define SP_TYPE_STAR (sp_star_get_type ())
#define SP_STAR(o) (GTK_CHECK_CAST ((o), SP_TYPE_STAR, SPStar))
#define SP_STAR_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_STAR, SPStarClass))
#define SP_IS_STAR(o) (GTK_CHECK_TYPE ((o), SP_TYPE_STAR))
#define SP_IS_STAR_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_STAR))

typedef struct _SPStar SPStar;
typedef struct _SPStarClass SPStarClass;

typedef enum {
	SP_STAR_POINT_KNOT1,
	SP_STAR_POINT_KNOT2
} SPStarPoint;

struct _SPStar {
	SPPolygon polygon;

	gint sides;

	float cx, cy;
	float r1, r2;
	float arg1, arg2;
};

struct _SPStarClass {
	SPPolygonClass parent_class;
};

GtkType sp_star_get_type (void);

void sp_star_set (SPStar *star, gint sides, gdouble cx, gdouble cy, gdouble r1, gdouble r2, gdouble arg1, gdouble arg2);

void sp_star_get_xy (SPStar *star, SPStarPoint point, gint index, ArtPoint *p);

END_GNOME_DECLS

#endif
