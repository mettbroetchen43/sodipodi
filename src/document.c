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

#include <xml/repr.h>
#include "sodipodi-private.h"
#include "sp-object-repr.h"
#include "sp-root.h"
#include "document-private.h"

#define A4_WIDTH  (21.0 * 72.0 / 2.54)
#define A4_HEIGHT (29.7 * 72.0 / 2.54)

static void sp_document_class_init (SPDocumentClass * klass);
static void sp_document_init (SPDocument * document);
static void sp_document_destroy (GtkObject * object);

static GtkObjectClass * parent_class;

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
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
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

	parent_class = gtk_type_class (gtk_object_get_type ());

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

	p->namedviews = NULL;

	p->sensitive = TRUE;
	p->undo = NULL;
	p->redo = NULL;
	p->actions = NULL;

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
		while (private->actions) {
			sp_repr_unref ((SPRepr *) document->private->actions->data);
			private->actions = g_list_remove_link (private->actions, private->actions);
		}

		sp_document_clear_redo (document);
		sp_document_clear_undo (document);

		if (private->base) g_free (private->base);

		if (private->uri) g_free (private->uri);

		if (private->root) gtk_object_unref (GTK_OBJECT (private->root));

		if (private->iddef) g_hash_table_destroy (private->iddef);

		if (private->rdoc) sp_repr_document_unref (private->rdoc);

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
			"fill:#000000;fill-opacity:50%;stroke:none");
	}
	/* A quick hack to get namespaces into doc */
	sp_repr_set_attr (rroot, "xmlns", "http://www.w3.org/Graphics/SVG/SVG-19991203.dtd");
	sp_repr_set_attr (rroot, "xmlns:sodipodi", "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd");
	/* End of quick hack */

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
		sp_repr_add_child (rroot, r, 0);
		g_assert (document->private->root->namedviews);
	}

	sodipodi_ref ();

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
	sp_repr_set_attr (rroot, "xmlns", "http://www.w3.org/Graphics/SVG/SVG-19991203.dtd");
	sp_repr_set_attr (rroot, "xmlns:sodipodi", "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd");
	/* End of quick hack */

	document = gtk_type_new (SP_TYPE_DOCUMENT);
	g_return_val_if_fail (document != NULL, NULL);

	document->private->rdoc = rdoc;
	document->private->rroot = rroot;

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
		g_assert (document->private->root->namedviews);
	}

	sodipodi_ref ();

	return document;
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
	gdouble width;

	g_return_val_if_fail (document != NULL, 0.0);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), 0.0);
	g_return_val_if_fail (document->private != NULL, 0.0);

	gtk_object_get (GTK_OBJECT (document->private->root), "width", &width, NULL);

	return width;
}

gdouble
sp_document_height (SPDocument * document)
{
	gdouble height;

	g_return_val_if_fail (document != NULL, 0.0);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), 0.0);
	g_return_val_if_fail (document->private != NULL, 0.0);

	gtk_object_get (GTK_OBJECT (document->private->root), "height", &height, NULL);

	return height;
}

const gchar *
sp_document_uri (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (document->private != NULL, NULL);

	return document->private->uri;
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
	const GSList * l;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);

	gtk_object_get (GTK_OBJECT (document->private->root), "namedviews", &l, NULL);
	g_assert (l != NULL);

	return l;
}

SPNamedView *
sp_document_namedview (SPDocument * document, const gchar * id)
{
	const GSList * nvl, * l;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);

	gtk_object_get (GTK_OBJECT (document->private->root), "namedviews", &nvl, NULL);
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
	ArtDRect b;
	GSList * s, * l;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (document->private != NULL, NULL);
	g_return_val_if_fail (box != NULL, NULL);

	group = SP_GROUP (document->private->root);

	s = NULL;

	for (l = group->children; l != NULL; l = l->next) {
		child = SP_ITEM (l->data);
		sp_item_bbox (child, &b);
		if ((b.x0 > box->x0) && (b.x1 < box->x1) &&
		    (b.y0 > box->y0) && (b.y1 < box->y1)) {
			s = g_slist_append (s, child);
		}
	}

	return s;
}

