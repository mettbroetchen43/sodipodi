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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <gtk/gtkmain.h>
#include <xml/repr.h>

#include "helper/sp-marshal.h"
#include "helper/sp-intl.h"

#include "api.h"
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

#define SP_DOCUMENT_UPDATE_PRIORITY (G_PRIORITY_HIGH_IDLE - 1)

enum {
	DESTROY,
	MODIFIED,
	URI_SET,
	RESIZED,
	OBJECT_ADDED,
	OBJECT_REMOVED,
	ORDER_CHANGED,
	OBJECT_MODIFIED,
	BEGIN,
	END,
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

/* Schedule either idle or timeout */
static void sp_document_schedule_timers (SPDocument *doc);

static GObjectClass * parent_class;
static guint signals[LAST_SIGNAL] = {0};
static gint doc_count = 0;

GType
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

	signals[DESTROY] =   g_signal_new ("destroy",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (SPDocumentClass, destroy),
					    NULL, NULL,
					    sp_marshal_NONE__NONE,
					    G_TYPE_NONE, 0);
	signals[MODIFIED] = g_signal_new ("modified",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (SPDocumentClass, modified),
					    NULL, NULL,
					    sp_marshal_NONE__UINT,
					    G_TYPE_NONE, 1,
					    G_TYPE_UINT);
	signals[URI_SET] =    g_signal_new ("uri_set",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (SPDocumentClass, uri_set),
					    NULL, NULL,
					    sp_marshal_NONE__POINTER,
					    G_TYPE_NONE, 1,
					    G_TYPE_POINTER);
	signals[RESIZED] =    g_signal_new ("resized",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (SPDocumentClass, uri_set),
					    NULL, NULL,
					    sp_marshal_NONE__DOUBLE_DOUBLE,
					    G_TYPE_NONE, 2,
					    G_TYPE_DOUBLE, G_TYPE_DOUBLE);
	signals[OBJECT_ADDED] = g_signal_new ("object_added",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (SPDocumentClass, object_added),
					    NULL, NULL,
					    sp_marshal_NONE__OBJECT_OBJECT,
					    G_TYPE_NONE, 2,
					    G_TYPE_OBJECT, G_TYPE_OBJECT);
	signals[OBJECT_REMOVED] = g_signal_new ("object_removed",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (SPDocumentClass, object_removed),
					    NULL, NULL,
					    sp_marshal_NONE__OBJECT_OBJECT,
					    G_TYPE_NONE, 2,
					    G_TYPE_OBJECT, G_TYPE_OBJECT);
	signals[ORDER_CHANGED] = g_signal_new ("order_changed",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (SPDocumentClass, order_changed),
					    NULL, NULL,
					    sp_marshal_NONE__OBJECT_OBJECT_OBJECT,
					    G_TYPE_NONE, 3,
					    G_TYPE_OBJECT, G_TYPE_OBJECT, G_TYPE_OBJECT);
	signals[OBJECT_MODIFIED] = g_signal_new ("object_modified",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (SPDocumentClass, object_modified),
					    NULL, NULL,
					    sp_marshal_NONE__POINTER_UINT,
					    G_TYPE_NONE, 2,
					    G_TYPE_POINTER, G_TYPE_UINT);
	signals[BEGIN] =      g_signal_new ("begin",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (SPDocumentClass, begin),
					    NULL, NULL,
					    sp_marshal_NONE__DOUBLE,
					    G_TYPE_NONE, 1,
					    G_TYPE_DOUBLE);
	signals[END] =        g_signal_new ("end",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (SPDocumentClass, end),
					    NULL, NULL,
					    sp_marshal_NONE__DOUBLE,
					    G_TYPE_NONE, 1,
					    G_TYPE_DOUBLE);
	object_class->dispose = sp_document_dispose;
}

static void
sp_document_init (SPDocument *doc)
{
	doc->priv = g_new0 (SPDocumentPrivate, 1);

	doc->priv->iddef = g_hash_table_new (g_str_hash, g_str_equal);

	doc->priv->resources = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
sp_document_dispose (GObject *object)
{
	SPDocument *doc;
	SPDocumentPrivate * priv;

	doc = (SPDocument *) object;
	priv = doc->priv;

	g_signal_emit (G_OBJECT (doc), signals [DESTROY], 0);

	if (priv) {
		if (priv->idle) {
			/* Remove idle handler */
			gtk_idle_remove (priv->idle);
			priv->idle = 0;
		}
		if (priv->timeout) {
			/* Remove timer */
			gtk_timeout_remove (priv->timeout);
			priv->timeout = 0;
		}
		while (priv->itqueue) {
			/* free (priv->itqueue->data); */
			priv->itqueue = g_slist_remove (priv->itqueue, priv->itqueue->data);
		}
		while (priv->etqueue) {
			/* free (priv->etqueue->data); */
			priv->etqueue = g_slist_remove (priv->etqueue, priv->etqueue->data);
		}
		if (priv->timers) free (priv->timers);

		if (priv->partial) {
			sp_repr_free_log (priv->partial);
			priv->partial = NULL;
		}

		sp_document_clear_redo (doc);
		sp_document_clear_undo (doc);

		if (doc->root) {
			sp_object_invoke_release (doc->root);
			g_object_unref (G_OBJECT (doc->root));
			doc->root = NULL;
		}

		if (priv->iddef) g_hash_table_destroy (priv->iddef);

		if (doc->rdoc) sp_repr_doc_unref (doc->rdoc);

		/* Free resources */
		g_hash_table_foreach_remove (priv->resources, sp_document_resource_list_free, doc);
		g_hash_table_destroy (priv->resources);

		g_free (priv);
		doc->priv = NULL;
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

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static SPDocument *
sp_document_create (SPReprDoc *rdoc,
		    const unsigned char *uri,
		    const unsigned char *base,
		    const unsigned char *name)
{
	SPDocument *document;
	SPRepr *rroot;
	guint version;

	rroot = sp_repr_doc_get_root (rdoc);

	document = g_object_new (SP_TYPE_DOCUMENT, NULL);

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
	if (!sp_repr_get_attr (rroot, "width")) sp_repr_set_attr (rroot, "width", A4_WIDTH_STR);
	if (!sp_repr_get_attr (rroot, "height")) sp_repr_set_attr (rroot, "height", A4_HEIGHT_STR);
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
	if (!sp_item_group_get_child_by_name ((SPGroup *) document->root, NULL, "sodipodi:namedview")) {
		SPRepr *r;
		r = sp_config_node_get ("template.sodipodi:namedview", FALSE);
		if (!r || strcmp (sp_repr_get_name (r), "sodipodi:namedview")) {
			r = sp_repr_new ("sodipodi:namedview");
		} else {
			r = sp_repr_duplicate (r);
		}
		sp_repr_set_attr (r, "id", "base");
		sp_repr_add_child (rroot, r, NULL);
		sp_repr_unref (r);
	}

	/* Defs */
	if (!SP_ROOT (document->root)->defs) {
		SPRepr *r;
		r = sp_repr_new ("defs");
		sp_repr_add_child (rroot, r, NULL);
		sp_repr_unref (r);
		g_assert (SP_ROOT (document->root)->defs);
	}

	return document;
}

SPDocument *
sp_document_new (const gchar *uri)
{
	SPDocument *doc;
	SPReprDoc *rdoc;
	unsigned char *base, *name;

	if (uri) {
		SPRepr *rroot;
		unsigned char *s, *p;
		/* Try to fetch repr from file */
		rdoc = sp_repr_doc_new_from_file (uri, SP_SVG_NS_URI);
		/* If file cannot be loaded, return NULL without warning */
		if (rdoc == NULL) return NULL;
		rroot = sp_repr_doc_get_root (rdoc);
		/* If xml file is not svg, return NULL without warning */
		/* fixme: destroy document */
		if (strcmp (sp_repr_get_name (rroot), "svg") != 0) return NULL;
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
		rdoc = sp_repr_doc_new ("svg");
		base = NULL;
		name = g_strdup_printf (_("New document %d"), ++doc_count);
	}

	doc = sp_document_create (rdoc, uri, base, name);


	if (!uri) {
		/* Quick hack - set xml:space */
		sp_repr_set_attr (doc->rroot, "xml:space", "preserve");
		/* End of quick hack 3 */
	}

	if (base) g_free (base);
	if (name) g_free (name);

	return doc;
}

SPDocument *
sp_document_new_from_mem (const gchar *buffer, gint length)
{
	SPDocument *doc;
	SPReprDoc *rdoc;
	SPRepr *rroot;
	unsigned char *name;

	rdoc = sp_repr_doc_new_from_mem (buffer, length, SP_SVG_NS_URI);

	/* If it cannot be loaded, return NULL without warning */
	if (rdoc == NULL) return NULL;

	rroot = sp_repr_doc_get_root (rdoc);
	/* If xml file is not svg, return NULL without warning */
	/* fixme: destroy document */
	if (strcmp (sp_repr_get_name (rroot), "svg") != 0) return NULL;

	name = g_strdup_printf (_("Memory document %d"), ++doc_count);

	doc = sp_document_create (rdoc, NULL, NULL, name);

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

/* Tree mutation signals */

void
sp_document_set_object_signals (SPDocument *doc, unsigned int enable)
{
	if (enable) {
		doc->object_signals += 1;
	} else {
		g_return_if_fail (doc->object_signals > 0);
		doc->object_signals -= 1;
	}
}

void
sp_document_invoke_object_added (SPDocument *doc, SPObject *parent, SPObject *ref)
{
	g_signal_emit (G_OBJECT (doc), signals [OBJECT_ADDED], 0, parent, ref);
}

void
sp_document_invoke_object_removed (SPDocument *doc, SPObject *parent, SPObject *ref)
{
	g_signal_emit (G_OBJECT (doc), signals [OBJECT_REMOVED], 0, parent, ref);
}

void
sp_document_invoke_order_changed (SPDocument *doc, SPObject *parent, SPObject *oldref, SPObject *ref)
{
	g_signal_emit (G_OBJECT (doc), signals [ORDER_CHANGED], 0, parent, oldref, ref);
}

void
sp_document_invoke_object_modified (SPDocument *doc, SPObject *object, unsigned int flags)
{
	g_signal_emit (G_OBJECT (doc), signals [OBJECT_MODIFIED], 0, object, flags);
}

gdouble
sp_document_width (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, 0.0);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), 0.0);
	g_return_val_if_fail (document->priv != NULL, 0.0);
	g_return_val_if_fail (document->root != NULL, 0.0);

	return SP_ROOT (document->root)->group.width.computed / 1.25;
}

gdouble
sp_document_height (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, 0.0);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), 0.0);
	g_return_val_if_fail (document->priv != NULL, 0.0);
	g_return_val_if_fail (document->root != NULL, 0.0);

	return SP_ROOT (document->root)->group.height.computed / 1.25;
}

const unsigned char *
sp_document_get_uri (SPDocument *doc)
{
	return doc->uri;
}

const unsigned char *
sp_document_get_name (SPDocument *doc)
{
	return doc->name;
}

SPReprDoc *
sp_document_get_repr_doc (SPDocument *doc)
{
	return doc->rdoc;
}

SPRepr *
sp_document_get_repr_root (SPDocument *doc)
{
	return doc->rroot;
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

void
sp_document_def_id (SPDocument * document, const gchar * id, SPObject * object)
{
	g_assert (g_hash_table_lookup (document->priv->iddef, id) == NULL);

	g_hash_table_insert (document->priv->iddef, (gchar *) id, object);
}

void
sp_document_undef_id (SPDocument * document, const gchar * id)
{
	g_assert (g_hash_table_lookup (document->priv->iddef, id) != NULL);

	g_hash_table_remove (document->priv->iddef, id);
}

SPObject *
sp_document_get_object_from_id (SPDocument *doc, const unsigned char *id)
{
	g_return_val_if_fail (doc != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (doc), NULL);
	g_return_val_if_fail (id != NULL, NULL);

	return g_hash_table_lookup (doc->priv->iddef, id);
}

SPObject *
sp_document_get_object_from_repr (SPDocument *doc, SPRepr *repr)
{
	const unsigned char *id;
	g_return_val_if_fail (doc != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (doc), NULL);
	g_return_val_if_fail (repr != NULL, NULL);

	id = sp_repr_get_attr (repr, "id");
	return sp_document_get_object_from_id (doc, id);
}

/* Object modification root handler */

void
sp_document_request_modified (SPDocument *doc)
{
	if (!doc->priv->idle) {
		sp_document_schedule_timers (doc);
	}
}

gint
sp_document_ensure_up_to_date (SPDocument *doc)
{
	int lc;
	lc = 16;
	while (doc->root->uflags || doc->root->mflags) {
		lc -= 1;
		if (lc < 0) {
			g_warning ("More than 16 iterations while updating document '%s'", doc->uri);
			/* doc->priv->need_update = 0; */
			/* Idle will be scheduled, but rien a faire */
			sp_document_schedule_timers (doc);
			return FALSE;
		}
		/* Process updates */
		if (doc->root->uflags) {
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
		}
		/* Emit "modified" signal on objects */
		sp_object_invoke_modified (doc->root, 0);
		/* Emit our own "modified" signal */
		g_signal_emit (G_OBJECT (doc), signals [MODIFIED], 0,
			       SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG);
	}
	sp_document_schedule_timers (doc);
	return TRUE;
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

	return sp_document_lookup_id (document, sp_repr_get_attr (repr, "id"));
}

/* Returns the sequence number of object */

unsigned int
sp_document_object_sequence_get (SPDocument *doc, SPObject *object)
{
	unsigned int seq;

	seq = 0;

	sp_object_invoke_sequence (doc->root, object, &seq);

	return seq;
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

	rlist = g_hash_table_lookup (document->priv->resources, key);
	g_return_val_if_fail (!g_slist_find (rlist, object), FALSE);
	rlist = g_slist_prepend (rlist, object);
	g_hash_table_insert (document->priv->resources, (gpointer) key, rlist);

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

	rlist = g_hash_table_lookup (document->priv->resources, key);
	g_return_val_if_fail (rlist != NULL, FALSE);
	g_return_val_if_fail (g_slist_find (rlist, object), FALSE);
	rlist = g_slist_remove (rlist, object);
	g_hash_table_insert (document->priv->resources, (gpointer) key, rlist);

	return TRUE;
}

const GSList *
sp_document_get_resource_list (SPDocument *document, const guchar *key)
{
	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (*key != '\0', NULL);

	return g_hash_table_lookup (document->priv->resources, key);
}

/* Helpers */

gboolean
sp_document_resource_list_free (gpointer key, gpointer value, gpointer data)
{
	g_slist_free ((GSList *) value);
	return TRUE;
}

guint
sp_document_get_version (SPDocument *document, guint version_type)
{
	SPRoot *root;

	g_return_val_if_fail (document != NULL, 0);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), 0);
	g_return_val_if_fail (document->root != NULL, 0);

	root = SP_ROOT (document->root);

	switch (version_type)
	{
	case SP_VERSION_SVG:
		return root->svg;
	case SP_VERSION_SODIPODI:
		return root->sodipodi;
	case SP_VERSION_ORIGINAL:
		return root->original;
	default:
		g_assert_not_reached ();
	}

	return 0;
}

/*
 * Timer
 *
 * Objects are required to use time services only from their parent document
 */

#ifdef WIN32
#include <sys/timeb.h>
#include <time.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

static double
sp_document_get_abstime (void)
{
#ifdef WIN32
        struct _timeb t;
        _ftime (&t);
        return (t.time + t.millitm / 1000.0);
#else
        struct timeval tv;
        struct timezone tz;

        gettimeofday (&tv, &tz);

        return (tv.tv_sec + tv.tv_usec / 1000000.0);
#endif
}

/* Begin document and start timer */
double
sp_document_begin (SPDocument *doc)
{
	g_return_val_if_fail (doc->begintime == 0.0, 0.0);
	doc->begintime = sp_document_get_abstime ();
	g_signal_emit (G_OBJECT (doc), signals [BEGIN], 0, doc->begintime);
	return doc->begintime;
}

/* End document and stop timer */
double
sp_document_end (SPDocument *doc)
{
	double endtime;
	g_return_val_if_fail (doc->begintime != 0.0, 0.0);
	endtime = sp_document_get_abstime ();
	g_signal_emit (G_OBJECT (doc), signals [BEGIN], 0, endtime);
	doc->begintime = 0.0;
	return endtime;
}

/* Get time in seconds from document begin */
double
sp_document_get_time (SPDocument *document)
{
	return sp_document_get_abstime () - document->begintime;
}

struct _SPDocTimer {
	double time;
	unsigned int flags : 1;
	unsigned int awake : 1;
	unsigned int (* callback) (double, void *);
	void *data;
};

gint
sp_doc_timer_compare (gconstpointer a, gconstpointer b)
{
	SPDocTimer *ta, *tb;
	ta = (SPDocTimer *) a;
	tb = (SPDocTimer *) b;
	if (ta->time < tb->time) return -1;
	if (ta->time > tb->time) return 1;
	return 0;
}

/* Create and set up timer */
unsigned int
sp_document_add_timer (SPDocument *doc, double time, unsigned int flags,
		       unsigned int (* callback) (double, void *),
		       void *data)
{
	SPDocumentPrivate *priv;
	SPDocTimer *timer;
	unsigned int id;
	priv = doc->priv;
	for (id = 0; id < priv->timers_size; id++) {
		if (!priv->timers[id].callback) break;
	}
	if (id >= priv->timers_size) {
		priv->timers_size = MIN ((priv->timers_size << 1), 4);
		priv->timers = realloc (priv->timers, priv->timers_size * sizeof (SPDocTimer));
		memset (priv->timers + id, 0, (priv->timers_size - id) * sizeof (SPDocTimer));
	}
	timer = &priv->timers[id];
	timer->time = time;
	timer->flags = flags;
	timer->awake = 1;
	timer->callback = callback;
	timer->data = data;
	if (flags == SP_TIMER_EXACT) {
		priv->etqueue = g_slist_insert_sorted (priv->etqueue, timer, sp_doc_timer_compare);
	} else {
		priv->itqueue = g_slist_prepend (priv->itqueue, timer);
	}
	return id;
}

/* Remove timer */
void
sp_document_remove_timer (SPDocument *doc, unsigned int id)
{
	SPDocumentPrivate *priv;
	SPDocTimer *timer;
	priv = doc->priv;
	timer = &priv->timers[id];
	if (timer->awake) {
		if (timer->flags == SP_TIMER_EXACT) {
			priv->etqueue = g_slist_remove (priv->etqueue, timer);
		} else {
			priv->itqueue = g_slist_remove (priv->itqueue, timer);
		}
	}
	timer->callback = NULL;
	timer->data = NULL;
}

/* Set up timer */
void
sp_document_set_timer (SPDocument *doc, unsigned int id, double time, unsigned int flags)
{
	SPDocumentPrivate *priv;
	SPDocTimer *timer;
	priv = doc->priv;
	timer = &priv->timers[id];
	/* Remove from queue */
	if (timer->awake) {
		if (timer->flags == SP_TIMER_EXACT) {
			priv->etqueue = g_slist_remove (priv->etqueue, timer);
		} else {
			priv->itqueue = g_slist_remove (priv->itqueue, timer);
		}
	}
	timer->time = time;
	timer->flags = flags;
	timer->awake = 1;
	if (flags == SP_TIMER_EXACT) {
		priv->etqueue = g_slist_insert_sorted (priv->etqueue, timer, sp_doc_timer_compare);
	} else {
		priv->itqueue = g_slist_prepend (priv->itqueue, timer);
	}
}

static void
sp_document_process_updates (SPDocument *doc)
{
	if (doc->root->uflags || doc->root->mflags) {
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
		/* if (doc->root->uflags & SP_OBJECT_MODIFIED_FLAG) return TRUE; */

		/* Emit "modified" signal on objects */
		sp_object_invoke_modified (doc->root, 0);

		/* Emit our own "modified" signal */
		g_signal_emit (G_OBJECT (doc), signals [MODIFIED], 0,
			       SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG);
	}
}

#define deltat 0.0

static void
sp_document_process_timers (SPDocument *doc)
{
	static int tsize = 0;
	static int tlen = 0;
	static SPDocTimer *timers = NULL;
	SPDocumentPrivate *priv;
	double ntime;
	GSList *l;
	int i;

	priv = doc->priv;

	ntime = sp_document_get_time (doc) + deltat;

	tlen = 0;
	while (priv->etqueue && (((SPDocTimer *) priv->etqueue->data)->time <= ntime)) {
		if (tlen >= tsize) {
			tsize = MAX (4, (tsize << 1));
			timers = (SPDocTimer *) realloc (timers, tsize * sizeof (SPDocTimer));
		}
		timers[tlen++] = *((SPDocTimer *) priv->etqueue->data);
		((SPDocTimer *) priv->etqueue->data)->awake = FALSE;
		priv->etqueue = g_slist_remove (priv->etqueue, priv->etqueue->data);
	}
	for (l = priv->itqueue; l != NULL; l = l->next) {
		if (tlen >= tsize) {
			tsize = MAX (4, (tsize << 1));
			timers = (SPDocTimer *) realloc (timers, tsize * sizeof (SPDocTimer));
		}
		timers[tlen++] = *((SPDocTimer *) l->data);
	}
	for (i = 0; i < tlen; i++) {
		timers[i].callback (ntime, timers[i].data);
	}
}

static int
sp_document_timeout_handler (void *data)
{
	SPDocument *doc;

	doc = SP_DOCUMENT (data);

	/* Process updates if needed */
	sp_document_process_updates (doc);
	/* Process timers */
	sp_document_process_timers (doc);

	sp_document_schedule_timers (doc);

	return FALSE;
}

static int
sp_document_idle_handler (void *data)
{
	SPDocument *doc;

	doc = SP_DOCUMENT (data);

	/* Process updates if needed */
	sp_document_process_updates (doc);
	/* Process timers */
	sp_document_process_timers (doc);

	sp_document_schedule_timers (doc);

	/* If idle not cleared we have to repeat */
	return doc->priv->idle != 0;
}

static void
sp_document_schedule_timers (SPDocument *doc)
{
	SPDocumentPrivate *priv;
	double ctime, dtime;
	unsigned int idle;
	priv = doc->priv;
	idle = FALSE;
	dtime = 1e18;
	if (doc->root && (doc->root->uflags || doc->root->mflags)) idle = TRUE;
	if (priv->itqueue) idle = TRUE;
	if (!idle && priv->etqueue) {
		SPDocTimer *timer;
		timer = (SPDocTimer *) priv->itqueue->data;
		/* Get current time */
		ctime = sp_document_get_time (doc);
		dtime = timer->time - ctime;
		if (dtime <= deltat) idle = TRUE;
	}
	/* We have to remove old timer always */
	if (priv->timeout) {
		/* Remove timer */
		gtk_timeout_remove (priv->timeout);
		priv->timeout = 0;
	}
	if (idle) {
		if (!priv->idle) {
			/* Register idle */
			priv->idle = gtk_idle_add_priority (SP_DOCUMENT_UPDATE_PRIORITY, sp_document_idle_handler, doc);
		}
	} else if (dtime < 1000 * 365.25 * 86400) {
		if (priv->idle) {
			/* Remove idle handler */
			gtk_idle_remove (priv->idle);
			priv->idle = 0;
		}
		/* Register timeout */
		priv->timeout = gtk_timeout_add ((int) dtime * 1000, sp_document_timeout_handler, doc);
	} else {
		if (priv->idle) {
			/* Remove idle handler */
			gtk_idle_remove (priv->idle);
			priv->idle = 0;
		}
	}
}


