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

typedef struct _SPInheritedString SPInheritedString;
typedef struct _SPInheritedFloat SPInheritedFloat;
typedef struct _SPInheritedScale30 SPInheritedScale30;
typedef struct _SPInheritedInt SPInheritedInt;
typedef struct _SPInheritedShort SPInheritedShort;
typedef struct _SPInheritedPaint SPInheritedPaint;

struct _SPInheritedString {
	guint set : 1;
	guint inherit : 1;
	guint data : 30;
	guchar *value;
};

struct _SPInheritedFloat {
	guint set : 1;
	guint inherit : 1;
	guint data : 30;
	gfloat value;
};

#define SP_SCALE30_MAX ((1 << 30) - 1)
#define SP_SCALE30_TO_FLOAT(v) ((float) (v) / SP_SCALE30_MAX)
#define SP_SCALE30_FROM_FLOAT(v) ((gint) floor ((v) * (SP_SCALE30_MAX + 0.9999)))

struct _SPInheritedScale30 {
	guint set : 1;
	guint inherit : 1;
	guint value : 30;
};

struct _SPInheritedInt {
	guint set : 1;
	guint inherit : 1;
	guint data : 30;
	gint value;
};

struct _SPInheritedShort {
	guint set : 1;
	guint inherit : 1;
	guint data : 14;
	gshort value;
};

#define SP_STYLE_FILL_SERVER(s) (((SPStyle *) (s))->fill.server)
#define SP_STYLE_STROKE_SERVER(s) (((SPStyle *) (s))->stroke.server)
#define SP_OBJECT_STYLE_FILL_SERVER(o) (SP_OBJECT (o)->style->fill.server)
#define SP_OBJECT_STYLE_STROKE_SERVER(o) (SP_OBJECT (o)->style->stroke.server)

typedef enum {
	SP_PAINT_TYPE_NONE,
	SP_PAINT_TYPE_COLOR,
	SP_PAINT_TYPE_PAINTSERVER
} SPPaintType;

struct _SPInheritedPaint {
	guint set : 1;
	guint inherit : 1;
	guint type : 3;
	SPColor color;
	SPPaintServer *server;
};

typedef struct _SPTextStyle SPTextStyle;

struct _SPStyle {
	gint refcount;
	/* Object we are attached to */
	/* fixme: I am not sure, whether style should be SPObject itself */
	/* fixme: Or alternatively, whole style to be moved inside SPObject[Styled] */
	SPObject *object;
	/* Our text style component */
	SPTextStyle *text;
	guint text_private : 1;

	/* Misc attributes */
	guint clip_set : 1;
	guint color_set : 1;
	guint cursor_set : 1;
	guint display_set : 1;
	guint overflow_set : 1;
	guint visibility_set : 1;
	guint clip_path_set : 1;
	guint clip_rule_set : 1;
	guint mask_set : 1;

	/* opacity */
	SPInheritedScale30 opacity;

	/* display */
	guint display : 1;
	/* visibility */
	guint visibility : 1;

	/* fill */
	SPInheritedPaint fill;
	/* fill-opacity */
	SPInheritedScale30 fill_opacity;
	/* fill-rule: 0 nonzero, 1 evenodd */
	guint fill_rule : 3;
	guint fill_rule_set : 1;

	/* stroke */
	SPInheritedPaint stroke;
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
	SPInheritedScale30 stroke_opacity;
};

SPStyle *sp_style_new_from_object (SPObject *object);

SPStyle *sp_style_ref (SPStyle *style);
SPStyle *sp_style_unref (SPStyle *style);

/*
 * 1. Reset existing object style
 * 2. Load current effective object style
 * 3. Load inherited attributes from immediate parent (which has to be up-to-date)
 */

void sp_style_read_from_object (SPStyle *style, SPObject *object);

guchar *sp_style_write_string (SPStyle *style);

void sp_style_set_fill_color_rgba (SPStyle *style, gfloat r, gfloat g, gfloat b, gfloat a, gboolean fill_set, gboolean opacity_set);
void sp_style_set_fill_color_cmyka (SPStyle *style, gfloat c, gfloat m, gfloat y, gfloat k, gfloat a, gboolean fill_set, gboolean opacity_set);
void sp_style_set_stroke_color_rgba (SPStyle *style, gfloat r, gfloat g, gfloat b, gfloat a, gboolean fill_set, gboolean opacity_set);
void sp_style_set_stroke_color_cmyka (SPStyle *style, gfloat c, gfloat m, gfloat y, gfloat k, gfloat a, gboolean fill_set, gboolean opacity_set);

void sp_style_set_opacity (SPStyle *style, gfloat opacity, gboolean opacity_set);

/* SPTextStyle */

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

struct _SPTextStyle {
	gint refcount;

	/* CSS font properties */
	SPInheritedString font;
	SPInheritedString font_family;

	guint font_size_set : 1;
	guint font_size_adjust_set : 1;
	guint font_stretch_set : 1;
	guint font_style_set : 1;
	guint font_variant_set : 1;
	guint font_weight_set : 1;

	/* fixme: SPDistance */
	gfloat font_size;
	/* fixme: Has to have 'none' option here */
	gfloat font_size_adjust;
	guint font_stretch : 4;
	guint font_style : 2;
	guint font_variant : 1;
	guint font_weight : 4;

	/* CSS text properties */
	guint direction_set : 1;
	guint letter_spacing_set : 1;
	guint text_decoration_set : 1;
	guint unicode_bidi_set : 1;
	guint word_spacing_set : 1;

	guint direction : 2;
	SPDistance letter_spacing;
	guint text_decoration : 3;
	guint unicode_bidi : 2;
	SPDistance word_spacing;

	/* Parsed values */
	GnomeFontFace *face;
	gfloat size;
	/* Text direction */
	SPInheritedShort writing_mode;
};

/*
 * SVG style properties
 *
 * Maybe they should go to source instead, but at moment they are
 * here for everyone to use
 */

enum {
	SP_PROP_INVALID,
	/* CSS2 */
	/* Font */
	SP_PROP_FONT,
	SP_PROP_FONT_FAMILY,
	SP_PROP_FONT_SIZE,
	SP_PROP_FONT_SIZE_ADJUST,
	SP_PROP_FONT_STRETCH,
	SP_PROP_FONT_STYLE,
	SP_PROP_FONT_VARIANT,
	SP_PROP_FONT_WEIGHT,
	/* Text */
	SP_PROP_DIRECTION,
	SP_PROP_LETTER_SPACING,
	SP_PROP_TEXT_DECORATION,
	SP_PROP_UNICODE_BIDI,
	SP_PROP_WORD_SPACING,
	/* Misc */
	SP_PROP_CLIP,
	SP_PROP_COLOR,
	SP_PROP_CURSOR,
	SP_PROP_DISPLAY,
	SP_PROP_OVERFLOW,
	SP_PROP_VISIBILITY,
	/* SVG */
	/* Clip/Mask */
	SP_PROP_CLIP_PATH,
	SP_PROP_CLIP_RULE,
	SP_PROP_MASK,
	SP_PROP_OPACITY,
	/* Filter */
	SP_PROP_ENABLE_BACKGROUND,
	SP_PROP_FILTER,
	SP_PROP_FLOOD_COLOR,
	SP_PROP_FLOOD_OPACITY,
	SP_PROP_LIGHTING_COLOR,
	/* Gradient */
	SP_PROP_STOP_COLOR,
	SP_PROP_STOP_OPACITY,
	/* Interactivity */
	SP_PROP_POINTER_EVENTS,
	/* Paint */
	SP_PROP_COLOR_INTERPOLATION,
	SP_PROP_COLOR_INTERPOLATION_FILTERS,
	SP_PROP_COLOR_PROFILE,
	SP_PROP_COLOR_RENDERING,
	SP_PROP_FILL,
	SP_PROP_FILL_OPACITY,
	SP_PROP_FILL_RULE,
	SP_PROP_IMAGE_RENDERING,
	SP_PROP_MARKER,
	SP_PROP_MARKER_END,
	SP_PROP_MARKER_MID,
	SP_PROP_MARKER_START,
	SP_PROP_SHAPE_RENDERING,
	SP_PROP_STROKE,
	SP_PROP_STROKE_DASHARRAY,
	SP_PROP_STROKE_DASHOFFSET,
	SP_PROP_STROKE_LINECAP,
	SP_PROP_STROKE_LINEJOIN,
	SP_PROP_STROKE_MITERLIMIT,
	SP_PROP_STROKE_OPACITY,
	SP_PROP_STROKE_WIDTH,
	SP_PROP_TEXT_RENDERING,
	/* Text */
	SP_PROP_ALIGNMENT_BASELINE,
	SP_PROP_BASELINE_SHIFT,
	SP_PROP_DOMINANT_BASELINE,
	SP_PROP_GLYPH_ORIENTATION_HORIZONTAL,
	SP_PROP_GLYPH_ORIENTATION_VERTICAL,
	SP_PROP_KERNING,
	SP_PROP_TEXT_ANCHOR,
	SP_PROP_WRITING_MODE
};

END_GNOME_DECLS

#endif
