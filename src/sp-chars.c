#define __SP_CHARS_C__

/*
 * SPChars - parent class for text objects
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "macros.h"
#include "helper/art-utils.h"
#include "display/nr-arena-shape.h"
#include "style.h"
#include "sp-chars.h"

static void sp_chars_class_init (SPCharsClass *class);
static void sp_chars_init (SPChars *chars);
static void sp_chars_destroy (GtkObject *object);

static void sp_chars_style_modified (SPObject *object, guint flags);

static void sp_chars_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform);
static NRArenaItem *sp_chars_show (SPItem *item, NRArena *arena);

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
		gdouble a[6], b[6];
		gint i;
		bpath = gnome_font_face_get_glyph_stdoutline (el->face, el->glyph);
		for (i = 0; i < 6; i++) a[i] = el->affine[i];
		art_affine_multiply (b, a, transform);

		sp_bpath_matrix_d_bbox_d_union (bpath, b, bbox, 0.25);
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

SPCurve *
sp_chars_normalized_bpath (SPChars *chars)
{
	SPCharElement *el;
	GSList *cc;
	SPCurve *curve;

	cc = NULL;
	for (el = chars->elements; el != NULL; el = el->next) {
		const ArtBpath *bp;
		ArtBpath *abp;
		SPCurve *c;
		gdouble a[6];
		gint i;
		for (i = 0; i < 6; i++) a[i] = el->affine[i];
		bp = gnome_font_face_get_glyph_stdoutline (el->face, el->glyph);
		abp = art_bpath_affine_transform (bp, a);
		c = sp_curve_new_from_bpath (abp);
		if (c) {
			cc = g_slist_prepend (cc, c);
		}
	}

	cc = g_slist_reverse (cc);

	curve = sp_curve_concat (cc);

	while (cc) {
		/* fixme: This is dangerous, as we are mixing art_alloc and g_new */
		sp_curve_unref ((SPCurve *) cc->data);
		cc = g_slist_remove (cc, cc->data);
	}

	return curve;
}

/* This is completely unrelated to SPItem::print */

static void
sp_chars_print_bpath (GnomePrintContext *ctx, const ArtBpath *bpath, const SPStyle *style,
		      const gdouble *ctm, const ArtDRect *dbox, const ArtDRect *bbox)
{
	if (style->fill.type == SP_PAINT_TYPE_COLOR) {
		gfloat rgb[3], opacity;

		sp_color_get_rgb_floatv (&style->fill.value.color, rgb);
		/* fixme: This is not correct, we should fall back to bitmap here instead */
		opacity = SP_SCALE24_TO_FLOAT (style->fill_opacity.value) * SP_SCALE24_TO_FLOAT (style->opacity.value);
		/* Printing code */
		gnome_print_gsave (ctx);
		gnome_print_setrgbcolor (ctx, rgb[0], rgb[1], rgb[2]);
		gnome_print_setopacity (ctx, opacity);
		gnome_print_bpath (ctx, (ArtBpath *) bpath, FALSE);
		if (style->fill_rule.value == 1) {
			gnome_print_eofill (ctx);
		} else {
			gnome_print_fill (ctx);
		}
		gnome_print_grestore (ctx);

	} else if (style->fill.type == SP_PAINT_TYPE_PAINTSERVER) {
		SPPainter *painter;
		static gdouble id[6] = {1,0,0,1,0,0};
		/* fixme: */
		painter = sp_paint_server_painter_new (SP_STYLE_FILL_SERVER (style), id, SP_SCALE24_TO_FLOAT (style->opacity.value), bbox);
		if (painter) {
			ArtDRect cbox, pbox;
			ArtIRect ibox;
			gdouble d2i[6];
			gint x, y;
			guchar *rgba;

			/* Find path bbox */
			pbox.x0 = pbox.y0 = pbox.x1 = pbox.y1 = 0.0;
			sp_bpath_matrix_d_bbox_d_union (bpath, ctm, &pbox, 0.25);

			art_drect_intersect (&cbox, dbox, &pbox);
			art_drect_to_irect (&ibox, &cbox);
			art_affine_invert (d2i, ctm);

			gnome_print_gsave (ctx);

			gnome_print_bpath (ctx, (ArtBpath *) bpath, FALSE);
			if (style->fill_rule.value == 1) {
				gnome_print_eoclip (ctx);
			} else {
				gnome_print_clip (ctx);
			}
			gnome_print_concat (ctx, d2i);

			/* Now we are in desktop coordinates */
			gnome_print_newpath (ctx);
			gnome_print_moveto (ctx, cbox.x0, cbox.y0);
			gnome_print_lineto (ctx, cbox.x1, cbox.y0);
			gnome_print_lineto (ctx, cbox.x1, cbox.y1);
			gnome_print_lineto (ctx, cbox.x0, cbox.y1);
			gnome_print_closepath (ctx);
			gnome_print_clip (ctx);

			rgba = nr_buffer_4_4096_get (FALSE, 0x00000000);
			for (y = ibox.y0; y < ibox.y1; y+= 64) {
				for (x = ibox.x0; x < ibox.x1; x+= 64) {
					painter->fill (painter, rgba, x, ibox.y1 + ibox.y0 - y - 64, 64, 64, 4 * 64);
					gnome_print_gsave (ctx);
					gnome_print_translate (ctx, x, y);
					gnome_print_scale (ctx, 64, 64);
					gnome_print_rgbaimage (ctx, rgba, 64, 64, 4 * 64);
					gnome_print_grestore (ctx);
				}
			}
			nr_buffer_4_4096_free (rgba);
			gnome_print_grestore (ctx);
			sp_painter_free (painter);
		}
	}
}

void
sp_chars_do_print (SPChars *chars, GnomePrintContext *gpc, const gdouble *ctm, const ArtDRect *dbox, const ArtDRect *bbox)
{
	SPCharElement *el;

	for (el = chars->elements; el != NULL; el = el->next) {
		gdouble chela[6];
		const ArtBpath *bpath;
		ArtBpath *abp;
		gint i;

		for (i = 0; i < 6; i++) chela[i] = el->affine[i];
		bpath = gnome_font_face_get_glyph_stdoutline (el->face, el->glyph);
		abp = art_bpath_affine_transform (bpath, chela);

		sp_chars_print_bpath (gpc, abp, SP_OBJECT_STYLE (chars), ctm, dbox, bbox);

		art_free (abp);
	}
}


