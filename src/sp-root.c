#define __SP_ROOT_C__

/*
 * SVG <svg> element
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#include <string.h>
#include "svg/svg.h"
#include "display/nr-arena-item.h"
#include "document.h"
#include "desktop.h"
#include "sp-defs.h"
#include "sp-namedview.h"
#include "sp-root.h"

#define SP_SVG_DEFAULT_WIDTH 595.27
#define SP_SVG_DEFAULT_HEIGHT 841.89

static void sp_root_class_init (SPRootClass *klass);
static void sp_root_init (SPRoot *root);
static void sp_root_destroy (GtkObject *object);

static void sp_root_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_root_read_attr (SPObject *object, const gchar *key);
static void sp_root_child_added (SPObject *object, SPRepr *child, SPRepr *ref);
static void sp_root_remove_child (SPObject *object, SPRepr *child);
static void sp_root_modified (SPObject *object, guint flags);

static SPGroupClass *parent_class;

GtkType
sp_root_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPRoot",
			sizeof (SPRoot),
			sizeof (SPRootClass),
			(GtkClassInitFunc) sp_root_class_init,
			(GtkObjectInitFunc) sp_root_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_GROUP, &info);
	}
	return type;
}

static void
sp_root_class_init (SPRootClass *klass)
{
	GtkObjectClass *gtk_object_class;
	SPObjectClass *sp_object_class;

	gtk_object_class = GTK_OBJECT_CLASS (klass);
	sp_object_class = SP_OBJECT_CLASS (klass);

	parent_class = gtk_type_class (SP_TYPE_GROUP);

	gtk_object_class->destroy = sp_root_destroy;

	sp_object_class->build = sp_root_build;
	sp_object_class->read_attr = sp_root_read_attr;
	sp_object_class->child_added = sp_root_child_added;
	sp_object_class->remove_child = sp_root_remove_child;
	sp_object_class->modified = sp_root_modified;
}

static void
sp_root_init (SPRoot *root)
{
	root->group.transparent = TRUE;

	root->resized = FALSE;

	root->width = SP_SVG_DEFAULT_WIDTH;
	root->height = SP_SVG_DEFAULT_HEIGHT;
	root->viewbox.x0 = root->viewbox.y0 = 0.0;
	root->viewbox.x1 = root->width;
	root->viewbox.y1 = root->height;

	root->namedviews = NULL;
	root->defs = NULL;
}

static void
sp_root_destroy (GtkObject *object)
{
	SPRoot * root;

	root = (SPRoot *) object;

	root->defs = NULL;
	g_slist_free (root->namedviews);
	root->namedviews = NULL;

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_root_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPGroup * group;
	SPRoot * root;
	SPObject * o;

	group = (SPGroup *) object;
	root = (SPRoot *) object;

	if (((SPObjectClass *) parent_class)->build)
		(* ((SPObjectClass *) parent_class)->build) (object, document, repr);

	sp_root_read_attr (object, "width");
	sp_root_read_attr (object, "height");
	sp_root_read_attr (object, "viewBox");

	/* Collect all out namedviews */
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
sp_root_read_attr (SPObject * object, const gchar * key)
{
	SPItem * item;
	SPRoot * root;
	const gchar * astr;
	const SPUnit *unit;
	gdouble len;
	SPItemView * v;

	item = SP_ITEM (object);
	root = SP_ROOT (object);

	astr = sp_repr_attr (object->repr, key);

	if (!strcmp (key, "width")) {
		len = sp_svg_read_length (&unit, astr, 0.0);
		if (len >= 1.0) {
			root->resized = TRUE;
			root->width = len;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "height")) {
		len = sp_svg_read_length (&unit, astr, 0.0);
		if (len >= 1.0) {
			root->height = len;
			root->resized = TRUE;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "viewBox")) {
		/* fixme: We have to take original item affine into account */
		gdouble x, y, width, height;
		gchar * eptr;
		gdouble t0[6], s[6], a[6];

		if (!astr) return;
		eptr = (gchar *) astr;
		x = strtod (eptr, &eptr);
		while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
		y = strtod (eptr, &eptr);
		while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
		width = strtod (eptr, &eptr);
		while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
		height = strtod (eptr, &eptr);
		while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
		if ((width > 0) && (height > 0)) {
			root->viewbox.x0 = x;
			root->viewbox.y0 = y;
			root->viewbox.x1 = x + width;
			root->viewbox.y1 = y + height;
			art_affine_translate (t0, x, y);
			art_affine_scale (s, root->width / width, root->height / height);
			art_affine_multiply (a, t0, s);
			memcpy (item->affine, a, 6 * sizeof (gdouble));
			for (v = item->display; v != NULL; v = v->next) {
				nr_arena_item_set_transform (v->arenaitem, item->affine);
			}
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

	if (root->resized) {
		sp_document_set_size (SP_OBJECT_DOCUMENT (root), root->width, root->height);
		root->resized = FALSE;
	}
}

