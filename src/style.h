#ifndef __SP_STYLE_H__
#define __SP_STYLE_H__

/*
 * SPStyle - a style object for SPItems
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_vpath_dash.h>
#include "xml/repr.h"
#include "color.h"
#include "forward.h"

typedef struct _SPIFloat SPIFloat;
typedef struct _SPIScale24 SPIScale24;
typedef struct _SPIInt SPIInt;
typedef struct _SPIShort SPIShort;
typedef struct _SPIEnum SPIEnum;
typedef struct _SPIString SPIString;
typedef struct _SPILength SPILength;
typedef struct _SPIPaint SPIPaint;
typedef struct _SPIFontSize SPIFontSize;

struct _SPIFloat {
	guint set : 1;
	guint inherit : 1;
	guint data : 30;
	gfloat value;
};

#define SP_SCALE24_MAX ((1 << 24) - 1)
#define SP_SCALE24_TO_FLOAT(v) ((float) ((double) (v) / SP_SCALE24_MAX))
#define SP_SCALE24_FROM_FLOAT(v) ((int) ((v) * ((double) SP_SCALE24_MAX + 0.9999)))

struct _SPIScale24 {
	guint set : 1;
	guint inherit : 1;
	guint value : 24;
};

struct _SPIInt {
	guint set : 1;
	guint inherit : 1;
	guint data : 30;
	gint value;
};

struct _SPIShort {
	guint set : 1;
	guint inherit : 1;
	guint data : 14;
	gint value : 16;
};

struct _SPIEnum {
	guint set : 1;
	guint inherit : 1;
	guint value : 8;
	guint computed : 8;
};

struct _SPIString {
	guint set : 1;
	guint inherit : 1;
	guint data : 30;
	guchar *value;
};

enum {
	SP_CSS_UNIT_NONE,
	SP_CSS_UNIT_PX,
	SP_CSS_UNIT_PT,
	SP_CSS_UNIT_PC,
	SP_CSS_UNIT_MM,
	SP_CSS_UNIT_CM,
	SP_CSS_UNIT_IN,
	SP_CSS_UNIT_EM,
	SP_CSS_UNIT_EX,
	SP_CSS_UNIT_PERCENT
};

struct _SPILength {
	guint set : 1;
	guint inherit : 1;
	guint unit : 4;
	gfloat value;
	gfloat computed;
};

#define SP_STYLE_FILL_SERVER(s) (((SPStyle *) (s))->fill.value.server)
#define SP_STYLE_STROKE_SERVER(s) (((SPStyle *) (s))->stroke.value.server)
#define SP_OBJECT_STYLE_FILL_SERVER(o) (SP_OBJECT (o)->style->fill.value.server)
#define SP_OBJECT_STYLE_STROKE_SERVER(o) (SP_OBJECT (o)->style->stroke.value.server)

enum {
	SP_PAINT_TYPE_NONE,
	SP_PAINT_TYPE_COLOR,
	SP_PAINT_TYPE_PAINTSERVER
};

struct _SPIPaint {
	guint set : 1;
	guint inherit : 1;
	guint type : 2;
	union {
		SPColor color;
		SPPaintServer *server;
	} value;
};

enum {
	SP_FONT_SIZE_LITERAL,
	SP_FONT_SIZE_LENGTH,
	SP_FONT_SIZE_PERCENTAGE
};

#define SP_FONT_SIZE ((1 << 24) - 1)

#define SP_F8_16_TO_FLOAT(v) ((gdouble) (v) / (1 << 16))
#define SP_F8_16_FROM_FLOAT(v) ((gint) floor ((v) * ((1 << 16) + 0.9999)))

struct _SPIFontSize {
	guint set : 1;
	guint inherit : 1;
	guint type : 2;
	guint value : 24;
	gfloat computed;
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

	/* CSS2 */
	/* Font */
	SPIFontSize font_size;
	SPIEnum font_style;
	SPIEnum font_variant;
	SPIEnum font_weight;
	SPIEnum font_stretch;

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
	SPIScale24 opacity;

	/* display */
	guint display : 1;
	/* visibility */
	guint visibility : 1;

	/* fill */
	SPIPaint fill;
	/* fill-opacity */
	SPIScale24 fill_opacity;
	/* fill-rule: 0 nonzero, 1 evenodd */
	SPIEnum fill_rule;

	/* stroke */
	SPIPaint stroke;
	/* stroke-width */
	SPILength stroke_width;
	/* stroke-linecap */
	SPIEnum stroke_linecap;
	/* stroke-linejoin */
	SPIEnum stroke_linejoin;
	/* stroke-miterlimit */
	SPIFloat stroke_miterlimit;
	/* stroke-dash* */
	ArtVpathDash stroke_dash;
	guint stroke_dasharray_set : 1;
	guint stroke_dashoffset_set : 1;
	/* stroke-opacity */
	SPIScale24 stroke_opacity;
	/* fixme: remove this */

	/* SVG */
	SPIEnum text_anchor;
	SPIEnum writing_mode;
};

SPStyle *sp_style_new (void);
SPStyle *sp_style_new_from_object (SPObject *object);

SPStyle *sp_style_ref (SPStyle *style);
SPStyle *sp_style_unref (SPStyle *style);

/*
 * 1. Reset existing object style
 * 2. Load current effective object style
 * 3. Load i attributes from immediate parent (which has to be up-to-date)
 */

void sp_style_read_from_object (SPStyle *style, SPObject *object);
void sp_style_read_from_repr (SPStyle *style, SPRepr *repr);
void sp_style_merge_from_parent (SPStyle *style, SPStyle *parent);

guchar *sp_style_write_string (SPStyle *style);
guchar *sp_style_write_difference (SPStyle *from, SPStyle *to);

void sp_style_set_fill_color_rgba (SPStyle *style, gfloat r, gfloat g, gfloat b, gfloat a, gboolean fill_set, gboolean opacity_set);
void sp_style_set_fill_color_cmyka (SPStyle *style, gfloat c, gfloat m, gfloat y, gfloat k, gfloat a, gboolean fill_set, gboolean opacity_set);
void sp_style_set_stroke_color_rgba (SPStyle *style, gfloat r, gfloat g, gfloat b, gfloat a, gboolean fill_set, gboolean opacity_set);
void sp_style_set_stroke_color_cmyka (SPStyle *style, gfloat c, gfloat m, gfloat y, gfloat k, gfloat a, gboolean fill_set, gboolean opacity_set);

void sp_style_set_opacity (SPStyle *style, gfloat opacity, gboolean opacity_set);

/* SPTextStyle */

typedef enum {
	SP_CSS_FONT_SIZE_XX_SMALL,
	SP_CSS_FONT_SIZE_X_SMALL,
	SP_CSS_FONT_SIZE_SMALL,
	SP_CSS_FONT_SIZE_MEDIUM,
	SP_CSS_FONT_SIZE_LARGE,
	SP_CSS_FONT_SIZE_X_LARGE,
	SP_CSS_FONT_SIZE_XX_LARGE,
	SP_CSS_FONT_SIZE_SMALLER,
	SP_CSS_FONT_SIZE_LARGER
} SPCSSFontSize;

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
	SP_CSS_TEXT_ANCHOR_START,
	SP_CSS_TEXT_ANCHOR_MIDDLE,
	SP_CSS_TEXT_ANCHOR_END
} SPTextAnchor;

typedef enum {
	SP_CSS_WRITING_MODE_LR,
	SP_CSS_WRITING_MODE_RL,
	SP_CSS_WRITING_MODE_TB
} SPWritingMode;

struct _SPTextStyle {
	gint refcount;

	/* CSS font properties */
	SPIString font_family;

	guint font_size_adjust_set : 1;

	/* fixme: Has to have 'none' option here */
	gfloat font_size_adjust;

	/* fixme: The 'font' property is ugly, and not working (lauris) */
	SPIString font;

	/* CSS text properties */
	guint direction_set : 1;
	guint text_decoration_set : 1;
	guint unicode_bidi_set : 1;

	guint direction : 2;
	guint text_decoration : 3;
	guint unicode_bidi : 2;
};

#endif
