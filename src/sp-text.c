#define __SP_TEXT_C__

/*
 * SVG <text> and <tspan> implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/*
 * fixme:
 *
 * These subcomponents should not be items, or alternately
 * we have to invent set of flags to mark, whether standard
 * attributes are applicable to given item (I even like this
 * idea somewhat - Lauris)
 *
 */

#include <config.h>

#include <string.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <gal/unicode/gunicode.h>

#include "macros.h"
#include "helper/art-utils.h"
#include "xml/repr-private.h"
#include "svg/svg.h"
#include "display/nr-arena-group.h"
#include "display/nr-arena-glyphs.h"
#include "document.h"
#include "style.h"
/* For versioning */
#include "sp-root.h"

#include "sp-text.h"

/* SPString */

static void sp_string_class_init (SPStringClass *class);
static void sp_string_init (SPString *string);
static void sp_string_destroy (GtkObject *object);

static void sp_string_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_string_read_content (SPObject *object);
static void sp_string_modified (SPObject *object, guint flags);

static void sp_string_calculate_dimensions (SPString *string);
static void sp_string_set_shape (SPString *string, SPLayoutData *ly, ArtPoint *cp, gboolean inspace);

static SPCharsClass *string_parent_class;

GtkType
sp_string_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPString",
			sizeof (SPString),
			sizeof (SPStringClass),
			(GtkClassInitFunc) sp_string_class_init,
			(GtkObjectInitFunc) sp_string_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_CHARS, &info);
	}
	return type;
}

static void
sp_string_class_init (SPStringClass *class)
{
	GtkObjectClass *object_class;
	SPObjectClass *sp_object_class;
	SPItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	string_parent_class = gtk_type_class (SP_TYPE_CHARS);

	object_class->destroy = sp_string_destroy;

	sp_object_class->build = sp_string_build;
	sp_object_class->read_content = sp_string_read_content;
	sp_object_class->modified = sp_string_modified;
}

static void
sp_string_init (SPString *string)
{
	string->text = NULL;
	string->p = NULL;
	string->start = 0;
	string->length = 0;
	string->bbox.x0 = string->bbox.y0 = 0.0;
	string->bbox.x1 = string->bbox.y1 = 0.0;
	string->advance.x = string->advance.y = 0.0;
}

static void
sp_string_destroy (GtkObject *object)
{
	SPString *string;

	string = SP_STRING (object);

	if (string->p) g_free (string->p);
	if (string->text) g_free (string->text);

	GTK_OBJECT_CLASS (string_parent_class)->destroy (object);
}

static void
sp_string_build (SPObject *object, SPDocument *doc, SPRepr *repr)
{
	SP_OBJECT_CLASS (string_parent_class)->build (object, doc, repr);

	sp_string_read_content (object);

	/* fixme: This can be waste here, but ensures loaded documents are up-to-date */
	sp_string_calculate_dimensions (SP_STRING (object));
}

/* fixme: We have to notify parents that we changed */

static void
sp_string_read_content (SPObject *object)
{
	SPString *string;
	const guchar *t;

	string = SP_STRING (object);

	if (string->p) g_free (string->p);
	string->p = NULL;
	if (string->text) g_free (string->text);
	t = sp_repr_content (object->repr);
	string->text = (t) ? g_strdup (t) : NULL;

	/* Is this correct? I think so (Lauris) */
	/* Virtual method will be invoked BEFORE signal, so we can update there */
	sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
}

/* This happen before parent does layouting but after styles have been set */
/* So it is the right place to calculate untransformed string dimensions */

static void
sp_string_modified (SPObject *object, guint flags)
{
	if (flags & (SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_MODIFIED_FLAG)) {
		/* Parent style or we ourselves changed, so recalculate */
		sp_string_calculate_dimensions (SP_STRING (object));
	}
}

/* Vertical metric simulator */

static ArtDRect *
sp_font_get_glyph_bbox (const GnomeFont *font, gint glyph, gint mode, ArtDRect *bbox)
{
	const ArtBpath *bpath;
	ArtDRect hbox;

	/* Find correct bbox (gnome-print does it wrong) */
	bpath = gnome_font_get_glyph_stdoutline (font, glyph);
	if (!bpath) return NULL;
	hbox.x0 = hbox.y0 = 1e18;
	hbox.x1 = hbox.y1 = -1e18;
	sp_bpath_matrix_d_bbox_d_union (bpath, NULL, &hbox, 0.25);
	if (art_drect_empty (&hbox)) return NULL;

	if (mode == SP_CSS_WRITING_MODE_LR) {
		*bbox = hbox;
		return bbox;
	} else {
		gdouble dy;
		/* Center horizontally */
		bbox->x0 = 0.0 - (hbox.x1 - hbox.x0) / 2;
		bbox->x1 = 0.0 + (hbox.x1 - hbox.x0) / 2;
		/* Just move down by EM */
		dy = gnome_font_get_size (font);
		bbox->y0 = hbox.y0 - dy;
		bbox->y1 = hbox.y1 - dy;
		return bbox;
	}
}

static ArtPoint *
sp_font_get_glyph_advance (const GnomeFont *font, gint glyph, gint mode, ArtPoint *adv)
{
	if (mode == SP_CSS_WRITING_MODE_LR) {
		return gnome_font_get_glyph_stdadvance (font, glyph, adv);
	} else {
		adv->x = 0.0;
		adv->y = -gnome_font_get_size (font);
		return adv;
	}
}

static void
sp_font_get_glyph_bbox_lr2tb (const GnomeFont *font, gint glyph, ArtPoint *d)
{
	ArtDRect hbox;

	d->x = 0.0;
	d->y = 0.0;

	if (sp_font_get_glyph_bbox (font, glyph, SP_CSS_WRITING_MODE_LR, &hbox)) {
		/* Center horizontally */
		d->x = 0.0 - (hbox.x1 + hbox.x0) / 2;
		/* Just move down by EM */
		d->y = 0.0 - gnome_font_get_size (font);
	}
}

static void
sp_string_calculate_dimensions (SPString *string)
{
	SPStyle *style;
	const GnomeFont *font;
	gdouble size, spwidth;
	gint spglyph;

	string->bbox.x0 = string->bbox.y0 = 1e18;
	string->bbox.x1 = string->bbox.y1 = -1e18;
	string->advance.x = 0.0;
	string->advance.y = 0.0;

	style = SP_OBJECT_STYLE (string);
	/* fixme: Adjusted value (Lauris) */
	size = style->font_size.computed;
	font = gnome_font_new_closest (style->text->font_family.value,
				       sp_text_font_weight_to_gp (style->font_weight.computed),
				       sp_text_font_italic_to_gp (style->font_style.computed),
				       size);
	spglyph = gnome_font_lookup_default (font, ' ');
	spwidth = (spglyph > 0) ? gnome_font_get_glyph_width (font, spglyph) : size;

	if (string->text) {
		const guchar *p;
		gboolean inspace, intext;

		inspace = FALSE;
		intext = FALSE;

		for (p = string->text; p && *p; p = g_utf8_next_char (p)) {
			gunichar unival;
			
			unival = g_utf8_get_char (p);

			if (unival == ' ') {
				if (intext) inspace = TRUE;
			} else {
				ArtDRect bbox;
				ArtPoint adv;
				gint glyph;

				glyph = gnome_font_lookup_default (font, unival);

				if (style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
					if (inspace) {
						string->advance.y += size;
						inspace = FALSE;
					}
					if (sp_font_get_glyph_bbox (font, glyph, SP_CSS_WRITING_MODE_TB, &bbox)) {
						string->bbox.x0 = MIN (string->bbox.x0, string->advance.x + bbox.x0);
						string->bbox.y0 = MIN (string->bbox.y0, string->advance.y - bbox.y1);
						string->bbox.x1 = MAX (string->bbox.x1, string->advance.x + bbox.x1);
						string->bbox.y1 = MAX (string->bbox.y1, string->advance.y - bbox.y0);
					}
					if (sp_font_get_glyph_advance (font, glyph, SP_CSS_WRITING_MODE_TB, &adv)) {
						string->advance.x += adv.x;
						string->advance.y -= adv.y;
					}
				} else {
					if (inspace) {
						string->advance.x += spwidth;
						inspace = FALSE;
					}
					if (sp_font_get_glyph_bbox (font, glyph, SP_CSS_WRITING_MODE_LR, &bbox)) {
						string->bbox.x0 = MIN (string->bbox.x0, string->advance.x + bbox.x0);
						string->bbox.y0 = MIN (string->bbox.y0, string->advance.y - bbox.y1);
						string->bbox.x1 = MAX (string->bbox.x1, string->advance.x + bbox.x1);
						string->bbox.y1 = MAX (string->bbox.y1, string->advance.y - bbox.y0);
					}
					if (sp_font_get_glyph_advance (font, glyph, SP_CSS_WRITING_MODE_LR, &adv)) {
						string->advance.x += adv.x;
						string->advance.y -= adv.y;
					}
				}
				intext = TRUE;
			}
		}
	}

	gnome_font_unref (font);

	if (art_drect_empty (&string->bbox)) {
		string->bbox.x0 = string->bbox.y0 = 0.0;
		string->bbox.x1 = string->bbox.y1 = 0.0;
	}

}

/* fixme: Should values be parsed by parent? */

static void
sp_string_set_shape (SPString *string, SPLayoutData *ly, ArtPoint *cp, gboolean inspace)
{
	SPChars *chars;
	SPStyle *style;
	const GnomeFont *font;
	gdouble size, spwidth;
	gint spglyph;
	gdouble x, y;
	gdouble a[6];
	const guchar *p;
	gboolean intext;
	gint len, pos;

	chars = SP_CHARS (string);
	style = SP_OBJECT_STYLE (string);

	sp_chars_clear (chars);

	if (!string->text || !*string->text) return;
	len = g_utf8_strlen (string->text, -1);
	if (!len) return;
	if (string->p) g_free (string->p);
	string->p = g_new (NRPointF, len + 1);

	/* fixme: Adjusted value (Lauris) */
	size = style->font_size.computed;
	font = gnome_font_new_closest (style->text->font_family.value,
				       sp_text_font_weight_to_gp (style->font_weight.computed),
				       sp_text_font_italic_to_gp (style->font_style.computed),
				       size);
	spglyph = gnome_font_lookup_default (font, ' ');
	spwidth = (spglyph > 0) ? gnome_font_get_glyph_width (font, spglyph) : size;

	/* fixme: Find a way how to manipulate these */
	x = cp->x;
	y = cp->y;

	g_print ("Drawing string (%s) at %g %g\n", string->text, x, y);

	art_affine_scale (a, size * 0.001, size * -0.001);

	intext = FALSE;
	pos = 0;
	for (p = string->text; p && *p; p = g_utf8_next_char (p)) {
		gunichar unival;
		if (inspace) {
			if (style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
				string->p[pos].x = x;
				string->p[pos].y = y + size;
			} else {
				string->p[pos].x = x + spwidth;
				string->p[pos].y = y;
			}
		} else {
			string->p[pos].x = x;
			string->p[pos].y = y;
		}
		unival = g_utf8_get_char (p);
		if (unival == ' ') {
			if (intext) inspace = TRUE;
		} else {
			ArtPoint adv;
			gint glyph;

			glyph = gnome_font_lookup_default (font, unival);

			if (style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
				ArtPoint d;
				if (inspace) {
					y += size;
					inspace = FALSE;
				}
				sp_font_get_glyph_bbox_lr2tb (font, glyph, &d);
				g_print ("Unival %d:%c delta %g %g\n", unival, (gchar) unival, d.x, d.y);
				a[4] = x + d.x;
				a[5] = y - d.y;
				sp_chars_add_element (chars, glyph, (GnomeFontFace *) gnome_font_get_face (font), a);
				if (sp_font_get_glyph_advance (font, glyph, SP_CSS_WRITING_MODE_TB, &adv)) {
					x += adv.x;
					y -= adv.y;
				}
			} else {
				if (inspace) {
					x += spwidth;
					inspace = FALSE;
				}
				a[4] = x;
				a[5] = y;
				sp_chars_add_element (chars, glyph, (GnomeFontFace *) gnome_font_get_face (font), a);
				if (sp_font_get_glyph_advance (font, glyph, SP_CSS_WRITING_MODE_LR, &adv)) {
					x += adv.x;
					y -= adv.y;
				}
			}
			intext = TRUE;
		}
		pos += 1;
	}

	if (inspace) {
		if (style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
			string->p[pos].x = x;
			string->p[pos].y = y + size;
		} else {
			string->p[pos].x = x + spwidth;
			string->p[pos].y = y;
		}
	} else {
		string->p[pos].x = x;
		string->p[pos].y = y;
	}

	cp->x = x;
	cp->y = y;
	g_print ("Finished string (%s) at %g %g\n", string->text, x, y);
}

/* SPTSpan */

static void sp_tspan_class_init (SPTSpanClass *class);
static void sp_tspan_init (SPTSpan *tspan);
static void sp_tspan_destroy (GtkObject *object);

static void sp_tspan_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_tspan_read_attr (SPObject * object, const gchar * attr);
static void sp_tspan_child_added (SPObject *object, SPRepr *rch, SPRepr *ref);
static void sp_tspan_remove_child (SPObject *object, SPRepr *rch);
static void sp_tspan_modified (SPObject *object, guint flags);
static SPRepr *sp_tspan_write (SPObject *object, SPRepr *repr, guint flags);

static void sp_tspan_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform);
static NRArenaItem *sp_tspan_show (SPItem *item, NRArena *arena);
static void sp_tspan_hide (SPItem *item, NRArena *arena);

static void sp_tspan_set_shape (SPTSpan *tspan, SPLayoutData *ly, ArtPoint *cp, gboolean firstline, gboolean inspace);

static SPItemClass *tspan_parent_class;

GtkType
sp_tspan_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPTSpan",
			sizeof (SPTSpan),
			sizeof (SPTSpanClass),
			(GtkClassInitFunc) sp_tspan_class_init,
			(GtkObjectInitFunc) sp_tspan_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_ITEM, &info);
	}
	return type;
}

static void
sp_tspan_class_init (SPTSpanClass *class)
{
	GtkObjectClass *object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	tspan_parent_class = gtk_type_class (SP_TYPE_ITEM);

	object_class->destroy = sp_tspan_destroy;

	sp_object_class->build = sp_tspan_build;
	sp_object_class->read_attr = sp_tspan_read_attr;
	sp_object_class->child_added = sp_tspan_child_added;
	sp_object_class->remove_child = sp_tspan_remove_child;
	sp_object_class->modified = sp_tspan_modified;
	sp_object_class->write = sp_tspan_write;

	item_class->bbox = sp_tspan_bbox;
	item_class->show = sp_tspan_show;
	item_class->hide = sp_tspan_hide;
}

static void
sp_tspan_init (SPTSpan *tspan)
{
	/* fixme: Initialize layout */
	sp_svg_length_unset (&tspan->ly.x, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&tspan->ly.y, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&tspan->ly.dx, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&tspan->ly.dy, SP_SVG_UNIT_NONE, 0.0, 0.0);
	tspan->string = NULL;
}

static void
sp_tspan_destroy (GtkObject *object)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (object);

	if (tspan->string) {
		tspan->string = sp_object_detach_unref (SP_OBJECT (object), tspan->string);
	} else {
		g_print ("NULL tspan content\n");
	}

	if (GTK_OBJECT_CLASS (tspan_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (tspan_parent_class)->destroy) (object);
}

static void
sp_tspan_build (SPObject *object, SPDocument *doc, SPRepr *repr)
{
	SPTSpan *tspan;
	SPRepr *rch;

	tspan = SP_TSPAN (object);

	if (SP_OBJECT_CLASS (tspan_parent_class)->build)
		SP_OBJECT_CLASS (tspan_parent_class)->build (object, doc, repr);

	for (rch = repr->children; rch != NULL; rch = rch->next) {
		if (rch->type == SP_XML_TEXT_NODE) break;
	}

	if (rch) {
		SPString *string;
		/* fixme: We should really pick up first child always */
		string = gtk_type_new (SP_TYPE_STRING);
		tspan->string = sp_object_attach_reref (object, SP_OBJECT (string), NULL);
		string->ly = &tspan->ly;
		sp_object_invoke_build (tspan->string, doc, rch, SP_OBJECT_IS_CLONED (object));
	}

	sp_tspan_read_attr (object, "x");
	sp_tspan_read_attr (object, "y");
	sp_tspan_read_attr (object, "dx");
	sp_tspan_read_attr (object, "dy");
	sp_tspan_read_attr (object, "rotate");
	sp_tspan_read_attr (object, "sodipodi:role");
}

static void
sp_tspan_read_attr (SPObject *object, const gchar *attr)
{
	SPTSpan *tspan;
	const guchar *str;

	tspan = SP_TSPAN (object);

	str = sp_repr_attr (SP_OBJECT_REPR (object), attr);

	/* fixme: Vectors */
	if (!strcmp (attr, "x")) {
		if (!sp_svg_length_read (str, &tspan->ly.x)) {
			tspan->ly.x.set = FALSE;
			tspan->ly.x.computed = 0.0;
		}
		/* fixme: Re-layout it */
		if (tspan->role != SP_TSPAN_ROLE_LINE) sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (attr, "y")) {
		if (!sp_svg_length_read (str, &tspan->ly.y)) {
			tspan->ly.y.set = FALSE;
			tspan->ly.y.computed = 0.0;
		}
		/* fixme: Re-layout it */
		if (tspan->role != SP_TSPAN_ROLE_LINE) sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (attr, "dx")) {
		if (!sp_svg_length_read (str, &tspan->ly.dx)) {
			tspan->ly.dx.set = FALSE;
			tspan->ly.dx.computed = 0.0;
		}
		/* fixme: Re-layout it */
	} else if (!strcmp (attr, "dy")) {
		if (!sp_svg_length_read (str, &tspan->ly.dy)) {
			tspan->ly.dy.set = FALSE;
			tspan->ly.dy.computed = 0.0;
		}
		/* fixme: Re-layout it */
	} else if (strcmp (attr, "rotate") == 0) {
		/* fixme: Implement SVGNumber or something similar (Lauris) */
		tspan->ly.rotate = (str) ? atof (str) : 0.0;
		tspan->ly.rotate_set = (str != NULL);
		/* fixme: Re-layout it */
	} else if (!strcmp (attr, "sodipodi:role")) {
		if (str && (!strcmp (str, "line") || !strcmp (str, "paragraph"))) {
			tspan->role = SP_TSPAN_ROLE_LINE;
		} else {
			tspan->role = SP_TSPAN_ROLE_UNSPECIFIED;
		}
	} else {
		if (SP_OBJECT_CLASS (tspan_parent_class)->read_attr)
			(SP_OBJECT_CLASS (tspan_parent_class)->read_attr) (object, attr);
	}
}

static void
sp_tspan_child_added (SPObject *object, SPRepr *rch, SPRepr *ref)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (object);

	if (SP_OBJECT_CLASS (tspan_parent_class)->child_added)
		SP_OBJECT_CLASS (tspan_parent_class)->child_added (object, rch, ref);

	if (!tspan->string && rch->type == SP_XML_TEXT_NODE) {
		SPString *string;
		/* fixme: We should really pick up first child always */
		string = gtk_type_new (SP_TYPE_STRING);
		tspan->string = sp_object_attach_reref (object, SP_OBJECT (string), NULL);
		string->ly = &tspan->ly;
		sp_object_invoke_build (tspan->string, SP_OBJECT_DOCUMENT (object), rch, SP_OBJECT_IS_CLONED (object));
	}
}

static void
sp_tspan_remove_child (SPObject *object, SPRepr *rch)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (object);

	if (SP_OBJECT_CLASS (tspan_parent_class)->remove_child)
		SP_OBJECT_CLASS (tspan_parent_class)->remove_child (object, rch);

	if (tspan->string && (SP_OBJECT_REPR (tspan->string) == rch)) {
		tspan->string = sp_object_detach_unref (object, tspan->string);
	}
}

static void
sp_tspan_modified (SPObject *object, guint flags)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (object);

	if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	flags &= SP_OBJECT_MODIFIED_CASCADE;

	if (tspan->string) {
		sp_object_modified (tspan->string, flags);
	}
}

static SPRepr *
sp_tspan_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("tspan");
	}

	if (tspan->ly.x.set) sp_repr_set_double_attribute (repr, "x", tspan->ly.x.computed);
	if (tspan->ly.y.set) sp_repr_set_double_attribute (repr, "y", tspan->ly.y.computed);
	if (tspan->ly.dx.set) sp_repr_set_double_attribute (repr, "dx", tspan->ly.dx.computed);
	if (tspan->ly.dy.set) sp_repr_set_double_attribute (repr, "dy", tspan->ly.dy.computed);
	if (tspan->ly.rotate_set) sp_repr_set_double_attribute (repr, "rotate", tspan->ly.rotate);
	if (flags & SP_OBJECT_WRITE_SODIPODI) {
		sp_repr_set_attr (repr, "sodipodi:role", (tspan->role != SP_TSPAN_ROLE_UNSPECIFIED) ? "line" : NULL);
	}

	if (flags & SP_OBJECT_WRITE_BUILD) {
		SPRepr *rstr;
		/* TEXT element */
		rstr = sp_xml_document_createTextNode (sp_repr_document (repr), SP_STRING_TEXT (tspan->string));
		sp_repr_append_child (repr, rstr);
		sp_repr_unref (rstr);
	} else {
		sp_repr_set_content (SP_OBJECT_REPR (tspan->string), SP_STRING_TEXT (tspan->string));
	}

	/* fixme: Strictly speaking, item class write 'transform' too */
	/* fixme: This is harmless as long as tspan affine is identity (lauris) */
	if (SP_OBJECT_CLASS (tspan_parent_class)->write)
		SP_OBJECT_CLASS (tspan_parent_class)->write (object, repr, flags);

	return repr;
}

static void
sp_tspan_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (item);

	if (tspan->string) {
		sp_item_invoke_bbox (SP_ITEM (tspan->string), bbox, transform);
	}
}

static NRArenaItem *
sp_tspan_show (SPItem *item, NRArena *arena)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (item);

	if (tspan->string) {
		NRArenaItem *ai, *ac;
		ai = nr_arena_item_new (arena, NR_TYPE_ARENA_GROUP);
		nr_arena_group_set_transparent (NR_ARENA_GROUP (ai), FALSE);
		ac = sp_item_show (SP_ITEM (tspan->string), arena);
		if (ac) {
			nr_arena_item_add_child (ai, ac, NULL);
			gtk_object_unref (GTK_OBJECT (ac));
		}
		return ai;
	}

	return NULL;
}

static void
sp_tspan_hide (SPItem *item, NRArena *arena)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (item);

	if (tspan->string) sp_item_hide (SP_ITEM (tspan->string), arena);

	if (SP_ITEM_CLASS (tspan_parent_class)->hide)
		(* SP_ITEM_CLASS (tspan_parent_class)->hide) (item, arena);
}

static void
sp_tspan_set_shape (SPTSpan *tspan, SPLayoutData *ly, ArtPoint *cp, gboolean firstline, gboolean inspace)
{
	SPStyle *style;

	style = SP_OBJECT_STYLE (tspan);

	sp_string_set_shape (SP_STRING (tspan->string), &tspan->ly, cp, inspace);
}

/* SPText */

static void sp_text_class_init (SPTextClass *class);
static void sp_text_init (SPText *text);
static void sp_text_destroy (GtkObject *object);

static void sp_text_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_text_read_attr (SPObject *object, const gchar * attr);
static void sp_text_child_added (SPObject *object, SPRepr *rch, SPRepr *ref);
static void sp_text_remove_child (SPObject *object, SPRepr *rch);
static void sp_text_modified (SPObject *object, guint flags);
static SPRepr *sp_text_write (SPObject *object, SPRepr *repr, guint flags);

static void sp_text_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform);
static NRArenaItem *sp_text_show (SPItem *item, NRArena *arena);
static void sp_text_hide (SPItem *item, NRArena *arena);
static char * sp_text_description (SPItem *item);
static GSList * sp_text_snappoints (SPItem *item, GSList *points);
static void sp_text_write_transform (SPItem *item, SPRepr *repr, gdouble *transform);
static void sp_text_print (SPItem *item, GnomePrintContext *gpc);

static void sp_text_request_relayout (SPText *text, guint flags);
static void sp_text_update_immediate_state (SPText *text);
static void sp_text_set_shape (SPText *text);

static SPObject *sp_text_get_child_by_position (SPText *text, gint pos);

static SPItemClass *text_parent_class;

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
		type = gtk_type_unique (SP_TYPE_ITEM, &info);
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

	text_parent_class = gtk_type_class (SP_TYPE_ITEM);

	object_class->destroy = sp_text_destroy;

	sp_object_class->build = sp_text_build;
	sp_object_class->read_attr = sp_text_read_attr;
	sp_object_class->child_added = sp_text_child_added;
	sp_object_class->remove_child = sp_text_remove_child;
	sp_object_class->modified = sp_text_modified;
	sp_object_class->write = sp_text_write;

	item_class->bbox = sp_text_bbox;
	item_class->show = sp_text_show;
	item_class->hide = sp_text_hide;
	item_class->description = sp_text_description;
	item_class->snappoints = sp_text_snappoints;
	item_class->write_transform = sp_text_write_transform;
	item_class->print = sp_text_print;
}

static void
sp_text_init (SPText *text)
{
	/* fixme: Initialize layout */
	sp_svg_length_unset (&text->ly.x, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&text->ly.y, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&text->ly.dx, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&text->ly.dy, SP_SVG_UNIT_NONE, 0.0, 0.0);
	text->children = NULL;
}

static void
sp_text_destroy (GtkObject *object)
{
	SPText *text;

	text = SP_TEXT (object);

	while (text->children) {
		text->children = sp_object_detach_unref (SP_OBJECT (object), text->children);
	}

	if (GTK_OBJECT_CLASS (text_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (text_parent_class)->destroy) (object);
}

/* fixme: Better place (Lauris) */

static guint
sp_text_find_version (SPObject *object)
{

	while (object) {
		if (SP_IS_ROOT (object)) {
			return SP_ROOT (object)->sodipodi;
		}
		object = SP_OBJECT_PARENT (object);
	}

	return 0;
}

static void
sp_text_build (SPObject *object, SPDocument *doc, SPRepr *repr)
{
	SPText *text;
	SPObject *ref;
	SPRepr *rch;
	guint version;

	text = SP_TEXT (object);

	if (SP_OBJECT_CLASS (text_parent_class)->build)
		SP_OBJECT_CLASS (text_parent_class)->build (object, doc, repr);

	version = sp_text_find_version (object);
	if ((version > 0) && (version < 25)) {
		const guchar *content;
		/* Old sodipodi */
		for (rch = repr->children; rch != NULL; rch = rch->next) {
			if (rch->type == SP_XML_TEXT_NODE) {
				content = sp_repr_content (rch);
				sp_text_set_repr_text_multiline (text, content);
				break;
			}
		}
	}

	ref = NULL;

	for (rch = repr->children; rch != NULL; rch = rch->next) {
		if (rch->type == SP_XML_TEXT_NODE) {
			SPString *string;
			string = gtk_type_new (SP_TYPE_STRING);
			(ref) ? ref->next : text->children = sp_object_attach_reref (object, SP_OBJECT (string), NULL);
			string->ly = &text->ly;
			sp_object_invoke_build (SP_OBJECT (string), doc, rch, SP_OBJECT_IS_CLONED (object));
			ref = SP_OBJECT (string);
		} else if ((rch->type == SP_XML_ELEMENT_NODE) && !strcmp (sp_repr_name (rch), "tspan")) {
			SPObject *child;
			child = gtk_type_new (SP_TYPE_TSPAN);
			ref ? ref->next : text->children = sp_object_attach_reref (object, child, NULL);
			sp_object_invoke_build (child, doc, rch, SP_OBJECT_IS_CLONED (object));
			ref = child;
		} else {
			continue;
		}
	}

	/* We can safely set these after building tree, as modified is scheduled anyways (Lauris) */
	sp_text_read_attr (object, "x");
	sp_text_read_attr (object, "y");
	sp_text_read_attr (object, "dx");
	sp_text_read_attr (object, "dy");
	sp_text_read_attr (object, "rotate");

	sp_text_update_immediate_state (text);
}

static void
sp_text_read_attr (SPObject *object, const gchar *attr)
{
	SPText *text;
	const guchar *str;

	text = SP_TEXT (object);

	str = sp_repr_attr (SP_OBJECT_REPR (object), attr);

	/* fixme: Vectors (Lauris) */
	if (!strcmp (attr, "x")) {
		if (!sp_svg_length_read (str, &text->ly.x)) {
			text->ly.x.set = FALSE;
			text->ly.x.computed = 0.0;
		}
		/* fixme: Re-layout it */
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG | SP_TEXT_LAYOUT_MODIFIED_FLAG);
	} else if (!strcmp (attr, "y")) {
		if (!sp_svg_length_read (str, &text->ly.y)) {
			text->ly.y.set = FALSE;
			text->ly.y.computed = 0.0;
		}
		/* fixme: Re-layout it */
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG | SP_TEXT_LAYOUT_MODIFIED_FLAG);
	} else if (!strcmp (attr, "dx")) {
		if (!sp_svg_length_read (str, &text->ly.dx)) {
			text->ly.dx.set = FALSE;
			text->ly.dx.computed = 0.0;
		}
		/* fixme: Re-layout it */
	} else if (!strcmp (attr, "dy")) {
		if (!sp_svg_length_read (str, &text->ly.dy)) {
			text->ly.dy.set = FALSE;
			text->ly.dy.computed = 0.0;
		}
		/* fixme: Re-layout it */
	} else if (strcmp (attr, "rotate") == 0) {
		/* fixme: Implement SVGNumber or something similar (Lauris) */
		text->ly.rotate = (str) ? atof (str) : 0.0;
		text->ly.rotate_set = (str != NULL);
		/* fixme: Re-layout it */
	} else {
		if (SP_OBJECT_CLASS (text_parent_class)->read_attr)
			(SP_OBJECT_CLASS (text_parent_class)->read_attr) (object, attr);
	}
}

static void
sp_text_child_added (SPObject *object, SPRepr *rch, SPRepr *ref)
{
	SPText *text;
	SPItem *item;
	SPObject *och, *prev;

	item = SP_ITEM (object);
	text = SP_TEXT (object);

	if (SP_OBJECT_CLASS (text_parent_class)->child_added)
		SP_OBJECT_CLASS (text_parent_class)->child_added (object, rch, ref);

	/* Search for position reference */
	prev = NULL;

	if (ref != NULL) {
		prev = text->children;
		while (prev && (prev->repr != ref)) prev = prev->next;
	}

	if (rch->type == SP_XML_TEXT_NODE) {
		SPString *string;
		string = gtk_type_new (SP_TYPE_STRING);
		if (prev) {
			prev->next = sp_object_attach_reref (object, SP_OBJECT (string), prev->next);
		} else {
			text->children = sp_object_attach_reref (object, SP_OBJECT (string), text->children);
		}
		string->ly = &text->ly;
		sp_object_invoke_build (SP_OBJECT (string), SP_OBJECT_DOCUMENT (object), rch, SP_OBJECT_IS_CLONED (object));
		och = SP_OBJECT (string);
	} else if ((rch->type == SP_XML_ELEMENT_NODE) && !strcmp (sp_repr_name (rch), "tspan")) {
		SPObject *child;
		child = gtk_type_new (SP_TYPE_TSPAN);
		if (prev) {
			prev->next = sp_object_attach_reref (object, child, prev->next);
		} else {
			text->children = sp_object_attach_reref (object, child, text->children);
		}
		sp_object_invoke_build (child, SP_OBJECT_DOCUMENT (object), rch, SP_OBJECT_IS_CLONED (object));
		och = child;
	} else {
		och = NULL;
	}

	if (och) {
		SPItemView *v;
		NRArenaItem *ac;

		for (v = item->display; v != NULL; v = v->next) {
			ac = sp_item_show (SP_ITEM (och), v->arena);
			if (ac) {
				nr_arena_item_add_child (v->arenaitem, ac, NULL);
				gtk_object_unref (GTK_OBJECT (ac));
			}
		}
	}

	sp_text_request_relayout (text, SP_OBJECT_MODIFIED_FLAG | SP_TEXT_CONTENT_MODIFIED_FLAG);
	/* fixme: Instead of forcing it, do it when needed */
	sp_text_update_immediate_state (text);
}

static void
sp_text_remove_child (SPObject *object, SPRepr *rch)
{
	SPText *text;
	SPObject *prev, *och;

	text = SP_TEXT (object);

	if (SP_OBJECT_CLASS (text_parent_class)->remove_child)
		SP_OBJECT_CLASS (text_parent_class)->remove_child (object, rch);

	prev = NULL;
	och = text->children;
	while (och->repr != rch) {
		prev = och;
		och = och->next;
	}

	if (prev) {
		prev->next = sp_object_detach_unref (object, och);
	} else {
		text->children = sp_object_detach_unref (object, och);
	}

	sp_text_request_relayout (text, SP_OBJECT_MODIFIED_FLAG | SP_TEXT_CONTENT_MODIFIED_FLAG);
	sp_text_update_immediate_state (text);
}

/* fixme: This is wrong, as we schedule relayout every time something changes */

static void
sp_text_modified (SPObject *object, guint flags)
{
	SPText *text;
	SPObject *child;
	GSList *l;
	guint cflags;

	text = SP_TEXT (object);

	cflags = (flags & SP_OBJECT_MODIFIED_CASCADE);
	if (flags & SP_OBJECT_MODIFIED_FLAG) cflags |= SP_OBJECT_PARENT_MODIFIED_FLAG;

	/* Create temporary list of children */
	l = NULL;
	for (child = text->children; child != NULL; child = child->next) {
		sp_object_ref (SP_OBJECT (child), object);
		l = g_slist_prepend (l, child);
	}
	l = g_slist_reverse (l);
	while (l) {
		child = SP_OBJECT (l->data);
		l = g_slist_remove (l, child);
		if (cflags || (SP_OBJECT_FLAGS (child) & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
			sp_object_modified (child, cflags);
		}
		sp_object_unref (SP_OBJECT (child), object);
	}

	if (text->relayout || (flags & (SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG | SP_TEXT_LAYOUT_MODIFIED_FLAG))) {
		/* fixme: It is not nice to have it here, but otherwise children content changes does not work */
		/* fixme: Even now it may not work, as we are delayed */
		/* fixme: So check modification flag everywhere immediate state is used */
		sp_text_update_immediate_state (text);
		sp_text_set_shape (text);
		text->relayout = FALSE;
	}
}

static SPRepr *
sp_text_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPText *text;
	SPObject *child;
	SPRepr *crepr;

	text = SP_TEXT (object);

	if (flags & SP_OBJECT_WRITE_BUILD) {
		GSList *l;
		if (!repr) repr = sp_repr_new ("text");
		l = NULL;
		for (child = text->children; child != NULL; child = child->next) {
			if (SP_IS_TSPAN (child)) {
				crepr = sp_object_invoke_write (child, NULL, flags);
				if (crepr) l = g_slist_prepend (l, crepr);
			} else {
				crepr = sp_xml_document_createTextNode (sp_repr_document (repr), SP_STRING_TEXT (child));
			}
		}
		while (l) {
			sp_repr_add_child (repr, (SPRepr *) l->data, NULL);
			sp_repr_unref ((SPRepr *) l->data);
			l = g_slist_remove (l, l->data);
		}
	} else {
		for (child = text->children; child != NULL; child = child->next) {
			if (SP_IS_TSPAN (child)) {
				sp_object_invoke_write (child, SP_OBJECT_REPR (child), flags);
			} else {
				sp_repr_set_content (SP_OBJECT_REPR (child), SP_STRING_TEXT (child));
			}
		}
	}

	if (text->ly.x.set) sp_repr_set_double_attribute (repr, "x", text->ly.x.computed);
	if (text->ly.y.set) sp_repr_set_double_attribute (repr, "y", text->ly.y.computed);
	if (text->ly.dx.set) sp_repr_set_double_attribute (repr, "dx", text->ly.dx.computed);
	if (text->ly.dy.set) sp_repr_set_double_attribute (repr, "dy", text->ly.dy.computed);
	if (text->ly.rotate_set) sp_repr_set_double_attribute (repr, "rotate", text->ly.rotate);

	if (((SPObjectClass *) (text_parent_class))->write)
		((SPObjectClass *) (text_parent_class))->write (object, repr, flags);

	return repr;
}

static void
sp_text_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform)
{
	SPText *text;
	SPItem *child;
	ArtDRect child_bbox;
	SPObject *o;

	text = SP_TEXT (item);

	bbox->x0 = bbox->y0 = bbox->x1 = bbox->y1 = 0.0;

	for (o = text->children; o != NULL; o = o->next) {
		gdouble a[6];
		child = SP_ITEM (o);
		art_affine_multiply (a, child->affine, transform);
		sp_item_invoke_bbox (child, &child_bbox, a);
		art_drect_union (bbox, bbox, &child_bbox);
	}
}

static NRArenaItem *
sp_text_show (SPItem *item, NRArena *arena)
{
	SPText *text;
	NRArenaItem *ai, *ac, *ar;
	SPItem * child;
	SPObject * o;

	text = SP_TEXT (item);

	ai = nr_arena_item_new (arena, NR_TYPE_ARENA_GROUP);
	nr_arena_group_set_transparent (NR_ARENA_GROUP (ai), FALSE);

	ar = NULL;
	for (o = text->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			ac = sp_item_show (child, arena);
			if (ac) {
				nr_arena_item_add_child (ai, ac, ar);
				ar = ac;
				gtk_object_unref (GTK_OBJECT (ac));
			}
		}
	}

	return ai;
}

static void
sp_text_hide (SPItem *item, NRArena *arena)
{
	SPText *text;
	SPItem * child;
	SPObject * o;

	text = SP_TEXT (item);

	for (o = text->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			sp_item_hide (child, arena);
		}
	}

	if (SP_ITEM_CLASS (text_parent_class)->hide)
		(* SP_ITEM_CLASS (text_parent_class)->hide) (item, arena);
}

static char *
sp_text_description (SPItem * item)
{
	SPText *text;

	text = (SPText *) item;

	/* fixme: */

	return g_strdup (_("Text object"));
}

/* 'lighter' and 'darker' have to be resolved earlier */

gint
sp_text_font_weight_to_gp (gint weight)
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

	return GNOME_FONT_BOOK;
}

static void
sp_text_update_length (SPSVGLength *length, gdouble em, gdouble ex, gdouble scale)
{
	if (length->unit == SP_SVG_UNIT_EM) {
		length->computed = length->value * em;
	} else if (length->unit == SP_SVG_UNIT_EX) {
		length->computed = length->value * ex;
	} else if (length->unit == SP_SVG_UNIT_PERCENT) {
		length->computed = length->value * scale;
	}
}

static void
sp_text_compute_values (SPText *text)
{
	SPObject *child;
	SPStyle *style;
	gdouble i2vp[6], vp2i[6];
	gdouble aw, ah;
	gdouble d;

	style = SP_OBJECT_STYLE (text);

	/* fixme: It is somewhat dangerous, yes (Lauris) */
	/* fixme: And it is terribly slow too (Lauris) */
	/* fixme: In general we want to keep viewport scales around */
	sp_item_i2vp_affine (SP_ITEM (text), i2vp);
	art_affine_invert (vp2i, i2vp);
	aw = sp_distance_d_matrix_d_transform (1.0, vp2i);
	ah = sp_distance_d_matrix_d_transform (1.0, vp2i);
	/* sqrt ((actual_width) ** 2 + (actual_height) ** 2)) / sqrt (2) */
	d = sqrt (aw * aw + ah * ah) * M_SQRT1_2;

	sp_text_update_length (&text->ly.x, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_text_update_length (&text->ly.y, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_text_update_length (&text->ly.dx, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_text_update_length (&text->ly.dy, style->font_size.computed, style->font_size.computed * 0.5, d);

	for (child = text->children; child != NULL; child = child->next) {
		if (SP_IS_TSPAN (child)) {
			SPTSpan *tspan;
			tspan = SP_TSPAN (child);
			style = SP_OBJECT_STYLE (tspan);
			sp_text_update_length (&tspan->ly.x, style->font_size.computed, style->font_size.computed * 0.5, d);
			sp_text_update_length (&tspan->ly.y, style->font_size.computed, style->font_size.computed * 0.5, d);
			sp_text_update_length (&tspan->ly.dx, style->font_size.computed, style->font_size.computed * 0.5, d);
			sp_text_update_length (&tspan->ly.dy, style->font_size.computed, style->font_size.computed * 0.5, d);
		}
	}
}

/* fixme: Do text chunks here (Lauris) */
/* fixme: We'll remove string bbox adjustment and bring it here for the whole chunk (Lauris) */

static void
sp_text_set_shape (SPText *text)
{
	ArtPoint cp;
	SPObject *child;
	gboolean isfirstline, haslast, lastwastspan;
	ArtDRect paintbox;

	/* fixme: Maybe track, whether we have em,ex,% (Lauris) */
	/* fixme: Alternately we can use ::modified to keep everything up-to-date (Lauris) */
	sp_text_compute_values (text);

	/* The logic should be: */
	/* 1. Calculate attributes */
	/* 2. Iterate through children asking them to set shape */

	cp.x = text->ly.x.computed;
	cp.y = text->ly.y.computed;

	isfirstline = TRUE;
	haslast = FALSE;
	lastwastspan = FALSE;

	child = text->children;
	while (child != NULL) {
		SPObject *next, *new;
		SPString *string;
		ArtDRect bbox;
		ArtPoint advance;
		if (SP_IS_TSPAN (child)) {
			SPTSpan *tspan;
			/* fixme: Maybe break this up into 2 pieces - relayout and set shape (Lauris) */
			tspan = SP_TSPAN (child);
			string = SP_TSPAN_STRING (tspan);
			switch (tspan->role) {
			case SP_TSPAN_ROLE_PARAGRAPH:
			case SP_TSPAN_ROLE_LINE:
				if (!isfirstline) {
					if (child->style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
						cp.x -= child->style->font_size.computed;
						cp.y = text->ly.y.computed;
					} else {
						cp.x = text->ly.x.computed;
						cp.y += child->style->font_size.computed;
					}
				}
				/* fixme: This is extremely (EXTREMELY) dangerous (Lauris) */
				/* fixme: Our only hope is to ensure LINE tspans do not request ::modified */
				sp_repr_set_double (SP_OBJECT_REPR (tspan), "x", cp.x);
				sp_repr_set_double (SP_OBJECT_REPR (tspan), "y", cp.y);
				break;
			case SP_TSPAN_ROLE_UNSPECIFIED:
				if (tspan->ly.x.set) {
					cp.x = tspan->ly.x.computed;
				} else {
					tspan->ly.x.computed = cp.x;
				}
				if (tspan->ly.y.set) {
					cp.y = tspan->ly.y.computed;
				} else {
					tspan->ly.y.computed = cp.y;
				}
				break;
			default:
				/* Error */
				break;
			}
		} else {
			string = SP_STRING (child);
		}
		/* Calculate block bbox */
		bbox = string->bbox;
		advance = string->advance;
		for (next = child->next; next != NULL; next = next->next) {
			SPString *string;
			if (SP_IS_TSPAN (next)) {
				SPTSpan *tspan;
				tspan = SP_TSPAN (next);
				if ((tspan->ly.x.set) || (tspan->ly.y.set)) break;
				string = SP_TSPAN_STRING (tspan);
			} else {
				string = SP_STRING (next);
			}
			bbox.x0 = MIN (bbox.x0, string->bbox.x0 + advance.x);
			bbox.y0 = MIN (bbox.y0, string->bbox.y0 + advance.y);
			bbox.x1 = MAX (bbox.x1, string->bbox.x1 + advance.x);
			bbox.y1 = MAX (bbox.y1, string->bbox.y1 + advance.y);
			advance.x += string->advance.x;
			advance.y += string->advance.y;
		}
		new = next;
		/* Calculate starting position */
		switch (child->style->text_anchor.computed) {
		case SP_CSS_TEXT_ANCHOR_START:
			break;
		case SP_CSS_TEXT_ANCHOR_MIDDLE:
			/* Ink midpoint */
			if (child->style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
				cp.y -= (bbox.y0 + bbox.y1) / 2;
			} else {
				cp.x -= (bbox.x0 + bbox.x1) / 2;
			}
			break;
		case SP_CSS_TEXT_ANCHOR_END:
			/* Ink endpoint */
			if (child->style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
				cp.y -= bbox.y1;
			} else {
				cp.x -= bbox.x1;
			}
			break;
		default:
			break;
		}
		/* Set child shapes */
		for (next = child; (next != NULL) && (next != new); next = next->next) {
			if (SP_IS_STRING (next)) {
				sp_string_set_shape (SP_STRING (next), &text->ly, &cp, haslast);
				haslast = TRUE;
				lastwastspan = FALSE;
			} else {
				sp_tspan_set_shape (SP_TSPAN (next), &text->ly, &cp, isfirstline, haslast && !lastwastspan);
				haslast = TRUE;
				lastwastspan = TRUE;
			}
		}
		child = next;
		isfirstline = FALSE;
	}

	sp_item_invoke_bbox (SP_ITEM (text), &paintbox, NR_MATRIX_D_IDENTITY.c);

	for (child = text->children; child != NULL; child = child->next) {
		SPString *string;
		if (SP_IS_TSPAN (child)) {
			string = SP_TSPAN_STRING (child);
		} else {
			string = SP_STRING (child);
		}
		sp_chars_set_paintbox (SP_CHARS (string), &paintbox);
	}
}

static GSList * 
sp_text_snappoints (SPItem *item, GSList *points)
{
	ArtPoint *p;
	gdouble affine[6];

	/* we use corners of item and x,y coordinates of ellipse */
	if (SP_ITEM_CLASS (text_parent_class)->snappoints)
		points = SP_ITEM_CLASS (text_parent_class)->snappoints (item, points);

	p = g_new (ArtPoint,1);
	p->x = SP_TEXT (item)->ly.x.computed;
	p->y = SP_TEXT (item)->ly.y.computed;
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
	gdouble d;
	guchar t[80];

	text = SP_TEXT (item);

	/*
	 * x * t[0] + y * t[2] = TRANS (lyx);
	 * x * t[1] + y * t[3] = TRANS (lyy);
	 * x = (t[3] * TRANS (lyx) - t[2] * TRANS (lyy)) / (t[0] * t[3] - t[1] * t[2]);
	 * y = (t[0] * TRANS (lyy) - t[1] * TRANS (lyx)) / (t[0] * t[3] - t[1] * t[2]);
	 */

	d = transform[0] * transform[3] - transform[1] * transform[2];

	if (fabs (d) > 1e-18) {
		gdouble px, py, x, y;
		SPObject *child;
		px = transform[0] * text->ly.x.computed + transform[2] * text->ly.y.computed + transform[4];
		py = transform[1] * text->ly.x.computed + transform[3] * text->ly.y.computed + transform[5];
		x = (transform[3] * px - transform[2] * py) / d;
		y = (transform[0] * py - transform[1] * px) / d;
		sp_repr_set_double_attribute (repr, "x", x);
		sp_repr_set_double_attribute (repr, "y", y);
		for (child = text->children; child != NULL; child = child->next) {
			if (SP_IS_TSPAN (child)) {
				SPTSpan *tspan;
				tspan = SP_TSPAN (child);
				if (tspan->ly.x.set || tspan->ly.y.set) {
					x = (tspan->ly.x.set) ? tspan->ly.x.computed : text->ly.x.computed;
					y = (tspan->ly.y.set) ? tspan->ly.y.computed : text->ly.y.computed;
					px = transform[0] * x + transform[2] * y + transform[4];
					py = transform[1] * x + transform[3] * y + transform[5];
					x = (transform[3] * px - transform[2] * py) / d;
					y = (transform[0] * py - transform[1] * px) / d;
					sp_repr_set_double_attribute (SP_OBJECT_REPR (tspan), "x", x);
					sp_repr_set_double_attribute (SP_OBJECT_REPR (tspan), "y", y);
				}
			}
		}
	}

	transform[4] = 0.0;
	transform[5] = 0.0;

	if (sp_svg_write_affine (t, 80, transform)) {
		sp_repr_set_attr (repr, "transform", t);
	} else {
		sp_repr_set_attr (repr, "transform", NULL);
	}
}

static void
sp_text_print (SPItem *item, GnomePrintContext *gpc)
{
	SPText *text;
	SPObject *ch;
	gdouble ctm[6];
	ArtDRect pbox, dbox, bbox;

	text = SP_TEXT (item);

	gnome_print_gsave (gpc);

	/* fixme: Think (Lauris) */
	sp_item_invoke_bbox (item, &pbox, NR_MATRIX_D_IDENTITY.c);
	sp_item_bbox_desktop (item, &bbox);
	dbox.x0 = 0.0;
	dbox.y0 = 0.0;
	dbox.x1 = sp_document_width (SP_OBJECT_DOCUMENT (item));
	dbox.y1 = sp_document_height (SP_OBJECT_DOCUMENT (item));
	sp_item_i2d_affine (item, ctm);

	for (ch = text->children; ch != NULL; ch = ch->next) {
		if (SP_IS_TSPAN (ch)) {
			sp_chars_do_print (SP_CHARS (SP_TSPAN (ch)->string), gpc, ctm, &pbox, &dbox, &bbox);
		} else if (SP_IS_STRING (ch)) {
			sp_chars_do_print (SP_CHARS (ch), gpc, ctm, &pbox, &dbox, &bbox);
		}
	}

	gnome_print_grestore (gpc);
}

gchar *
sp_text_get_string_multiline (SPText *text)
{
	SPObject *ch;
	GSList *strs, *l;
	gint len;
	guchar *str, *p;

	strs = NULL;
	for (ch = text->children; ch != NULL; ch = ch->next) {
		if (SP_IS_TSPAN (ch)) {
			SPTSpan *tspan;
			tspan = SP_TSPAN (ch);
			if (tspan->string && SP_STRING (tspan->string)->text) {
				strs = g_slist_prepend (strs, SP_STRING (tspan->string)->text);
			}
		} else if (SP_IS_STRING (ch) && SP_STRING (ch)->text) {
			strs = g_slist_prepend (strs, SP_STRING (ch)->text);
		} else {
			continue;
		}
	}

	len = 0;
	for (l = strs; l != NULL; l = l->next) {
		len += strlen (l->data);
		len += strlen ("\n");
	}

	len += 1;

	strs = g_slist_reverse (strs);

	str = g_new (guchar, len);
	p = str;
	while (strs) {
		memcpy (p, strs->data, strlen (strs->data));
		p += strlen (strs->data);
		strs = g_slist_remove (strs, strs->data);
		if (strs) *p++ = '\n';
	}
	*p++ = '\0';

	return str;
}

void
sp_text_set_repr_text_multiline (SPText *text, const guchar *str)
{
	SPRepr *repr;
	SPStyle *style;
	guchar *content, *p;
	ArtPoint cp;

	g_return_if_fail (text != NULL);
	g_return_if_fail (SP_IS_TEXT (text));

	repr = SP_OBJECT_REPR (text);
	style = SP_OBJECT_STYLE (text);

	if (!str) str = "";
	content = g_strdup (str);

	sp_repr_set_content (SP_OBJECT_REPR (text), NULL);
	while (repr->children) {
		sp_repr_remove_child (repr, repr->children);
	}

	p = content;

	cp.x = text->ly.x.computed;
	cp.y = text->ly.y.computed;

	while (p) {
		SPRepr *rtspan, *rstr;
		guchar *e;
		e = strchr (p, '\n');
		if (e) *e = '\0';
		rtspan = sp_repr_new ("tspan");
		sp_repr_set_double (rtspan, "x", cp.x);
		sp_repr_set_double (rtspan, "y", cp.y);
		if (style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
			/* fixme: real line height */
			/* fixme: What to do with mixed direction tspans? */
			cp.x -= style->font_size.computed;
		} else {
			cp.y += style->font_size.computed;
		}
		sp_repr_set_attr (rtspan, "sodipodi:role", "line");
		rstr = sp_xml_document_createTextNode (sp_repr_document (repr), p);
		sp_repr_add_child (rtspan, rstr, NULL);
		sp_repr_append_child (repr, rtspan);
		p = (e) ? e + 1 : NULL;
	}

	g_free (content);

	/* fixme: Calculate line positions (Lauris) */
}

SPCurve *
sp_text_normalized_bpath (SPText *text)
{
	SPObject *child;
	GSList *cc;
	SPCurve *curve;

	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (SP_IS_TEXT (text), NULL);

	cc = NULL;
	for (child = text->children; child != NULL; child = child->next) {
		SPCurve *c;
		if (SP_IS_STRING (child)) {
			c = sp_chars_normalized_bpath (SP_CHARS (child));
		} else if (SP_IS_TSPAN (child)) {
			SPTSpan *tspan;
			tspan = SP_TSPAN (child);
			c = sp_chars_normalized_bpath (SP_CHARS (tspan->string));
		} else {
			c = NULL;
		}
		if (c) cc = g_slist_prepend (cc, c);
	}

	cc = g_slist_reverse (cc);

	curve = sp_curve_concat (cc);

	while (cc) {
		sp_curve_unref ((SPCurve *) cc->data);
		cc = g_slist_remove (cc, cc->data);
	}

	return curve;
}

SPTSpan *
sp_text_append_line (SPText *text)
{
	SPRepr *rtspan, *rstring;
	SPObject *child;
	SPStyle *style;
	ArtPoint cp;

	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (SP_IS_TEXT (text), NULL);

	style = SP_OBJECT_STYLE (text);

	cp.x = text->ly.x.computed;
	cp.y = text->ly.y.computed;

	for (child = text->children; child != NULL; child = child->next) {
		if (SP_IS_TSPAN (child)) {
			SPTSpan *tspan;
			tspan = SP_TSPAN (child);
			if (tspan->role == SP_TSPAN_ROLE_LINE) {
				cp.x = tspan->ly.x.computed;
				cp.y = tspan->ly.y.computed;
			}
		}
	}

	/* Create <tspan> */
	rtspan = sp_repr_new ("tspan");
	if (style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
		/* fixme: real line height */
		/* fixme: What to do with mixed direction tspans? */
		sp_repr_set_double (rtspan, "x", cp.x - style->font_size.computed);
		sp_repr_set_double (rtspan, "y", cp.y);
	} else {
		sp_repr_set_double (rtspan, "x", cp.x);
		sp_repr_set_double (rtspan, "y", cp.y + style->font_size.computed);
	}
	sp_repr_set_attr (rtspan, "sodipodi:role", "line");

	/* Create TEXT */
	rstring = sp_xml_document_createTextNode (sp_repr_document (rtspan), "");
	sp_repr_add_child (rtspan, rstring, NULL);
	sp_repr_unref (rstring);
	/* Append to text */
	sp_repr_append_child (SP_OBJECT_REPR (text), rtspan);
	sp_repr_unref (rtspan);

	return (SPTSpan *) sp_document_lookup_id (SP_OBJECT_DOCUMENT (text), sp_repr_attr (rtspan, "id"));
}

static void
sp_text_update_immediate_state (SPText *text)
{
	SPObject *child;
	guint start;

	start = 0;
	for (child = text->children; child != NULL; child = child->next) {
		SPString *string;
		if (SP_IS_TSPAN (child)) {
			string = SP_TSPAN_STRING (child);
		} else {
			string = SP_STRING (child);
		}
		string->start = start;
		string->length = (string->text) ? g_utf8_strlen (string->text, -1) : 0;
		start += string->length;
		/* Count newlines as well */
		if (child->next) start += 1;
	}
}

static void
sp_text_request_relayout (SPText *text, guint flags)
{
	text->relayout = TRUE;

	sp_object_request_modified (SP_OBJECT (text), flags);
}

/* fixme: Think about these (Lauris) */

gint
sp_text_get_length (SPText *text)
{
	SPObject *child;
	SPString *string;

	g_return_val_if_fail (text != NULL, 0);
	g_return_val_if_fail (SP_IS_TEXT (text), 0);

	if (!text->children) return 0;

	child = text->children;
	while (child->next) child = child->next;

	if (SP_IS_STRING (child)) {
		string = SP_STRING (child);
	} else {
		string = SP_TSPAN_STRING (child);
	}

	return string->start + string->length;
}

gint
sp_text_append (SPText *text, const guchar *utf8)
{
	SPObject *child;
	SPString *string;
	const guchar *content;
	gint clen, ulen, cchars, uchars;
	guchar b[1024], *p;

	g_return_val_if_fail (text != NULL, -1);
	g_return_val_if_fail (SP_IS_TEXT (text), -1);
	g_return_val_if_fail (utf8 != NULL, -1);

	if (!text->children) {
		SPRepr *rtspan, *rstring;
		/* Create <tspan> */
		rtspan = sp_repr_new ("tspan");
		/* Create TEXT */
		rstring = sp_xml_document_createTextNode (sp_repr_document (rtspan), "");
		sp_repr_add_child (rtspan, rstring, NULL);
		sp_repr_unref (rstring);
		/* Add string */
		sp_repr_add_child (SP_OBJECT_REPR (text), rtspan, NULL);
		sp_repr_unref (rtspan);
		g_assert (text->children);
	}

	child = text->children;
	while (child->next) child = child->next;
	if (SP_IS_STRING (child)) {
		string = SP_STRING (child);
	} else {
		string = SP_TSPAN_STRING (child);
	}
		
	content = sp_repr_content (SP_OBJECT_REPR (string));

	clen = (content) ? strlen (content) : 0;
	cchars = (content) ? g_utf8_strlen (content, clen) : 0;

	ulen = (utf8) ? strlen (utf8) : 0;
	uchars = (utf8) ? g_utf8_strlen (utf8, ulen) : 0;

	if (ulen < 1) return cchars;

	if ((clen + ulen) < 1024) {
		p = b;
	} else {
		p = g_new (guchar, clen + ulen + 1);
	}

	if (clen > 0) memcpy (p, content, clen);
	if (ulen > 0) memcpy (p + clen, utf8, ulen);
	*(p + clen + ulen) = '\0';

	sp_repr_set_content (SP_OBJECT_REPR (string), p);

	if (p != b) g_free (p);

	return string->start + cchars + uchars;
}

/* Returns start position */

gint 
sp_text_delete (SPText *text, gint start, gint end)
{
	SPObject *schild, *echild;

	g_return_val_if_fail (text != NULL, -1);
	g_return_val_if_fail (SP_IS_TEXT (text), -1);
	g_return_val_if_fail (start >= 0, -1);
	g_return_val_if_fail (end >= start, -1);

	if (!text->children) return 0;
	if (start == end) return start;

	schild = sp_text_get_child_by_position (text, start);
	echild = sp_text_get_child_by_position (text, end);

	if (schild != echild) {
		SPString *sstring, *estring;
		SPObject *child;
		guchar *utf8, *sp, *ep;
		GSList *cl;
		/* Easy case */
		sstring = SP_TEXT_CHILD_STRING (schild);
		estring = SP_TEXT_CHILD_STRING (echild);
		sp = g_utf8_offset_to_pointer (sstring->text, start - sstring->start);
		ep = g_utf8_offset_to_pointer (estring->text, end - estring->start);
		utf8 = g_new (guchar, (sp - sstring->text) + strlen (ep) + 1);
		if (sp > sstring->text) memcpy (utf8, sstring->text, sp - sstring->text);
		memcpy (utf8 + (sp - sstring->text), ep, strlen (ep) + 1);
		sp_repr_set_content (SP_OBJECT_REPR (sstring), utf8);
		g_free (utf8);
		/* Delete nodes */
		cl = NULL;
		for (child = schild->next; child != echild; child = child->next) {
			cl = g_slist_prepend (cl, SP_OBJECT_REPR (child));
		}
		cl = g_slist_prepend (cl, SP_OBJECT_REPR (child));
		while (cl) {
			sp_repr_unparent ((SPRepr *) cl->data);
			cl = g_slist_remove (cl, cl->data);
		}
	} else {
		SPString *string;
		gchar *sp, *ep;
		/* Easy case */
		string = SP_TEXT_CHILD_STRING (schild);
		sp = g_utf8_offset_to_pointer (string->text, start - string->start);
		ep = g_utf8_offset_to_pointer (string->text, end - string->start);
		memmove (sp, ep, strlen (ep) + 1);
		sp_repr_set_content (SP_OBJECT_REPR (string), string->text);
	}

	return start;
}

void
sp_text_get_cursor_coords (SPText *text, gint position, ArtPoint *p0, ArtPoint *p1)
{
	SPObject *child;
	SPString *string;
	gfloat x, y;

	child = sp_text_get_child_by_position (text, position);
	string = SP_TEXT_CHILD_STRING (child);

	if (!string->p) {
		x = string->ly->x.computed;
		y = string->ly->y.computed;
	} else {
		x = string->p[position - string->start].x;
		y = string->p[position - string->start].y;
	}

	if (child->style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
		p0->x = x - child->style->font_size.computed / 2.0;
		p0->y = y;
		p1->x = x + child->style->font_size.computed / 2.0;
		p1->y = y;
	} else {
		p0->x = x;
		p0->y = y - child->style->font_size.computed;
		p1->x = x;
		p1->y = y;
	}
}

static SPObject *
sp_text_get_child_by_position (SPText *text, gint pos)
{
	SPObject *child;

	for (child = text->children; child && child->next; child = child->next) {
		SPString *string;
		if (SP_IS_STRING (child)) {
			string = SP_STRING (child);
		} else {
			string = SP_TSPAN_STRING (child);
		}
		if (pos <= (string->start + string->length)) return child;
	}

	return child;
}

