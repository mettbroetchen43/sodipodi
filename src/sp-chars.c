#define __SP_CHARS_C__

/*
 * SPChars - parent class for text objects
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include "sp-chars.h"

static void sp_chars_class_init (SPCharsClass *class);
static void sp_chars_init (SPChars *chars);
static void sp_chars_destroy (GtkObject *object);

static void sp_chars_set_shape (SPChars * chars);

static SPShapeClass *parent_class;

GtkType
sp_chars_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPChars",
			sizeof (SPChars),
			sizeof (SPCharsClass),
			(GtkClassInitFunc) sp_chars_class_init,
			(GtkObjectInitFunc) sp_chars_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_SHAPE, &info);
	}
	return type;
}

static void
sp_chars_class_init (SPCharsClass *class)
{
	GtkObjectClass *object_class;
	SPItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (SP_TYPE_SHAPE);

	object_class->destroy = sp_chars_destroy;
}

static void
sp_chars_init (SPChars *chars)
{
	chars->shape.path.independent = FALSE;
	chars->element = NULL;
}

static void
sp_chars_destroy (GtkObject *object)
{
	SPChars * chars;
	SPCharElement * el;

	chars = SP_CHARS (object);

	while (chars->element) {
		el = (SPCharElement *) chars->element->data;
		gnome_font_face_unref (el->face);
		g_free (el);
		chars->element = g_list_remove (chars->element, el);
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_chars_set_shape (SPChars * chars)
{
	SPShape * shape;
	GList * list;
	SPCharElement * el;
	SPCurve * curve;

	shape = (SPShape *) chars;

	sp_path_clear (SP_PATH (chars));

	for (list = chars->element; list != NULL; list = list->next) {
		const ArtBpath * bpath;
		el = (SPCharElement *) list->data;
		bpath = gnome_font_face_get_glyph_stdoutline (el->face, el->glyph);
		curve = sp_curve_new_from_static_bpath ((ArtBpath *) bpath);
		if (curve != NULL) {
			gdouble a[6];
			gint i;
			for (i = 0; i < 6; i++) a[i] = el->affine[i];
			sp_path_add_bpath (SP_PATH (chars), curve, FALSE, a);
			sp_curve_unref (curve);
		}
	}
}

void
sp_chars_clear (SPChars * chars)
{
	SPCharElement * el;

	sp_path_clear (SP_PATH (chars));

	while (chars->element) {
		el = (SPCharElement *) chars->element->data;
		gnome_font_face_unref (el->face);
		g_free (el);
		chars->element = g_list_remove (chars->element, el);
	}
}

void
sp_chars_add_element (SPChars *chars, guint glyph, GnomeFontFace *face, const gdouble *affine)
{
	SPCharElement * el;
	gint i;

	el = g_new (SPCharElement, 1);

	el->glyph = glyph;
	el->face = face;
	gnome_font_face_ref (face);

	for (i = 0; i < 6; i++) el->affine[i] = affine[i];

	chars->element = g_list_prepend (chars->element, el);
	/* fixme: */
	sp_chars_set_shape (chars);
}





















