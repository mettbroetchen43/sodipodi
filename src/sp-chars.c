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

#include <string.h>

#include <libnr/nr-pixblock.h>
#include "macros.h"
#include "helper/art-utils.h"
#include "display/nr-arena-glyphs.h"
#include "style.h"
#include "sp-chars.h"

static void sp_chars_class_init (SPCharsClass *class);
static void sp_chars_init (SPChars *chars);

static void sp_chars_release (SPObject *object);
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

	sp_object_class->release = sp_chars_release;
	sp_object_class->style_modified = sp_chars_style_modified;

	item_class->bbox = sp_chars_bbox;
	item_class->show = sp_chars_show;
}

static void
sp_chars_init (SPChars *chars)
{
	chars->elements = NULL;

	chars->paintbox.x0 = chars->paintbox.y0 = 0.0;
	chars->paintbox.x1 = chars->paintbox.y1 = 1.0;
}

static void
sp_chars_release (SPObject *object)
{
	SPChars *chars;

	chars = SP_CHARS (object);

	while (chars->elements) {
		SPCharElement *el;
		el = chars->elements;
		chars->elements = el->next;
		nr_font_unref (el->font);
		g_free (el);
	}

	if (((SPObjectClass *) parent_class)->release)
		((SPObjectClass *) parent_class)->release (object);
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
		nr_arena_glyphs_group_set_style (NR_ARENA_GLYPHS_GROUP (v->arenaitem), object->style);
	}
}

static void
sp_chars_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform)
{
	SPChars *chars;
	SPCharElement *el;

	chars = SP_CHARS (item);

	for (el = chars->elements; el != NULL; el = el->next) {
		NRBPath bpath;
		gdouble a[6], b[6];
		gint i;
		if (nr_font_glyph_outline_get (el->font, el->glyph, &bpath, FALSE)) {
			for (i = 0; i < 6; i++) a[i] = el->transform.c[i];
			art_affine_multiply (b, a, transform);
			sp_bpath_matrix_d_bbox_d_union (bpath.path, b, bbox, 0.25);
		}
	}
}

static NRArenaItem *
sp_chars_show (SPItem *item, NRArena *arena)
{
	SPChars *chars;
	NRArenaItem *arenaitem;
	SPCharElement *el;

	chars = SP_CHARS (item);

	arenaitem = nr_arena_item_new (arena, NR_TYPE_ARENA_GLYPHS_GROUP);

	nr_arena_glyphs_group_set_style (NR_ARENA_GLYPHS_GROUP (arenaitem), SP_OBJECT_STYLE (item));

	for (el = chars->elements; el != NULL; el = el->next) {
#if 0
		NRBPath bpath;
		SPCurve *curve;

		if (nr_font_glyph_outline_get (el->font, el->glyph, &bpath, FALSE)) {
			curve = sp_curve_new_from_static_bpath (bpath.path);
			if (curve) {
				gdouble a[6];
				gint i;
				for (i = 0; i < 6; i++) a[i] = el->transform.c[i];
				nr_arena_glyphs_group_add_component (NR_ARENA_GLYPHS_GROUP (arenaitem), curve, FALSE, a);
				sp_curve_unref (curve);
			}
		}
#else
		nr_arena_glyphs_group_add_component (NR_ARENA_GLYPHS_GROUP (arenaitem), el->font, el->glyph, &el->transform);
#endif
	}

	nr_arena_glyphs_group_set_paintbox (NR_ARENA_GLYPHS_GROUP (arenaitem), &chars->paintbox);

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
		nr_font_unref (el->font);
		g_free (el);
	}

	for (v = item->display; v != NULL; v = v->next) {
		nr_arena_glyphs_group_clear (NR_ARENA_GLYPHS_GROUP (v->arenaitem));
	}
}

void
sp_chars_add_element (SPChars *chars, guint glyph, NRFont *font, const NRMatrixF *transform)
{
	SPItem *item;
	SPItemView *v;
	SPCharElement * el;
#if 0
	NRBPath bpath;
	SPCurve *curve;
#endif

	item = SP_ITEM (chars);

	el = g_new (SPCharElement, 1);

	el->glyph = glyph;
	el->font = font;
	nr_font_ref (font);

	el->transform = *transform;

	el->next = chars->elements;
	chars->elements = el;

#if 0
	if (nr_font_glyph_outline_get (el->font, el->glyph, &bpath, FALSE)) {
		curve = sp_curve_new_from_static_bpath (bpath.path);
		if (curve) {
			gdouble a[6];
			gint i;
			for (i = 0; i < 6; i++) a[i] = el->transform.c[i];
			for (v = item->display; v != NULL; v = v->next) {
				nr_arena_glyphs_group_add_component (NR_ARENA_GLYPHS_GROUP (v->arenaitem), curve, FALSE, a);
			}
			sp_curve_unref (curve);
		}
	}
#else
	for (v = item->display; v != NULL; v = v->next) {
		nr_arena_glyphs_group_add_component (NR_ARENA_GLYPHS_GROUP (v->arenaitem), el->font, el->glyph, &el->transform);
	}
#endif
}

SPCurve *
sp_chars_normalized_bpath (SPChars *chars)
{
	SPCharElement *el;
	GSList *cc;
	SPCurve *curve;

	cc = NULL;
	for (el = chars->elements; el != NULL; el = el->next) {
		NRBPath bp;
		ArtBpath *abp;
		SPCurve *c;
		gdouble a[6];
		gint i;
		for (i = 0; i < 6; i++) a[i] = el->transform.c[i];
		if (nr_font_glyph_outline_get (el->font, el->glyph, &bp, FALSE)) {
			abp = art_bpath_affine_transform (bp.path, a);
			c = sp_curve_new_from_bpath (abp);
			if (c) cc = g_slist_prepend (cc, c);
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
sp_chars_print_bpath (GnomePrintContext *ctx, const ArtBpath *bpath, const SPStyle *style, const gdouble *ctm,
		      const ArtDRect *pbox, const ArtDRect *dbox, const ArtDRect *bbox)
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
		/* fixme: */
		painter = sp_paint_server_painter_new (SP_STYLE_FILL_SERVER (style), ctm, pbox);
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

			rgba = nr_pixelstore_16K_new (FALSE, 0x00000000);
			for (y = ibox.y0; y < ibox.y1; y+= 64) {
				for (x = ibox.x0; x < ibox.x1; x+= 64) {
					memset (rgba, 0x0, 4 * 64 * 64);
					painter->fill (painter, rgba, x, y, 64, 64, 4 * 64);
					gnome_print_gsave (ctx);
					gnome_print_translate (ctx, x, y + 64);
					gnome_print_scale (ctx, 64, -64);
					gnome_print_rgbaimage (ctx, rgba, 64, 64, 4 * 64);
					gnome_print_grestore (ctx);
				}
			}
			nr_pixelstore_16K_free (rgba);
			gnome_print_grestore (ctx);
			sp_painter_free (painter);
		}
	}

	if (style->stroke.type == SP_PAINT_TYPE_COLOR) {
		gfloat rgb[3], opacity;
		sp_color_get_rgb_floatv (&style->stroke.value.color, rgb);
		/* fixme: This is not correct, we should fall back to bitmap here instead */
		opacity = SP_SCALE24_TO_FLOAT (style->stroke_opacity.value) * SP_SCALE24_TO_FLOAT (style->opacity.value);
		/* Printing code */
		gnome_print_gsave (ctx);
		gnome_print_setrgbcolor (ctx, rgb[0], rgb[1], rgb[2]);
		gnome_print_setopacity (ctx, opacity);
		gnome_print_setlinewidth (ctx, style->stroke_width.computed);
		gnome_print_setlinejoin (ctx, style->stroke_linejoin.value);
		gnome_print_setlinecap (ctx, style->stroke_linecap.value);
		gnome_print_bpath (ctx, (ArtBpath *) bpath, FALSE);
		gnome_print_stroke (ctx);
		gnome_print_grestore (ctx);
	}

	/* fixme: Print gradient stroke (Lauris) */
}

/*
 * pbox is bbox for paint server (user coordinates)
 * dbox is the whole display area
 * bbox is item bbox on desktop
 */

void
sp_chars_do_print (SPChars *chars, GnomePrintContext *gpc, const gdouble *ctm, const ArtDRect *pbox, const ArtDRect *dbox, const ArtDRect *bbox)
{
	SPCharElement *el;

	for (el = chars->elements; el != NULL; el = el->next) {
		gdouble chela[6];
		NRBPath bpath;
		ArtBpath *abp;
		gint i;

		for (i = 0; i < 6; i++) chela[i] = el->transform.c[i];
		if (nr_font_glyph_outline_get (el->font, el->glyph, &bpath, FALSE)) {
			abp = art_bpath_affine_transform (bpath.path, chela);

			sp_chars_print_bpath (gpc, abp, SP_OBJECT_STYLE (chars), ctm, pbox, dbox, bbox);

			art_free (abp);
		}
	}
}

void
sp_chars_set_paintbox (SPChars *chars, ArtDRect *paintbox)
{
	SPItemView *v;

	memcpy (&chars->paintbox, paintbox, sizeof (ArtDRect));

	for (v = SP_ITEM (chars)->display; v != NULL; v = v->next) {
		nr_arena_glyphs_group_set_paintbox (NR_ARENA_GLYPHS_GROUP (v->arenaitem), paintbox);
	}
}

