#ifndef __SP_STYLE_H__
#define __SP_STYLE_H__

/*
 * SPStyle - a style object for SPItems
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_vpath_dash.h>
#include <libart_lgpl/art_svp_vpath_stroke.h>
#include "color.h"
#include "forward.h"

/* Colors */

typedef struct _SPPaint SPPaint;
typedef struct _SPDistance SPDistance;

typedef enum {
	SP_PAINT_TYPE_NONE,
	SP_PAINT_TYPE_COLOR,
	SP_PAINT_TYPE_PAINTSERVER
} SPPaintType;

typedef enum {
	SP_UNIT_PIXELS   = (1 << 4),
	SP_UNIT_ABSOLUTE = (1 << 5),
	SP_UNIT_USER     = (1 << 6),
	SP_UNIT_PERCENT  = (1 << 7),
	SP_UNIT_EM       = (1 << 8),
	SP_UNIT_EX       = (1 << 9)
} SPUnit;

struct _SPPaint {
	SPPaintType type;
	SPColor color;
	SPPaintServer *server;
};

struct _SPDistance {
	SPUnit unit;
	gdouble distance;
};

struct _SPStyle {
	gint refcount;
	/* Object we are attached to */
	/* fixme: I am not sure, whether style should be SPObject itself */
	/* fixme: Or alternatively, whole style to be moved inside SPObject[Styled] */
	SPObject *object;
	/* Global opacity */
	gdouble opacity;
	guint opacity_set : 1;
	/* Computed value */
	gdouble real_opacity;
	guint real_opacity_set : 1;
	/* display */
	guint display : 1;
	guint display_set : 1;
	/* visibility */
	guint visibility : 1;
	guint visibility_set : 1;
	/* fill */
	SPPaint fill;
	guint fill_set : 1;
	/* fill-rule: 0 nonzero, 1 evenodd */
	guint fill_rule : 1;
	guint fill_rule_set : 1;
	/* fill-opacity */
	gdouble fill_opacity;
	guint fill_opacity_set : 1;
	/* stroke */
	SPPaint stroke;
	guint stroke_set : 1;
	/* stroke-width */
	SPDistance stroke_width;
	guint stroke_width_set : 1;
	/* Computed value */
	gdouble absolute_stroke_width;
	gdouble user_stroke_width;
	guint real_stroke_width_set : 1;
	/* stroke-linecap */
	ArtPathStrokeCapType stroke_linecap;
	guint stroke_linecap_set : 1;
	/* stroke-linejoin */
	ArtPathStrokeJoinType stroke_linejoin;
	guint stroke_linejoin_set : 1;
	/* stroke-miterlimit */
	gdouble stroke_miterlimit;
	guint stroke_miterlimit_set;
	/* stroke-dash* */
	ArtVpathDash stroke_dash;
	guint stroke_dasharray_set : 1;
	guint stroke_dashoffset_set : 1;
	/* stroke-opacity */
	gdouble stroke_opacity;
	guint stroke_opacity_set : 1;
};

#define SP_RGBA_FROM_COLOR(c,o) (((guint32)((c)->r*255)<<24)|((guint32)((c)->g*255)<<16)|((guint32)((c)->b*255)<<8)|((guint32)((o)*255)))

SPStyle *sp_style_new (SPObject *object);

SPStyle *sp_style_ref (SPStyle *style);
SPStyle *sp_style_unref (SPStyle *style);

void sp_style_read_from_string (SPStyle *style, const guchar *str, SPDocument *document);
void sp_style_read_from_object (SPStyle *style, SPObject *object);
void sp_style_merge_from_object (SPStyle *style, SPObject *object);

guchar *sp_style_write_string (SPStyle *style);

END_GNOME_DECLS

#endif
