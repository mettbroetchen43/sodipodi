#define __SP_CHARS_C__

/*
 * SPChars - parent class for text objects
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 */

#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_vpath_bpath.h>
#include "display/nr-arena-shape.h"
#include "sp-chars.h"

static void sp_chars_class_init (SPCharsClass *class);
static void sp_chars_init (SPChars *chars);
static void sp_chars_destroy (GtkObject *object);

static void sp_chars_style_modified (SPObject *object, guint flags);

static void sp_chars_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform);
static NRArenaItem *sp_chars_show (SPItem *item, NRArena *arena);
void sp_chars_print (SPItem * item, GnomePrintContext * gpc);

static SPItemClass *parent_class;

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
		type = gtk_type_unique (SP_TYPE_ITEM, &info);
	}
	return type;
}

static void
sp_chars_class_init (SPCharsClass *klass)
{
	GtkObjectClass *object_class;
	SPObjectClass *sp_object_class;
	SPItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;

	parent_class = gtk_type_class (SP_TYPE_ITEM);

	object_class->destroy = sp_chars_destroy;

	sp_object_class->style_modified = sp_chars_style_modified;

	item_class->bbox = sp_chars_bbox;
	item_class->show = sp_chars_show;
	item_class->print = sp_chars_print;
}

static void
sp_chars_init (SPChars *chars)
{
	chars->elements = NULL;
}

static void
sp_chars_destroy (GtkObject *object)
{
	SPChars *chars;

	chars = SP_CHARS (object);

	while (chars->elements) {
		SPCharElement *el;
		el = chars->elements;
		chars->elements = el->next;
		gnome_font_face_unref (el->face);
		g_free (el);
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_chars_style_modified (SPObject *object, guint flags)
{
	SPChars *chars;
	SPItemView *v;

	chars = SP_CHARS (object);

	/* Item class reads style */
	if (((SPObjectClass *) (parent_class))->style_modified)
		(* ((SPObjectClass *) (parent_class))->style_modified) (object, flags);

	for (v = SP_ITEM (chars)->display; v != NULL; v = v->next) {
		/* fixme: */
		nr_arena_shape_group_set_style (NR_ARENA_SHAPE_GROUP (v->arenaitem), object->style);
	}
}

static void
sp_chars_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform)
{
	SPChars *chars;
	SPCharElement *el;

	chars = SP_CHARS (item);

	bbox->x0 = bbox->y0 = bbox->x1 = bbox->y1 = 0.0;

	for (el = chars->elements; el != NULL; el = el->next) {
		const ArtBpath *bpath;
		ArtBpath *abp;
		ArtVpath *vpath;
		ArtDRect bb;
		gdouble a[6], b[6];
		gint i;
		bpath = gnome_font_face_get_glyph_stdoutline (el->face, el->glyph);
		for (i = 0; i < 6; i++) a[i] = el->affine[i];
		art_affine_multiply (b, a, transform);
		abp = art_bpath_affine_transform (bpath, b);
		vpath = art_bez_path_to_vec (abp, 0.25);
		art_free (abp);
		art_vpath_bbox_drect (vpath, &bb);
		art_free (vpath);
		art_drect_union (bbox, bbox, &bb);
	}
}

static NRArenaItem *
sp_chars_show (SPItem *item, NRArena *arena)
{
	SPChars *chars;
	NRArenaItem *arenaitem;
	SPCharElement *el;

	chars = SP_CHARS (item);

	arenaitem = nr_arena_item_new (arena, NR_TYPE_ARENA_SHAPE_GROUP);

	nr_arena_shape_group_set_style (NR_ARENA_SHAPE_GROUP (arenaitem), SP_OBJECT_STYLE (item));

	g_print ("Chars show\n");

	for (el = chars->elements; el != NULL; el = el->next) {
		const ArtBpath *bpath;
		SPCurve *curve;
		bpath = gnome_font_face_get_glyph_stdoutline (el->face, el->glyph);
		curve = sp_curve_new_from_static_bpath ((ArtBpath *) bpath);
		if (curve) {
			gdouble a[6];
			gint i;
			for (i = 0; i < 6; i++) a[i] = el->affine[i];
			nr_arena_shape_group_add_component (NR_ARENA_SHAPE_GROUP (arenaitem), curve, FALSE, a);
			sp_curve_unref (curve);
		}
	}

	return arenaitem;
}

void
sp_chars_print (SPItem * item, GnomePrintContext * gpc)
{
	g_warning ("SPChars::print not implemented");
}

void
sp_chars_clear (SPChars *chars)
{
	SPItem *item;
	SPItemView *v;

	item = SP_ITEM (chars);

	while (chars->elements) {
		SPCharElement *el;
		el = chars->elements;
		chars->elements = el->next;
		gnome_font_face_unref (el->face);
		g_free (el);
	}

	for (v = item->display; v != NULL; v = v->next) {
		nr_arena_shape_group_clear (NR_ARENA_SHAPE_GROUP (v->arenaitem));
	}
}

void
sp_chars_add_element (SPChars *chars, guint glyph, GnomeFontFace *face, const gdouble *affine)
{
	SPItem *item;
	SPItemView *v;
	SPCharElement * el;
	const ArtBpath *bpath;
	SPCurve *curve;
	gint i;

	item = SP_ITEM (chars);

	el = g_new (SPCharElement, 1);

	el->glyph = glyph;
	el->face = face;
	gnome_font_face_ref (face);

	for (i = 0; i < 6; i++) el->affine[i] = affine[i];

	el->next = chars->elements;
	chars->elements = el;

	bpath = gnome_font_face_get_glyph_stdoutline (el->face, el->glyph);
	curve = sp_curve_new_from_static_bpath ((ArtBpath *) bpath);
	if (curve) {
		gdouble a[6];
		gint i;
		for (i = 0; i < 6; i++) a[i] = el->affine[i];
		for (v = item->display; v != NULL; v = v->next) {
			nr_arena_shape_group_add_component (NR_ARENA_SHAPE_GROUP (v->arenaitem), curve, FALSE, a);
		}
		sp_curve_unref (curve);
	}
}





















