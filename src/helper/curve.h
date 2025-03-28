#ifndef __SP_CURVE_H__
#define __SP_CURVE_H__

/*
 * Wrapper around ArtBpath
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 */

#include <libnr/nr-path.h>

#include <glib.h>
#include <libart_lgpl/art_bpath.h>

typedef struct _SPCurve SPCurve;

struct _SPCurve {
	gint refcount;
	ArtBpath * bpath;
	gint end;		/* ART_END position */
	gint length;		/* Num allocated Bpaths */
	gint substart;		/* subpath start */
	gdouble x, y;		/* previous moveto position */
	guint sbpath : 1;	/* Bpath is static */
	guint hascpt : 1;	/* Currentpoint is defined */
	guint posset : 1;	/* Previous was moveto */
	guint moving : 1;	/* Bpath end is moving */
	guint closed : 1;	/* All subpaths are closed */
};

#define SP_CURVE_LENGTH(c) (((SPCurve *)(c))->end)
#define SP_CURVE_BPATH(c) (((SPCurve *)(c))->bpath)
#define SP_CURVE_SEGMENT(c,i) (((SPCurve *)(c))->bpath + (i))
/* Constructors */

SPCurve * sp_curve_new (void);
SPCurve * sp_curve_new_sized (gint length);
SPCurve * sp_curve_new_from_bpath (ArtBpath * bpath);
SPCurve * sp_curve_new_from_static_bpath (ArtBpath * bpath);
SPCurve * sp_curve_new_from_foreign_bpath (ArtBpath * bpath);

SPCurve *sp_curve_new_from_nr_path (NRPath *path);

SPCurve *sp_curve_ref (SPCurve *curve);
SPCurve *sp_curve_unref (SPCurve *curve);

void sp_curve_finish (SPCurve * curve);
void sp_curve_ensure_space (SPCurve * curve, gint space);
SPCurve * sp_curve_copy (SPCurve * curve);
SPCurve * sp_curve_concat (const GSList * list);
GSList * sp_curve_split (SPCurve * curve);
SPCurve *sp_curve_transform (SPCurve *curve, const gdouble transform[]);

/* Methods */

void sp_curve_reset (SPCurve * curve);

void sp_curve_moveto (SPCurve * curve, gdouble x, gdouble y);
void sp_curve_lineto (SPCurve * curve, gdouble x, gdouble y);
void sp_curve_lineto_moving (SPCurve * curve, gdouble x, gdouble y);
void sp_curve_curveto (SPCurve * curve, gdouble x0, gdouble y0, gdouble x1, gdouble y1, gdouble x2, gdouble y2);
void sp_curve_closepath (SPCurve * curve);
void sp_curve_closepath_current (SPCurve * curve);

SPCurve *sp_curve_append_continuous (SPCurve *c0, SPCurve *c1, gdouble tolerance);

#define sp_curve_is_empty sp_curve_empty
gboolean sp_curve_empty (SPCurve * curve);
ArtBpath *sp_curve_last_bpath (SPCurve *curve);
ArtBpath *sp_curve_first_bpath (SPCurve *curve);

void sp_curve_append (SPCurve *curve, SPCurve *curve2, gboolean use_lineto);
SPCurve *sp_curve_reverse (SPCurve *curve);
void sp_curve_backspace (SPCurve *curve);

#endif
