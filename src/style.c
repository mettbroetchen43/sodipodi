#define __SP_STYLE_C__

/*
 * SPStyle - a style object for SPItems
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_wind.h>
#include <gtk/gtksignal.h>
#include "svg/svg.h"
#include "document.h"
#include "uri-references.h"
#include "sp-paint-server.h"
#include "style.h"

typedef struct _SPStyleEnum SPStyleEnum;

static void sp_style_clear (SPStyle *style);

static void sp_style_merge_from_style_string (SPStyle *style, const guchar *p);
static void sp_style_merge_property (SPStyle *style, gint id, const guchar *val);

static void sp_style_merge_inherited_paint (SPStyle *style, SPInheritedPaint *paint, SPInheritedPaint *parent);
static void sp_style_read_dash (ArtVpathDash *dash, const guchar *str);

static SPTextStyle *sp_text_style_new (void);
static void sp_text_style_clear (SPTextStyle *ts);
static SPTextStyle *sp_text_style_ref (SPTextStyle *st);
static SPTextStyle *sp_text_style_unref (SPTextStyle *st);
static SPTextStyle *sp_text_style_duplicate_unset (SPTextStyle *st);
static guint sp_text_style_write (guchar *p, guint len, SPTextStyle *st);
static void sp_style_privatize_text (SPStyle *style);

static gint sp_style_property_index (const guchar *str);

static void sp_style_read_inherited_float (SPInheritedFloat *val, const guchar *str);
static void sp_style_read_inherited_scale30 (SPInheritedScale30 *val, const guchar *str);
static void sp_style_read_inherited_enum (SPInheritedShort *val, const guchar *str, const SPStyleEnum *dict, gboolean inherit);
static void sp_style_read_inherited_string (SPInheritedString *val, const guchar *str);
static void sp_style_read_inherited_paint (SPInheritedPaint *paint, const guchar *str, SPStyle *style, SPDocument *document);
static gint sp_style_write_inherited_float (guchar *p, gint len, const guchar *key, SPInheritedFloat *val);
static gint sp_style_write_inherited_scale30 (guchar *p, gint len, const guchar *key, SPInheritedScale30 *val);
static gint sp_style_write_inherited_enum (guchar *p, gint len, const guchar *key, const SPStyleEnum *dict, SPInheritedShort *val);
static gint sp_style_write_inherited_string (guchar *p, gint len, const guchar *key, SPInheritedString *val);
static gint sp_style_write_inherited_paint (guchar *b, gint len, const guchar *key, SPInheritedPaint *paint);

static SPColor *sp_style_read_color_cmyk (SPColor *color, const guchar *str);

struct _SPStyleEnum {
	const guchar *key;
	gint value;
};

static const SPStyleEnum enum_fill_rule[] = {
	{"nonzero", ART_WIND_RULE_NONZERO},
	{"evenodd", ART_WIND_RULE_ODDEVEN},
	{NULL, -1}
};

static const SPStyleEnum enum_stroke_linecap[] = {
	{"butt", ART_PATH_STROKE_CAP_BUTT},
	{"round", ART_PATH_STROKE_CAP_ROUND},
	{"square", ART_PATH_STROKE_CAP_SQUARE},
	{NULL, -1}
};

static const SPStyleEnum enum_stroke_linejoin[] = {
	{"miter", ART_PATH_STROKE_JOIN_MITER},
	{"round", ART_PATH_STROKE_JOIN_ROUND},
	{"bevel", ART_PATH_STROKE_JOIN_BEVEL},
	{NULL, -1}
};

static const SPStyleEnum enum_font_style[] = {
	{"normal", SP_CSS_FONT_STYLE_NORMAL},
	{"italic", SP_CSS_FONT_STYLE_ITALIC},
	{"oblique", SP_CSS_FONT_STYLE_OBLIQUE},
	{NULL, -1}
};

static const SPStyleEnum enum_font_variant[] = {
	{"normal", SP_CSS_FONT_VARIANT_NORMAL},
	{"small-caps", SP_CSS_FONT_VARIANT_SMALL_CAPS},
	{NULL, -1}
};

static const SPStyleEnum enum_font_weight[] = {
	{"100", SP_CSS_FONT_WEIGHT_100},
	{"200", SP_CSS_FONT_WEIGHT_200},
	{"300", SP_CSS_FONT_WEIGHT_300},
	{"400", SP_CSS_FONT_WEIGHT_400},
	{"500", SP_CSS_FONT_WEIGHT_500},
	{"600", SP_CSS_FONT_WEIGHT_600},
	{"700", SP_CSS_FONT_WEIGHT_700},
	{"800", SP_CSS_FONT_WEIGHT_800},
	{"900", SP_CSS_FONT_WEIGHT_900},
	{"normal", SP_CSS_FONT_WEIGHT_NORMAL},
	{"bold", SP_CSS_FONT_WEIGHT_BOLD},
	{"lighter", SP_CSS_FONT_WEIGHT_LIGHTER},
	{"darker", SP_CSS_FONT_WEIGHT_DARKER},
	{NULL, -1}
};

static const SPStyleEnum enum_font_stretch[] = {
	{"ultra-condensed", SP_CSS_FONT_STRETCH_ULTRA_CONDENSED},
	{"extra-condensed", SP_CSS_FONT_STRETCH_EXTRA_CONDENSED},
	{"condensed", SP_CSS_FONT_STRETCH_CONDENSED},
	{"semi-condensed", SP_CSS_FONT_STRETCH_SEMI_CONDENSED},
	{"normal", SP_CSS_FONT_STRETCH_NORMAL},
	{"semi-expanded", SP_CSS_FONT_STRETCH_SEMI_EXPANDED},
	{"expanded", SP_CSS_FONT_STRETCH_EXPANDED},
	{"extra-expanded", SP_CSS_FONT_STRETCH_EXTRA_EXPANDED},
	{"ultra-expanded", SP_CSS_FONT_STRETCH_ULTRA_EXPANDED},
	{"narrower", SP_CSS_FONT_STRETCH_NARROWER},
	{"wider", SP_CSS_FONT_STRETCH_WIDER},
	{NULL, -1}
};

static const SPStyleEnum enum_writing_mode[] = {
	{"lr", SP_CSS_WRITING_MODE_LR},
	{"rl", SP_CSS_WRITING_MODE_RL},
	{"tb", SP_CSS_WRITING_MODE_TB},
	{"lr-tb", SP_CSS_WRITING_MODE_LR},
	{"rl-tb", SP_CSS_WRITING_MODE_RL},
	{"tb-rl", SP_CSS_WRITING_MODE_TB},
	{NULL, -1}
};

static void
sp_style_object_destroyed (GtkObject *object, SPStyle *style)
{
	style->object = NULL;
}

SPStyle *
sp_style_new_from_object (SPObject *object)
{
	SPStyle *style;

	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);

	style = g_new0 (SPStyle, 1);

	style->refcount = 1;
	style->object = object;

	style->text = sp_text_style_new ();
	style->text_private = TRUE;

	gtk_signal_connect (GTK_OBJECT (object), "destroy", GTK_SIGNAL_FUNC (sp_style_object_destroyed), style);

	sp_style_clear (style);

	return style;
}

SPStyle *
sp_style_ref (SPStyle *style)
{
	g_return_val_if_fail (style != NULL, NULL);
	g_return_val_if_fail (style->refcount > 0, NULL);

	style->refcount += 1;

	return style;
}

SPStyle *
sp_style_unref (SPStyle *style)
{
	g_return_val_if_fail (style != NULL, NULL);
	g_return_val_if_fail (style->refcount > 0, NULL);

	style->refcount -= 1;

	if (style->refcount < 1) {
		if (style->object) gtk_signal_disconnect_by_data (GTK_OBJECT (style->object), style);
		if (style->text) sp_text_style_unref (style->text);
		if (style->fill.server) {
			sp_object_hunref (SP_OBJECT (style->fill.server), style);
			gtk_signal_disconnect_by_data (GTK_OBJECT (style->fill.server), style);
		}
		if (style->stroke.server) {
			sp_object_hunref (SP_OBJECT (style->stroke.server), style);
			gtk_signal_disconnect_by_data (GTK_OBJECT (style->stroke.server), style);
		}
		if (style->stroke_dash.dash) {
			g_free (style->stroke_dash.dash);
		}
		g_free (style);
	}

	return NULL;
}

void
sp_style_read_from_object (SPStyle *style, SPObject *object)
{
	const guchar *val;

	g_return_if_fail (style != NULL);
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));

	sp_style_clear (style);

	/* CMYK has precedence and can only be presentation attribute */
	val = sp_repr_attr (object->repr, "fill-cmyk");
	if (val && sp_style_read_color_cmyk (&style->fill.color, val)) {
		style->fill.set = TRUE;
		style->fill.inherit = FALSE;
	}
	val = sp_repr_attr (object->repr, "stroke-cmyk");
	if (val && sp_style_read_color_cmyk (&style->stroke.color, val)) {
		style->stroke.set = TRUE;
		style->stroke.inherit = FALSE;
	}

	val = sp_repr_attr (object->repr, "style");
	if (val != NULL) {
		sp_style_merge_from_style_string (style, val);
	}

	/* FIXME: CSS etc. parsing goes here */

	/* opacity */
	if (!style->opacity.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "opacity");
		if (val) {
			sp_style_read_inherited_scale30 (&style->opacity, val);
		}
	}
	/* fill */
	if (!style->fill.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "fill");
		if (val) {
			sp_style_read_inherited_paint (&style->fill, val, style, SP_OBJECT_DOCUMENT (object));
		}
	}
	/* fill-opacity */
	if (!style->fill_opacity.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "fill-opacity");
		if (val) {
			sp_style_read_inherited_scale30 (&style->fill_opacity, val);
		}
	}
	/* fill-rule */
	if (!style->fill_rule.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "fill-rule");
		if (val) {
			sp_style_read_inherited_enum (&style->fill_rule, val, enum_fill_rule, TRUE);
		}
	}
	/* stroke */
	if (!style->stroke.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "stroke");
		if (val) {
			sp_style_read_inherited_paint (&style->stroke, val, style, SP_OBJECT_DOCUMENT (object));
		}
	}
	/* stroke-linecap */
	if (!style->stroke_linecap.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "stroke-linecap");
		if (val) {
			sp_style_read_inherited_enum (&style->stroke_linecap, val, enum_stroke_linecap, TRUE);
		}
	}
	/* stroke-linejoin */
	if (!style->stroke_linejoin.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "stroke-linejoin");
		if (val) {
			sp_style_read_inherited_enum (&style->stroke_linejoin, val, enum_stroke_linejoin, TRUE);
		}
	}
	/* stroke-opacity */
	if (!style->stroke_opacity.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "stroke-opacity");
		if (val) {
			sp_style_read_inherited_scale30 (&style->stroke_opacity, val);
		}
	}

	/* font-family */
	if (!style->text_private || !style->text->font_family.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "font-family");
		if (val) {
			if (!style->text_private) sp_style_privatize_text (style);
			sp_style_read_inherited_string (&style->text->font_family, val);
		}
	}
	/* font-size */
	/* fixme: Really we should parse distances, percentages and so on (Lauris) */
	if (!style->text_private || !style->text->font_size.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "font-size");
		if (val) {
			if (!style->text_private) sp_style_privatize_text (style);
			sp_style_read_inherited_float (&style->text->font_size, val);
		}
	}
	/* font-style */
	if (!style->text_private || !style->text->writing_mode.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "font-style");
		if (val) {
			if (!style->text_private) sp_style_privatize_text (style);
			sp_style_read_inherited_enum (&style->text->font_style, val, enum_font_style, TRUE);
		}
	}
	/* font-variant */
	if (!style->text_private || !style->text->writing_mode.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "font-variant");
		if (val) {
			if (!style->text_private) sp_style_privatize_text (style);
			sp_style_read_inherited_enum (&style->text->font_style, val, enum_font_variant, TRUE);
		}
	}
	/* font-weight */
	if (!style->text_private || !style->text->writing_mode.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "font-weight");
		if (val) {
			if (!style->text_private) sp_style_privatize_text (style);
			sp_style_read_inherited_enum (&style->text->font_style, val, enum_font_weight, TRUE);
		}
	}
	/* font-stretch */
	if (!style->text_private || !style->text->writing_mode.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "font-stretch");
		if (val) {
			if (!style->text_private) sp_style_privatize_text (style);
			sp_style_read_inherited_enum (&style->text->font_style, val, enum_font_stretch, TRUE);
		}
	}
	/* writing-mode */
	if (!style->text_private || !style->text->writing_mode.set) {
		val = sp_repr_attr (SP_OBJECT_REPR (object), "writing-mode");
		if (val) {
			if (!style->text_private) sp_style_privatize_text (style);
			sp_style_read_inherited_enum (&style->text->writing_mode, val, enum_writing_mode, TRUE);
		}
	}

	if (object->parent) {
		sp_style_merge_from_object_parent (style, object->parent);
	}
}

static void
sp_style_privatize_text (SPStyle *style)
{
	SPTextStyle *text;

	text = style->text;
	style->text = sp_text_style_duplicate_unset (style->text);
	sp_text_style_unref (text);
	style->text_private = TRUE;
}

static void
sp_style_merge_property (SPStyle *style, gint id, const guchar *val)
{
	switch (id) {
	/* CSS2 */
	/* Font */
	case SP_PROP_FONT_FAMILY:
		if (!style->text_private) sp_style_privatize_text (style);
		if (!style->text->font_family.set) {
			sp_style_read_inherited_string (&style->text->font_family, val);
		}
		break;
	case SP_PROP_FONT_SIZE:
		if (!style->text_private) sp_style_privatize_text (style);
		if (!style->text->font_size.set) {
			sp_style_read_inherited_float (&style->text->font_size, val);
		}
		break;
	case SP_PROP_FONT_SIZE_ADJUST:
		g_warning ("Unimplemented style property id: %d value: %s", id, val);
		break;
	case SP_PROP_FONT_STYLE:
		if (!style->text_private) sp_style_privatize_text (style);
		if (!style->text->font_style.set) {
			sp_style_read_inherited_enum (&style->text->font_style, val, enum_font_style, TRUE);
		}
		break;
	case SP_PROP_FONT_VARIANT:
		if (!style->text_private) sp_style_privatize_text (style);
		if (!style->text->font_variant.set) {
			sp_style_read_inherited_enum (&style->text->font_variant, val, enum_font_variant, TRUE);
		}
		break;
	case SP_PROP_FONT_WEIGHT:
		if (!style->text_private) sp_style_privatize_text (style);
		if (!style->text->font_weight.set) {
			sp_style_read_inherited_enum (&style->text->font_weight, val, enum_font_weight, TRUE);
		}
		break;
	case SP_PROP_FONT_STRETCH:
		if (!style->text_private) sp_style_privatize_text (style);
		if (!style->text->font_stretch.set) {
			sp_style_read_inherited_enum (&style->text->font_stretch, val, enum_font_stretch, TRUE);
		}
		break;
	case SP_PROP_FONT:
		if (!style->text_private) sp_style_privatize_text (style);
		if (!style->text->font.set) {
			if (style->text->font.value) g_free (style->text->font.value);
			style->text->font.value = g_strdup (val);
			style->text->font.set = TRUE;
			style->text->font.inherit = (val && !strcmp (val, "inherit"));
		}
		break;
	/* Text */
	case SP_PROP_DIRECTION:
	case SP_PROP_LETTER_SPACING:
	case SP_PROP_TEXT_DECORATION:
	case SP_PROP_UNICODE_BIDI:
	case SP_PROP_WORD_SPACING:
	/* Misc */
	case SP_PROP_CLIP:
	case SP_PROP_COLOR:
	case SP_PROP_CURSOR:
		g_warning ("Unimplemented style property id: %d value: %s", id, val);
		break;
	case SP_PROP_DISPLAY:
		if (!style->display_set) {
			/* fixme: */
			style->display = strncmp (val, "none", 4);
			style->display_set = TRUE;
		}
		break;
	case SP_PROP_OVERFLOW:
		g_warning ("Unimplemented style property id: %d value: %s", id, val);
		break;
	case SP_PROP_VISIBILITY:
		if (!style->visibility_set) {
			/* fixme: */
			style->visibility = !strncmp (val, "visible", 7);
			style->visibility_set = TRUE;
		}
		break;
	/* SVG */
	/* Clip/Mask */
	case SP_PROP_CLIP_PATH:
	case SP_PROP_CLIP_RULE:
	case SP_PROP_MASK:
		g_warning ("Unimplemented style property id: %d value: %s", id, val);
		break;
	case SP_PROP_OPACITY:
		if (!style->opacity.set) {
			sp_style_read_inherited_scale30 (&style->opacity, val);
		}
		break;
	/* Filter */
	case SP_PROP_ENABLE_BACKGROUND:
	case SP_PROP_FILTER:
	case SP_PROP_FLOOD_COLOR:
	case SP_PROP_FLOOD_OPACITY:
	case SP_PROP_LIGHTING_COLOR:
	/* Gradient */
	case SP_PROP_STOP_COLOR:
	case SP_PROP_STOP_OPACITY:
	/* Interactivity */
	case SP_PROP_POINTER_EVENTS:
	/* Paint */
	case SP_PROP_COLOR_INTERPOLATION:
	case SP_PROP_COLOR_INTERPOLATION_FILTERS:
	case SP_PROP_COLOR_PROFILE:
	case SP_PROP_COLOR_RENDERING:
		g_warning ("Unimplemented style property id: %d value: %s", id, val);
		break;
	case SP_PROP_FILL:
		if (!style->fill.set) {
			sp_style_read_inherited_paint (&style->fill, val, style, SP_OBJECT_DOCUMENT (style->object));
		}
		break;
	case SP_PROP_FILL_OPACITY:
		if (!style->fill_opacity.set) {
			sp_style_read_inherited_scale30 (&style->fill_opacity, val);
		}
		break;
	case SP_PROP_FILL_RULE:
		if (!style->fill_rule.set) {
			sp_style_read_inherited_enum (&style->fill_rule, val, enum_fill_rule, TRUE);
		}
		break;
	case SP_PROP_IMAGE_RENDERING:
	case SP_PROP_MARKER:
	case SP_PROP_MARKER_END:
	case SP_PROP_MARKER_MID:
	case SP_PROP_MARKER_START:
	case SP_PROP_SHAPE_RENDERING:
		g_warning ("Unimplemented style property id: %d value: %s", id, val);
		break;
	case SP_PROP_STROKE:
		if (!style->stroke.set) {
			sp_style_read_inherited_paint (&style->stroke, val, style, SP_OBJECT_DOCUMENT (style->object));
		}
		break;
	case SP_PROP_STROKE_DASHARRAY:
		if (!style->stroke_dasharray_set) {
			sp_style_read_dash (&style->stroke_dash, val);
			style->stroke_dasharray_set = TRUE;
		}
		break;
	case SP_PROP_STROKE_DASHOFFSET:
		if (!style->stroke_dashoffset_set) {
			const SPUnit *unit;
			/* fixme */
			style->stroke_dash.offset = sp_svg_read_length (&unit, val, style->stroke_dash.offset);
			style->stroke_dashoffset_set = TRUE;
		}
		break;
	case SP_PROP_STROKE_LINECAP:
		if (!style->stroke_linecap.set) {
			sp_style_read_inherited_enum (&style->stroke_linecap, val, enum_stroke_linecap, TRUE);
		}
		break;
	case SP_PROP_STROKE_LINEJOIN:
		if (!style->stroke_linejoin.set) {
			sp_style_read_inherited_enum (&style->stroke_linejoin, val, enum_stroke_linejoin, TRUE);
		}
		break;
	case SP_PROP_STROKE_MITERLIMIT:
		if (!style->stroke_miterlimit_set) {
			const SPUnit *unit;
			/* fixme */
			style->stroke_miterlimit = sp_svg_read_length (&unit, val, style->stroke_miterlimit);
			style->stroke_miterlimit_set = TRUE;
		}
		break;
	case SP_PROP_STROKE_OPACITY:
		if (!style->stroke_opacity.set) {
			sp_style_read_inherited_scale30 (&style->stroke_opacity, val);
		}
		break;
	case SP_PROP_STROKE_WIDTH:
		if (!style->stroke_width_set) {
			style->stroke_width.distance = sp_svg_read_length (&style->stroke_width.unit, val, style->stroke_width.distance);
			style->stroke_width_set = TRUE;
			style->real_stroke_width_set = FALSE;
		}
		break;
	case SP_PROP_TEXT_RENDERING:
	/* Text */
	case SP_PROP_ALIGNMENT_BASELINE:
	case SP_PROP_BASELINE_SHIFT:
	case SP_PROP_DOMINANT_BASELINE:
	case SP_PROP_GLYPH_ORIENTATION_HORIZONTAL:
	case SP_PROP_GLYPH_ORIENTATION_VERTICAL:
	case SP_PROP_KERNING:
	case SP_PROP_TEXT_ANCHOR:
		g_warning ("Unimplemented style property id: %d value: %s", id, val);
		break;
	case SP_PROP_WRITING_MODE:
		if (!style->text_private) sp_style_privatize_text (style);
		if (!style->text->writing_mode.set) {
			sp_style_read_inherited_enum (&style->text->writing_mode, val, enum_writing_mode, TRUE);
		}
		break;
	default:
		g_warning ("Invalid style property id: %d value: %s", id, val);
		break;
	}
}

/*
 * Parses style="fill:red;fill-rule:evenodd;" type string
 */

static void
sp_style_merge_from_style_string (SPStyle *style, const guchar *p)
{
	guchar c[4096];

	while (*p) {
		const guchar *s, *e;
		gint len, idx;
		while (!isalpha (*p)) {
			if (!*p) return;
			p += 1;
		}
		s = strchr (p, ':');
		if (!s) {
			g_warning ("No separator at style at: %s", p);
			return;
		}
		e = strchr (p, ';');
		if (!e) {
			g_warning ("No end marker at style at: %s", p);
			e = p + strlen (p);
		}
		len = MIN (s - p, 4095);
		if (len < 1) {
			g_warning ("Zero length style property at: %s", p);
			return;
		}
		memcpy (c, p, len);
		c[len] = '\0';
		idx = sp_style_property_index (c);
		if (idx > 0) {
			len = MIN (e - s - 1, 4095);
			if (len > 0) memcpy (c, s + 1, len);
			c[len] = '\0';
			sp_style_merge_property (style, idx, c);
		} else {
			g_warning ("Unknown style property at: %s", p);
		}
		if (!*e) return;
		p = e + 1;
	}
}

void
sp_style_merge_from_object_parent (SPStyle *style, SPObject *object)
{
	g_return_if_fail (style != NULL);
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));

	if (object->style) {
		if (style->opacity.inherit) {
			style->opacity.value = object->style->opacity.value;
		}
		if (!style->display_set && object->style->display_set) {
			style->display = object->style->display;
			style->display_set = TRUE;
		}
		if (!style->visibility_set && object->style->visibility_set) {
			style->visibility = object->style->visibility;
			style->visibility_set = TRUE;
		}
		if (!style->fill.set || style->fill.inherit) {
			sp_style_merge_inherited_paint (style, &style->fill, &object->style->fill);
		}
		if (!style->fill_opacity.set || style->fill_opacity.inherit) {
			style->fill_opacity.value = object->style->fill_opacity.value;
		}
		if (!style->fill_rule.set || style->fill_rule.inherit) {
			style->fill_rule.value = object->style->fill_rule.value;
		}
		if (!style->stroke.set || style->stroke.inherit) {
			sp_style_merge_inherited_paint (style, &style->stroke, &object->style->stroke);
		}
		if (!style->stroke_width_set && object->style->stroke_width_set) {
			style->stroke_width = object->style->stroke_width;
			style->stroke_width_set = TRUE;
			style->real_stroke_width_set = FALSE;
		}
		if (!style->stroke_linecap.set || style->stroke_linecap.inherit) {
			style->stroke_linecap.value = object->style->stroke_linecap.value;
		}
		if (!style->stroke_linejoin.set && style->stroke_linejoin.inherit) {
			style->stroke_linejoin.value = object->style->stroke_linejoin.value;
		}
		if (!style->stroke_miterlimit_set && object->style->stroke_miterlimit_set) {
			style->stroke_miterlimit = object->style->stroke_miterlimit;
			style->stroke_miterlimit_set = TRUE;
		}
		if (!style->stroke_dasharray_set && object->style->stroke_dasharray_set) {
			style->stroke_dash.n_dash = object->style->stroke_dash.n_dash;
			if (style->stroke_dash.n_dash > 0) {
				style->stroke_dash.dash = g_new (gdouble, style->stroke_dash.n_dash);
				memcpy (style->stroke_dash.dash, object->style->stroke_dash.dash, style->stroke_dash.n_dash * sizeof (gdouble));
			}
			style->stroke_dasharray_set = TRUE;
		}
		if (!style->stroke_dashoffset_set && object->style->stroke_dashoffset_set) {
			style->stroke_dash.offset = object->style->stroke_dash.offset;
			style->stroke_dashoffset_set = TRUE;
		}
		if (!style->stroke_opacity.set || style->stroke_opacity.inherit) {
			style->stroke_opacity.value = object->style->stroke_opacity.value;
		}

		if (style->text && object->style->text) {
			if (!style->text->font_family.set || style->text->font_family.inherit) {
				if (style->text->font_family.value) g_free (style->text->font_family.value);
				style->text->font_family.value = g_strdup (object->style->text->font_family.value);
			}
			if (!style->text->font_size.set || style->text->font_size.inherit) {
				style->text->font_size.value = object->style->text->font_size.value;
			}
			if (!style->text->font_style.set || style->text->font_style.inherit) {
				style->text->font_style.value = object->style->text->font_style.value;
			}
			if (!style->text->font_variant.set || style->text->font_variant.inherit) {
				style->text->font_variant.value = object->style->text->font_variant.value;
			}
			if (!style->text->font_weight.set || style->text->font_weight.inherit) {
				style->text->font_weight.value = object->style->text->font_weight.value;
			}
			if (!style->text->font_stretch.set || style->text->font_stretch.inherit) {
				style->text->font_stretch.value = object->style->text->font_stretch.value;
			}
			if (!style->text->writing_mode.set || style->text->writing_mode.inherit) {
				style->text->writing_mode.value = object->style->text->writing_mode.value;
			}
		}
	}
}

static void
sp_style_paint_server_destroy (SPPaintServer *server, SPStyle *style)
{
	if (server == style->fill.server) {
		g_assert (style->fill.set);
		g_assert (style->fill.type == SP_PAINT_TYPE_PAINTSERVER);
		sp_object_hunref (SP_OBJECT (style->fill.server), style);
		style->fill.type = SP_PAINT_TYPE_NONE;
		style->fill.server = NULL;
	} else if (server == style->stroke.server) {
		g_assert (style->stroke.set);
		g_assert (style->stroke.type == SP_PAINT_TYPE_PAINTSERVER);
		sp_object_hunref (SP_OBJECT (style->stroke.server), style);
		style->stroke.type = SP_PAINT_TYPE_NONE;
		style->stroke.server = NULL;
	} else {
		g_assert_not_reached ();
	}
}

static void
sp_style_paint_server_modified (SPPaintServer *server, guint flags, SPStyle *style)
{
	if (server == style->fill.server) {
		g_assert (style->fill.set);
		g_assert (style->fill.type == SP_PAINT_TYPE_PAINTSERVER);
		/* fixme: I do not know, whether it is optimal - we are forcing reread of everything (Lauris) */
		/* fixme: We have to use object_modified flag, because parent flag is only available downstreams */
		sp_object_request_modified (style->object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
	} else if (server == style->stroke.server) {
		g_assert (style->stroke.set);
		g_assert (style->stroke.type == SP_PAINT_TYPE_PAINTSERVER);
		/* fixme: */
		sp_object_request_modified (style->object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
	} else {
		g_assert_not_reached ();
	}
}

static void
sp_style_merge_inherited_paint (SPStyle *style, SPInheritedPaint *paint, SPInheritedPaint *parent)
{
	if ((paint->type == SP_PAINT_TYPE_PAINTSERVER) && paint->server) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (paint->server), style);
		sp_object_hunref (SP_OBJECT (paint->server), style);
	}
	paint->type = parent->type;
	switch (paint->type) {
	case SP_PAINT_TYPE_COLOR:
		sp_color_copy (&paint->color, &parent->color);
		break;
	case SP_PAINT_TYPE_PAINTSERVER:
		paint->server = parent->server;
		if (paint->server) {
			sp_object_href (SP_OBJECT (paint->server), style);
			gtk_signal_connect (GTK_OBJECT (paint->server), "destroy",
					    GTK_SIGNAL_FUNC (sp_style_paint_server_destroy), style);
			gtk_signal_connect (GTK_OBJECT (paint->server), "modified",
					    GTK_SIGNAL_FUNC (sp_style_paint_server_modified), style);
		}
		break;
	case SP_PAINT_TYPE_NONE:
		break;
	default:
		g_assert_not_reached ();
		break;
	}
}

/* fixme: Write real thing */

guchar *
sp_style_write_string (SPStyle *style)
{
	guchar c[4096], *p;

	g_return_val_if_fail (style != NULL, NULL);

	p = c;
	*p = '\0';

	if (style->opacity.set && style->opacity.value != SP_SCALE30_MAX) {
		p += sp_style_write_inherited_scale30 (p, c + 4096 - p, "opacity", &style->opacity);
	}
	if (style->fill.set) {
		p += sp_style_write_inherited_paint (p, c + 4096 - p, "fill", &style->fill);
	}
	if (style->fill_opacity.set) {
		p += sp_style_write_inherited_scale30 (p, c + 4096 - p, "fill-opacity", &style->fill_opacity);
	}
	if (style->fill_rule.set) {
		p += sp_style_write_inherited_enum (p, c + 4096 - p, "fill-rule", enum_fill_rule, &style->fill_rule);
	}
	if (style->stroke.set) {
		p += sp_style_write_inherited_paint (p, c + 4096 - p, "stroke", &style->stroke);
	}
	if (style->stroke_width_set) {
		p += g_snprintf (p, c + 4096 - p, "stroke-width:");
		p += sp_svg_write_length (p, c + 4096 - p, style->stroke_width.distance, style->stroke_width.unit);
		p += g_snprintf (p, c + 4096 - p, ";");
	}
	if (style->stroke_linecap.set) {
		p += sp_style_write_inherited_enum (p, c + 4096 - p, "stroke-linecap", enum_stroke_linecap, &style->stroke_linecap);
	}
	if (style->stroke_linejoin.set) {
		p += sp_style_write_inherited_enum (p, c + 4096 - p, "stroke-linejoin", enum_stroke_linejoin, &style->stroke_linejoin);
	}
	if (style->stroke_miterlimit_set) {
		p += g_snprintf (p, c + 4096 - p, "stroke-miterlimit:%g;", style->stroke_miterlimit);
	}
	if (style->stroke_dasharray_set) {
		if (style->stroke_dash.n_dash && style->stroke_dash.dash) {
			gint i;
			p += g_snprintf (p, c + 4096 - p, "stroke-dasharray:");
			for (i = 0; i < style->stroke_dash.n_dash; i++) {
				p += g_snprintf (p, c + 4096 - p, "%g ", style->stroke_dash.dash[i]);
			}
			p += g_snprintf (p, c + 4096 - p, ";");
		}
	}
	if (style->stroke_dashoffset_set) {
		p += g_snprintf (p, c + 4096 - p, "stroke-dashoffset:%g;", style->stroke_dash.offset);
	}
	if (style->stroke_opacity.set) {
		p += sp_style_write_inherited_scale30 (p, c + 4096 - p, "stroke-opacity", &style->stroke_opacity);
	}

	sp_text_style_write (p, c + 4096 - p, style->text);

	return g_strdup (c);
}

static void
sp_style_clear (SPStyle *style)
{
	SPObject *object;
	gint refcount;
	SPTextStyle *text;
	gboolean text_private;

	g_return_if_fail (style != NULL);

	if (style->fill.server) {
		sp_object_hunref (SP_OBJECT (style->fill.server), style);
		gtk_signal_disconnect_by_data (GTK_OBJECT (style->fill.server), style);
	}
	if (style->stroke.server) {
		sp_object_hunref (SP_OBJECT (style->stroke.server), style);
		gtk_signal_disconnect_by_data (GTK_OBJECT (style->stroke.server), style);
	}
	if (style->stroke_dash.dash) {
		g_free (style->stroke_dash.dash);
	}

	/* fixme: Do that text manipulation via parents */
	object = style->object;
	refcount = style->refcount;
	text = style->text;
	text_private = style->text_private;
	memset (style, 0, sizeof (SPStyle));
	style->refcount = refcount;
	style->object = object;
	style->text = text;
	style->text_private = text_private;
	/* fixme: */
	style->text->font_family.set = FALSE;
	style->text->font_size.set = FALSE;
	style->text->font_style.set = FALSE;
	style->text->font_weight.set = FALSE;
	style->text->font.set = FALSE;

	style->opacity.value = SP_SCALE30_MAX;
	style->display = TRUE;
	style->visibility = TRUE;

	style->fill.type = SP_PAINT_TYPE_COLOR;
	sp_color_set_rgb_float (&style->fill.color, 0.0, 0.0, 0.0);
	style->fill.server = NULL;
	style->fill_opacity.value = SP_SCALE30_MAX;
	style->fill_rule.value = ART_WIND_RULE_NONZERO;

	style->stroke.type = SP_PAINT_TYPE_NONE;
	sp_color_set_rgb_float (&style->stroke.color, 0.0, 0.0, 0.0);
	style->stroke.server = NULL;
	style->stroke_width.unit = sp_unit_get_identity (SP_UNIT_USERSPACE);
	style->stroke_width.distance = 1.0;
	style->absolute_stroke_width = 1.0;
	style->user_stroke_width = 1.0;
	style->stroke_linecap.value = ART_PATH_STROKE_CAP_BUTT;
	style->stroke_linejoin.value = ART_PATH_STROKE_JOIN_MITER;
	style->stroke_miterlimit = 4.0;
	style->stroke_dash.n_dash = 0;
	style->stroke_dash.dash = NULL;
	style->stroke_dash.offset = 0.0;
	style->stroke_opacity.value = SP_SCALE30_MAX;
}

static void
sp_style_read_dash (ArtVpathDash *dash, const guchar *str)
{
	gint n_dash;
	gdouble d[64];
	guchar *e;

	n_dash = 0;
	e = NULL;

	while (e != str && n_dash < 64) {
		d[n_dash] = strtod (str, (char **) &e);
		if (e != str) {
			n_dash += 1;
			str = e;
		}
	}

	if (n_dash > 0) {
		dash->dash = g_new (gdouble, n_dash);
		memcpy (dash->dash, d, sizeof (gdouble) * n_dash);
		dash->n_dash = n_dash;
	}
}

void
sp_style_set_fill_color_rgba (SPStyle *style, gfloat r, gfloat g, gfloat b, gfloat a, gboolean fill_set, gboolean opacity_set)
{
	g_return_if_fail (style != NULL);

	if (style->fill.server) {
		sp_object_hunref (SP_OBJECT (style->fill.server), style);
		gtk_signal_disconnect_by_data (GTK_OBJECT (style->fill.server), style);
	}

	style->fill.set = fill_set;
	style->fill.inherit = FALSE;
	style->fill.type = SP_PAINT_TYPE_COLOR;
	sp_color_set_rgb_float (&style->fill.color, r, g, b);
	style->fill_opacity.set = opacity_set;
	style->fill_opacity.inherit = FALSE;
	style->fill_opacity.value = SP_SCALE30_FROM_FLOAT (a);

	sp_object_request_modified (style->object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
}

void
sp_style_set_fill_color_cmyka (SPStyle *style, gfloat c, gfloat m, gfloat y, gfloat k, gfloat a, gboolean fill_set, gboolean opacity_set)
{
	g_return_if_fail (style != NULL);

	if (style->fill.server) {
		sp_object_hunref (SP_OBJECT (style->fill.server), style);
		gtk_signal_disconnect_by_data (GTK_OBJECT (style->fill.server), style);
	}

	style->fill.set = fill_set;
	style->fill.inherit = FALSE;
	style->fill.type = SP_PAINT_TYPE_COLOR;
	sp_color_set_cmyk_float (&style->fill.color, c, m, y, k);
	style->fill_opacity.set = opacity_set;
	style->fill_opacity.inherit = FALSE;
	style->fill_opacity.value = SP_SCALE30_FROM_FLOAT (a);

	sp_object_request_modified (style->object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
}

void
sp_style_set_stroke_color_rgba (SPStyle *style, gfloat r, gfloat g, gfloat b, gfloat a, gboolean stroke_set, gboolean opacity_set)
{
	g_return_if_fail (style != NULL);

	if (style->stroke.server) {
		sp_object_hunref (SP_OBJECT (style->stroke.server), style);
		gtk_signal_disconnect_by_data (GTK_OBJECT (style->stroke.server), style);
	}

	style->stroke.set = stroke_set;
	style->stroke.inherit = FALSE;
	style->stroke.type = SP_PAINT_TYPE_COLOR;
	sp_color_set_rgb_float (&style->stroke.color, r, g, b);
	style->stroke_opacity.set = opacity_set;
	style->stroke_opacity.inherit = FALSE;
	style->stroke_opacity.value = SP_SCALE30_FROM_FLOAT (a);

	sp_object_request_modified (style->object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
}

void
sp_style_set_stroke_color_cmyka (SPStyle *style, gfloat c, gfloat m, gfloat y, gfloat k, gfloat a, gboolean stroke_set, gboolean opacity_set)
{
	g_return_if_fail (style != NULL);

	if (style->stroke.server) {
		sp_object_hunref (SP_OBJECT (style->stroke.server), style);
		gtk_signal_disconnect_by_data (GTK_OBJECT (style->stroke.server), style);
	}

	style->stroke.set = stroke_set;
	style->stroke.inherit = FALSE;
	style->stroke.type = SP_PAINT_TYPE_COLOR;
	sp_color_set_cmyk_float (&style->stroke.color, c, m, y, k);
	style->stroke_opacity.set = opacity_set;
	style->stroke_opacity.inherit = FALSE;
	style->stroke_opacity.value = SP_SCALE30_FROM_FLOAT (a);

	sp_object_request_modified (style->object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
}

void
sp_style_set_opacity (SPStyle *style, gfloat opacity, gboolean opacity_set)
{
	g_return_if_fail (style != NULL);

	style->opacity.set = opacity_set;
	style->opacity.inherit = FALSE;
	style->opacity.value = SP_SCALE30_FROM_FLOAT (opacity);

	sp_object_request_modified (style->object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
}

/* SPTextStyle operations */

static SPTextStyle *
sp_text_style_new (void)
{
	SPTextStyle *ts;

	ts = g_new0 (SPTextStyle, 1);

	ts->refcount = 1;

	sp_text_style_clear (ts);

	ts->font.value = g_strdup ("Bitstream Cyberbit 12");
	ts->font_family.value = g_strdup ("Bitstream Cyberbit");
	ts->font_size.value = 12.0;
	ts->font_style.value = SP_CSS_FONT_STYLE_NORMAL;
	ts->font_variant.value = SP_CSS_FONT_VARIANT_NORMAL;
	ts->font_weight.value = SP_CSS_FONT_WEIGHT_NORMAL;
	ts->font_stretch.value = SP_CSS_FONT_STRETCH_NORMAL;

	ts->writing_mode.value = SP_CSS_WRITING_MODE_LR;

	return ts;
}

static void
sp_text_style_clear (SPTextStyle *ts)
{
	ts->font.set = FALSE;
	ts->font_family.set = FALSE;
	ts->font_size.set = FALSE;
	ts->font_size_adjust_set = FALSE;
	ts->font_stretch.set = FALSE;
	ts->font_style.set = FALSE;
	ts->font_variant.set = FALSE;
	ts->font_weight.set = FALSE;

	ts->direction_set = FALSE;
	ts->letter_spacing_set = FALSE;
	ts->text_decoration_set = FALSE;
	ts->unicode_bidi_set = FALSE;
	ts->word_spacing_set = FALSE;

	ts->writing_mode.set = FALSE;
}

static SPTextStyle *
sp_text_style_ref (SPTextStyle *st)
{
	st->refcount += 1;

	return st;
}

static SPTextStyle *
sp_text_style_unref (SPTextStyle *st)
{
	st->refcount -= 1;

	if (st->refcount < 1) {
		if (st->font.value) g_free (st->font.value);
		if (st->font_family.value) g_free (st->font_family.value);
		if (st->face) gnome_font_face_unref (st->face);
		g_free (st);
	}

	return NULL;
}

static SPTextStyle *
sp_text_style_duplicate_unset (SPTextStyle *st)
{
	SPTextStyle *nt;

	nt = g_new0 (SPTextStyle, 1);

	nt->refcount = 1;

	nt->font.value = g_strdup (st->font.value);
	nt->font_family.value = g_strdup (st->font_family.value);

	nt->font_size.value = st->font_size.value;
	nt->font_style.value = st->font_style.value;
	nt->font_variant.value = st->font_variant.value;
	nt->font_weight.value = st->font_weight.value;
	nt->font_stretch.value = st->font_stretch.value;

	/* fixme: ??? */
	nt->writing_mode = st->writing_mode;

	return nt;
}

#if 0
static guint
sp_text_style_write_property (guchar *p, guint len, const guchar *key, const guchar *value)
{
	guint klen, vlen;

	klen = strlen (key);
	vlen = strlen (value);

	if ((klen + vlen + 2) <= len) {
		memcpy (p, key, klen);
		*(p + klen) = ':';
		memcpy (p + klen + 1, value, vlen);
		*(p + klen + vlen + 1) = ';';
		return klen + vlen + 2;
	}

	return 0;
}
#endif

static guint
sp_text_style_write (guchar *p, guint len, SPTextStyle *st)
{
	gint d;

	d = 0;

	if (st->font_family.set) {
		d += sp_style_write_inherited_string (p + d, len - d, "font-family", &st->font_family);
	}
	if (st->font_size.set) {
		d += sp_style_write_inherited_float (p + d, len - d, "font-size", &st->font_size);
	}
	if (st->font_style.set) {
		d += sp_style_write_inherited_enum (p + d, len - d, "font-style", enum_font_style, &st->font_style);
	}
	if (st->font_variant.set) {
		d += sp_style_write_inherited_enum (p + d, len - d, "font-variant", enum_font_variant, &st->font_variant);
	}
	if (st->font_weight.set) {
		d += sp_style_write_inherited_enum (p + d, len - d, "font-weight", enum_font_weight, &st->font_weight);
	}
	if (st->font_stretch.set) {
		d += sp_style_write_inherited_enum (p + d, len - d, "font-stretch", enum_font_stretch, &st->font_stretch);
	}

	if (st->writing_mode.set) {
		d += sp_style_write_inherited_enum (p + d, len - d, "writing-mode", enum_writing_mode, &st->writing_mode);
	}

	return d;
}

typedef struct {
	gint code;
	guchar *name;
} SPStyleProp;

static const SPStyleProp props[] = {
	/* CSS2 */
	/* Font */
	{SP_PROP_FONT, "font"},
	{SP_PROP_FONT_FAMILY, "font-family"},
	{SP_PROP_FONT_SIZE, "font-size"},
	{SP_PROP_FONT_SIZE_ADJUST, "font-size-adjust"},
	{SP_PROP_FONT_STRETCH, "font-stretch"},
	{SP_PROP_FONT_STYLE, "font-style"},
	{SP_PROP_FONT_VARIANT, "font-variant"},
	{SP_PROP_FONT_WEIGHT, "font-weight"},
	/* Text */
	{SP_PROP_DIRECTION, "direction"},
	{SP_PROP_LETTER_SPACING, "letter-spacing"},
	{SP_PROP_TEXT_DECORATION, "text-decoration"},
	{SP_PROP_UNICODE_BIDI, "unicode-bidi"},
	{SP_PROP_WORD_SPACING, "word-spacing"},
	/* Misc */
	{SP_PROP_CLIP, "clip"},
	{SP_PROP_COLOR, "color"},
	{SP_PROP_CURSOR, "cursor"},
	{SP_PROP_DISPLAY, "display"},
	{SP_PROP_OVERFLOW, "overflow"},
	{SP_PROP_VISIBILITY, "visibility"},
	/* SVG */
	/* Clip/Mask */
	{SP_PROP_CLIP_PATH, "clip-path"},
	{SP_PROP_CLIP_RULE, "clip-rule"},
	{SP_PROP_MASK, "mask"},
	{SP_PROP_OPACITY, "opacity"},
	/* Filter */
	{SP_PROP_ENABLE_BACKGROUND, "enable-background"},
	{SP_PROP_FILTER, "filter"},
	{SP_PROP_FLOOD_COLOR, "flood-color"},
	{SP_PROP_FLOOD_OPACITY, "flood-opacity"},
	{SP_PROP_LIGHTING_COLOR, "lighting-color"},
	/* Gradient */
	{SP_PROP_STOP_COLOR, "stop-color"},
	{SP_PROP_STOP_OPACITY, "stop-opacity"},
	/* Interactivity */
	{SP_PROP_POINTER_EVENTS, "pointer-events"},
	/* Paint */
	{SP_PROP_COLOR_INTERPOLATION, "color-interpolation"},
	{SP_PROP_COLOR_INTERPOLATION_FILTERS, "color-interpolation-filters"},
	{SP_PROP_COLOR_PROFILE, "color-profile"},
	{SP_PROP_COLOR_RENDERING, "color-rendering"},
	{SP_PROP_FILL, "fill"},
	{SP_PROP_FILL_OPACITY, "fill-opacity"},
	{SP_PROP_FILL_RULE, "fill-rule"},
	{SP_PROP_IMAGE_RENDERING, "image-rendering"},
	{SP_PROP_MARKER, "marker"},
	{SP_PROP_MARKER_END, "marker-end"},
	{SP_PROP_MARKER_MID, "marker-mid"},
	{SP_PROP_MARKER_START, "marker-start"},
	{SP_PROP_SHAPE_RENDERING, "shape-rendering"},
	{SP_PROP_STROKE, "stroke"},
	{SP_PROP_STROKE_DASHARRAY, "stroke-dasharray"},
	{SP_PROP_STROKE_DASHOFFSET, "stroke-dashoffset"},
	{SP_PROP_STROKE_LINECAP, "stroke-linecap"},
	{SP_PROP_STROKE_LINEJOIN, "stroke-linejoin"},
	{SP_PROP_STROKE_MITERLIMIT, "stroke-miterlimit"},
	{SP_PROP_STROKE_OPACITY, "stroke-opacity"},
	{SP_PROP_STROKE_WIDTH, "stroke-width"},
	{SP_PROP_TEXT_RENDERING, "text-rendering"},
	/* Text */
	{SP_PROP_ALIGNMENT_BASELINE, "alignment-baseline"},
	{SP_PROP_BASELINE_SHIFT, "baseline-shift"},
	{SP_PROP_DOMINANT_BASELINE, "dominant-baseline"},
	{SP_PROP_GLYPH_ORIENTATION_HORIZONTAL, "glyph-orientation-horizontal"},
	{SP_PROP_GLYPH_ORIENTATION_VERTICAL, "glyph-orientation-vertical"},
	{SP_PROP_KERNING, "kerning"},
	{SP_PROP_TEXT_ANCHOR, "text-anchor"},
	{SP_PROP_WRITING_MODE, "writing-mode"},
	{SP_PROP_INVALID, NULL}
};

static gint
sp_style_property_index (const guchar *str)
{
	static GHashTable *propdict = NULL;

	if (!propdict) {
		const SPStyleProp *prop;
		propdict = g_hash_table_new (g_str_hash, g_str_equal);
		for (prop = props; prop->code != SP_PROP_INVALID; prop++) {
			g_hash_table_insert (propdict, prop->name, GINT_TO_POINTER (prop->code));
		}
	}

	return GPOINTER_TO_INT (g_hash_table_lookup (propdict, str));
}

static void
sp_style_read_inherited_float (SPInheritedFloat *val, const guchar *str)
{
	if (!strcmp (str, "inherit")) {
		val->set = TRUE;
		val->inherit = TRUE;
	} else {
		gfloat value;
		if (sp_svg_read_number_f (str, &value)) {
			val->set = TRUE;
			val->inherit = FALSE;
			val->value = value;
		}
	}
}

static void
sp_style_read_inherited_scale30 (SPInheritedScale30 *val, const guchar *str)
{
	if (!strcmp (str, "inherit")) {
		val->set = TRUE;
		val->inherit = TRUE;
	} else {
		gfloat value;
		if (sp_svg_read_number_f (str, &value)) {
			val->set = TRUE;
			val->inherit = FALSE;
			val->value = SP_SCALE30_FROM_FLOAT (value);
		}
	}
}

static void
sp_style_read_inherited_enum (SPInheritedShort *val, const guchar *str, const SPStyleEnum *dict, gboolean inherit)
{
	if (inherit && !strcmp (str, "inherit")) {
		val->set = TRUE;
		val->inherit = TRUE;
	} else {
		gint i;
		for (i = 0; dict[i].key; i++) {
			if (!strcmp (str, dict[i].key)) {
				val->set = TRUE;
				val->inherit = FALSE;
				val->value = dict[i].value;
				break;
			}
		}
	}
}

static void
sp_style_read_inherited_string (SPInheritedString *val, const guchar *str)
{
	if (val->value) g_free (val->value);

	if (!strcmp (str, "inherit")) {
		val->set = TRUE;
		val->inherit = TRUE;
		val->value = NULL;
	} else {
		val->set = TRUE;
		val->inherit = FALSE;
		val->value = g_strdup (str);
	}
}

static void
sp_style_read_inherited_paint (SPInheritedPaint *paint, const guchar *str, SPStyle *style, SPDocument *document)
{
	if (!strcmp (str, "inherit")) {
		paint->set = TRUE;
		paint->inherit = TRUE;
	} else {
		guint32 color;
		if (!strncmp (str, "url", 3)) {
			SPObject *ps;
			ps = sp_uri_reference_resolve (document, str);
			if (!ps || !SP_IS_PAINT_SERVER (ps)) {
				paint->type = SP_PAINT_TYPE_NONE;
				return;
			}
			paint->type = SP_PAINT_TYPE_PAINTSERVER;
			paint->server = SP_PAINT_SERVER (ps);
			sp_object_href (SP_OBJECT (paint->server), style);
			gtk_signal_connect (GTK_OBJECT (paint->server), "destroy",
					    GTK_SIGNAL_FUNC (sp_style_paint_server_destroy), style);
			gtk_signal_connect (GTK_OBJECT (paint->server), "modified",
					    GTK_SIGNAL_FUNC (sp_style_paint_server_modified), style);
			paint->set = TRUE;
			paint->inherit = FALSE;
			return;
		} else if (!strncmp (str, "none", 4)) {
			paint->type = SP_PAINT_TYPE_NONE;
			paint->set = TRUE;
			paint->inherit = FALSE;
			return;
		}

		paint->type = SP_PAINT_TYPE_COLOR;
		color = sp_color_get_rgba32_ualpha (&paint->color, 0);
		color = sp_svg_read_color (str, color);
		sp_color_set_rgb_rgba32 (&paint->color, color);
		paint->set = TRUE;
		paint->inherit = FALSE;
	}
}

static gint
sp_style_write_inherited_float (guchar *p, gint len, const guchar *key, SPInheritedFloat *val)
{
	if (val->inherit) {
		return g_snprintf (p, len, "%s:inherit;", key);
	} else {
		return g_snprintf (p, len, "%s:%g;", key, val->value);
	}
}

static gint
sp_style_write_inherited_scale30 (guchar *p, gint len, const guchar *key, SPInheritedScale30 *val)
{
	if (val->inherit) {
		return g_snprintf (p, len, "%s:inherit;", key);
	} else {
		return g_snprintf (p, len, "%s:%g;", key, SP_SCALE30_TO_FLOAT (val->value));
	}
}

static gint
sp_style_write_inherited_enum (guchar *p, gint len, const guchar *key, const SPStyleEnum *dict, SPInheritedShort *val)
{
	gint i;

	for (i = 0; dict[i].key; i++) {
		if (dict[i].value == val->value) {
			return g_snprintf (p, len, "%s:%s;", key, dict[i].key);
		}
	}

	return 0;
}

static gint
sp_style_write_inherited_string (guchar *p, gint len, const guchar *key, SPInheritedString *val)
{
	if (val->inherit) {
		return g_snprintf (p, len, "%s:inherit;", key);
	} else {
		return g_snprintf (p, len, "%s:%s;", key, val->value);
	}
}

static gint
sp_style_write_inherited_paint (guchar *p, gint len, const guchar *key, SPInheritedPaint *paint)
{
	if (paint->inherit) {
		return g_snprintf (p, len, "%s:inherit;", key);
	} else {
		switch (paint->type) {
		case SP_PAINT_TYPE_COLOR:
			return g_snprintf (p, len, "%s:#%06x;", key, sp_color_get_rgba32_falpha (&paint->color, 0.0) >> 8);
			break;
		case SP_PAINT_TYPE_PAINTSERVER:
			if (paint->server) {
				return g_snprintf (p, len, "%s:url(#%s);", key, SP_OBJECT (paint->server)->id);
			}
			break;
		default:
			break;
		}

		return g_snprintf (p, len, "%s:none;", key);
	}
}

/*
 * (C M Y K) tetraplet
 */

static SPColor *
sp_style_read_color_cmyk (SPColor *color, const guchar *str)
{
	gdouble c, m, y, k;
	gchar *cptr, *eptr;

	g_return_val_if_fail (str != NULL, NULL);

	c = m = y = k = 0.0;
	cptr = (gchar *) str + 1;
	c = strtod (cptr, &eptr);
	if (eptr && (eptr != cptr)) {
		cptr = eptr;
		m = strtod (cptr, &eptr);
		if (eptr && (eptr != cptr)) {
			cptr = eptr;
			y = strtod (cptr, &eptr);
			if (eptr && (eptr != cptr)) {
				cptr = eptr;
				k = strtod (cptr, &eptr);
				if (eptr && (eptr != cptr)) {
					sp_color_set_cmyk_float (color, c, m, y, k);
					return color;
				}
			}
		}
	}

	return NULL;
}
