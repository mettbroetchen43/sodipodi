#define __SP_TEXT_C__

/*
 * SPText - a SVG <text> element
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <gal/unicode/gunicode.h>
#include "svg/svg.h"
#include "style.h"
#include "sp-text.h"

static void sp_text_class_init (SPTextClass *class);
static void sp_text_init (SPText *text);
static void sp_text_destroy (GtkObject *object);

static void sp_text_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_text_read_attr (SPObject * object, const gchar * attr);
static void sp_text_read_content (SPObject * object);
static void sp_text_style_modified (SPObject *object, guint flags);

static char * sp_text_description (SPItem * item);
static GSList * sp_text_snappoints (SPItem * item, GSList * points);
static void sp_text_write_transform (SPItem *item, SPRepr *repr, gdouble *transform);

static void sp_text_set_shape (SPText * text);

static SPCharsClass *parent_class;

GtkType
sp_text_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPText",
			sizeof (SPText),
			sizeof (SPTextClass),
			(GtkClassInitFunc) sp_text_class_init,
			(GtkObjectInitFunc) sp_text_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_CHARS, &info);
	}
	return type;
}

static void
sp_text_class_init (SPTextClass *class)
{
	GtkObjectClass *object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_chars_get_type ());

	object_class->destroy = sp_text_destroy;

	sp_object_class->build = sp_text_build;
	sp_object_class->read_attr = sp_text_read_attr;
	sp_object_class->read_content = sp_text_read_content;
	sp_object_class->style_modified = sp_text_style_modified;

	item_class->description = sp_text_description;
	item_class->snappoints = sp_text_snappoints;
	item_class->write_transform = sp_text_write_transform;
}

static void
sp_text_init (SPText *text)
{
	text->x = text->y = 0.0;
	text->text = NULL;

#if 0
	text->fontname = g_strdup ("Helvetica");
	text->weight = GNOME_FONT_BOOK;
	text->italic = FALSE;
	text->size = 12.0;
#endif
}

static void
sp_text_destroy (GtkObject *object)
{
	SPText *text;

	text = SP_TEXT (object);

	if (text->text) {
		g_free (text->text);
		text->text = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_text_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	if (SP_OBJECT_CLASS (parent_class)->build)
		(SP_OBJECT_CLASS (parent_class)->build) (object, document, repr);

	sp_text_read_attr (object, "x");
	sp_text_read_attr (object, "y");
	sp_text_read_content (object);
}

static void
sp_text_read_attr (SPObject * object, const gchar * attr)
{
	SPText *text;
	const guchar *astr;
	const SPUnit *unit;

	text = SP_TEXT (object);

	if (strcmp (attr, "x") == 0) {
		astr = sp_repr_attr (SP_OBJECT_REPR (object), attr);
		text->x = sp_svg_read_length (&unit, astr, 0.0);
		sp_text_set_shape (text);
		return;
	}
	if (strcmp (attr, "y") == 0) {
		astr = sp_repr_attr (SP_OBJECT_REPR (object), attr);
		text->y = sp_svg_read_length (&unit, astr, 0.0);
		sp_text_set_shape (text);
		return;
	}

	if (SP_OBJECT_CLASS (parent_class)->read_attr)
		(SP_OBJECT_CLASS (parent_class)->read_attr) (object, attr);
}

static void
sp_text_read_content (SPObject * object)
{
	SPText * text;
	const gchar * t;

	text = SP_TEXT (object);

	if (text->text) g_free (text->text);

	t = sp_repr_content (object->repr);
	if (t != NULL) {
		text->text = g_strdup (t);
	} else {
		text->text = NULL;
	}
	sp_text_set_shape (text);
	sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
}

static void
sp_text_style_modified (SPObject *object, guint flags)
{
	SPText *text;

	text = SP_TEXT (object);

	/* Item class reads style */
	if (((SPObjectClass *) (parent_class))->style_modified)
		(* ((SPObjectClass *) (parent_class))->style_modified) (object, flags);

	sp_text_set_shape (text);
}

static char *
sp_text_description (SPItem * item)
{
	SPText *text;

	text = (SPText *) item;

	if (text->text) {
		return g_strdup (text->text);
	}

	return g_strdup (_("Text object"));
}

static gint
sp_text_font_weight_to_gp (SPCSSFontWeight weight)
{
	switch (weight) {
	case SP_CSS_FONT_WEIGHT_100:
		return GNOME_FONT_EXTRA_LIGHT;
		break;
	case SP_CSS_FONT_WEIGHT_200:
		return GNOME_FONT_THIN;
		break;
	case SP_CSS_FONT_WEIGHT_300:
		return GNOME_FONT_LIGHT;
		break;
	case SP_CSS_FONT_WEIGHT_400:
	case SP_CSS_FONT_WEIGHT_NORMAL:
		return GNOME_FONT_BOOK;
		break;
	case SP_CSS_FONT_WEIGHT_500:
		return GNOME_FONT_MEDIUM;
		break;
	case SP_CSS_FONT_WEIGHT_600:
		return GNOME_FONT_SEMI;
		break;
	case SP_CSS_FONT_WEIGHT_700:
	case SP_CSS_FONT_WEIGHT_BOLD:
		return GNOME_FONT_BOLD;
		break;
	case SP_CSS_FONT_WEIGHT_800:
		return GNOME_FONT_HEAVY;
		break;
	case SP_CSS_FONT_WEIGHT_900:
		return GNOME_FONT_BLACK;
		break;
	default:
		return GNOME_FONT_BOOK;
		break;
	}

	/* fixme: case SP_CSS_FONT_WEIGHT_LIGHTER: */
	/* fixme: case SP_CSS_FONT_WEIGHT_DARKER: */

	return GNOME_FONT_BOOK;
}

static gboolean
sp_text_font_italic_to_gp (SPCSSFontStyle style)
{
	if (style == SP_CSS_FONT_STYLE_NORMAL) return FALSE;

	return TRUE;
}

static void
sp_text_set_shape (SPText *text)
{
	SPChars *chars;
	SPStyle *style;
	const GnomeFontFace *face;
	gdouble size;
	guint glyph;
	gdouble x, y;
	gdouble a[6];
	gdouble w;
	const guchar *p;

	chars = SP_CHARS (text);
	style = SP_OBJECT_STYLE (text);

	sp_chars_clear (chars);

	face = gnome_font_unsized_closest (style->text->font_family.value,
					   sp_text_font_weight_to_gp (style->text->font_weight),
					   sp_text_font_italic_to_gp (style->text->font_style));
	size = style->text->font_size;

	x = text->x;
	y = text->y;

	art_affine_scale (a, size * 0.001, size * -0.001);
	if (text->text) {
		for (p = text->text; p && *p; p = g_utf8_next_char (p)) {
			gunichar u;
			u = g_utf8_get_char (p);
			if (u == '\n') {
				if (style->text->writing_mode.value == SP_CSS_WRITING_MODE_TB) {
					x -= size;
					y = text->y;
				} else {
					x = text->x;
					y += size;
				}
			} else {
				glyph = gnome_font_face_lookup_default (face, u);

				if (style->text->writing_mode.value == SP_CSS_WRITING_MODE_TB) {
					a[4] = x;
					a[5] = y;
					sp_chars_add_element (chars, glyph, (GnomeFontFace *) face, a);
					y += size;
				} else {
					w = gnome_font_face_get_glyph_width (face, glyph);
					w = w * size / 1000.0;
					a[4] = x;
					a[5] = y;
					sp_chars_add_element (chars, glyph, (GnomeFontFace *) face, a);
					x += w;
				}
			}
		}
	}
}

static GSList * 
sp_text_snappoints (SPItem * item, GSList * points)
{
  ArtPoint * p;
  gdouble affine[6];

  /* we use corners of item and x,y coordinates of ellipse */
  if (SP_ITEM_CLASS (parent_class)->snappoints)
    points = (SP_ITEM_CLASS (parent_class)->snappoints) (item, points);

  p = g_new (ArtPoint,1);
  p->x = SP_TEXT(item)->x;
  p->y = SP_TEXT(item)->y;
  sp_item_i2d_affine (item, affine);
  art_affine_point (p, p, affine);
  g_slist_append (points, p);
  return points;
}

/*
 * Initially we'll do:
 * Transform x, y, set x, y, clear translation
 */

static void
sp_text_write_transform (SPItem *item, SPRepr *repr, gdouble *transform)
{
	SPText *text;
	gdouble rev[6];
	gdouble px, py;

	text = SP_TEXT (item);

	/* Calculate text start in parent coords */
	px = transform[0] * text->x + transform[2] * text->y + transform[4];
	py = transform[1] * text->x + transform[3] * text->y + transform[5];
	/* Clear translation */
	transform[4] = 0.0;
	transform[5] = 0.0;
	/* Find text start in item coords */
	art_affine_invert (rev, transform);
	sp_repr_set_double_attribute (repr, "x", px * rev[0] + py * rev[2]);
	sp_repr_set_double_attribute (repr, "y", px * rev[1] + py * rev[3]);

	if ((fabs (transform[0] - 1.0) > 1e-9) ||
	    (fabs (transform[3] - 1.0) > 1e-9) ||
	    (fabs (transform[1]) > 1e-9) ||
	    (fabs (transform[2]) > 1e-9)) {
		guchar t[80];
		sp_svg_write_affine (t, 80, transform);
		sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", t);
	} else {
		sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", NULL);
	}
}


