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
#include <libgnomeprint/gnome-font-face.h>
#include "helper/units.h"
#include "color.h"
#include "forward.h"

/* Colors */

typedef struct _SPPaint SPPaint;

typedef enum {
	SP_PAINT_TYPE_NONE,
	SP_PAINT_TYPE_COLOR,
	SP_PAINT_TYPE_PAINTSERVER
} SPPaintType;

struct _SPPaint {
	SPPaintType type;
	SPColor color;
	SPPaintServer *server;
};

#define SP_STYLE_FILL_SERVER(s) (((SPStyle *) (s))->fill.server)
#define SP_STYLE_STROKE_SERVER(s) (((SPStyle *) (s))->stroke.server)
#define SP_OBJECT_STYLE_FILL_SERVER(o) (SP_OBJECT (o)->style->fill.server)
#define SP_OBJECT_STYLE_STROKE_SERVER(o) (SP_OBJECT (o)->style->stroke.server)

typedef struct _SPStyleText SPStyleText;

struct _SPStyle {
	gint refcount;
	/* Object we are attached to */
	/* fixme: I am not sure, whether style should be SPObject itself */
	/* fixme: Or alternatively, whole style to be moved inside SPObject[Styled] */
	SPObject *object;
	/* Our text style component */
	SPStyleText *text;
	guint text_private : 1;
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
	guint fill_rule : 3;
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

SPStyle *sp_style_new (SPObject *object);

SPStyle *sp_style_ref (SPStyle *style);
SPStyle *sp_style_unref (SPStyle *style);

void sp_style_read_from_object (SPStyle *style, SPObject *object);
void sp_style_merge_from_object (SPStyle *style, SPObject *object);

guchar *sp_style_write_string (SPStyle *style);

void sp_style_set_fill_color_rgba (SPStyle *style, gfloat r, gfloat g, gfloat b, gfloat a, gboolean fill_set, gboolean opacity_set);
void sp_style_set_fill_color_cmyka (SPStyle *style, gfloat c, gfloat m, gfloat y, gfloat k, gfloat a, gboolean fill_set, gboolean opacity_set);
void sp_style_set_stroke_color_rgba (SPStyle *style, gfloat r, gfloat g, gfloat b, gfloat a, gboolean fill_set, gboolean opacity_set);
void sp_style_set_stroke_color_cmyka (SPStyle *style, gfloat c, gfloat m, gfloat y, gfloat k, gfloat a, gboolean fill_set, gboolean opacity_set);

void sp_style_set_opacity (SPStyle *style, gfloat opacity, gboolean opacity_set);

/* SPStyleText */

typedef enum {
	SP_CSS_FONT_STYLE_NORMAL,
	SP_CSS_FONT_STYLE_ITALIC,
	SP_CSS_FONT_STYLE_OBLIQUE
} SPCSSFontStyle;

typedef enum {
	SP_CSS_FONT_VARIANT_NORMAL,
	SP_CSS_FONT_VARIANT_SMALL_CAPS
} SPCSSFontVariant;

typedef enum {
	SP_CSS_FONT_WEIGHT_100,
	SP_CSS_FONT_WEIGHT_200,
	SP_CSS_FONT_WEIGHT_300,
	SP_CSS_FONT_WEIGHT_400,
	SP_CSS_FONT_WEIGHT_500,
	SP_CSS_FONT_WEIGHT_600,
	SP_CSS_FONT_WEIGHT_700,
	SP_CSS_FONT_WEIGHT_800,
	SP_CSS_FONT_WEIGHT_900,
	SP_CSS_FONT_WEIGHT_NORMAL,
	SP_CSS_FONT_WEIGHT_BOLD,
	SP_CSS_FONT_WEIGHT_LIGHTER,
	SP_CSS_FONT_WEIGHT_DARKER
} SPCSSFontWeight;

typedef enum {
	SP_CSS_FONT_STRETCH_ULTRA_CONDENSED,
	SP_CSS_FONT_STRETCH_EXTRA_CONDENSED,
	SP_CSS_FONT_STRETCH_CONDENSED,
	SP_CSS_FONT_STRETCH_SEMI_CONDENSED,
	SP_CSS_FONT_STRETCH_NORMAL,
	SP_CSS_FONT_STRETCH_SEMI_EXPANDED,
	SP_CSS_FONT_STRETCH_EXPANDED,
	SP_CSS_FONT_STRETCH_EXTRA_EXPANDED,
	SP_CSS_FONT_STRETCH_ULTRA_EXPANDED,
	SP_CSS_FONT_STRETCH_NARROWER,
	SP_CSS_FONT_STRETCH_WIDER
} SPCSSFontStretch;

typedef enum {
	SP_CSS_WRITING_MODE_LR,
	SP_CSS_WRITING_MODE_RL,
	SP_CSS_WRITING_MODE_TB
} SPWritingMode;

struct _SPStyleText {
	gint refcount;
	/* SVG properties */
	guchar *font_family;
	guint font_family_set : 1;

	/* fixme: relatives, units, percentages... */
	gfloat font_size;
	guint font_size_set : 1;
#if 0
	/* fixme: Has to have 'none' option here */
	gfloat font_size_adjust;
	guint font_size_adjust_set : 1;
#endif
	guint font_style : 2;
	guint font_style_set : 1;

	guint font_variant : 1;
	guint font_variant_set : 1;

	guint font_weight : 4;
	guint font_weight_set : 1;

	guint font_stretch : 4;
	guint font_stretch_set : 1;

	/* Parsed values */
	GnomeFontFace *face;
	gfloat size;
	/* Text direction */
	SPWritingMode writing_mode;
	guint writing_mode_set : 1;
};

END_GNOME_DECLS

#endif
