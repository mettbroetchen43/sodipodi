#define SP_DOCUMENT_C

/*
 * SPDocument
 *
 * This is toplevel class, implementing a gateway from repr to most
 * editing properties, like canvases (desktops), undo stacks etc.
 *
 * Copyright (C) Lauris Kaplinski <lauris@ariman.ee> 1999-2000
 *
 */

#include <string.h>
#include <xml/repr.h>
#include "marshal.h"
#include "sodipodi-private.h"
#include "sp-object-repr.h"
#include "sp-root.h"
#include "sp-namedview.h"
#include "document-private.h"
#include "desktop.h"

#define A4_WIDTH  (21.0 * 72.0 / 2.54)
#define A4_HEIGHT (29.7 * 72.0 / 2.54)

enum {
	MODIFIED,
	URI_SET,
	RESIZED,
	LAST_SIGNAL
};

static void sp_document_class_init (SPDocumentClass * klass);
static void sp_document_init (SPDocument * document);
static void sp_document_destroy (GtkObject * object);

static gint sp_document_idle_handler (gpointer data);

gboolean sp_document_resource_list_free (gpointer key, gpointer value, gpointer data);

static GtkObjectClass * parent_class;
static guint signals[LAST_SIGNAL] = { 0 };
static gint doc_count = 0;

GtkType
sp_document_get_type (void)
{
	static GtkType document_type = 0;
	if (!document_type) {
		GtkTypeInfo document_info = {
			"SPDocument",
			sizeof (SPDocument),
			sizeof (SPDocumentClass),
			(GtkClassInitFunc) sp_document_class_init,
			(GtkObjectInitFunc) sp_document_init,
			NULL, NULL, NULL
		};
		document_type = gtk_type_unique (gtk_object_get_type (), &document_info);
	}
	return document_type;
}

static void
sp_document_class_init (SPDocumentClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (GTK_TYPE_OBJECT);

	signals[MODIFIED] = gtk_signal_new ("modified",
					    GTK_RUN_FIRST,
					    object_class->type,
					    GTK_SIGNAL_OFFSET (SPDocumentClass, modified),
					    gtk_marshal_NONE__UINT,
					    GTK_TYPE_NONE, 1, GTK_TYPE_UINT);
	signals[URI_SET] =  gtk_signal_new ("uri_set",
					    GTK_RUN_FIRST,
					    object_class->type,
					    GTK_SIGNAL_OFFSET (SPDocumentClass, uri_set),
					    gtk_marshal_NONE__STRING,
					    GTK_TYPE_NONE, 1, GTK_TYPE_STRING);
	signals[RESIZED] =  gtk_signal_new ("resized",
					    GTK_RUN_FIRST,
					    object_class->type,
					    GTK_SIGNAL_OFFSET (SPDocumentClass, resized),
					    sp_marshal_NONE__DOUBLE_DOUBLE,
					    GTK_TYPE_NONE, 2, GTK_TYPE_DOUBLE, GTK_TYPE_DOUBLE);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	object_class->destroy = sp_document_destroy;
}

static void
sp_document_init (SPDocument * document)
{
	SPDocumentPrivate * p;

	p = g_new (SPDocumentPrivate, 1);

	p->rdoc = NULL;
	p->rroot = NULL;

	p->root = NULL;

	p->iddef = g_hash_table_new (g_str_hash, g_str_equal);

	p->uri = NULL;
	p->base = NULL;

	p->aspect = SPXMidYMid;
	p->clip = FALSE;

#if 0
	p->namedviews = NULL;
	p->defs = NULL;
#endif

	p->resources = g_hash_table_new (g_str_hash, g_str_equal);

	p->sensitive = FALSE;
	p->undo = NULL;
	p->redo = NULL;
	p->actions = NULL;

	p->modified_id = 0;

	document->private = p;
}

static void
sp_document_destroy (GtkObject * object)
{
	SPDocument * document;
	SPDocumentPrivate * private;

	document = (SPDocument *) object;
	private = document->private;

	if (private) {
		sodipodi_remove_document (document);

		if (private->actions) {
			sp_action_free_list (document->private->actions);
		}

		sp_document_clear_redo (document);
		sp_document_clear_undo (document);

		if (private->base) g_free (private->base);

		if (private->uri) g_free (private->uri);

		if (private->root) gtk_object_unref (GTK_OBJECT (private->root));

		if (private->iddef) g_hash_table_destroy (private->iddef);

		if (private->rdoc) sp_repr_document_unref (private->rdoc);

		if (private->modified_id) gtk_idle_remove (private->modified_id);

		/* Free resources */
		g_hash_table_foreach_remove (private->resources, sp_document_resource_list_free, document);
		g_hash_table_destroy (private->resources);

		g_free (private);
		document->private = NULL;
	}

	sodipodi_unref ();

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

SPDocument *
sp_document_new (const gchar * uri)
{
	SPDocument * document;
	SPReprDoc * rdoc;
	SPRepr * rroot;
	SPObject * object;
	gchar * b;
	gint len;

	if (uri != NULL) {
		/* Try to fetch repr from file */
		rdoc = sp_repr_read_file (uri);
		/* If file cannot be loaded, return NULL without warning */
		if (rdoc == NULL) return NULL;
		rroot = sp_repr_document_root (rdoc);
		/* If xml file is not svg, return NULL without warning */
		/* fixme: destroy document */
		if (strcmp (sp_repr_name (rroot), "svg") != 0) return NULL;
	} else {
		rdoc = sp_repr_document_new ("svg");
		rroot = sp_repr_document_root (rdoc);
		g_return_val_if_fail (rroot != NULL, NULL);
		sp_repr_set_attr (rroot, "style",
			"fill:#000000;fill-opacity:0.5;stroke:none");
	}
	/* A quick hack to get namespaces into doc */
	sp_repr_set_attr (rroot, "xmlns", "http://www.w3.org/2000/svg");
	sp_repr_set_attr (rroot, "xmlns:sodipodi", "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd");
	sp_repr_set_attr (rroot, "xmlns:xlink", "http://www.w3.org/1999/xlink");
	/* End of quick hack */
	/* Quick hack 2 - get default image size into document */
	if (!sp_repr_attr (rroot, "width")) sp_repr_set_double_attribute (rroot, "width", A4_WIDTH);
	if (!sp_repr_attr (rroot, "height")) sp_repr_set_double_attribute (rroot, "height", A4_HEIGHT);
	/* End of quick hack 2 */

	document = gtk_type_new (SP_TYPE_DOCUMENT);
	g_return_val_if_fail (document != NULL, NULL);

	document->private->rdoc = rdoc;
	document->private->rroot = rroot;

	if (uri != NULL) {
		b = g_strdup (uri);
		g_return_val_if_fail (b != NULL, NULL);
		document->private->uri = b;
		b = g_dirname (uri);
		g_return_val_if_fail (b != NULL, NULL);
		len = strlen (b) + 2;
		document->private->base = g_malloc (len);
		g_snprintf (document->private->base, len, "%s/", b);
		g_free (b);
		sp_repr_set_attr (rroot, "sodipodi:docname", uri);
		sp_repr_set_attr (rroot, "sodipodi:docbase", document->private->base);
	} else {
		document->private->uri = g_strdup_printf ("Unnamed document %d", ++doc_count);
	}

	object = sp_object_repr_build_tree (document, rroot);
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_ROOT (object), NULL);

	document->private->root = SP_ROOT (object);

	/* Namedviews */
	if (!document->private->root->namedviews) {
		SPRepr * r;
		r = sp_repr_new ("sodipodi:namedview");
		sp_repr_set_attr (r, "id", "base");
		sp_repr_add_child (rroot, r, NULL);
		sp_repr_unref (r);
		g_assert (document->private->root->namedviews);
	}

	/* Defs */
	if (!document->private->root->defs) {
		SPRepr *r;
		r = sp_repr_new ("defs");
		sp_repr_add_child (rroot, r, NULL);
		sp_repr_unref (r);
		g_assert (document->private->root->defs);
	}

	sodipodi_ref ();

	sp_document_set_undo_sensitive (document, TRUE);

	sodipodi_add_document (document);

	return document;
}

SPDocument *
sp_document_new_from_mem (const gchar * buffer, gint length)
{
	SPDocument * document;
	SPReprDoc * rdoc;
	SPRepr * rroot;
	SPObject * object;

	rdoc = sp_repr_read_mem (buffer, length);

	/* If it cannot be loaded, return NULL without warning */
	if (rdoc == NULL) return NULL;

	rroot = sp_repr_document_root (rdoc);

	/* If xml file is not svg, return NULL without warning */
	/* fixme: destroy document */
	if (strcmp (sp_repr_name (rroot), "svg") != 0) return NULL;

	/* A quick hack to get namespaces into doc */
	sp_repr_set_attr (rroot, "xmlns", "http://www.w3.org/2000.svg");
	sp_repr_set_attr (rroot, "xmlns:sodipodi", "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd");
	sp_repr_set_attr (rroot, "xmlns:xlink", "http://www.w3.org/1999/xlink");
	/* End of quick hack */
	/* Quick hack 2 - get default image size into document */
	if (!sp_repr_attr (rroot, "width")) sp_repr_set_double_attribute (rroot, "width", A4_WIDTH);
	if (!sp_repr_attr (rroot, "height")) sp_repr_set_double_attribute (rroot, "height", A4_HEIGHT);
	/* End of quick hack 2 */

	document = gtk_type_new (SP_TYPE_DOCUMENT);
	g_return_val_if_fail (document != NULL, NULL);

	document->private->rdoc = rdoc;
	document->private->rroot = rroot;

	document->private->uri = g_strdup_printf ("Memory document %d", ++doc_count);

	object = sp_object_repr_build_tree (document, rroot);
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_ROOT (object), NULL);

	document->private->root = SP_ROOT (object);

	/* fixme: docbase */

	/* Namedviews */
	if (!document->private->root->namedviews) {
		SPRepr * r;
		r = sp_repr_new ("sodipodi:namedview");
		sp_repr_set_attr (r, "id", "base");
		sp_repr_add_child (rroot, r, 0);
		sp_repr_unref (r);
		g_assert (document->private->root->namedviews);
	}

	/* Defs */
	if (!document->private->root->defs) {
		SPRepr *r;
		r = sp_repr_new ("defs");
		sp_repr_add_child (rroot, r, NULL);
		sp_repr_unref (r);
		g_assert (document->private->root->defs);
	}

	sodipodi_ref ();

	sp_document_set_undo_sensitive (document, TRUE);

	sodipodi_add_document (document);

	return document;
}

SPDocument *
sp_document_ref (SPDocument *doc)
{
	g_return_val_if_fail (doc != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (doc), NULL);

	gtk_object_ref (GTK_OBJECT (doc));

	return doc;
}

SPDocument *
sp_document_unref (SPDocument *doc)
{
	g_return_val_if_fail (doc != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (doc), NULL);

	gtk_object_unref (GTK_OBJECT (doc));

	return NULL;
}

SPReprDoc *
sp_document_repr_doc (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (document->private != NULL, NULL);

	return document->private->rdoc;
}

SPRepr *
sp_document_repr_root (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (document->private != NULL, NULL);

	return document->private->rroot;
}

SPRoot *
sp_document_root (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (document->private != NULL, NULL);

	return document->private->root;
}

gdouble
sp_document_width (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, 0.0);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), 0.0);
	g_return_val_if_fail (document->private != NULL, 0.0);

	return document->private->root->width;
}

gdouble
sp_document_height (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, 0.0);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), 0.0);
	g_return_val_if_fail (document->private != NULL, 0.0);

	return document->private->root->height;
}

const gchar *
sp_document_uri (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (document->private != NULL, NULL);

	return document->private->uri;
}

void
sp_document_set_uri (SPDocument *document, const guchar *uri)
{
	g_return_if_fail (document != NULL);
	g_return_if_fail (SP_IS_DOCUMENT (document));

	if (document->private->uri) {
		g_free (document->private->uri);
		document->private->uri = NULL;
	}

	if (uri) {
		document->private->uri = g_strdup (uri);
	}

	gtk_signal_emit (GTK_OBJECT (document), signals [URI_SET], document->private->uri);
}

void
sp_document_set_size (SPDocument *doc, gdouble width, gdouble height)
{
	g_return_if_fail (doc != NULL);
	g_return_if_fail (SP_IS_DOCUMENT (doc));
	g_return_if_fail (width > 0.001);
	g_return_if_fail (height > 0.001);

	gtk_signal_emit (GTK_OBJECT (doc), signals [RESIZED], width, height);
}

const gchar *
sp_document_base (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (document->private != NULL, NULL);

	return document->private->base;
}

/* named views */

const GSList *
sp_document_namedview_list (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);

	return document->private->root->namedviews;
}

SPNamedView *
sp_document_namedview (SPDocument * document, const gchar * id)
{
	const GSList * nvl, * l;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);

	nvl = document->private->root->namedviews;
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
sp_document_lookup_id (SPDocument * document, const gchar * id)
{
	return g_hash_table_lookup (document->private->iddef, id);
}

/* Object modification root handler */

void
sp_document_request_modified (SPDocument *document)
{
	if (!document->private->modified_id) {
		document->private->modified_id = gtk_idle_add (sp_document_idle_handler, document);
	}
}

static gint
sp_document_idle_handler (gpointer data)
{
	SPDocument *document;

	document = SP_DOCUMENT (data);

	document->private->modified_id = 0;

	/* Emit "modified" signal on objects */
	sp_object_modified (SP_OBJECT (document->private->root), 0);

	/* Emit our own "modified" signal */
	gtk_signal_emit (GTK_OBJECT (document), signals [MODIFIED],
			 SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG);

	return FALSE;
}

SPObject *
sp_document_add_repr (SPDocument *document, SPRepr *repr)
{
	GtkType type;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (repr != NULL, NULL);

	type = sp_repr_type_lookup (repr);

	if (gtk_type_is_a (type, SP_TYPE_ITEM)) {
		sp_repr_append_child (document->private->rroot, repr);
	} else if (gtk_type_is_a (type, SP_TYPE_OBJECT)) {
		sp_repr_append_child (SP_OBJECT_REPR (document->private->root->defs), repr);
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
sp_document_items_in_box (SPDocument * document, ArtDRect * box)
{
	SPGroup * group;
	SPItem * child;
	SPObject * o;
	ArtDRect b;
	GSList * s;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (document->private != NULL, NULL);
	g_return_val_if_fail (box != NULL, NULL);

	group = SP_GROUP (document->private->root);

	s = NULL;

	for (o = group->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			sp_item_bbox (child, &b);
			if ((b.x0 > box->x0) && (b.x1 < box->x1) &&
			    (b.y0 > box->y0) && (b.y1 < box->y1)) {
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
