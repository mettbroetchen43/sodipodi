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
#include "xml/repr-private.h"
#include "svg/svg.h"
#include "display/nr-arena-group.h"
#include "display/nr-arena-glyphs.h"
#include "document.h"
#include "style.h"

#include "sp-text.h"

/* fixme: Better place for these */
static gint sp_text_font_weight_to_gp (SPCSSFontWeight weight);
static gboolean sp_text_font_italic_to_gp (SPCSSFontStyle style);

/* SPString */

static void sp_string_class_init (SPStringClass *class);
static void sp_string_init (SPString *string);
static void sp_string_destroy (GtkObject *object);

static void sp_string_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_string_read_content (SPObject *object);
static void sp_string_modified (SPObject *object, guint flags);

static void sp_string_set_shape (SPString *string, SPLayoutData *ly, ArtPoint *cp);

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
}

static void
sp_string_destroy (GtkObject *object)
{
	SPString *string;

	string = SP_STRING (object);

	if (string->text) g_free (string->text);

	GTK_OBJECT_CLASS (string_parent_class)->destroy (object);
}

static void
sp_string_build (SPObject *object, SPDocument *doc, SPRepr *repr)
{
	SP_OBJECT_CLASS (string_parent_class)->build (object, doc, repr);

	sp_string_read_content (object);
}

/* fixme: We have to notify parents that we changed */

static void
sp_string_read_content (SPObject *object)
{
	SPString *string;
	const guchar *t;

	string = SP_STRING (object);

	if (string->text) g_free (string->text);
	t = sp_repr_content (object->repr);
	string->text = (t) ? g_strdup (t) : NULL;

	/* Is this correct? I think so (Lauris) */
	/* Virtual method will be invoked BEFORE signal, so we can update there */
	sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
}

/*
 * fixme: I am not sure
 * But - thinking - signal is emitted after virtual method
 * So - everything seems OK
 */

static void
sp_string_modified (SPObject *object, guint flags)
{
	/* We do not need it here, as set_shape is cascaded for text classes */
#if 0
	if ((flags & SP_OBJECT_MODIFIED_FLAG) || (flags & SP_OBJECT_STYLE_MODIFIED_FLAG)) {
		sp_string_set_shape (SP_STRING (object));
	}
#endif
}

/* fixme: Should values be parsed by parent? */

static void
sp_string_set_shape (SPString *string, SPLayoutData *ly, ArtPoint *cp)
{
#if 1
	SPChars *chars;
	SPStyle *style;
	const GnomeFontFace *face;
	gdouble size;
	guint glyph;
	gdouble x, y;
	gdouble a[6];
	gdouble w;
	const guchar *p;

	chars = SP_CHARS (string);
	style = SP_OBJECT_STYLE (string);

	sp_chars_clear (chars);

	face = gnome_font_unsized_closest (style->text->font_family.value,
					   sp_text_font_weight_to_gp (style->text->font_weight),
					   sp_text_font_italic_to_gp (style->text->font_style));
	size = style->text->font_size;

	/* fixme: Find a way how to manipulate these */
	x = cp->x;
	y = cp->y;
	g_print ("Drawing string (%s) at %g %g\n", string->text, x, y);

	art_affine_scale (a, size * 0.001, size * -0.001);
	if (string->text) {
		for (p = string->text; p && *p; p = g_utf8_next_char (p)) {
			gunichar u;
			u = g_utf8_get_char (p);
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

	cp->x = x;
	cp->y = y;
	g_print ("Finished string (%s) at %g %g\n", string->text, x, y);
#else
	g_warning ("sp_string_set_shape: Not implemented");
#endif
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

static void sp_tspan_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform);
static NRArenaItem *sp_tspan_show (SPItem *item, NRArena *arena);
static void sp_tspan_hide (SPItem *item, NRArena *arena);
static void sp_tspan_print (SPItem *item, GnomePrintContext *gpc);

static void sp_tspan_set_shape (SPTSpan *tspan, SPLayoutData *ly, ArtPoint *cp);

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

	item_class->bbox = sp_tspan_bbox;
	item_class->show = sp_tspan_show;
	item_class->hide = sp_tspan_hide;
	item_class->print = sp_tspan_print;
}

static void
sp_tspan_init (SPTSpan *tspan)
{
	/* fixme: Initialize layout */
	tspan->ly.x = tspan->ly.y = 0.0;
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
	const guchar *astr;
	const SPUnit *unit;

	tspan = SP_TSPAN (object);

	astr = sp_repr_attr (SP_OBJECT_REPR (object), attr);

	if (strcmp (attr, "x") == 0) {
		if (astr) tspan->ly.x = sp_svg_read_length (&unit, astr, 0.0);
		tspan->ly.x_set = (astr != NULL);
		/* fixme: Re-layout it */
		return;
	} else if (strcmp (attr, "y") == 0) {
		if (astr) tspan->ly.y = sp_svg_read_length (&unit, astr, 0.0);
		tspan->ly.y_set = (astr != NULL);
		/* fixme: Re-layout it */
		return;
	} else if (strcmp (attr, "dx") == 0) {
		if (astr) tspan->ly.dx = sp_svg_read_length (&unit, astr, 0.0);
		tspan->ly.dx_set = (astr != NULL);
		/* fixme: Re-layout it */
		return;
	} else if (strcmp (attr, "dy") == 0) {
		if (astr) tspan->ly.dy = sp_svg_read_length (&unit, astr, 0.0);
		tspan->ly.dy_set = (astr != NULL);
		/* fixme: Re-layout it */
		return;
	} else if (strcmp (attr, "rotate") == 0) {
		if (astr) tspan->ly.rotate = sp_svg_read_length (&unit, astr, 0.0);
		tspan->ly.rotate_set = (astr != NULL);
		/* fixme: Re-layout it */
		return;
	} else if (!strcmp (attr, "sodipodi:role")) {
		if (astr && !strcmp (astr, "line")) {
			tspan->role = SP_TSPAN_ROLE_LINE;
		} else {
			tspan->role = SP_TSPAN_ROLE_UNKNOWN;
		}
	}

	if (SP_OBJECT_CLASS (tspan_parent_class)->read_attr)
		(SP_OBJECT_CLASS (tspan_parent_class)->read_attr) (object, attr);
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
sp_tspan_print (SPItem *item, GnomePrintContext *gpc)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (item);

	if (tspan->string) {
		sp_item_print (SP_ITEM (tspan->string), gpc);
	}
}

static void
sp_tspan_set_shape (SPTSpan *tspan, SPLayoutData *ly, ArtPoint *cp)
{
	if (tspan->ly.x_set) cp->x = tspan->ly.x;
	if (tspan->ly.y_set) cp->y = tspan->ly.y;

	sp_string_set_shape (SP_STRING (tspan->string), &tspan->ly, cp);
}

/* SPText */

static void sp_text_class_init (SPTextClass *class);
static void sp_text_init (SPText *text);
static void sp_text_destroy (GtkObject *object);

static void sp_text_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_text_read_attr (SPObject * object, const gchar * attr);
static void sp_text_child_added (SPObject *object, SPRepr *rch, SPRepr *ref);
static void sp_text_remove_child (SPObject *object, SPRepr *rch);
static void sp_text_modified (SPObject *object, guint flags);

static void sp_text_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform);
static NRArenaItem *sp_text_show (SPItem *item, NRArena *arena);
static void sp_text_hide (SPItem *item, NRArena *arena);
static char * sp_text_description (SPItem * item);
static GSList * sp_text_snappoints (SPItem * item, GSList * points);
static void sp_text_write_transform (SPItem *item, SPRepr *repr, gdouble *transform);

static void sp_text_set_shape (SPText * text);

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

	item_class->bbox = sp_text_bbox;
	item_class->show = sp_text_show;
	item_class->hide = sp_text_hide;
	item_class->description = sp_text_description;
	item_class->snappoints = sp_text_snappoints;
	item_class->write_transform = sp_text_write_transform;
}

static void
sp_text_init (SPText *text)
{
	/* fixme: Initialize layout */
	text->ly.x = text->ly.y = text->ly.dx = text->ly.dy = 0.0;
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

static void
sp_text_build (SPObject *object, SPDocument *doc, SPRepr *repr)
{
	SPText *text;
	SPObject *ref;
	SPRepr *rch;

	text = SP_TEXT (object);

	if (SP_OBJECT_CLASS (text_parent_class)->build)
		SP_OBJECT_CLASS (text_parent_class)->build (object, doc, repr);

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
}

static void
sp_text_read_attr (SPObject *object, const gchar *attr)
{
	SPText *text;
	const guchar *astr;
	const SPUnit *unit;

	text = SP_TEXT (object);

	astr = sp_repr_attr (SP_OBJECT_REPR (object), attr);

	if (strcmp (attr, "x") == 0) {
		text->ly.x = sp_svg_read_length (&unit, astr, 0.0);
		text->ly.x_set = (astr != NULL);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	}
	if (strcmp (attr, "y") == 0) {
		text->ly.y = sp_svg_read_length (&unit, astr, 0.0);
		text->ly.y_set = (astr != NULL);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	}
	if (strcmp (attr, "dx") == 0) {
		text->ly.dx = sp_svg_read_length (&unit, astr, 0.0);
		text->ly.dx_set = (astr != NULL);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	}
	if (strcmp (attr, "dy") == 0) {
		text->ly.dy = sp_svg_read_length (&unit, astr, 0.0);
		text->ly.dy_set = (astr != NULL);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	}
	if (strcmp (attr, "rotate") == 0) {
		text->ly.rotate = sp_svg_read_length (&unit, astr, 0.0);
		text->ly.rotate_set = (astr != NULL);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	}

	if (SP_OBJECT_CLASS (text_parent_class)->read_attr)
		(SP_OBJECT_CLASS (text_parent_class)->read_attr) (object, attr);
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

	sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
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

	sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
}

static void
sp_text_modified (SPObject *object, guint flags)
{
	SPText *text;
	SPObject *child;
	GSList *l;

	text = SP_TEXT (object);

	if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	flags &= SP_OBJECT_MODIFIED_CASCADE;

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
		if (flags || (SP_OBJECT_FLAGS (child) & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
			sp_object_modified (child, flags);
		}
		sp_object_unref (SP_OBJECT (child), object);
	}

	sp_text_set_shape (text);
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
	ArtPoint cp;
	SPObject *child;

	/* The logic should be: */
	/* 1. Calculate attributes */
	/* 2. Iterate through children asking them to set shape */

	cp.x = cp.y = 0.0;
	if (text->ly.x_set) cp.x = text->ly.x;
	if (text->ly.y_set) cp.y = text->ly.y;

	for (child = text->children; child != NULL; child = child->next) {
		if (SP_IS_STRING (child)) {
			sp_string_set_shape (SP_STRING (child), &text->ly, &cp);
		} else {
			sp_tspan_set_shape (SP_TSPAN (child), &text->ly, &cp);
		}
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
	p->x = SP_TEXT (item)->ly.x;
	p->y = SP_TEXT (item)->ly.y;
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
		px = transform[0] * text->ly.x + transform[2] * text->ly.y + transform[4];
		py = transform[1] * text->ly.x + transform[3] * text->ly.y + transform[5];
		x = (transform[3] * px - transform[2] * py) / d;
		y = (transform[0] * py - transform[1] * px) / d;
		sp_repr_set_double_attribute (repr, "x", x);
		sp_repr_set_double_attribute (repr, "y", y);
		for (child = text->children; child != NULL; child = child->next) {
			if (SP_IS_TSPAN (child)) {
				SPTSpan *tspan;
				tspan = SP_TSPAN (child);
				if (tspan->ly.x_set || tspan->ly.y_set) {
					x = (tspan->ly.x_set) ? tspan->ly.x : text->ly.x;
					y = (tspan->ly.y_set) ? tspan->ly.y : text->ly.y;
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

	if ((fabs (transform[0] - 1.0) > 1e-9) ||
	    (fabs (transform[3] - 1.0) > 1e-9) ||
	    (fabs (transform[1]) > 1e-9) ||
	    (fabs (transform[2]) > 1e-9)) {
		guchar str[80];
		sp_svg_write_affine (str, 80, transform);
		sp_repr_set_attr (repr, "transform", str);
	} else {
		sp_repr_set_attr (repr, "transform", NULL);
	}
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
	guchar *content, *p;

	g_return_if_fail (text != NULL);
	g_return_if_fail (SP_IS_TEXT (text));

	repr = SP_OBJECT_REPR (text);

	while (repr->children) {
		sp_repr_remove_child (repr, repr->children);
	}

	if (!str) str = "";

	content = g_strdup (str);
	p = content;

	while (p) {
		SPRepr *rch, *rstr;
		guchar *e;
		e = strchr (p, '\n');
		if (e) *e = '\0';
		rch = sp_repr_new ("tspan");
		rstr = sp_xml_document_createTextNode (sp_repr_document (repr), p);
		sp_repr_add_child (rch, rstr, NULL);
		sp_repr_append_child (repr, rch);
		p = (e) ? e + 1 : NULL;
	}

	g_free (content);
}

SPItem *
sp_text_get_last_string (SPText *text)
{
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (SP_IS_TEXT (text), NULL);

	if (text->children) {
		SPObject *child;
		child = text->children;
		while (child->next) child = child->next;
		if (SP_IS_TSPAN (child)) {
			return (SPItem *) SP_TSPAN (child)->string;
		} else if (SP_IS_STRING (child)) {
			return (SPItem *) child;
		}
	}

	return NULL;
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

	cp.x = text->ly.x;
	cp.y = text->ly.y;

	for (child = text->children; child != NULL; child = child->next) {
		if (SP_IS_TSPAN (child)) {
			SPTSpan *tspan;
			tspan = SP_TSPAN (child);
			if (tspan->role == SP_TSPAN_ROLE_LINE) {
				cp.x = tspan->ly.x;
				cp.y = tspan->ly.y;
			}
		}
	}

	/* Create <tspan> */
	rtspan = sp_repr_new ("tspan");
	if (style->text->writing_mode.value == SP_CSS_WRITING_MODE_TB) {
		/* fixme: real line height */
		/* fixme: What to do with mixed direction tspans? */
		sp_repr_set_double (rtspan, "x", cp.x - style->text->font_size);
		sp_repr_set_double (rtspan, "y", cp.y);
	} else {
		sp_repr_set_double (rtspan, "x", cp.x);
		sp_repr_set_double (rtspan, "y", cp.y + style->text->font_size);
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


