#define __SP_DOCUMENT_C__

/*
 * SVG document implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define noSP_DOCUMENT_DEBUG_IDLE
#define noSP_DOCUMENT_DEBUG_UNDO

#include <config.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtkmain.h>
#include <xml/repr.h>
#include "helper/sp-marshal.h"
#include "helper/sp-intl.h"
#include "sodipodi-private.h"
#include "sp-object-repr.h"
#include "sp-root.h"
#include "sp-namedview.h"
#include "document-private.h"
#include "desktop.h"

#define SP_NAMESPACE_SVG "http://www.w3.org/2000/svg"
#define SP_NAMESPACE_SODIPODI "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
#define SP_NAMESPACE_XLINK "http://www.w3.org/1999/xlink"

#define A4_WIDTH_STR "210mm"
#define A4_HEIGHT_STR "297mm"

#define SP_DOCUMENT_UPDATE_PRIORITY GTK_PRIORITY_REDRAW

enum {
	MODIFIED,
	URI_SET,
	RESIZED,
	LAST_SIGNAL
};

static void sp_document_class_init (SPDocumentClass * klass);
static void sp_document_init (SPDocument *document);
static void sp_document_dispose (GObject *object);

static gint sp_document_idle_handler (gpointer data);

gboolean sp_document_resource_list_free (gpointer key, gpointer value, gpointer data);

#ifdef SP_DOCUMENT_DEBUG_UNDO
static gboolean sp_document_warn_undo_stack (SPDocument *doc);
#endif

static GObjectClass * parent_class;
static guint signals[LAST_SIGNAL] = {0};
static gint doc_count = 0;

unsigned int
sp_document_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPDocumentClass),
			NULL, NULL,
			(GClassInitFunc) sp_document_class_init,
			NULL, NULL,
			sizeof (SPDocument),
			4,
			(GInstanceInitFunc) sp_document_init,
		};
		type = g_type_register_static (G_TYPE_OBJECT, "SPDocument", &info, 0);
	}
	return type;
}

static void
sp_document_class_init (SPDocumentClass * klass)
{
	GObjectClass * object_class;

	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	signals[MODIFIED] = g_signal_new ("modified",
					    G_TYPE_FROM_CLASS(klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (SPDocumentClass, modified),
					    NULL, NULL,
					    sp_marshal_NONE__UINT,
					    G_TYPE_NONE, 1,
					    G_TYPE_UINT);
	signals[URI_SET] =    g_signal_new ("uri_set",
					    G_TYPE_FROM_CLASS(klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (SPDocumentClass, uri_set),
					    NULL, NULL,
					    sp_marshal_NONE__POINTER,
					    G_TYPE_NONE, 1,
					    G_TYPE_POINTER);
	signals[RESIZED] =    g_signal_new ("resized",
					    G_TYPE_FROM_CLASS(klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (SPDocumentClass, uri_set),
					    NULL, NULL,
					    sp_marshal_NONE__DOUBLE_DOUBLE,
					    G_TYPE_NONE, 2,
					    G_TYPE_DOUBLE, G_TYPE_DOUBLE);
	object_class->dispose = sp_document_dispose;
}

static void
sp_document_init (SPDocument *doc)
{
	SPDocumentPrivate *p;

	doc->advertize = FALSE;
	doc->keepalive = FALSE;

	doc->modified_id = 0;

	doc->rdoc = NULL;
	doc->rroot = NULL;
	doc->root = NULL;

	doc->uri = NULL;
	doc->base = NULL;
	doc->name = NULL;

	p = g_new (SPDocumentPrivate, 1);


	p->iddef = g_hash_table_new (g_str_hash, g_str_equal);

	p->resources = g_hash_table_new (g_str_hash, g_str_equal);

	p->sensitive = FALSE;
	p->undo = NULL;
	p->redo = NULL;
	p->actions = NULL;

	doc->private = p;
}

static void
sp_document_dispose (GObject *object)
{
	SPDocument *doc;
	SPDocumentPrivate * private;

	doc = (SPDocument *) object;
	private = doc->private;

	if (private) {
		sodipodi_remove_document (doc);

		if (private->actions) {
			sp_action_free_list (private->actions);
		}

		sp_document_clear_redo (doc);
		sp_document_clear_undo (doc);

		if (doc->root) {
			sp_object_invoke_release (doc->root);
			g_object_unref (G_OBJECT (doc->root));
			doc->root = NULL;
		}

		if (private->iddef) g_hash_table_destroy (private->iddef);

		if (doc->rdoc) sp_repr_document_unref (doc->rdoc);

		/* Free resources */
		g_hash_table_foreach_remove (private->resources, sp_document_resource_list_free, doc);
		g_hash_table_destroy (private->resources);

		g_free (private);
		doc->private = NULL;
	}

	if (doc->name) {
		g_free (doc->name);
		doc->name = NULL;
	}
	if (doc->base) {
		g_free (doc->base);
		doc->base = NULL;
	}
	if (doc->uri) {
		g_free (doc->uri);
		doc->uri = NULL;
	}

	if (doc->modified_id) {
		gtk_idle_remove (doc->modified_id);
		doc->modified_id = 0;
	}

	if (doc->keepalive) {
		sodipodi_unref ();
		doc->keepalive = FALSE;
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static SPDocument *
sp_document_create (SPReprDoc *rdoc,
		    const unsigned char *uri,
		    const unsigned char *base,
		    const unsigned char *name,
		    unsigned int advertize,
		    unsigned int keepalive)
{
	SPDocument *document;
	SPRepr *rroot;
	guint version;

	rroot = sp_repr_document_root (rdoc);

	document = g_object_new (SP_TYPE_DOCUMENT, NULL);

	document->advertize = advertize;
	document->keepalive = keepalive;

	document->rdoc = rdoc;
	document->rroot = rroot;

	document->uri = g_strdup (uri);
	document->base = g_strdup (base);
	document->name = g_strdup (name);

	document->root = sp_object_repr_build_tree (document, rroot);

#if 0
	/* fixme: Is this correct here? (Lauris) */
	sp_document_ensure_up_to_date (document);
#endif

	version = SP_ROOT (document->root)->sodipodi;

	/* fixme: Not sure about this, but lets assume ::build updates */
	sp_repr_set_attr (rroot, "sodipodi:version", VERSION);
	/* fixme: Again, I moved these here to allow version determining in ::build (Lauris) */
	/* A quick hack to get namespaces into doc */
	sp_repr_set_attr (rroot, "xmlns", SP_NAMESPACE_SVG);
	sp_repr_set_attr (rroot, "xmlns:sodipodi", SP_NAMESPACE_SODIPODI);
	sp_repr_set_attr (rroot, "xmlns:xlink", SP_NAMESPACE_XLINK);
	/* End of quick hack */
	/* Quick hack 2 - get default image size into document */
	if (!sp_repr_attr (rroot, "width")) sp_repr_set_attr (rroot, "width", A4_WIDTH_STR);
	if (!sp_repr_attr (rroot, "height")) sp_repr_set_attr (rroot, "height", A4_HEIGHT_STR);
	/* End of quick hack 2 */
	/* Quick hack 3 - Set uri attributes */
	if (uri) {
		/* fixme: Think, what this means for images (Lauris) */
		sp_repr_set_attr (rroot, "sodipodi:docname", uri);
		sp_repr_set_attr (rroot, "sodipodi:docbase", document->base);
	}
	/* End of quick hack 3 */
	if ((version > 0) && (version < 25)) {
		/* Clear ancient spec violating attributes */
		sp_repr_set_attr (rroot, "SP-DOCNAME", NULL);
		sp_repr_set_attr (rroot, "SP-DOCBASE", NULL);
		sp_repr_set_attr (rroot, "docname", NULL);
		sp_repr_set_attr (rroot, "docbase", NULL);
	}

	/* Namedviews */
	if (!SP_ROOT (document->root)->namedviews) {
		SPRepr *r;
		r = sodipodi_get_repr (SODIPODI, "template.sodipodi:namedview");
		if (!r) r = sp_repr_new ("sodipodi:namedview");
		sp_repr_set_attr (r, "id", "base");
		sp_repr_add_child (rroot, r, NULL);
		sp_repr_unref (r);
		g_assert (SP_ROOT (document->root)->namedviews);
	}

	/* Defs */
	if (!SP_ROOT (document->root)->defs) {
		SPRepr *r;
		r = sp_repr_new ("defs");
		sp_repr_add_child (rroot, r, NULL);
		sp_repr_unref (r);
		g_assert (SP_ROOT (document->root)->defs);
	}

	if (keepalive) {
		sodipodi_ref ();
	}

	sp_document_set_undo_sensitive (document, TRUE);

	sodipodi_add_document (document);

	return document;
}

SPDocument *
sp_document_new (const gchar *uri, unsigned int advertize, unsigned int keepalive)
{
	SPDocument *doc;
	SPReprDoc *rdoc;
	unsigned char *base, *name;

	if (uri) {
		SPRepr *rroot;
		unsigned char *s, *p;
		/* Try to fetch repr from file */
		rdoc = sp_repr_read_file (uri, SP_SVG_NS_URI);
		/* If file cannot be loaded, return NULL without warning */
		if (rdoc == NULL) return NULL;
		rroot = sp_repr_document_root (rdoc);
		/* If xml file is not svg, return NULL without warning */
		/* fixme: destroy document */
		if (strcmp (sp_repr_name (rroot), "svg") != 0) return NULL;
		s = g_strdup (uri);
		p = strrchr (s, '/');
		if (p) {
			name = g_strdup (p + 1);
			p[1] = '\0';
			base = g_strdup (s);
		} else {
			base = NULL;
			name = g_strdup (uri);
		}
		g_free (s);
	} else {
		rdoc = sp_repr_document_new ("svg");
		base = NULL;
		name = g_strdup_printf (_("New document %d"), ++doc_count);
	}

	doc = sp_document_create (rdoc, uri, base, name, advertize, keepalive);

	if (base) g_free (base);
	if (name) g_free (name);

	return doc;
}

SPDocument *
sp_document_new_from_mem (const gchar *buffer, gint length, unsigned int advertize, unsigned int keepalive)
{
	SPDocument *doc;
	SPReprDoc *rdoc;
	SPRepr *rroot;
	unsigned char *name;

	rdoc = sp_repr_read_mem (buffer, length, SP_SVG_NS_URI);

	/* If it cannot be loaded, return NULL without warning */
	if (rdoc == NULL) return NULL;

	rroot = sp_repr_document_root (rdoc);
	/* If xml file is not svg, return NULL without warning */
	/* fixme: destroy document */
	if (strcmp (sp_repr_name (rroot), "svg") != 0) return NULL;

	name = g_strdup_printf (_("Memory document %d"), ++doc_count);

	doc = sp_document_create (rdoc, NULL, NULL, name, advertize, keepalive);

	return doc;
}

SPDocument *
sp_document_ref (SPDocument *doc)
{
	g_return_val_if_fail (doc != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (doc), NULL);

	g_object_ref (G_OBJECT (doc));

	return doc;
}

SPDocument *
sp_document_unref (SPDocument *doc)
{
	g_return_val_if_fail (doc != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (doc), NULL);

	g_object_unref (G_OBJECT (doc));

	return NULL;
}

gdouble
sp_document_width (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, 0.0);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), 0.0);
	g_return_val_if_fail (document->private != NULL, 0.0);
	g_return_val_if_fail (document->root != NULL, 0.0);

	return SP_ROOT (document->root)->width.computed / 1.25;
}

gdouble
sp_document_height (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, 0.0);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), 0.0);
	g_return_val_if_fail (document->private != NULL, 0.0);
	g_return_val_if_fail (document->root != NULL, 0.0);

	return SP_ROOT (document->root)->height.computed / 1.25;
}

void
sp_document_set_uri (SPDocument *doc, const guchar *uri)
{
	g_return_if_fail (doc != NULL);
	g_return_if_fail (SP_IS_DOCUMENT (doc));

	if (doc->name) {
		g_free (doc->name);
		doc->name = NULL;
	}
	if (doc->base) {
		g_free (doc->base);
		doc->base = NULL;
	}
	if (doc->uri) {
		g_free (doc->uri);
		doc->uri = NULL;
	}

	if (uri) {
		guchar *s, *p;
		doc->uri = g_strdup (uri);
		/* fixme: Think, what this means for images (Lauris) */
		s = g_strdup (uri);
		p = strrchr (s, '/');
		if (p) {
			doc->name = g_strdup (p + 1);
			p[1] = '\0';
			doc->base = g_strdup (s);
		} else {
			doc->base = NULL;
			doc->name = g_strdup (doc->uri);
		}
		g_free (s);
	} else {
		doc->uri = g_strdup_printf (_("Unnamed document %d"), ++doc_count);
		doc->base = NULL;
		doc->name = g_strdup (doc->uri);
	}

	g_signal_emit (G_OBJECT (doc), signals [URI_SET], 0, doc->uri);
}

void
sp_document_set_size_px (SPDocument *doc, gdouble width, gdouble height)
{
	g_return_if_fail (doc != NULL);
	g_return_if_fail (SP_IS_DOCUMENT (doc));
	g_return_if_fail (width > 0.001);
	g_return_if_fail (height > 0.001);

	g_signal_emit (G_OBJECT (doc), signals [RESIZED], 0, width / 1.25, height / 1.25);
}

/* named views */

const GSList *
sp_document_namedview_list (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);

	return SP_ROOT (document->root)->namedviews;
}

SPNamedView *
sp_document_namedview (SPDocument * document, const gchar * id)
{
	const GSList * nvl, * l;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);

	nvl = SP_ROOT (document->root)->namedviews;
	g_assert (nvl != NULL);

	if (id == NULL) return SP_NAMEDVIEW (nvl->data);

	for (l = nvl; l != NULL; l = l->next) {
		if (strcmp (id, SP_OBJECT (l->data)->id) == 0)
			return SP_NAMEDVIEW (l->data);
	}

	return NULL;
}

void
sp_document_def_id (SPDocument * document, const gchar * id, SPObject * object)
{
	g_assert (g_hash_table_lookup (document->private->iddef, id) == NULL);

	g_hash_table_insert (document->private->iddef, (gchar *) id, object);
}

void
sp_document_undef_id (SPDocument * document, const gchar * id)
{
	g_assert (g_hash_table_lookup (document->private->iddef, id) != NULL);

	g_hash_table_remove (document->private->iddef, id);
}

SPObject *
sp_document_lookup_id (SPDocument *doc, const gchar *id)
{
	g_return_val_if_fail (doc != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (doc), NULL);
	g_return_val_if_fail (id != NULL, NULL);

	return g_hash_table_lookup (doc->private->iddef, id);
}

/* Object modification root handler */

void
sp_document_request_modified (SPDocument *doc)
{
	if (!doc->modified_id) {
		doc->modified_id = gtk_idle_add_priority (SP_DOCUMENT_UPDATE_PRIORITY, sp_document_idle_handler, doc);
	}
}

gint
sp_document_ensure_up_to_date (SPDocument *doc)
{
	if (doc->modified_id) {
		/* Remove handler */
		gtk_idle_remove (doc->modified_id);
		doc->modified_id = 0;
		/* Process updates */
		if (SP_OBJECT_FLAGS (doc->root) & SP_OBJECT_UPDATE_FLAG) {
			SPItemCtx ctx;
			ctx.ctx.flags = 0;
			nr_matrix_d_set_identity (&ctx.i2doc);
			/* Set up viewport in case svg has it defined as percentages */
			ctx.vp.x0 = 0.0;
			ctx.vp.y0 = 0.0;
			ctx.vp.x1 = 21.0 / 2.54 * 72.0 * 1.25;
			ctx.vp.y1 = 29.7 / 2.54 * 72.0 * 1.25;
			nr_matrix_d_set_identity (&ctx.i2vp);
			sp_object_invoke_update (doc->root, (SPCtx *) &ctx, 0);
			g_assert (!(SP_OBJECT_FLAGS (doc->root) & SP_OBJECT_UPDATE_FLAG));
		}
		/* Emit "modified" signal on objects */
		sp_object_invoke_modified (doc->root, 0);
		/* Emit our own "modified" signal */
		g_signal_emit (G_OBJECT (doc), signals [MODIFIED], 0,
			       SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG);
		return TRUE;
	}

	return FALSE;
}

static gint
sp_document_idle_handler (gpointer data)
{
	SPDocument *doc;

	doc = SP_DOCUMENT (data);

	doc->modified_id = 0;

#ifdef SP_DOCUMENT_DEBUG_UNDO
	/* ------------------------- */
	if (doc->private->actions) {
		static gboolean warn = TRUE;
		if (warn) {
			warn = sp_document_warn_undo_stack (doc);
		}
	}
	/* ------------------------- */
#endif

#ifdef SP_DOCUMENT_DEBUG_IDLE
	g_print ("->\n");
#endif

	/* Process updates */
	if (SP_OBJECT_FLAGS (doc->root) & SP_OBJECT_UPDATE_FLAG) {
		SPItemCtx ctx;
		ctx.ctx.flags = 0;
		nr_matrix_d_set_identity (&ctx.i2doc);
		/* Set up viewport in case svg has it defined as percentages */
		ctx.vp.x0 = 0.0;
		ctx.vp.y0 = 0.0;
		ctx.vp.x1 = 21.0 / 2.54 * 72.0 * 1.25;
		ctx.vp.y1 = 29.7 / 2.54 * 72.0 * 1.25;
		nr_matrix_d_set_identity (&ctx.i2vp);
		sp_object_invoke_update (doc->root, (SPCtx *) &ctx, 0);
		g_assert (!(SP_OBJECT_FLAGS (doc->root) & SP_OBJECT_UPDATE_FLAG));
	}
	/* Emit "modified" signal on objects */
	sp_object_invoke_modified (doc->root, 0);

#ifdef SP_DOCUMENT_DEBUG_IDLE
	g_print ("\n->");
#endif

	/* Emit our own "modified" signal */
	g_signal_emit (G_OBJECT (doc), signals [MODIFIED], 0,
			 SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG);

#ifdef SP_DOCUMENT_DEBUG_IDLE
	g_print (" S ->\n");
#endif
	return FALSE;
}

SPObject *
sp_document_add_repr (SPDocument *document, SPRepr *repr)
{
	GType type;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (repr != NULL, NULL);

	type = sp_repr_type_lookup (repr);

	if (g_type_is_a (type, SP_TYPE_ITEM)) {
		sp_repr_append_child (sp_document_repr_root(document), repr);
	} else if (g_type_is_a (type, SP_TYPE_OBJECT)) {
		sp_repr_append_child (SP_OBJECT_REPR (SP_DOCUMENT_DEFS(document)), repr);
	}

	return sp_document_lookup_id (document, sp_repr_attr (repr, "id"));
}

/*
 * Return list of items, contained in box
 *
 * Assumes box is normalized (and g_asserts it!)
 *
 */

GSList *
sp_document_items_in_box (SPDocument *document, NRRectD *box)
{
	SPGroup * group;
	SPItem * child;
	SPObject * o;
	NRRectF b;
	GSList * s;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (document->private != NULL, NULL);
	g_return_val_if_fail (box != NULL, NULL);

	group = SP_GROUP (document->root);

	s = NULL;

	for (o = group->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			sp_item_bbox_desktop (child, &b);
			if ((b.x0 > box->x0) && (b.x1 < box->x1) &&
			    (b.y0 > box->y0) && (b.y1 < box->y1)) {
				s = g_slist_append (s, child);
			}
		}
	}

	return s;
}

/*
 * Return list of items, that the parts of the item contained in box
 *
 * Assumes box is normalized (and g_asserts it!)
 *
 */

GSList *
sp_document_partial_items_in_box (SPDocument *document, NRRectD *box)
{
	SPGroup * group;
	SPItem * child;
	SPObject * o;
	NRRectF b;
	GSList * s;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (document->private != NULL, NULL);
	g_return_val_if_fail (box != NULL, NULL);

	group = SP_GROUP (document->root);

	s = NULL;

	for (o = group->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			sp_item_bbox_desktop (child, &b);
			if ((((b.x0 > box->x0) && (b.x0 < box->x1)) ||
			     ((b.x1 > box->x0) && (b.x1 < box->x1)))
			    &&
			    (((b.y0 > box->y0) && (b.y0 < box->y1)) ||
			     ((b.y1 > box->y0) && (b.y1 < box->y1)))) {
				s = g_slist_append (s, child);
			}
		}
	}

	return s;
}

/* Resource management */

gboolean
sp_document_add_resource (SPDocument *document, const guchar *key, SPObject *object)
{
	GSList *rlist;

	g_return_val_if_fail (document != NULL, FALSE);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (*key != '\0', FALSE);
	g_return_val_if_fail (object != NULL, FALSE);
	g_return_val_if_fail (SP_IS_OBJECT (object), FALSE);

	rlist = g_hash_table_lookup (document->private->resources, key);
	g_return_val_if_fail (!g_slist_find (rlist, object), FALSE);
	rlist = g_slist_prepend (rlist, object);
	g_hash_table_insert (document->private->resources, (gpointer) key, rlist);

	return TRUE;
}

gboolean
sp_document_remove_resource (SPDocument *document, const guchar *key, SPObject *object)
{
	GSList *rlist;

	g_return_val_if_fail (document != NULL, FALSE);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (*key != '\0', FALSE);
	g_return_val_if_fail (object != NULL, FALSE);
	g_return_val_if_fail (SP_IS_OBJECT (object), FALSE);

	rlist = g_hash_table_lookup (document->private->resources, key);
	g_return_val_if_fail (rlist != NULL, FALSE);
	g_return_val_if_fail (g_slist_find (rlist, object), FALSE);
	rlist = g_slist_remove (rlist, object);
	g_hash_table_insert (document->private->resources, (gpointer) key, rlist);

	return TRUE;
}

const GSList *
sp_document_get_resource_list (SPDocument *document, const guchar *key)
{
	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (*key != '\0', NULL);

	return g_hash_table_lookup (document->private->resources, key);
}

/* Helpers */

gboolean
sp_document_resource_list_free (gpointer key, gpointer value, gpointer data)
{
	g_slist_free ((GSList *) value);
	return TRUE;
}

#ifdef SP_DOCUMENT_DEBUG_UNDO

static GSList *
sp_action_print_pending_list (SPAction *action)
{
	GSList *al;
	gchar *s;

	al = NULL;

	while (action) {
		s = g_strdup_printf ("SPAction: Id %s\n", action->id);
		al = g_slist_prepend (al, s);
		switch (action->type) {
		case SP_ACTION_ADD:
			s = g_strdup_printf ("SPAction: Add ref %s\n", action->act.add.ref);
			al = g_slist_prepend (al, s);
			break;
		case SP_ACTION_DEL:
			s = g_strdup_printf ("SPAction: Del ref %s\n", action->act.del.ref);
			al = g_slist_prepend (al, s);
			break;
		case SP_ACTION_CHGATTR:
			s = g_strdup_printf ("SPAction: ChgAttr %s: %s -> %s\n",
				 g_quark_to_string (action->act.chgattr.key),
				 action->act.chgattr.oldval,
				 action->act.chgattr.newval);
			al = g_slist_prepend (al, s);
			break;
		case SP_ACTION_CHGCONTENT:
			s = g_strdup_printf ("SPAction: ChgContent %s -> %s\n",
				 action->act.chgcontent.oldval,
				 action->act.chgcontent.newval);
			al = g_slist_prepend (al, s);
			break;
		case SP_ACTION_CHGORDER:
			s = g_strdup_printf ("SPAction: ChgOrder %s: %s -> %s\n",
				 action->act.chgorder.child,
				 action->act.chgorder.oldref,
				 action->act.chgorder.newref);
			al = g_slist_prepend (al, s);
			break;
		default:
			s = g_strdup_printf ("SPAction: Invalid action type %d\n", action->type);
			al = g_slist_prepend (al, s);
			break;
		}
		action = action->next;
	}

	return al;
}

static gboolean
sp_document_warn_undo_stack (SPDocument *doc)
{
	GtkDialog *dlg;
	GtkWidget *l, *t;
	GSList *al;

	dlg = (GtkDialog *) gtk_dialog_new_with_buttons ("Stale undo stack warning",
					    NULL,
					    GTK_DIALOG_MODAL,
					    GTK_STOCK_OK,
					    GTK_RESPONSE_OK,
					    NULL);

	l = gtk_label_new ("WARNING");
	gtk_widget_show (l);
	gtk_box_pack_start (GTK_BOX (dlg->vbox), l, FALSE, FALSE, 8);

	l = gtk_label_new ("Last operation did not flush undo stack");
	gtk_widget_show (l);
	gtk_box_pack_start (GTK_BOX (dlg->vbox), l, FALSE, FALSE, 8);

	l = gtk_label_new ("Please report following data to developers");
	gtk_widget_show (l);
	gtk_box_pack_start (GTK_BOX (dlg->vbox), l, FALSE, FALSE, 8);

	t = gtk_text_new (NULL, NULL);
	gtk_widget_show (t);
	gtk_box_pack_start (GTK_BOX (dlg->vbox), t, FALSE, FALSE, 8);

	al = sp_action_print_pending_list (doc->private->actions);
	while (al) {
		gtk_text_set_point (GTK_TEXT (t), 0);
		gtk_text_insert (GTK_TEXT (t), NULL, NULL, NULL, al->data, strlen (al->data));
		g_free (al->data);
		al = g_slist_remove (al, al->data);
	}

	gtk_dialog_run (dlg);
	gtk_widget_destroy (GTK_WIDGET(dlg));

	return TRUE;
}

#endif
