#define __SP_ROOT_C__

/*
 * SVG <svg> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <string.h>
#include <libart_lgpl/art_affine.h>
#include "svg/svg.h"
#include "display/nr-arena-group.h"
#include "print.h"
#include "document.h"
#include "desktop.h"
#include "sp-defs.h"
#include "sp-namedview.h"
#include "sp-root.h"

#define SP_SVG_DEFAULT_WIDTH_PX SP_SVG_PX_FROM_MM (210.0)
#define SP_SVG_DEFAULT_HEIGHT_PX SP_SVG_PX_FROM_MM (297.0)

static void sp_root_class_init (SPRootClass *klass);
static void sp_root_init (SPRoot *root);

static void sp_root_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_root_release (SPObject *object);
static void sp_root_read_attr (SPObject *object, const gchar *key);
static void sp_root_child_added (SPObject *object, SPRepr *child, SPRepr *ref);
static void sp_root_remove_child (SPObject *object, SPRepr *child);
static void sp_root_modified (SPObject *object, guint flags);
static SPRepr *sp_root_write (SPObject *object, SPRepr *repr, guint flags);

static NRArenaItem *sp_root_show (SPItem *item, NRArena *arena);
static void sp_root_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags);
static void sp_root_print (SPItem *item, SPPrintContext *ctx);

static SPGroupClass *parent_class;

GType
sp_root_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPRootClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_root_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPRoot),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_root_init,
		};
		type = g_type_register_static (SP_TYPE_GROUP, "SPRoot", &info, 0);
	}
	return type;
}

static void
sp_root_class_init (SPRootClass *klass)
{
	GObjectClass *object_class;
	SPObjectClass *sp_object_class;
	SPItemClass *sp_item_class;

	object_class = G_OBJECT_CLASS (klass);
	sp_object_class = (SPObjectClass *) klass;
	sp_item_class = (SPItemClass *) klass;

	parent_class = g_type_class_ref (SP_TYPE_GROUP);

	sp_object_class->build = sp_root_build;
	sp_object_class->release = sp_root_release;
	sp_object_class->read_attr = sp_root_read_attr;
	sp_object_class->child_added = sp_root_child_added;
	sp_object_class->remove_child = sp_root_remove_child;
	sp_object_class->modified = sp_root_modified;
	sp_object_class->write = sp_root_write;

	sp_item_class->show = sp_root_show;
	sp_item_class->bbox = sp_root_bbox;
	sp_item_class->print = sp_root_print;
}

static void
sp_root_init (SPRoot *root)
{
	root->group.transparent = TRUE;

	root->svg = 100;
	root->sodipodi = 0;
	root->original = 0;

	sp_svg_length_unset (&root->width, SP_SVG_UNIT_NONE, SP_SVG_DEFAULT_WIDTH_PX, SP_SVG_DEFAULT_WIDTH_PX);
	sp_svg_length_unset (&root->height, SP_SVG_UNIT_NONE, SP_SVG_DEFAULT_HEIGHT_PX, SP_SVG_DEFAULT_HEIGHT_PX);

	nr_matrix_d_set_identity (&root->viewbox);

	root->namedviews = NULL;
	root->defs = NULL;
}

static void
sp_root_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	SPGroup *group;
	SPRoot *root;
	SPObject *o;

	group = (SPGroup *) object;
	root = (SPRoot *) object;

	if (sp_repr_attr (repr, "xmlns:sodipodi") || sp_repr_attr (repr, "sodipodi:docname") || sp_repr_attr (repr, "SP-DOCNAME")) {
		/* This is ugly, but works */
		root->original = 1;
	}
	sp_root_read_attr (object, "sodipodi:version");
	/* It is important to parse these here, so objects will have viewport build-time */
	sp_root_read_attr (object, "width");
	sp_root_read_attr (object, "height");
	sp_root_read_attr (object, "viewBox");

	if (((SPObjectClass *) parent_class)->build)
		(* ((SPObjectClass *) parent_class)->build) (object, document, repr);

	/* Collect all our namedviews */
	for (o = group->children; o != NULL; o = o->next) {
		if (SP_IS_NAMEDVIEW (o)) {
			root->namedviews = g_slist_prepend (root->namedviews, o);
		}
	}
	root->namedviews = g_slist_reverse (root->namedviews);

	/* Search for first <defs> node */
	for (o = group->children; o != NULL; o = o->next) {
		if (SP_IS_DEFS (o)) {
			root->defs = SP_DEFS (o);
			break;
		}
	}
}

static void
sp_root_release (SPObject *object)
{
	SPRoot * root;

	root = (SPRoot *) object;

	root->defs = NULL;
	g_slist_free (root->namedviews);
	root->namedviews = NULL;

	if (((SPObjectClass *) parent_class)->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_root_read_attr (SPObject * object, const gchar * key)
{
	SPItem *item;
	SPRoot *root;
	const guchar *str;
	gulong unit;
	SPItemView *v;

	item = SP_ITEM (object);
	root = SP_ROOT (object);

	str = sp_repr_attr (object->repr, key);

	if (!strcmp (key, "sodipodi:version")) {
		if (str) {
			root->sodipodi = (guint) (atof (str) * 100.0);
		} else {
			root->sodipodi = root->original;
		}
	} else if (!strcmp (key, "width")) {
		if (sp_svg_length_read_lff (str, &unit, &root->width.value, &root->width.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX) &&
		    (unit != SP_SVG_UNIT_PERCENT) &&
		    (root->width.computed >= 1.0)) {
			root->width.set = TRUE;
			root->width.unit = unit;
		} else {
			root->width.set = FALSE;
			root->width.unit = SP_SVG_UNIT_NONE;
			root->width.computed = SP_SVG_DEFAULT_WIDTH_PX;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
	} else if (!strcmp (key, "height")) {
		if (sp_svg_length_read_lff (str, &unit, &root->height.value, &root->height.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX) &&
		    (unit != SP_SVG_UNIT_PERCENT) &&
		    (root->height.computed >= 1.0)) {
			root->height.set = TRUE;
			root->height.unit = unit;
		} else {
			root->height.set = FALSE;
			root->height.unit = SP_SVG_UNIT_NONE;
			root->height.computed = SP_SVG_DEFAULT_WIDTH_PX;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
	} else if (!strcmp (key, "viewBox")) {
		/* fixme: We have to take original item affine into account */
		/* fixme: Think (Lauris) */
		gdouble x, y, width, height;
		gchar *eptr;

		if (!str) return;
		eptr = (gchar *) str;
		x = strtod (eptr, &eptr);
		while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
		y = strtod (eptr, &eptr);
		while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
		width = strtod (eptr, &eptr);
		while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
		height = strtod (eptr, &eptr);
		while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
		if ((width > 0) && (height > 0)) {
			root->viewbox.c[0] = root->width.computed / width;
			root->viewbox.c[1] = 0.0;
			root->viewbox.c[2] = 0.0;
			root->viewbox.c[3] = root->height.computed / height;
			root->viewbox.c[4] = -x * root->viewbox.c[0];
			root->viewbox.c[5] = -y * root->viewbox.c[3];
			for (v = item->display; v != NULL; v = v->next) {
				nr_arena_group_set_child_transform (NR_ARENA_GROUP (v->arenaitem), &root->viewbox);
			}
			sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		}
	} else if (((SPObjectClass *) parent_class)->read_attr) {
		(* ((SPObjectClass *) parent_class)->read_attr) (object, key);
	}
}

static void
sp_root_child_added (SPObject *object, SPRepr *child, SPRepr *ref)
{
	SPRoot *root;
	SPGroup *group;
	SPObject *co;
	const gchar * id;

	root = (SPRoot *) object;
	group = (SPGroup *) object;

	if (((SPObjectClass *) (parent_class))->child_added)
		(* ((SPObjectClass *) (parent_class))->child_added) (object, child, ref);

	id = sp_repr_attr (child, "id");
	co = sp_document_lookup_id (object->document, id);
	g_assert (co != NULL);

	if (SP_IS_NAMEDVIEW (co)) {
		root->namedviews = g_slist_append (root->namedviews, co);
	} else if (SP_IS_DEFS (co)) {
		SPObject *c;
		/* We search for first <defs> node - it is not beautiful, but works */
		for (c = group->children; c != NULL; c = c->next) {
			if (SP_IS_DEFS (c)) {
				root->defs = SP_DEFS (c);
				break;
			}
		}
	}
}

static void
sp_root_remove_child (SPObject * object, SPRepr * child)
{
	SPRoot * root;
	SPObject * co;
	const gchar * id;

	root = (SPRoot *) object;

	id = sp_repr_attr (child, "id");
	co = sp_document_lookup_id (object->document, id);
	g_assert (co != NULL);

	if (SP_IS_NAMEDVIEW (co)) {
		root->namedviews = g_slist_remove (root->namedviews, co);
	} else if (SP_IS_DEFS (co) && root->defs == (SPDefs *) co) {
		SPObject *c;
		/* We search for next <defs> node - it is not beautiful, but works */
		for (c = co->next; c != NULL; c = c->next) {
			if (SP_IS_DEFS (c)) break;
		}
		root->defs = SP_DEFS (c);
	}

	if (((SPObjectClass *) (parent_class))->remove_child)
		(* ((SPObjectClass *) (parent_class))->remove_child) (object, child);
}

static void
sp_root_modified (SPObject *object, guint flags)
{
	SPRoot *root;

	root = SP_ROOT (object);

	if (((SPObjectClass *) (parent_class))->modified)
		(* ((SPObjectClass *) (parent_class))->modified) (object, flags);

	if (flags & SP_OBJECT_VIEWPORT_MODIFIED_FLAG) {
		sp_document_set_size_px (SP_OBJECT_DOCUMENT (root), root->width.computed, root->height.computed);
	}
}

static SPRepr *
sp_root_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPRoot *root;

	root = SP_ROOT (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("svg");
	}

	sp_repr_set_attr (repr, "xmlns", "http://www.w3.org/2000/svg");
	sp_repr_set_attr (repr, "xmlns:xlink", "http://www.w3.org/1999/xlink");

	if (flags & SP_OBJECT_WRITE_SODIPODI) {
		sp_repr_set_attr (repr, "xmlns:sodipodi", "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd");
		sp_repr_set_double (repr, "sodipodi:version", (double) root->sodipodi / 100.0);
	}

	sp_repr_set_double (repr, "width", root->width.computed);
	sp_repr_set_double (repr, "height", root->height.computed);
	sp_repr_set_attr (repr, "viewBox", sp_repr_attr (object->repr, "viewBox"));

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

static NRArenaItem *
sp_root_show (SPItem *item, NRArena *arena)
{
	SPRoot *root;
	NRArenaItem *ai;

	root = SP_ROOT (item);

	if (((SPItemClass *) (parent_class))->show) {
		ai = ((SPItemClass *) (parent_class))->show (item, arena);
		if (ai) {
			nr_arena_group_set_child_transform (NR_ARENA_GROUP (ai), &root->viewbox);
		}
	} else {
		ai = NULL;
	}

	return ai;
}

static void
sp_root_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags)
{
	SPRoot *root;
	NRMatrixD a[6];

	root = SP_ROOT (item);

	nr_matrix_multiply_ddd (a, &root->viewbox, transform);

	if (((SPItemClass *) (parent_class))->bbox) {
		((SPItemClass *) (parent_class))->bbox (item, bbox, a, flags);
	}
}

static void
sp_root_print (SPItem *item, SPPrintContext *ctx)
{
	SPRoot *root;
	NRMatrixF t;

	root = SP_ROOT (item);

	t.c[0] = root->viewbox.c[0];
	t.c[1] = root->viewbox.c[1];
	t.c[2] = root->viewbox.c[2];
	t.c[3] = root->viewbox.c[3];
	t.c[4] = root->viewbox.c[4];
	t.c[5] = root->viewbox.c[5];
	sp_print_bind (ctx, &t, 1.0);

	if (((SPItemClass *) (parent_class))->print) {
		((SPItemClass *) (parent_class))->print (item, ctx);
	}

	sp_print_release (ctx);
}
