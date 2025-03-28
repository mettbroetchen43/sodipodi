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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <libnr/nr-rect.h>
#include <libnr/nr-matrix.h>
#include <libnrtype/nr-type-directory.h>
#include <libnrtype/nr-font.h>
#include <libnrtype/nr-glyphs.h>
#include <libarikkei/arikkei-strlib.h>

#include "helper/sp-intl.h"
#include "xml/repr-private.h"
#include "svg/svg.h"
#include "display/nr-arena-group.h"
#include "display/nr-arena-glyphs.h"
#include "attributes.h"
#include "document.h"
#include "style.h"
/* For versioning */
#include "sp-root.h"

#include "sp-text.h"

/* fixme: */
unsigned int
sp_repr_set_content_ucs2 (SPRepr *repr, const NRUShort *newcontent)
{
	char *utf8;
	unsigned int retval;

	utf8 = g_utf16_to_utf8 (newcontent, -1, NULL, NULL, NULL);

	retval = sp_repr_set_content (repr, utf8);

	g_free (utf8);

	return retval;
}

static guchar *sp_text_font_style_to_lookup (SPStyle *style);

static void sp_text_update_length (SPSVGLength *length, gdouble em, gdouble ex, gdouble scale);

/* SPString */

static void sp_string_class_init (SPStringClass *class);
static void sp_string_init (SPString *string);

static void sp_string_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_string_release (SPObject *object);
static void sp_string_read_content (SPObject *object);
static void sp_string_update (SPObject *object, SPCtx *ctx, unsigned int flags);

static void sp_string_calculate_dimensions (SPString *string);
static void sp_string_set_shape (SPString *string, SPLayoutData *ly, ArtPoint *cp, gboolean *inspace);

static SPCharsClass *string_parent_class;

GType
sp_string_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPStringClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_string_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPString),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_string_init,
		};
		type = g_type_register_static (SP_TYPE_CHARS, "SPString", &info, 0);
	}
	return type;
}

static void
sp_string_class_init (SPStringClass *class)
{
	SPObjectClass *sp_object_class;
	SPItemClass *item_class;

	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	string_parent_class = g_type_class_ref (SP_TYPE_CHARS);

	sp_object_class->build = sp_string_build;
	sp_object_class->release = sp_string_release;
	sp_object_class->read_content = sp_string_read_content;
	sp_object_class->update = sp_string_update;
}

static void
sp_string_init (SPString *string)
{
	/* Nothing here */
}

static void
sp_string_build (SPObject *object, SPDocument *doc, SPRepr *repr)
{
	if (((SPObjectClass *) string_parent_class)->build)
		((SPObjectClass *) string_parent_class)->build (object, doc, repr);

	sp_string_read_content (object);

	/* fixme: This can be waste here, but ensures loaded documents are up-to-date */
	sp_string_calculate_dimensions (SP_STRING (object));
}

static void
sp_string_release (SPObject *object)
{
	SPString *string;

	string = SP_STRING (object);

	if (string->uchars) nr_free (string->uchars);
	if (string->pgl) nr_pgl_free (string->pgl);

	if (((SPObjectClass *) string_parent_class)->release)
		((SPObjectClass *) string_parent_class)->release (object);
}

/* fixme: We have to notify parents that we changed */

static void
sp_string_read_content (SPObject *object)
{
	SPString *string;
	const guchar *t;

	string = SP_STRING (object);

	t = sp_repr_get_content (object->repr);

	/* New content stuff */
	if (string->uchars) free (string->uchars);
	string->uchars = NULL;
	string->ulen = 0;
	if (t) {
		const unsigned char *p;
		unsigned int unival;
		unsigned int preserve, inspace, intext;
		int pos;

		preserve = SP_STRING_IS_SPACE_PRESERVE (string);
		inspace = FALSE;
		intext = FALSE;
		for (p = t; p && *p; p = g_utf8_next_char (p)) {
			unival = g_utf8_get_char (p);
			if (g_unichar_isspace (unival)) {
				if (preserve) string->ulen += 1;
				if (unival == ' ' || unival == '\t') inspace = TRUE;
			} else {
				/* Append space if needed */
				if (!preserve && inspace && intext) string->ulen += 1;
				/* Append char */
				string->ulen += 1;
				inspace = FALSE;
				intext = TRUE;
			}
		}
		if (!preserve && inspace && intext) string->ulen += 1;

		string->uchars = nr_new (NRUShort, string->ulen + 1);
		string->spstart = FALSE;
		string->spend = FALSE;
		inspace = FALSE;
		intext = FALSE;
		pos = 0;
		for (p = t; p && *p; p = g_utf8_next_char (p)) {
			unival = g_utf8_get_char (p);
			if (g_unichar_isspace (unival)) {
				if (preserve) string->uchars[pos++] = 32;
				if (!intext) string->spstart = TRUE;
				if (unival == ' ' || unival == '\t') inspace = TRUE;
			} else {
				/* Append space if needed */
				if (!preserve && inspace && intext) string->uchars[pos++] = 32;
				/* Append char */
				string->uchars[pos++] = unival;
				inspace = FALSE;
				intext = TRUE;
			}
		}
		if (!preserve && inspace && intext) string->uchars[pos++] = 32;

		string->uchars[pos] = '\0';
		string->spend = inspace;
	}

	/* Is this correct? I think so (Lauris) */
	/* Virtual method will be invoked BEFORE signal, so we can update there */
	sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
}

/* This happen before parent does layouting but after styles have been set */
/* So it is the right place to calculate untransformed string dimensions */

static void
sp_string_update (SPObject *object, SPCtx *ctx, unsigned int flags)
{
	if (((SPObjectClass *) string_parent_class)->update)
		((SPObjectClass *) string_parent_class)->update (object, ctx, flags);

	if (flags & (SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_MODIFIED_FLAG)) {
		/* Parent style or we ourselves changed, so recalculate */
		sp_string_calculate_dimensions (SP_STRING (object));
	}
}

/* Vertical metric simulator */

static void
sp_string_calculate_dimensions (SPString *string)
{
	SPStyle *style;
	NRTypeFace *face;
	NRFont *font;
	gdouble size;
	unsigned int metrics;
	NRMatrixF gtr;

	string->bbox.x0 = string->bbox.y0 = 1e18;
	string->bbox.x1 = string->bbox.y1 = -1e18;
	string->advance.x = 0.0;
	string->advance.y = 0.0;

	style = SP_OBJECT_STYLE (SP_OBJECT_PARENT (string));
	/* fixme: Adjusted value (Lauris) */
	size = style->font_size.computed;
	face = nr_type_directory_lookup_fuzzy (style->text->font_family.value, sp_text_font_style_to_lookup (style),
                                               style->font_weight.computed,
                                               style->font_style.computed);
	if (style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
		metrics = NR_TYPEFACE_METRICS_VERTICAL;
	} else {
		metrics = NR_TYPEFACE_METRICS_HORIZONTAL;
	}
	font = nr_font_new_default (face, metrics, size);

	/* fixme: SPChars should do this upright instead */
	nr_matrix_f_set_scale (&gtr, 1.0, -1.0);
	if (string->pgl) nr_pgl_free (string->pgl);

	string->pgl = nr_pgl_new_from_string (string->uchars, string->ulen, font, &gtr);
	string->bbox = string->pgl->area;
	string->advance = string->pgl->advance;

	nr_font_unref (font);
	nr_typeface_unref (face);

	if (nr_rect_f_test_empty (&string->bbox)) {
		string->bbox.x0 = string->bbox.y0 = 0.0;
		string->bbox.x1 = string->bbox.y1 = 0.0;
	}

}

/* fixme: Should values be parsed by parent? */

static void
sp_string_set_shape (SPString *string, SPLayoutData *ly, ArtPoint *cp, gboolean *pinspace)
{
	SPChars *chars;
	SPStyle *style;
	gint len;
	NRMatrixF a;
	gdouble x, y;
	gint i;

	chars = SP_CHARS (string);
	style = SP_OBJECT_STYLE (SP_OBJECT_PARENT (string));

	sp_chars_clear (chars);

	if (!string->uchars || !*string->uchars) return;
	len = arikkei_ucs2_strlen (string->uchars);
	if (!len) return;

	/* fixme: Find a way how to manipulate these */
	x = cp->x;
	y = cp->y;

	/* fixme: SPChars should do this upright instead */
	nr_matrix_f_set_scale (&a, 1.0, -1.0);

	g_assert (string->pgl);
	nr_pgl_set_origin (string->pgl, x, y);

	for (i = 0; i < string->pgl->length; i++) {
		NRFont *font;
		NRPosGlyph *pglyph;

		/* fixme: we should use rasterfont instead */
		font = string->pgl->rfont->font;
		pglyph = &string->pgl->glyphs[i];
		a.c[4] = pglyph->x;
		a.c[5] = pglyph->y;

		sp_chars_add_element (chars, pglyph->glyph, font, &a);
	}

	cp->x += string->pgl->advance.x;
	cp->y -= string->pgl->advance.y;
}

/* SPTSpan */

static void sp_tspan_class_init (SPTSpanClass *class);
static void sp_tspan_init (SPTSpan *tspan);

static void sp_tspan_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_tspan_release (SPObject *object);
static void sp_tspan_set (SPObject *object, unsigned int key, const unsigned char *value);
static void sp_tspan_child_added (SPObject *object, SPRepr *rch, SPRepr *ref);
static unsigned int sp_tspan_remove_child (SPObject *object, SPRepr *rch);
static void sp_tspan_update (SPObject *object, SPCtx *ctx, guint flags);
static void sp_tspan_modified (SPObject *object, unsigned int flags);
static SPRepr *sp_tspan_write (SPObject *object, SPRepr *repr, guint flags);

static void sp_tspan_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags);
static NRArenaItem *sp_tspan_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags);
static void sp_tspan_hide (SPItem *item, unsigned int key);

static void sp_tspan_set_shape (SPTSpan *tspan, SPLayoutData *ly, ArtPoint *cp, gboolean firstline, gboolean *inspace);

static SPItemClass *tspan_parent_class;

GType
sp_tspan_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPTSpanClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_tspan_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPTSpan),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_tspan_init,
		};
		type = g_type_register_static (SP_TYPE_ITEM, "SPTSpan", &info, 0);
	}
	return type;
}

static void
sp_tspan_class_init (SPTSpanClass *class)
{
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	tspan_parent_class = g_type_class_ref (SP_TYPE_ITEM);

	sp_object_class->build = sp_tspan_build;
	sp_object_class->release = sp_tspan_release;
	sp_object_class->set = sp_tspan_set;
	sp_object_class->child_added = sp_tspan_child_added;
	sp_object_class->remove_child = sp_tspan_remove_child;
	sp_object_class->update = sp_tspan_update;
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
	tspan->ly.linespacing = 1.0;
}

static void
sp_tspan_build (SPObject *object, SPDocument *doc, SPRepr *repr)
{
	SPTSpan *tspan;
	SPString *string;
	SPRepr *rch;

	tspan = SP_TSPAN (object);

	if (((SPObjectClass *) tspan_parent_class)->build)
		((SPObjectClass *) tspan_parent_class)->build (object, doc, repr);

	sp_object_read_attr (object, "x");
	sp_object_read_attr (object, "y");
	sp_object_read_attr (object, "dx");
	sp_object_read_attr (object, "dy");
	sp_object_read_attr (object, "rotate");
	sp_object_read_attr (object, "sodipodi:role");

	for (rch = sp_repr_get_children (repr); rch != NULL; rch = sp_repr_get_next (rch)) {
		if (sp_repr_is_text (rch)) break;
		if (sp_repr_is_cdata (rch)) break;
	}

	if (!rch) {
		rch = sp_xml_document_createTextNode (sp_repr_get_doc (repr), "");
		sp_repr_add_child (repr, rch, NULL);
	}

	/* fixme: We should really pick up first child always */
	string = g_object_new (SP_TYPE_STRING, NULL);
	object->children = sp_object_attach_reref (object, SP_OBJECT (string), NULL);
	string->ly = &tspan->ly;
	sp_object_invoke_build (object->children, doc, rch, SP_OBJECT_IS_CLONED (object));
}

static void
sp_tspan_release (SPObject *object)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (object);

	if (object->children) {
		object->children = sp_object_detach_unref (SP_OBJECT (object), object->children);
	} else {
		g_print ("NULL tspan content\n");
	}

	if (((SPObjectClass *) tspan_parent_class)->release)
		((SPObjectClass *) tspan_parent_class)->release (object);
}

static void
sp_tspan_set (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (object);

	/* fixme: Vectors */
	switch (key) {
	case SP_ATTR_X:
		if (!sp_svg_length_read (value, &tspan->ly.x)) {
			sp_svg_length_unset (&tspan->ly.x, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		/* fixme: Re-layout it */
		if (tspan->role != SP_TSPAN_ROLE_LINE) sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_Y:
		if (!sp_svg_length_read (value, &tspan->ly.y)) {
			sp_svg_length_unset (&tspan->ly.y, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		/* fixme: Re-layout it */
		if (tspan->role != SP_TSPAN_ROLE_LINE) sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_DX:
		if (!sp_svg_length_read (value, &tspan->ly.dx)) {
			sp_svg_length_unset (&tspan->ly.dx, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		/* fixme: Re-layout it */
		break;
	case SP_ATTR_DY:
		if (!sp_svg_length_read (value, &tspan->ly.dy)) {
			sp_svg_length_unset (&tspan->ly.dy, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		/* fixme: Re-layout it */
		break;
	case SP_ATTR_ROTATE:
		/* fixme: Implement SVGNumber or something similar (Lauris) */
		tspan->ly.rotate = (value) ? sp_svg_atof (value) : 0.0;
		tspan->ly.rotate_set = (value != NULL);
		/* fixme: Re-layout it */
		break;
	case SP_ATTR_SODIPODI_ROLE:
		if (value && (!strcmp (value, "line") || !strcmp (value, "paragraph"))) {
			tspan->role = SP_TSPAN_ROLE_LINE;
		} else {
			tspan->role = SP_TSPAN_ROLE_UNSPECIFIED;
		}
		break;
	default:
		if (((SPObjectClass *) tspan_parent_class)->set)
			(((SPObjectClass *) tspan_parent_class)->set) (object, key, value);
		break;
	}
}

static void
sp_tspan_child_added (SPObject *object, SPRepr *rch, SPRepr *ref)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (object);

	if (((SPObjectClass *) tspan_parent_class)->child_added)
		((SPObjectClass *) tspan_parent_class)->child_added (object, rch, ref);

	if (!object->children && (sp_repr_is_text (rch) || sp_repr_is_cdata (rch))) {
		SPString *string;
		/* fixme: We should really pick up first child always */
		string = g_object_new (SP_TYPE_STRING, 0);
		object->children = sp_object_attach_reref (object, SP_OBJECT (string), NULL);
		string->ly = &tspan->ly;
		sp_object_invoke_build (object->children, SP_OBJECT_DOCUMENT (object), rch, SP_OBJECT_IS_CLONED (object));
	}
}

static unsigned int
sp_tspan_remove_child (SPObject *object, SPRepr *rch)
{
	SPTSpan *tspan;
	unsigned int ret;

	tspan = SP_TSPAN (object);

	if (((SPObjectClass *) tspan_parent_class)->remove_child) {
		ret = ((SPObjectClass *) tspan_parent_class)->remove_child (object, rch);
		if (!ret) return ret;
	}

	if (object->children && (SP_OBJECT_REPR (object->children) == rch)) {
		object->children = sp_object_detach_unref (object, object->children);
	}

	return TRUE;
}

static void
sp_tspan_update (SPObject *object, SPCtx *ctx, guint flags)
{
	SPTSpan *tspan;
	SPItemCtx *ictx;
	SPStyle *style;
	double d;

	tspan = SP_TSPAN (object);
	style = SP_OBJECT_STYLE (object);
	ictx = (SPItemCtx *) ctx;

	if (((SPObjectClass *) tspan_parent_class)->update)
		((SPObjectClass *) tspan_parent_class)->update (object, ctx, flags);

	if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	flags &= SP_OBJECT_MODIFIED_CASCADE;

	/* Update relative distances */
	d = 1.0 / NR_MATRIX_DF_EXPANSION (&ictx->i2vp);
	sp_text_update_length (&tspan->ly.x, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_text_update_length (&tspan->ly.y, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_text_update_length (&tspan->ly.dx, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_text_update_length (&tspan->ly.dy, style->font_size.computed, style->font_size.computed * 0.5, d);

	if (object->children) {
		if (flags || (object->children->uflags & SP_OBJECT_MODIFIED_FLAG)) {
			sp_object_invoke_update (object->children, ctx, flags);
		}
	}
}

static void
sp_tspan_modified (SPObject *object, unsigned int flags)
{
	SPTSpan *tspan;
	SPStyle *style;

	tspan = SP_TSPAN (object);
	style = SP_OBJECT_STYLE (object);

	if (((SPObjectClass *) tspan_parent_class)->modified)
		((SPObjectClass *) tspan_parent_class)->modified (object, flags);

	if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	flags &= SP_OBJECT_MODIFIED_CASCADE;

	if (object->children) {
		if (flags || (object->children->mflags & SP_OBJECT_MODIFIED_FLAG)) {
			sp_object_invoke_modified (object->children, flags);
		}
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

	if (tspan->ly.x.set) sp_repr_set_double (repr, "x", tspan->ly.x.computed);
	if (tspan->ly.y.set) sp_repr_set_double (repr, "y", tspan->ly.y.computed);
	if (tspan->ly.dx.set) sp_repr_set_double (repr, "dx", tspan->ly.dx.computed);
	if (tspan->ly.dy.set) sp_repr_set_double (repr, "dy", tspan->ly.dy.computed);
	if (tspan->ly.rotate_set) sp_repr_set_double (repr, "rotate", tspan->ly.rotate);
	if (flags & SP_OBJECT_WRITE_SODIPODI) {
		sp_repr_set_attr (repr, "sodipodi:role", (tspan->role != SP_TSPAN_ROLE_UNSPECIFIED) ? "line" : NULL);
	}

	if (flags & SP_OBJECT_WRITE_BUILD) {
		SPRepr *rstr;
		char *text;
		text = g_utf16_to_utf8 (SP_STRING_UCHARS (object->children), -1, NULL, NULL, NULL);
		/* TEXT element */
		rstr = sp_xml_document_createTextNode (sp_repr_get_doc (repr), text);
		g_free (text);

		sp_repr_append_child (repr, rstr);
		sp_repr_unref (rstr);
	} else {
		sp_repr_set_content_ucs2 (SP_OBJECT_REPR (object->children), SP_STRING_UCHARS (object->children));
	}

	/* fixme: Strictly speaking, item class write 'transform' too */
	/* fixme: This is harmless as long as tspan affine is identity (lauris) */
	if (((SPObjectClass *) tspan_parent_class)->write)
		((SPObjectClass *) tspan_parent_class)->write (object, repr, flags);

	return repr;
}

static void
sp_tspan_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (item);

	if (((SPObject *) tspan)->children) {
		sp_item_invoke_bbox_full (SP_ITEM (((SPObject *) tspan)->children), bbox, transform, flags, FALSE);
	}
}

static NRArenaItem *
sp_tspan_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (item);

	if (((SPObject *) tspan)->children) {
		NRArenaItem *ai, *ac;
		ai = nr_arena_item_new (arena, NR_TYPE_ARENA_GROUP);
		nr_arena_group_set_transparent (NR_ARENA_GROUP (ai), FALSE);
		ac = sp_item_invoke_show (SP_ITEM (((SPObject *) tspan)->children), arena, key, flags);
		if (ac) {
			nr_arena_item_add_child (ai, ac, NULL);
			nr_arena_item_unref (ac);
		}
		return ai;
	}

	return NULL;
}

static void
sp_tspan_hide (SPItem *item, unsigned int key)
{
	SPTSpan *tspan;

	tspan = SP_TSPAN (item);

	if (((SPObject *) tspan)->children) sp_item_invoke_hide (SP_ITEM (((SPObject *) tspan)->children), key);

	if (((SPItemClass *) tspan_parent_class)->hide)
		((SPItemClass *) tspan_parent_class)->hide (item, key);
}

static void
sp_tspan_set_shape (SPTSpan *tspan, SPLayoutData *ly, ArtPoint *cp, gboolean firstline, gboolean *inspace)
{
	SPStyle *style;

	style = SP_OBJECT_STYLE (tspan);

	sp_string_set_shape (SP_STRING (((SPObject *) tspan)->children), &tspan->ly, cp, inspace);
}

/* SPText */

static void sp_text_class_init (SPTextClass *class);
static void sp_text_init (SPText *text);

static void sp_text_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_text_release (SPObject *object);
static void sp_text_set (SPObject *object, unsigned int key, const unsigned char *value);
static void sp_text_child_added (SPObject *object, SPRepr *rch, SPRepr *ref);
static unsigned int sp_text_remove_child (SPObject *object, SPRepr *rch);
static void sp_text_update (SPObject *object, SPCtx *ctx, guint flags);
static void sp_text_modified (SPObject *object, guint flags);
static unsigned int sp_text_sequence (SPObject *object, SPObject *target, unsigned int *seq);
static SPRepr *sp_text_write (SPObject *object, SPRepr *repr, guint flags);

static void sp_text_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags);
static NRArenaItem *sp_text_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags);
static void sp_text_hide (SPItem *item, unsigned int key);
static char * sp_text_description (SPItem *item);
static int sp_text_snappoints (SPItem *item, NRPointF *p, int size, const NRMatrixF *transform);
static void sp_text_write_transform (SPItem *item, SPRepr *repr, NRMatrixF *transform);
static void sp_text_print (SPItem *item, SPPrintContext *gpc);

static void sp_text_request_relayout (SPText *text, guint flags);
static void sp_text_update_immediate_state (SPText *text);
static void sp_text_set_shape (SPText *text);

static SPObject *sp_text_get_child_by_position (SPText *text, gint pos);

static SPItemClass *text_parent_class;

GType
sp_text_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPTextClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_text_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPText),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_text_init,
		};
		type = g_type_register_static (SP_TYPE_ITEM, "SPText", &info, 0);
	}
	return type;
}

static void
sp_text_class_init (SPTextClass *class)
{
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	text_parent_class = g_type_class_ref (SP_TYPE_ITEM);

	sp_object_class->build = sp_text_build;
	sp_object_class->release = sp_text_release;
	sp_object_class->set = sp_text_set;
	sp_object_class->child_added = sp_text_child_added;
	sp_object_class->remove_child = sp_text_remove_child;
	sp_object_class->update = sp_text_update;
	sp_object_class->modified = sp_text_modified;
	sp_object_class->sequence = sp_text_sequence;
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
	text->ly.linespacing = 1.0;
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

	if (((SPObjectClass *) text_parent_class)->build)
		((SPObjectClass *) text_parent_class)->build (object, doc, repr);
	sp_object_read_attr (object, "x");
	sp_object_read_attr (object, "y");
	sp_object_read_attr (object, "dx");
	sp_object_read_attr (object, "dy");
	sp_object_read_attr (object, "rotate");
	sp_object_read_attr (object, "sodipodi:linespacing");

	version = sp_text_find_version (object);

	if ((version > 0) && (version < 25)) {
		const guchar *content;
		/* Old sodipodi */
		for (rch = repr->children; rch != NULL; rch = rch->next) {
			if (sp_repr_is_text (rch) || sp_repr_is_cdata (rch)) {
				content = sp_repr_get_content (rch);
				sp_text_set_repr_text_multiline (text, content);
				break;
			}
		}
	}

	ref = NULL;

	for (rch = repr->children; rch != NULL; rch = rch->next) {
		if (sp_repr_is_text (rch) || sp_repr_is_cdata (rch)) {
			SPString *string;
			string = g_object_new (SP_TYPE_STRING, 0);
			if (ref) {
				ref->next = sp_object_attach_reref (object, SP_OBJECT (string), NULL);
			} else {
				object->children = sp_object_attach_reref (object, SP_OBJECT (string), NULL);
			}
			string->ly = &text->ly;
			sp_object_invoke_build (SP_OBJECT (string), doc, rch, SP_OBJECT_IS_CLONED (object));
			ref = SP_OBJECT (string);
		} else if (sp_repr_is_element(rch) && !strcmp (sp_repr_name (rch), "tspan")) {
			SPObject *child;
			child = g_object_new (SP_TYPE_TSPAN, 0);
			if (ref) {
				ref->next = sp_object_attach_reref (object, child, NULL);
			} else {
				object->children = sp_object_attach_reref (object, child, NULL);
			}
			sp_object_invoke_build (child, doc, rch, SP_OBJECT_IS_CLONED (object));
			ref = child;
		} else {
			continue;
		}
	}

	sp_text_update_immediate_state (text);
}

static void
sp_text_release (SPObject *object)
{
	SPText *text;

	text = SP_TEXT (object);

	while (object->children) {
		object->children = sp_object_detach_unref (SP_OBJECT (object), object->children);
	}

	if (((SPObjectClass *) text_parent_class)->release)
		((SPObjectClass *) text_parent_class)->release (object);
}

static void
sp_text_set (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPText *text;

	text = SP_TEXT (object);

	/* fixme: Vectors (Lauris) */
	switch (key) {
	case SP_ATTR_X:
		if (!sp_svg_length_read (value, &text->ly.x)) {
			sp_svg_length_unset (&text->ly.x, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_TEXT_LAYOUT_MODIFIED_FLAG);
		break;
	case SP_ATTR_Y:
		if (!sp_svg_length_read (value, &text->ly.y)) {
			sp_svg_length_unset (&text->ly.y, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_TEXT_LAYOUT_MODIFIED_FLAG);
		break;
	case SP_ATTR_DX:
		if (!sp_svg_length_read (value, &text->ly.dx)) {
			sp_svg_length_unset (&text->ly.dx, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		/* fixme: Re-layout it */
		break;
	case SP_ATTR_DY:
		if (!sp_svg_length_read (value, &text->ly.dy)) {
			sp_svg_length_unset (&text->ly.dy, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		/* fixme: Re-layout it */
		break;
	case SP_ATTR_ROTATE:
		/* fixme: Implement SVGNumber or something similar (Lauris) */
		text->ly.rotate = (value) ? sp_svg_atof (value) : 0.0;
		text->ly.rotate_set = (value != NULL);
		/* fixme: Re-layout it */
		break;
	case SP_ATTR_SODIPODI_LINESPACING:
		text->ly.linespacing = 1.0;
		if (value) {
			text->ly.linespacing = sp_svg_read_percentage (value, 1.0);
			text->ly.linespacing = CLAMP (text->ly.linespacing, 0.01, 10.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_TEXT_LAYOUT_MODIFIED_FLAG);
		break;
	default:
		if (((SPObjectClass *) text_parent_class)->set)
			((SPObjectClass *) text_parent_class)->set (object, key, value);
		break;
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

	if (((SPObjectClass *) text_parent_class)->child_added)
		((SPObjectClass *) text_parent_class)->child_added (object, rch, ref);

	/* Search for position reference */
	prev = NULL;

	if (ref != NULL) {
		prev = object->children;
		while (prev && (prev->repr != ref)) prev = prev->next;
	}

	if (sp_repr_is_text (rch) || sp_repr_is_cdata (rch)) {
		SPString *string;
		string = g_object_new (SP_TYPE_STRING, 0);
		if (prev) {
			prev->next = sp_object_attach_reref (object, SP_OBJECT (string), prev->next);
		} else {
			object->children = sp_object_attach_reref (object, SP_OBJECT (string), object->children);
		}
		string->ly = &text->ly;
		sp_object_invoke_build (SP_OBJECT (string), SP_OBJECT_DOCUMENT (object), rch, SP_OBJECT_IS_CLONED (object));
		och = SP_OBJECT (string);
	} else if (sp_repr_is_element(rch) && !strcmp (sp_repr_name (rch), "tspan")) {
		SPObject *child;
		child = g_object_new (SP_TYPE_TSPAN, 0);
		if (prev) {
			prev->next = sp_object_attach_reref (object, child, prev->next);
		} else {
			object->children = sp_object_attach_reref (object, child, object->children);
		}
		sp_object_invoke_build (child, SP_OBJECT_DOCUMENT (object), rch, SP_OBJECT_IS_CLONED (object));
		och = child;
	} else {
		och = NULL;
	}

	if (och) {
		SPItemView *v;
		NRArenaItem *ac;

		for (v = item->display; v != NULL; v = v->view.next) {
			ac = sp_item_invoke_show (SP_ITEM (och), NR_ARENA_ITEM_ARENA (v), v->view.key, v->view.flags);
			if (ac) {
				nr_arena_item_add_child (v, ac, NULL);
				nr_arena_item_unref (ac);
			}
		}
	}

	sp_text_request_relayout (text, SP_OBJECT_MODIFIED_FLAG | SP_TEXT_CONTENT_MODIFIED_FLAG);
	/* fixme: Instead of forcing it, do it when needed */
	sp_text_update_immediate_state (text);
}

static unsigned int
sp_text_remove_child (SPObject *object, SPRepr *rch)
{
	SPText *text;
	SPObject *prev, *och;
	unsigned int ret;

	text = SP_TEXT (object);

	if (((SPObjectClass *) text_parent_class)->remove_child) {
		ret = ((SPObjectClass *) text_parent_class)->remove_child (object, rch);
		if (!ret) return ret;
	}

	prev = NULL;
	och = object->children;
	while (och->repr != rch) {
		prev = och;
		och = och->next;
	}

	if (prev) {
		prev->next = sp_object_detach_unref (object, och);
	} else {
		object->children = sp_object_detach_unref (object, och);
	}

	sp_text_request_relayout (text, SP_OBJECT_MODIFIED_FLAG | SP_TEXT_CONTENT_MODIFIED_FLAG);
	sp_text_update_immediate_state (text);

	return TRUE;
}

/* fixme: This is wrong, as we schedule relayout every time something changes */

static void
sp_text_update (SPObject *object, SPCtx *ctx, guint flags)
{
	SPText *text;
	SPItemCtx *ictx;
	SPObject *child;
	GSList *l;
	guint cflags;
	SPStyle *style;
	double d;

	text = SP_TEXT (object);
	style = SP_OBJECT_STYLE (text);
	ictx = (SPItemCtx *) ctx;

	if (((SPObjectClass *) text_parent_class)->update)
		((SPObjectClass *) text_parent_class)->update (object, ctx, flags);

	cflags = (flags & SP_OBJECT_MODIFIED_CASCADE);
	if (flags & SP_OBJECT_MODIFIED_FLAG) cflags |= SP_OBJECT_PARENT_MODIFIED_FLAG;

	/* Update relative distances */
	d = 1.0 / NR_MATRIX_DF_EXPANSION (&ictx->i2vp);
	sp_text_update_length (&text->ly.x, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_text_update_length (&text->ly.y, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_text_update_length (&text->ly.dx, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_text_update_length (&text->ly.dy, style->font_size.computed, style->font_size.computed * 0.5, d);

	/* Create temporary list of children */
	l = NULL;
	for (child = object->children; child != NULL; child = child->next) {
		sp_object_ref (SP_OBJECT (child), object);
		l = g_slist_prepend (l, child);
	}
	l = g_slist_reverse (l);
	while (l) {
		child = SP_OBJECT (l->data);
		l = g_slist_remove (l, child);
		if (cflags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
			/* fixme: Do we need transform? */
			sp_object_invoke_update (child, ctx, cflags);
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

static void
sp_text_modified (SPObject *object, guint flags)
{
	SPText *text;
	SPObject *child;
	GSList *l;
	guint cflags;

	text = SP_TEXT (object);

	if (((SPObjectClass *) text_parent_class)->modified)
		((SPObjectClass *) text_parent_class)->modified (object, flags);

	cflags = (flags & SP_OBJECT_MODIFIED_CASCADE);
	if (flags & SP_OBJECT_MODIFIED_FLAG) cflags |= SP_OBJECT_PARENT_MODIFIED_FLAG;

	/* Create temporary list of children */
	l = NULL;
	for (child = object->children; child != NULL; child = child->next) {
		sp_object_ref (SP_OBJECT (child), object);
		l = g_slist_prepend (l, child);
	}
	l = g_slist_reverse (l);
	while (l) {
		child = SP_OBJECT (l->data);
		l = g_slist_remove (l, child);
		if (cflags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
			sp_object_invoke_modified (child, cflags);
		}
		sp_object_unref (SP_OBJECT (child), object);
	}
}

static unsigned int
sp_text_sequence (SPObject *object, SPObject *target, unsigned int *seq)
{
	SPText *text;
	SPObject *child;

	text = (SPText *) object;

	for (child = object->children; child != NULL; child = child->next) {
		if (SP_IS_TSPAN (child)) {
			*seq += 1;
			if (sp_object_invoke_sequence (child, target, seq)) return TRUE;
		}
	}

	return FALSE;
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
		for (child = object->children; child != NULL; child = child->next) {
			if (SP_IS_TSPAN (child)) {
				crepr = sp_object_invoke_write (child, NULL, flags);
				if (crepr) l = g_slist_prepend (l, crepr);
			} else {
				char *text;
				text = g_utf16_to_utf8 (SP_STRING_UCHARS (child), -1, NULL, NULL, NULL);
				crepr = sp_xml_document_createTextNode (sp_repr_get_doc (repr), text);
				g_free (text);
			}
		}
		while (l) {
			sp_repr_add_child (repr, (SPRepr *) l->data, NULL);
			sp_repr_unref ((SPRepr *) l->data);
			l = g_slist_remove (l, l->data);
		}
	} else {
		for (child = object->children; child != NULL; child = child->next) {
			if (SP_IS_TSPAN (child)) {
				sp_object_invoke_write (child, SP_OBJECT_REPR (child), flags);
			} else {
				sp_repr_set_content_ucs2 (SP_OBJECT_REPR (child), SP_STRING_UCHARS (child));
			}
		}
	}

	if (text->ly.x.set) sp_repr_set_double (repr, "x", text->ly.x.computed);
	if (text->ly.y.set) sp_repr_set_double (repr, "y", text->ly.y.computed);
	if (text->ly.dx.set) sp_repr_set_double (repr, "dx", text->ly.dx.computed);
	if (text->ly.dy.set) sp_repr_set_double (repr, "dy", text->ly.dy.computed);
	if (text->ly.rotate_set) sp_repr_set_double (repr, "rotate", text->ly.rotate);

	if (((SPObjectClass *) (text_parent_class))->write)
		((SPObjectClass *) (text_parent_class))->write (object, repr, flags);

	return repr;
}

static void
sp_text_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags)
{
	SPText *text;
	SPItem *child;
	SPObject *o;

	text = SP_TEXT (item);

	for (o = ((SPObject *) text)->children; o != NULL; o = o->next) {
		NRMatrixD a;
		child = SP_ITEM (o);
		nr_matrix_multiply_dfd (&a, &child->transform, transform);
		sp_item_invoke_bbox_full (child, bbox, &a, flags, FALSE);
	}
}

static NRArenaItem *
sp_text_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags)
{
	SPText *text;
	NRArenaItem *ai, *ac, *ar;
	SPItem * child;
	SPObject * o;

	text = SP_TEXT (item);

	ai = nr_arena_item_new (arena, NR_TYPE_ARENA_GROUP);
	nr_arena_group_set_transparent (NR_ARENA_GROUP (ai), FALSE);

	ar = NULL;
	for (o = ((SPObject *) text)->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			ac = sp_item_invoke_show (child, arena, key, flags);
			if (ac) {
				nr_arena_item_add_child (ai, ac, ar);
				ar = ac;
				nr_arena_item_unref (ac);
			}
		}
	}

	return ai;
}

static void
sp_text_hide (SPItem *item, unsigned int key)
{
	SPText *text;
	SPItem * child;
	SPObject * o;

	text = SP_TEXT (item);

	for (o = ((SPObject *) text)->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			sp_item_invoke_hide (child, key);
		}
	}

	if (((SPItemClass *) text_parent_class)->hide)
		((SPItemClass *) text_parent_class)->hide (item, key);
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

static guchar *
sp_text_font_style_to_lookup (SPStyle *style)
{
	static guchar c[256];
	guchar *wstr, *sstr, *p;

	switch (style->font_weight.computed) {
	case SP_CSS_FONT_WEIGHT_100:
		wstr = "extra light";
		break;
	case SP_CSS_FONT_WEIGHT_200:
		wstr = "thin";
		break;
	case SP_CSS_FONT_WEIGHT_300:
		wstr = "light";
		break;
	case SP_CSS_FONT_WEIGHT_400:
	case SP_CSS_FONT_WEIGHT_NORMAL:
		wstr = NULL;
		break;
	case SP_CSS_FONT_WEIGHT_500:
		wstr = "medium";
		break;
	case SP_CSS_FONT_WEIGHT_600:
		wstr = "semi";
		break;
	case SP_CSS_FONT_WEIGHT_700:
	case SP_CSS_FONT_WEIGHT_BOLD:
		wstr = "bold";
		break;
	case SP_CSS_FONT_WEIGHT_800:
		wstr = "heavy";
		break;
	case SP_CSS_FONT_WEIGHT_900:
		wstr = "black";
		break;
	default:
		wstr = NULL;
		break;
	}

	switch (style->font_style.computed) {
	case SP_CSS_FONT_STYLE_NORMAL:
		sstr = NULL;
		break;
	case SP_CSS_FONT_STYLE_ITALIC:
		sstr = "italic";
		break;
	case SP_CSS_FONT_STYLE_OBLIQUE:
		sstr = "oblique";
		break;
	default:
		sstr = NULL;
		break;
	}

	p = c;
	if (wstr) {
		if (p != c) *p++ = ' ';
		memcpy (p, wstr, strlen (wstr));
		p += strlen (wstr);
	}
	if (sstr) {
		if (p != c) *p++ = ' ';
		memcpy (p, sstr, strlen (sstr));
		p += strlen (sstr);
	}
	*p = '\0';

	return c;
}

/* fixme: Do text chunks here (Lauris) */
/* fixme: We'll remove string bbox adjustment and bring it here for the whole chunk (Lauris) */

static void
sp_text_set_shape (SPText *text)
{
	ArtPoint cp;
	SPObject *child;
	gboolean isfirstline, inspace;
	NRRectF paintbox;

	/* The logic should be: */
	/* 1. Calculate attributes */
	/* 2. Iterate through children asking them to set shape */

	cp.x = text->ly.x.computed;
	cp.y = text->ly.y.computed;
	if (text->ly.dx.set) cp.x += text->ly.dx.computed;
	if (text->ly.dy.set) cp.y += text->ly.dy.computed;

	isfirstline = TRUE;
	inspace = FALSE;

	child = ((SPObject *) text)->children;
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
						cp.x -= child->style->font_size.computed * text->ly.linespacing;
						cp.y = text->ly.y.computed;
					} else {
						cp.x = text->ly.x.computed;
						cp.y += child->style->font_size.computed * text->ly.linespacing;
					}
				}
				/* fixme: This is extremely (EXTREMELY) dangerous (Lauris) */
				/* fixme: Our only hope is to ensure LINE tspans do not request ::modified */
				sp_document_set_undo_sensitive (SP_OBJECT_DOCUMENT (tspan), FALSE);
				sp_repr_set_double (SP_OBJECT_REPR (tspan), "x", cp.x);
				sp_repr_set_double (SP_OBJECT_REPR (tspan), "y", cp.y);
				sp_document_set_undo_sensitive (SP_OBJECT_DOCUMENT (tspan), TRUE);
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
				if (tspan->ly.dx.set) cp.x += tspan->ly.dx.computed;
				if (tspan->ly.dy.set) cp.y += tspan->ly.dy.computed;
				break;
			default:
				/* Error */
				break;
			}
		} else {
			string = SP_STRING (child);
		}
		/* Calculate block bbox */
		advance.x = (string->ly->dx.set) ? string->ly->dx.computed : 0.0;
		advance.y = (string->ly->dy.set) ? string->ly->dy.computed : 0.0;
		bbox.x0 = string->bbox.x0 + advance.x;
		bbox.y0 = string->bbox.y0 + advance.y;
		bbox.x1 = string->bbox.x1 + advance.x;
		bbox.y1 = string->bbox.y1 + advance.y;
		advance.x += string->advance.x;
		advance.y += string->advance.y;
		for (next = child->next; next != NULL; next = next->next) {
			SPString *string;
			if (SP_IS_TSPAN (next)) {
				SPTSpan *tspan;
				tspan = SP_TSPAN (next);
				if (tspan->role != SP_TSPAN_ROLE_UNSPECIFIED) break;
				if ((tspan->ly.x.set) || (tspan->ly.y.set)) break;
				if (tspan->ly.dx.set) advance.x += tspan->ly.dx.computed;
				if (tspan->ly.dy.set) advance.y += tspan->ly.dy.computed;
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
				sp_string_set_shape (SP_STRING (next), &text->ly, &cp, &inspace);
			} else {
				SPTSpan *tspan;
				tspan = SP_TSPAN (next);
				if (tspan->ly.dx.set) cp.x += tspan->ly.dx.computed;
				if (tspan->ly.dy.set) cp.y += tspan->ly.dy.computed;
				sp_tspan_set_shape (tspan, &text->ly, &cp, isfirstline, &inspace);
			}
		}
		child = next;
		isfirstline = FALSE;
	}

	sp_item_invoke_bbox (SP_ITEM (text), &paintbox, NULL, TRUE);

	for (child = ((SPObject *) text)->children; child != NULL; child = child->next) {
		SPString *string;
		if (SP_IS_TSPAN (child)) {
			string = SP_TSPAN_STRING (child);
		} else {
			string = SP_STRING (child);
		}
		sp_chars_set_paintbox (SP_CHARS (string), &paintbox);
	}
}

static int
sp_text_snappoints (SPItem *item, NRPointF *p, int size, const NRMatrixF *transform)
{
	SPLayoutData *ly;
	int pos;

	pos = 0;
	if (((SPItemClass *) text_parent_class)->snappoints)
		pos = ((SPItemClass *) text_parent_class)->snappoints (item, p, size, transform);

	if (pos < size) {
		ly = &SP_TEXT (item)->ly;
		p[pos].x = NR_MATRIX_DF_TRANSFORM_X (transform, ly->x.computed, ly->y.computed);
		p[pos].y = NR_MATRIX_DF_TRANSFORM_Y (transform, ly->x.computed, ly->y.computed);
	}

	return pos;
}

/*
 * Initially we'll do:
 * Transform x, y, set x, y, clear translation
 */

static void
sp_text_write_transform (SPItem *item, SPRepr *repr, NRMatrixF *t)
{
	SPText *text;
	NRMatrixF i2p, p2i;
	gdouble px, py, x, y;
	SPObject *child;

	text = SP_TEXT (item);

	i2p = *t;
	i2p.c[4] = 0.0;
	i2p.c[5] = 0.0;
	nr_matrix_f_invert (&p2i, &i2p);

	px = NR_MATRIX_DF_TRANSFORM_X (t, text->ly.x.computed, text->ly.y.computed);
	py = NR_MATRIX_DF_TRANSFORM_Y (t, text->ly.x.computed, text->ly.y.computed);
	x = NR_MATRIX_DF_TRANSFORM_X (&p2i, px, py);
	y = NR_MATRIX_DF_TRANSFORM_Y (&p2i, px, py);
	sp_repr_set_double (repr, "x", x);
	sp_repr_set_double (repr, "y", y);
	for (child = ((SPObject *) text)->children; child != NULL; child = child->next) {
		if (SP_IS_TSPAN (child)) {
			SPTSpan *tspan;
			tspan = SP_TSPAN (child);
			if (tspan->ly.x.set || tspan->ly.y.set) {
				x = (tspan->ly.x.set) ? tspan->ly.x.computed : text->ly.x.computed;
				y = (tspan->ly.y.set) ? tspan->ly.y.computed : text->ly.y.computed;
				px = NR_MATRIX_DF_TRANSFORM_X (t, x, y);
				py = NR_MATRIX_DF_TRANSFORM_Y (t, x, y);
				x = NR_MATRIX_DF_TRANSFORM_X (&p2i, px, py);
				y = NR_MATRIX_DF_TRANSFORM_Y (&p2i, px, py);
				/* fixme: This is wrong - what if repr != SP_OBJECT_REPR (text) */
				sp_repr_set_double (SP_OBJECT_REPR (tspan), "x", x);
				sp_repr_set_double (SP_OBJECT_REPR (tspan), "y", y);
			}
		}
	}
	*t = i2p;
}

static void
sp_text_print (SPItem *item, SPPrintContext *ctx)
{
	SPText *text;
	SPObject *ch;
	NRMatrixF ctm;
	NRRectF pbox, dbox, bbox;

	text = SP_TEXT (item);

	/* fixme: Think (Lauris) */
	sp_item_invoke_bbox (item, &pbox, NULL, TRUE);
	sp_item_get_bbox_document (item, &bbox, 0, TRUE);
	dbox.x0 = 0.0;
	dbox.y0 = 0.0;
	dbox.x1 = sp_document_width (SP_OBJECT_DOCUMENT (item));
	dbox.y1 = sp_document_height (SP_OBJECT_DOCUMENT (item));
	sp_item_i2doc_affine (item, &ctm);

	for (ch = ((SPObject *) text)->children; ch != NULL; ch = ch->next) {
		if (SP_IS_TSPAN (ch)) {
			sp_chars_do_print (SP_CHARS (((SPObject *) ch)->children), ctx, &ctm, &pbox, &dbox, &bbox);
		} else if (SP_IS_STRING (ch)) {
			sp_chars_do_print (SP_CHARS (ch), ctx, &ctm, &pbox, &dbox, &bbox);
		}
	}
}

int
sp_text_is_empty (SPText *text)
{
	SPObject *ch;

	for (ch = ((SPObject *) text)->children; ch != NULL; ch = ch->next) {
		SPString *str;
		NRUShort *p;
		str = SP_TEXT_CHILD_STRING (ch);
		for (p = str->uchars; p && *p; p++) {
			gunichar unival;
			unival = (gunichar)*p;
			if ((unival > 0xe000) && (unival <= 0xf8ff)) return FALSE;
			if (g_unichar_isgraph (unival)) return FALSE;
		}
	}

	return TRUE;
}

gchar *
sp_text_get_string_multiline (SPText *text)
{
	SPObject *ch;
	GSList *strs, *l;
	gint len;
	guchar *str, *p;

	strs = NULL;
	for (ch = ((SPObject *) text)->children; ch != NULL; ch = ch->next) {
		if (SP_IS_TSPAN (ch)) {
			SPTSpan *tspan;
			tspan = SP_TSPAN (ch);

			if (((SPObject *) tspan)->children && SP_STRING_UCHARS (((SPObject *) tspan)->children)) {
				char *text;
				/* Memory allocated, it should be freed later */
				text = g_utf16_to_utf8 (SP_STRING_UCHARS (((SPObject *) tspan)->children), -1,
							NULL, NULL, NULL);
				strs = g_slist_prepend (strs, text);
			}
		} else if (SP_IS_STRING (ch) && SP_STRING_UCHARS (ch)) {
			char *text;
			text = g_utf16_to_utf8 (SP_STRING_UCHARS (ch), -1, NULL, NULL, NULL);
			strs = g_slist_prepend (strs, text);
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

		/* Free utf8 string */
		g_free (strs->data);

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
		rstr = sp_xml_document_createTextNode (sp_repr_get_doc (repr), p);
		sp_repr_add_child (rtspan, rstr, NULL);
		sp_repr_append_child (repr, rtspan);
		p = (e) ? e + 1 : NULL;
	}

	g_free (content);

	/* fixme: Calculate line positions (Lauris) */
}

GSList *
sp_text_normalized_bpath_list (SPText *text)
{
	SPObject *child;
	GSList *cc;
	/* SPCurve *curve; */

	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (SP_IS_TEXT (text), NULL);

	cc = NULL;
	for (child = ((SPObject *) text)->children; child != NULL; child = child->next) {
		GSList *c;
		if (SP_IS_STRING (child)) {
			c = sp_chars_normalized_bpath_list (SP_CHARS (child));
		} else if (SP_IS_TSPAN (child)) {
			SPTSpan *tspan;
			tspan = SP_TSPAN (child);
			c = sp_chars_normalized_bpath_list (SP_CHARS (((SPObject *) tspan)->children));
		} else {
			c = NULL;
		}
		if (c) cc = g_slist_concat (cc, c);
	}

	cc = g_slist_reverse (cc);

#if 0
	curve = sp_curve_concat (cc);

	while (cc) {
		sp_curve_unref ((SPCurve *) cc->data);
		cc = g_slist_remove (cc, cc->data);
	}

	return curve;
#endif

	return cc;
}

static void
sp_text_update_immediate_state (SPText *text)
{
	SPObject *child;
	guint start;

	start = 0;
	for (child = ((SPObject *) text)->children; child != NULL; child = child->next) {
		SPString *string;
		if (SP_IS_TSPAN (child)) {
			string = SP_TSPAN_STRING (child);
		} else {
			string = SP_STRING (child);
		}
		string->start = start;
		string->ulen = (string->uchars) ? arikkei_ucs2_strlen (string->uchars) : 0;
		start += string->ulen;

		/* Count newlines as well */
		if (child->next) start += 1;
	}
}

static void
sp_text_request_relayout (SPText *text, guint flags)
{
	text->relayout = TRUE;

	sp_object_request_update (SP_OBJECT (text), flags);
}

/* fixme: Think about these (Lauris) */

gint
sp_text_get_length (SPText *text)
{
	SPObject *child;
	SPString *string;

	g_return_val_if_fail (text != NULL, 0);
	g_return_val_if_fail (SP_IS_TEXT (text), 0);

	if (!((SPObject *) text)->children) return 0;

	child = ((SPObject *) text)->children;
	while (child->next) child = child->next;

	if (SP_IS_STRING (child)) {
		string = SP_STRING (child);
	} else {
		string = SP_TSPAN_STRING (child);
	}

	return string->start + string->ulen;
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

	for (child = ((SPObject *) text)->children; child != NULL; child = child->next) {
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
	rstring = sp_xml_document_createTextNode (sp_repr_get_doc (rtspan), "");
	sp_repr_add_child (rtspan, rstring, NULL);
	sp_repr_unref (rstring);
	/* Append to text */
	sp_repr_append_child (SP_OBJECT_REPR (text), rtspan);
	sp_repr_unref (rtspan);

	return (SPTSpan *) sp_document_lookup_id (SP_OBJECT_DOCUMENT (text), sp_repr_attr (rtspan, "id"));
}

SPTSpan *
sp_text_insert_line (SPText *text, gint pos)
{
	SPObject *child;
	SPString *string;
	SPRepr *rtspan, *rstring;

	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (SP_IS_TEXT (text), NULL);
	g_return_val_if_fail (pos >= 0, NULL);

	child = sp_text_get_child_by_position (text, pos);
	string = SP_TEXT_CHILD_STRING (child);

	/* Create <tspan> */
	rtspan = sp_repr_new ("tspan");
	sp_repr_set_attr (rtspan, "sodipodi:role", "line");
	/* Create TEXT */
	rstring = sp_xml_document_createTextNode (sp_repr_get_doc (rtspan), "");
	sp_repr_add_child (rtspan, rstring, NULL);
	sp_repr_unref (rstring);

	sp_repr_add_child (SP_OBJECT_REPR (text), rtspan, SP_OBJECT_REPR (child));
	sp_repr_unref (rtspan);

	if (string->uchars) {
		NRUShort *ip;
		ip = string->uchars + (pos - string->start);
		sp_repr_set_content_ucs2 (rstring, ip);
		*ip = '\0';
		sp_repr_set_content_ucs2 (SP_OBJECT_REPR (string), string->uchars);
	}

	return (SPTSpan *) sp_document_lookup_id (SP_OBJECT_DOCUMENT (text), sp_repr_attr (rtspan, "id"));
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

	if (!((SPObject *) text)->children) {
		SPRepr *rtspan, *rstring;
		/* Create <tspan> */
		rtspan = sp_repr_new ("tspan");
		/* Create TEXT */
		rstring = sp_xml_document_createTextNode (sp_repr_get_doc (rtspan), "");
		sp_repr_add_child (rtspan, rstring, NULL);
		sp_repr_unref (rstring);
		/* Add string */
		sp_repr_add_child (SP_OBJECT_REPR (text), rtspan, NULL);
		sp_repr_unref (rtspan);
		g_assert (((SPObject *) text)->children);
	}

	child = ((SPObject *) text)->children;
	while (child->next) child = child->next;
	if (SP_IS_STRING (child)) {
		string = SP_STRING (child);
	} else {
		string = SP_TSPAN_STRING (child);
	}
		
	content = sp_repr_get_content (SP_OBJECT_REPR (string));

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

static NRUShort *
ucs2_concat_ws (NRUShort *d, NRUShort *s, int len)
{
	NRUShort *dp, *sp, *spacep;
	gboolean inspace, intext;

	g_assert (d);

	dp = d;
	inspace = FALSE;
	intext = FALSE;
	while (*dp) {
		if (g_unichar_isspace (*dp)) {
			if (!inspace) spacep = dp;
			inspace = TRUE;
		} else {
			inspace = FALSE;
			intext = TRUE;
		}
		dp++;
	}
	if (inspace) dp = spacep;

	if (!s || len == 0) {
		if (inspace && intext) *dp++ = ' ';
		*dp = '\0';
		return dp;
	}

	sp = s;
	if (len < 0) {
		while (*sp) {
			if (g_unichar_isspace (*sp)) {
				inspace = TRUE;
				sp++;
			} else {
				if (inspace && intext) *dp++ = ' ';
				*dp++ = *sp++;
				inspace = FALSE;
				intext = TRUE;
			}
		}
	} else {
		int i;
		for (i = 0; *sp && (i < len); i++) {
			if (g_unichar_isspace (*sp)) {
				inspace = TRUE;
				sp++;
			} else {
				if (inspace) *dp++ = ' ';
				*dp++ = *sp++;
				inspace = FALSE;
				intext = TRUE;
			}
		}
	}

	if (inspace && intext) *dp++ = ' ';
	*dp = '\0';

#if 0
	guchar *utf8;
	utf8 = arikkei_ucs2_utf8_strdup (d);
	g_print ("d[%s] ", utf8);
	g_free (utf8);
	utf8 = arikkei_ucs2_utf8_strdup (s);
	g_print ("s[%s]\n", utf8);
	g_free (utf8);
#endif

	return dp;
}

/* Returns position after inserted */
gint
sp_text_insert (SPText *text, gint pos, const guchar *utf8, gboolean preservews)
{
	SPObject *child;
	SPString *string;
	NRUShort *ucs2, *new, *ip;
	int slen, ulen, newpos, i;

	g_return_val_if_fail (text != NULL, -1);
	g_return_val_if_fail (SP_IS_TEXT (text), -1);
	g_return_val_if_fail (pos >= 0, -1);

	if (!utf8) return pos;
	if (!*utf8) return pos;

	child = sp_text_get_child_by_position (text, pos);
	if (!child) return sp_text_append (text, utf8);
	string = SP_TEXT_CHILD_STRING (child);

	ip = string->uchars + (pos - string->start);
	/* fixme: Do it the right way (Lauris) */
	if (!preservews && !strcmp (utf8, " ") && (ip > string->uchars) && *(ip - 1) == (NRUShort)' ') {
		return pos;
	}

	slen = ip - string->uchars;
	ulen = g_utf8_strlen (utf8, -1);
	ucs2 = g_utf8_to_utf16 (utf8, -1, NULL, NULL, NULL);
	new = g_new (NRUShort, arikkei_ucs2_strlen (string->uchars) + ulen + 1);

#if 1
	if (SP_STRING_IS_SPACE_PRESERVE (string)) {
		/* Copy start */
		arikkei_ucs2_strncpy (string->uchars, new, slen);
		/* Copy string */
		arikkei_ucs2_strncpy (ucs2, new + slen, ulen);
		for (i = slen; i < slen + ulen; i++) {
			if ((new[i] < 32) && ((new[i] != 9) && (new[i] != 10) && (new[i] != 13))) new[i] = 32;
		}
		/* Copy end */
		arikkei_ucs2_strncpy (ip, new + slen + ulen, -1);

		newpos = pos + ulen;
	} else {
		NRUShort *p1, *p2;
		new[0] = '\0';

		/* Copy start */
		p1 = ucs2_concat_ws (new, string->uchars, slen);
		/* Copy string */
		p2 = ucs2_concat_ws (new, ucs2, -1); /* ulen */
		/* Copy end */
		ucs2_concat_ws (new, ip, -1);

		newpos = pos + (int)(p2 - p1);
	}
	
	sp_repr_set_content_ucs2 (SP_OBJECT_REPR (string), new);
	g_free (new);
	g_free (ucs2);

	return newpos;
#else
	/* Copy start */
	arikkei_ucs2_strncpy (string->uchars, new, slen);
	/* Copy string */
	arikkei_ucs2_strncpy (ucs2, new + slen, ulen);
	for (i = slen; i < slen + ulen; i++) {
		if ((new[i] < 32) && ((new[i] != 9) && (new[i] != 10) && (new[i] != 13))) new[i] = 32;
	}
	/* Copy end */
	arikkei_ucs2_strncpy (ip, new + slen + ulen, -1);

	sp_repr_set_content_ucs2 (SP_OBJECT_REPR (string), new);
	g_free (new);
	g_free (ucs2);

	return pos + ulen;
/* 	return CLAMP (pos + ulen, string->start, string->start + string->ulen); */
#endif
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

	if (!((SPObject *) text)->children) return 0;
	if (start == end) return start;

	schild = sp_text_get_child_by_position (text, start);
	echild = sp_text_get_child_by_position (text, end);

	if (schild != echild) {
		SPString *sstring, *estring;
		SPObject *child;
		NRUShort *ucs2, *sp, *ep;
		GSList *cl;

		/* Easy case */
		sstring = SP_TEXT_CHILD_STRING (schild);
		estring = SP_TEXT_CHILD_STRING (echild);

		sp = sstring->uchars + (start - sstring->start);
		ep = estring->uchars + (end - estring->start);
		ucs2 = g_new (NRUShort, (sp - sstring->uchars) + arikkei_ucs2_strlen (ep) + 1);
		if (sp > sstring->uchars) arikkei_ucs2_strncpy (sstring->uchars, ucs2, sp - sstring->uchars);
		arikkei_ucs2_strncpy (ep, ucs2 + (sp - sstring->uchars), arikkei_ucs2_strlen (ep) + 1);
		sp_repr_set_content_ucs2 (SP_OBJECT_REPR (sstring), ucs2);
		g_free (ucs2);

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
		NRUShort *sp, *ep;

		/* Easy case */
		string = SP_TEXT_CHILD_STRING (schild);
		sp = string->uchars + (start - string->start);
		ep = string->uchars + (end - string->start);
		memmove (sp, ep, sizeof(NRUShort) * (arikkei_ucs2_strlen (ep) + 1));
		sp_repr_set_content_ucs2 (SP_OBJECT_REPR (string), string->uchars);
	}

	return start;
}

/* fixme: Should look roles here */

gint
sp_text_up (SPText *text, gint pos)
{
	SPObject *child, *up;
	SPString *string;
	gint col;

	child = sp_text_get_child_by_position (text, pos);
	if (!child || child == ((SPObject *) text)->children) return pos;
	string = SP_TEXT_CHILD_STRING (child);
	col = pos - string->start;

	up = ((SPObject *) text)->children;
	while (up->next != child) up = up->next;
	string = SP_TEXT_CHILD_STRING (up);

	col = MIN (col, string->ulen);

	return string->start + col;
}

gint
sp_text_down (SPText *text, gint pos)
{
	SPObject *child;
	SPString *string;
	gint col;

	child = sp_text_get_child_by_position (text, pos);
	if (!child || !child->next) return pos;
	string = SP_TEXT_CHILD_STRING (child);
	col = pos - string->start;

	child = child->next;
	string = SP_TEXT_CHILD_STRING (child);

	col = MIN (col, string->ulen);

	return string->start + col;
}

gint
sp_text_start_of_line (SPText *text, gint pos)
{
	SPObject *child;
	SPString *string;

	child = sp_text_get_child_by_position (text, pos);
	if (!child) return 0;
	string = SP_TEXT_CHILD_STRING (child);

	return string->start;
}

gint
sp_text_end_of_line (SPText *text, gint pos)
{
	SPObject *child;
	SPString *string;

	child = sp_text_get_child_by_position (text, pos);
	if (!child) return sp_text_get_length (text);
	string = SP_TEXT_CHILD_STRING (child);

	return string->start + string->ulen;
}

void
sp_text_get_cursor_coords (SPText *text, gint position, ArtPoint *p0, ArtPoint *p1)
{
	SPObject *child;
	SPString *string;
	gfloat x, y;
	NRPosGlyph *pglyph;
	/* NRRectF area; */
	/* NRPointF adv; */

	child = sp_text_get_child_by_position (text, position);
	string = SP_TEXT_CHILD_STRING (child);

	if (string->ulen > 0 && string->pgl) {
		pglyph = &string->pgl->glyphs[position - string->start];
		x = pglyph->x;
		y = pglyph->y;
	} else {
		x = string->ly->x.computed;
		y = string->ly->y.computed;
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

	for (child = ((SPObject *) text)->children; child && child->next; child = child->next) {
		SPString *string;
		if (SP_IS_STRING (child)) {
			string = SP_STRING (child);
		} else {
			string = SP_TSPAN_STRING (child);
		}

		if (pos <= (string->start + string->ulen)) return child;
	}

	return child;
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
