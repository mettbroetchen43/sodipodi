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
#include "svg/svg.h"
#include "document.h"
#include "style.h"

static void sp_style_init (SPStyle *style);
static void sp_style_read_paint (SPPaint *paint, const guchar *str, SPDocument *document);
static void sp_style_read_dash (ArtVpathDash *dash, const guchar *str);
static const guchar *sp_style_str_value (const guchar *str, const guchar *key);

SPStyle *
sp_style_new (void)
{
	SPStyle *style;

	style = g_new0 (SPStyle, 1);

	style->refcount = 1;

	sp_style_init (style);

	return style;
}

void
sp_style_ref (SPStyle *style)
{
	g_return_if_fail (style != NULL);

	style->refcount += 1;
}

void
sp_style_unref (SPStyle *style)
{
	g_return_if_fail (style != NULL);

	if (--style->refcount < 1) {
		if (style->fill.server) {
			gtk_object_unref (GTK_OBJECT (style->fill.server));
		}
		if (style->stroke.server) {
			gtk_object_unref (GTK_OBJECT (style->stroke.server));
		}
		if (style->stroke_dash.dash) {
			g_free (style->stroke_dash.dash);
		}
		g_free (style);
	}
}

void
sp_style_read_from_string (SPStyle *style, const guchar *str, SPDocument *document)
{
	g_return_if_fail (style != NULL);
	g_return_if_fail (document != NULL);
	g_return_if_fail (SP_IS_DOCUMENT (document));

	sp_style_init (style);

	if (str) {
		const guchar *val;
		val = sp_style_str_value (str, "opacity");
		if (val) {
			style->opacity = sp_svg_read_percentage (val, style->opacity);
			style->opacity_set = TRUE;
			style->real_opacity_set = FALSE;
		}
		val = sp_style_str_value (str, "display");
		if (val) {
			/* fixme: */
			style->display = strncmp (val, "none", 4);
			style->display_set = TRUE;
		}
		val = sp_style_str_value (str, "visibility");
		if (val) {
			/* fixme: */
			style->visibility = !strncmp (val, "visible", 7);
			style->visibility_set = TRUE;
		}
		val = sp_style_str_value (str, "fill");
		if (val) {
			sp_style_read_paint (&style->fill, val, document);
			style->fill_set = TRUE;
		}
		val = sp_style_str_value (str, "fill-rule");
		if (val) {
			/* fixme: */
			style->fill_rule = strncmp (val, "nonzero", 7);
			style->fill_rule_set = TRUE;
		}
		val = sp_style_str_value (str, "fill-opacity");
		if (val) {
			style->fill_opacity = sp_svg_read_percentage (val, style->fill_opacity);
			style->fill_opacity_set = TRUE;
		}
		val = sp_style_str_value (str, "stroke");
		if (val) {
			sp_style_read_paint (&style->stroke, val, document);
			style->stroke_set = TRUE;
		}
		val = sp_style_str_value (str, "stroke-width");
		if (val) {
			style->stroke_width.distance = sp_svg_read_length (&style->stroke_width.unit, val, style->stroke_width.distance);
			style->stroke_width_set = TRUE;
			style->real_stroke_width_set = FALSE;
		}
		val = sp_style_str_value (str, "stroke-linecap");
		if (val) {
			/* fixme: */
			if (!strncmp (val, "butt", 4)) {
				style->stroke_linecap = ART_PATH_STROKE_CAP_BUTT;
			} else if (!strncmp (val, "round", 5)) {
				style->stroke_linecap = ART_PATH_STROKE_CAP_ROUND;
			} else {
				style->stroke_linecap = ART_PATH_STROKE_CAP_SQUARE;
			}
			style->stroke_linecap_set = TRUE;
		}
		val = sp_style_str_value (str, "stroke-linejoin");
		if (val) {
			/* fixme: */
			if (!strncmp (val, "miter", 5)) {
				style->stroke_linejoin = ART_PATH_STROKE_JOIN_MITER;
			} else if (!strncmp (val, "round", 5)) {
				style->stroke_linejoin = ART_PATH_STROKE_JOIN_ROUND;
			} else {
				style->stroke_linejoin = ART_PATH_STROKE_JOIN_BEVEL;
			}
			style->stroke_linejoin_set = TRUE;
		}
		val = sp_style_str_value (str, "stroke-miterlimit");
		if (val) {
			SPSVGUnit unit;
			/* fixme */
			style->stroke_miterlimit = sp_svg_read_length (&unit, val, style->stroke_miterlimit);
			style->stroke_miterlimit_set = TRUE;
		}
		val = sp_style_str_value (str, "stroke-dasharray");
		if (val) {
			sp_style_read_dash (&style->stroke_dash, val);
			style->stroke_dasharray_set = TRUE;
		}
		val = sp_style_str_value (str, "stroke-dashoffset");
		if (val) {
			SPSVGUnit unit;
			/* fixme */
			style->stroke_dash.offset = sp_svg_read_length (&unit, val, style->stroke_dash.offset);
			style->stroke_dashoffset_set = TRUE;
		}
		val = sp_style_str_value (str, "stroke-opacity");
		if (val) {
			style->stroke_opacity = sp_svg_read_percentage (val, style->stroke_opacity);
			style->stroke_opacity_set = TRUE;
		}
	}
}

void
sp_style_read_from_object (SPStyle *style, SPObject *object)
{
	const guchar *str;

	g_return_if_fail (style != NULL);
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));

	sp_style_init (style);

	str = sp_repr_attr (object->repr, "style");

	sp_style_read_from_string (style, str, object->document);

	if (object->parent) {
		sp_style_merge_from_object (style, object->parent);
	}
}

void
sp_style_merge_from_object (SPStyle *style, SPObject *object)
{
	g_return_if_fail (style != NULL);
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));

	if (object->style) {
		if (!style->opacity_set && object->style->opacity_set) {
			style->opacity = object->style->opacity;
			style->opacity_set = TRUE;
			style->real_opacity_set = FALSE;
		}
		if (!style->display_set && object->style->display_set) {
			style->display = object->style->display;
			style->display_set = TRUE;
		}
		if (!style->visibility_set && object->style->visibility_set) {
			style->visibility = object->style->visibility;
			style->visibility_set = TRUE;
		}
		if (!style->fill_set && object->style->fill_set) {
			style->fill.type = object->style->fill.type;
			style->fill_set = TRUE;
			if (style->fill.type == SP_PAINT_TYPE_COLOR) {
				style->fill.color = object->style->fill.color;
			} else if (style->fill.type == SP_PAINT_TYPE_PAINTSERVER) {
				style->fill.server = object->style->fill.server;
				if (style->fill.server) gtk_object_ref (GTK_OBJECT (style->fill.server));
			}
		}
		if (!style->fill_rule_set && object->style->fill_rule_set) {
			style->fill_rule = object->style->fill_rule;
			style->fill_rule_set = TRUE;
		}
		if (!style->fill_opacity_set && object->style->fill_opacity_set) {
			style->fill_opacity = object->style->fill_opacity;
			style->fill_opacity_set = TRUE;
		}
		if (!style->stroke_set && object->style->stroke_set) {
			style->stroke.type = object->style->stroke.type;
			style->stroke_set = TRUE;
			if (style->stroke.type == SP_PAINT_TYPE_COLOR) {
				style->stroke.color = object->style->stroke.color;
			} else if (style->stroke.type == SP_PAINT_TYPE_PAINTSERVER) {
				style->stroke.server = object->style->stroke.server;
				if (style->stroke.server) gtk_object_ref (GTK_OBJECT (style->stroke.server));
			}
		}
		if (!style->stroke_width_set && object->style->stroke_width_set) {
			style->stroke_width = object->style->stroke_width;
			style->stroke_width_set = TRUE;
			style->real_stroke_width_set = FALSE;
		}
		if (!style->stroke_linecap_set && object->style->stroke_linecap_set) {
			style->stroke_linecap = object->style->stroke_linecap;
			style->stroke_linecap_set = TRUE;
		}
		if (!style->stroke_linejoin_set && object->style->stroke_linejoin_set) {
			style->stroke_linejoin = object->style->stroke_linejoin;
			style->stroke_linejoin_set = TRUE;
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
		if (!style->stroke_opacity_set && object->style->stroke_opacity_set) {
			style->stroke_opacity = object->style->stroke_opacity;
			style->stroke_opacity_set = TRUE;
		}
	}

	if (object->parent) {
		sp_style_merge_from_object (style, object->parent);
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

	if (style->opacity_set && style->opacity != 1.0) {
		p += g_snprintf (p, c + 4096 - p, "opacity:%g;", style->opacity);
	}
	if (style->fill_set) {
		if (style->fill.type == SP_PAINT_TYPE_COLOR) {
			p += g_snprintf (p, c + 4096 - p, "fill:");
			p += sp_svg_write_color (p, c + 4096 - p, SP_RGBA_FROM_COLOR (&style->fill.color,1.0));
			p += g_snprintf (p, c + 4096 - p, ";");
		}
	}
	if (style->fill_opacity_set) {
		p += g_snprintf (p, c + 4096 - p, "fill-opacity:%g;", style->fill_opacity);
	}
	if (style->stroke_set) {
		if (style->stroke.type == SP_PAINT_TYPE_COLOR) {
			p += g_snprintf (p, c + 4096 - p, "stroke:");
			p += sp_svg_write_color (p, c + 4096 - p, SP_RGBA_FROM_COLOR (&style->stroke.color, 1.0));
			p += g_snprintf (p, c + 4096 - p, ";");
		}
	}
	if (style->stroke_opacity_set) {
		p += g_snprintf (p, c + 4096 - p, "stroke-opacity:%g;", style->stroke_opacity);
	}

	return g_strdup (c);
}

static void
sp_style_init (SPStyle *style)
{
	gint refcount;

	g_return_if_fail (style != NULL);

	if (style->fill.server) {
		gtk_object_unref (GTK_OBJECT (style->fill.server));
	}
	if (style->stroke.server) {
		gtk_object_unref (GTK_OBJECT (style->stroke.server));
	}
	if (style->stroke_dash.dash) {
		g_free (style->stroke_dash.dash);
	}

	refcount = style->refcount;
	memset (style, 0, sizeof (SPStyle));
	style->refcount = refcount;

	style->opacity = 1.0;
	style->real_opacity = 1.0;
	style->display = TRUE;
	style->visibility = TRUE;
	style->fill.type = SP_PAINT_TYPE_COLOR;
	style->fill.color.r = 0.0;
	style->fill.color.g = 0.0;
	style->fill.color.b = 0.0;
	style->fill.server = NULL;
	style->fill_rule = TRUE;
	style->fill_opacity = 1.0;
	style->stroke.type = SP_PAINT_TYPE_NONE;
	style->stroke.color.r = 0.0;
	style->stroke.color.g = 0.0;
	style->stroke.color.b = 0.0;
	style->stroke.server = NULL;
	style->stroke_width.unit = SP_UNIT_USER;
	style->stroke_width.distance = 1.0;
	style->absolute_stroke_width = 1.0;
	style->user_stroke_width = 1.0;
	style->stroke_linecap = ART_PATH_STROKE_CAP_BUTT;
	style->stroke_linejoin = ART_PATH_STROKE_JOIN_MITER;
	style->stroke_miterlimit = 4.0;
	style->stroke_dash.n_dash = 0;
	style->stroke_dash.dash = NULL;
	style->stroke_dash.offset = 0.0;
	style->stroke_opacity = 1.0;
}

static void
sp_style_read_paint (SPPaint *paint, const guchar *str, SPDocument *document)
{
	guint32 color;

	if (!strncmp (str, "url", 3)) {
		const guchar *e;
		guchar *id;
		gint len;
		SPObject *ps;
		/* fixme: */
		while (*str != '#') {
			if (!*str || *str == ';') {
				paint->type = SP_PAINT_TYPE_NONE;
				return;
			}
			str++;
		}
		while (!isalpha (*str)) {
			if (!*str || *str == ';') {
				paint->type = SP_PAINT_TYPE_NONE;
				return;
			}
			str++;
		}
		for (e = str; *e != ')'; e++) {
			if (!*e || *e == ';') {
				paint->type = SP_PAINT_TYPE_NONE;
				return;
			}
		}
		len = e - str;
		id = g_new (guchar, len + 1);
		memcpy (id, str, len);
		id[len] = '\0';
		ps = sp_document_lookup_id (document, id);
		g_free (id);
		if (!ps || !SP_IS_PAINT_SERVER (ps)) {
			paint->type = SP_PAINT_TYPE_NONE;
			return;
		}
		paint->type = SP_PAINT_TYPE_PAINTSERVER;
		paint->server = SP_PAINT_SERVER (ps);
		gtk_object_ref (GTK_OBJECT (ps));
		return;
	} else if (!strncmp (str, "none", 4)) {
		paint->type = SP_PAINT_TYPE_NONE;
		return;
	}

	paint->type = SP_PAINT_TYPE_COLOR;
	color = ((guint) (paint->color.r * 255) << 24) | ((guint) (paint->color.g * 255) << 16) | ((guint) (paint->color.b * 255));
	color = sp_svg_read_color (str, color);
	paint->color.r = (color >> 24) / 255.0;
	paint->color.g = ((color >> 16) & 0xff) / 255.0;
	paint->color.b = ((color >> 8) & 0xff) / 255.0;
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

static const guchar *
sp_style_str_value (const guchar *str, const guchar *key)
{
	gint len;

	len = strlen (key);

	while (*str) {
		while (!isalpha (*str)) {
			if (!*str) return NULL;
			str += 1;
		}
		if (!strncmp (str, key, len) && str[len] == ':') {
			str += len + 1;
			while (isspace (*str)) {
				if (!*str) return NULL;
				str += 1;
			}
			if (*str == ';') return NULL;
			return str;
		}
		while (*str != ';') {
			if (!*str) return NULL;
			str += 1;
		}
	}

	return NULL;
}

